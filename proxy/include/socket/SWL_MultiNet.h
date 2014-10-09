// SWL_MultiNetComm.h: interface for the CSWL_MultiNetComm class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __SWL_MULTINET_H__
#define __SWL_MULTINET_H__

#include "SWL_Public.h"
#include "TVT_PubCom.h"

#include <string.h>
#include <list>

using namespace std;

typedef struct _recv_data_buffer_
{
	long			bufferSize;	//缓冲区容量
	long			dataSize;	//缓冲区中数据大小
	char			*pData;		//接收数据的缓冲区
	long			dataType;	//用于解析一些特殊数据，为兼容以前的传输模块设计
}RECV_DATA_BUFFER;

//数据处理成功返回0，否则返回-1，此时会进入阻塞状态
typedef int (*RECVASYN_CALLBACK)(long clientID, void* pParam, RECV_DATA_BUFFER *pDataBuffer);

class CSWL_MultiNet 
{
public:
	CSWL_MultiNet();
	virtual ~CSWL_MultiNet();

public:
	virtual bool Init();

	virtual void Destroy();
	//增加新的网络连接
	virtual int AddComm(long deviceID, TVT_SOCKET sock);

	//删除指定ID的网络连接
	virtual int RemoveComm(long deviceID);

	//删除所有的网络连接
	virtual int RemoveAllConnect();

	virtual int SendData(long deviceID, const void *pBuf, int iLen, bool bBlock);

	//设置自动接收数据的回调函数
	virtual int SetAutoRecvCallback(long deviceID, RECVASYN_CALLBACK pfRecvCallBack, void *pParam){return 0;}

	//设置指定连接的接收缓冲区大小
	virtual void SetRecvBufferLen(long deviceID, long bufferLen){}
};

#endif
