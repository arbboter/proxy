#include "TVTHttpBody.h"
CTVTHttpBody::CTVTHttpBody() : m_pBodyBuf(NULL), m_iBodyBufLen(0), m_iBodyLen(0), m_iOperatePos(0), m_iTotalBodyLen(0), m_iBodyType(0), m_bNeedParseLength(true)
{
	m_pBodyBuf = new char [HTTP_BODY_BUF_LEN];
	if (NULL != m_pBodyBuf)
	{
		m_iBodyBufLen = HTTP_BODY_BUF_LEN;
		m_pBodyBuf[0] = '\0';
	}
	else
	{
		printf("%s:%s:%d, no enough memory\n", __FUNCTION__, __FILE__, __LINE__);
	}
}

CTVTHttpBody::~CTVTHttpBody()
{
	if (NULL != m_pBodyBuf)
	{
		delete [] m_pBodyBuf;
		m_pBodyBuf = NULL;
	}
}

bool CTVTHttpBody::Initial(const CTVTHttpHeader * pBodyHeader)
{
	Quit();

	m_bChunked = pBodyHeader->IsChunkedContentData();
	m_iBodyType = pBodyHeader->ContentType();
	m_iBodyLen = pBodyHeader->ContentLength();

	if (m_iBodyLen > m_iBodyBufLen)
	{
		return false;
	}

	if (m_bChunked)
	{
		int iLength = 0;
		const char * pBody = pBodyHeader->ContentData(iLength);
		if (0 < iLength)
		{
			if (0 > ParseChunckedDataEx(pBody, iLength))
			{
				return false;
			}
		}
	}
	else
	{
		m_iOperatePos = m_iBodyBufLen;
		pBodyHeader->ContentData(m_pBodyBuf, m_iOperatePos);
		if (m_iOperatePos == m_iBodyLen)
		{
			m_bRecvOver = true;
		}
	}

	return true;
}

void CTVTHttpBody::Quit()
{
	m_iBodyLen = 0;
	m_iOperatePos = 0;
	m_iTotalBodyLen = 0;
	m_iBodyType = 0;

	m_bChunked = false;
	m_iCurChunkedLen = 0;
	m_iCurChunkedRecv= 0;
	m_bNeedParseLength = true;
	m_bRecvOver = false;
}

int CTVTHttpBody::SendBody(CTcpSession * pTcpSession)
{
	return 0;
}

int CTVTHttpBody::RecvBody(CTcpSession * pTcpSession)
{
	if (m_bRecvOver)
	{
		return 0;
	}

	if (m_bChunked)
	{
		char buf[1024 * 128] = {0};
		while (1)
		{
			int recvLen = pTcpSession->Recv(buf, sizeof(buf) - 1, MAX_SYNC_TIME);
			if (0 < recvLen)
			{
				buf[recvLen] = '\0';
				if (0 > ParseChunckedDataEx(buf, recvLen))
				{
					return -1;
				}
			}
			else if (0 == recvLen)
			{
				return -1;
			}
			else
			{
				return -1;
			}

			if (m_bRecvOver)
			{
				return 0;
			}
		}
	}
	else
	{
		while (!m_bRecvOver)
		{
			int recvLen = pTcpSession->Recv(m_pBodyBuf + m_iOperatePos, m_iBodyBufLen - m_iOperatePos - 1, MAX_SYNC_TIME);
			if (0 < recvLen)
			{
				m_iOperatePos += recvLen;
				m_pBodyBuf[m_iOperatePos] = '\0';
				if (m_iOperatePos == m_iBodyLen)
				{
					m_bRecvOver = true;

					return 0;
				}
			}
			else if (0 == recvLen)
			{
				return -1;
			}
			else
			{
				return recvLen;
			}
		}
	}

    return 0;
}

int CTVTHttpBody::ParseChunckedData(const char * pBody, int iLength)
{
	if ((NULL == pBody) || ( 0 >= iLength))
	{
		return -1;
	}

	int pos = ParseChunckedDataLen(pBody, iLength);
	if (0 > pos)
	{
		return pos;
	}

	int dataLen = iLength - pos;
	if (dataLen > m_iCurChunkedLen)
	{
		dataLen = m_iCurChunkedLen;
	}

	//拷贝剩余数据到缓冲区中
	if (m_iBodyBufLen - m_iOperatePos > dataLen)
	{
		if (0 < dataLen)
		{
			memcpy(m_pBodyBuf + m_iOperatePos, pBody + pos, dataLen);
			m_iOperatePos += dataLen;
			m_iBodyLen = m_iOperatePos;
			m_pBodyBuf[m_iOperatePos] = '\0';

			pos += dataLen;
			m_iCurChunkedLen -= dataLen;
		}

		if (0 >= m_iCurChunkedLen)
		{
			m_bNeedParseLength = true;
			m_iCurChunkedLen = 0;
		}

		return pos;
	}
	else
	{
		return -1;
	}
}

int CTVTHttpBody::ParseChunckedDataEx(const char * pBody, int iLength)
{
	int pos = 0;
	while(pos < iLength)
	{
		int retVal = ParseChunckedData(pBody + pos, iLength - pos);
		if (0 >= retVal)
		{
			return retVal;
		}

		pos += retVal;
	}

	return pos;
}

int CTVTHttpBody::ParseChunckedDataLen(const char * pBody, int iLength)
{
	static bool bParseLenOver = true;

	int pos = 0;

	if (m_bNeedParseLength)
	{
		char ch = * (pBody + pos);

		if ((0 == m_iCurChunkedLen) && bParseLenOver)
		{
			//找到数据长度开始的地方
			while ((ch == '\n') || (ch == '\r'))
			{
				pos++;
				if (pos == iLength)
				{
					return pos;
				}

				ch = * (pBody + pos);
			}

			if ((ch < '0') || (('9' < ch) && (ch < 'A')) || (('F' < ch) && (ch < 'a')) || ('f' < ch))
			{
				assert(false);
				return -1;
			}

			bParseLenOver = false;
		}

		if (!bParseLenOver)
		{
			while ((('0' <= ch) && ('9' >= ch)) || (('a' <= ch) && ('f' >= ch)) || (('A' <= ch) && ('F' >= ch)))
			{
				m_iCurChunkedLen = m_iCurChunkedLen * 16;

				if (('0' <= ch) && ('9' >= ch))
				{
					m_iCurChunkedLen += (ch - '0');
				}
				else if (('a' <= ch) && ('f' >= ch))
				{
					m_iCurChunkedLen += 10;
					m_iCurChunkedLen += (ch - 'a');
				}
				else
				{
					m_iCurChunkedLen += 10;
					m_iCurChunkedLen += (ch - 'A');
				}

				pos++;
				if (pos == iLength)
				{
					//数据长度解析未完毕,需要下次来数据继续解析
					return pos;
				}

				ch = * (pBody + pos);
			}

			if ('\r' == ch || '\n' == ch)
			{
				if ('\n' == ch)
				{
					pos++;
					bParseLenOver = true;
				}
				else 
				{
					pos++;
					if (pos == iLength)
					{
						return pos;
					}

					ch = * (pBody + pos);
					if ('\n' == ch)
					{
						bParseLenOver = true;
						pos++;
					}
					else
					{
						return -1;
					}
				}
			}
			else
			{
				return -1;
			}

			if (pos == iLength)
			{
				return pos;
			}
		}

		if (bParseLenOver)
		{
			if (0 == m_iCurChunkedLen)
			{
				//数据接收结束
				m_bRecvOver = true;
				if (4 > iLength - pos)
				{
					int k = 0;
					k++;
				}
			}

			//去掉数据长度后空字符
			ch = * (pBody + pos);
			while ((ch == '\n') || (ch == '\r'))
			{
				pos++;
				if (pos == iLength)
				{
					return pos;
				}

				ch = * (pBody + pos);
			}
		}

		m_bNeedParseLength = false;
	}
	
	return pos;
}



