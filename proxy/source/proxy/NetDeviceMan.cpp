#include "netdeviceman.h"
#include "SWL_MultiNetCommEx.h"
#include "NatMultiNetComm.h"

#define NEW_CONNECTOR	1

#if !NEW_CONNECTOR
#include "DeviceConnector.h"
#else
#include "NetDeviceConnector.h"
#endif


//处理客户端请求
static PUB_THREAD_RESULT PUB_THREAD_CALL SecondTimerThread(void *pParam)
{
	assert(NULL != pParam);
	CNetDeviceMan *pThis = reinterpret_cast<CNetDeviceMan *>(pParam);
	pThis->SecondTimerProc();
	return 0;
}


//回复客户端
static PUB_THREAD_RESULT PUB_THREAD_CALL ConnectThread(void *pParam)
{
	assert(NULL != pParam);
	CNetDeviceMan *pThis = reinterpret_cast<CNetDeviceMan *>(pParam);
	pThis->ConnectProc();
	return 0;
}

//网络数据接收回调函数
static int DevManDataCallback(long netSessionID, void* pParam, RECV_DATA_BUFFER *pDataBuffer)
{
	assert(NULL != pParam);
	CNetDeviceMan *pThis = reinterpret_cast<CNetDeviceMan *>(pParam);
	pThis->RecvCallback(netSessionID, pDataBuffer);
	return 0;
}

//流数据回调
static long DevManStreamCallBack(void *pParam,unsigned long deviceID,int channl,void *data,long len,bool bLive)
{
	assert(NULL != pParam);
	CNetDeviceMan *pThis = reinterpret_cast<CNetDeviceMan *>(pParam);

	return pThis->StreamCallBack(deviceID,channl,data,len,bLive);
}


bool CNetDeviceMan::Init()
{
	m_deviceList.clear();
	
    m_hSecondTimerThread = PUB_CREATE_THREAD_FAIL;
	m_bSecondTimerRun = false;

	m_hConnectThread = PUB_CREATE_THREAD_FAIL;
	m_bConnectRun = false;

	m_pMultiNet = new CSWL_MultiNetCommEx;
	m_pMultiNet->Init();

	
	m_pMultiNat = new CNatMultiNetComm;
	m_pMultiNat->Init();

	m_streamBufferMan.Init();
	
	DEVICE_INFO deviceInfo;
	memset(&deviceInfo,0,sizeof(deviceInfo));
	//strcpy(deviceInfo.serialNo,"987654321");
	strcpy(deviceInfo.serialNo,"abcdefghijk");
	strcpy(deviceInfo.userName,"abc@abc.com");
	strcpy(deviceInfo.password,"12345678");
	AddDevice(&deviceInfo);

	return true;
}

void CNetDeviceMan::Quit()
{
	if (!m_deviceList.empty())
	{
		for (NET_DEVICE_RES_LIST::iterator iter = m_deviceList.begin(); iter != m_deviceList.end(); ++iter)
		{
			if (NULL != (*iter)->pNetDevice)
			{
				delete (*iter)->pNetDevice;
			}

			delete (*iter);
		}

		m_deviceList.clear();
	}

	if (NULL != m_pMultiNet)
	{
		m_pMultiNet->Destroy();
		delete m_pMultiNet;
		m_pMultiNet = NULL;
	}

	if (NULL != m_pMultiNat)
	{
		m_pMultiNat->Destroy();
		delete m_pMultiNat;
		m_pMultiNat = NULL;
	}

	m_streamBufferMan.Quit();
}

bool CNetDeviceMan::Start()
{
	m_hSecondTimerThread = PUB_CreateThread(&SecondTimerThread,this,&m_bSecondTimerRun);
	if (PUB_CREATE_THREAD_FAIL == m_hSecondTimerThread)
	{
		goto ERROR_PROC;
	}

	m_hConnectThread = PUB_CreateThread(&ConnectThread,this,&m_bConnectRun);
	if (PUB_CREATE_THREAD_FAIL == m_hConnectThread)
	{
		goto ERROR_PROC;
	}
	
	return true;

ERROR_PROC:

	if (PUB_CREATE_THREAD_FAIL != m_hSecondTimerThread)
	{
		PUB_ExitThread(&m_hSecondTimerThread,&m_bSecondTimerRun);
	}

	if (PUB_CREATE_THREAD_FAIL != m_hConnectThread)
	{
		PUB_ExitThread(&m_hConnectThread,&m_bConnectRun);
	}

	return false;
}

void CNetDeviceMan::Stop()
{

	if (PUB_CREATE_THREAD_FAIL != m_hSecondTimerThread)
	{
		PUB_ExitThread(&m_hSecondTimerThread,&m_bSecondTimerRun);
	}

	if (PUB_CREATE_THREAD_FAIL != m_hConnectThread)
	{
		PUB_ExitThread(&m_hConnectThread,&m_bConnectRun);
	}

}


bool CNetDeviceMan::BindUser(std::string serialNo,std::string &info)
{
	bool ret = false;

	if (NULL != GetDevice(serialNo))
	{
		RelDevice(serialNo);
		return ret;
	}

	NET_DEVICE_RES *pDevice = new NET_DEVICE_RES;
	memset(pDevice,0,sizeof(*pDevice));
	pDevice->deviceID = (unsigned long)(pDevice);
	strncpy(pDevice->deviceInfo.serialNo,serialNo.c_str(),SHORT_STR_LEN);
	pDevice->pNetDevice = new CNetDevice;
	pDevice->bAutoLogin = false;

	AddDevice(pDevice);
	if (Connect(pDevice))
	{
		ret = pDevice->pNetDevice->BindUser(info);
	}
	DeleteDevice(serialNo);

	delete pDevice->pNetDevice;
	delete pDevice;

	return ret;
}

bool CNetDeviceMan::AddDevice(DEVICE_INFO *pDeviceInfo)
{
	if (NULL != GetDevice(pDeviceInfo->serialNo))
	{
		RelDevice(pDeviceInfo->serialNo);
		return true;
	}

	NET_DEVICE_RES *pDevice = new NET_DEVICE_RES;
	memset(pDevice,0,sizeof(*pDevice));
	pDevice->deviceID = (unsigned long)(pDevice);
	memcpy(&pDevice->deviceInfo,pDeviceInfo,sizeof(pDevice->deviceInfo));

	pDevice->pNetDevice = new CNetDevice;
	pDevice->bAutoLogin = true;

	AddDevice(pDevice);

	return true;
}

void CNetDeviceMan::DelDevice(std::string serialNo)
{
	NET_DEVICE_RES *pDevice = NULL;
	pDevice = DeleteDevice(serialNo);
	
	if (NULL != pDevice)
	{
		m_streamBufferMan.DelDeviceBuf(pDevice->deviceID);
		delete pDevice->pNetDevice;
		delete pDevice;
	}
}


void CNetDeviceMan::DelAllDevice()
{
	std::list<std::string> deviceSnList;
	GetAllDevice(deviceSnList);

	NET_DEVICE_RES *pDevice = NULL;
	for (std::list<std::string>::iterator iter = deviceSnList.begin(); iter != deviceSnList.end(); iter++)
	{
		DelDevice((*iter));
	}
}


bool CNetDeviceMan::StartLiveChannel(std::string serialNo, long channel,bool bMainStream)
{
	bool ret = false;
	NET_DEVICE_RES *pDevice = GetDevice(serialNo);

	if (NULL == pDevice)
	{
		return ret;
	}

	if(!m_streamBufferMan.AddDeviceBuf(pDevice->deviceID,channel,true,bMainStream))
	{
		RelDevice(serialNo);

		return ret;
	}

	ret = pDevice->pNetDevice->StartLiveChannel(channel,bMainStream);
	
	if (!ret)
	{
		m_streamBufferMan.DelDeviceBuf(pDevice->deviceID,channel,true,bMainStream);
	}

	RelDevice(serialNo);
	
	return ret;
}

void CNetDeviceMan::StopLiveChannel(std::string serialNo, long channel,bool bMainStream)
{
	NET_DEVICE_RES *pDevice = GetDevice(serialNo);

	if (NULL != pDevice)
	{
		m_streamBufferMan.DelDeviceBuf(pDevice->deviceID,channel,true,bMainStream);
		pDevice->pNetDevice->StopLiveChannel(channel,bMainStream);
		RelDevice(serialNo);
	}
}

bool CNetDeviceMan::StartDownload(std::string serialNo,std::string fileName,unsigned long begin,unsigned long end)
{
	bool ret = false;
	NET_DEVICE_RES *pDevice = GetDevice(serialNo);

	if (NULL == pDevice)
	{
		return ret;
	}

	if (!m_streamBufferMan.AddDeviceBuf(pDevice->deviceID,0,false,false))
	{
		RelDevice(serialNo);
		return ret;
	}

	ret = pDevice->pNetDevice->StartDownload(fileName,begin,end);

	if (!ret)
	{
		m_streamBufferMan.DelDeviceBuf(pDevice->deviceID,0,false,false);
	}

	RelDevice(serialNo);
	
	return ret;
}

void CNetDeviceMan::StopDownload(std::string serialNo)
{

	NET_DEVICE_RES *pDevice = GetDevice(serialNo);

	if (NULL != pDevice)
	{
		m_streamBufferMan.DelDeviceBuf(pDevice->deviceID,0,false,false);
		pDevice->pNetDevice->StopDownload();
		RelDevice(serialNo);
	}
}

bool CNetDeviceMan::Config(std::string serialNo,std::vector<char> &inParam,std::vector<char> &outParam)
{
	bool ret = false;
	NET_DEVICE_RES *pDevice = GetDevice(serialNo);

	if (NULL != pDevice)
	{
		ret = pDevice->pNetDevice->Config(inParam,outParam);
		RelDevice(serialNo);
	}

	return ret;
}

bool CNetDeviceMan::GetDataBlock(std::string serialNo,long channl,bool bLive,bool bMainStream,char **ppData,unsigned long *pDataLen)
{
	bool bRet = false;

	NET_DEVICE_RES *pDevice = GetDevice(serialNo);
	if (NULL == pDevice)
	{
		return bRet;
	}

	bRet = m_streamBufferMan.GetData(pDevice->deviceID,channl,bLive,bMainStream,ppData,pDataLen);

	unsigned long recvBytes = m_streamBufferMan.NeedFeedBack(pDevice->deviceID,channl,bLive);
	if (recvBytes > 0)
	{
		pDevice->pNetDevice->DownloadFeedBack(recvBytes);
	}

	RelDevice(pDevice->deviceInfo.serialNo);
	
	return bRet;
}

void CNetDeviceMan::ReleaseDataBlock(std::string serialNo,long channl,bool bLive,bool bMainStream,char *pData,unsigned long dataLen)
{

	NET_DEVICE_RES *pDevice = GetDevice(serialNo);
	if (NULL == pDevice)
	{
		return ;
	}

	m_streamBufferMan.RelData(pDevice->deviceID,channl,bLive,bMainStream,pData,dataLen);

	RelDevice(pDevice->deviceInfo.serialNo);

}

bool CNetDeviceMan::bOnLine(std::string serialNo)
{
	NET_DEVICE_RES *pDevice = GetDevice(serialNo);
	if (NULL == pDevice)
	{
		return  false;
	}
	bool ret = pDevice->pNetDevice->bOnLine();
	
	RelDevice(pDevice->deviceInfo.serialNo);

	return ret;
}

bool CNetDeviceMan::bAdd(std::string serialNo)
{
	NET_DEVICE_RES *pDevice = GetDevice(serialNo);
	if (NULL == pDevice)
	{
		return  false;
	}
	
	RelDevice(pDevice->deviceInfo.serialNo);

	return true;
}


bool CNetDeviceMan::bReachable(std::string serialNo)
{
	bool bRet = false;
//#if NEW_CONNECTOR
//	NET_DEVICE_CONN_CONFIG config = {0};
//	NET_DEVICE_CONN_RESULT result = {0};
//	NetDeviceConnector	dev;
//
//	strcpy(config.szDeviceNo, serialNo.c_str());
//	strcpy(config.szServerAddr, "192.168.3.198");
//
//	//设置本地信息
//	config.pDeviceConnCache = new NET_DEVICE_CONN_CACHE;
//	strcpy(config.pDeviceConnCache->sServerInfo.szAddr, "192.168.3.198");
//	config.pDeviceConnCache->sServerInfo.sPort = 8989;
//	config.pDeviceConnCache->sServerInfo.sTestPort = 0;
//
//	if(dev.Connect(&config, 2000, NULL, &result))
//	{
//		bRet = true;
//	}
//	else
//	{
//		bRet = false;
//	}
//
//	delete result.pDeviceConnCache;
//	result.pDeviceConnCache = NULL;
//	delete result.pServerInfoList;
//	result.pServerInfoList = NULL;
//#else
//	CDeviceConnector deviceConnector(NULL,NULL,serialNo);
//	return CONNECT_MODE_NONE != deviceConnector.InitConnetMode();
//#endif
	return bRet;
}

void CNetDeviceMan::GetAllLocalDevice(TVT_DEV_LIST &devList)
{
	CDeviceSearcher::Instance().GetDeviceList(devList);
}


bool CNetDeviceMan::GetLocalDevice(std::string serialNo,TVT_DISCOVER_DEVICE &devInfo)
{
	return CDeviceSearcher::Instance().GetDeviceInfo(serialNo.c_str(),devInfo);
}

void CNetDeviceMan::UpGradeLocalDevice(std::string sn,std::string param)
{
	CDeviceSearcher::Instance().UpGradeLocalDevice(sn,param);
}

long CNetDeviceMan::RecvCallback(unsigned long deviceID,RECV_DATA_BUFFER *pDataBuffer)
{
	NET_DEVICE_RES *pDevice = GetDevice(deviceID);
	
	if (NULL == pDevice)
	{
		return 0;
	}

	assert(0 != deviceID);
	pDevice->pNetDevice->OnRecvData(pDataBuffer);
	RelDevice(pDevice->deviceInfo.serialNo);
	return 0;
}


long CNetDeviceMan::StreamCallBack(unsigned long deviceID,int channl,void *data,long len,bool bLive)
{
	if(m_streamBufferMan.InputData(deviceID,channl,bLive,false,(char *)data,len))
	{
		return 0;
	}

	return -1;
}

void CNetDeviceMan::SecondTimerProc()
{
	std::list<unsigned long> deviceIDList;
	NET_DEVICE_RES *pDevice = NULL;

	while(m_bSecondTimerRun)
	{
		GetAllDevice(deviceIDList);
		
		for (std::list<unsigned long>::iterator iter = deviceIDList.begin();iter!= deviceIDList.end(); ++iter)
		{
			pDevice = GetDevice((*iter));
			if (NULL != pDevice)
			{
				pDevice->pNetDevice->OnSecondTimer();
				RelDevice(pDevice->deviceInfo.serialNo);
			}
		}

		PUB_Sleep(1000);
	}
}

void CNetDeviceMan::ConnectProc()
{
	std::list<unsigned long> deviceIDList;
	NET_DEVICE_RES *pDevice = NULL;
	 
	while(m_bConnectRun)
	{
		GetAllDevice(deviceIDList);

		for (std::list<unsigned long>::iterator iter = deviceIDList.begin();iter!= deviceIDList.end(); ++iter)
		{
			pDevice = GetDevice((*iter));
			if(NULL == pDevice)
			{
				continue;
			}

			if (pDevice->bAutoLogin && (pDevice->pNetDevice->GetNetStatus()==NET_STATUS_LOGOUT))
			{ 
				do 
				{
					pDevice->pNetDevice->Logout();

					if (!Connect(pDevice))
					{
						break;
					}

					if (!pDevice->pNetDevice->Login(&pDevice->deviceInfo))
					{
						break;
					}

					std::cout<<"Device Login:"<<pDevice->deviceInfo.userName<<std::endl;
				} while (false);	
			}

			RelDevice(pDevice->deviceInfo.serialNo);
		}

		PUB_Sleep(1000);
	}

}

void CNetDeviceMan::AddDevice(NET_DEVICE_RES *pDevice)
{
	m_devceLock.Lock();

	pDevice->count = 0;
	m_deviceList.push_back(pDevice);

	m_devceLock.UnLock();
}


CNetDeviceMan::NET_DEVICE_RES* CNetDeviceMan::GetDevice(std::string serialNo)
{
	NET_DEVICE_RES *pDev = NULL;
	
	m_devceLock.Lock();
	for (NET_DEVICE_RES_LIST::iterator iter = m_deviceList.begin(); iter != m_deviceList.end(); ++iter)
	{
		if (serialNo == (*iter)->deviceInfo.serialNo)
		{
			pDev = (*iter);
			pDev->count++;
			assert(pDev->count > 0);
			break;
		}
	}
	m_devceLock.UnLock();

	return pDev;
}

CNetDeviceMan::NET_DEVICE_RES* CNetDeviceMan::GetDevice(unsigned long deviceID)
{
	NET_DEVICE_RES *pDev = NULL;

	m_devceLock.Lock();
	for (NET_DEVICE_RES_LIST::iterator iter = m_deviceList.begin(); iter != m_deviceList.end(); ++iter)
	{
		if (deviceID == (*iter)->deviceID)
		{
			pDev = (*iter);
			pDev->count++;
			assert(pDev->count > 0);
			break;
		}
	}
	m_devceLock.UnLock();

	return pDev;
}

void CNetDeviceMan::RelDevice(std::string serialNo)
{
	m_devceLock.Lock();
	for (NET_DEVICE_RES_LIST::iterator iter = m_deviceList.begin(); iter != m_deviceList.end(); ++iter)
	{
		if (serialNo == (*iter)->deviceInfo.serialNo)
		{
			(*iter)->count--;
			assert((*iter)->count >= 0);
			break;
		}
	}
	m_devceLock.UnLock();
}

CNetDeviceMan::NET_DEVICE_RES* CNetDeviceMan::DeleteDevice(std::string serialNo)
{
	NET_DEVICE_RES *pDev = NULL;

	while(true)
	{
		m_devceLock.Lock();

		pDev = NULL;
		for (NET_DEVICE_RES_LIST::iterator iter = m_deviceList.begin(); iter != m_deviceList.end(); ++iter)
		{
			if (serialNo == (*iter)->deviceInfo.serialNo)
			{
				pDev = (*iter);
				break;
			}
		}
		
		if (NULL == pDev)
		{
			m_devceLock.UnLock();
			break;
		}

		if (0 == pDev->count)
		{
			m_deviceList.remove(pDev);
			m_devceLock.UnLock();
			break;
		}
		
		m_devceLock.UnLock();
		PUB_Sleep(1);
	}

	return pDev;
}

void CNetDeviceMan::GetAllDevice(std::list<unsigned long> &deviceList)
{
	deviceList.clear();

	m_devceLock.Lock();
	for (NET_DEVICE_RES_LIST::iterator iter = m_deviceList.begin(); iter != m_deviceList.end(); ++iter)
	{
		deviceList.push_back((*iter)->deviceID);
	}
	m_devceLock.UnLock();
}

void CNetDeviceMan::GetAllDevice(std::list<std::string> &deviceList)
{
	deviceList.clear();

	m_devceLock.Lock();
	for (NET_DEVICE_RES_LIST::iterator iter = m_deviceList.begin(); iter != m_deviceList.end(); ++iter)
	{
		deviceList.push_back((*iter)->deviceInfo.serialNo);
	}
	m_devceLock.UnLock();
}

bool CNetDeviceMan::Connect(NET_DEVICE_RES *pDevice)
{
	bool bRet = false;
	TVT_SOCKET sock = SWL_INVALID_SOCKET;
	CSWL_MultiNet *pNetHandle = NULL;

#if NEW_CONNECTOR
	NET_DEVICE_CONN_CONFIG config = {0};
	NET_DEVICE_CONN_RESULT result = {0};
	NetDeviceConnector	dev;

	strcpy(config.szDeviceNo, pDevice->deviceInfo.serialNo);
	strcpy(config.szServerAddr, "192.168.3.198");

	//设置本地信息
	config.pDeviceConnCache = new NET_DEVICE_CONN_CACHE;
	if(0)//config.pDeviceConnCache)
	{
		strcpy(config.pDeviceConnCache->sServerInfo.szAddr, "192.168.3.198");
		config.pDeviceConnCache->sServerInfo.sPort = 8989;
		config.pDeviceConnCache->sServerInfo.sTestPort = 0;
	}

	if(!dev.Connect(&config, 2000, NULL, &result))
	{
		bRet = true;
		switch(result.sockType)
		{
		case NET_TRANS_COMMON:
			pNetHandle = m_pMultiNet;
			break;
		case NET_TRANS_NAT:
			pNetHandle = m_pMultiNat;
			break;
		default:
			bRet = false;
		}

		if(result.iServerInfoCount)
		{
			cout << "最新的NAT服务器列表:" << endl;
			for (int i=0; i<result.iServerInfoCount; i++)
			{
				cout << i << "\t" << result.pServerInfoList[i].szAddr << "\t"
					 << result.pServerInfoList[i].sPort << "\t" << result.pServerInfoList[i].sTestPort << endl;
			}
		}

		if(result.pDeviceConnCache)
		{
			cout << "输出的缓存设备信息:" << endl;
			cout << result.pDeviceConnCache->sServerInfo.szAddr << "\t" 
				 << result.pDeviceConnCache->sServerInfo.sPort << "\t"
				 << result.pDeviceConnCache->sServerInfo.sTestPort << endl;
		}
	}
	else
	{
		bRet = false;
	}

	// 释放存储空间
	delete result.pDeviceConnCache;
	result.pDeviceConnCache = NULL;
	delete result.pServerInfoList;
	result.pServerInfoList = NULL;

#else
	CONNECT_UNIT connectUnit;
	memset(&connectUnit,0,sizeof(connectUnit));

	CDeviceConnector deviceConnector(m_pMultiNet,m_pMultiNat,pDevice->deviceInfo.serialNo);

	if (deviceConnector.ConnectDevice(&connectUnit))
	{
		pNetHandle = connectUnit.pMultiNet;
		sock = connectUnit.sock;
	}
#endif

	if (NULL != pNetHandle)
	{
		pNetHandle->AddComm(pDevice->deviceID,sock);
		pNetHandle->SetAutoRecvCallback(pDevice->deviceID,DevManDataCallback,this);
		pNetHandle->SetRecvBufferLen(pDevice->deviceID,512*1024);
		pDevice->pNetDevice->SetNetHandle(pNetHandle,pDevice->deviceID);
		pDevice->pNetDevice->SetFrameCallBack(DevManStreamCallBack,this);
	}

	return NULL != pNetHandle;
}
