// NatListenProc.h: interface for the CNatListenProc class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_LISTEN_PROC_H_
#define _NAT_LISTEN_PROC_H_

#include "NatUserDefine.h"

#ifdef NAT_TVT_DVR_4_0
#include "TVT_PubCom.h"
#else
#include "PUB_common.h"
#endif//NAT_TVT_DVR_4_0

#include "SWL_Public.h"
#include <string.h>
#include <list>
#include "NatSocket.h"

using namespace std;

//for test
#define __NAT_DEVICE_INFO_EX__

/**
 * 设备配置
 * 扩展版
 */
typedef struct _nat_device_config_ex_
{
    char           deviceNo[64];        /**< 设备的序列号 */
    char           serverIp[256];       /**< NAT服务器IP地址 */
    unsigned short serverPort;          /**< NAT服务器端口号 */
	unsigned short localPort;           /**< 本地绑定的端口（穿透） */
	unsigned long  localIp;             /**< 本地绑定的IP地址（穿透） */
	unsigned char  refuseRelay;         /**< 设备是否拒绝中继服务。为0表示不拒绝；为非0表示拒绝。默认为0 */      
    char           caExtraInfo[512];    /**< 扩展信息，必须是XML格式，内部的数据只会在设备和客户端上解析，对穿透服务器是透明的 */

}NAT_DEVICE_CONFIG_EX;

#ifdef __NAT_DEVICE_CONFIG_EX__

/**
 * 设备配置
 * 扩展版
 */
typedef NAT_DEVICE_CONFIG_EX NAT_DEVICE_CONFIG;

#else

/**
 * 设备配置
 * 标准版
 */
typedef struct _nat_device_config_
{
    char           deviceNo[64];        /**< 设备的序列号 */
    char           serverIp[256];       /**< NAT服务器IP地址 */
    unsigned short serverPort;          /**< NAT服务器端口号 */
	unsigned short localPort;           /**< 本地绑定的端口（穿透） */
	unsigned long  localIp;             /**< 本地绑定的IP地址（穿透） */  
	unsigned char  refuseRelay;         /**< 设备是否拒绝中继服务。为0表示不拒绝；为非0表示拒绝。默认为0 */
	
	//////////////////////////
	// 以下是扩展信息（ExtraInfo）
    unsigned long  deviceType;          /**< 设备类型，等价于实际设备中的产品型号 */
    char           deviceVersion[128];  /**< 设备版本号，以"-"为分隔符， 如"1.0.0-XXX-XX"
                                             其中，第1部分为主要版本号 */
    unsigned short deviceWebPort;       /**< 设备的WEB侦听端口号（TCP） */
    unsigned short deviceDataPort;      /**< 设备的数据侦听端口（TCP） */
}NAT_DEVICE_CONFIG;

#endif //__NAT_DEVICE_STD__

/**
 * 设备的服务器状态
 */
enum NAT_DEVICE_SERVER_STATE
{
	NAT_STATE_NONE,             /*初始态*/
    NAT_STATE_CONNECTING,       /**< 正在连接服务器 */
    NAT_STATE_CONNECTED,        /**< 已连接服务器 */
    NAT_STATE_DISCONNECTED      /**< 连接服务器失败 */
};

/**
 * 设备的服务器错误
 */
enum NAT_DEVICE_SERVER_ERROR
{
    NAT_ERR_OK,                /**< 无错误  */  
    NAT_ERR_UNKNOWN,           /**< 未知错误 */ 
    NAT_ERR_CONNECT_FAILED,    /**< 连接服务器失败 */ 
    NAT_ERR_REGISTER_FAILED,   /**< 向服务器注册失败 */ 
};

/**
 * 接受连接回调函数
 * 当有客户端连接后，accept了以后调用该函数
 * @param[in] pClientSockInfo 客户端的信息
 * @param[in] pParam1 未定义
 * @param[in] pParam2 未定义
 * @return 返回值未定义
 */
typedef int (*NAT_ACCEPT_LINK_CALLBACK)(CLIENT_SOCK_INFO * pClientSockInfo, void *pParam1, void *pParam2);

class  CNatDevicePeer;

class CNatListenProc 
{
public:
    /**
     * 构造函数
     * pCallback函数要尽量快返回，因为程序还要接着listen
     * 如果pCallback里没有把生成的socket接管来则会导致资源泄露
     * @param[in] pCallback 接受连接回调函数
     * @param[in] pParam 相关联参数
     */    
    CNatListenProc(NAT_ACCEPT_LINK_CALLBACK pCallback, void* pParam);
    ~CNatListenProc();

    /**
     * 开始监听端口，检查是否有网络接入
     * @param[in] pConfig 设备配置
     * @return 如果成功，返回0；否则，返回其它值。
     */
    int StartListen(const NAT_DEVICE_CONFIG* pConfig);

    /**
     * 停止监听端口
     * @return 如果成功，返回0；否则，返回其它值。
     */
    int StopListen();

    /**
     * 是否已开始侦听
     */
    bool IsStarted();

    /**
     * 重设服务器
     * 只是重设与服务器的通信，不影响现有的客户端穿透连接
     * @param[in] szServerIp     服务IP地址
     * @param[in] usServerPort   服务器端口号
     */
    void ResetServer(const char *serverIp, unsigned short serverPort);

#ifdef __NAT_DEVICE_CONFIG_EX__   
	/**
	 * 设置设备的扩展信息
	 * 扩展版
	 * @param[in] extraInfo 设备扩展信息
	 */
	void SetDeviceExtraInfo(const char* extraInfo);
#else
    /**
     * 设置设备的WEB侦听端口号，仅标准版支持
     * @param[in] deviceWebPort  设备的WEB侦听端口号（TCP）
     */
    void SetDeviceWebPort(unsigned short deviceWebPort);
    /**
     * 设置设备的数据侦听端口（TCP），仅标准版支持
     * @param[in] deviceDataPort 设备的数据侦听端口（TCP）
     */
    void SetDeviceDataPort(unsigned short deviceDataPort);
#endif
    /**
     * 获取设备的服务器状态
     */
    NAT_DEVICE_SERVER_STATE GetServerState();

    /**
     * 获取设备的服务器错误
     */
    NAT_DEVICE_SERVER_ERROR GetServerError();
protected:
	void OnConnect(NatSocket sock, unsigned long iIP, unsigned short nPort);
private:
    void Clear();
private:
    class COnConnectNotifier;
    friend class COnConnectNotifier;
    COnConnectNotifier *m_onConnectNotifier;
private:
    /**
     * 禁止使用默认的拷贝构造函数
     */
    CNatListenProc(const CNatListenProc&);
    /**
     * 禁止使用默认的=运算符
     */
    CNatListenProc& operator=(const CNatListenProc&);

#ifndef __NAT_DEVICE_CONFIG_EX__   
private:
	struct DEV_EXTRA_INFO;
	void UpdateExtraInfo();
	DEV_EXTRA_INFO		*m_extraInfo;
#endif//!__NAT_DEVICE_CONFIG_EX__
private:
	bool				m_isStarted;     	//是否已开始侦听
	NAT_ACCEPT_LINK_CALLBACK m_pAcceptCallback;    //有客户端connect上来，accept后调用此回调函数
	void				*m_pParam;             //回调函数的
	PUB_lock_t			m_listenLock;      	//开始监听和停止监听的锁
	CLIENT_SOCK_INFO	m_clientSockInfo;
	CNatDevicePeer      *m_pNatDevicePeer;   //Nat设备端入口类
};

#endif//_NAT_LISTEN_PROC_H_
