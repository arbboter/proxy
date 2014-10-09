/////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __TCT_SESSION_H__
#define __TCT_SESSION_H__

#include <stdio.h>
#include <assert.h>

#include "SWL_Public.h"
#include "TVT_PubCom.h"

#if HTTP_SSL
#include "SSLClient.h"
#endif

typedef enum __session_type__
{
	SESSION_NONE	= 0,
	SESSION_COMMON = 1,		//普通http
	SESSION_SSL	= 2,		//HTTPS		
}SESSION_TYPE;

const unsigned long MAX_SEND_BUFFER_LEN = 524288;
const unsigned long MAX_RECV_BUFFER_LEN = 16384;

class CTcpSession
{
public:
	CTcpSession(SESSION_TYPE sessionType=SESSION_COMMON);
	virtual ~CTcpSession();

	SWL_socket_t Connect(const char * pServer, unsigned short sport, unsigned short lport=0);
	void Close();
	int Send(const char * pData, unsigned long dataLen, unsigned long ulSynTime=0);
	int Recv(char * pBuff, unsigned long buffLen, unsigned long ulSynTime=0);
	SESSION_TYPE SessionType(){return m_sessonType;}
	bool IsConnected(){return (SWL_INVALID_SOCKET != m_sock) ? true : false;}
	int GetSocket(){return m_sock;}
private:
	virtual int Connect(SWL_socket_t sock){return 0;}

	virtual int Close(SWL_socket_t sock){return 0;}

	virtual int SendData(const char * pData, unsigned long dataLen){ return SWL_Send(m_sock, pData, dataLen, 0);}
	
	virtual int RecvData(char * pBuff, unsigned long bufLen){return SWL_Recv(m_sock, pBuff, bufLen, 0);}

	int CanRecvedData(unsigned long timeOut);
	int CanSendData(unsigned long timeOut);
	void SetSockBlock(bool bBlock = true)
	{
#if defined WIN32
		unsigned long dwVal = 1; //非0为非阻塞模式
		if (bBlock)
		{
			dwVal = 0;			//0为阻塞模式
		}

		ioctlsocket(m_sock, FIONBIO, &dwVal);
#else
		int iSave = fcntl(m_sock, F_GETFL);
		if (bBlock)
		{
			fcntl(m_sock, F_SETFL, iSave & ~O_NONBLOCK);
		}
		else
		{
			fcntl(m_sock, F_SETFL, iSave | O_NONBLOCK);
		}
#endif
	}

	void SetSockMaxBandwidth()
	{
		int optVal = 0;
		socklen_t sockLen = sizeof(int);

#if defined WIN32
		getsockopt(m_sock, IPPROTO_IP, IP_TOS, (char *)&optVal, &sockLen);
		printf("%s:%s:%d, sock tos is 0x%08x\n", __FUNCTION__, __FILE__, __LINE__, optVal);
#endif
		optVal = 8;
		//setsockopt(m_sock, IPPROTO_IP, IP_TOS, (const char  *)&optVal, sizeof(optVal));
#if defined WIN32
		optVal = 0;
		sockLen = sizeof(int);
		getsockopt(m_sock, IPPROTO_IP, IP_TOS, (char *)&optVal,  &sockLen);
		printf("%s:%s:%d, sock tos is 0x%08x\n", __FUNCTION__, __FILE__, __LINE__, optVal);
#endif
	}

	void SetSockSendBuf(unsigned long ulSendBufLen=MAX_SEND_BUFFER_LEN, unsigned long ulRecvBufLen=MAX_RECV_BUFFER_LEN)
	{
       
		int  sendBufLen = 0;
		int  recvBufLen = 0;
		socklen_t sockLen = sizeof(int);

#if defined WIN32
		getsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (char *)&sendBufLen, &sockLen);
		getsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, (char *)&recvBufLen, &sockLen);
		printf("%s:%s:%d, sock send buf=%d, receive buf=%d\n", __FUNCTION__, __FILE__, __LINE__, sendBufLen, recvBufLen);
#endif

		setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (const char *)&ulSendBufLen, sizeof(int));
		setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, (const char *)&ulRecvBufLen, sizeof(int));

#if defined WIN32
		getsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (char *)&sendBufLen, &sockLen);
		getsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, (char *)&recvBufLen, &sockLen);
		printf("%s:%s:%d, sock send buf=%d, receive buf=%d\n", __FUNCTION__, __FILE__, __LINE__, sendBufLen, recvBufLen);
#endif
	}
protected:

	SWL_socket_t	m_sock;
	SESSION_TYPE	m_sessonType;
private:

};

#if HTTP_SSL
class CTcpSSession : public CTcpSession
{
public:
	CTcpSSession() : CTcpSession(SESSION_SSL){;}
	virtual ~CTcpSSession(){;}
protected:
private:
	inline virtual int Connect(SWL_socket_t sock);
	inline virtual int Close(SWL_socket_t sock);
	inline virtual int SendData(const char * pData, unsigned long dataLen);
	inline virtual int RecvData(char * pBuff, unsigned long bufLen);

	CSSLClient m_SSLClient;
};

inline int CTcpSSession::Connect(SWL_socket_t sock)
{
	if (!m_SSLClient.Initial())
	{
		return -1;
	}

	if (0 != m_SSLClient.SSLConnect(m_sock))
	{
		//m_SSLClient.Quit();

		return -1;
	}

	return 0;
}

inline int CTcpSSession::Close(SWL_socket_t sock)
{
	//m_SSLClient.SSLShutdown();
	m_SSLClient.Quit();

	return 0;
}

inline int CTcpSSession::SendData(const char * pData, unsigned long dataLen)
{
	return m_SSLClient.SSLWrite(pData, dataLen);
}

inline int CTcpSSession::RecvData(char * pBuff, unsigned long bufLen)
{
	return m_SSLClient.SSLRead(pBuff, bufLen);
}

#endif

#endif

/////////////////////////////////////////////////////////////////////////////////////////////////
