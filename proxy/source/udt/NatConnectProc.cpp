// NatConnectProc.cpp: implementation of the CNatConnectProc class.
//
//////////////////////////////////////////////////////////////////////
#include "NatCommon.h"
#include "NatConnectProc.h"
#include "NatClientPeerManager.h"
#include <string.h>

#ifdef	__ENVIRONMENT_LINUX__
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#include "NatUdtClientPeer.h"
#include "NatTraversalClientPeer.h"

#include "NatDebug.h"

class CNatConnectProc::COnConnectNotifier: public CNatSocketConnectNotifier
{
public:
    COnConnectNotifier(CNatConnectProc *connectProc)
        :m_connectProc(connectProc)
    {
    }
    virtual void OnConnect(NatSocket sock, int iErrorCode)
    {
        m_connectProc->NotifyOnConnect(sock, (NAT_CLIENT_ERROR)iErrorCode);
    };

private:
    CNatConnectProc *m_connectProc;
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNatConnectProc::CNatConnectProc(const NAT_CLIENT_CONFIG &config)
:
m_onConnectNotifier(NULL),
m_state(STATE_NONE),
m_isConnectSync(false),
m_syncSock(NAT_SOCKET_INVALID),
m_connectTraversalMode(TRAVERSAL_P2P),
m_pClientPeer(NULL),
m_pNatTraversalClientPeer(NULL),
m_config(config),
m_clientState(NAT_CLI_STATE_INIT)
{    
}

CNatConnectProc::~CNatConnectProc()
{
    Clear();
    if (m_onConnectNotifier != NULL)
    {
        delete m_onConnectNotifier;
    }
}

NAT_CLIENT_ERROR CNatConnectProc::GetConnectError()
{
	return m_connectError;
}

NAT_CLIENT_STATE CNatConnectProc::GetConnectState()
{
	return m_clientState;
}

CNatConnectProc* CNatConnectProc::NewConnectProc(const NAT_CLIENT_CONFIG *config)
{
    CNatConnectProc* connectProc = new CNatConnectProc(*config);
    if (connectProc != NULL)
    {
        if(!connectProc->Init())
        {
            delete connectProc;
            connectProc = NULL;
        }
    }
	return  connectProc;
}

CNatConnectProc*CNatConnectProc::NewConnectProc(const NAT_CLIENT_DIRECT_CONFIG *config)
{
    NAT_CLIENT_CONFIG clientConfig;
    clientConfig.ConfigType = DIRECT_CONFIG;
	memcpy(&clientConfig.DirectConfig,config,sizeof(NAT_CLIENT_DIRECT_CONFIG));
	return  NewConnectProc(&clientConfig);
}

int CNatConnectProc::Destroy()
{
	delete this;
	return 0;
}


NatSocket CNatConnectProc::ConnectSyn( int timeOut )
{
	return ConnectSyn(timeOut, NULL);
}

NatSocket CNatConnectProc::ConnectSyn( int timeOut, bool *cancel )
{
	int iRet = 0;
	NAT_LOG_INFO("CNatConnectProc start new connect\n");
		 
    if (m_state != STATE_NONE)
    {
        NAT_LOG_WARN("CNatConnectProc is connecting, can not call connect duplicately!\n");
        return NAT_SOCKET_INVALID;
    }

    m_isConnectSync = true;
    m_syncSock = NAT_SOCKET_INVALID;
    m_state = STATE_CONNECTING;
	bool isCenceled = false;

    iRet = CreateNatSocket(timeOut);
	if(iRet == 0)
    {
        while (true)
        {
            m_ConnectSynLock.Lock();
            ConnectState theState = m_state;
            m_ConnectSynLock.UnLock();
			if (theState == STATE_FINISHED)
			{
				break;
			} 
			if (cancel != NULL && *cancel)
			{
				isCenceled = true;
				break;
			}

            PUB_Sleep(10); // TODO: how long to wait?
        }
    }
    else
	{
		NAT_LOG_WARN("CNatConnectProc create nat client peer failed!\n");
		return NAT_SOCKET_INVALID;
	}

    Clear();
	if (isCenceled)
	{
		NAT_LOG_INFO("CNatConnectProc has been canceled by caller!\n");
		if (m_syncSock != NAT_SOCKET_INVALID)
		{
			// after Clear(), no need to lock protecting m_syncSock
			NAT_CloseSocket(m_syncSock);
			m_syncSock = NULL;
		}
	}
    m_state = STATE_NONE;
    return m_syncSock;
}

void  CNatConnectProc::NotifyOnConnect(NatSocket sock, NAT_CLIENT_ERROR iErrorCode)
{
    m_connectError = iErrorCode;

	if(m_config.ConfigType == TRAVERSAL_CONFIG)
	{
		if(NULL!=m_pNatTraversalClientPeer)
		{
			m_clientState = m_pNatTraversalClientPeer->GetClientState();
		}
	}
	
    if (sock != NAT_SOCKET_INVALID)
    {
        assert(m_connectError == NAT_CLI_OK);
        NAT_LOG_INFO("CNatConnectProc connect device succeed!\n");
    }
    else
    {
        assert(m_connectError != NAT_CLI_OK);
        NAT_LOG_INFO("CNatConnectProc connect device failed! error=%s\n", 
            NAT_GetClientErrorText(m_connectError));
    }

    if (m_isConnectSync)
    {
        m_ConnectSynLock.Lock();
        m_syncSock = sock;
        m_state = STATE_FINISHED;
        m_ConnectSynLock.UnLock();
    } 
    else
    {
        if (NULL != m_pConnectCallback)
        {
            m_pConnectCallback(sock, m_pConnectCallbackParam, NULL);
        }
        m_state = STATE_NONE;
    }


}
int CNatConnectProc::ConnectAsyn(NAT_CONNECT_LINK_CALLBACKEX pCallback, void* pObject, int iTimeOut)
{
    if (m_state != STATE_NONE)
    {
        NAT_LOG_WARN("CNatConnectProc is connecting, can not call connect duplicately!\n");
        assert(false);
        return -1;
    }

    m_isConnectSync = false;
    m_syncSock = NAT_SOCKET_INVALID;
    m_state = STATE_CONNECTING;
	int ret = 0;
    m_pConnectCallback      =  pCallback;
    m_pConnectCallbackParam =  pObject;
    m_iTimeOut			   =  iTimeOut;
    ret = CreateNatSocket(iTimeOut);
    if (ret <0)
    {        
        Clear();
        return -1;
    }
    return ret;
}
void CNatConnectProc::SetConnectTraversalMode(TraversalMode mode)
{
	m_connectTraversalMode = mode;
}

int CNatConnectProc::CreateNatSocket(int iTimeOut)
{
	int ret = 0;
    if (m_config.ConfigType == DIRECT_CONFIG)
    {
        m_pClientPeer     = new CNatUdtClientPeer();
        m_pClientPeer->SetConnectTimeout(iTimeOut);
        m_pClientPeer->SetConnectNotifier(m_onConnectNotifier);

        ret = m_pClientPeer->Init(&m_config.DirectConfig);
        if (0 != ret)
        {
            delete m_pClientPeer;
            m_pClientPeer = NULL;
            return -1;
        }
    }
    else if(m_config.ConfigType == TRAVERSAL_CONFIG)
    {
        m_pNatTraversalClientPeer = new CNatTraversalClientPeer();
        m_pNatTraversalClientPeer->SetStartTraversalMode(m_connectTraversalMode);
        m_pNatTraversalClientPeer->SetConnectTimeout(iTimeOut);
        m_pNatTraversalClientPeer->SetConnectNotifier(m_onConnectNotifier);

		ret = m_pNatTraversalClientPeer->Init(&m_config.TraversalConfig);
		if (0 != ret)
		{
			delete m_pNatTraversalClientPeer;
            m_pNatTraversalClientPeer = NULL;
			return -1;
		}
    }
    return 0;
}

bool CNatConnectProc::Init()
{
   m_onConnectNotifier = new COnConnectNotifier(this); 
   if (m_onConnectNotifier == NULL)
   {
       return false;
   }
   return true;
}

int CNatConnectProc::Clear()
{
    if(NULL != m_pClientPeer)
    {
        delete m_pClientPeer;
        m_pClientPeer =NULL;
    }

    if(NULL!= m_pNatTraversalClientPeer)
    {
        delete m_pNatTraversalClientPeer;
        m_pNatTraversalClientPeer =NULL;
    }
    m_state = STATE_NONE;
    return 0;
}
