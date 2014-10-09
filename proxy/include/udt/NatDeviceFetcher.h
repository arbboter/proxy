// NatDeviceFetcher.h: interface for the CNatDeviceFetcher class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_DEVICE_FETCHER_H_
#define _NAT_DEVICE_FETCHER_H_

#include "NatUserDefine.h"

#ifdef NAT_TVT_DVR_4_0
#include "TVT_PubCom.h"
#else
#include "PUB_common.h"
#endif//NAT_TVT_DVR_4_0

#include "SWL_Public.h"
#include "NatSocket.h"
#include "NatBaseTypes.h"

typedef struct _nat_device_info_ex
{
	uint32                      dwDevicePeerIp;         // DevicePeer的IP地址，遵循“192.168.1.1”的格式
	uint16                      usDevicePeerPort;       // DevicePeer的端口号
	byte                        ucCanRelay;             // 设备是否可以使用中继连接
	char						caExtraInfo[512];		// 扩展信息，内部的数据只会在设备和客户端上解析，对穿透服务器是透明的	
}NAT_DEVICE_INFO_EX;


#ifdef __NAT_DEVICE_CONFIG_EX__

typedef NAT_DEVICE_INFO_EX NAT_DEVICE_INFO;

#else

/**
 * 设备信息，由客户端从NAT服务器上获取下来。
 */
typedef struct _nat_device_info
{
    unsigned long   dwDevicePeerIp;         /**< DevicePeer的IP地址 */
    unsigned short  usDevicePeerPort;       /**< DevicePeer的端口号 */
    unsigned char   ucCanRelay;             /**< 设备是否可以使用中继连接 */

	//////////////////////////
	// 以下是扩展信息（ExtraInfo）
    unsigned long   dwDeviceType;           /**< 设备类型，等价于实际设备中的产品型号 */
    char            caDeviceVersion[128];   /**< 设备版本号，以"-"为分隔符， 如"1.0.0-XXX-XX"
                                                 其中，第1部分为主要版本号 */
    unsigned short  usDeviceWebPort;        /**< 设备本地WEB服务端口号 */
    unsigned short  usDeviceDataPort;       /**< 设备本地WEB数据端口号 */
}NAT_DEVICE_INFO;

#endif//__NAT_DEVICE_CONFIG_EX__

/**
 * 设备版本类型
 */
enum NAT_DEVICE_VER_TYPE
{
    NAT_DEVICE_VER_3 = 3,       /**< 3.0设备类型 */
    NAT_DEVICE_VER_4 = 4        /**< 4.0设备类型 */
};

/**
 * 设备版本信息
 */
typedef struct _nat_device_ver_info_
{
    NAT_DEVICE_VER_TYPE verType;    /**< 设备版本类型 */
    // 如果需要，可以根据版本类型在后面增加字段
}NAT_DEVICE_VER_INFO;

/**
 * 解析设备版本信息
 * @param[in] pVerStr 版本信息文本串
 * @param[in] pVerInfo 设备版本信息
 */
void NAT_ParseDeviceVer(const char* pVerStr, NAT_DEVICE_VER_INFO* pVerInfo);


/**
 * 当获取设备返回结果后，调用该函数
 * @param[in] pDeviceInfo 获取的设备信息，如果出错，则等于NULL
 * @param[in] pObject 回调相关对象指针
 * @param[in] nParam 保留参数。
 * @return 返回值未定义。
 */
typedef int (*NAT_FETCH_DEVICE_CALLBACKEX)(const NAT_DEVICE_INFO *pDeviceInfo, void *pObject, void *pParam);


/**
 * Device Fetcher 的配置
 */
typedef struct _nat_device_fetcher_config_
{
    char           dwDeviceNo[64];  /*< 要连接的设备的序列号 */
    char           pServerIp[256];  /*< NAT服务器IP地址 */
    unsigned short nServerPort;     /*< NAT服务器端口号 */
}NAT_DEVICE_FETCHER_CONFIG;


class CNatDeviceFetcherWorker;

/**
 * 设备信息获取者
 */
class CNatDeviceFetcher
{
    friend class CNatDeviceFetcherWorker;
public:
    /**
     * 新建连接处理。
     * @param[in] config 客户端配置。
     * @return 如果成功，新建的连接者；否则返回NULL。
     */  
    static CNatDeviceFetcher* NewDeviceFetcher(const NAT_DEVICE_FETCHER_CONFIG *config);

    /**
     * 销毁函数。
     * @return 如果成功，返回0；否则，返回其它值。
     */  
    int Destroy();

    // 以同步调用的方式获取设备信息；
    // 不允许重复调用。
    // @param[in] timeout 超时时间，单位：毫秒
    // @param[in] pDeviceInfo 返回获取的设备信息的指针，传入不能为NULL。
    // @return 如果获取成功，返回0；否则，返回其它值
    int FetchSyn(int iTimeOut, NAT_DEVICE_INFO *pDeviceInfo);

    /**
     * 开始以异步方式获取设备信息。
     * 返回的结果由pCallback函数回调，不允许重复调用。
     * @param[in] pCallback 结果回调函数。
     * @param[in] pObject 相关联对象指针。
     * @param[in] iTimeOut 超时时间，单位：毫秒。
     * @return 如果成功，返回0；否则，返回其它值。
     */  
    int FetchAsyn(NAT_FETCH_DEVICE_CALLBACKEX pCallback, void* pObject, int iTimeOut);	

    /**
     * 获取错误代码
     * 如果获取设备信息出错，使用该函数可以获得失败的错误代码
     */
	NAT_CLIENT_ERROR GetError();
protected:
    CNatDeviceFetcher(const NAT_DEVICE_FETCHER_CONFIG *config);
    virtual ~CNatDeviceFetcher();
    bool Init();
    void NotifyCallback(NAT_DEVICE_INFO *pDeviceInfo);
private:
    CNatDeviceFetcherWorker *m_worker;
    NAT_CLIENT_ERROR m_error;
    NAT_DEVICE_FETCHER_CONFIG m_config;
    int m_iTimeout;
    bool m_isSync;
    bool m_syncCompleted;
    CPUB_Lock m_syncLock;
    NAT_DEVICE_INFO m_deviceInfo;

    NAT_FETCH_DEVICE_CALLBACKEX m_pCallback;
    void* m_pObject;
};


const static char* NAT_DEVICE_VER_PREFIX_3_0 = "3.";
const static char* NAT_DEVICE_VER_PREFIX_4_0 = "4.";

#endif//_NAT_DEVICE_FETCHER_H_