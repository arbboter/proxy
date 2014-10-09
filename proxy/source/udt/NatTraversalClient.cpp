/************************************************************************/
// NatTraversalClient.cpp: 主要负责具体穿透功能的实现，包含xml的封装和解析
//
/************************************************************************/

#include "NatCommon.h"
#include "NatTraversalClient.h"
#include "NatTraversalClientPeer.h"
#include "NatXml.h"
#include "NatDebug.h"

NAT_CLIENT_ERROR Nat_TraslateClientError( TraversalReplyStatus replyStatus )
{
    NAT_CLIENT_ERROR clientError;
    switch(replyStatus)
    {
    case REPLY_STATUS_OK:
        clientError  = NAT_CLI_OK;
        break;
    case REPLY_STATUS_UNKNOWN:
        clientError = NAT_CLI_ERR_UNKNOWN;
        break;
    case REPLY_STATUS_DEVICE_OFFLINE:
        clientError = NAT_CLI_ERR_DEVICE_OFFLINE;
        break;
    case REPLY_STATUS_SERVER_OVERLOAD:
        clientError  = NAT_CLI_ERR_SERVER_OVERLOAD;
        break;
    case REPLY_STATUS_DEVICE_NO_RELAY:
        clientError = NAT_CLI_ERR_DEVICE_NO_RELAY;
        break;
    default:
        clientError  = NAT_CLI_ERR_UNKNOWN;
        break;
    }
    return clientError;
}


NAT_CLIENT_ERROR Nat_TraslateClientError(int replyStatus )
{
    return Nat_TraslateClientError((TraversalReplyStatus)replyStatus);
}

inline bool IsTraversalReplyCmd(NatTraversalCmdId id)
{
    return id >= 20000 && id < 30000;
}

////////////////////////////////////////////////////////////////////////////////
// class CNatTraversalClientUtil implements

CNatTraversalClient::CNatTraversalClient()
:
m_sendTransport(NULL),
m_state(STATE_STOPED),
m_notifier(NULL),
m_reqState(STATE_REQ_NONE),
m_reqSeq(0)
{

}

CNatTraversalClient::~CNatTraversalClient()
{
    Clear();
}

bool CNatTraversalClient::IsConnected()
{
    return m_state == STATE_CONNECTED;
}

bool CNatTraversalClient::Start(unsigned long serverIp, unsigned short serverPort, CNatUdtTransport *sendTransport)
{
    m_sendTransport = sendTransport;
    if(!m_traversalHandler.Initialize())
    {
        NAT_LOG_WARN("Traversal Client initialize traversal handler failed!\n");
        Clear();
        return false;
    }
    m_traversalHandler.SetNotifier(this);

    NAT_UDT_CONFIG udtConfig;
    memset(&udtConfig, 0, sizeof(udtConfig));
    udtConfig.category = NAT_UDT_CATEGORY_P2NS;
    udtConfig.remoteIp = serverIp;
    udtConfig.remotePort = serverPort;
    udtConfig.connectionID = Nat_GetTickCount();
    udtConfig.maxRecvWindowSize = UDT_RECV_WINDOW_SIZE_MAX;
    udtConfig.pRecvHeapDataManeger = NULL;  
    udtConfig.maxSendWindowSize = UDT_SEND_WINDOW_SIZE_MAX;
    udtConfig.pSendHeapDataManeger = NULL;
    if(!m_udt.Start(&udtConfig))
    {
        NAT_LOG_WARN("CNatTraversalClientPeer start udt failed!\n");
        Clear();
        return false;
    }
    m_udt.SetNotifier(this);
    m_udt.SetAutoRecv(true);
    m_reqSeq = 0;
    m_reqStartTime = 0;
    m_state = STATE_CONNECTING;
    m_reqState = STATE_REQ_NONE;
    return true;
}


void CNatTraversalClient::Stop()
{
    Clear();
    m_state = STATE_STOPED;
    m_reqState = STATE_REQ_NONE;
}

bool CNatTraversalClient::IsStarted()
{
    return m_state != STATE_STOPED;
}

NatRunResult CNatTraversalClient::Run(unsigned long curTickCount)
{
    assert(m_state != STATE_STOPED);

    if (m_state == STATE_DISCONNECTED)
    {
        return RUN_DO_FAILED;
    }

    NatRunResult runResult = RUN_NONE;
    NatRunResult tempRet = RUN_NONE;

    tempRet = m_udt.Run();
    runResult = NatRunResultMax(runResult, tempRet);
    if (runResult == RUN_DO_FAILED)
    {
        NAT_LOG_DEBUG("Traversal Client run error! Udt run failed!\n");
        ChangeDisconnected();
        return runResult;
    }

    tempRet = m_traversalHandler.RunSend();
    runResult = NatRunResultMax(runResult, tempRet);
    if (runResult == RUN_DO_FAILED)
    {
        NAT_LOG_DEBUG("Traversal Client run error! Traversal handler run send failed!\n");
        ChangeDisconnected();
        return runResult;
    }

    if (m_state == STATE_CONNECTED)
    {
        if (IsRequesting())
        {
			unsigned long reqStartTime = m_reqStartTime;
            if(PUB_IsTimeOutEx(reqStartTime, NAT_TRAVERSAL_REQ_TIMEOUT, Nat_GetTickCount()))
            {
                NotifyReply(false, NAT_ID_UNKNOW, NULL);
            }
        }
        TrySendReq();
    }
    return runResult;
}

void CNatTraversalClient::NotifyRecvDatagram(const NAT_TRANSPORT_RECV_DATAGRAM* datagram)
{
    // TODO: state?

    unsigned char category = CNatUdt::GetDatagramCategory(datagram);

    if( category == NAT_UDT_CATEGORY_P2NS 
        && m_udt.IsMineDatagram(datagram))
    {
        m_udt.NotifyRecvDatagram(datagram);
    }
    else
    {
        // print warning
    }
}

bool CNatTraversalClient::IsSendingCmd()
{
    return m_state == STATE_CONNECTED && m_traversalHandler.IsSending();
}

void CNatTraversalClient::SendCmd(NatTraversalCmdId cmdId, void *cmd, int cmdLen)
{
    assert(m_state == STATE_CONNECTED && !IsSendingCmd());
    m_traversalHandler.SendCmd(cmdId, cmd, cmdLen);
}

bool CNatTraversalClient::IsRequesting()
{
    return m_state == STATE_CONNECTED && (m_reqState != STATE_REQ_NONE && m_reqState != STATE_REQ_COMPLETED);
}

void CNatTraversalClient::StartRequest(NatTraversalCmdId reqId, const void* req, unsigned long reqLen)
{
    assert(m_state == STATE_CONNECTED && !IsRequesting());
    m_reqId = reqId;
    m_reqLen = reqLen;
    memcpy(m_reqBuf, req, m_reqLen);
    m_reqState = STATE_REQ_STARTING;
    m_reqStartTime = Nat_GetTickCount();
    TrySendReq();
}

unsigned long CNatTraversalClient::GenNextReqSeq()
{
    m_reqSeq++;
    return m_reqSeq;
}

void CNatTraversalClient::SetNotifier(CTraversalNotifier *notifier)
{
    m_notifier = notifier;
}

void CNatTraversalClient::OnConnect( CNatUdt *pUDT, int iErrorCode )
{
    if (m_state == STATE_CONNECTING)
    {
        if (0 == iErrorCode)
        {
            NAT_LOG_DEBUG("Connect NatServer succeed!\n");
            m_state = STATE_CONNECTED;
            if (m_notifier != NULL)
            {
                m_notifier->OnServerConnect();
            }
        }
    }

}

int CNatTraversalClient::OnRecv(CNatUdt *pUDT, const void* pBuf, int iLen)
{
    if (m_state == STATE_CONNECTED)
    {
        m_traversalHandler.RecvTraversalData((char*)pBuf, iLen);
    }
    return iLen;
}

int CNatTraversalClient::OnSendPacket(CNatUdt *pUDT, const void* pBuf, int iLen)
{
    return m_sendTransport->Send(m_udt.GetRemoteIp(), m_udt.GetRemotePort(),
        pBuf, iLen);
}

int CNatTraversalClient::OnSendTraversalData(const char* pData, int iLen)
{
    return m_udt.Send(pData, iLen);
}

void CNatTraversalClient::OnBeforeRecvCmd(const char* version, const NatTraversalCmdId &cmdId, bool &isHandled)
{
    isHandled = true;    
}

void CNatTraversalClient::OnRecvCmd(const char* version, const NatTraversalCmdId &cmdId, void* cmd)
{
    assert(m_state == STATE_CONNECTED);
    if (IsTraversalReplyCmd(cmdId))
    {
        NotifyReply(true, cmdId, cmd);
    }
    else
    {
        if (m_notifier != NULL)
        {
            m_notifier->OnActiveCmd(cmdId, cmd);
        }
    }
}

void CNatTraversalClient::OnRecvCmdError(CNatTraversalXmlParser::ParseError error)
{
    NAT_LOG_INFO("Recv NatServer cmd error: %s", CNatTraversalXmlParser::GetParseErrorText(error));
}

void CNatTraversalClient::Clear()
{
    if (m_traversalHandler.IsInitialized())
    {
        m_traversalHandler.Finalize();
    }
    if (m_udt.IsStarted())
    {
        m_udt.Stop();
    }
    m_sendTransport = NULL;
}

void CNatTraversalClient::ChangeDisconnected()
{
    assert(STATE_STOPED != m_state);
    if (STATE_DISCONNECTED != m_state)
    {
        m_state = STATE_DISCONNECTED;
    }
}

void CNatTraversalClient::TrySendReq()
{
    assert(m_state == STATE_CONNECTED);
    if (m_reqState == STATE_REQ_STARTING)
    {
        if (!m_traversalHandler.IsSending())
        {
            m_traversalHandler.SendCmd(m_reqId, m_reqBuf, m_reqLen);
            m_reqState = STATE_REQ_SENT;
        }
    }
}

void CNatTraversalClient::NotifyReply(bool succeeded, NatTraversalCmdId replyId, void* reply)
{
    if (IsRequesting())
    {
        m_reqState = STATE_REQ_COMPLETED;
        if (m_notifier != NULL)
        {
            m_notifier->OnReply(m_reqId, m_reqBuf, succeeded, replyId, reply);
        }
    }

}
