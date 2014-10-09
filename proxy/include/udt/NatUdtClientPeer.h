// NatUdtClientPeer.h: interface for the CNatUdtClientPeer class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_UDT_CLIENT_PEER_H_
#define _NAT_UDT_CLIENT_PEER_H_

#include "NatSocketBase.h"
#include "NatRunner.h"
#include "NatConnectProc.h"
#include "NatUdtTransport.h"
#include "NatUdt.h"
#include "NatClientUdtSocket.h"

class CNatUdtClientPeer: public CNatUdtTransportNotifier, CUdtNotifier
{
public:
	enum P2PConnectState
	{
		STATE_P2P_NONE,
		STATE_P2P_CONNECTING,
		STATE_P2P_CONNECTED,
		STATE_P2P_DISCONNECTED
	};
public:
    static const int UDT_SEND_WINDOW_SIZE_MAX = 170; // * CNatUdt::SEND_PACKET_BUF_SIZE = 92 KB
    static const int UDT_RECV_WINDOW_SIZE_MAX = 171; // * CNatUdt::RECV_PACKET_BUF_SIZE = 256 K

    static const int CONNECT_TIMEOUT_MS = UDP_TIMEOUT;
public:
    CNatUdtClientPeer();
    virtual ~CNatUdtClientPeer();

    void SetConnectTimeout(unsigned long timeoutMs);

    int Init(const NAT_CLIENT_DIRECT_CONFIG* config);

    void SetConnectNotifier( CNatSocketConnectNotifier* notifier);
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
private:
    void NotifyOnConnect(NatSocket sock,int iErrorCode);
	
    void InternalClose();

    void ChangeDisConnected();

	int AddToClientManager();

	static PUB_THREAD_RESULT PUB_THREAD_CALL WorkThreadFunc(void *pParam);

	int RunWork();

	NatRunResult Run();
private:
    unsigned long m_connectTimeoutMs;
    unsigned long m_connectStartTimeMs;
	CPUB_Lock m_notifierLock;
    CNatSocketConnectNotifier *m_connectNotifier;
    CNatUdtTransport *m_pUdtTransport;
	CNatUdt *m_udt;
    
	P2PConnectState      m_p2pConnectState;
	bool m_bClientAdded;  //是否被加入ClientManager;
	bool      m_workThreadRuning;  
	PUB_thread_t m_workThreadID;
};

#endif//_NAT_UDT_CLIENT_PEER_H_