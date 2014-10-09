#include "TcpSession.h"


CTcpSession::CTcpSession(SESSION_TYPE sessionType) : m_sock(SWL_INVALID_SOCKET) , m_sessonType(sessionType)
{
}

CTcpSession::~CTcpSession()
{
	Close();
}

SWL_socket_t CTcpSession::Connect(const char * pServer, unsigned short sport, unsigned short lport/*=0*/)
{
	if (SWL_INVALID_SOCKET != m_sock)
	{
		return m_sock;
	}

	if ((NULL == pServer) || (0 == strlen(pServer)))
	{
		assert(false);
		return SWL_INVALID_SOCKET;
	}

	if (0 == sport)
	{
		assert(false);
		return SWL_INVALID_SOCKET;
	}

	m_sock = SWL_CreateSocket(AF_INET, SOCK_STREAM, 0);
/*
	m_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (SWL_INVALID_SOCKET == m_sock) 
	{
#ifdef __ENVIRONMENT_WIN32__
		PrintDebugInfo("SWL_socket_t sock = socket(iDomain, iType, iProtocol)", WSAGetLastError());
#endif
		return SWL_INVALID_SOCKET;
	}
	int  opt = 1;
	if(0 != setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(int)))
	{
		SWL_PrintError(__FILE__, __LINE__);
#ifdef __ENVIRONMENT_WIN32__
		PrintDebugInfo("setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(int))", WSAGetLastError());
#endif
		return SWL_INVALID_SOCKET;
	}

	SetSockBlock();
*/

	if (SWL_INVALID_SOCKET == m_sock)
	{
		assert(false);
		return SWL_INVALID_SOCKET;
	}

	SetSockBlock();
	SetSockMaxBandwidth();
	SetSockSendBuf();

	if (0 != lport)
	{
		struct sockaddr_in sockAddr;
		memset(&sockAddr, 0, sizeof(struct sockaddr_in));
		sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		sockAddr.sin_family = AF_INET;
		sockAddr.sin_port = htons(lport);
		SWL_Bind(m_sock, (const struct sockaddr *)&sockAddr, sizeof(struct sockaddr_in));
	}

	unsigned long serverInAddr = inet_addr(pServer);
	if (INADDR_NONE == serverInAddr)
	{
		//检查是不是域名
		struct hostent *pHost = GetHostbyname(pServer);
		if (pHost == NULL)
		{
			return SWL_INVALID_SOCKET;	
		}

		struct in_addr sin_addr;
		sin_addr = *(struct in_addr *)pHost->h_addr;
		serverInAddr = sin_addr.s_addr;

		ReleaseHost(pHost);
	}

	struct sockaddr_in	serverSockAddr;
	memset(&serverSockAddr, 0, sizeof(struct sockaddr_in));
	serverSockAddr.sin_port		= htons(sport);
	serverSockAddr.sin_family	= AF_INET;
	serverSockAddr.sin_addr.s_addr = serverInAddr;

	if (0 != SWL_Connect(m_sock, (const struct sockaddr *)&serverSockAddr, sizeof(struct sockaddr_in), 2000))
	{
		SWL_CloseSocket(m_sock);
		m_sock = SWL_INVALID_SOCKET;

		return SWL_INVALID_SOCKET;
	}

	if (0 != Connect(m_sock))
	{
		SWL_CloseSocket(m_sock);
		m_sock = SWL_INVALID_SOCKET;

		return SWL_INVALID_SOCKET;
	}

	return m_sock;
}

void CTcpSession::Close()
{
	if (SWL_INVALID_SOCKET != m_sock)
	{
		Close(m_sock);

		//关闭tcp连接
		SWL_CloseSocket(m_sock);
		m_sock = SWL_INVALID_SOCKET;
	}
}

int CTcpSession::Send(const char * pData, unsigned long dataLen, unsigned long ulSynTime/*=0*/)
{
	int retVal = -1;

	if (0 < ulSynTime)
	{
		int tryCount = 0;
		while (1)
		{
			retVal = CanSendData(ulSynTime);
			if (0 > retVal)
			{
				return retVal;
			}
			else if (0 == retVal)
			{
				tryCount++;
				if (10 < tryCount)
				{
					return -1;
				}
			}
			else
			{
				break;
			}
		}
	}

	while (1)
	{
		retVal = SendData(pData, dataLen);
		if (0 > retVal)
		{
			if (SWL_EWOULDBLOCK())
			{
				printf("%s:%s:%d, try again later\n", __FUNCTION__, __FILE__, __LINE__);
				errno = 0;
				continue;
			}
			else
			{
				printf("%s:%s:%d, close socket\n", __FUNCTION__, __FILE__, __LINE__);
			}
		}
		else if (0 == retVal)
		{
			assert(false);
			printf("%s:%s:%d, running at here\n", __FUNCTION__, __FILE__, __LINE__);
			retVal = -1;
		}

		break;
	}

	return retVal;
}

int CTcpSession::Recv(char * pBuff, unsigned long buffLen, unsigned long ulSynTime/*=0*/)
{
	int ret = 0;

	if (0 < ulSynTime)
	{
		int tryCount = 0;
		while (1)
		{
			ret = CanRecvedData(ulSynTime);
			if (0 > ret)
			{
				return ret;
			}
			else if (0 == ret)
			{
				tryCount++;
				if (10 < tryCount)
				{
					return -1;
				}
			}
			else
			{
				break;
			}
		}
	}

	while (1)
	{
		errno = 0;
		ret =  RecvData(pBuff, buffLen);
		if (0 > ret)
		{
			if (SWL_EWOULDBLOCK())
			{
				continue;
			}
			else
			{
				printf("%s:%s:%d, running at here, close the socket\n", __FUNCTION__, __FILE__, __LINE__);
			}
		}
		else if (0 == ret)
		{
			assert(false);
			printf("%s:%s:%d, running at here\n", __FUNCTION__, __FILE__, __LINE__);
			ret = -1;
		}
		else
		{
		}

		break;
	}

	return ret;
}

int CTcpSession::CanRecvedData(unsigned long timeOut)
{
	if(SWL_INVALID_SOCKET == m_sock)
	{
		assert(false);
		return -1;
	}

	unsigned long time = timeOut;
	
	while (0 < time)
	{
		fd_set rset;
		FD_ZERO(&rset);
		FD_SET(m_sock, &rset);

		struct timeval timeout = {time/1000 , time%1000};
		errno = 0;

		int fdcount = select(m_sock + 1, &rset, NULL, NULL, &timeout);
		if (0 > fdcount)
		{
			if (SWL_EWOULDBLOCK())
			{
				time = (time >> 2);
				continue;
			}
			else
			{
				return -1;
			}
		}
		else if (0 == fdcount)
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}

	assert(false);

	return 0;
}

int CTcpSession::CanSendData(unsigned long timeOut)
{
	if(SWL_INVALID_SOCKET == m_sock)
	{
		assert(false);
		return -1;
	}

	unsigned long time = timeOut;

	while (0 < time)
	{
		fd_set wset;
		FD_ZERO(&wset);
		FD_SET(m_sock, &wset);
		errno = 0;

		struct timeval timeout = {time/1000 , time%1000};
		int fdcount = select(m_sock + 1, NULL, &wset, NULL, &timeout);
		if (0 > fdcount)
		{
			if (SWL_EWOULDBLOCK())
			{
				time = (time >> 2);
				continue;
			}
			else
			{
				return -1;
			}
		}
		else if (0 == fdcount)
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}

	assert(false);

	return 0;
}
