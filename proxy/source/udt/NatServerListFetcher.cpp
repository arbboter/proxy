
#include "NatCommon.h"
#include "NatServerListFetcher.h"
#include "NatTestProtocol.h"
#include "SWL_Public.h"
#include "NatXml.h"
#include "NatUdtTransport.h"

#include "NatDebug.h"

#define SERVER_LIST_PATH "http://%s/NatServer/NatServerList.xml"
#define RET_FAILED -1
#define RET_NET_ERR -2
#define RET_SUCCESS 0

#define XML_SERV_INFO_NODE "Item"
#define XML_IP_NAME "Addr"
#define XML_NAT_PORT "Port"
#define XML_TEST_PORT "TestPort"

#define DEF_NAT_PORT 8989
#define DEF_TEST_PORT 8993
#define TEST_SPEED_DATA "abcdefghijklmnopqrstuvwxyz"

using namespace std;




static const int RAND_BIND_TIMES_MAX = 3;
static const int SEND_TEST_SPEED_COUNT = 3;
static const int SEND_TEST_SPEED_INTERVAL = 1000; // ms


CNatServerListFetcher::CNatServerListFetcher()
	:m_isCompleted(true),
	m_pHttp(NULL),
	m_isTestSpeed(false),
	m_workThreadID(PUB_THREAD_ID_NOINIT),
	m_workThreadRunning(false)
{
	memset(m_szDefaultServerAddr, 0, sizeof(m_szDefaultServerAddr));
	m_isCompleted = true;
}

CNatServerListFetcher::~CNatServerListFetcher()
{
	Clear();
}

void CNatServerListFetcher::SetDefaultServerAddr( const char* addr )
{
	if (addr != NULL)
	{
		strcpy(m_szDefaultServerAddr, addr);
	}
}

void CNatServerListFetcher::SetServerList(const NAT_SERVER_INFO* pServerList, int iCount)
{
	m_cacheServerVector.clear();
	if (pServerList != NULL && iCount > 0)
	{
		for (int i = 0; i < iCount; ++i)
		{
			m_cacheServerVector.push_back(pServerList[i]);
		}
	}
}

void CNatServerListFetcher::SetTestSpeed(bool bTestSpeed)
{
	m_isTestSpeed = bTestSpeed;
}


bool CNatServerListFetcher::Start()
{
	if(!m_isCompleted)
	{
		assert(false);
		return false;
	}
	m_retServerVector.clear();
	m_speedTestList.clear();
	m_downloadServerVector.clear();
    m_workThreadID = PUB_CreateThread(WorkThreadFunc, (void *)this, &m_workThreadRunning);	
    if (PUB_THREAD_ID_NOINIT == m_workThreadID) 
    {
        return false;
    }
	m_isCompleted = false;
	return true;
}

bool CNatServerListFetcher::IsCompleted()
{
	return m_isCompleted;
}

void CNatServerListFetcher::Cancel()
{
	Clear();
}

int CNatServerListFetcher::GetServerCount()
{
	CNatScopeLock lock(&m_lock);
	return m_retServerVector.size();
}

bool CNatServerListFetcher::GetServerInfo(NAT_SERVER_INFO *pServerInfo, int index)
{
	if (pServerInfo == NULL)
	{
		return false;
	}
	CNatScopeLock lock(&m_lock);
	if (index >= (int)m_retServerVector.size())
	{
		return false;
	}

	memcpy(pServerInfo, &m_retServerVector.at(index), sizeof(NAT_SERVER_INFO));
	return true;
}

bool CNatServerListFetcher::IsTestSpeed()
{
	return m_isTestSpeed;
}

PUB_THREAD_RESULT PUB_THREAD_CALL CNatServerListFetcher::WorkThreadFunc( void *pParam )
{
#ifdef	__ENVIRONMENT_LINUX__
    NAT_LOG_INFO("CNatServerListFetcher thread pid = %d\n", getpid());
#endif
    assert(NULL != pParam);
    CNatServerListFetcher *runner = reinterpret_cast<CNatServerListFetcher*>(pParam);
    runner->RunWork();

    return 0;
}

int CNatServerListFetcher::RunWork()
{
	bool ret = false;
    if (!m_cacheServerVector.empty())
	{
		for(NatServerVector::iterator iter = m_cacheServerVector.begin(); iter != m_cacheServerVector.end(); iter++)
		{
			ret = FetchListFromServ(m_downloadServerVector, iter->szAddr);
			if (ret)
			{
				break;
			}
		}
	}

	if (!ret)
    {
		ret = FetchListFromServ(m_downloadServerVector, m_szDefaultServerAddr);
    }

	if (ret)
	{
		if(m_isTestSpeed)
		{
			ProcTestSpeed();
		}
		else
		{
			CNatScopeLock lock(&m_lock);
			m_retServerVector = m_downloadServerVector;
		}
		m_downloadServerVector.clear();
	}
	m_workThreadID = PUB_THREAD_ID_NOINIT;
	m_workThreadRunning = false;
	m_isCompleted = true;
    return ret;
}


void CNatServerListFetcher::ProcTestSpeed()
{
	if (!InitSpeedTestList(m_downloadServerVector))
	{
		return;
	}

	SWL_socket_t sock = SWL_INVALID_SOCKET;
	unsigned short localPort;
	int randBindTimes = 0;
	Nat_SRand();
	while(SWL_INVALID_SOCKET == sock && randBindTimes < RAND_BIND_TIMES_MAX)
	{
		localPort = GenRandomPort();
		sock = CreateAndBindUdpSock(0, localPort);
		randBindTimes++;
	}

	if (sock == SWL_INVALID_SOCKET)
	{
		return;
	}

	unsigned long startTime = Nat_GetTickCount();
	unsigned long curTickCount = startTime;
	
	while(m_workThreadRunning)
	{
		curTickCount = Nat_GetTickCount();
		if (m_speedTestList.size() > 0)
		{
			for (NatServerList::iterator it = m_speedTestList.begin(); it != m_speedTestList.end(); ++it)
			{
				SERVER_SPEED_INFO &speedInfo = *it;
				if (speedInfo.sendCount < SEND_TEST_SPEED_COUNT 
					&& (speedInfo.sendCount == 0 || PUB_IsTimeOutEx(speedInfo.lastSendTime, SEND_TEST_SPEED_INTERVAL, curTickCount)))
				{
					int nMsgLen = 0;
					NAT_TEST_SPEED_REQ testReq = {0};
					memset(&testReq, 0, sizeof(NAT_TEST_SPEED_REQ));
					testReq.header.cmdId = NAT_TEST_ID_SPEED_REQ;
					testReq.header.seq = speedInfo.sendCount;
					memcpy(testReq.header.startTag, NAT_TEST_PROT_START_TAG, sizeof(NAT_TEST_PROT_START_TAG));
					testReq.header.versionHigh = NAT_TEST_PROT_VER_HIGH;
					testReq.header.versionLow = NAT_TEST_PROT_VER_LOW;
					testReq.timestamp = curTickCount;
					testReq.mode = NAT_TEST_SPEED_MODE_ECHO;
					strcpy(testReq.testData, TEST_SPEED_DATA);
					nMsgLen = sizeof(NAT_TEST_SPEED_REQ) - sizeof(testReq.testData) + strlen(testReq.testData);

					int sendRet = NatUdpSendTo(sock, &testReq, nMsgLen, speedInfo.remoteIp, speedInfo.serverInfo.sTestPort);
					if (sendRet > 0)
					{
						speedInfo.sendCount++;
						speedInfo.lastSendTime = curTickCount;
					}
					else
					{
						speedInfo.lastSendTime += 10;
					}					
				}
			}
		}


		NAT_TEST_SPEED_REPLY reply; 
		unsigned long remoteIp;
		unsigned short remotePort;
		int recvRet = NatUdpRecvfrom(sock, &reply, sizeof(reply), &remoteIp, &remotePort, 10);
		if (recvRet > 0)
		{
			for (NatServerList::iterator it = m_speedTestList.begin(); it != m_speedTestList.end(); it++)
			{
				SERVER_SPEED_INFO &speedInfo = *it;
				if (remoteIp == speedInfo.remoteIp && remotePort == speedInfo.serverInfo.sTestPort)
				{
					m_lock.Lock();
					m_retServerVector.push_back(speedInfo.serverInfo);
					m_lock.UnLock();
					m_speedTestList.erase(it);
					break;
				}
			}
		}
		else //recvRet <= 0
		{
			// ignore
		}
		if (m_speedTestList.empty() || PUB_IsTimeOutEx(startTime, 10000, curTickCount))
		{
			break;
		}

	}

	{
		CNatScopeLock lock(&m_lock);
		if (!m_speedTestList.empty())
		{
			for (NatServerList::iterator it = m_speedTestList.begin(); it != m_speedTestList.end(); ++it)
			{
				SERVER_SPEED_INFO &speedInfo = *it;
				m_retServerVector.push_back(speedInfo.serverInfo);
			}
		}
	}
	m_speedTestList.clear();

}

bool CNatServerListFetcher::InitSpeedTestList(NatServerVector &serverVector)
{
	if (serverVector.empty())
	{
		return false;
	}
	m_speedTestList.clear();
	SERVER_SPEED_INFO speedInfo;
	for (NatServerVector::iterator it = serverVector.begin(); it != serverVector.end(); ++it)
	{
		speedInfo.serverInfo = *it;
		speedInfo.remoteIp = inet_addr(speedInfo.serverInfo.szAddr);
		speedInfo.lastSendTime = 0;
		speedInfo.sendCount = 0;
		speedInfo.hasReceived = false;
		m_speedTestList.push_back(speedInfo);
	}
	return true;
}

//解析服务器信息
int CNatServerListFetcher::ParseServInfo(TiXmlElement* pElement, NAT_SERVER_INFO* pServInfo)
{
	TiXmlElement* pElementTmp = NULL;
	if((NULL == pElement) || (NULL == pServInfo))
	{
		return RET_FAILED;
	}

	//IP
	pElementTmp = pElement->FirstChildElement(XML_IP_NAME);
	if(NULL == pElementTmp)
	{
		return RET_FAILED;
	}	
	strncpy(pServInfo->szAddr, pElementTmp->GetText(), sizeof(pServInfo->szAddr) - 1);

	//nat port
	pElementTmp = pElement->FirstChildElement(XML_NAT_PORT);
	if(NULL != pElementTmp)
	{
		pServInfo->sPort = atoi(pElementTmp->GetText());
	}

	//test port
	pElementTmp = pElement->FirstChildElement(XML_TEST_PORT);
	if(NULL != pElementTmp)
	{
		pServInfo->sTestPort = atoi(pElementTmp->GetText());
	}

	return RET_SUCCESS;
}

//解析服务器列表,XmlType=0从文件加载，否则从内存加载
bool CNatServerListFetcher::ParseServList(NatServerVector &serverVector, const char* pXmlSrc, int nXmlType)
{
	TiXmlDocument* pXmlDoc = NULL;
	TiXmlElement* pElement = NULL;
	TiXmlElement* pRootElement = NULL;
	NAT_SERVER_INFO servInfo = {0};
	int nRet = false;

	if(NULL == pXmlSrc)
	{
		return false;
	}

	pXmlDoc = new TiXmlDocument();
	if(NULL == pXmlDoc)
	{
		return false;
	}


	nRet = pXmlDoc->LoadXML(pXmlSrc);	
	if(!nRet)
	{
		delete pXmlDoc;
		return false;
	}

	pRootElement = pXmlDoc->RootElement();
	if(NULL == pRootElement)
	{
		delete pXmlDoc;
		return false;
	}

	pElement = pRootElement->FirstChildElement(XML_SERV_INFO_NODE);
	if(NULL == pElement)
	{
		delete pXmlDoc;
		return false;
	}

	for(;pElement; pElement = pElement->NextSiblingElement())
	{
		memset(&servInfo, 0, sizeof(NAT_SERVER_INFO));
		servInfo.sPort = DEF_NAT_PORT;
		servInfo.sTestPort = DEF_TEST_PORT;
		nRet = ParseServInfo(pElement, &servInfo);
		if(nRet < 0)
		{
			delete pXmlDoc;
			return false;
		}

		serverVector.push_back(servInfo);
	}

	delete pXmlDoc;
	return true;
}

//下载服务器列表
bool CNatServerListFetcher::FetchListFromServ(NatServerVector &serverVector, const char *pServerAddr)
{
	if(NULL == pServerAddr)
	{
		return false;
	}


	m_pHttp = new CTVTHttp();
	if(NULL == m_pHttp)
	{
		return false;
	}

	char szUrl[256] = {0};
	//先从本地列表下载
	memset(szUrl, 0, sizeof(szUrl));
	snprintf(szUrl, sizeof(szUrl) - 1, SERVER_LIST_PATH, pServerAddr);

	bool ret = false;
	//下载
	if( m_pHttp->Get(szUrl))
	{			
		ret = ParseServList(serverVector, m_pHttp->GetRetBody(), 1);
	}

	m_pHttp->Shutdown();
	delete m_pHttp;
	m_pHttp = NULL;

	return ret;
}


//清空
void CNatServerListFetcher::Clear()
{

	if (m_workThreadID != PUB_THREAD_ID_NOINIT)
	{
		PUB_ExitThread(&m_workThreadID, &m_workThreadRunning);
		m_workThreadID = PUB_THREAD_ID_NOINIT;
	}

	if(m_pHttp != NULL)
	{
		delete m_pHttp;
		m_pHttp = NULL;
	}
	m_retServerVector.clear();
	m_cacheServerVector.clear();
	m_downloadServerVector.clear();
	m_speedTestList.clear();
	m_isCompleted = true;
}
