// NatTraversalClientPeer.cpp: implements for the NatTraversalClientPeer class.
//
//////////////////////////////////////////////////////////////////////

#include "NatCommon.h"
#include "NatTraversalClient.h"
#include "NatTraversalClientPeer.h"
#include "NatClientPeerManager.h"
#include "NatDebug.h"

////////////////////////////////////////////////////////////////////////////////
// class CNatP2PClientPeer implements

CNatP2PClientPeer::CNatP2PClientPeer()
:
m_state(TRAVERSAL_STATE_NONE),
m_config(),
m_pUdtTransport(NULL),
m_traversalClient(),
m_p2pUdt(NULL),
m_p2pUdtConfig(),
m_timeoutMs(TRAVERSAL_P2P_TIMEOUT),
m_startTimeMs(0),
m_error(NAT_CLI_OK),
m_isP2PErrorOnly(false)
{

}

CNatP2PClientPeer::~CNatP2PClientPeer()
{
    if (m_traversalClient.IsStarted())
    {
        m_traversalClient.Stop();
    }
    if (m_p2pUdt != NULL)
    {
        if (m_p2pUdt->IsStarted())
        {
            m_p2pUdt->Stop();
        }
        delete m_p2pUdt;
        m_p2pUdt = NULL;
    }
    if (m_pUdtTransport != NULL)
    {
        if (m_pUdtTransport->IsStarted())
        {
            m_pUdtTransport->Stop();
        }
        delete m_pUdtTransport;
        m_pUdtTransport = NULL;
    }

}

void CNatP2PClientPeer::SetTraversalTimeout(unsigned long timeout)
{
    m_timeoutMs = timeout;
}

bool CNatP2PClientPeer::Initilize(const NAT_CLIENT_TRAVERSAL_CONFIG* config, unsigned long natServerIp )
{
    int ret = 0;
    m_config = *config;

    NAT_UDT_TRANSPORT_CONFIG udtTransportConfig;
    udtTransportConfig.localIp = 0;
    udtTransportConfig.localPort = 0;
    m_connectionId = Nat_GetTickCount();
    NAT_LOG_DEBUG("CNatP2PClientPeer traversal ConnectionId=%d\n", m_connectionId);

    m_pUdtTransport = new CNatUdtTransport();
    if (m_pUdtTransport == NULL)
    {
        NAT_LOG_WARN("CNatP2PClientPeer create udt transport failed!\n");
        return false;
    }
    m_pUdtTransport->SetNotifier(this);
    ret = m_pUdtTransport->Start(&udtTransportConfig);
    if (ret != 0)
    {
        NAT_LOG_WARN("CNatP2PClientPeer start udt transport failed!\n");
        return false;
    }

    if (!m_traversalClient.Start(natServerIp, m_config.nServerPort, m_pUdtTransport))
    {
        NAT_LOG_WARN("CNatP2PClientPeer Start traversal client failed!\n");
        return false;
    }
    m_traversalClient.SetNotifier(this);
    m_startTimeMs = Nat_GetTickCount();
    m_state = TRAVERSAL_STATE_SER_CONNECTING;
	m_error = NAT_CLI_OK;
    return true;
}

NatTaskResult CNatP2PClientPeer::RunTask(unsigned long curTickCount)
{

    NatTaskResult taskResult = NAT_TASK_IDLE;
    NatRunResult runResult = RUN_NONE;

	if (m_error != NAT_CLI_OK || m_state == TRAVERSAL_STATE_DEV_CONNECTED)
	{
		return NAT_TASK_DONE;
	}

    runResult = m_pUdtTransport->Run();
    if (Nat_CheckTaskDoneFrom(taskResult, runResult))
    {
        NAT_LOG_DEBUG("CNatP2PClientPeer udt transport run failed!\n");
        SetError(NAT_CLI_ERR_NETWORK);
        return NAT_TASK_DONE;
    }

    if (m_error == NAT_CLI_OK && (TRAVERSAL_STATE_SER_CONNECTING == m_state || TRAVERSAL_STATE_SER_REQUSTING == m_state))
    {
        if(m_traversalClient.IsStarted())
        {
            runResult = m_traversalClient.Run(Nat_GetTickCount());
            if (Nat_CheckTaskDoneFrom(taskResult, runResult))
            {
                NAT_LOG_DEBUG("CNatP2PClientPeer traversal client run failed!\n");
                SetError(NAT_CLI_ERR_NETWORK);
                return NAT_TASK_DONE;
            } 
        }
    }
    else
    {
        // 及早关闭与NAT服务器的连接。
        if (m_traversalClient.IsStarted())
        {
            NAT_LOG_DEBUG("CNatP2PClientPeer stop the connection to NatServer as soon as possible!\n");
            m_traversalClient.Stop();
        }
    }

    if (m_error == NAT_CLI_OK && m_state == TRAVERSAL_STATE_DEV_CONNECTING)
    {
        assert(m_p2pUdt != NULL);
        runResult = NatRunResultMax(runResult, m_p2pUdt->Run());
        if (Nat_CheckTaskDoneFrom(taskResult, runResult))
        {
            NAT_LOG_DEBUG("CNatP2PClientPeer p2p udt run failed!\n");
            SetP2PFailed();
            return NAT_TASK_DONE;
        } 
    }

    if (m_error == NAT_CLI_OK && m_state != TRAVERSAL_STATE_DEV_CONNECTED)
    {
        if (PUB_IsTimeOutEx(m_startTimeMs, m_timeoutMs, curTickCount))
        {
            if (m_state == TRAVERSAL_STATE_SER_CONNECTING || m_state == TRAVERSAL_STATE_SER_REQUSTING)
            {
                NAT_LOG_DEBUG("CNatP2PClientPeer traversal timeout failed!\n");
                SetError(NAT_CLI_ERR_TIMEOUT);
            }
            else
            {
                NAT_LOG_DEBUG("CNatP2PClientPeer p2p connect timeout failed!\n");
                SetP2PFailed();
            }
            return NAT_TASK_DONE;
        }
    }

    if (m_error != NAT_CLI_OK || m_state == TRAVERSAL_STATE_DEV_CONNECTED)
    {
        return NAT_TASK_DONE;
    }
    return taskResult;
}

NatSocket CNatP2PClientPeer::HandleCompleteResult()
{
    assert(m_state == TRAVERSAL_STATE_DEV_CONNECTED);

    NatSocket sockHandle = NAT_SOCKET_INVALID;

    m_p2pUdt->SetNotifier(NULL);
    m_pUdtTransport->SetNotifier(NULL); 
    CNatClientUdtSocket *udtSocket = new CNatClientUdtSocket(m_p2pUdt, m_pUdtTransport);
    if(NULL != udtSocket)
    {
        sockHandle = CNatClientPeerManager::instance()->AddConnectedSocket(udtSocket);
        if (sockHandle != NAT_SOCKET_INVALID)
        {
            m_p2pUdt = NULL;
            m_pUdtTransport = NULL;
        }
        else
        {
            delete udtSocket;
        }
    }
    m_state = TRAVERSAL_STATE_NONE;
    return sockHandle;
};

NAT_CLIENT_STATE CNatP2PClientPeer::GetClientState()
{
	NAT_CLIENT_STATE clientState = NAT_CLI_STATE_INIT;
	switch (m_state)
	{
	case TRAVERSAL_STATE_NONE:
		clientState = NAT_CLI_STATE_INIT;
		break;
	case TRAVERSAL_STATE_SER_CONNECTING:
	case TRAVERSAL_STATE_SER_REQUSTING:
		clientState = NAT_CLI_STATE_P2P_SERVER;
		break;
	case TRAVERSAL_STATE_DEV_CONNECTING:
	case TRAVERSAL_STATE_DEV_CONNECTED:
		clientState = NAT_CLI_STATE_P2P_DEVICE;
		break;
	}
	return clientState;
}

void CNatP2PClientPeer::OnRecvDatagram( CNatUdtTransport *transport, const NAT_TRANSPORT_RECV_DATAGRAM* datagram )
{
    assert(m_state != TRAVERSAL_STATE_NONE);
    unsigned char category = CNatUdt::GetDatagramCategory(datagram);
    if(NAT_UDT_CATEGORY_P2NS == category)
    {
        if (m_state == TRAVERSAL_STATE_SER_CONNECTING || m_state == TRAVERSAL_STATE_SER_REQUSTING)
        {
            m_traversalClient.NotifyRecvDatagram(datagram);
        }
    }
    else
    {
        if(m_state == TRAVERSAL_STATE_DEV_CONNECTING 
            || m_state == TRAVERSAL_STATE_DEV_CONNECTED )
        {
            assert(m_p2pUdt != NULL);
            if ( m_p2pUdt->IsMineDatagram(datagram))
            {
                if (m_p2pUdt->NotifyRecvDatagram(datagram) < 0)
                {
                    SetP2PFailed();
                }
            }
            else if (m_state == TRAVERSAL_STATE_DEV_CONNECTING 
                && CNatUdt::IsDatagramSynCmd(datagram)
                && (m_p2pUdt->GetRemoteIp() == datagram->fromIp)
                && (m_p2pUdt->GetRemotePort() != datagram->fromPort) /* 只是端口号不同 */
                && (m_p2pUdt->GetConnectionID() == CNatUdt::GetDatagramConnectionId(datagram)))
            {
                //对方可能是对称型NAT网络，尝试重置端口发起新的连接进行尝试。
                NAT_LOG_INFO("The remote device is Symmetric NAT! device=%s:%d\n",Nat_inet_ntoa(datagram->fromIp), datagram->fromPort);				 
                if(ResetP2pConnect(datagram->fromPort))
                {
                    if (m_p2pUdt->NotifyRecvDatagram(datagram) < 0)
                    {
                        SetP2PFailed();
                    }
                }
            }
            else
            {
                // ignore...
            }
        }
        else
        {
            // ignore...
        }

    }
}

void CNatP2PClientPeer::OnConnect( CNatUdt *pUDT, int iErrorCode )
{
    if (m_state == TRAVERSAL_STATE_DEV_CONNECTING)
    {
        m_state = TRAVERSAL_STATE_DEV_CONNECTED;
    }
}

int CNatP2PClientPeer::OnRecv( CNatUdt *pUDT, const void* pBuf, int iLen )
{
    // ignore
    return 0;
}

int CNatP2PClientPeer::OnSendPacket( CNatUdt *pUDT, const void* pBuf, int iLen )
{
    return m_pUdtTransport->Send(m_p2pUdt->GetRemoteIp(), m_p2pUdt->GetRemotePort(),
        pBuf, iLen);
}

void CNatP2PClientPeer::OnServerConnect()
{
    if (m_state == TRAVERSAL_STATE_SER_CONNECTING)
    {
        assert(!m_traversalClient.IsRequesting());

        NAT_CLIENT_CONNECT_P2P_REQ p2pReq;

        memset(&p2pReq,0,sizeof(NAT_CLIENT_CONNECT_P2P_REQ));
        p2pReq.stHeader.dwRequestSeq		= m_traversalClient.GenNextReqSeq();
        p2pReq.ucRequestPeerNat			= 0; //保留，暂时未使用
        strcpy(p2pReq.dwDeviceNo, m_config.dwDeviceNo);
        p2pReq.dwConnectionId			    = m_connectionId;
        p2pReq.usP2PVersion.sVersion.ucHighVer   = P2P_VERSION_HIGH;
        p2pReq.usP2PVersion.sVersion.ucLowVer    = P2P_VERSION_LOW;
        m_traversalClient.StartRequest(NAT_ID_CLIENT_CONNECT_P2P_REQ, &p2pReq, 
            sizeof(NAT_CLIENT_CONNECT_P2P_REQ));
        m_state = TRAVERSAL_STATE_SER_REQUSTING;
    }
}

void CNatP2PClientPeer::OnReply( NatTraversalCmdId reqId, const void* req, 
    bool succeeded, NatTraversalCmdId replyId, void* reply )
{
    if (m_state == TRAVERSAL_STATE_SER_REQUSTING)
    {
        assert(reqId == NAT_ID_CLIENT_CONNECT_P2P_REQ);
        if (succeeded)
        {
            assert(replyId == NAT_ID_CLIENT_CONNECT_P2P_REPLY);
            HandleP2pConnectReply((NAT_CLIENT_CONNECT_P2P_REPLY*)reply);
        }
        else
        {
            SetError(NAT_CLI_ERR_TIMEOUT);
        }
    }
}

bool CNatP2PClientPeer::HandleP2pConnectReply(NAT_CLIENT_CONNECT_P2P_REPLY *p2pReply)
{
    assert(m_state == TRAVERSAL_STATE_SER_REQUSTING
        && m_p2pUdt == NULL);

    if (p2pReply->stHeader.dwStatus != REPLY_STATUS_OK)
    {
        NAT_LOG_DEBUG("CNatP2PClientPeer recv p2p connect reply Failed: error=%s\n", 
            GetTraversalReplyStatusName(p2pReply->stHeader.dwStatus));
        SetError(Nat_TraslateClientError(p2pReply->stHeader.dwStatus));
        return false;
    }

    NAT_LOG_DEBUG("CNatP2PClientPeer recv p2p connect reply OK: DeviceAddr=(%s:%d)\n", 
        Nat_inet_ntoa(p2pReply->dwDevicePeerIp), p2pReply->usDevicePeerPort);

    m_p2pUdt = new CNatUdt();
    if (m_p2pUdt == NULL)
    {
        NAT_LOG_WARN("CNatTraversalClientPeer create p2p CNatUdt failed!\n");
        SetError(NAT_CLI_ERR_SYS);
        return false;
    }
    memset(&m_p2pUdtConfig, 0, sizeof(NAT_UDT_CONFIG));
    m_p2pUdtConfig.category = NAT_UDT_CATEGORY_P2P;
    m_p2pUdtConfig.remoteIp = p2pReply->dwDevicePeerIp;
    m_p2pUdtConfig.remotePort = p2pReply->usDevicePeerPort;
    m_p2pUdtConfig.connectionID = m_connectionId;
    m_p2pUdtConfig.maxRecvWindowSize = UDT_RECV_WINDOW_SIZE_MAX;
    m_p2pUdtConfig.pRecvHeapDataManeger = NULL;
    m_p2pUdtConfig.maxSendWindowSize = UDT_SEND_WINDOW_SIZE_MAX;
    m_p2pUdtConfig.pSendHeapDataManeger = NULL;
    if(!m_p2pUdt->Start(&m_p2pUdtConfig))
    {
        NAT_LOG_WARN("CNatP2PClientPeer start p2p udt failed!\n");
        return false;
    }
    m_p2pUdt->SetNotifier(this);
    m_state = TRAVERSAL_STATE_DEV_CONNECTING;
    return true;
}

bool CNatP2PClientPeer::ResetP2pConnect(unsigned short newPort)
{
    assert(m_state == TRAVERSAL_STATE_DEV_CONNECTING
        && m_p2pUdt != NULL && m_p2pUdt->IsStarted());
    m_p2pUdt->SetNotifier(NULL);
    m_p2pUdt->Stop();

    m_p2pUdtConfig.remotePort = newPort;
    if(!m_p2pUdt->Start(&m_p2pUdtConfig))
    {
        NAT_LOG_WARN("CNatP2PClientPeer reset p2p udt failed!\n");
        return false;
    }
    m_p2pUdt->SetNotifier(this);
    return true;
}

void CNatP2PClientPeer::SetP2PFailed()
{
    if (m_error == NAT_CLI_OK)
    {
        m_error = NAT_CLI_ERR_UNKNOWN;
        m_isP2PErrorOnly = true;
    }
}

void CNatP2PClientPeer::SetError( NAT_CLIENT_ERROR error )
{
    assert(error != NAT_CLI_OK);
    if (m_error == NAT_CLI_OK && m_state != TRAVERSAL_STATE_DEV_CONNECTED)
    {
        m_error = error;
        m_isP2PErrorOnly = false;
    }
}


////////////////////////////////////////////////////////////////////////////////
//class CNatRelayClientPeer implements

CNatRelayClientPeer::CNatRelayClientPeer()
:
m_state(TRAVERSAL_STATE_NONE),
m_config(),
m_timeoutMs(TRAVERSAL_RELAY_TIMEOUT),
m_startTimeMs(0),
m_pUdtTransport(NULL),
m_pRelaySock(NULL),
m_traversalClient(),
m_error(NAT_CLI_OK)
{

}

CNatRelayClientPeer::~CNatRelayClientPeer()
{

    if (m_traversalClient.IsStarted())
    {
        m_traversalClient.Stop();
    }
    if (m_pRelaySock != NULL)
    {
        delete m_pRelaySock;
        m_pRelaySock = NULL;
    }
    if (m_pUdtTransport != NULL)
    {
        if (m_pUdtTransport->IsStarted())
        {
            m_pUdtTransport->Stop();
        }
        delete m_pUdtTransport;
        m_pUdtTransport = NULL;
    }
}

void CNatRelayClientPeer::SetTraversalTimeout(unsigned long timeout)
{
    m_timeoutMs = timeout;
}

bool CNatRelayClientPeer::Initilize( const NAT_CLIENT_TRAVERSAL_CONFIG* config, unsigned long natServerIp )
{
    int ret = 0;
    m_config = *config;

    NAT_UDT_TRANSPORT_CONFIG udtTransportConfig;
    udtTransportConfig.localIp = 0;
    udtTransportConfig.localPort = 0;

    m_pUdtTransport = new CNatUdtTransport();
    if (m_pUdtTransport == NULL)
    {
        NAT_LOG_WARN("CNatRelayClientPeer create udt transport failed!\n");
        return false;
    }
    m_pUdtTransport->SetNotifier(this);
    ret = m_pUdtTransport->Start(&udtTransportConfig);
    if (ret != 0)
    {
        NAT_LOG_WARN("CNatRelayClientPeer start udt transport failed!\n");
        return false;
    }

    if (!m_traversalClient.Start(natServerIp, m_config.nServerPort, m_pUdtTransport))
    {
        NAT_LOG_WARN("CNatRelayClientPeer start traversal client failed!\n");
        return false;
    }
    m_traversalClient.SetNotifier(this);
    m_startTimeMs = Nat_GetTickCount();
    m_state = TRAVERSAL_STATE_SER_CONNECTING;
    return true;
}

NatTaskResult CNatRelayClientPeer::RunTask( unsigned long curTickCount )
{
    NatTaskResult taskResult = NAT_TASK_IDLE;
    NatRunResult runResult  = RUN_NONE;

	if (m_error != NAT_CLI_OK || m_state == TRAVERSAL_STATE_DEV_CONNECTED)
	{
		return NAT_TASK_DONE;
	}

    runResult = m_pUdtTransport->Run();
    if (Nat_CheckTaskDoneFrom(taskResult, runResult))
    {
        NAT_LOG_DEBUG("CNatRelayClientPeer udt transport run failed!\n");
        SetError(NAT_CLI_ERR_NETWORK);
        return NAT_TASK_DONE;
    }


    if (m_error == NAT_CLI_OK && (TRAVERSAL_STATE_SER_CONNECTING == m_state || TRAVERSAL_STATE_SER_REQUSTING == m_state))
    {
        if(m_traversalClient.IsStarted())
        {
            runResult = m_traversalClient.Run(Nat_GetTickCount());
            if (Nat_CheckTaskDoneFrom(taskResult, runResult))
            {
                NAT_LOG_DEBUG("CNatRelayClientPeer traversal client run failed!\n");
                SetError(NAT_CLI_ERR_NETWORK);
                return NAT_TASK_DONE;
            }
        }
    }
    else
    {
        // 及早关闭与NAT服务器的连接。
        if (m_traversalClient.IsStarted())
        {
            NAT_LOG_DEBUG("CNatRelayClientPeer stop the connection to NatServer as soon as possible!\n");
            m_traversalClient.Stop();
        }
    }

    if (m_error == NAT_CLI_OK && m_state == TRAVERSAL_STATE_DEV_CONNECTING)
    {
        assert(m_pRelaySock != NULL);
        runResult = m_pRelaySock->Run();
        if (Nat_CheckTaskDoneFrom(taskResult, runResult))
        {
            NAT_LOG_DEBUG("CNatRelayClientPeer run RelaySocket failed!\n");
            SetError(NAT_CLI_ERR_NETWORK);
            return NAT_TASK_DONE;
        }
    }
    if (m_error == NAT_CLI_OK && m_state != TRAVERSAL_STATE_DEV_CONNECTED)
    {
        if (PUB_IsTimeOutEx(m_startTimeMs, m_timeoutMs, curTickCount))
        {
            NAT_LOG_DEBUG("CNatRelayClientPeer traversal timeout failed!\n");
            SetError(NAT_CLI_ERR_TIMEOUT);
            return NAT_TASK_DONE;
        }
    }

    if (m_error != NAT_CLI_OK || m_state == TRAVERSAL_STATE_DEV_CONNECTED)
    {
        return NAT_TASK_DONE;
    }

    return taskResult;
}

NatSocket CNatRelayClientPeer::HandleCompleteResult()
{
    assert(m_state == TRAVERSAL_STATE_DEV_CONNECTED);
    NatSocket sockHandle = NAT_SOCKET_INVALID;
    m_pRelaySock->SetCommNotifier(NULL);
    sockHandle = CNatClientPeerManager::instance()->AddConnectedSocket(m_pRelaySock);
    if (sockHandle != NAT_SOCKET_INVALID)
    {
        m_pRelaySock = NULL;
    }
    m_state = TRAVERSAL_STATE_NONE;
    return sockHandle;
}

NAT_CLIENT_STATE CNatRelayClientPeer::GetClientState()
{
	NAT_CLIENT_STATE clientState = NAT_CLI_STATE_INIT;
	switch (m_state)
	{
	case TRAVERSAL_STATE_NONE:
		clientState = NAT_CLI_STATE_INIT;
		break;
	case TRAVERSAL_STATE_SER_CONNECTING:
	case TRAVERSAL_STATE_SER_REQUSTING:
		clientState = NAT_CLI_STATE_RELAY_SERVER;
		break;
	case TRAVERSAL_STATE_DEV_CONNECTING:
	case TRAVERSAL_STATE_DEV_CONNECTED:
		clientState = NAT_CLI_STATE_RELAY_DEVICE;
		break;
	}
	return clientState;
}

void CNatRelayClientPeer::OnRecvDatagram( CNatUdtTransport *transport, 
    const NAT_TRANSPORT_RECV_DATAGRAM* datagram )
{
    unsigned char category = CNatUdt::GetDatagramCategory(datagram);
    if(NAT_UDT_CATEGORY_P2NS == category 
        && (TRAVERSAL_STATE_SER_CONNECTING == m_state || TRAVERSAL_STATE_SER_REQUSTING == m_state))
    {
        m_traversalClient.NotifyRecvDatagram(datagram);     
    }
}

void CNatRelayClientPeer::RelayOnConnect( CNatSocketBase *pSockBase, int iErrorCode )
{
    //完成与NatServer的中继请求，关闭该udt连接,减少对服务器的压力，但不删除UDT对象
    if (m_state == TRAVERSAL_STATE_DEV_CONNECTING)
    {
        if (0 == iErrorCode)
        {
            NAT_LOG_DEBUG("CNatRelayClientPeer relay connect succeeded!\n");
            m_state = TRAVERSAL_STATE_DEV_CONNECTED;
        }
        else
        {
            NAT_LOG_DEBUG("CNatRelayClientPeer relay connect failed!\n");
            SetError(NAT_CLI_ERR_NETWORK);
        }
    }
}

void CNatRelayClientPeer::OnServerConnect()
{
    if (m_state == TRAVERSAL_STATE_SER_CONNECTING)
    {
        assert(!m_traversalClient.IsRequesting());
        NAT_CLIENT_CONNECT_RELAY_REQ  relayReq;
        memset(&relayReq,0,sizeof(NAT_CLIENT_CONNECT_RELAY_REQ));
        relayReq.stHeader.dwRequestSeq	= m_traversalClient.GenNextReqSeq();
        strcpy(relayReq.dwDeviceNo, m_config.dwDeviceNo);
        m_traversalClient.StartRequest(NAT_ID_CLIENT_CONNECT_RELAY_REQ, &relayReq, 
            sizeof(NAT_CLIENT_CONNECT_RELAY_REQ));
        m_state = TRAVERSAL_STATE_SER_REQUSTING;
    }
}

void CNatRelayClientPeer::OnReply( NatTraversalCmdId reqId, const void* req, 
    bool succeeded, NatTraversalCmdId replyId, void* reply )
{

    if (m_state == TRAVERSAL_STATE_SER_REQUSTING)
    {
        assert(reqId == NAT_ID_CLIENT_CONNECT_RELAY_REQ);
        if (succeeded)
        {
            assert(replyId == NAT_ID_CLIENT_CONNECT_RELAY_REPLY);
            HandleRelayConnectReply((NAT_CLIENT_CONNECT_RELAY_REPLY*)reply);
        }
        else
        {
            NAT_LOG_DEBUG("CNatRelayClientPeer relay request to NatServer timeout failed!\n");
            SetError(NAT_CLI_ERR_TIMEOUT);
        }
    }
}

int CNatRelayClientPeer::HandleRelayConnectReply(NAT_CLIENT_CONNECT_RELAY_REPLY *relayReply)
{
    assert(m_state == TRAVERSAL_STATE_SER_REQUSTING && m_pRelaySock == NULL);
    if (relayReply->stHeader.dwStatus != REPLY_STATUS_OK)
    {
        NAT_LOG_DEBUG("CNatRelayClientPeer recv relay connect reply failed: error=%s\n", 
            GetTraversalReplyStatusName(relayReply->stHeader.dwStatus));
        SetError(Nat_TraslateClientError(relayReply->stHeader.dwStatus));
        return false;
    }

    NAT_LOG_DEBUG("CNatRelayClientPeer recv relay connect reply OK: RelayServer=(%s:%d)\n", 
        Nat_inet_ntoa(relayReply->dwRelayServerIp), relayReply->usRelayServerPort);

    m_pRelaySock = new CRelayObj(relayReply->ucRelayConnectionId, 
        relayReply->dwRelayServerIp, relayReply->usRelayServerPort, RELAY_ROLE_CLIENT);
    if(NULL == m_pRelaySock)
    {
        NAT_LOG_WARN("CNatTraversalClientPeer create Relay Object failed!\n");
        SetError(NAT_CLI_ERR_SYS);
        return 0;
    }
    m_pRelaySock->SetRelayNotifier(this);
    m_pRelaySock->Initial();
    m_state = TRAVERSAL_STATE_DEV_CONNECTING;
    return 0;
}

void CNatRelayClientPeer::SetError( NAT_CLIENT_ERROR error )
{
    assert(error != NAT_CLI_OK);
    if (m_error == NAT_CLI_OK && m_state != TRAVERSAL_STATE_DEV_CONNECTED)
    {
        m_error = error;
    }
}

////////////////////////////////////////////////////////////////////////////////
//class CNatUdtClientPeer implements


const double CNatTraversalClientPeer::P2P_TIMEOUT_RATIO = 0.67;

const double CNatTraversalClientPeer::RELAY_TIMEOUT_RATIO = 1 - P2P_TIMEOUT_RATIO;

CNatTraversalClientPeer::CNatTraversalClientPeer()
:
m_initialized(false),
m_connectTimeoutMs(CONNECT_TIMEOUT_MS),
m_connectNotifier(NULL),
m_traversalMode(TRAVERSAL_P2P),
m_bClientAdded(false),
m_traversalThreadRuning(false),
m_p2pClient(NULL),
m_relayClient(NULL),
m_clientState(NAT_CLI_STATE_INIT)
{    
}

CNatTraversalClientPeer::~CNatTraversalClientPeer()
{
	if (PUB_THREAD_ID_NOINIT != m_traversalThreadID) 
	{		
		PUB_ExitThread(&m_traversalThreadID, &m_traversalThreadRuning);		
	}
    if (m_p2pClient != NULL)
    {
        delete m_p2pClient;
        m_p2pClient = NULL;
    }
    if (m_relayClient != NULL)
    {
        delete m_relayClient;
        m_relayClient = NULL;
    }
}

int CNatTraversalClientPeer::Init( const NAT_CLIENT_TRAVERSAL_CONFIG* config )
{
    int ret = 0;
    m_config = *config;

    m_traversalThreadID = PUB_CreateThread(TraversalThreadFunc, (void *)this, &m_traversalThreadRuning);	
    if (PUB_THREAD_ID_NOINIT == m_traversalThreadID) 
    {
        return -1;
    }	
    m_initialized = true;
    return 0;
}

void CNatTraversalClientPeer::SetStartTraversalMode(TraversalMode mode)
{
    if (!m_initialized)
    {
        m_traversalMode		   = mode;
    }
    else
    {
        assert(false);
    }
}
TraversalMode CNatTraversalClientPeer::GetTraversalMode()
{
	return m_traversalMode;
}


NAT_CLIENT_STATE CNatTraversalClientPeer::GetClientState()
{
	return m_clientState;
}

void CNatTraversalClientPeer::SetConnectTimeout( unsigned long timeoutMs )
{
    assert(!m_initialized);
	m_connectTimeoutMs = timeoutMs;
}

PUB_THREAD_RESULT PUB_THREAD_CALL CNatTraversalClientPeer::TraversalThreadFunc( void *pParam )
{
#ifdef	__ENVIRONMENT_LINUX__
	NAT_LOG_INFO("CNatTraversalClientPeer thread pid = %d\n", getpid());
#endif
	assert(NULL != pParam);
	CNatTraversalClientPeer *manager = reinterpret_cast<CNatTraversalClientPeer*>(pParam);
	manager->RunWork();

	return 0;
}

int CNatTraversalClientPeer::RunWork()
{

    if (InitTraversalClient() != 0)
    {
        // TODO: sys error
        NotifyOnConnect(NAT_SOCKET_INVALID, NAT_CLI_ERR_SYS);
        return 0;
    }


    NatTaskResult taskResult = NAT_TASK_IDLE;
    unsigned long curTickCount = 0;
    NAT_CLIENT_ERROR error = NAT_CLI_OK;
    bool isCompleted = false;


	while (m_traversalThreadRuning)
	{
        curTickCount = Nat_GetTickCount();
        taskResult = NAT_TASK_IDLE;

        if (m_traversalMode == TRAVERSAL_P2P)
        {
            assert(m_p2pClient != NULL);
            taskResult = m_p2pClient->RunTask(curTickCount);
			m_clientState = m_p2pClient->GetClientState();
            if (taskResult == NAT_TASK_DONE)
			{
				error = m_p2pClient->GetError();

                if (m_p2pClient->GetState() == TRAVERSAL_STATE_DEV_CONNECTED)
                {
                    error = NAT_CLI_OK;
                    isCompleted = true;
                }
				else if (error != NAT_CLI_OK)
				{
					if (m_p2pClient->IsP2PErrorOnly())
					{
						// Change to Relay mode
						m_traversalMode = TRAVERSAL_RELAY;
						if(EnterRelayMode())
						{
							error = NAT_CLI_OK;
							isCompleted = false;
						}
						else
						{
							error = NAT_CLI_ERR_SYS;
							isCompleted = true;
						}
					}
					else
					{
						isCompleted = true;
					}
				}
                else
                {
					assert(false);
					error = NAT_CLI_ERR_UNKNOWN;
					isCompleted = true;
                }
            }

        }
        else // m_traversalMode == TRAVERSAL_RELAY
        {
            assert(m_relayClient != NULL);
			taskResult = m_relayClient->RunTask(curTickCount);
			m_clientState = m_relayClient->GetClientState();
            if (taskResult == NAT_TASK_DONE)
            {
				error = m_relayClient->GetError();

                if (m_relayClient->GetState() == TRAVERSAL_STATE_DEV_CONNECTED)
                {
                    error = NAT_CLI_OK;
                }
				else if (error == NAT_CLI_OK)
				{
					assert(false);
					error = NAT_CLI_ERR_UNKNOWN;
				}
				isCompleted = true;
            }
        }

        if (isCompleted)
        {
            HandleComplete(error);
            break;
        }

        if (taskResult != NAT_TASK_DO_MORE)
        {
            PUB_Sleep(10);
        }

	}
 
	return 0;
}


void CNatTraversalClientPeer::SetConnectNotifier( CNatSocketConnectNotifier* notifier )
{
	CNatScopeLock lock(&m_notifierLock);
	m_connectNotifier = notifier;
}



void CNatTraversalClientPeer::NotifyOnConnect(NatSocket sock,int iErrorCode)
{
	if(m_connectNotifier != NULL)
	{	
		CNatScopeLock lock(&m_notifierLock);
		if(0 != iErrorCode)
		{
			m_connectNotifier->OnConnect(NULL, iErrorCode);
		}
		else
		{
			m_connectNotifier->OnConnect(sock, 0);
		}
	}
}


void CNatTraversalClientPeer::HandleComplete(NAT_CLIENT_ERROR error)
{
    NatSocket sock = NAT_SOCKET_INVALID;
    if (error == NAT_CLI_OK)
    {
        if(TRAVERSAL_P2P == m_traversalMode)
        {
            sock = m_p2pClient->HandleCompleteResult();
        }
        else
        {
            sock = m_relayClient->HandleCompleteResult();

        }
        if (sock == NAT_SOCKET_INVALID)
        {
            error = NAT_CLI_ERR_SYS;
        }
    }

    NotifyOnConnect(sock, error);
    return;
}

int CNatTraversalClientPeer::InitTraversalClient()
{
    unsigned long ip = 0;
    if (!Nat_ParseIpByName(m_config.pServerIp, ip))
    {
        NAT_LOG_INFO("CNatTraversalClientPeer parse nat server address(%s) failed!\n", m_config.pServerIp);
        return -1;
    }
    NAT_LOG_DEBUG("CNatTraversalClientPeer parse NatServer Ip = %s \n", Nat_inet_ntoa(ip));
    m_natServerIp = ip;

    if (m_traversalMode == TRAVERSAL_P2P)
    {
        NAT_LOG_DEBUG("CNatTraversalClientPeer enter p2p mode!\n");
        m_p2pClient = new CNatP2PClientPeer();
        if (m_p2pClient == NULL)
        {
            NAT_LOG_WARN("CNatTraversalClientPeer create p2p traversal client failed!\n");
            return -1;
        }
        m_p2pClient->SetTraversalTimeout((unsigned long)(P2P_TIMEOUT_RATIO * m_connectTimeoutMs));
        if(!m_p2pClient->Initilize(&m_config, m_natServerIp))
        {
            NAT_LOG_WARN("CNatTraversalClientPeer init p2p traversal client failed!\n");
            return -1;
        }
    }
    else
    {
        if(!EnterRelayMode())
        {
            NAT_LOG_DEBUG("CNatTraversalClientPeer init relay traversal client failed!\n");
            return -1;
        }
    }
    
    return 0;
}

bool CNatTraversalClientPeer::EnterRelayMode()
{
    assert(m_relayClient == NULL);
    NAT_LOG_DEBUG("CNatTraversalClientPeer enter relay mode!\n");

    m_relayClient = new CNatRelayClientPeer();
    if (m_relayClient == NULL)
    {
        NAT_LOG_WARN("CNatTraversalClientPeer create relay client failed!\n");
        return false;
    }
    m_relayClient->SetTraversalTimeout((unsigned long)(RELAY_TIMEOUT_RATIO * m_connectTimeoutMs));
    if (!m_relayClient->Initilize(&m_config, m_natServerIp))
    {
        NAT_LOG_WARN("CNatTraversalClientPeer start relay client failed!\n");
        return false;
    }

    return true;
}
