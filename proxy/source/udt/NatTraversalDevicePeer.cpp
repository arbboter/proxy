/************************************************************************/
// NatTraversalClient.cpp: 主要负责具体穿透功能的实现，包含xml的封装和解析
//
/************************************************************************/

#include "NatCommon.h"
#include "NatTraversalDevicePeer.h"
#include "NatDebug.h"
#include "NatDevicePeer.h"
#include "NatXml.h"

#include "NatDebug.h"


CNatDnsParser::CNatDnsParser()
:
m_state(STATE_NONE),
m_workThreadRunning(false),
m_workThreadId(PUB_THREAD_ID_NOINIT),
m_lock(),
m_destName(),
m_destIp(0),
m_isSucceeded(false)
{

}

CNatDnsParser::~CNatDnsParser()
{

}

bool CNatDnsParser::Initialize()
{
    assert(m_state == STATE_NONE);
    m_workThreadId = PUB_CreateThread(WorkThreadFunc, (void *)this, &m_workThreadRunning);	
    if (m_workThreadId == PUB_THREAD_ID_NOINIT)
    {
        NAT_LOG_WARN("CNatDnsParser create work thread failed!\n");
        return false;
    }
    m_state = STATE_BEGIN_INIT;
    return true;
}

void CNatDnsParser::Finalize()
{
    if (PUB_THREAD_ID_NOINIT != m_workThreadId) 
    {		
        PUB_ExitThread(&m_workThreadId, &m_workThreadRunning);
        m_workThreadId = PUB_THREAD_ID_NOINIT;
    }
    m_destName = "";
    m_destIp = 0;
    m_isSucceeded = false;
    m_state = STATE_NONE;
}

bool CNatDnsParser::IsInitialized()
{
    return m_state != STATE_NONE;
}


void CNatDnsParser::BeginParse(const char* destName)
{
    m_lock.Lock();
    m_destName = destName;
    m_destIp = 0;
    m_isSucceeded = false;
    m_state = STATE_BEGIN_INIT;
    m_lock.UnLock();
}

bool CNatDnsParser::IsSucceeded()
{
    return m_isSucceeded;
}

PUB_THREAD_RESULT PUB_THREAD_CALL CNatDnsParser::WorkThreadFunc( void *pParam )
{
#ifdef	__ENVIRONMENT_LINUX__
    NAT_LOG_INFO("CNatTraversalDevicePeer thread pid = %d\n", getpid());
#endif
    assert(NULL != pParam);
    CNatDnsParser *dnsParser = reinterpret_cast<CNatDnsParser*>(pParam);
    dnsParser->RunWork();

    return 0;
}
int CNatDnsParser::RunWork()
{
    assert(m_state != STATE_NONE);
    int  ret = 0;
    bool doMore = false;
    bool isSucceeded = false;
    while(m_workThreadRunning)
    {
        assert(m_state != STATE_PARSING);
        doMore = false;
        isSucceeded = false;
        if (m_state == STATE_BEGIN_INIT)
        {
            m_lock.Lock();
            m_state = STATE_PARSING;
            m_lock.UnLock();

            m_destIp = INADDR_NONE;
            if(Nat_ParseIpByName(m_destName.c_str(), m_destIp))
            {
                NAT_LOG_DEBUG("CNatTraversalDevicePeer parse NatServer IP succeeded! name=%s ip=%s\n",
                    m_destName.c_str(), Nat_inet_ntoa(m_destIp));
                isSucceeded = true;
            }
            else
            {
                m_destIp = INADDR_NONE;
                NAT_LOG_DEBUG("CNatTraversalDevicePeer parse NatServer IP failed! name=%s\n", m_destName.c_str());
                isSucceeded = false;
            }

            m_lock.Lock();
            m_isSucceeded = isSucceeded;
            if (m_state == STATE_PARSING)
            {
                m_state = STATE_COMPLETED;
            }
            else
            {
                doMore = true;
            }
            m_lock.UnLock();
        }

        if (!doMore)
        {
            PUB_Sleep(100);
        }
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// class CNatTraversalDevicePeer implements

CNatTraversalDevicePeer::CNatTraversalDevicePeer()
:
m_pDevicePeer(NULL),
m_pUdtTransport(NULL),
m_isResetServer(false),
m_connectState(STATE_NONE),
m_lastError(NAT_ERR_OK),
m_curTickCount(0)
{
	memset(&m_replyInfo,0,sizeof(SERVER_REPLY_INFO));
    m_traversalClient.SetNotifier(this);
}

CNatTraversalDevicePeer::~CNatTraversalDevicePeer()
{

}

bool CNatTraversalDevicePeer::Start(CNatDevicePeer *devicePeer, CNatUdtTransport *udtTransport, 
									const CNatTraversalDevicePeer::REGISTER_CONFIG *config)
{
	m_curTickCount = Nat_GetTickCount();
    m_pDevicePeer = devicePeer;
    m_pUdtTransport = udtTransport;
    memset(&m_registerConfig, 0, sizeof(m_registerConfig));
	m_registerConfig = *config;
    m_resetConfig = m_registerConfig;

    if(!m_dnsParser.Initialize())
    {
        NAT_LOG_WARN("CNatTraversalDevicePeer init dns parser failed!\n");
        return false;
    }

    m_lastSendCmdTime = 0;
    m_lastRecvCmdTime = 0;
    m_lastErrorTime = 0;

    NAT_LOG_DEBUG("NatTraversalDevicePeer start OK!\n");
    ChangeParseServIp();
    assert(m_connectState != STATE_NONE);
    return true;
}

void CNatTraversalDevicePeer::Stop()
{
    InternalClose();
    NAT_LOG_DEBUG("NatTraversalDevicePeer stop!\n");
}

bool CNatTraversalDevicePeer::IsStarted()
{
    return (m_connectState != STATE_NONE) ;
}

void  CNatTraversalDevicePeer::StopTraversalClient()
{
	if(m_traversalClient.IsStarted())
	{
        m_traversalClient.Stop();
	}

    memset(&m_replyInfo,0,sizeof(SERVER_REPLY_INFO));
}

int CNatTraversalDevicePeer::ResetNatServer(const char *serverIp, unsigned short serverPort)
{	
    if (IsStarted())
    {
        m_resetServerLock.Lock();	
        strcpy(m_resetConfig.serverIp, serverIp);
        m_resetConfig.serverPort = serverPort;
        m_isResetServer = true;
        m_resetServerLock.UnLock();

        NAT_LOG_DEBUG("CNatTraversalDevicePeer reset NatServer(%s:%d)\n", 
            serverIp, serverPort);
        return 0;
    }
    return -1;
}


void CNatTraversalDevicePeer::SetDeviceExtraInfo(const char* extraInfo)
{
	if (IsStarted())
	{
		m_resetServerLock.Lock();	
		strcpy(m_resetConfig.caExtraInfo, extraInfo);
		m_isResetServer = true;
		m_resetServerLock.UnLock();
	}
}

bool  CNatTraversalDevicePeer::CheckConnectTimeout()
{
	if(PUB_IsTimeOutEx(m_lastRecvCmdTime, NAT_TRAVERSAL_DEVICE_REG_TIMEOUT, m_curTickCount))
	{
		return true;
	}
	return  false;
}

NatRunResult CNatTraversalDevicePeer::Run(unsigned long curTickCount)
{
    assert(m_connectState != STATE_NONE);
	NatRunResult runResult = RUN_NONE;
    m_curTickCount = curTickCount;

	if (m_isResetServer)
	{
		m_resetServerLock.Lock();
		if (m_isResetServer)
		{
			m_isResetServer = false;
			StopTraversalClient();
            m_registerConfig = m_resetConfig;
            ChangeParseServIp();
		}
		m_resetServerLock.UnLock();
	}


    if (m_traversalClient.IsStarted())
    {
        assert(m_connectState == STATE_REGISTERING || m_connectState == STATE_REGISTERED
            || m_connectState == STATE_CONNECTING || m_connectState == STATE_REGISTER_FAILED);
        runResult = NatRunResultMax(runResult, m_traversalClient.Run(curTickCount));
        if(runResult == RUN_DO_FAILED)
        {
            NAT_LOG_DEBUG("CNatTraversalDevicePeer run traversal client failed!\n");
            ChangeDisconnected();
        }
    }

    if(STATE_PARSE_SERV_IP == m_connectState)
    {	
        RunDnsPaseCompleted();        
    }
    else if(STATE_REGISTERED == m_connectState)
    {
        TrySendReply();	
        CheckSendHeartBeat();
    }
    else if(STATE_REGISTER_FAILED == m_connectState)
    {
        if (CheckErrorTimeout())
        {
            if (!m_traversalClient.IsRequesting())
            {
                NAT_LOG_DEBUG("CNatTraversalDevicePeer re-register to NatServer!\n");
                ChangeRegistering();
            }
        }

    }
    else if(STATE_DISCONNECTED == m_connectState)
    {
        if (CheckErrorTimeout())
        {
            assert(!m_traversalClient.IsStarted());
            ChangeParseServIp();
        }	
    }
    return runResult;
}

void CNatTraversalDevicePeer::OnServerConnect()
{
    if (STATE_CONNECTING == m_connectState) 
    {
        ChangeRegistering();
    }
}

void CNatTraversalDevicePeer::OnActiveCmd(NatTraversalCmdId cmdId, void* cmd)
{
    if (m_connectState != STATE_REGISTERED)
    {
        NAT_LOG_DEBUG("CNatTraversalDevicePeer has not registered, but recv cmd=%d", cmdId);
        return;
    }

    bool handled = false;
    switch(cmdId)
    {
    case NAT_ID_NOTIFY_CONNECT_P2P_REQ:
        HandleConnectP2PReq(reinterpret_cast<NAT_NOTIFY_CONNECT_P2P_REQ *>(cmd));        
        handled = true;
        break;
    case NAT_ID_NOTIFY_CONNECT_RELAY_REQ:
        HandleConnectRelayReq(reinterpret_cast<NAT_NOTIFY_CONNECT_RELAY_REQ *>(cmd));
        handled = true;
        break;
    case NAT_ID_HEARTBEAT:
        //printf("recv heart beat\n");
        handled = true;
        break;
    default:
        break;        
    }

    if (handled)
    {
        m_lastRecvCmdTime = m_curTickCount;
    }
}

void CNatTraversalDevicePeer::OnReply( NatTraversalCmdId reqId, const void* req, 
                                bool succeeded, NatTraversalCmdId replyId, void* reply )
{
    if (m_connectState == STATE_REGISTERING)
    {
        assert(reqId == NAT_ID_DEVICE_REGISTER_REQ);
        if (succeeded)
        {
            assert(replyId == NAT_ID_DEVICE_REGISTER_REPLY);
            if(ChangeRegistered(reinterpret_cast<NAT_DEVICE_REGISTER_REPLY*>(reply)))
            {
                return;
            }
        }
        else
        {
            NAT_LOG_DEBUG("CNatTraversalDevicePeer register to NatServer failed! Error=Register Timeout\n");
        }
        ChangeRegisterFailed();
    }

}

void CNatTraversalDevicePeer::OnRecvDatagram( const NAT_TRANSPORT_RECV_DATAGRAM* datagram)
{
    if (m_traversalClient.IsStarted())
    {
        assert(m_connectState == STATE_REGISTERING || m_connectState == STATE_REGISTERED
            || m_connectState == STATE_CONNECTING || m_connectState == STATE_REGISTER_FAILED);
        m_traversalClient.NotifyRecvDatagram(datagram);
    }
}


void CNatTraversalDevicePeer::HandleConnectP2PReq(const NAT_NOTIFY_CONNECT_P2P_REQ *req)
{
    assert(m_connectState == STATE_REGISTERED);
    NAT_LOG_INFO("CNatTraversalDevicePeer recv client P2P connect. client=(%s:%d), connectionId=%lu\n", 
        Nat_inet_ntoa(req->dwRequestPeerIp), req->dwRequestPeerPort, req->dwConnectionId);

    NAT_NOTIFY_CONNECT_P2P_REPLY reply;
    memset(&reply, 0, sizeof(reply));	
    reply.stHeader.dwRequestSeq = req->stHeader.dwRequestSeq;
    reply.stHeader.dwStatus     = REPLY_STATUS_OK;
    StartSendReply(NAT_ID_NOTIFY_CONNECT_P2P_REPLY, &reply, sizeof(reply));

    m_pDevicePeer->OnP2PClientConnect(req->dwConnectionId, req->dwRequestPeerIp, 
        req->dwRequestPeerPort);
}

void CNatTraversalDevicePeer::HandleConnectRelayReq(const NAT_NOTIFY_CONNECT_RELAY_REQ *req)
{
    NAT_LOG_INFO("CNatTraversalDevicePeer recv client Relay connect. RelayServer=(%s, %d), connectionId=%lu\n",
        Nat_inet_ntoa(req->dwRelayServerIp), req->dwRelayServerPort, req->dwConnectionId);

    assert(m_connectState == STATE_REGISTERED);
    NAT_NOTIFY_CONNECT_RELAY_REPLY reply;
    memset(&reply ,0, sizeof(reply));
    reply.stHeader.dwRequestSeq = req->stHeader.dwRequestSeq;
    reply.stHeader.dwStatus     = REPLY_STATUS_OK;
    StartSendReply(NAT_ID_NOTIFY_CONNECT_RELAY_REPLY, &reply, sizeof(reply));

    m_pDevicePeer->OnRelayClientConnect(req->dwConnectionId, req->dwRelayServerIp,
        req->dwRelayServerPort);
}

int CNatTraversalDevicePeer::CheckSendHeartBeat()
{
	//检测是否需要发送心跳
	if(PUB_IsTimeOutEx(m_lastSendCmdTime, NAT_TRAVERSAL_DEVICE_REG_HEARTBEAT, m_curTickCount))
	{
        if (!m_traversalClient.IsSendingCmd())
        {
            NAT_TRAVERSAL_HEARTBEAT cmd;
            memset(&cmd, 0, sizeof(cmd));
            strcpy(cmd.uaDeviceNo, m_registerConfig.deviceNo);
            m_traversalClient.SendCmd(NAT_ID_HEARTBEAT, &cmd, sizeof(cmd));
            m_lastSendCmdTime = m_curTickCount;
        }
	}
	return 0;
}

void CNatTraversalDevicePeer::ReplyRelayConnectReq(unsigned long dwRequestSeq,unsigned long dwStatus)
{
    assert(m_connectState == STATE_REGISTERED);
    NAT_NOTIFY_CONNECT_RELAY_REPLY reply;
	memset(&reply ,0, sizeof(reply));
	reply.stHeader.dwRequestSeq = dwRequestSeq;
	reply.stHeader.dwStatus     = dwStatus;
    StartSendReply(NAT_ID_NOTIFY_CONNECT_RELAY_REPLY, &reply, sizeof(reply));
}


void CNatTraversalDevicePeer::InternalClose()
{
    if (m_dnsParser.IsInitialized())
    {
        m_dnsParser.Finalize();
    }

    if (m_traversalClient.IsStarted())
    {
        m_traversalClient.Stop();
    }

	//恢复初始态
	m_connectState  = STATE_NONE;
    m_lastError = NAT_ERR_OK;
    m_isResetServer = false;
    m_pUdtTransport = NULL;
    m_pDevicePeer = NULL;
	memset(&m_replyInfo,0,sizeof(SERVER_REPLY_INFO));
}


void CNatTraversalDevicePeer::ChangeParseServIp()
{
    assert(!m_traversalClient.IsStarted());
    m_dnsParser.BeginParse(m_registerConfig.serverIp);
    m_connectState = STATE_PARSE_SERV_IP;
}

bool CNatTraversalDevicePeer::ChangeConnecting(long serverIp, unsigned short serverPort)
{
    if (m_traversalClient.Start(serverIp, serverPort, m_pUdtTransport))
    {
        m_connectState		   = STATE_CONNECTING;
        m_lastRecvCmdTime	   = m_curTickCount;
        m_lastSendCmdTime      = m_curTickCount;
        NAT_LOG_DEBUG("CNatTraversalDevicePeer start traversal client to NatServer(%s:%d)!\n",
            Nat_inet_ntoa(serverIp), serverPort);
        return true;
    }
    else
    {
        NAT_LOG_DEBUG("CNatTraversalDevicePeer start traversal client to NatServer failed!\n");
        return false;
    }
}

void CNatTraversalDevicePeer::ChangeRegistering()
{
    assert(!m_traversalClient.IsRequesting());

    NAT_DEVICE_REGISTER_REQ  req;

    memset(&req, 0, sizeof(NAT_DEVICE_REGISTER_REQ));
    req.stHeader.dwRequestSeq					= m_traversalClient.GenNextReqSeq();
    strcpy(req.caDeviceNo,  m_registerConfig.deviceNo);
    req.usP2PVersion.sVersion.ucHighVer		= P2P_VERSION_HIGH;
    req.usP2PVersion.sVersion.ucLowVer		= P2P_VERSION_LOW;
    req.ucRefuseRelay = m_registerConfig.refuseRelay;
	strcpy(req.caExtraInfo, m_registerConfig.caExtraInfo);
	

    NAT_LOG_DEBUG("CNatTraversalDevicePeer Send Register Request to NatServer!\n");
    m_traversalClient.StartRequest(NAT_ID_DEVICE_REGISTER_REQ, &req, 
        sizeof(NAT_DEVICE_REGISTER_REQ));
    m_lastSendCmdTime = Nat_GetTickCount();

    m_connectState = STATE_REGISTERING;
}

bool CNatTraversalDevicePeer::ChangeRegistered(const NAT_DEVICE_REGISTER_REPLY *reply )
{
    assert(m_connectState == STATE_REGISTERING);

    if (reply->stHeader.dwStatus != REPLY_STATUS_OK)
    {
        NAT_LOG_INFO("CNatTraversalDevicePeer register to NatServer failed! Reply status=%s\n", 
            GetTraversalReplyStatusName(reply->stHeader.dwStatus));
        return false;
    }

    NAT_LOG_INFO("CNatTraversalDevicePeer register succeeded! Device public addr=%s:%d\n", 
        Nat_inet_ntoa(reply->dwPeerIp), reply->dwPeerPort);
    m_connectState = STATE_REGISTERED;
    return true;
}

void CNatTraversalDevicePeer::ChangeRegisterFailed()
{
    if (STATE_REGISTER_FAILED != m_connectState)
    {
        m_lastError = NAT_ERR_REGISTER_FAILED;
        m_lastErrorTime  = m_curTickCount;
        m_connectState    =  STATE_REGISTER_FAILED;
    }    
}

void CNatTraversalDevicePeer::ChangeDisconnected()
{
	if (STATE_DISCONNECTED != m_connectState)
	{
        StopTraversalClient();
        m_lastError = NAT_ERR_CONNECT_FAILED;
        m_lastErrorTime  = m_curTickCount;
		m_connectState    =  STATE_DISCONNECTED;
	}
}

void CNatTraversalDevicePeer::TrySendReply()
{
	if(!m_traversalClient.IsSendingCmd() && m_replyInfo.hasReply )
	{
        m_traversalClient.SendCmd(m_replyInfo.replyCmdId, m_replyInfo.replyBuf, m_replyInfo.replyBufSize);
        m_replyInfo.hasReply = false;
        m_lastSendCmdTime = m_curTickCount;
	}	
}

NAT_DEVICE_SERVER_ERROR CNatTraversalDevicePeer::GetServerError()
{
    NAT_DEVICE_SERVER_ERROR ret = NAT_ERR_OK;
	switch(m_connectState)
	{
	case STATE_REGISTER_FAILED:
		ret = NAT_ERR_REGISTER_FAILED;
		break;
	case STATE_DISCONNECTED:
		ret = NAT_ERR_CONNECT_FAILED;
		break;
	default:
		ret = NAT_ERR_OK;
	}

	return ret;
}

NAT_DEVICE_SERVER_STATE CNatTraversalDevicePeer::GetServerState()
{
    NAT_DEVICE_SERVER_STATE ret = NAT_STATE_NONE;
	switch(m_connectState)
	{
	case STATE_PARSE_SERV_IP:
	case STATE_CONNECTING:
	case STATE_REGISTERING:
		 ret = NAT_STATE_CONNECTING;
		 break;

	case STATE_REGISTERED:
		ret  = NAT_STATE_CONNECTED;
        break;

    case STATE_REGISTER_FAILED:
	case STATE_DISCONNECTED:
		ret  = NAT_STATE_DISCONNECTED;
        break;

    case NAT_STATE_NONE:
    default:
        ret = NAT_STATE_NONE;
	}
	return ret;
}

void CNatTraversalDevicePeer::StartSendReply( NatTraversalCmdId replyId, const void* reply, int replySize )
{
    m_replyInfo.hasReply = true;
    m_replyInfo.replyCmdId = replyId;
    m_replyInfo.replyBufSize = replySize;
    memcpy(m_replyInfo.replyBuf, reply, replySize);
    
    TrySendReply();
}

void CNatTraversalDevicePeer::RunDnsPaseCompleted()
{
    assert(m_dnsParser.IsInitialized());
    if (m_dnsParser.IsCompleted())
    {	
        CNatScopeLock lock(&m_dnsParser.GetLock());
        if(m_dnsParser.IsCompleted())
        {
            if (m_dnsParser.IsSucceeded())
            {
                if(ChangeConnecting(m_dnsParser.GetDestIp(), m_registerConfig.serverPort))
                {
                    return;
                }
            }
            else
            {
                NAT_LOG_DEBUG("CNatTraversalDevicePeer pase NatServer ip failed!\n");
            }
            ChangeDisconnected();
        }
    }
    
}

bool CNatTraversalDevicePeer::CheckErrorTimeout()
{
    return PUB_IsTimeOutEx(m_lastErrorTime, ERROR_WAIT_TIME, m_curTickCount);
}

