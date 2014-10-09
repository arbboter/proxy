#ifndef _LIVE_STREAM_BUFFER_H_
#define _LIVE_STREAM_BUFFER_H_
#include "StreamBuffer.h"
#include "TSMuxer.h"
#include "aacEncode.h"

class CLiveStreamBuffer : public CStreamBuffer
{
	typedef struct _live_stream_buf_
	{
		char *pBuf;
		long bufSize;
		long unitSize;
		STREAM_BLOCK_LIST *pEmptyList;
		STREAM_BLOCK_LIST *pFilledList;
		STREAM_BLOCK_LIST *pReleaseList;
		CTSMuxer *pTSMuxer;
		long programID;
		long videoID;
		long audioID;
		char *pData;
		int dataLen;
	}LIVE_STREAM_BUF;

public:
	CLiveStreamBuffer();
	virtual ~CLiveStreamBuffer();

	//为主子码流添加删除缓存;
	virtual bool AddBuffer(long bufSize,long bufUnitSize,bool bMainStream);
	virtual bool DelBuffer(bool bMainStream);

	//判断时候删除了所有缓存
	virtual bool bDelAllBuffer();

	//把数据放入缓存
	virtual bool InputData(bool bMainStream,char *pData,unsigned long dataLen);

	//获取缓冲的数据
	virtual bool GetData(bool bMainStream,char **ppData,unsigned long *pDataLen);

	//释放数据块
	virtual void RelData(bool bMainStream,char *pData,unsigned long dataLen);
	
	//仅供mp4流使用
	virtual unsigned long NeedFeedBack();

private:
	bool InputStreamBuffer(char *pData,unsigned long dataLen,bool bMainStream);
	
	void DelLiveStream(LIVE_STREAM_BUF *pLiveStream);
private:
	LIVE_STREAM_BUF *m_pMainStreamBuf;
	LIVE_STREAM_BUF *m_pSubStreamBuf;

	CAacEncoder		m_encode;
};

#endif
