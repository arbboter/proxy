#ifndef _STREAM_BUFFER_MAN_H_
#define _STREAM_BUFFER_MAN_H_

#include "TVT_PubCom.h"
#include "StreamBuffer.h"
#include <list>


class CStreamBufferMan
{
	typedef struct _data_block_
	{	
		long deviceID;
		bool bLive;
		long channl;

		CStreamBuffer *pStream;
	}STREAM_BUFFER;

	typedef std::list<STREAM_BUFFER*> STREAM_BUFFER_LIST;

public:
	CStreamBufferMan();
	~CStreamBufferMan();

	bool Init(long bufSize = STREAM_BUF_SIZE,long bufUnitSize = STREAM_UNIT_SIZE);

	void Quit();

	bool InputData(long deviceID,long channl,bool bLive,bool bMainStream,char *pData,unsigned long dataLen);

	//获取缓冲的数据
	bool GetData(long deviceID,long channl,bool bLive,bool bMainStream,char **ppData,unsigned long *pDataLen);

	//释放数据块
	void RelData(long deviceID,long channl,bool bLive,bool bMainStream,char *pData,unsigned long dataLen);

	//为设备添加缓存
	bool AddDeviceBuf(long deviceID,long channl,bool bLive,bool bMainStream);

	//删除设备指定通道的Buf
	void DelDeviceBuf(long deviceID,long channl,bool bLive,bool bMainStream);

	//删除设备的所有Buf
	void DelDeviceBuf(long deviceID);

	//仅供mp4流使用
	unsigned long NeedFeedBack(long deviceID,long channl,bool bLive);

private:
	STREAM_BUFFER * FindStreamBuffer(long deviceID,long channl,bool bLive);

private:
	STREAM_BUFFER_LIST m_streamBufList;
	CPUB_Lock m_streamBufLock;
	
	long m_streamBufSize;
	long m_bufUnitSize;
};
#endif