// NatTraversalClientPeer.h: interface for the CNatTraversalClientPeer class.
//  备注：一条命令需处理完成后，才能处理下一条命令
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_TRAVERSAL_CLIENT_CONNECTOR_H_
#define _NAT_TRAVERSAL_CLIENT_CONNECTOR_H_

#include "NatSocketBase.h"
#include "NatRunner.h"
#include "NatUdtTransport.h"
#include "NatUdt.h"

class CNatClientUdtSocket: public CNatSocketBase, CNatUdtTransportNotifier,CUdtNotifier
{
public:    

	CNatClientUdtSocket(CNatUdt *pUdt,CNatUdtTransport *pUdtTransport);
	virtual ~CNatClientUdtSocket();

public:
    ////////////////////////////////////////////////////////////////////////////////
    // Implements CNatSocketBase public interface
    virtual int Close();

    virtual NatRunResult Run();

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

    virtual unsigned long GetRemoteIp();

    virtual unsigned short GetRemotePort();
public:
    ////////////////////////////////////////////////////////////////////////////////
    // Implements CNatUdtTransportNotifier public interface

    virtual void OnRecvDatagram(CNatUdtTransport *transport, const NAT_TRANSPORT_RECV_DATAGRAM* packet);
public:
	////////////////////////////////////////////////////////////////////////////////
	// Implements CUdtNotifier public interface

	virtual void OnConnect(CNatUdt *pUDT, int iErrorCode);

	virtual int OnRecv(CNatUdt *pUDT, const void* pBuf, int iLen);

	virtual int OnSendPacket(CNatUdt *pUDT, const void* pBuf, int iLen);
private:
	void InternalClose();
private:

    CNatUdtTransport *m_pUdtTransport;
	CNatUdt		     *m_udt;
};

#endif //_NAT_TRAVERSAL_CLIENT_CONNECTOR_H_