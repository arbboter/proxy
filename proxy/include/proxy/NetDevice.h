#ifndef _NET_DEVICE_H_
#define _NET_DEVICE_H_

#include "SWL_MultiNet.h"
#include "ProxyProtocol.h"
#include "CryptAes.h"
#include <string>
#include <vector>
#include <memory>
#include <map>

typedef enum _device_net_status_
{
	NET_STATUS_LOGOUT,
	NET_STATUS_LOGIN,
}DEVICE_NET_STATUS;


typedef long (*DATA_CALLBACK)(void *pUserData,unsigned long	deviceID,int channl,void *data,long len,bool bLive);


enum PUSH_PLATFORM
{
	NONE_PLATFORM,
	APPLE_PLATFORM,
	BAIDU_PLATFORM,
	GOOGLE_PLATFORM,
};


typedef struct _push_client_info_
{
	int platform;
	union
	{
		struct 
		{
			char token[LONG_LONG_STR_LEN];
			int device_type;
		}applePlatform;

		struct 
		{
			char user_id[SHORT_STR_LEN];
			char channel_id[SHORT_STR_LEN];
			int device_type;
		}baiduPlatForm;
	};
}PUSH_CLIENT_INFO;


typedef struct _device_info_
{
	char serialNo[SHORT_STR_LEN];
	char userName[SHORT_STR_LEN];
	char password[SHORT_STR_LEN];
	char session[SHORT_STR_LEN];
	char addr[SHORT_STR_LEN];
	PUSH_CLIENT_INFO  pushInfo;
}DEVICE_INFO;


class CNetDevice
{
	struct NET_DEV_BUF
	{
		NET_DEV_BUF()
		{	
			buf = NULL;
			bufLen = 0;
			dataLen = 0;
		}
		~NET_DEV_BUF()
		{
			if (buf)
			{
				delete [] buf;
			}
		}
		char* buf;
		long	  bufLen;
		long	  dataLen;
	};

	typedef std::auto_ptr<NET_DEV_BUF> REPLY;

	const static int AES_KEY_LEN  = 16;

public:
	CNetDevice();
	~CNetDevice();
	
	bool BindUser(std::string &info);

	bool Login(DEVICE_INFO *pDeviceInfo);
	void Logout();

	bool Config(std::vector<char> &inParam,std::vector<char> &outParam);

	bool StartLiveChannel(long channl,bool bMainStream);
	void StopLiveChannel(long channl,bool bMainStream);

	bool StartDownload(std::string fileName,long begin,long end);
	void DownloadFeedBack(long recvBytes);
	void StopDownload();

	bool OnSecondTimer();
	
	long GetNetStatus();

	void OnRecvData(RECV_DATA_BUFFER *pDataBuffer);
	
	void SetFrameCallBack(DATA_CALLBACK pDataCallBack,void *pUsrData);

	void SetNetHandle(CSWL_MultiNet	*pNetHandle,unsigned long deviceID);

	bool bOnLine();

	bool bLegal(long cmd);

private:
	int NetPackParser(NET_DEV_BUF &recvBuf);	
	
	bool OnMsg(char *pData,long dataLen);
	bool OnLive(char *pData,long dataLen);
	bool OnMp4(char *pData,long dataLen);
	bool OnPush(char *pData,long dataLen);
	bool OnTalk(char *pData,long dataLen);
	
	REPLY SendCmd(long msg,const char *pData,long len,bool bHasReply = true);
	bool SendPackage(long msg,const char *pData,long len,long padding,bool bEncrypt);

	bool bAgreeMent();

private:
	CSWL_MultiNet	*m_pNetHandle;
	unsigned long	m_deviceID;
	CPUB_Lock		m_deviceLock;

	bool			m_bLoging;
	DEVICE_NET_STATUS m_netStatus;

	DATA_CALLBACK	m_pDataCallBack;
	void			*m_pUserData;

	std::map<long,long>	m_lastRecvPackIndexmap;
	std::map<long,long>	m_lastSendPackIndexmap;

	std::map<long,NET_DEV_BUF>		m_recvBufMap;
	NET_DEV_BUF		m_sendBuf;
	NET_DEV_BUF		m_encryptBuf;
	NET_DEV_BUF		m_decryptBuf;


	NET_DEV_BUF		m_msgBuf;
	CPUB_Lock		m_msgLock;

	unsigned long	m_lastTick;

	bool			m_bNeedFirstFrame;
	
	CCryptAes		m_aesCrypt;
};

#endif