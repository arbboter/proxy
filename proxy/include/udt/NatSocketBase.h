// NatSocketBase.h: interface for the CNatSocketBase class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_SOCKET_BASE_H_
#define _NAT_SOCKET_BASE_H_

#include "NatCommon.h"
#include "NatSocket.h"
#include "NatRunner.h"

class CNatSocketBase;

/**
 * Socket通信数据处理回调通知
 */
class CNatSocketCommNotifier
{
public:
    virtual ~CNatSocketCommNotifier() {};
    /**
     * 接收数据回调函数。
     * 当有数据接收后，触发该回调。
     * @param sock 套接字。
     * @param pBuf 数据指针，指向已接收的数据。
     * @param iLen 要接收数据的字节大小。
     * @return 返回已经处理的数据字节大小
     */
    virtual int OnRecv(CNatSocketBase *sock, const void* pBuf, int iLen) = 0;
    
};

/**
 * NatSocket基础类，
 * 定义了NatSocket的行为接口，比如Send()和Recv()以及标准的状态。
 */
class CNatSocketBase: public CNatRunner
{
public:
    /**
     * NatSocket的状态
     */
    enum SocketState
    {
        STATE_CLOSED,       /**< 已关闭，表示NatSocket已经处于无效状态 */
        STATE_INIT,         /**< 已初始化，表示NatSocket已经初始化，此时正在处理连接过程 */
		STATE_CONNECTING,   /**< 正在连接，表示IP层的连接已建立，协议层的连接正在交互*/
        STATE_CONNECTED,    /**< 已连接，表示NatSocket已经建立连接，可以正常收发数据 */
        STATE_DISCONNECTED  /**< 已断开连接，表示NatSocket已经断开连接，即将被关闭 */
    };
public:
    /**
     * 构造函数
     */
    CNatSocketBase();
    /**
     * 析构函数
     */
    virtual ~CNatSocketBase();
    /**
     * 是否已经建立连接
     * @return 返回是否已经建立连接
     */
    bool IsConnected();
    /**
     * 是否已经初始化
     * @return 返回是否已经初始化
     */
    bool IsInited();

    /**
     * 发送数据。
     * @param[in] pBuf 数据指针，指向要发送的数据。
     * @param[in] iLen 要发送数据的字节大小。
     * @return 如果没有错误，则返回实际发送的字节数，该数要小于或等于iLen；否则返回错误值-1。
     */    
    virtual int Send(const void *pBuf, int iLen) = 0;

    /**
     * 接收数据
     * 主动接收数据，需要在非自动接收模式下才能起作用，接收模式默认情况下是非自动模式
     * @param[in] pBuf 数据指针，指向要接收数据的内存。
     * @param[in] iLen 要接收数据的字节大小。
     * @return 如果没有错误，则返回实际接收的字节数，该数要小于或等于iLen；否则返回错误值-1。
     */
    virtual int Recv(void *pBuf, int iLen) = 0;

    /**
     * 获取当前可以发送数据的字节大小
     * @return 如果没有错误，则返回当前可以发送数据的字节大小，0表示发送缓冲区满了；否则，出错返回-1
     */
    virtual int GetSendAvailable() = 0;

    /**
     * 获取可以接收数据的字节大小，主要用于主动接收数据
     * @return 如果没有错误，则返回当前可以接收数据的字节大小，0表示无数据；否则，出错返回-1
     */
    virtual int GetRecvAvailable() = 0;

    /**
     * 设置是否启用自动接收模式
     * NatSocket默认情况下是处于非自动接收模式
     * @param[in] bAutoRecv 是否启用自动接收模式
     * @return 如果没有错误，返回0；否则，出错返回-1（一般是NatSocket已经处于断开连接或已关闭状态）
     */
    virtual int SetAutoRecv(bool bAutoRecv) = 0;
    
    /**
     * 设置传输通信（Communication）通知回调
     * 调用者只有设置该通知回调，才能接收自动接收模式所接收到的数据
     * @param[in] notifier 传输通信通知回调
     */
    virtual void SetCommNotifier( CNatSocketCommNotifier* notifier );

    /**
     * 关闭NatSocket
     * 执行关闭后，NatSocket应该处于Closing状态，内部处理会尽快将其真正的关闭。
     * @return 如果没有错误，则返0；否则，返回错误值-1。
     */
    virtual int Close();

    /**
     * 获取远程连接的IP地址
     * 只有当NatSocket已经建立连接以后，才能确保该值是精确的
     * @return 返回远程连接的IP地址
     */
    virtual unsigned long GetRemoteIp() = 0;
    /**
     * 获取远程连接的端口号
     * 只有当NatSocket已经建立连接以后，才能确保该值是精确的
     * @return 返回远程连接的端口号
     */
    virtual unsigned short GetRemotePort() = 0;

    /**
     * 获取NatSocket的状态
     * @return 返回NatSocket的状态
     */
    SocketState GetState();

	/**
     * 标示已被使用，不能直接被释放
     */
	void Use();

	 /**
     * 是否正在使用
     * @see Use()
     * @return 是否处于正在使用的过程
     */
    bool IsUsing();
protected:
    /**
     * 通知有数据接收
     * 当处于自动接收模式，且收到了对方发送的数据，则应调用该函数，以触发数据接收的回调通知
     * 子类可以使用
     */
    virtual int NotifyOnRecv(const void* pBuf, int iLen);

    /**
     * 更改NatSocket的状态
     * 子类可以使用
     */
    virtual void ChangeState(SocketState state);
protected:
    CNatSocketCommNotifier* m_commNotifier;
    SocketState m_socketState;
    bool m_bIsUsing;
    CPUB_Lock m_notifierLock;
};

/**
 * Socket连接回调通知
 */
class CNatSocketConnectNotifier
{
public:
    virtual ~CNatSocketConnectNotifier() {};
    virtual void OnConnect(NatSocket sock, int iErrorCode)= 0;
};


#endif//_NAT_SOCKET_BASE_H_