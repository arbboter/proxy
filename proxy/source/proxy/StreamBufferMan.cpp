#include "StreamBufferMan.h"
#include "Mp4StreamBuffer.h"
#include "LiveStreamBuffer.h"

CStreamBufferMan::CStreamBufferMan()
{
	m_streamBufSize = 0;
	m_bufUnitSize = 0;
}


CStreamBufferMan::~CStreamBufferMan()
{
	for (STREAM_BUFFER_LIST::iterator iter = m_streamBufList.begin(); iter != m_streamBufList.end(); ++iter)
	{
		if (NULL != (*iter)->pStream)
		{
			delete (*iter)->pStream;
		}
	}

	m_streamBufList.clear();
}


bool CStreamBufferMan::Init(long bufSize,long bufUnitSize)
{
	m_streamBufSize = bufSize;
	m_bufUnitSize = bufUnitSize;

	return true;
}

void CStreamBufferMan::Quit()
{
	for (STREAM_BUFFER_LIST::iterator iter = m_streamBufList.begin(); iter != m_streamBufList.end(); ++iter)
	{
		if (NULL != (*iter)->pStream)
		{
			delete (*iter)->pStream;
		}
	}

	m_streamBufList.clear();
}

bool CStreamBufferMan::InputData(long deviceID,long channl,bool bLive,bool bMainStream,char *pData,unsigned long dataLen)
{
	bool ret = false;
	STREAM_BUFFER *pBuf = NULL;

	m_streamBufLock.Lock();
	pBuf = FindStreamBuffer(deviceID,channl,bLive);
	if (NULL != pBuf)
	{
		ret = pBuf->pStream->InputData(bMainStream,pData,dataLen);
	}
	else
	{
		ret = true;
	}
	m_streamBufLock.UnLock();
	
	return ret;
}

//获取缓冲的数据
bool CStreamBufferMan::GetData(long deviceID,long channl,bool bLive,bool bMainStream,char **ppData,unsigned long *pDataLen)
{
	bool ret = false;
	STREAM_BUFFER *pBuf = NULL;

	m_streamBufLock.Lock();
	pBuf = FindStreamBuffer(deviceID,channl,bLive);
	if (NULL != pBuf)
	{
		ret = pBuf->pStream->GetData(bMainStream,ppData,pDataLen);
	}
	m_streamBufLock.UnLock();

	return ret;
}

//释放数据块
void CStreamBufferMan::RelData(long deviceID,long channl,bool bLive,bool bMainStream,char *pData,unsigned long dataLen)
{
	STREAM_BUFFER *pBuf = NULL;

	m_streamBufLock.Lock();
	pBuf = FindStreamBuffer(deviceID,channl,bLive);
	if (NULL != pBuf)
	{
		pBuf->pStream->RelData(bMainStream,pData,dataLen);
	}
	m_streamBufLock.UnLock();
}


//为设备添加缓存
bool CStreamBufferMan::AddDeviceBuf(long deviceID,long channl,bool bLive,bool bMainStream)
{
	STREAM_BUFFER *pBuf = NULL;

	m_streamBufLock.Lock();
	pBuf = FindStreamBuffer(deviceID,channl,bLive);

	if (NULL == pBuf)
	{
		pBuf = new STREAM_BUFFER;
		memset(pBuf,0,sizeof(STREAM_BUFFER));
		pBuf->deviceID = deviceID;
		pBuf->channl = channl;
		pBuf->bLive = bLive;

		if (pBuf->bLive)
		{
			pBuf->pStream = new CLiveStreamBuffer;
		}
		else
		{
			pBuf->pStream = new CMp4StreamBuffer;
		}
		
		m_streamBufList.push_back(pBuf);
	}

	assert(pBuf != NULL);
	pBuf->pStream->AddBuffer(m_streamBufSize,m_bufUnitSize,bMainStream);

	m_streamBufLock.UnLock();

	return true;
}

//删除设备指定通道的Buf
void CStreamBufferMan::DelDeviceBuf(long deviceID,long channl,bool bLive,bool bMainStream)
{
	STREAM_BUFFER *pBuf = NULL;

	m_streamBufLock.Lock();
	pBuf = FindStreamBuffer(deviceID,channl,bLive);
	if (NULL != pBuf)
	{
		pBuf->pStream->DelBuffer(bMainStream);
		
		if (pBuf->pStream->bDelAllBuffer())
		{
			delete pBuf->pStream;
			delete pBuf;

			m_streamBufList.remove(pBuf);
		}
	}
	m_streamBufLock.UnLock();
}

//删除设备的所有Buf
void CStreamBufferMan::DelDeviceBuf(long deviceID)
{
	m_streamBufLock.Lock();
	for (STREAM_BUFFER_LIST::iterator iter = m_streamBufList.begin(); iter != m_streamBufList.end();)
	{
		if (deviceID == (*iter)->deviceID)
		{
			delete (*iter)->pStream;
			delete (*iter);

			iter = m_streamBufList.erase(iter);
		}
		else
		{
			++iter;
		}
	}
	m_streamBufLock.UnLock();
}

unsigned long CStreamBufferMan::NeedFeedBack(long deviceID,long channl,bool bLive)
{
	unsigned long ret = 0;
	if (bLive)
	{
		return ret;
	}

	STREAM_BUFFER *pBuf = NULL;
	m_streamBufLock.Lock();
	pBuf = FindStreamBuffer(deviceID,channl,bLive);
	if (NULL != pBuf)
	{
		ret = pBuf->pStream->NeedFeedBack();
	}

	m_streamBufLock.UnLock();

	return ret;
}

CStreamBufferMan::STREAM_BUFFER *CStreamBufferMan::FindStreamBuffer(long deviceID,long channl,bool bLive)
{
	STREAM_BUFFER *pBuf = NULL;

	for (STREAM_BUFFER_LIST::iterator iter = m_streamBufList.begin(); iter != m_streamBufList.end(); ++iter)
	{
		if ((deviceID==(*iter)->deviceID) && (channl==(*iter)->channl) && (bLive==(*iter)->bLive))
		{
			pBuf = (*iter);
			break;
		}
	}
	

	return pBuf;
}