// SWL_MultiNetComm.h: interface for the CNatMultiNetComm class.
//
//////////////////////////////////////////////////////////////////////

#ifndef  __NAT_MULTI_NET_MAT__
#define __NAT_MULTI_NET_MAT__

#include "SWL_Public.h"
#include "TVT_PubCom.h"
#include "NatSocket.h"

#include "SWL_MultiNet.h"

#define		NAT_MAX_KEEP_TIME		60000
//网络连接资源结构体
 typedef struct _nat_link_resource_
 {
 	NatSocket 		    sock;
 	long				deviceID;
 	long				bufferMaxLen;		//根据请求数据的通道数动态指定的最大可用缓冲大小
 	RECV_DATA_BUFFER	recvBuffer;			//要接收的数据
 	RECVASYN_CALLBACK	pfRecvCallBack;		//接收数据的回调函数
 	void*				pParam;				//回调函数参数
 	bool				bBroken;			//是否已断开
 }NAT_LINK_RESOURCE;

class CNatMultiNetComm : public CSWL_MultiNet
{
public:
	CNatMultiNetComm();
	virtual ~CNatMultiNetComm();

public:
	bool Init();

	void Destroy();
	//增加新的网络连接
	int AddComm(long deviceID, TVT_SOCKET sock);

	//删除指定ID的网络连接
	int RemoveComm(long deviceID);

	//删除所有的网络连接
	int RemoveAllConnect();

	int SendData(long deviceID, const void *pBuf, int iLen, bool bBlock);

	//设置自动接收数据的回调函数
	int SetAutoRecvCallback(long deviceID, RECVASYN_CALLBACK pfRecvCallBack, void *pParam);

	//设置指定连接的接收缓冲区大小
	void SetRecvBufferLen(long deviceID, long bufferLen);


private:
	int SendBuff(long deviceID, const void *pBuf, int iLen, bool bBlock, long lBlockTime);
	int RecvBuff(long deviceID, void *pBuf, int iLen, bool bBlock, long lBlockTime);
	static PUB_THREAD_RESULT PUB_THREAD_CALL RecvThread(void *pParam);
	void RecvThreadRun();

	bool GetLinkResourceByLinkID(long deviceID, NAT_LINK_RESOURCE * &pLinkResource);

	void UpdateRemoveComm();

private:
	list<NAT_LINK_RESOURCE *>				m_listNetLink;
	list<NAT_LINK_RESOURCE *>				m_tempDelNetLink;
	CPUB_Lock								m_lstLinkLock;
	CPUB_Lock								m_lstLinkLockForCallback;
	
//	int										m_maxSockInt;
	bool									m_bSockThreadRun;
 	NatSocketSet							m_fdReadSet;
	PUB_thread_t							m_threadId;
};

#endif // !defined(AFX_SWL_MULTINETCOMM_H__9E723A1A_97DE_445E_80BD_820A85DC30B6__INCLUDED_)
