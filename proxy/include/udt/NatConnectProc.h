// NatConnectProc.h: interface for the CNatConnectProc class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_CONNECT_PROC_H_
#define _NAT_CONNECT_PROC_H_

#include "NatUserDefine.h"

#ifdef NAT_TVT_DVR_4_0
#include "TVT_PubCom.h"
#else
#include "PUB_common.h"
#endif//NAT_TVT_DVR_4_0

#include "SWL_Public.h"
#include "NatSocket.h"

/**
 * 当连接建立以后，调用该函数
 * @param[in] sock 新连接的套接字，如果出错，则等于-1
 * @param[in] pObject 回调相关对象指针
 * @param[in] nParam 保留参数。
 * @return 返回值未定义。
 */
typedef int (*NAT_CONNECT_LINK_CALLBACKEX)(NatSocket sock, void *pObject, void *pParam);

/**
 * 客户端的穿透类型的配置
 */
typedef struct _nat_client_traversal_config_
{
    char           dwDeviceNo[64];  /*< 要连接的设备的序列号 */
    char           pServerIp[256];  /*< NAT服务器IP地址 */
    unsigned short nServerPort;     /*< NAT服务器端口号 */
}NAT_CLIENT_TRAVERSAL_CONFIG;

/**
 * 客户端的直连类型的配置
 */
typedef struct _nat_client_direct_config_
{
    unsigned long dwDeviceIp;            /*< 要连接的设备的IP地址 */
    unsigned short nDevicePort;  /*< 要连接的设备的端口号 */    
}NAT_CLIENT_DIRECT_CONFIG;

typedef enum
{
	TRAVERSAL_CONFIG,
	DIRECT_CONFIG
} CONFIG_TYPE;

/**
* 穿透模式
* 应先执行P2P穿透，然后才执行Relay穿透
*/
enum TraversalMode
{
	TRAVERSAL_P2P = 0,
	TRAVERSAL_RELAY
};

/**
 * 客户端配置
 */
typedef struct _nat_client_config_
{
	CONFIG_TYPE ConfigType;
    union
    {
        NAT_CLIENT_TRAVERSAL_CONFIG TraversalConfig;
        NAT_CLIENT_DIRECT_CONFIG DirectConfig;
    };
}NAT_CLIENT_CONFIG;

class CNatUdtClientPeer; 

class CNatTraversalClientPeer;

/**
 * 连接处理，连接成功或者失败后都需要释放掉改对象.
 */
class CNatConnectProc
{

public:
    /**
     * 新建连接处理。
     * @param[in] config 客户端配置。
     * @return 如果成功，新建的连接者；否则返回NULL。
     */  
    static CNatConnectProc* NewConnectProc(const NAT_CLIENT_CONFIG *config);

	//新建连接处理，直连的方式
	static CNatConnectProc* NewConnectProc(const NAT_CLIENT_DIRECT_CONFIG *config);


	//当前连接的模式 0：穿透，1：转发
	void SetConnectTraversalMode(TraversalMode mode);

    /**
     * 销毁函数。
     * @return 如果成功，返回0；否则，返回其它值。
     */  
    int Destroy();

	/**
	 * 以同步的方式尝试连接一次
	 * @param[in] timeOut 连接超时时间，单位：毫秒
	 * @return 如果返回非NULL值，该值表示成功连接的NatSocket；否则返回NULL表示失败。
	 */		
	NatSocket ConnectSyn(int timeOut);


	/**
	 * 以同步的方式尝试连接一次
	 * @param[in] timeOut 连接超时时间，单位：毫秒
	 * @param[in] cancel 是否取消连接过程指针，由调用者传入。可以为NULL，表示不处理取消过程；
	 *                   不为NULL时，传入时应初始化为false，调用者需要取消连接过程时，只需将指针所指内容设为true即可。
	 * @return 如果返回非NULL值，该值表示成功连接的NatSocket；否则返回NULL表示失败。
	 */
	NatSocket ConnectSyn(int timeOut, bool *cancel);

    /**
    * 开始以异步方式执行连接。
     * @param[in] pCallback 连接回调函数。
     * @param[in] pObject 相关联对象指针。
     * @param[in] iTimeOut 连接超时。
     * @return 如果成功，返回0；否则，返回其它值。
     */  
    int ConnectAsyn(NAT_CONNECT_LINK_CALLBACKEX pCallback, void* pObject, int iTimeOut);	


	/**
	 * 获取连接错误
	 */
	NAT_CLIENT_ERROR GetConnectError();
	/**
	 * 获取连接出错时的状态
	 */
	NAT_CLIENT_STATE GetConnectState();
protected:
    CNatConnectProc(const NAT_CLIENT_CONFIG &config);
    virtual ~CNatConnectProc();
protected:
	void NotifyOnConnect(NatSocket sock, NAT_CLIENT_ERROR iErrorCode);
private:

	int CreateNatSocket(int iTimeOut);

    bool Init();

	int Clear();
private:
    /**
     * 禁止使用默认的拷贝构造函数
     */
    CNatConnectProc(const CNatConnectProc&);
    /**
     * 禁止使用默认的=运算符
     */
    CNatConnectProc& operator=(const CNatConnectProc&);
private:
    class COnConnectNotifier;
    friend class COnConnectNotifier;
    COnConnectNotifier *m_onConnectNotifier;
private:
    enum ConnectState
    {
        STATE_NONE,
        STATE_CONNECTING,
        STATE_FINISHED
    };
private:
    ConnectState m_state;
    bool m_isConnectSync;
    NatSocket m_syncSock;
	int						  m_iTimeOut;	   //异步连接的时候的超时时间，以为毫秒为单位

	CNatUdtClientPeer		*m_pClientPeer;      //直连
	CNatTraversalClientPeer *m_pNatTraversalClientPeer; //穿透模式
	TraversalMode				m_connectTraversalMode;    //连接的模式

	NAT_CONNECT_LINK_CALLBACKEX m_pConnectCallback;
	void					  *m_pConnectCallbackParam;

	CPUB_Lock				m_ConnectSynLock; 
    NAT_CLIENT_CONFIG		m_config;
    NAT_CLIENT_ERROR		m_connectError;
	NAT_CLIENT_STATE		m_clientState;
};

#endif//_NAT_CONNECT_PROC_H_