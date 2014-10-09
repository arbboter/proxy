/***********************************************************************
** Copyright (C) Tongwei Video Technology Co.,Ltd. All rights reserved.
** Author       : YSW
** Date         : 2012-03-14
** Name         : DataBufferEx.cpp
** Version      : 1.0
** Description  :
** Modify Record:
1:
***********************************************************************/
#include "DataBufferEx.h"
#ifdef __DEBUG_NEW__
#include "debug_new.h"
#define new DEBUG_NEW
#endif


#ifndef WIN32
CDataBufferEx::BUFFER			CDataBufferEx::s_buffer[MAX_BUFFER_NUM] = {0};
CPUB_Lock						CDataBufferEx::s_lockBuff;

void CDataBufferEx::PushBuffer(unsigned char *pBuf, unsigned int index, int bufLength)
{
	assert (NULL != pBuf);
	assert (index < MAX_BUFFER_NUM);
	assert ((0 < bufLength) && (bufLength < 8*1024*1024));

	s_buffer[index].pThis		= NULL;
	s_buffer[index].pBuffer	= pBuf;
	s_buffer[index].length		= bufLength;
}
#endif

CDataBufferEx::CDataBufferEx(unsigned int bufLength/* = DEFAULT_BUFFER_LENGTH_EX*/):\
		m_pBuffer(NULL), m_bufLength(bufLength), m_pageNum(0)
{
#ifdef WIN32
	assert((0 < m_bufLength) && (m_bufLength <= 8*1024*1024));
	assert(0 == (m_bufLength % DEFAULT_PAGE_LENGTH));

	m_pBuffer	= new unsigned char [m_bufLength];
#else
	s_lockBuff.Lock();

	for(unsigned int i = 0; i < MAX_BUFFER_NUM; i++)
	{
		if((NULL == s_buffer[i].pThis) && (bufLength == s_buffer[i].length))
		{
			s_buffer[i].pThis = this;

			m_bufIndex			= i;
			m_pBuffer			= s_buffer[i].pBuffer;
			m_bufLength			= s_buffer[i].length;

			break;
		}
	}

	assert(NULL != m_pBuffer);

	s_lockBuff.UnLock();
#endif
	assert (NULL != m_pBuffer);

	m_pageNum = (m_bufLength / DEFAULT_PAGE_LENGTH);

	m_freeHeadPage = 0;
	m_freeTailPage = 0;
	m_bufTailPage = m_pageNum;
	m_freePageNum = m_pageNum;

	m_bBusy = false;
}

CDataBufferEx::~CDataBufferEx()
{
#ifdef WIN32
	delete [] m_pBuffer;
	m_pBuffer		= NULL;
	m_bufLength		= 0;
#else
	s_lockBuff.Lock();

	assert (m_bufIndex < MAX_BUFFER_NUM);

	if((NULL != m_pBuffer) && (s_buffer[m_bufIndex].pThis == this))
	{
		s_buffer[m_bufIndex].pThis = NULL;
		m_bufIndex = 0;
	}
	else
	{
		assert(false);
	}

	s_lockBuff.UnLock();

	m_pBuffer		= NULL;
	m_bufLength		= 0;
#endif
}

int CDataBufferEx::GetBuffer(unsigned char **ppBuf, unsigned int & bufLength)
{
	assert (NULL != ppBuf);
	assert((0 < bufLength) && (bufLength <= m_bufLength));
	assert(!m_bBusy);

	if ((NULL == ppBuf) || (0 == bufLength) || (m_bufLength < bufLength) || m_bBusy)
	{
		return 2;
	}

	unsigned int pageNum = (bufLength + DEFAULT_PAGE_LENGTH - 1) / DEFAULT_PAGE_LENGTH;

	while(m_freePageNum >= pageNum)
	{
		if(m_freeHeadPage < m_freeTailPage)
		{
			*ppBuf = m_pBuffer + m_freeHeadPage * DEFAULT_PAGE_LENGTH;

			m_bBusy = true;
			return 0;
		}
		else
		{
			if((m_pageNum - m_freeHeadPage) >= pageNum)
			{
				*ppBuf = m_pBuffer + m_freeHeadPage * DEFAULT_PAGE_LENGTH;

				m_bBusy = true;
				return 0;
			}
			else
			{
				if(m_freeHeadPage == m_freeTailPage)
				{
					m_freeHeadPage = 0;
					m_freeTailPage = 0;
					m_freePageNum = m_pageNum;
					m_bufTailPage = m_pageNum;
				}
				else
				{
					m_freePageNum -= (m_pageNum - m_freeHeadPage);
					m_bufTailPage = m_freeHeadPage;
					m_freeHeadPage = 0;
				}
			}
		}
	}

	return 1;
}

void CDataBufferEx::UsedBuffer(unsigned char *pBuf, unsigned int bufLength)
{
	assert ((m_pBuffer <= pBuf) && (pBuf < (m_pBuffer + m_bufLength)));
	assert ((pBuf + bufLength) <= (m_pBuffer +m_bufLength));
	assert (pBuf == (m_pBuffer + m_freeHeadPage * DEFAULT_PAGE_LENGTH));

	if ((m_pBuffer <= pBuf) && (pBuf < (m_pBuffer + m_bufLength)) && (pBuf == (m_pBuffer + m_freeHeadPage * DEFAULT_PAGE_LENGTH)))
	{
		unsigned int usedPage = (bufLength + DEFAULT_PAGE_LENGTH - 1) / DEFAULT_PAGE_LENGTH;

		m_freeHeadPage += usedPage;
		m_freePageNum -= usedPage;

		m_bBusy = false;
	}
}

void CDataBufferEx::ReleaseBuf(unsigned char *pBuf, unsigned int bufLength)
{
	assert ((m_pBuffer <= pBuf) && (pBuf < (m_pBuffer + m_bufLength)));
	assert ((pBuf + bufLength) <= (m_pBuffer +m_bufLength));
	assert (pBuf == (m_pBuffer + m_freeTailPage * DEFAULT_PAGE_LENGTH));

	if ((m_pBuffer <= pBuf) && (pBuf < (m_pBuffer + m_bufLength)) && (pBuf == (m_pBuffer + m_freeTailPage * DEFAULT_PAGE_LENGTH)))
	{
		unsigned int freePage = (bufLength + DEFAULT_PAGE_LENGTH - 1) / DEFAULT_PAGE_LENGTH;
		m_freeTailPage += freePage;
		m_freePageNum += freePage;

		//释放到和最后一块没用到的内存页面组连接时，则同时释放最后一块没用的页面组
		if(m_freeTailPage == m_bufTailPage)
		{
			m_freePageNum += (m_pageNum - m_freeTailPage);
			m_freeTailPage = 0;
			m_bufTailPage = m_pageNum;
		}
	}
}

//end
