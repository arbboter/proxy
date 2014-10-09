// NatTraversalClientPeer.h: interface for the CNatTraversalClientPeer class.
//  备注：一条命令需处理完成后，才能处理下一条命令
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_TRAVERSAL_CLIENT_PEER_H_
#define _NAT_TRAVERSAL_CLIENT_PEER_H_

#include "NatSocketBase.h"
#include "NatRunner.h"
#include "NatConnectProc.h"
#include "NatUdtTransport.h"
#include "NatUdt.h"
#include "NatTraversalProtocol.h"
#include "NatSimpleDataBlock.h"
#include "tinyxml.h"
#include "RelayObj.h"
#include "NatTraversalClient.h"
#include "NatClientUdtSocket.h"


/**
* 穿透的状态
*/
enum NatTraversalClientState
{
    TRAVERSAL_STATE_NONE = 0,           /**< 初始状态，处于无效的状态 */
    TRAVERSAL_STATE_SER_CONNECTING,     /**< 正在向NAT服务器连接的状态 */
    TRAVERSAL_STATE_SER_REQUSTING,      /**< 正在向NAT服务器请求的状态 */
    TRAVERSAL_STATE_DEV_CONNECTING,     /**< 正在与对方（设备）建立连接的状态 */
    TRAVERSAL_STATE_DEV_CONNECTED      /**< 已经与对方（设备）建立连接的状态 */
};

class CNatP2PClientPeer: public CNatUdtTransportNotifier,CUdtNotifier,
    CNatTraversalClient::CTraversalNotifier
{
public:
    static const int UDT_SEND_WINDOW_SIZE_MAX = 170; // * CNatUdt::SEND_PACKET_BUF_SIZE = 92 KB
    static const int UDT_RECV_WINDOW_SIZE_MAX = 171; // * CNatUdt::RECV_PACKET_BUF_SIZE = 256 K

    static const int TRAVERSAL_P2P_TIMEOUT = 60 * 1000;
public:
    CNatP2PClientPeer();
    ~CNatP2PClientPeer();

    void SetTraversalTimeout(unsigned long timeout);
    bool Initilize(const NAT_CLIENT_TRAVERSAL_CONFIG* config, unsigned long natServerIp);
    NatTaskResult RunTask(unsigned long curTickCount);
    NatTraversalClientState GetState() { return m_state; };
    NAT_CLIENT_ERROR GetError() { return m_error; };
    bool IsP2PErrorOnly() { return m_isP2PErrorOnly; };
    NatSocket HandleCompleteResult();
	NAT_CLIENT_STATE GetClientState();
public:
    ////////////////////////////////////////////////////////////////////////////////
    // Implements CNatUdtTransportNotifier public interface

    virtual void OnRecvDatagram(CNatUdtTransport *transport, const NAT_TRANSPORT_RECV_DATAGRAM* datagram);
public:
    ////////////////////////////////////////////////////////////////////////////////
    // Implements CUdtNotifier public interface

    virtual void OnConnect(CNatUdt *pUDT, int iErrorCode);

    virtual int OnRecv(CNatUdt *pUDT, const void* pBuf, int iLen);

    virtual int OnSendPacket(CNatUdt *pUDT, const void* pBuf, int iLen);
public:
    ////////////////////////////////////////////////////////////////////////////////
    // interface for CNatTraversalClientUtil::CTraversalNotifier
    virtual void OnServerConnect();
    virtual void OnActiveCmd(NatTraversalCmdId cmdId, void* cmd) { /* ignore */};
    virtual void OnReply(NatTraversalCmdId reqId, const void* req, bool succeeded, 
        NatTraversalCmdId replyId, void* reply);
private:
    bool HandleP2pConnectReply(NAT_CLIENT_CONNECT_P2P_REPLY *p2pReply);

    bool ResetP2pConnect(unsigned short newPort);

    void SetP2PFailed();
    void SetError(NAT_CLIENT_ERROR error);
private:
    unsigned long m_connectionId;
    NatTraversalClientState m_state;
    NAT_CLIENT_TRAVERSAL_CONFIG m_config;
    CNatUdtTransport	  *m_pUdtTransport;
    CNatTraversalClient m_traversalClient;
    CNatUdt   *m_p2pUdt;
    NAT_UDT_CONFIG m_p2pUdtConfig;
    unsigned long m_timeoutMs;
    unsigned long m_startTimeMs;
    NAT_CLIENT_ERROR m_error;
    bool m_isP2PErrorOnly;
    
};

class CNatRelayClientPeer: public CNatUdtTransportNotifier,CRelayNotifier,
    CNatTraversalClient::CTraversalNotifier
{
public:
    static const int TRAVERSAL_RELAY_TIMEOUT = 30 * 1000;
public:
    CNatRelayClientPeer();
    ~CNatRelayClientPeer();
    void SetTraversalTimeout(unsigned long timeout);
    bool Initilize(const NAT_CLIENT_TRAVERSAL_CONFIG* config, unsigned long natServerIp);
    NatTaskResult RunTask(unsigned long curTickCount);
	NatTraversalClientState GetState() { return m_state; };
	NAT_CLIENT_ERROR GetError() { return m_error; };    
	NatSocket HandleCompleteResult();
	NAT_CLIENT_STATE GetClientState();
public:
    ////////////////////////////////////////////////////////////////////////////////
    // Implements CNatUdtTransportNotifier public interface

    virtual void OnRecvDatagram(CNatUdtTransport *transport, const NAT_TRANSPORT_RECV_DATAGRAM* datagram);
public:
    ////////////////////////////////////////////////////////////////////////////////
    // Implements CRelayNotifier public interface
    virtual void RelayOnConnect(CNatSocketBase *pSockBase, int iErrorCode);
public:
    ////////////////////////////////////////////////////////////////////////////////
    // interface for CNatTraversalClientUtil::CTraversalNotifier
    virtual void OnServerConnect();
    virtual void OnActiveCmd(NatTraversalCmdId cmdId, void* cmd) { /* ignore */};
    virtual void OnReply(NatTraversalCmdId reqId, const void* req, bool succeeded, 
        NatTraversalCmdId replyId, void* reply);
private:
    //连接转发服务器
    int HandleRelayConnectReply(NAT_CLIENT_CONNECT_RELAY_REPLY *relayReply);
    void SetError(NAT_CLIENT_ERROR error);
private:
    NatTraversalClientState m_state;
    NAT_CLIENT_TRAVERSAL_CONFIG m_config;

    unsigned long m_timeoutMs;
    unsigned long m_startTimeMs;
    CNatUdtTransport	  *m_pUdtTransport;
    CRelayObj *m_pRelaySock;
    CNatTraversalClient m_traversalClient;
    NAT_CLIENT_ERROR m_error;
};

/*************************************/
/*不管连接成功或者失败，该对象都需释放掉。
**************************************/
class CNatTraversalClientPeer
{
public:
    static const int CONNECT_TIMEOUT_MS = CNatRelayClientPeer::TRAVERSAL_RELAY_TIMEOUT
        + CNatP2PClientPeer::TRAVERSAL_P2P_TIMEOUT;
    static const double P2P_TIMEOUT_RATIO;
    static const double RELAY_TIMEOUT_RATIO;
public:    

    CNatTraversalClientPeer();
    virtual ~CNatTraversalClientPeer();

    void SetConnectTimeout(unsigned long timeoutMs);

    int Init(const NAT_CLIENT_TRAVERSAL_CONFIG* config);

    void SetConnectNotifier( CNatSocketConnectNotifier* notifier);

    void SetStartTraversalMode(TraversalMode mode);

    TraversalMode GetTraversalMode();

	NAT_CLIENT_STATE GetClientState();
private:
    void NotifyOnConnect(NatSocket sock,int iErrorCode);

    void HandleComplete(NAT_CLIENT_ERROR error);

    static PUB_THREAD_RESULT PUB_THREAD_CALL TraversalThreadFunc(void *pParam);

    int RunWork();

    int InitTraversalClient();

    bool EnterRelayMode();

private:
    bool m_initialized;
    NAT_CLIENT_TRAVERSAL_CONFIG m_config;

    TraversalMode m_traversalMode;       //穿透模式 0:穿透 1:转发
    unsigned long m_connectTimeoutMs;

    CNatSocketConnectNotifier *m_connectNotifier;
    CPUB_Lock m_notifierLock;

    bool m_bClientAdded; //是否被加入ClientManager;

    bool      m_traversalThreadRuning;  
    PUB_thread_t m_traversalThreadID;

    unsigned long m_natServerIp;

    CNatP2PClientPeer *m_p2pClient;
    CNatRelayClientPeer *m_relayClient;
	NAT_CLIENT_STATE m_clientState;
};

#endif//_NAT_TRAVERSAL_CLIENT_PEER_H_