
#ifndef _NET_DEVICE_LIST_H_
#define _NET_DEVICE_LIST_H_

#include "netdevice.h"
#include "StreamBufferMan.h"
#include "SWL_MultiNet.h"
#include "MulticastHeader.h"
#include "DeviceSearcher.h"
#include <list>


class CNetDeviceMan
{
	typedef struct _net_device_res
	{
		unsigned long deviceID;
		CNetDevice	*pNetDevice;
		DEVICE_INFO deviceInfo;
		bool bAutoLogin;
		unsigned short port;
		long count;
	}NET_DEVICE_RES;
	
	typedef std::list<NET_DEVICE_RES *> NET_DEVICE_RES_LIST;

public:
	bool Init();
	void Quit();

	bool Start();
	void Stop();

	bool BindUser(std::string serialNo,std::string &info);

	bool AddDevice(DEVICE_INFO *pDeviceInfo);
	void DelDevice(std::string serialNo);
	void DelAllDevice();

	bool StartLiveChannel(std::string serialNo, long channel,bool bMainStream);
	void StopLiveChannel(std::string serialNo, long channel,bool bMainStream);
	
	bool StartDownload(std::string serialNo,std::string fileName,unsigned long begin,unsigned long end);
	void StopDownload(std::string serialNo);

	bool Config(std::string serialNo,std::vector<char> &inParam,std::vector<char> &outParam);
	
	bool GetDataBlock(std::string serialNo,long channl,bool bLive,bool bMainStream,char **ppData,unsigned long *pDataLen);
	void ReleaseDataBlock(std::string serialNo,long channl,bool bLive,bool bMainStream,char *pData,unsigned long dataLen);

	bool bOnLine(std::string serialNo);
	bool bAdd(std::string serialNo);
	bool bReachable(std::string serialNo);

	void GetAllLocalDevice(TVT_DEV_LIST &devList);
	bool GetLocalDevice(std::string serialNo,TVT_DISCOVER_DEVICE &devInfo);
	void UpGradeLocalDevice(std::string sn,std::string param);

public:

	long RecvCallback(unsigned long deviceID,RECV_DATA_BUFFER *pDataBuffer);
	
	long StreamCallBack(unsigned long deviceID,int channl,void *data,long len,bool bLive);

	void SecondTimerProc();

	void ConnectProc();

private:
	void AddDevice(NET_DEVICE_RES *pDevice);
	NET_DEVICE_RES* DeleteDevice(std::string serialNo);

	NET_DEVICE_RES* GetDevice(unsigned long deviceID);
	NET_DEVICE_RES* GetDevice(std::string serialNo);
	void RelDevice(std::string serialNo);
	
	void GetAllDevice(std::list<unsigned long> &deviceList);
	void GetAllDevice(std::list<std::string> &deviceList);

	bool Connect(NET_DEVICE_RES *pDevice);
private:
	CSWL_MultiNet *m_pMultiNet;
	CSWL_MultiNet *m_pMultiNat;

	NET_DEVICE_RES_LIST m_deviceList;
	CPUB_Lock	m_devceLock;

	PUB_thread_t		m_hSecondTimerThread;
	bool				m_bSecondTimerRun;

	PUB_thread_t		m_hConnectThread;
	bool				m_bConnectRun;

	CStreamBufferMan	m_streamBufferMan;
};

#endif
