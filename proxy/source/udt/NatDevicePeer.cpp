// NatDevicePeer.cpp: implements for the CNatDevicePeer class.
//
//////////////////////////////////////////////////////////////////////

#include "NatCommon.h"
#include "NatDevicePeer.h"
#include "NatSockManger.h"
#include "NatDebug.h"

//////////////////////////////////////////////////////////////////////////
//CNatDevicePeer::CDeviceTransportNotifier implements

CNatDevicePeer::CUdtTransportDispatch::CUdtTransportDispatch()
:
m_devicePeer(NULL)
{
}

void CNatDevicePeer::CUdtTransportDispatch::Init( CNatDevicePeer* devicePeer )
{
    m_devicePeer = devicePeer;
}

void CNatDevicePeer::CUdtTransportDispatch::OnRecvDatagram(CNatUdtTransport *transport, 
    const NAT_TRANSPORT_RECV_DATAGRAM* datagram)
{
    CNatUdtDeviceClient *client;

    CNatScopeLock scopeLock(&m_devicePeer->m_clientsLock);

    unsigned char category = CNatUdt::GetDatagramCategory(datagram);
	if(category == NAT_UDT_CATEGORY_P2NS)
	{
		m_devicePeer->m_traversalDevicePeer.OnRecvDatagram(datagram);
	}else
	{
		client = m_devicePeer->FindUdtClient(datagram);
		if (client == NULL)
		{     
            if (CNatUdt::IsDatagramSynCmd(datagram))
			{
                client = m_devicePeer->FindUdtTraversalClient(datagram);
                if (client == NULL)
                {
                    client = m_devicePeer->CreateUdtClient(CNatUdt::GetDatagramConnectionId(datagram),
                        datagram->fromIp, datagram->fromPort, CNatUdtDeviceClient::MODE_DIRECT); 
                    NAT_LOG_INFO("New Direct P2P Client Enter! Peer=%s:%d, Cid=%d\n",
                        Nat_inet_ntoa(datagram->fromIp), datagram->fromPort, client->GetConnectionId());           
                }
                else
                {
                    NAT_LOG_INFO("New Direct P2P Client (May be Symmetric NAT) Enter! Old Port=%d, Peer=%s:%d, Cid=%d\n",
                        client->GetRemotePort(), Nat_inet_ntoa(datagram->fromIp), datagram->fromPort, client->GetConnectionId());
                    client->Reset(datagram);
                }
			}          
		}
		if (client != NULL) 
		{
			client->RecvPacket(datagram);
		}
		else
		{
			// TODO: print error
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
//class CNatUdtDeviceClient implements

CNatUdtDeviceClient::CNatUdtDeviceClient( CNatDevicePeer *devicePeer,unsigned long connectionId,
    unsigned long fromIP, unsigned short fromPort, CLIENT_COME_IN_MODE comeInMode)
:
m_devicePeer(devicePeer),
m_isClosed(false),
m_comeInMode(comeInMode)
{
    memset(&m_config, 0, sizeof(m_config));
    m_config.category = NAT_UDT_CATEGORY_P2P;
    m_config.remoteIp = fromIP;
    m_config.remotePort = fromPort;
    m_config.connectionID = connectionId;
    m_config.maxRecvWindowSize = UDT_RECV_WINDOW_SIZE_MAX;  
    m_config.pRecvHeapDataManeger = NULL;  
    m_config.maxSendWindowSize = UDT_SEND_WINDOW_SIZE_MAX;
    m_config.pSendHeapDataManeger = NULL;
	m_udt.Start(&m_config);
    m_udt.SetNotifier(this);
    ChangeState(CNatSocketBase::STATE_INIT);
}

CNatUdtDeviceClient::~CNatUdtDeviceClient()
{
	m_udt.Stop();
    ChangeState(CNatSocketBase::STATE_CLOSED);
}

void CNatUdtDeviceClient::RecvPacket( const NAT_TRANSPORT_RECV_DATAGRAM* datagram)
{
    assert(m_udt.IsMineDatagram(datagram));
    if(m_udt.NotifyRecvDatagram(datagram) < 0)
    {
        ChangeDisConnected();
    }
}

bool CNatUdtDeviceClient::IsMineDatagram( const NAT_TRANSPORT_RECV_DATAGRAM* datagram )
{
    return m_udt.IsMineDatagram(datagram);
}

bool CNatUdtDeviceClient::IsSymmetricClientDatagram(const NAT_TRANSPORT_RECV_DATAGRAM* datagram)
{
    assert(CNatUdt::GetDatagramCategory(datagram) == NAT_UDT_CATEGORY_P2P);
    return  (GetState() == CNatSocketBase::STATE_INIT || GetState() == CNatSocketBase::STATE_CONNECTING)
        && m_comeInMode != MODE_DIRECT
        && m_udt.IsDatagramValid(datagram)
        && m_udt.GetConnectionID() == CNatUdt::GetDatagramConnectionId(datagram)        
        && m_udt.GetRemoteIp() == datagram->fromIp;
}

void CNatUdtDeviceClient::Reset(const NAT_TRANSPORT_RECV_DATAGRAM* datagram)
{
    assert(IsSymmetricClientDatagram(datagram));
    m_udt.Stop();
    m_config.remotePort = datagram->fromPort;
    m_udt.Start(&m_config);
    m_udt.SetNotifier(this);
    m_comeInMode = MODE_MIXED;
}

CNatUdtDeviceClient::CLIENT_COME_IN_MODE CNatUdtDeviceClient::GetComeInMode() 
{ 
    return m_comeInMode; 
}

int CNatUdtDeviceClient::Send( const void *pBuf, int iLen )
{
    int ret = -1;
    if (IsConnected())
    {
        ret = m_udt.Send(pBuf, iLen);
        if (ret < 0)
        {
            ChangeDisConnected();
        }
    }
    return ret;
}

int CNatUdtDeviceClient::Recv( void *pBuf, int iLen )
{
    if (IsConnected())
    {
        return m_udt.Recv(pBuf, iLen);
    }
    return -1;
}

int CNatUdtDeviceClient::GetSendAvailable()
{
    if (IsConnected())
    {
        return m_udt.GetSendAvailable();
    }
    return -1;
}

int CNatUdtDeviceClient::GetRecvAvailable()
{
    if (IsConnected())
    {
        return m_udt.GetRecvAvailable();
    }
    return -1;
}

int CNatUdtDeviceClient::SetAutoRecv( bool bAutoRecv )
{
    if (IsConnected())
    {
        m_udt.SetAutoRecv(bAutoRecv);
        return 0;
    }
    return -1;
}

int CNatUdtDeviceClient::Close()
{
    return CNatSocketBase::Close(); 
}

unsigned long CNatUdtDeviceClient::GetRemoteIp()
{
    return m_udt.GetRemoteIp();
}

unsigned short CNatUdtDeviceClient::GetRemotePort()
{
    return m_udt.GetRemotePort();
}

unsigned long CNatUdtDeviceClient::GetConnectionId()
{
	return m_udt.GetConnectionID();
}

void CNatUdtDeviceClient::OnConnect( CNatUdt *pUDT, int iErrorCode )
{
    assert(IsInited());
    if (0 == iErrorCode)
    {
        if (GetState() == CNatSocketBase::STATE_INIT) 
        {
            ChangeState(CNatSocketBase::STATE_CONNECTED);
            m_devicePeer->NotifyOnConnect(this);
        }
    }
    else
    {
        ChangeDisConnected();
    }

}

int CNatUdtDeviceClient::OnRecv( CNatUdt *pUDT, const void* pBuf, int iLen )
{
    if (IsUsing())
    {
        return NotifyOnRecv(pBuf, iLen);
    }
    return 0;
}

int CNatUdtDeviceClient::OnSendPacket( CNatUdt *pUDT, const void* pBuf, int iLen )
{
    assert(IsInited());
    return m_devicePeer->m_udtTransport.Send(m_udt.GetRemoteIp(), m_udt.GetRemotePort(), 
        pBuf, iLen);
}

NatRunResult CNatUdtDeviceClient::Run()
{
    NatRunResult runResult = RUN_NONE;
    if (IsInited() && GetState() != CNatSocketBase::STATE_DISCONNECTED)
    {        
        runResult = m_udt.Run();
        if (RUN_DO_FAILED == runResult)
        {
            ChangeDisConnected();
        }
    }
    return RUN_NONE;
}

void CNatUdtDeviceClient::ChangeDisConnected()
{
    assert(IsInited());
    if (CNatSocketBase::STATE_DISCONNECTED != GetState())
    {
        SocketState oldState = GetState();
        ChangeState(CNatSocketBase::STATE_DISCONNECTED);
    }
}

////////////////////////////////////////////////////////////////////////////////
//class CNatDevicePeer implements

CNatDevicePeer::CNatDevicePeer()
:
m_started(false),
m_connectNotifier(NULL),
m_udtClients(),
m_udtTransport(),
m_workThreadRunning(false),
m_workThreadID(PUB_THREAD_ID_NOINIT),
m_clientsLock(),
m_notifierLock(),
m_udtTransportDispatch(),
m_traversalDevicePeer()
{
    m_udtTransportDispatch.Init(this);
}

CNatDevicePeer::~CNatDevicePeer()
{
}

int CNatDevicePeer::ResetServer( const char *serverIp, unsigned short serverPort )
{
	return m_traversalDevicePeer.ResetNatServer(serverIp, serverPort);
}

void CNatDevicePeer::SetDeviceExtraInfo(const char* extraInfo)
{
	m_traversalDevicePeer.SetDeviceExtraInfo(extraInfo); 
}

int CNatDevicePeer::Start( const NAT_DEVICE_CONFIG_EX *config)
{
    int ret = 0;
	strcpy(m_deviceNo, config->deviceNo);
	strcpy(m_serverIp, config->serverIp);
	m_serverPort = config->serverPort;

	NAT_UDT_TRANSPORT_CONFIG udtTransportConfig;
	udtTransportConfig.localIp = config->localIp;
	udtTransportConfig.localPort = config->localPort;

	ret = m_udtTransport.Start(&udtTransportConfig);
	if (ret != 0)
	{
		InternalStop();
		return ret;
	}
	m_udtTransport.SetNotifier(&m_udtTransportDispatch);

	CNatTraversalDevicePeer::REGISTER_CONFIG traversalConfig;
	memset(&traversalConfig, 0, sizeof(traversalConfig));
	strcpy(traversalConfig.deviceNo, config->deviceNo);
	strcpy(traversalConfig.serverIp, config->serverIp);
	traversalConfig.serverPort = config->serverPort;
	traversalConfig.refuseRelay = config->refuseRelay;
	strcpy(traversalConfig.caExtraInfo, config->caExtraInfo);

    if (!m_traversalDevicePeer.Start(this, &m_udtTransport, &traversalConfig))
    {
        InternalStop();
        return ret;        
    }
  
    m_workThreadID = PUB_CreateThread(WorkThreadFunc, (void *)this, &m_workThreadRunning);	

    if (PUB_THREAD_ID_NOINIT == m_workThreadID) 
    {
        InternalStop();
        return -1;
    }
	NAT_LOG_DEBUG("CNatDevicePeer start ok!\n");
    m_started = true;
    return 0;
}

int CNatDevicePeer::Stop()
{
    if (m_started)
    {
        InternalStop();
    }
    return 0;
}


void CNatDevicePeer::InternalStop()
{

    if (PUB_THREAD_ID_NOINIT != m_workThreadID) 
    {		
        PUB_ExitThread(&m_workThreadID, &m_workThreadRunning);
        m_workThreadID = PUB_THREAD_ID_NOINIT;
    }


    if(m_traversalDevicePeer.IsStarted())
    {
        m_traversalDevicePeer.Stop();
    }

    // destroy udp socket
    if (!m_udtClients.empty())
    {
        CNatScopeLock scopeLock(&m_clientsLock);
        for (std::vector<CNatUdtDeviceClient*>::iterator it = m_udtClients.begin(); 
            it != m_udtClients.end(); it++)
        {
            CNatUdtDeviceClient *udtClient = *it; 
            if (udtClient->IsUsing())
            {
                CNatSockManager::GetSocketManager()->Disable(udtClient);
            }
            delete udtClient;
        }
        m_udtClients.clear();
    }

	if (!m_RelayClients.empty())
	{
		 CNatScopeLock scopeLock(&m_clientsLock);
		 for (std::vector<CRelayObj*>::iterator it = m_RelayClients.begin(); 
			 it != m_RelayClients.end(); it++)
		 {
             CRelayObj *relayClient = *it;
             if (relayClient->IsUsing())
             {
                 CNatSockManager::GetSocketManager()->Disable(relayClient);
             }
			 delete relayClient;
		 }
		 m_RelayClients.clear();
	}
	
    if (m_udtTransport.IsStarted())
    {
        m_udtTransport.Stop();
    }
}

bool CNatDevicePeer::IsStarted()
{
    return m_started;
}

void CNatDevicePeer::SetConnectNotifier( CNatSocketConnectNotifier* notifier )
{
    m_connectNotifier = notifier;
}

CNatSocketConnectNotifier* CNatDevicePeer::GetConnectNotifier()
{
    return m_connectNotifier;
}

int  CNatDevicePeer::OnP2PClientConnect(unsigned long connectionId,unsigned long clientPeerIp,unsigned short clientPeerPort)
{
    CNatUdtDeviceClient *client;
    client = FindUdtClient(connectionId,clientPeerIp, clientPeerPort);
    if (client == NULL)
    {
        client = CreateUdtClient(connectionId,clientPeerIp, clientPeerPort, CNatUdtDeviceClient::MODE_TRAVERSAL); 
        NAT_LOG_INFO("New Traversal P2P Client Enter! Peer=%s:%d, Cid=%d\n",
            Nat_inet_ntoa(clientPeerIp), clientPeerPort, connectionId); 
        return 0;
    }	
    return -1;
}

void CNatDevicePeer::OnRelayClientConnect( unsigned long connectionId, 
                                          unsigned long relayServerIp, unsigned short relayServerPort )
{
    CRelayObj *client;
    client = FindRelayClient(connectionId);
    if (client == NULL)
    {
        client = CreateRelayClient(connectionId, relayServerIp, relayServerPort);
        NAT_LOG_INFO("New Relay Client Enter! Peer=%s:%d, Cid=%d\n",
            Nat_inet_ntoa(relayServerIp), relayServerPort, connectionId); 
    }	
}

CNatUdtDeviceClient* CNatDevicePeer::FindUdtClient(unsigned long connectionId,unsigned long fromIp, unsigned short fromPort)
{
    for (std::vector<CNatUdtDeviceClient*>::iterator it = m_udtClients.begin(); 
        it != m_udtClients.end(); it++)
    {
        CNatUdtDeviceClient* client = *it;
        if (client->GetState() != CNatSocketBase::STATE_CLOSED 
            && client->GetState() != CNatSocketBase::STATE_DISCONNECTED
            && client->GetConnectionId() == connectionId
            && client->GetRemoteIp() == fromIp
            && client->GetRemotePort() == fromPort)
        {
            return client;
        }
    }
    return NULL;
}

CNatUdtDeviceClient* CNatDevicePeer::FindUdtClient( const NAT_TRANSPORT_RECV_DATAGRAM* datagram )
{
    for (std::vector<CNatUdtDeviceClient*>::iterator it = m_udtClients.begin(); 
        it != m_udtClients.end(); it++)
    {
        CNatUdtDeviceClient* client = *it;
        if (client->GetState() != CNatSocketBase::STATE_CLOSED 
            && client->GetState() != CNatSocketBase::STATE_DISCONNECTED
            && client->IsMineDatagram(datagram))
        {
            return client;
        }
    }
    return NULL;
}

CNatUdtDeviceClient* CNatDevicePeer::FindUdtTraversalClient(const NAT_TRANSPORT_RECV_DATAGRAM* datagram)
{
    for (std::vector<CNatUdtDeviceClient*>::iterator it = m_udtClients.begin(); 
        it != m_udtClients.end(); it++)
    {
        CNatUdtDeviceClient* client = *it;
        if (client->IsSymmetricClientDatagram(datagram))
        {
            return client;
        }
    }
    return NULL;
}

CRelayObj* CNatDevicePeer::FindRelayClient( unsigned long connectionID )
{
	for (std::vector<CRelayObj*>::iterator it = m_RelayClients.begin(); 
		it != m_RelayClients.end(); it++)
	{
		CRelayObj* client = *it;
		if (client->GetcnnectionID() == connectionID)
		{
			return client;
		}
	}
	return NULL;
}

CNatUdtDeviceClient* CNatDevicePeer::CreateUdtClient(unsigned long connectionID, 
    unsigned long fromIP, unsigned short fromPort, CNatUdtDeviceClient::CLIENT_COME_IN_MODE comeInMode)
{
    CNatUdtDeviceClient* client = new CNatUdtDeviceClient(this, connectionID,fromIP, fromPort, comeInMode);
    if (client != NULL)
    {
        m_udtClients.push_back(client);
    }
    return client;
}


CRelayObj* CNatDevicePeer::CreateRelayClient( unsigned long connectionID, unsigned long serverIp,unsigned short serverPort )
{
	CRelayObj* client = new CRelayObj(connectionID, serverIp, serverPort, RELAY_ROLE_DEVICE);
	if (client != NULL)
	{
		if (!client->Initial())
		{
			delete client;
			client = NULL;
			return NULL;
		}
		assert(client->GetState() == CNatSocketBase::STATE_INIT);
		client->SetRelayNotifier(this);
		m_RelayClients.push_back(client);
	}
	return client;
}

void CNatDevicePeer::NotifyOnConnect(CNatSocketBase* sockBase)
{
    CNatScopeLock lock(&m_notifierLock);
    if( m_connectNotifier != NULL)
    {
        sockBase->Use();
        NatSocket sock = CNatSockManager::GetSocketManager()->Add(sockBase);
        if (sock != NAT_SOCKET_INVALID)
        {
            m_connectNotifier->OnConnect(sock, 0);
        }
    }
}

PUB_THREAD_RESULT PUB_THREAD_CALL CNatDevicePeer::WorkThreadFunc( void *pParam )
{
#ifdef	__ENVIRONMENT_LINUX__
    NAT_LOG_INFO("CNatDevicePeer thread pid = %d\n", getpid());
#endif
    assert(NULL != pParam);
    CNatDevicePeer *devicePeer = reinterpret_cast<CNatDevicePeer*>(pParam);
    devicePeer->RunWork();

    return 0;
}

int CNatDevicePeer::RunWork()
{
    NatRunResult runResult;
    unsigned long curTickCount = 0;

    while(m_workThreadRunning)
    {
        runResult = RUN_NONE;
        curTickCount = Nat_GetTickCount();

        // run clear closed clients
        runResult = NatRunResultMax(runResult, ClearClosedClients());

        // run udt transport, it will handle recv udp data , and then notify OnRecvPacket
        runResult = NatRunResultMax(runResult, m_udtTransport.Run());

		if(m_traversalDevicePeer.IsStarted())
		{
			runResult = NatRunResultMax(runResult, m_traversalDevicePeer.Run(curTickCount));
		}

        // run udt clients
        if (!m_udtClients.empty())
        {
            m_clientsLock.Lock();

            // delete udt clients

            // run udt clients
            for (std::vector<CNatUdtDeviceClient*>::iterator it = m_udtClients.begin(); 
                it != m_udtClients.end(); it++)
            {
                runResult = NatRunResultMax(runResult, (*it)->Run());
            }
            m_clientsLock.UnLock();
        }

		//run relay clients
		if (!m_RelayClients.empty())
		{
			m_clientsLock.Lock();
			std::vector<CRelayObj*>::iterator it = m_RelayClients.begin(); 
			for (; it!=m_RelayClients.end(); ++it)
			{
				runResult = NatRunResultMax(runResult, (*it)->Run());
			}
			m_clientsLock.UnLock();
		}

        if (RUN_NONE == runResult)
        {
            PUB_Sleep(10); // TODO: 10ms ?
        }

    }
    return 0;
}

NatRunResult CNatDevicePeer::ClearClosedClients()
{
    if (!m_udtClients.empty())
    {
        std::vector<CNatUdtDeviceClient*>::iterator it;
        CNatScopeLock lock(&m_clientsLock);
        for (it = m_udtClients.begin(); it != m_udtClients.end(); )
        {
            CNatUdtDeviceClient* client = *it;

			//当由上层关闭socketBase的时候，IsUsing置为false，这时为了保证线程安全，不会发送RST或关闭socket，
			//发送RST或关闭socket由CNatSocketBase的析构实现；另外，当处于STATE_INIT状态时，不会把socketBase回调出去
			if (!client->IsUsing() && (client->GetState() != CNatSocketBase::STATE_INIT) && (client->GetState() != CNatSocketBase::STATE_CONNECTING))
            {
                NAT_LOG_INFO("P2P Client Leave! Peer=%s:%d, Cid=%d\n",
                    Nat_inet_ntoa(client->GetRemoteIp()), client->GetRemotePort(), client->GetConnectionId());                  
                it = m_udtClients.erase(it);
                delete client;
				client = NULL;
            }
            else
            {
                it++;
            }
        }
    }

	if (!m_RelayClients.empty())
	{
		std::vector<CRelayObj*>::iterator it;
		CNatScopeLock lock(&m_clientsLock);
		for (it = m_RelayClients.begin(); it != m_RelayClients.end(); )
		{
			CRelayObj* client = *it;
			if (!client->IsUsing() && (client->GetState() != CNatSocketBase::STATE_INIT) && (client->GetState() != CNatSocketBase::STATE_CONNECTING))
            {
                NAT_LOG_INFO("Relay Client Leave! Peer=%s:%d, Cid=%d\n",
                    Nat_inet_ntoa(client->GetRemoteIp()), client->GetRemotePort(), client->GetcnnectionID());    
				it = m_RelayClients.erase(it);
				delete client;
				client = NULL;
			}
			else
			{
				it++;
			}
		}
	}

    return RUN_NONE;
}

void CNatDevicePeer::RelayOnConnect( CNatSocketBase* pSockBase, int iErrorCode )
{
    if(iErrorCode == 0)
    {
	    NotifyOnConnect(pSockBase);
    }
}

NAT_DEVICE_SERVER_ERROR CNatDevicePeer::GetServerError()
{
	return m_traversalDevicePeer.GetServerError();
}

NAT_DEVICE_SERVER_STATE CNatDevicePeer::GetServerState()
{
	return m_traversalDevicePeer.GetServerState();
}