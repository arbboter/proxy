// NetDeviceConnector.h: interface for the CNetDeviceConnector class.
//
//////////////////////////////////////////////////////////////////////
#ifndef __NET_DEVICE_CONNECTOR__
#define __NET_DEVICE_CONNECTOR__

#include "SWL_TypeDefine.h"
#include "NatPublicTypes.h"
#include "ManagerServer.h"
#include <string>
#include <vector>

 /* 错误码 */
#define NET_DEV_CON_RET_SUCCESS					0		// 成功
#define NET_DEV_CON_RET_ERROR					1		// 一般错误
#define NET_DEV_CON_RET_DEV_NOT_EXIST			2		// NAT上找不到设备信息
#define NET_DEV_CON_RET_DEV_NOT_CONNECT			3		// NAT连接不上设备
#define NET_DEV_CON_RET_INVALIDE_PARAM			4		// 参数错误

typedef struct __net_device_conn_cache__
{
	NAT_SERVER_INFO sServerInfo;						// 设备注册的穿透服务器信息
	unsigned long	iLastConnectTime;					// 上次连接的时间，unit: second
}NET_DEVICE_CONN_CACHE;

typedef struct __net_device_conn_config__
{
	char					szDeviceNo[64];				// 设备序列号
	char					szServerAddr[128];			// 穿透服务器地址
	unsigned short			sServerPort;				// 穿透服务器端口号
	CManagerServer			*pManagerServer;			// 管理服务器对象，如果为NULL，表示不支持管理服务器
	int						iServerInfoCount;			// 穿透服务器（缓存）个数，如果为0，则忽略pServerInfoList
	NAT_SERVER_INFO			*pServerInfoList;			// 穿透服务器列表指针，如果为NULL，表示无数据
	NET_DEVICE_CONN_CACHE	*pDeviceConnCache;			// 设备连接缓存，如果为NULL，表示无数据

}NET_DEVICE_CONN_CONFIG;


typedef struct __nat_device_conn_result__
{
	char					szDeviceNo[64];				// 设备序列号
	NET_TRANS_TYPE			sockType;					// 连接Socket的类型
	TVT_SOCKET				sock;						// 连接Socket
	int						iServerInfoCount;			// 穿透服务器（缓存）个数，如果为0，则忽略pServerInfoList
	NAT_SERVER_INFO			*pServerInfoList;			// 穿透服务器列表指针，如果为NULL，表示无数据
	NET_DEVICE_CONN_CACHE	*pDeviceConnCache;			// 设备连接缓存，如果为NULL，表示无数据

}NET_DEVICE_CONN_RESULT;


// 对外类，用于完成与设备的连接	
// 
class NetDeviceConnector
{
public:
	NetDeviceConnector(){}
	~NetDeviceConnector(){}
	
	/** 
	 * 传入设备的序列号,如果与设备连接成功，返回该连接的描述符，同时由第三个参数指明连接的类型
	 * @param[in] pConfig 传入配置
	 * @param[in] iTimeout 连接超时时间，单位：ms
	 * @param[in] bIsCanceled 是否取消连接过程，调用者可以通过该指针中断执行过程，为NULL表示不处理
	 * @param[in] pResult 连接结果的指针，由调用者传入指针，内部填充内容
	 * @return 成功返回0；失败返回其它值
	 */
	int Connect(const NET_DEVICE_CONN_CONFIG* pConfig, int iTimeout, bool *bIsCanceled, NET_DEVICE_CONN_RESULT *pResult);

private:
	typedef struct _connect_thread_param_
	{
		const NET_DEVICE_CONN_CONFIG*	pConfig;
		int								iTimeout;
		bool*							bIsCanceled;
		NET_DEVICE_CONN_RESULT*			pResult;
		int								nError;
		bool							bFinished;
	}CONNECT_THREAD_PARAM;

	static PUB_THREAD_RESULT PUB_THREAD_CALL ConnectThread(void *pParam);

private:
	static int	GetDeviceInfo(const CManagerServer* pMS, const char* pDevSN, char* pDevIP, int& nDevPort);
};

#endif//__NET_DEVICE_CONNECTOR__
