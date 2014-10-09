// NatTraversalClientPeer.h: interface for the CNatTraversalClientPeer class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_TRAVERSAL_CLIENT_PEER_H_
#define _NAT_TRAVERSAL_CLIENT_PEER_H_

#include "NatRunner.h"
#include "NatConnectProc.h"
#include "NatUdtTransport.h"
#include "NatListenProc.h"
#include "NatTraversalClient.h"
#include <string>

class CNatDnsParser
{
public:
    enum ParseState
    {
        STATE_NONE,
        STATE_BEGIN_INIT,
        STATE_BEGIN_PARSE,
        STATE_PARSING,
        STATE_COMPLETED
    };
public:
    CNatDnsParser();
    ~CNatDnsParser();
    bool Initialize();
    void Finalize();
    bool IsInitialized();

    void BeginParse(const char* destName);

    ParseState GetState() { return m_state; };

    bool IsCompleted() { return m_state == STATE_COMPLETED; };

    bool IsSucceeded();

    unsigned long GetDestIp() { return m_destIp; };

    CPUB_Lock& GetLock() { return m_lock; };
private:

    static PUB_THREAD_RESULT PUB_THREAD_CALL WorkThreadFunc( void *pParam);
    int RunWork();
private:
    ParseState m_state;
    bool m_workThreadRunning;
    PUB_thread_t m_workThreadId;
    CPUB_Lock      m_lock;
    std::string m_destName;
    unsigned long m_destIp;
    bool m_isSucceeded;
};

class CNatDevicePeer;

/**
* NAT穿透客户端
* 主要是负责与NAT服务器的通信处理，包括：Traversal XML 协议的解析（客户端部分）
*/
class CNatTraversalDevicePeer: public  CNatTraversalClient::CTraversalNotifier
{
public:
    typedef struct _register_config_
    {
        char           deviceNo[64];        /**< 设备的序列号 */
        char           serverIp[256];       /**< NAT服务器IP地址 */
        unsigned short serverPort;          /**< NAT服务器端口号 */        
		unsigned char  refuseRelay;         /**< 设备是否拒绝中继服务。为0表示不拒绝；为非0表示拒绝。默认为0 */
		char           caExtraInfo[512];    /**< 扩展信息，必须是XML格式，内部的数据只会在设备和客户端上解析，对穿透服务器是透明的 */
    }REGISTER_CONFIG;


    enum ConnectState
    {
        STATE_NONE=0,
        STATE_PARSE_SERV_IP,
        STATE_CONNECTING,
        STATE_REGISTERING,
        STATE_REGISTERED,
        STATE_REGISTER_FAILED,
        STATE_DISCONNECTED
    };

	
	typedef struct _server_reply_info
	{
		bool hasReply;
		NatTraversalCmdId replyCmdId;
		int replyBufSize;
		char replyBuf[NAT_DATA_BLOCK_MAX_SIZE];
	}SERVER_REPLY_INFO;

public:
    static const int UDT_SEND_WINDOW_SIZE_MAX = 3; // * CNatUdt::SEND_PACKET_BUF_SIZE = 2 KB
    static const int UDT_RECV_WINDOW_SIZE_MAX = 4; // * CNatUdt::RECV_PACKET_BUF_SIZE = 6 K

	/****
	*超时时间
	****/
	static const unsigned long ERROR_WAIT_TIME	 = 1*60*1000;     // 错误等待时间

	CNatTraversalDevicePeer();
	~CNatTraversalDevicePeer();

	/**
	* 开始。
	* @param config 设备配置。
	* @return 如果成功，返回0；否则返回其它值。
	*/    
	bool Start(CNatDevicePeer *devicePeer, CNatUdtTransport *pUdtTransport, 
        const REGISTER_CONFIG *config);

	/**
	* 停止。
	* @return 如果成功，返回0；否则返回其它值。
	*/
	void Stop();

	/**
	* 是否已开始。
	* @return 返回是否已开始。
	*/
	bool IsStarted();


    void OnRecvDatagram(const NAT_TRANSPORT_RECV_DATAGRAM* datagram);

	/**
	* 重置与NatServer的连接
	* @return 返回0
	*/
	int ResetNatServer(const char *serverIp, unsigned short serverPort);

	/**
	 * 设置设备的扩展信息
	 * @param[in] extraInfo 设备扩展信息
	 */
	void SetDeviceExtraInfo(const char* extraInfo);

	/**
	*异步驱动接口
	*@return 返回NatRunResult
	*/
	NatRunResult Run(unsigned long curTickCount);

	NAT_DEVICE_SERVER_ERROR GetServerError();

	NAT_DEVICE_SERVER_STATE GetServerState();
public:
    ////////////////////////////////////////////////////////////////////////////////
    // interface for CNatTraversalClientUtil::CTraversalNotifier
    virtual void OnServerConnect();
    virtual void OnActiveCmd(NatTraversalCmdId cmdId, void* cmd);
    virtual void OnReply(NatTraversalCmdId reqId, const void* req, bool succeeded, 
        NatTraversalCmdId replyId, void* reply);

private:

	void InternalClose();

    void ChangeParseServIp();

    bool ChangeConnecting(long serverIp, unsigned short serverPort);

    void ChangeRegistering();

    bool ChangeRegistered(const NAT_DEVICE_REGISTER_REPLY *reply);

    void ChangeRegisterFailed();

	void ChangeDisconnected();

	void ReplyRelayConnectReq(unsigned long dwRequestSeq,unsigned long dwStatus);

    void HandleConnectP2PReq(const NAT_NOTIFY_CONNECT_P2P_REQ *req);

    void HandleConnectRelayReq(const NAT_NOTIFY_CONNECT_RELAY_REQ *req);

	int CheckSendHeartBeat();

	void TrySendReply();

	bool CheckConnectTimeout();

	void  StopTraversalClient();

    void StartSendReply(NatTraversalCmdId replyId, const void* reply, int replySize);

    void RunDnsPaseCompleted();

    bool CheckErrorTimeout();
private:
    REGISTER_CONFIG m_registerConfig;
    REGISTER_CONFIG m_resetConfig;
	CNatUdtTransport *m_pUdtTransport;
	CNatDevicePeer  *m_pDevicePeer;

	SERVER_REPLY_INFO m_replyInfo;
	unsigned long  m_lastSendCmdTime;   //命令超时时间
	unsigned long  m_lastRecvCmdTime; //心跳的开始时间
	unsigned long  m_lastErrorTime;   // 上次错误时间

	bool           m_isResetServer;
	CPUB_Lock      m_resetServerLock;

	ConnectState m_connectState;  //当前连接的状态

    CNatDnsParser m_dnsParser;
    CNatTraversalClient m_traversalClient;
    NAT_DEVICE_SERVER_ERROR m_lastError;
	unsigned long m_curTickCount;

};

#endif