#include "TVTHttpHeader.h"

CTVTHttpHeader::CTVTHttpHeader() : m_pHeaderBuf(NULL), m_iHeaderBufLen(0), m_iHeaderLen(0), m_iOperatePos(0), m_contentLength(0), m_bChunked(0), m_bClosed(false)
{
	m_pHeaderBuf = new char [HTTP_HEAD_BUF_LEN];
	if (NULL != m_pHeaderBuf)
	{
		m_pHeaderBuf[HTTP_HEAD_BUF_LEN - 1] = '\0';
		m_iHeaderBufLen = HTTP_HEAD_BUF_LEN;
	}
	else
	{
		printf("%s:%s:%d, no enough memory\n", __FUNCTION__, __FILE__, __LINE__);
	}
	
	m_szCookie[0] = '\0';
}

CTVTHttpHeader::~CTVTHttpHeader()
{
	if (NULL != m_pHeaderBuf)
	{
		delete [] m_pHeaderBuf;
		m_pHeaderBuf = NULL;
	}

	m_iHeaderBufLen = 0;
	m_iHeaderLen = 0;
	m_iOperatePos = 0;

	ClearMSGList();
	ClearCookie();
}

bool CTVTHttpHeader::Initial(const char * pHost, const char * pURI, const char * pCookie, const char * pAuthInfo, bool bPost/*=false*/, HTTP_BODY_INFO * pBodyInfo/*=NULL*/)
{
	Quit();

	//开始行
	if (bPost)
	{
		if (!AddStartLine("POST", pURI, HTTP_VER_1_1))
		{
			return false;
		}
	}
	else
	{
		if (!AddStartLine("GET", pURI, HTTP_VER_1_1))
		{
			return false;
		}
	}

	//头信息
	if (!AddMsgHeaderLine("Host", pHost))
	{
		return false;
	}

// 	if (!AddMsgHeaderLine("Connection", "keep-alive"))
// 	{
// 		return false;
// 	}
// 
// 	if (!AddMsgHeaderLine("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"))
// 	{
// 		return false;
// 	}
// 
// 	if (!AddMsgHeaderLine("Accept-Language", "zh-cn,zh;q=0.8,en-us;q=0.5,en;q=0.3"))
// 	{
// 		return false;
// 	}

	if (NULL != pCookie)
	{
		snprintf(m_szCookie, sizeof(m_szCookie), "%s", pCookie);
	}

	if (0 < strlen(m_szCookie))
	{
		if (!AddMsgHeaderLine("Cookie", m_szCookie))
		{
			return false;
		}
	}

	if (NULL != pAuthInfo)
	{
		if (!AddMsgHeaderLine("Authorization", pAuthInfo))
		{
			return false;
		}
	}

	if (NULL != pBodyInfo)
	{
		if (!AddMsgHeaderLine("Content-Length", pBodyInfo->iTotalBodyLen))
		{
			return false;
		}

		switch (pBodyInfo->iBodyType)
		{
		case HTTP_BODY_TYPE_APP_JSON:
			{
				if (!AddMsgHeaderLine("Content-Type", "application/json"))
				{
					return false;
				}
				break;
			}
		case HTTP_BODY_TYPE_APP_RRLENCODED:
			{
				if (!AddMsgHeaderLine("Content-Type", "application/x-www-form-urlencoded"))
				{
					return false;
				}
				break;
			}
		case HTTP_BODY_TYPE_MUL_FORM_DATA:
			{
				if (!AddMsgHeaderLine("Content-Type", "multipart/form-data; boundary=A300x"))
				{
					return false;
				}
				break;
			}
		case HTTP_BODY_TYPE_IMAGE_JPEG:
			{
				if (!AddMsgHeaderLine("Content-Type", "image/jpeg"))
				{
					return false;
				}
				break;
			}
		default:
			break;
		}
	}

	//添加空行
	if (!AddEndLine())
	{
		return false;
	}

	return true;
}

void CTVTHttpHeader::Quit()
{
	m_pHeaderBuf[0] = '\0';
	m_iHeaderLen = 0;
	m_iOperatePos = 0;
	m_contentLength = 0;
	m_bChunked = false;
	m_bClosed = false;

}

int CTVTHttpHeader::SendHeader(CTcpSession * pTcpSession)
{
	while (m_iOperatePos < m_iHeaderLen)
	{
		int retVal = pTcpSession->Send(m_pHeaderBuf + m_iOperatePos, m_iHeaderLen - m_iOperatePos, MAX_SYNC_TIME);
		if (0 < retVal)
		{
			m_iOperatePos += retVal;
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

	return m_iHeaderLen;
}

int CTVTHttpHeader::RecvHeader(CTcpSession * pTcpSession)
{
	ClearMSGList();

	m_pHeaderBuf[0] = '\0';
	m_iHeaderLen = 0;
	m_iOperatePos = 0;

	//处理第一行
	int iLineLen = 0;
	const char * pLine = GetOneLine(pTcpSession, iLineLen, 1000);
	if (NULL == pLine)
	{
		return -1;
	}
	else
	{
		char szVersion[128];
		char szErrNum[128];
		char szErrDes[128];

		if (3 != sscanf(pLine, "%s %s %[0-9,a-z,A-Z, ]s", szVersion, szErrNum, szErrDes))
		{
			assert(false);
		}
		else
		{
			//printf("=================\r\n");
		}
	}

	do 
	{
		if (NULL == (pLine = GetOneLine(pTcpSession, iLineLen, 1000)))
		{
			return -1;
		}

		if (0 == strncasecmp(pLine, "\r\n", strlen("\r\n")))
		{
			//头接收结束
			return m_iOperatePos;
		}

		MSG_HEADER msgHeader;
		char *pFieldName	= new char[iLineLen];
		sscanf(pLine, "%[^:]s", pFieldName);
		msgHeader.pName = pFieldName;


		char *pFieldValue	= new char [iLineLen];
		if (' ' == pLine[strlen(pFieldName)+1])
		{
			sscanf(pLine + strlen(pFieldName) + 2 , "%[^\r]s", pFieldValue);
		}
		else
		{
			sscanf(pLine + strlen(pFieldName) + 1, "%[^\r]s", pFieldValue);
		}
		msgHeader.pValue = pFieldValue;


		if (0 == strncasecmp(msgHeader.pName, "Set-Cookie", 10))
		{
			SetCookie(msgHeader.pValue);
		}

		if (0 == strncasecmp(msgHeader.pName, "CONTENT-LENGTH", 14))
		{
			m_contentLength = atoi(msgHeader.pValue);
		}
		
		if ((0 == strncasecmp(msgHeader.pName, "TRANSFER-ENCODING", 17)) && 
			(0 == strncasecmp(msgHeader.pValue, "CHUNKED", 7)))
		{
			m_bChunked = true;
		}

		if ((0 == strncasecmp(msgHeader.pName, "CONNECTION", 10)) && 
			(0 == strncasecmp(msgHeader.pValue, "CLOSE", 5)))
		{
			m_bClosed = true;
		}

		m_msgList.push_back(msgHeader);

	} while (1);

	return -1;
}

const char * CTVTHttpHeader::GetHeaderValue(const char *pHeaderName)
{
	for (MSG_HEADER_LIST_ITER iter = m_msgList.begin(); iter != m_msgList.end(); iter++)
	{
		if (0 == strncasecmp(iter->pName, pHeaderName, strlen(pHeaderName)))
		{
			return iter->pValue;
		}
	}

	return NULL;
}

const char * CTVTHttpHeader::ContentData(char * pBuf, int &iBufLen)const
{
	if (NULL == pBuf)
	{
		iBufLen = 0;
		return NULL;
	}

	int iBodyLen = 0;

	if (m_iHeaderLen <= m_iOperatePos)
	{
		iBufLen = 0;
		return NULL;
	}
	else
	{
		iBodyLen = m_iHeaderLen - m_iOperatePos;
	}

	if (iBufLen < iBodyLen + 1)
	{
		iBufLen = 0;
		assert(false);
		printf("%s:%s:%d, No enough buffer\n", __FUNCTION__, __FILE__, __LINE__);
		return NULL;
	}
	else
	{
		iBufLen = iBodyLen;
	}

	memcpy(pBuf, m_pHeaderBuf + m_iOperatePos, iBodyLen);
	pBuf[iBodyLen] = '\0';

	return pBuf;
}

const char * CTVTHttpHeader::ContentData(int &iLength)const
{
	if (m_iHeaderLen <= m_iOperatePos)
	{
		return NULL;
	}
	else
	{
		iLength = m_iHeaderLen - m_iOperatePos;
	}

	return m_pHeaderBuf + m_iOperatePos;
}


const char * CTVTHttpHeader::GetOneLine(CTcpSession * pTcpSession, int &len, unsigned long rTimeOut)
{
	const char * pPos = NULL;

	while ((NULL == (pPos = strstr(m_pHeaderBuf + m_iOperatePos, "\r\n"))))
	{
		if (m_iHeaderBufLen == m_iHeaderLen + 1)
		{
			//接收空间不够
			printf("%s:%s:%d, Not enough buffer\n", __FUNCTION__, __FILE__, __LINE__);
			return NULL;
		}

		int retVal = pTcpSession->Recv(m_pHeaderBuf + m_iHeaderLen, m_iHeaderBufLen - m_iHeaderLen - 1, MAX_SYNC_TIME);
		if (0 < retVal)
		{
			m_iHeaderLen += retVal;
			m_pHeaderBuf[m_iHeaderLen] = '\0';
		}
		else if (0 > retVal)
		{
			//接收错误
			return NULL;
		}
		else
		{
			//等待超时
			return NULL;
		}
	}

	const char * pPosRet = m_pHeaderBuf + m_iOperatePos;
	int lastOperPos = m_iOperatePos;
	m_iOperatePos = pPos - m_pHeaderBuf + 2;
	len = m_iOperatePos - lastOperPos;

	return pPosRet;
}

bool CTVTHttpHeader::AddStartLine(const char *pMethod, const char *pRequestURI, const char *pHTTPVersion)
{
	if ((NULL == pMethod) || (0 == strlen(pMethod)))
	{
		return false;
	}

	if ((NULL == pRequestURI) || (0 == strlen(pRequestURI)))
	{
		return false;
	}

	if ((NULL == pHTTPVersion) || (0 == strlen(pHTTPVersion)))
	{
		return false;
	}

	if (m_iHeaderBufLen <= strlen(pMethod) + strlen(pRequestURI) + strlen(pHTTPVersion) + 5)
	{
		return false;
	}

	strcat(m_pHeaderBuf, pMethod);
	strcat(m_pHeaderBuf, " ");
	strcat(m_pHeaderBuf, pRequestURI);
	strcat(m_pHeaderBuf, " ");
	strcat(m_pHeaderBuf, pHTTPVersion);
	strcat(m_pHeaderBuf, "\r\n");

	m_iHeaderLen = strlen(m_pHeaderBuf);

	return true;
}

bool CTVTHttpHeader::AddMsgHeaderLine(const char *pHeaderName, const char *pHeaderValue)
{
	if ((NULL == pHeaderName) || (0 == strlen(pHeaderName)))
	{
		return false;
	}

	if ((NULL == pHeaderValue) || (0 == strlen(pHeaderValue)))
	{
		return false;
	}

	if (m_iHeaderBufLen <= m_iHeaderLen + strlen(pHeaderName) + strlen(pHeaderValue) + 5)
	{
		return false;
	}

	strcat(m_pHeaderBuf, pHeaderName);
	strcat(m_pHeaderBuf, ": ");
	strcat(m_pHeaderBuf, pHeaderValue);
	strcat(m_pHeaderBuf, "\r\n");

	m_iHeaderLen = strlen(m_pHeaderBuf);

	return true;
}

bool CTVTHttpHeader::AddMsgHeaderLine(const char *pHeaderName, int value)
{
	if ((NULL == pHeaderName) || (0 == strlen(pHeaderName)))
	{
		assert(false);
		return false;
	}

	char szNumber[64] = {0};
#ifdef WIN32
	if (NULL == itoa(value, szNumber, 10))
	{
		return false;
	}
#else
	snprintf(szNumber, sizeof(szNumber) - 1, "%d", value);
#endif

	return AddMsgHeaderLine(pHeaderName, szNumber);
}

bool CTVTHttpHeader::AddEndLine()
{
	if (m_iHeaderLen + 5 > m_iHeaderBufLen)
	{
		return false;
	}

	strcat(m_pHeaderBuf, "\r\n");
	m_iHeaderLen = strlen(m_pHeaderBuf);

	return true;
}

void CTVTHttpHeader::SetCookie(const char * pNewCookie)
{
	int len = strlen(pNewCookie) + 10;
	char * pCookie = new char [len];
	if (NULL != pCookie)
	{
		pCookie[0] = '\0';

		//如果新来的cookie已经存在，就替换， 否则添加到cookie中
		char szCookName[128] = {0};
		if ((1 == sscanf(pNewCookie, "%[^;]s", pCookie)) &&
			(1 == sscanf(pCookie, "%[^=]s", szCookName)))
		{
			for (std::list<char *>::iterator iter=m_cookieList.begin(); iter != m_cookieList.end(); iter++)
			{
				if (0 == strncmp(szCookName, *iter, strlen(szCookName)))
				{
					delete *iter;
					m_cookieList.erase(iter);
					break;
				}
			}

			m_cookieList.push_back(pCookie);
			m_szCookie[0] = '\0';

			//列表肯定不为空
			std::list<char *>::iterator iter=m_cookieList.begin();
			while(1)
			{
				strcat(m_szCookie, *iter);
				iter++;
				if (iter != m_cookieList.end())
				{
					strcat(m_szCookie, "; ");
				}
				else
				{
					break;
				}
			}
		}
		else
		{
			printf("%s:%s:%d, error==============\n", __FUNCTION__, __FILE__, __LINE__);
		}
	}
	else
	{
		printf("%s:%s:%d, no memory\n", __FUNCTION__, __FILE__, __LINE__);
	}
}

void CTVTHttpHeader::ClearMSGList()
{
	for (MSG_HEADER_LIST_ITER iter=m_msgList.begin(); iter != m_msgList.end(); iter++)
	{
		delete [] iter->pName;
		iter->pName = NULL;

		delete [] iter->pValue;
		iter->pValue = NULL;
	}

	m_msgList.clear();
}


void CTVTHttpHeader::ClearCookie()
{
	m_szCookie[0] = '\0';

	for (std::list<char *>::iterator iter=m_cookieList.begin(); iter != m_cookieList.end(); iter++)
	{
		delete *iter;
	}

	m_cookieList.clear();
}
