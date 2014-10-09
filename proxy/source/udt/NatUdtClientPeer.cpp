// NatUdtClientPeer.cpp: implements for the CNatUdtClientPeer class.
//
//////////////////////////////////////////////////////////////////////

#include "NatCommon.h"
#include "NatUdtClientPeer.h"
#include "NatClientPeerManager.h"
#include "NatDebug.h"

////////////////////////////////////////////////////////////////////////////////
//class CNatUdtClientPeer implements

CNatUdtClientPeer::CNatUdtClientPeer( )
:
m_connectTimeoutMs(CONNECT_TIMEOUT_MS),
m_connectStartTimeMs(Nat_GetTickCount()),
m_connectNotifier(NULL),
m_bClientAdded(false),
m_pUdtTransport(NULL),
m_p2pConnectState(STATE_P2P_NONE),
m_udt(NULL),
m_workThreadID(PUB_THREAD_ID_NOINIT),
m_workThreadRuning(false)
{

}

CNatUdtClientPeer::~CNatUdtClientPeer()
{
	if (PUB_THREAD_ID_NOINIT != m_workThreadID) 
	{		
		PUB_ExitThread(&m_workThreadID, &m_workThreadRuning);
	}
	InternalClose();
}


void CNatUdtClientPeer::SetConnectTimeout( unsigned long timeoutMs )
{
    m_connectTimeoutMs = timeoutMs;
}
PUB_THREAD_RESULT PUB_THREAD_CALL CNatUdtClientPeer::WorkThreadFunc( void *pParam )
{
#ifdef	__ENVIRONMENT_LINUX__
	NAT_LOG_INFO("CNatUdtClientPeer thread pid = %d\n",getpid());
#endif
	assert(NULL != pParam);
	CNatUdtClientPeer *manager = reinterpret_cast<CNatUdtClientPeer*>(pParam);
	manager->RunWork();

	return 0;
}
int CNatUdtClientPeer::RunWork()
{
	NatRunResult runResult=RUN_NONE;

	while (m_workThreadRuning)
	{
		runResult=Run();

		if (RUN_NONE == runResult)
		{
			PUB_Sleep(10); // TODO: 10ms ?
		}

	}
	return 0;
}
int CNatUdtClientPeer::Init( const NAT_CLIENT_DIRECT_CONFIG* config )
{
    int ret;
    NAT_UDT_TRANSPORT_CONFIG udtTransportConfig;
    udtTransportConfig.localIp = 0;
    udtTransportConfig.localPort = 0;

	m_pUdtTransport  = new CNatUdtTransport();
    m_pUdtTransport->SetNotifier(this);
    ret = m_pUdtTransport->Start(&udtTransportConfig);
    if (ret != 0)
    {
        InternalClose();
        return ret;
    }

	m_udt = new CNatUdt();
	if (m_udt == NULL)
	{
		InternalClose();
		return -1;
	}

    NAT_UDT_CONFIG udtConfig;
    memset(&udtConfig, 0, sizeof(udtConfig));
    udtConfig.category = NAT_UDT_CATEGORY_P2P;
    udtConfig.remoteIp = config->dwDeviceIp;
    udtConfig.remotePort = config->nDevicePort;
    udtConfig.connectionID = GetRandomConnectionId();
    udtConfig.maxRecvWindowSize = UDT_RECV_WINDOW_SIZE_MAX;
    udtConfig.pRecvHeapDataManeger = NULL;
    udtConfig.maxSendWindowSize = UDT_SEND_WINDOW_SIZE_MAX;
    udtConfig.pSendHeapDataManeger = NULL;
	NAT_LOG_DEBUG("connectionID=%d\n",udtConfig.connectionID);
	m_udt->Start(&udtConfig);

    m_udt->SetNotifier(this);
    m_p2pConnectState    = STATE_P2P_CONNECTING;
    m_connectStartTimeMs = Nat_GetTickCount();

	m_workThreadID = PUB_CreateThread(WorkThreadFunc, (void *)this, &m_workThreadRuning);	
	if (PUB_THREAD_ID_NOINIT == m_workThreadID) 
	{
		InternalClose();
		return -1;
	}

    return 0;
}

NatRunResult CNatUdtClientPeer::Run()
{
    NatRunResult runResult = RUN_NONE;

    // check is connecting and is timeout?
	if(NULL == m_pUdtTransport)
	{
		return RUN_NONE ;
	}
	
	assert(m_pUdtTransport->IsStarted());
	
    runResult = NatRunResultMax(runResult, m_pUdtTransport->Run());
    if (RUN_DO_FAILED == runResult)
    {
        ChangeDisConnected();
        return RUN_DO_MORE;
    }
	if(NULL != m_udt)
	{
		runResult = NatRunResultMax(runResult, m_udt->Run());
		if (RUN_DO_FAILED == runResult)
		{
			ChangeDisConnected();
			return RUN_DO_MORE;
		}
	}
    
    return runResult;
}


void CNatUdtClientPeer::SetConnectNotifier( CNatSocketConnectNotifier* notifier )
{
	CNatScopeLock lock(&m_notifierLock);
    m_connectNotifier = notifier;
}

void CNatUdtClientPeer::OnRecvDatagram( CNatUdtTransport *transport, const NAT_TRANSPORT_RECV_DATAGRAM* datagram)
{
    if(NULL != m_udt)
    {
        if(m_udt->IsMineDatagram(datagram))
        {
            if (m_udt->NotifyRecvDatagram(datagram) < 0)
            {
                ChangeDisConnected();
            }
        } 
        else
        {
            // print warning
        }
    }
}

int CNatUdtClientPeer::AddToClientManager()
{

	m_pUdtTransport->SetNotifier(NULL);
	m_udt->SetNotifier(NULL);
	CNatClientUdtSocket *pNatClientUdtSocket = new CNatClientUdtSocket(m_udt,m_pUdtTransport);
	if(NULL != pNatClientUdtSocket)
	{
		NatSocket sock = CNatClientPeerManager::instance()->AddConnectedSocket(pNatClientUdtSocket);
        if (sock != NAT_SOCKET_INVALID)
        {
            m_udt			 =  NULL;
            m_pUdtTransport  =  NULL;
            NotifyOnConnect(sock, 0);
            m_bClientAdded =true;
            return 0;
        }
        delete pNatClientUdtSocket;
	}
	NotifyOnConnect(NAT_SOCKET_INVALID, -1);
	return -1;
}
void CNatUdtClientPeer::OnConnect( CNatUdt *pUDT, int iErrorCode )
{

    if (0 == iErrorCode)
    {
        if ( STATE_P2P_CONNECTING == m_p2pConnectState)
        {
            m_p2pConnectState  = STATE_P2P_CONNECTED;
			AddToClientManager();
        }
    }
    else
    {
        ChangeDisConnected();
    }

}

int CNatUdtClientPeer::OnRecv( CNatUdt *pUDT, const void* pBuf, int iLen )
{
//     if (iLen < 0)
//     {
//         ChangeDisConnected();
//     }
//     if (IsUsing())
//     {
//         return NotifyOnRecv(pBuf, iLen);
//     }
    return 0;
}

int CNatUdtClientPeer::OnSendPacket( CNatUdt *pUDT, const void* pBuf, int iLen )
{

    int ret = m_pUdtTransport->Send(m_udt->GetRemoteIp(), m_udt->GetRemotePort(),
        pBuf, iLen);
    if (ret < 0)
    {
        ChangeDisConnected();
    }
    return ret;
}

void CNatUdtClientPeer::InternalClose()
{

	//如果没有被加入ClientManager管理，则释放
	if(!m_bClientAdded)
	{	
		if (m_udt != NULL)
		{ 
			m_udt->Stop();
			delete m_udt;
			m_udt = NULL;
		}

		if (m_pUdtTransport->IsStarted())
		{
			m_pUdtTransport->Stop();
			delete m_pUdtTransport;
			m_pUdtTransport =NULL;
		}

	   m_p2pConnectState = STATE_P2P_DISCONNECTED;
	}   
}
void CNatUdtClientPeer::NotifyOnConnect(NatSocket sock, int iErrorCode)
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
void CNatUdtClientPeer::ChangeDisConnected()
{
    if (STATE_P2P_DISCONNECTED != m_p2pConnectState)
    {
	    m_p2pConnectState  = STATE_P2P_DISCONNECTED;
        NotifyOnConnect(NAT_SOCKET_INVALID, -1);
    }
}
