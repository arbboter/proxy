#ifndef _STREAM_BUFEER_H_
#define _STREAM_BUFEER_H_

#include <list>

typedef struct _stream_block_
{
	char *pBuf;
	long dataLen;
}STREAM_BLOCK;

typedef std::list<STREAM_BLOCK *> STREAM_BLOCK_LIST;

const int STREAM_BUF_SIZE  = 8*1024*1024;
const int STREAM_UNIT_SIZE = 8*1024;


class CStreamBuffer
{
public:
	CStreamBuffer(){}
	virtual ~CStreamBuffer(){}

	//为主子码流添加删除缓存;
	virtual bool AddBuffer(long bufSize,long bufUnitSize,bool bMainStream) = 0;
	virtual bool DelBuffer(bool bMainStream) = 0;
	
	//判断时候删除了所有缓存
	virtual bool bDelAllBuffer() = 0;

	//把数据放入缓存
	virtual bool InputData(bool bMainStream,char *pData,unsigned long dataLen) = 0;

	//获取缓冲的数据
	virtual bool GetData(bool bMainStream,char **ppData,unsigned long *pDataLen) = 0;

	//释放数据块
	virtual void RelData(bool bMainStream,char *pData,unsigned long dataLen) = 0;

	//仅供mp4流使用
	virtual unsigned long NeedFeedBack() = 0;
};

#endif