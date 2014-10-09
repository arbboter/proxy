#include "Mp4StreamBuffer.h"
#include <assert.h>

const int MP4_FEED_BACK_BYTES = 256*1024;

CMp4StreamBuffer::CMp4StreamBuffer()
{
	m_bufSize = 0;
	m_unitSize = 0;
	m_pBuf = NULL;

	m_recvBytes = 0;
	m_feedBackBytes = 0;
}

CMp4StreamBuffer::~CMp4StreamBuffer()
{
	if (NULL != m_pBuf)
	{
		delete [] m_pBuf;m_pBuf = NULL;
	}

	m_emptyList.insert(m_emptyList.end(),m_filledList.begin(),m_filledList.end());
	m_filledList.clear();
	m_emptyList.insert(m_emptyList.end(),m_releaseList.begin(),m_releaseList.end());
	m_releaseList.clear();

	STREAM_BLOCK *pStreamBlock = NULL;
	while(!m_emptyList.empty())
	{
		pStreamBlock = m_emptyList.front();
		m_emptyList.pop_front();

		assert(NULL != pStreamBlock);
		delete pStreamBlock;
	}

	m_emptyList.clear();
}

//为主子码流添加删除缓存;
bool CMp4StreamBuffer::AddBuffer(long bufSize,long bufUnitSize,bool bMainStream)
{
	if (NULL != m_pBuf)
	{
		return true;
	}

	m_bufSize = bufSize;
	m_unitSize = bufUnitSize;
	m_pBuf = new char[m_bufSize];


	long unitNum = m_bufSize/m_unitSize;
	STREAM_BLOCK *pStreamBlock = NULL;
	for (long i = 0; i < unitNum; i++)
	{
		pStreamBlock = new STREAM_BLOCK;
		
		memset(pStreamBlock,0,sizeof(STREAM_BLOCK));
		pStreamBlock->pBuf = m_pBuf+i*m_unitSize;
		
		m_emptyList.push_back(pStreamBlock);
	}

	m_recvBytes = 0;
	m_feedBackBytes = 0;
	
	return true;
}

bool CMp4StreamBuffer::DelBuffer(bool bMainStream)
{
	m_emptyList.clear();
	m_filledList.clear();
	m_releaseList.clear();

	if (NULL != m_pBuf)
	{
		delete [] m_pBuf;m_pBuf = NULL;
	}

	m_bufSize = 0;
	m_unitSize = 0;

	return true;
}

//判断时候删除了所有缓存
bool CMp4StreamBuffer::bDelAllBuffer()
{
	return NULL == m_pBuf;
}

//把数据放入缓存
bool CMp4StreamBuffer::InputData(bool bMainStream,char *pData,unsigned long dataLen)
{
	long bufUnitNum = (dataLen+m_unitSize)/m_unitSize;
	char *pReadPos = pData;
	long remainDataLen = dataLen;
	STREAM_BLOCK *pStreamBlock = NULL;

	if (NULL == pReadPos || remainDataLen <= 0)
	{
		return true;
	}

	if (m_emptyList.size() >= bufUnitNum)
	{
		while(remainDataLen > 0)
		{
			pStreamBlock = m_emptyList.front();
			m_emptyList.pop_front();

			if (remainDataLen > m_unitSize)
			{
				memcpy(pStreamBlock->pBuf,pReadPos,m_unitSize);
				pStreamBlock->dataLen = m_unitSize;
				pReadPos += m_unitSize;
				remainDataLen -= m_unitSize;
			}
			else
			{
				memcpy(pStreamBlock->pBuf,pReadPos,remainDataLen);
				pStreamBlock->dataLen = remainDataLen;
				remainDataLen = 0;
			}
			m_filledList.push_back(pStreamBlock);
		}
	
		m_recvBytes += dataLen;

		return true;
	}

	return false;
}

//获取缓冲的数据
bool CMp4StreamBuffer::GetData(bool bMainStream,char **ppData,unsigned long *pDataLen)
{
	if (m_filledList.empty())
	{
		return false;
	}

	*ppData = m_filledList.front()->pBuf;
	*pDataLen = m_filledList.front()->dataLen;

	m_releaseList.push_back(m_filledList.front());
	m_filledList.pop_front();

	return true;
}

//释放数据块
void CMp4StreamBuffer::RelData(bool bMainStream,char *pData,unsigned long dataLen)
{
	assert(m_releaseList.front()->pBuf == pData);
	m_emptyList.push_back(m_releaseList.front());
	m_releaseList.pop_front();
}

unsigned long CMp4StreamBuffer::NeedFeedBack()
{
	unsigned long ret = 0;

	if ((m_recvBytes - m_feedBackBytes) > (MP4_FEED_BACK_BYTES/2))
	{
		ret = m_recvBytes;
		m_feedBackBytes = m_recvBytes;
	}
	
	return ret;
}