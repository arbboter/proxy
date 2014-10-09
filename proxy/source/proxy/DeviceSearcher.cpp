#include "TVT_PubCom.h"
#include "Multicast.h"
#include "MulticastHeader.h"
#include "DeviceSearcher.h"
#include "json.h"
#include "ProxyProtocol.h"
#include <iostream>


CDeviceSearcher::CDeviceSearcher()
{
	m_bDVRSearching = false;
	m_serachDVRThreadHandle = PUB_THREAD_ID_NOINIT;
	m_bUpGrade = false;
}

CDeviceSearcher::~CDeviceSearcher()
{
	if (PUB_THREAD_ID_NOINIT != m_serachDVRThreadHandle)
	{
		StopSearchThread();
	}
}

CDeviceSearcher & CDeviceSearcher::Instance()
{
	static CDeviceSearcher deviceSearcher;
	return deviceSearcher;
}

bool CDeviceSearcher::StartSearchThread()
{
	m_serachDVRThreadHandle = PUB_CreateThread(SearchDVRThread, this, &m_bDVRSearching);

	if(PUB_CREATE_THREAD_FAIL == m_serachDVRThreadHandle)
	{
		m_serachDVRThreadHandle = PUB_THREAD_ID_NOINIT;
		return false;
	}

	return true;
}

bool CDeviceSearcher::StopSearchThread()
{

	if (PUB_THREAD_ID_NOINIT != m_serachDVRThreadHandle)
	{
		PUB_ExitThread(&m_serachDVRThreadHandle, &m_bDVRSearching);
	}

	return true;
}

void CDeviceSearcher::UpGradeLocalDevice(std::string sn,std::string param)
{
	m_devListLock.Lock();
	m_bUpGrade = true;
	m_upGradeSn = sn;
	m_upGradeParam = param;
	m_devListLock.UnLock();
}

void CDeviceSearcher::GetDeviceList(TVT_DEV_LIST &devList)
{
	m_devListLock.Lock();
	for (SEARCH_IPC_INFO_LIST::iterator iter = m_devList.begin(); iter != m_devList.end(); ++iter)
	{
		devList.push_back((*iter).info);
	}
	m_devListLock.UnLock();
}

bool CDeviceSearcher::GetDeviceInfo(const char *pSn,TVT_DISCOVER_DEVICE &devInfo)
{
	bool ret = false;
	
	unsigned long curTime = PUB_GetTickCount();

	if ((curTime > m_startTime) && ((m_startTime + LATEST_WAIT_TIME) > curTime))
	{
		PUB_Sleep(curTime-m_startTime);
	}

	m_devListLock.Lock();
	for (SEARCH_IPC_INFO_LIST::iterator iter = m_devList.begin(); iter != m_devList.end(); ++iter)
	{
		if (strcmp(pSn,(*iter).info.sn) == 0)
		{
			memcpy(&devInfo,&(*iter).info,sizeof(devInfo));
			ret = true;
			break;
		}
	}
	m_devListLock.UnLock();


	return ret;
}

PUB_THREAD_RESULT PUB_THREAD_CALL CDeviceSearcher::SearchDVRThread(void *pParam)
{
	CDeviceSearcher *pSearcher = reinterpret_cast<CDeviceSearcher *>(pParam);

	assert(NULL != pSearcher);
	if (NULL == pSearcher)
	{
		return 0;
	}

	pSearcher->SerachDVRPro();
	return 0;
}


void CDeviceSearcher::SerachDVRPro()
{
	CMulticast searcher;
	TVT_DISCOVER_DEVICE ipcInfo;
	struct sockaddr_in sockaddrIn;

	int dataBuffSize = 4*1024;
	char *pDataBuff = new  char[dataBuffSize];
	memset(pDataBuff, 0, dataBuffSize);

	//组播放请求命令和回复命令共用同一个BUFFER，这里事先把命令头以及请求和回复参数结构体的指针偏移好
	//这个是组播命令公共头信息
	TVT_MULTICAST_HEAD *pMulticastHead = (TVT_MULTICAST_HEAD *)pDataBuff;

	m_startTime = PUB_GetTickCount();

	while(m_bDVRSearching)
	{
		if (searcher.Init(TVT_MULTICAST_SEND_PORT,TVT_MULTICAST_GROUP_PORT,TVT_MULTICAST_GROUP_IP) < 0)
		{
			PUB_Sleep(1000);
			continue;
		}

		memset(pMulticastHead, 0, sizeof(TVT_MULTICAST_HEAD));
		pMulticastHead->headFlag= TVT_MULTICAST_HEAD_FLAG;
		pMulticastHead->cmd = TVT_MCMD_SEARCH;

		ClearTimeOutDevice();

		if (m_bUpGrade)
		{
			Upgrade(&searcher);
			m_bUpGrade = false;
		}

		searcher.SendTo(pDataBuff, sizeof(TVT_MULTICAST_HEAD));
		int ret = 0;
		unsigned int tryTimes = SEARCH_WAIT_TIME/50;

		while(tryTimes)
		{
			ret = searcher.RecvFrom(pDataBuff, dataBuffSize);
			
			if (ret < 0)
			{
				tryTimes--;
				continue;
			}

			if (ret >= sizeof(TVT_MULTICAST_HEAD))
			{
				//判断标记
				if(pMulticastHead->headFlag != TVT_MULTICAST_HEAD_FLAG)
				{
					//assert(false);
					continue;
				}

				//判断命令
				if(TVT_MCMD_INFO == pMulticastHead->cmd)
				{
					std::string devInfo(pDataBuff + sizeof(TVT_MULTICAST_HEAD),pMulticastHead->paramLen);

					Json::Reader reader;
					Json::Value rp;

					if (reader.parse(devInfo,rp) && !rp[MULTICAST_DEV_SN].asString().empty())
					{
						memset(&ipcInfo,0,sizeof(ipcInfo));
						
						strncpy(ipcInfo.sn,rp[MULTICAST_DEV_SN].asString().c_str(),DEV_SN_LEN);
						strncpy(ipcInfo.ip,rp[MULTICAST_IPV4_ADDR].asString().c_str(),IPV4_ADDR_LEN);
						strncpy(ipcInfo.netmask,rp[MULTICAST_IPV4_SUBNETMASK].asString().c_str(),IPV4_ADDR_LEN);
						strncpy(ipcInfo.gateway,rp[MULTICAST_IPV4_GATEWAY].asString().c_str(),IPV4_ADDR_LEN);
						strncpy(ipcInfo.mac,rp[MULTICAST_MAC_ADDR].asString().c_str(),DEV_MAC_LEN);
						strncpy(ipcInfo.owner,rp[MULTICAST_DEV_OWNER].asString().c_str(),OWNER_NAME_LEN);
						strncpy(ipcInfo.devType,rp[MULTICAST_DEV_TYPE].asString().c_str(),DEVICE_TYPE_LEN);
						strncpy(ipcInfo.version,rp[MULTICAST_DEV_VERSION].asString().c_str(),VERSION_LEN);
						strncpy(ipcInfo.buildDate,rp[MULTICAST_BUILD_DATE].asString().c_str(),BUILD_DATE_LEN);

						ipcInfo.port = rp[MULTICAST_DATA_PORT].asInt();
						
						PushDevice(ipcInfo);
					}
				}
			}
			else
			{
				PUB_Sleep(1);
			}

			tryTimes--;
		}

		searcher.Quit();
		PUB_Sleep(1000);
	}

	delete [] pDataBuff;
	searcher.Quit();
}

void CDeviceSearcher::ClearTimeOutDevice()
{
	m_devListLock.Lock();
	for (SEARCH_IPC_INFO_LIST::iterator iter = m_devList.begin(); iter != m_devList.end();)
	{
		(*iter).count++;
		if ((*iter).count > MAX_FAILED_TIMES)
		{
			iter = m_devList.erase(iter);
		}
		else
		{
			iter++;
		}
	}
	m_devListLock.UnLock();
}


void CDeviceSearcher::PushDevice(TVT_DISCOVER_DEVICE &info)
{
	if (strlen(info.sn) == 0 || strlen(info.ip) == 0)
	{
		return;
	}

	m_devListLock.Lock();
	SEARCH_IPC_INFO_LIST::iterator iter = m_devList.begin(),end = m_devList.end();

	for (;iter != end; ++iter)
	{
		if (strcmp((*iter).info.ip,info.ip) == 0)
		{
			memcpy(&((*iter).info),&info,sizeof(info));
			(*iter).count = 0;
			break;
		}
	}

	if (iter == end)
	{
		SEARCH_IPC_INFO searchInfo;
		memcpy(&(searchInfo.info),&info,sizeof(info));
		searchInfo.count = 0;
		m_devList.push_back(searchInfo);
	}

	m_devListLock.UnLock();
}


void CDeviceSearcher::Upgrade(CMulticast *pMulticast)
{
	m_devListLock.Lock();
	std::string upGradeParam = m_upGradeParam;
	std::string upGradeSn = m_upGradeSn;
	m_devListLock.UnLock();

	if (upGradeParam.empty())
	{
		return;
	}

	Json::Value reqJson;
	reqJson[MULTICAST_UPGRADE_PARAM] = upGradeParam;
	
	if (!upGradeSn.empty())
	{
		reqJson[MULTICAST_DEV_SN] = upGradeSn;
	}

	Json::FastWriter writer;
	std::string reqStr = writer.write(reqJson);
	
	std::vector<char> Buf;
	Buf.resize(sizeof(TVT_MULTICAST_HEAD) + reqStr.size());
	memset(&Buf[0],0,Buf.size());
	TVT_MULTICAST_HEAD *pMultiHead = reinterpret_cast<TVT_MULTICAST_HEAD *>(&Buf[0]);
	pMultiHead->headFlag = TVT_MULTICAST_HEAD_FLAG;
	pMultiHead->cmd = TVT_MCMD_UPGRADE;
	pMultiHead->paramLen = reqStr.size();

	memcpy(&Buf[0]+sizeof(TVT_MULTICAST_HEAD),reqStr.c_str(),reqStr.size());
	int count = 15;
	while(count-- > 0)
	{
		pMulticast->SendTo(&Buf[0],Buf.size());
		PUB_Sleep(100);
	}
}
