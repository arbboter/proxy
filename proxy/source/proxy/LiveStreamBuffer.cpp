#include "LiveStreamBuffer.h"

const int MUXER_BUF_SIZE = 2*1024*1024;

CLiveStreamBuffer::CLiveStreamBuffer()
{
	m_pMainStreamBuf = NULL;
	m_pSubStreamBuf = NULL;

	m_encode.Init();
}

CLiveStreamBuffer::~CLiveStreamBuffer()
{
	DelLiveStream(m_pMainStreamBuf);
	m_pMainStreamBuf = NULL;

	DelLiveStream(m_pSubStreamBuf);
	m_pSubStreamBuf = NULL;

	m_encode.Quit();
}


//为主子码流添加删除缓存;
bool CLiveStreamBuffer::AddBuffer(long bufSize,long bufUnitSize,bool bMainStream)
{
	if ((bMainStream && (NULL != m_pMainStreamBuf)) || (!bMainStream && (NULL != m_pSubStreamBuf)))
	{
		assert(false);
		return true;
	}

	LIVE_STREAM_BUF *pLiveStream = new LIVE_STREAM_BUF;
	memset(pLiveStream,0,sizeof(LIVE_STREAM_BUF));

	pLiveStream->bufSize = bufSize;
	pLiveStream->unitSize = bufUnitSize;
	pLiveStream->pBuf = new char[pLiveStream->bufSize];

	long unitNum = pLiveStream->bufSize/pLiveStream->unitSize;

	pLiveStream->pEmptyList = new STREAM_BLOCK_LIST;
	pLiveStream->pFilledList = new STREAM_BLOCK_LIST;
	pLiveStream->pReleaseList = new STREAM_BLOCK_LIST;

	STREAM_BLOCK *pStreamBlock = NULL;
	for (long i = 0; i < unitNum; i++)
	{
		pStreamBlock = new STREAM_BLOCK;

		memset(pStreamBlock,0,sizeof(STREAM_BLOCK));
		pStreamBlock->pBuf = pLiveStream->pBuf+i*pLiveStream->unitSize;
		pLiveStream->pEmptyList->push_back(pStreamBlock);
	}


	pLiveStream->pTSMuxer = new CTSMuxer;

	TS_OPTION option;
	option.SetBitRate(374 * 1024);

	pLiveStream->programID = option.AddProgram();
	pLiveStream->videoID = option.AddStream(pLiveStream->programID, 'v');
	pLiveStream->audioID = option.AddStream(pLiveStream->programID,'a');

	option.SetStreamFPSOrBPS(pLiveStream->programID, pLiveStream->videoID, 30);
	option.SetStreamFPSOrBPS(pLiveStream->programID, pLiveStream->audioID, 8000);

	pLiveStream->pTSMuxer->Init(option);
	pLiveStream->pTSMuxer->AllocBuffer(MUXER_BUF_SIZE);

	pLiveStream->pData = NULL;

	if (bMainStream)
	{
		m_pMainStreamBuf = pLiveStream;
	}
	else
	{
		m_pSubStreamBuf = pLiveStream;
	}

	return true;
}

bool CLiveStreamBuffer::DelBuffer(bool bMainStream)
{
	LIVE_STREAM_BUF *pLiveStream = NULL;
	if (bMainStream)
	{
		pLiveStream = m_pMainStreamBuf;
		m_pMainStreamBuf = NULL;
	}
	else
	{
		pLiveStream = m_pSubStreamBuf;
		m_pSubStreamBuf = NULL;
	}

	DelLiveStream(pLiveStream);

	return true;
}

//判断时候删除了所有缓存
bool CLiveStreamBuffer::bDelAllBuffer()
{
	return (NULL==m_pMainStreamBuf) && (NULL==m_pSubStreamBuf);
}

//把数据放入缓存
bool CLiveStreamBuffer::InputData(bool bMainStream,char *pData,unsigned long dataLen)
{
	bool ret = false;

	if (((NULL!=m_pMainStreamBuf)&&(NULL!=m_pMainStreamBuf->pData)) || ((NULL!=m_pSubStreamBuf)&&(NULL!=m_pSubStreamBuf->pData)))
	{
		ret = false;
	}
	else
	{
		TVT_DATA_FRAME *pframe = (TVT_DATA_FRAME *)pData;
		TVT_DATA_FRAME *pAACDataFrame = NULL;


		pframe->pData = (unsigned char *)pData;

		if ((TVT_STREAM_TYPE_VIDEO_1==pframe->frame.streamType) && (NULL!=m_pMainStreamBuf))
		{
			if (CTSMuxer::TS_MUXER_OK == m_pMainStreamBuf->pTSMuxer->Put(m_pMainStreamBuf->programID,m_pMainStreamBuf->videoID,pframe))
			{
				m_pMainStreamBuf->pTSMuxer->Get(&(m_pMainStreamBuf->pData),&(m_pMainStreamBuf->dataLen));
			}
			else
			{
				assert(false);
			}
		}
		else if((TVT_STREAM_TYPE_VIDEO_2==pframe->frame.streamType) && (NULL!=m_pSubStreamBuf))
		{
			if (CTSMuxer::TS_MUXER_OK == m_pSubStreamBuf->pTSMuxer->Put(m_pSubStreamBuf->programID,m_pSubStreamBuf->videoID,pframe))
			{
				m_pSubStreamBuf->pTSMuxer->Get(&(m_pSubStreamBuf->pData),&(m_pSubStreamBuf->dataLen));
			}
			else
			{
				assert(false);
			}
		}
		/*else if((TVT_STREAM_TYPE_AUDIO==pframe->frame.streamType) && (m_encode.AudioFrameToAac(pframe,&pAACDataFrame)))
		{	
		assert(NULL != pAACDataFrame);
		if ((NULL!=m_pMainStreamBuf) && (CTSMuxer::TS_MUXER_OK==m_pMainStreamBuf->pTSMuxer->Put(m_pMainStreamBuf->programID,m_pMainStreamBuf->audioID,pAACDataFrame)))
		{
		m_pMainStreamBuf->pTSMuxer->Get(&(m_pMainStreamBuf->pData),&(m_pMainStreamBuf->dataLen));
		}

		if ((NULL!=m_pSubStreamBuf) && (CTSMuxer::TS_MUXER_OK==m_pSubStreamBuf->pTSMuxer->Put(m_pSubStreamBuf->programID,m_pSubStreamBuf->audioID,pAACDataFrame)))
		{
		m_pSubStreamBuf->pTSMuxer->Get(&(m_pSubStreamBuf->pData),&(m_pSubStreamBuf->dataLen));
		}

		pAACDataFrame = NULL;
		}*/

		ret = true;
	}

	if ((NULL!=m_pMainStreamBuf) && (NULL!=m_pMainStreamBuf->pData) && InputStreamBuffer(m_pMainStreamBuf->pData,m_pMainStreamBuf->dataLen,true))
	{
		m_pMainStreamBuf->pTSMuxer->Free(m_pMainStreamBuf->pData,m_pMainStreamBuf->dataLen);
		m_pMainStreamBuf->pData = NULL;
		m_pMainStreamBuf->dataLen = 0;
	}

	if ((NULL!=m_pSubStreamBuf) && (NULL!=m_pSubStreamBuf->pData) && InputStreamBuffer(m_pSubStreamBuf->pData,m_pSubStreamBuf->dataLen,false))
	{
		m_pSubStreamBuf->pTSMuxer->Free(m_pSubStreamBuf->pData,m_pSubStreamBuf->dataLen);
		m_pSubStreamBuf->pData = NULL;
		m_pSubStreamBuf->dataLen = 0;
	}

	return ret;
}

//获取缓冲的数据
bool CLiveStreamBuffer::GetData(bool bMainStream,char **ppData,unsigned long *pDataLen)
{
	STREAM_BLOCK_LIST *pFilledBlockList = NULL;
	STREAM_BLOCK_LIST *pReleaseBlockList = NULL;

	if (bMainStream && (NULL != m_pMainStreamBuf))
	{
		pFilledBlockList = m_pMainStreamBuf->pFilledList;
		pReleaseBlockList = m_pMainStreamBuf->pReleaseList;
	}
	else if(!bMainStream && (NULL != m_pSubStreamBuf))
	{
		pFilledBlockList = m_pSubStreamBuf->pFilledList;
		pReleaseBlockList = m_pSubStreamBuf->pReleaseList;
	}
	
	if ((NULL == pFilledBlockList) || (NULL == pReleaseBlockList))
	{
		return false;
	}

	if (pFilledBlockList->empty())
	{
		return false;
	}

	*ppData = pFilledBlockList->front()->pBuf;
	*pDataLen = pFilledBlockList->front()->dataLen;

	pReleaseBlockList->push_back(pFilledBlockList->front());
	pFilledBlockList->pop_front();
	
	return true;
}

//释放数据块
void CLiveStreamBuffer::RelData(bool bMainStream,char *pData,unsigned long dataLen)
{
	STREAM_BLOCK_LIST *pReleaseBlockList = NULL;
	STREAM_BLOCK_LIST *pEmptyBlockList = NULL;

	if (bMainStream && (NULL != m_pMainStreamBuf))
	{
		pReleaseBlockList = m_pMainStreamBuf->pReleaseList;
		pEmptyBlockList = m_pMainStreamBuf->pEmptyList;
	}
	else if(!bMainStream && (NULL != m_pSubStreamBuf))
	{
		pReleaseBlockList = m_pSubStreamBuf->pReleaseList;
		pEmptyBlockList = m_pSubStreamBuf->pEmptyList;
	}

	if ((NULL == pEmptyBlockList) || (NULL == pReleaseBlockList) || pReleaseBlockList->empty())
	{
		return ;
	}

	if (pData == pReleaseBlockList->front()->pBuf)
	{
		pEmptyBlockList->push_back(pReleaseBlockList->front());
		pReleaseBlockList->pop_front();
	}
	else
	{
		assert(false);
	}
}


unsigned long CLiveStreamBuffer::NeedFeedBack()
{
	return 0;
}


bool CLiveStreamBuffer::InputStreamBuffer(char *pData,unsigned long dataLen,bool bMainStream)
{
	STREAM_BLOCK_LIST *pFilledBlockList = NULL;
	STREAM_BLOCK_LIST *pEmptyBlockList = NULL; 
	long unitSize = 0;

	if (bMainStream && (NULL!=m_pMainStreamBuf))
	{
		pFilledBlockList = m_pMainStreamBuf->pFilledList;
		pEmptyBlockList = m_pMainStreamBuf->pEmptyList;
		unitSize = m_pMainStreamBuf->unitSize;
	}
	else if (!bMainStream && (NULL!=m_pSubStreamBuf))
	{
		pFilledBlockList = m_pSubStreamBuf->pFilledList;
		pEmptyBlockList = m_pSubStreamBuf->pEmptyList;
		unitSize = m_pSubStreamBuf->unitSize;
	}
	
	if ((NULL == pEmptyBlockList) || (NULL == pFilledBlockList) || (0 == unitSize))
	{
		return true;
	}

	long bufUnitNum = (dataLen+unitSize)/unitSize;
	char *pReadPos = pData;
	long remainDataLen = dataLen;
	STREAM_BLOCK *pStreamBlock = NULL;

	if (pEmptyBlockList->size() >= bufUnitNum)
	{
		while(remainDataLen > 0)
		{
			pStreamBlock = pEmptyBlockList->front();
			pEmptyBlockList->pop_front();

			if (remainDataLen > unitSize)
			{
				memcpy(pStreamBlock->pBuf,pReadPos,unitSize);
				pStreamBlock->dataLen = unitSize;
				pReadPos += unitSize;
				remainDataLen -= unitSize;
			}
			else
			{
				memcpy(pStreamBlock->pBuf,pReadPos,remainDataLen);
				pStreamBlock->dataLen = remainDataLen;
				remainDataLen = 0;
			}

			pFilledBlockList->push_back(pStreamBlock);
		}

		return true;
	}
	else
	{
		return false;
	}
}

void CLiveStreamBuffer::DelLiveStream(LIVE_STREAM_BUF *pLiveStream)
{
	if (NULL == pLiveStream)
	{
		return;
	}

	delete pLiveStream->pBuf;
	pLiveStream->pTSMuxer->DestroyBuffer();
	pLiveStream->pTSMuxer->Quit();
	delete pLiveStream->pTSMuxer;

	pLiveStream->pEmptyList->insert(pLiveStream->pEmptyList->begin(),pLiveStream->pFilledList->begin(),pLiveStream->pFilledList->end());
	pLiveStream->pEmptyList->insert(pLiveStream->pEmptyList->begin(),pLiveStream->pReleaseList->begin(),pLiveStream->pReleaseList->end());

	STREAM_BLOCK *pStreamBlock = NULL;
	while(!pLiveStream->pEmptyList->empty())
	{
		pStreamBlock = pLiveStream->pEmptyList->front();
		pLiveStream->pEmptyList->pop_front();
		assert(NULL != pStreamBlock);
		delete pStreamBlock;
	}
}