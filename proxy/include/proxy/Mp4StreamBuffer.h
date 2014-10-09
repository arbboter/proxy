#ifndef _MP4_STREAM_BUFFER_H_
#define _MP4_STREAM_BUFFER_H_
#include "StreamBuffer.h"

class CMp4StreamBuffer : public CStreamBuffer
{
public:
	CMp4StreamBuffer();
	virtual ~CMp4StreamBuffer();

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
	char *m_pBuf;
	long m_bufSize;
	long m_unitSize;

	STREAM_BLOCK_LIST m_emptyList;
	STREAM_BLOCK_LIST m_filledList;
	STREAM_BLOCK_LIST m_releaseList;

	unsigned long m_recvBytes;
	unsigned long m_feedBackBytes;
};

#endif