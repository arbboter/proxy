#ifndef _TVT_SEARCH_DEVICE_H_
#define _TVT_SEARCH_DEVICE_H_
#include "TVT_PubCom.h"
#include "TVT_PubDefine.h"
#include <list>

const int IPV4_ADDR_LEN = 16;
const int DEV_SN_LEN = 64;
const int DEV_MAC_LEN = 48;
const int OWNER_NAME_LEN = 64;
const int DEVICE_TYPE_LEN = 8;
const int VERSION_LEN = 16;
const int BUILD_DATE_LEN = 16;
const int LATEST_WAIT_TIME = 10000;

typedef struct _tvt_discover_device_
{
	char sn[DEV_SN_LEN+1];
	char ip[IPV4_ADDR_LEN+1];
	char netmask[IPV4_ADDR_LEN+1];
	char gateway[IPV4_ADDR_LEN+1];
	char mac[DEV_MAC_LEN+1];
	char owner[OWNER_NAME_LEN+1];
	char devType[DEVICE_TYPE_LEN+1];
	char version[VERSION_LEN];
	char buildDate[BUILD_DATE_LEN];
	unsigned short port;
}TVT_DISCOVER_DEVICE;

typedef std::list<TVT_DISCOVER_DEVICE> TVT_DEV_LIST;

class CMulticast;

class CDeviceSearcher
{
	static const int SEARCH_WAIT_TIME = 5000;
	static const int MAX_FAILED_TIMES = 30;

	typedef struct _serach_ipc_info_
	{
		TVT_DISCOVER_DEVICE info;
		unsigned long count;
	}SEARCH_IPC_INFO;

	typedef list<SEARCH_IPC_INFO> SEARCH_IPC_INFO_LIST; 
public:
	static CDeviceSearcher & Instance();

	bool StartSearchThread();
	bool StopSearchThread();

	void UpGradeLocalDevice(std::string sn,std::string param);

	void GetDeviceList(TVT_DEV_LIST &devList);
	bool GetDeviceInfo(const char *pSn,TVT_DISCOVER_DEVICE &devInfo);	

	static PUB_THREAD_RESULT PUB_THREAD_CALL SearchDVRThread(void *pParam);
	void SerachDVRPro();

private:
	CDeviceSearcher();
	~CDeviceSearcher();

	void ClearTimeOutDevice();
	void PushDevice(TVT_DISCOVER_DEVICE &info);
	void Upgrade(CMulticast *pMulticast);
private:
	bool			m_bDVRSearching;
	PUB_thread_t	m_serachDVRThreadHandle;

	SEARCH_IPC_INFO_LIST	m_devList;
	CPUB_Lock				m_devListLock;

	unsigned long			m_startTime;

	bool					m_bUpGrade;
	std::string				m_upGradeParam;
	std::string				m_upGradeSn;
};


#endif