// NatDevicePeer.h: interface for the CNatDevicePeer class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_DEVICE_PEER_H_
#define _NAT_DEVICE_PEER_H_

#include "NatCommon.h"
#include <vector>
#include <string>
#include "NatListenProc.h"
#include "NatSocketBase.h"
#include "NatUdtTransport.h"
#include "NatTraversalDevicePeer.h"
#include "NatUdt.h"
#include "RelayObj.h"

class CNatDevicePeer;

class CNatUdtDeviceClient: public CNatSocketBase, CUdtNotifier
{
public:
    static const int UDT_SEND_WINDOW_SIZE_MAX = 170; // * CNatUdt::SEND_PACKET_BUF_SIZE = 92 KB
    static const int UDT_RECV_WINDOW_SIZE_MAX = 171; // * CNatUdt::RECV_PACKET_BUF_SIZE = 256 K

    enum CLIENT_COME_IN_MODE
    {
        MODE_DIRECT,    // 直接连入的模式
        MODE_TRAVERSAL, // 穿透连入的模式
        MODE_MIXED      // 混合模式，有可能客户端是对称形NAT
    };
public:
    CNatUdtDeviceClient(CNatDevicePeer *devicePeer, unsigned long connectionId,
        unsigned long fromIP, unsigned short fromPort, CLIENT_COME_IN_MODE comeInMode);
    virtual ~CNatUdtDeviceClient();

    virtual void RecvPacket(const NAT_TRANSPORT_RECV_DATAGRAM* datagram);
    bool IsMineDatagram(const NAT_TRANSPORT_RECV_DATAGRAM* datagram);
    bool IsSymmetricClientDatagram(const NAT_TRANSPORT_RECV_DATAGRAM* datagram);
    void Reset(const NAT_TRANSPORT_RECV_DATAGRAM* datagram);
    CLIENT_COME_IN_MODE GetComeInMode();
public:
    virtual int Send(const void *pBuf, int iLen);

    virtual int Recv(void *pBuf, int iLen);
    /**
     * 获取当前可以发送数据的字节大小
     * @return 如果没有错误，则返回当前可以发送数据的字节大小，0表示发送缓冲区满了；否则，出错返回-1
     */
    virtual int GetSendAvailable();

    /**
     * 获取可以接收数据的字节大小，主要用于主动接收数据
     * @return 如果没有错误，则返回当前可以接收数据的字节大小，0表示无数据；否则，出错返回-1
     */
    virtual int GetRecvAvailable();

    virtual int SetAutoRecv(bool bAutoRecv);

    virtual int Close();

    virtual unsigned long GetRemoteIp();

    virtual unsigned short GetRemotePort();

	virtual unsigned long GetConnectionId();
public:
    virtual void OnConnect(CNatUdt *pUDT, int iErrorCode);

    virtual int OnRecv(CNatUdt *pUDT, const void* pBuf, int iLen);

    virtual int OnSendPacket(CNatUdt *pUDT, const void* pBuf, int iLen);
public:
    virtual NatRunResult Run();
private:
    void ChangeDisConnected();
private:
    NAT_UDT_CONFIG m_config;
    CNatDevicePeer *m_devicePeer;
    CNatUdt m_udt;
    bool m_isClosed;
    CPUB_Lock m_lock;
    CLIENT_COME_IN_MODE m_comeInMode;
};

// TODO: CNatRelayDeviceClient;


/**
 * 设备结点
 */
class CNatDevicePeer: public CRelayNotifier
{
    friend class CNatUdtDeviceClient;
public:
    CNatDevicePeer();
    virtual ~CNatDevicePeer();

    /**
     * 开始。
     * @param config 设备配置。
     * @return 如果成功，返回0；否则返回其它值。
     */    
	int Start(const NAT_DEVICE_CONFIG_EX *config);
    /**
     * 停止。
     * @return 如果成功，返回0；否则返回其它值。
     */
    int Stop();
    /**
     * 是否已开始。
     * @return 返回是否已开始。
     */
    bool IsStarted();


	 /*brief 
	 * 重置与NatServer的连接
	 */
	int ResetServer(const char *serverIp, unsigned short serverPort);

	/**
	 * 设置设备的扩展信息
	 * @param[in] extraInfo 设备扩展信息
	 */
	void SetDeviceExtraInfo(const char* extraInfo);

    /**
     * 设置通知回调。
     * 只支持单通知回调。
     * @param notifier 通知回调
     */
    void SetConnectNotifier( CNatSocketConnectNotifier* notifier);

    CNatSocketConnectNotifier* GetConnectNotifier();
    
	void RelayOnConnect(CNatSocketBase* pSockBase, int iErrorCode);


	NAT_DEVICE_SERVER_ERROR GetServerError();

	NAT_DEVICE_SERVER_STATE GetServerState();

    // TODO: Get informations 
public:
    ////////////////////////////////////////////////////////////////////////////////
	// interface for class CNatTraversalDevicePeer

	int  OnP2PClientConnect(unsigned long connectionId, unsigned long clientPeerIp,
        unsigned short clientPeerPort);

	void OnRelayClientConnect(unsigned long connectionId, unsigned long relayServerIp, 
        unsigned short relayServerPort);
private:
    CNatUdtDeviceClient* FindUdtClient(const NAT_TRANSPORT_RECV_DATAGRAM* datagram);
    CNatUdtDeviceClient* FindUdtTraversalClient(const NAT_TRANSPORT_RECV_DATAGRAM* datagram);
    CNatUdtDeviceClient* FindUdtClient(unsigned long connectionId,unsigned long fromIP, unsigned short fromPort);
    CNatUdtDeviceClient* CreateUdtClient(unsigned long connectionId,unsigned long fromIP, 
        unsigned short fromPort, CNatUdtDeviceClient::CLIENT_COME_IN_MODE comeInMode);
	CRelayObj* FindRelayClient(unsigned long connectionID);
	CRelayObj* CreateRelayClient(unsigned long connectionID, unsigned long serverIp,unsigned short serverPort);

    void InternalStop();
    NatRunResult ClearClosedClients();
private:
    void NotifyOnConnect(CNatSocketBase* sockBase);
private:
    static PUB_THREAD_RESULT PUB_THREAD_CALL WorkThreadFunc(void *pParam);
    int RunWork();
private:
    /**
     * 用于响应CNatUdtTransport的通知回调，并将数据进行分发。
     */
    class CUdtTransportDispatch: public CNatUdtTransportNotifier
    {
    public:
        CUdtTransportDispatch();

        void Init(CNatDevicePeer* devicePeer);

        virtual void OnRecvDatagram(CNatUdtTransport *transport, const NAT_TRANSPORT_RECV_DATAGRAM* datagram);
    private:
        CNatDevicePeer* m_devicePeer;
    };

    friend class CUdtTransportDispatch;
private:
    bool m_started;
    char m_deviceNo[64];    /**< 设备的序列号 */
    char m_serverIp[256];      /**< NAT服务器IP地址 */
    unsigned short m_serverPort; /**< NAT服务器端口号 */
    CNatSocketConnectNotifier *m_connectNotifier;
    std::vector<CNatUdtDeviceClient*> m_udtClients;
	std::vector<CRelayObj*> m_RelayClients;
    CNatUdtTransport m_udtTransport;
    CUdtTransportDispatch m_udtTransportDispatch;
	CNatTraversalDevicePeer m_traversalDevicePeer;

    bool m_workThreadRunning;
    PUB_thread_t m_workThreadID;
    CPUB_Lock m_clientsLock;
    CPUB_Lock m_notifierLock;
};

#endif //_NAT_DEVICE_PEER_H_