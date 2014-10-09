#include "DeviceConnector.h"
#include "DeviceSearcher.h"
#include "NatDeviceFetcher.h"
#include "SWL_ConnectProcEx.h"
#include "NatConnectProc.h"

const char NAT_SER_ADDR[] = "115.28.100.185";
//const char NAT_SER_ADDR[] = "192.168.2.167";
const unsigned short NAT_SER_PORT = 8989;

const unsigned long  TCP_TIME_OUT = 5000;
const unsigned long  UDT_TIME_OUT = TCP_TIME_OUT*2;
const unsigned long  TCP_EARLIER_TIME = 3000;

static int TcpLinkCallBack(SWL_socket_t sock, void *pObject, void *pParam)
{
	assert(NULL != pObject);
	CDeviceConnector *pDeviceConnector = reinterpret_cast<CDeviceConnector *>(pObject);
	TVT_SOCKET tvtsock(sock);
	
	if (sock != SWL_INVALID_SOCKET)
	{
		pDeviceConnector->OnConnect(tvtsock,true);
	}
	
	return 0;
}

static int UdtLinkCallBack(NatSocket sock, void *pObject, void *pParam)
{
	assert(NULL != pObject);
	CDeviceConnector *pDeviceConnector = reinterpret_cast<CDeviceConnector *>(pObject);
	TVT_SOCKET tvtsock(sock);
	
	if (sock != NAT_SOCKET_INVALID)
	{
		pDeviceConnector->OnConnect(tvtsock,false);
	}
	
	return 0;
}


CDeviceConnector::CDeviceConnector(CSWL_MultiNet *pTcpMultiNet,CSWL_MultiNet *pNatMultiNet,std::string serialNo)
:m_pTcpMultiNet(pTcpMultiNet),
m_pTcpConnectProc(NULL),
m_pNatMultiNet(pNatMultiNet),
m_pNatConnectProc(NULL),
m_serialNo(serialNo),
m_bConnected(false),
m_bTcp(false)
{
}

CDeviceConnector::~CDeviceConnector()
{
	if (NULL != m_pNatConnectProc)
	{
		m_pNatConnectProc->Destroy();
		m_pNatConnectProc = NULL;
	}

	if (NULL != m_pTcpConnectProc)
	{
		m_pTcpConnectProc->Destroy();
		m_pTcpConnectProc = NULL;
	}
}

bool CDeviceConnector::ConnectDevice(CONNECT_UNIT *pUnit)
{
	bool ret = false;

	if (NULL == pUnit)
	{
		return ret;
	}

	switch(InitConnetMode())
	{
	case CONNECT_MODE_TCP:
		ret = ConnectTcpMode(pUnit);
		break;
	case CONNECT_MODE_UDT:
		ret = ConnectUdtMode(pUnit);
		break;
	case CONNECT_MODE_MIX:
		ret = ConnectMixMode(pUnit);
		break;
	default:
		ret = false;
		break;
	}

	return ret;
}

void CDeviceConnector::OnConnect(TVT_SOCKET sock,bool bTcp)
{
	m_lock.Lock();
	if (!m_bConnected)
	{
		m_sock = sock;
		m_bTcp = bTcp;
		m_bConnected = true;
	}
	m_lock.UnLock();
}


CONNECT_MODE CDeviceConnector::InitConnetMode()
{
	TVT_DISCOVER_DEVICE deviceInfo;
	memset(&deviceInfo,0,sizeof(deviceInfo));

	NAT_DEVICE_FETCHER_CONFIG fectherConfig;
	memset(&fectherConfig,0,sizeof(fectherConfig));
	strcpy(fectherConfig.pServerIp,NAT_SER_ADDR);
	strcpy(fectherConfig.dwDeviceNo,m_serialNo.c_str());
	fectherConfig.nServerPort = NAT_SER_PORT;
	CNatDeviceFetcher *pDeviceFetcher = CNatDeviceFetcher::NewDeviceFetcher(&fectherConfig);

	NAT_DEVICE_INFO natDeviceInfo;
	memset(&natDeviceInfo,0,sizeof(natDeviceInfo));

	CONNECT_MODE connectMode = CONNECT_MODE_NONE;

	if(CDeviceSearcher::Instance().GetDeviceInfo(m_serialNo.c_str(),deviceInfo))
	{
		m_addr = deviceInfo.ip;
		m_port = deviceInfo.port;
		connectMode = CONNECT_MODE_TCP;
	}
	else if(pDeviceFetcher->FetchSyn(UDT_TIME_OUT,&natDeviceInfo) == 0)
	{
		if (natDeviceInfo.usDeviceDataPort)
		{
			struct sockaddr_in sockaddrIn;
			sockaddrIn.sin_addr.s_addr = natDeviceInfo.dwDevicePeerIp;
			m_addr = inet_ntoa(sockaddrIn.sin_addr);
			m_port = natDeviceInfo.usDeviceDataPort;
			connectMode = CONNECT_MODE_MIX;
		}
		else
		{
			m_addr = NAT_SER_ADDR;
			m_port = NAT_SER_PORT;
			connectMode = CONNECT_MODE_UDT;
		}
	}
	else
	{
		connectMode = CONNECT_MODE_NONE;
	}

	pDeviceFetcher->Destroy();

	return connectMode;
}

bool CDeviceConnector::ConnectTcpMode(CONNECT_UNIT *pUnit)
{
	InitTcpConnectProc();
	WaitForConnect(TCP_TIME_OUT);

	if (m_bConnected)
	{
		assert(m_bTcp);
		pUnit->pMultiNet = m_pTcpMultiNet;
		pUnit->sock = m_sock;
	}

	return m_bConnected;
}

bool CDeviceConnector::ConnectUdtMode(CONNECT_UNIT *pUnit)
{
	InitUdtConnectProc();
	WaitForConnect(UDT_TIME_OUT);

	if (m_bConnected)
	{
		assert(!m_bTcp);
		pUnit->pMultiNet = m_pNatMultiNet;
		pUnit->sock = m_sock;
	}

	return m_bConnected;
}

bool CDeviceConnector::ConnectMixMode(CONNECT_UNIT *pUnit)
{
	InitTcpConnectProc();
	WaitForConnect(TCP_EARLIER_TIME);
	if (m_bConnected)
	{
		assert(m_bTcp);
		pUnit->pMultiNet = m_pTcpMultiNet;
		pUnit->sock = m_sock;
	
		return true;
	}
	
	InitUdtConnectProc();
	WaitForConnect(UDT_TIME_OUT);

	if (m_bConnected)
	{
		if (m_bTcp)
		{
			pUnit->pMultiNet = m_pTcpMultiNet;
		}
		else
		{
			pUnit->pMultiNet = m_pNatMultiNet;
		}	
		pUnit->sock = m_sock;
	}

	return m_bConnected;
}

void CDeviceConnector::WaitForConnect(int timeouts)
{
	int count = 0;
	while(!m_bConnected && count < timeouts)
	{
		PUB_Sleep(10);
		count += 10;
	}
}

void CDeviceConnector::InitUdtConnectProc()
{
	NAT_CLIENT_CONFIG clientConfig;
	memset(&clientConfig,0,sizeof(clientConfig));

	clientConfig.ConfigType = TRAVERSAL_CONFIG;
	strcpy(clientConfig.TraversalConfig.dwDeviceNo,m_serialNo.c_str());
	strcpy(clientConfig.TraversalConfig.pServerIp,NAT_SER_ADDR);
	clientConfig.TraversalConfig.nServerPort = NAT_SER_PORT;

	m_pNatConnectProc = CNatConnectProc::NewConnectProc(&clientConfig);
	m_pNatConnectProc->ConnectAsyn(UdtLinkCallBack,this,UDT_TIME_OUT);
}

void CDeviceConnector::InitTcpConnectProc()
{
	m_pTcpConnectProc = CSWL_ConnectProcEx::NewConnectProc(m_addr.c_str(),m_port);
	m_pTcpConnectProc->ConnectAsyn(TcpLinkCallBack,this,TCP_TIME_OUT);
}