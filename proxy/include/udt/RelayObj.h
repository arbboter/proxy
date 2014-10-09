#ifndef __RELAY_ADAPTER__
#define __RELAY_ADAPTER__

#include "NatSocketBase.h"
#include <map>

const unsigned char RELAY_CMD_CONNETCT		=	0x00;//client device 向RelayServer发送连接配对请求
const unsigned char RELAY_CMD_CONNETCT_ACK	=	0x01;//RelayServer向client device分别发送连接配对应答
const unsigned char RELAY_CMD_DATA			=	0x03;//转发的数据

const unsigned long RELAY_RECV_BUF_LEN		=	64*1024;

typedef enum
{
	RELAY_ROLE_DEVICE,
	RELAY_ROLE_CLIENT,
}RELAY_ROLE;

class CRelayNotifier;

#pragma pack (1) /*指定按1字节对齐*/ 
typedef struct _relay_connection_info_ 
{
	unsigned long	serverIp;
	unsigned long	connectionID;
	unsigned long	deviceNo;
	CRelayNotifier	*pNofitier;
	unsigned short	serverPort;
	unsigned char	role;
}RELAY_CONNECTION_INFO;

typedef struct _relay_cmd_connect_data_
{
	unsigned long connectionID;
	unsigned long deviceNo;
	unsigned char role;
}RELAY_CMD_CONNECT_DATA;

typedef struct _relay_cmd_header_
{
	unsigned short	version;
	unsigned char	option;
	unsigned char	cmdID;
	unsigned long	cmdDataSize;
}RELAY_CMD_HEADER;


typedef struct _relay_trans_ctrl_
{
	unsigned long		sendRemainSize;
	unsigned long		recvRemainSize;
}RELAY_TRANS_CTRL;

typedef struct _relay_recv_data_buffer_
{
	long			bufferSize;	//缓冲区容量
	long			dataSize;	//缓冲区中数据大小
	char			*pData;		//接收数据的缓冲区
}RELAY_RECV_DATA_BUFFER;

typedef struct _net_link_resource_
{
	SWL_socket_t		sock;
	RELAY_RECV_DATA_BUFFER	recvBuffer;			//要接收的数据
}NET_LINK_RESOURCE;

#pragma pack ()/*取消字节对齐，按默认*/ 


class CRelayNotifier
{
public:
	virtual void RelayOnConnect(CNatSocketBase *pSockBase, int iErrorCode) = 0;
};

class CRelayObj : public CNatSocketBase
{
public:
    static const unsigned long CONNECT_TIMEOUT = 10000; // ms
public:
	typedef struct _Relay_raw_data_
	{
		unsigned char		cmdID;
		unsigned long		cmdDataSize;
		char*				pData;
	}RELAY_RAW_DATA;

public:
	explicit CRelayObj(unsigned long connectionID, unsigned long serverIp, unsigned short serverPort, unsigned char role);
	virtual ~CRelayObj();

	bool Initial();
	void SetRelayNotifier(CRelayNotifier *pNotifier);
	unsigned long GetcnnectionID() {return m_RelayInfo.connectionID;}
	//CNatSocketBase::NotifyOnRecv
public:
	NatRunResult Run();

	virtual int Send(const void *pBuf, int iLen);

    virtual int Recv(void *pBuf, int iLen);

    virtual int GetSendAvailable();

    virtual int GetRecvAvailable();

    virtual int SetAutoRecv(bool bAutoRecv);

    virtual int Close();

    virtual unsigned long GetRemoteIp();

    virtual unsigned short GetRemotePort();

protected:
	virtual void ChangeState(SocketState state);

private:
	CRelayObj(const CRelayObj&);
	CRelayObj& operator=(const CRelayObj&) {return *this;}

	int SendBuff(const void *pBuf, int iLen);
	int RecvBuff(void *pBuf, int iLen);
	int HandleRecv(RELAY_RECV_DATA_BUFFER *pDataBuffer);
	int ConnectServer();

private:
    static const int RECV_DATA_BUF_SIZE = 100 * 1024;
    static const int RECV_CTRL_DATA_BUF_SIZE = 1024;
private:
    bool                    m_recvHeader;
    RELAY_CMD_HEADER        m_header;
    char                    m_data[RECV_DATA_BUF_SIZE];
    int                     m_recvDataPos;
    int                     m_dataSize;
    char                    m_ctrlData[RECV_CTRL_DATA_BUF_SIZE];

	RELAY_CONNECTION_INFO	m_RelayInfo;
	RELAY_TRANS_CTRL		m_Ctrl;
	RELAY_CMD_HEADER		m_CmdHeader;
	NET_LINK_RESOURCE		m_LinkResource;

	CPUB_Lock				m_BuffLock;
	bool					m_bAutoRecv;
	int						m_maxSockFd;
	fd_set					m_fdSet;
	unsigned long			m_FirstConnectTime;//连接服务器的超时时间
};



#endif