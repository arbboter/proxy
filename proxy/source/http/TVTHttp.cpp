#include "TVTHttp.h"

CTVTHttp::CTVTHttp()
{
	m_sock = SWL_INVALID_SOCKET;

	m_pTcpSession = NULL;
	m_httpType = SESSION_NONE;
	m_szHost[0] = '\0';
	m_serPort = 0;
}

CTVTHttp::~CTVTHttp()
{
	if (NULL != m_pTcpSession)
	{
		m_pTcpSession->Close();
		delete m_pTcpSession;
	}
}

bool CTVTHttp::Get(const char * pURL, const char * pCookie/*=NULL*/, const char * pAuthInfo/*=NULL*/)
{
	char szHost[128] = {0};
	char szUri[1024] = {0};
	unsigned short port = 0;

	if (!GetURLInfo(pURL, szHost, sizeof(szHost), port, szUri, sizeof(szUri)))
	{
		return false;
	}

	if (!m_httpHeader.Initial(szHost, szUri, pCookie, pAuthInfo))
	{
		return false;
	}

	if (!ConnectHost(szHost, port))
	{
		return false;
	}

	//发送头
	if (0 > m_httpHeader.SendHeader(m_pTcpSession))
	{
		CloseHost();
		return false;
	}

	//接收头
	if (0 > m_httpHeader.RecvHeader(m_pTcpSession))
	{
		CloseHost();
		return false;
	}

	//接收body
	if (0 > m_httpBody.Initial(&m_httpHeader))
	{
		CloseHost();
		return false;
	}

	if (0 > m_httpBody.RecvBody(m_pTcpSession))
	{
		CloseHost();
		return false;
	}

	if (m_httpHeader.Closed())
	{
		CloseHost();
	}

	return true;
}

bool CTVTHttp::Post(const char * pURL, HTTP_BODY_INFO * pBodyInfo, const char * pCookie/*=NULL*/, const char * pAuthInfo/*=NULL*/)
{
	char szHost[128] = {0};
	char szUri[1024] = {0};
	unsigned short port = 0;

	if (!GetURLInfo(pURL, szHost, sizeof(szHost), port, szUri, sizeof(szUri)))
	{
		return false;
	}

	if (!m_httpHeader.Initial(szHost, szUri, pCookie, pAuthInfo, true, pBodyInfo))
	{
		return false;
	}

	if (!ConnectHost(szHost, port))
	{
		return false;
	}

	//发送头
	if (!m_httpHeader.SendHeader(m_pTcpSession))
	{
		CloseHost();
		return false;
	}

	//发送body
	if (0 > SendHttpBody(pBodyInfo, 1000))
	{
		CloseHost();
		return false;
	}

	//接收头
	if (0 > m_httpHeader.RecvHeader(m_pTcpSession))
	{
		CloseHost();
		return false;
	}

	//接收body
	if (!m_httpBody.Initial(&m_httpHeader))
	{
		CloseHost();
		return false;
	}
	
	if (0 > m_httpBody.RecvBody(m_pTcpSession))
	{
		CloseHost();
		return false;
	}

	if (m_httpHeader.Closed())
	{
		CloseHost();
	}

	return true;
}

bool CTVTHttp::PostHeader(const char * pURL, HTTP_BODY_INFO * pBodyInfo, const char * pCookie, const char * pAuthInfo)
{
	char szHost[128] = {0};
	char szUri[1024] = {0};
	unsigned short port = 0;

	if (!GetURLInfo(pURL, szHost, sizeof(szHost), port, szUri, sizeof(szUri)))
	{
		return false;
	}

	if (!m_httpHeader.Initial(szHost, szUri, pCookie, pAuthInfo, true, pBodyInfo))
	{
		return false;
	}

	if (!ConnectHost(szHost, port))
	{
		return false;
	}

	//发送头
	if (0 > m_httpHeader.SendHeader(m_pTcpSession))
	{
		CloseHost();
		return false;
	}

	//发送body
	if (0 > SendHttpBody(pBodyInfo, 1000))
	{
		CloseHost();
		return false;
	}

	return true;
}

int CTVTHttp::PostBodyData(const char * pBD, unsigned long bdLen)
{
	return m_pTcpSession->Send(pBD, bdLen);
}

int CTVTHttp::PostOver(unsigned long rTimeOut)
{
	//接收头
	if (0 > m_httpHeader.RecvHeader(m_pTcpSession))
	{
		CloseHost();
		return -1;
	}

	//接收body
	if (!m_httpBody.Initial(&m_httpHeader))
	{
		CloseHost();
		return -1;
	}

	if (0 > m_httpBody.RecvBody(m_pTcpSession))
	{
		CloseHost();
		return -1;
	}
	
	//if (m_httpHeader.Closed())
	{
		CloseHost();
	}

	return 0;
}

int CTVTHttp::SendHttpBody(HTTP_BODY_INFO * pBodyInfo, unsigned long sTimeOut)
{
	if (NULL == pBodyInfo)
	{
		return 0;
	}

	pBodyInfo->iOperatePos = 0;

	while (pBodyInfo->iOperatePos < pBodyInfo->iBodyLen)
	{
		int retVal = m_pTcpSession->Send(pBodyInfo->pBodyBuf + pBodyInfo->iOperatePos, pBodyInfo->iBodyLen - pBodyInfo->iOperatePos, MAX_SYNC_TIME);
		if (0 < retVal)
		{
			pBodyInfo->iOperatePos += retVal;
		}
		else if (0 == retVal)
		{
			return -1;
		}
		else
		{
			return retVal;
		}
	}

	return pBodyInfo->iBodyLen;
}

bool CTVTHttp::GetURLInfo(const char * pURL, char * pHost, int hostBufLen, unsigned short &port, char * pURI, int uriBufLen)
{
	if (NULL == pURL)
	{
		return false;
	}

	SESSION_TYPE httpType = SESSION_COMMON ;

	//获取HTTP的类型
	if (0 == strncasecmp(pURL, "HTTPS", 5))
	{
		httpType = SESSION_SSL;
		pURL += 5;
	}
	else if (0 == strncasecmp(pURL, "HTTP", 4))
	{
		httpType = SESSION_COMMON;
		pURL += 4;
	}
	else
	{
		httpType = SESSION_COMMON;
	}

	if (0 == strncasecmp(pURL, "://", 3))
	{
		pURL += 3;
	}

	//获取地址
	int urlLen = strlen(pURL);
	int pos = 0;
	char ch = 0;
	for (pos=0; pos<urlLen; pos++)
	{
		ch = pURL[pos];
		if ((':' == ch) || ('/' == ch))
		{
			break;
		}
	}

	if (hostBufLen <= pos)
	{
		httpType = SESSION_NONE;
		return false;
	}
	else
	{
		strncpy(pHost, pURL, pos);
	}

	//获取端口
	pURL += pos;

	if (':' == pURL[0])
	{
		//指定了端口
		pURL += 1;
		if (uriBufLen <= strlen(pURL))
		{
			pHost[0] = '\0';
			httpType = SESSION_NONE;
			return false;
		}
        int tmp;
		sscanf(pURL, "%d%s", &tmp, pURI);
        port = tmp;
	}
	else if ('/' == pURL[0])
	{
		//没有指定端口
		if (SESSION_COMMON == httpType)
		{
			port = 80;
		}
		else
		{
			port = 443;
		}

		if (uriBufLen > strlen(pURL))
		{
			strcpy(pURI, pURL);
		}
		else
		{
			pHost[0] = '\0';
			httpType = SESSION_NONE;
			return false;
		}
	}
	else
	{
		if (pos == urlLen)
		{
			//没有指定端口
			if (SESSION_COMMON == httpType)
			{
				port = 80;
			}
			else
			{
				port = 443;
			}

			//没有URI
			pURI[0] = '/';
		}
		else
		{
			pHost[0] = '\0';
			httpType = SESSION_NONE;
			assert(false);
			return false;
		}
	}

	m_httpType = httpType;
	if (NULL != m_pTcpSession && m_httpType != m_pTcpSession->SessionType())
	{
		m_pTcpSession->Close();
		delete m_pTcpSession;
		m_pTcpSession = NULL;
	}
	
	return true;
}

bool CTVTHttp::ConnectHost(const char * pHost, unsigned short port)
{
	if (NULL != m_pTcpSession)
	{
		if (m_pTcpSession->IsConnected())
		{
			//如果已经连接上，如果变化了IP和端口，则从新连接
			if ((0 != strcmp(pHost, m_szHost)) || (port != m_serPort))
			{
				//先关闭端口
				m_pTcpSession->Close();
			}
		}
	}
	else
	{
		if (SESSION_SSL == m_httpType)
		{
#if HTTP_SSL
			m_pTcpSession = new CTcpSSession;
#else
			// 不支持
#endif
		}
		else
		{
			m_pTcpSession = new CTcpSession;
		}
	}

	if (!m_pTcpSession->IsConnected())
	{
		if (SWL_INVALID_SOCKET == (m_sock = m_pTcpSession->Connect(pHost, port, 0)))
		{
			return false;
		}

		snprintf(m_szHost, sizeof(m_szHost), pHost);
		m_serPort = port;
	}

	return true;
}

bool CTVTHttp::CloseHost()
{
	if (NULL != m_pTcpSession)
	{
		m_pTcpSession->Close();

		m_szHost[0] = '\0';
		m_serPort = 0;
	}

	return true;
}



