// NatTraversalClientPeer.h: interface for the CNatTraversalClientPeer class.
//  备注：一条命令需处理完成后，才能处理下一条命令
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_TRAVERSAL_CLIENT_H_
#define _NAT_TRAVERSAL_CLIENT_H_

#include "NatSocketBase.h"
#include "NatRunner.h"
#include "NatConnectProc.h"
#include "NatUdtTransport.h"
#include "NatUdt.h"
#include "NatTraversalHandler.h"
#include "NatTraversalProtocol.h"
#include "NatSimpleDataBlock.h"
#include "tinyxml.h"
#include "RelayObj.h"

/**
 * 穿透客户端
 * 主要处理穿透协议的客户相关逻辑，与穿透协议的服务器相对应
 */
class CNatTraversalClient: public CUdtNotifier, CNatTraversalHandler::CHandlerNotifier
{
public:
    static const int UDT_SEND_WINDOW_SIZE_MAX = 3; // * CNatUdt::SEND_PACKET_BUF_SIZE = 2 KB
    static const int UDT_RECV_WINDOW_SIZE_MAX = 4; // * CNatUdt::RECV_PACKET_BUF_SIZE = 6 K
public:
    enum TraversalServerState
    {
        STATE_STOPED,
        STATE_CONNECTING,
        STATE_CONNECTED,
        STATE_DISCONNECTED
    };

    enum TraversalRequestState
    {
        STATE_REQ_NONE,
        STATE_REQ_STARTING,
        STATE_REQ_SENT,
        STATE_REQ_COMPLETED
    };

    class CTraversalNotifier
    {
    public:
        /**
         * 与服务器连接成功的回调通知
         */
        virtual void OnServerConnect() = 0;
        /**
         * 接收主动命令回调通知
         * 主动命令包括普通命令和请求命令，应答命令是通过OnReply回调的。
         */
        virtual void OnActiveCmd(NatTraversalCmdId cmdId, void* cmd) = 0;

        /**
         * 接收请求的应答回调通知
         * @param[in] reqId     请求ID
         * @param[in] req       请求数据
         * @param[in] succeeded 请求是否成功，如果失败，表示为超时，且需要忽略后面的字段（replyId和reply）
         * @param[in] replyId   应答ID
         * @param[in] reply     应答数据
         */
        virtual void OnReply(NatTraversalCmdId reqId, const void* req, bool succeeded,
            NatTraversalCmdId replyId, void* reply) = 0;

    };

public:
    CNatTraversalClient();
    ~CNatTraversalClient();

    bool Start(unsigned long serverIp, unsigned short serverPort, 
        CNatUdtTransport *sendTransport);

    void Stop();

    bool IsStarted();

    bool IsConnected();

    TraversalServerState GetConnectState() { return m_state; };

    NatRunResult Run(unsigned long curTickCount);

    void NotifyRecvDatagram(const NAT_TRANSPORT_RECV_DATAGRAM* datagram);

    bool IsSendingCmd();

    void SendCmd(NatTraversalCmdId cmdId, void *cmd, int cmdLen);

    bool IsRequesting();

    void StartRequest(NatTraversalCmdId reqId, const void* req, unsigned long reqLen);

    unsigned long GenNextReqSeq();

    void SetNotifier(CTraversalNotifier *notifier);
public:
    ////////////////////////////////////////////////////////////////////////////////
    // interface for CUdtNotifier
    virtual void OnConnect(CNatUdt *pUDT, int iErrorCode);

    virtual int OnRecv(CNatUdt *pUDT, const void* pBuf, int iLen);

    virtual int OnSendPacket(CNatUdt *pUDT, const void* pBuf, int iLen);
public:
    ////////////////////////////////////////////////////////////////////////////////
    // CNatTraversalHandler::CHandlerNotifier public interface
    virtual int OnSendTraversalData(const char* pData, int iLen);

    virtual void OnBeforeRecvCmd(const char* version, const NatTraversalCmdId &cmdId, bool &isHandled);

    virtual void OnRecvCmd(const char* version, const NatTraversalCmdId &cmdId, void* cmd);

    virtual void OnRecvCmdError(CNatTraversalXmlParser::ParseError error);
private:
    void Clear();
    void ChangeDisconnected();
    void TrySendReq();
    void NotifyReply(bool succeeded, NatTraversalCmdId replyId, void* reply);
private:

    CNatUdtTransport *m_sendTransport;
    TraversalServerState m_state;
    CTraversalNotifier *m_notifier;
    CNatUdt m_udt;
    CNatTraversalHandler m_traversalHandler;

    TraversalRequestState m_reqState;
    unsigned long m_reqStartTime;
    NatTraversalCmdId m_reqId;
    char m_reqBuf[NAT_DATA_BLOCK_MAX_SIZE];
    int m_reqLen;
    unsigned long m_reqSeq;
};


NAT_CLIENT_ERROR Nat_TraslateClientError( TraversalReplyStatus replyStatus );

NAT_CLIENT_ERROR Nat_TraslateClientError(int replyStatus);

#endif//_NAT_TRAVERSAL_CLIENT_H_