// SWL_MultiNetComm.cpp: implementation of the CNatMultiNetComm class.
//
//////////////////////////////////////////////////////////////////////
#include "TVT_PubDefine.h"
#ifndef WIN32
//#include "lvfw.h"
#else
#include <vfw.h>
#endif
#include "NatMultiNetComm.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNatMultiNetComm::CNatMultiNetComm()
{
	m_bSockThreadRun	= false;
	m_threadId			= PUB_CREATE_THREAD_FAIL;
}

CNatMultiNetComm::~CNatMultiNetComm()
{

}

bool CNatMultiNetComm::Init()
{
	m_threadId = PUB_CreateThread(RecvThread, this, &m_bSockThreadRun);
	if(m_threadId == PUB_CREATE_THREAD_FAIL)
	{
		assert(false);
		return false;
	}
	return true;
}

void CNatMultiNetComm::Destroy()
{
	if(m_threadId != PUB_CREATE_THREAD_FAIL)
	{
		PUB_ExitThread(&m_threadId, &m_bSockThreadRun);
	}
	m_lstLinkLock.Lock();

	list<NAT_LINK_RESOURCE*>::iterator it;
	for(it = m_listNetLink.begin(); it != m_listNetLink.end(); it++)
	{
		NAT_LINK_RESOURCE *pNetLinkResource = *it;
 		NAT_EraseSocketFromSet(pNetLinkResource->sock, &m_fdReadSet);
        NAT_CloseSocket(pNetLinkResource->sock);
		if(pNetLinkResource->recvBuffer.pData != NULL)
		{
			delete [] pNetLinkResource->recvBuffer.pData;
		}
		delete pNetLinkResource;
	}
	m_listNetLink.clear();
	m_tempDelNetLink.clear();

	m_lstLinkLock.UnLock();
}
/******************************************************************************
*
*
*
******************************************************************************/
int CNatMultiNetComm::AddComm(long deviceID, TVT_SOCKET sock)
{
	m_lstLinkLockForCallback.Lock();
	m_lstLinkLock.Lock();

	NAT_LINK_RESOURCE *pNetLinkResource = new NAT_LINK_RESOURCE;
	assert(NULL != pNetLinkResource);
	pNetLinkResource->deviceID = deviceID;
	pNetLinkResource->sock = sock.natSock;
	pNetLinkResource->bBroken = false;
	pNetLinkResource->pfRecvCallBack = NULL;
	memset(&pNetLinkResource->recvBuffer, 0, sizeof(RECV_DATA_BUFFER));

	m_listNetLink.push_back(pNetLinkResource);

 	NAT_InsertSocketToSet(sock.natSock, &m_fdReadSet);
	
	m_lstLinkLock.UnLock();
	m_lstLinkLockForCallback.UnLock();

	return 0;
}

/******************************************************************************
*
*
*
******************************************************************************/
int CNatMultiNetComm::RemoveComm(long deviceID)
{
	m_lstLinkLock.Lock();

	list<NAT_LINK_RESOURCE*>::iterator it;
	for(it = m_listNetLink.begin(); it != m_listNetLink.end(); it++)
	{
		NAT_LINK_RESOURCE *pNetLinkResource = *it;
		if(pNetLinkResource->deviceID == deviceID)
		{
			m_tempDelNetLink.push_back(pNetLinkResource);
			break;
		}
	}

	m_lstLinkLock.UnLock();

	return 0;
}

int CNatMultiNetComm::RemoveAllConnect()
{
	m_lstLinkLock.Lock();

	list<NAT_LINK_RESOURCE*>::iterator it;
	for(it = m_listNetLink.begin(); it != m_listNetLink.end(); it++)
	{
		m_tempDelNetLink.push_back(*it);
	}

	m_lstLinkLock.UnLock();

	UpdateRemoveComm();

	return 0;
}

/******************************************************************************
*
*	采用同步发送数据的方式，通过此接口直接调用socket发送接口发送数据
*
*	返回值：
*	发送成功，返回实际发送成功的字节数
	发送失败，返回-1
*
*	参数：bBlock: 如果设置为true，即此接口为阻塞模式，则会等待数据发送完毕或超时为止
*
******************************************************************************/
int CNatMultiNetComm::SendData(long deviceID, const void *pBuf, int iLen, bool bBlock)
{
	m_lstLinkLock.Lock();

	int sendLen = -1;
	
	for(list<NAT_LINK_RESOURCE*>::iterator it = m_tempDelNetLink.begin(); it != m_tempDelNetLink.end(); it++)
	{
		if (deviceID == (*it)->deviceID)
		{
			m_lstLinkLock.UnLock();
			return sendLen;
		}
	}

	if(bBlock)
	{
		sendLen = SendBuff(deviceID, pBuf, iLen, true, 100);	//假定100秒会能发送成功
//		assert(sendLen == iLen);
	}
	else
	{
		sendLen = SendBuff(deviceID, pBuf, iLen, false, 0);
	}

	m_lstLinkLock.UnLock();

	return sendLen;
}

/******************************************************************************
*
*设置自动接收数据的回调函数
*成功返回0，失败返回-1
*
******************************************************************************/
int CNatMultiNetComm::SetAutoRecvCallback(long deviceID, RECVASYN_CALLBACK pfRecvCallBack, void *pParam)
{
	m_lstLinkLockForCallback.Lock();

	NAT_LINK_RESOURCE *pNetLinkResource;
	if(GetLinkResourceByLinkID(deviceID, pNetLinkResource))
	{
		pNetLinkResource->pfRecvCallBack = pfRecvCallBack;
		pNetLinkResource->pParam = pParam;
		m_lstLinkLockForCallback.UnLock();
		return 0;
	}
	m_lstLinkLockForCallback.UnLock();

	return -1;
}

void CNatMultiNetComm::SetRecvBufferLen(long deviceID, long bufferLen)
{
	assert(bufferLen > 0);

	m_lstLinkLockForCallback.Lock();

	NAT_LINK_RESOURCE *pNetLinkResource;
	if(GetLinkResourceByLinkID(deviceID, pNetLinkResource))
	{
		assert(pNetLinkResource->recvBuffer.dataSize <= bufferLen);

		//指定的发送缓冲区大小必须大于待发送数据大小
		if(pNetLinkResource->recvBuffer.dataSize > bufferLen)
		{
			m_lstLinkLockForCallback.UnLock();
			return;
		}

		char *pOldBuffer = pNetLinkResource->recvBuffer.pData;
		pNetLinkResource->recvBuffer.bufferSize = bufferLen;
		pNetLinkResource->recvBuffer.pData = new char [bufferLen];
		assert(NULL != pNetLinkResource->recvBuffer.pData);

		if(pOldBuffer != NULL)
		{
			if(pNetLinkResource->recvBuffer.dataSize > 0)
			{
				memcpy(pNetLinkResource->recvBuffer.pData, pOldBuffer, pNetLinkResource->recvBuffer.dataSize);
			}

			delete [] pOldBuffer;
		}
	}

	m_lstLinkLockForCallback.UnLock();
}

/******************************************************************************
*
*功能：执行一次网络数据发送
*参数说明：bBlock true阻塞 false非阻塞，此时dwBlockTime才有效，
		   dwBlockTime 以秒为单位阻塞
*返回值：>=0发送的字节数
		 -1表示对方断开socket
		 -2其他错误
*
******************************************************************************/

int CNatMultiNetComm::SendBuff(long deviceID, const void *pBuf, int iLen, bool bBlock, long lBlockTime)
{
	NAT_LINK_RESOURCE *pNetLinkResource;
	if(!GetLinkResourceByLinkID(deviceID, pNetLinkResource))
	{
		return -2;
	}

	assert(NULL != pBuf);
	assert(0 < iLen);
	int iRet = 0;
	int iLeft = iLen;    //还要发送的数据长度
	const char *pTmp = static_cast<const char*>(pBuf);

	time_t		startTime = time(NULL);	//用来记录第一次没发送成功的时间，计算超时用
	time_t		nowTime = 0;	//用来记录当前时间
	time_t		disTime = 0;

	while ( 0 != iLeft )
	{
		//如果recv返回0（即知道对方断开），但是这里发送还可以发送到缓冲区
		//所以先判断一下是否已经断开
		if (pNetLinkResource->bBroken) 
		{
			return -1;
		}
		
		assert(0 < iLeft);
		assert(iLeft <= iLen);
		assert(pTmp >= pBuf);
		assert(pTmp <= (static_cast<const char*>(pBuf) + iLen));

		//iRet = SWL_Send(pNetLinkResource->sock, pTmp , iLeft, 0);
        iRet = NAT_Send(pNetLinkResource->sock, pTmp, iLeft);

		if ( 0 < iRet)
		{
			pTmp += iRet;
			iLeft -= iRet;
		}
        else if ((0 == iRet)&& (!pNetLinkResource->bBroken)) // Send buffer is full
        {
			if (!bBlock) 
			{
				nowTime = time(NULL);
				disTime = nowTime - startTime;
				
				//BUGS 除了正常超时外，包系统时间改快也可能出现这种情况
				if(static_cast<time_t>(lBlockTime) <= disTime)
				{
					return iLen - iLeft;
				}
				else if (0 <= disTime) //防止系统时间被改慢很多的情况	
				{
					;//什么都不需要做
				}
				else	//系统将时间调慢将导致此情况，简单的处理方法为重新记时
				{
					startTime = time(NULL);
				}
			}				

#ifdef WIN32
				PUB_Sleep(3);
#else
				PUB_Sleep(0);
#endif
				continue;
		}
		else	// 0 > iRet // send error
		{			
			pNetLinkResource->bBroken = true;
			SWL_PrintError(__FILE__, __LINE__);
			return -1;
		}
	}
	
	return iLen - iLeft;
}

/******************************************************************************
*
*功能：执行一次网络数据接收
*参数说明：bBlock true阻塞 false非阻塞，此时dwBlockTime才有效，
		   dwBlockTime 以秒为单位阻塞
*返回值：>=0接收的字节数,
		 -1表示对方断开socket连接
		 -2表示其他错误
*
******************************************************************************/
int CNatMultiNetComm::RecvBuff(long deviceID, void *pBuf, int iLen, bool bBlock, long lBlockTime)
{
	NAT_LINK_RESOURCE *pNetLinkResource;
	if(!GetLinkResourceByLinkID(deviceID, pNetLinkResource))
	{
		assert(false);
		return -2;
	}
	assert( (NULL != pBuf) && (0 != iLen) );

	int iRet = 0;
	int iLeft = iLen;    //还要接收的数据长度
	char *pTmp = reinterpret_cast<char*>(pBuf);

	bool bContinue = false;	//在非阻塞情况下，是否需要再等待数据
	time_t startTime = time(NULL);
	time_t nowTime;
	time_t disTime;
	
	while ( 0 != iLeft ) 
	{	
		if (pNetLinkResource->bBroken) 
		{
			return -1;
		}	

		assert(0 < iLeft);
		assert(iLeft <= iLen);
		assert(pTmp >= pBuf);
		assert(pTmp < (static_cast<const char*>(pBuf) + iLen));

//		iRet = SWL_Recv(pNetLinkResource->sock, pTmp, iLeft, 0);
 //       assert(pNetLinkResource->sock.sockType == SERVER_NAT);
        iRet = NAT_Recv(pNetLinkResource->sock, pTmp, iLeft);

		if (0 < iRet)
		{
			pTmp += iRet;
			iLeft -= iRet;
			bContinue = true;
        }
        else if( 0 == iRet) // send buffer is full
        {
// 		else if( SWL_SOCKET_ERROR == iRet)
// 		{
// 			//如果是暂时没有数据,判断是否已经超时
// 			if(SWL_EWOULDBLOCK() && !pNetLinkResource->bBroken)
// 			{
				nowTime = time(NULL);
				disTime = nowTime - startTime;
				if (disTime >= lBlockTime)	//BUGS 除了正常的超时外，系统把时间改快也可能导致
				{
//					pNetLinkResource->bBroken = true;
					return iLen - iLeft;
				}
				else if(disTime >= 0)	//防止系统时间调慢后导致差值为负数，甚至导致需要一个太长时间才能超时，如几十年
				{
					if(bBlock || bContinue) //阻塞，或非阻塞接收了部分数据，
					{
						PUB_Sleep(0);
						continue;
					}
					else
					{
						return iLen - iLeft;
					}
				}
				else	//系统将时间调慢将导致此情况，简单的处理方法为重新记时
				{
					startTime = time(NULL);
				}
// 			}
// 			else
// 			{
// 				SWL_PrintError(__FILE__, __LINE__);
// 				pNetLinkResource->bBroken = true;
// 				return -1;
// 			}			
		}
		else //0 > iRet // senderror
		{
			SWL_PrintError(__FILE__, __LINE__);
			pNetLinkResource->bBroken = true;
			return -1;
		}
	}
	return iLen - iLeft;
}

PUB_THREAD_RESULT PUB_THREAD_CALL CNatMultiNetComm::RecvThread(void *pParam)
{
#ifndef	WIN32
	pid_t pid = getpid();
	pid_t tid = syscall(__NR_gettid);
	printf("%s, %d, pid = %d, tid=%d\n", __FILE__, __LINE__, pid, tid);
#endif

	CNatMultiNetComm * pThis = reinterpret_cast<CNatMultiNetComm *>(pParam);
	pThis->RecvThreadRun();

	return 0;
}

void CNatMultiNetComm::RecvThreadRun()
{
	int ret = 0;
	int callbackRet;
//	timeval timeOut;


	NatSocketSet tempReadSet, tempWriteSet;
	while(m_bSockThreadRun)
	{
		UpdateRemoveComm();

		m_lstLinkLockForCallback.Lock();
		if(m_listNetLink.empty())
		{
			m_lstLinkLockForCallback.UnLock();
			PUB_Sleep(10);
			continue;
		}
		m_lstLinkLockForCallback.UnLock();
 		tempReadSet = m_fdReadSet;

		//设置select超时时间为10ms
// 		timeOut.tv_sec = 0;
// 		timeOut.tv_usec = 10000;
		int timeOut = 10; // 10ms

		ret = NAT_Select(&tempReadSet, &tempWriteSet, timeOut);
		if(ret > 0)
		{
			m_lstLinkLockForCallback.Lock();

			//在此次读检查过程中，有连接进行了读操作，bProcessedData便置为true，以便马上进入下一次读检查
			bool bProcessedData = false;

			//对每个socket作读写检查
			list<NAT_LINK_RESOURCE *>::iterator it;
			for(it = m_listNetLink.begin(); it != m_listNetLink.end(); it++)
			{
				NAT_LINK_RESOURCE *pNetLinkResource = *it;

				if (pNetLinkResource->bBroken)
				{
					NAT_EraseSocketFromSet(pNetLinkResource->sock, &m_fdReadSet);
					RECVASYN_CALLBACK pfRecvCallBack = pNetLinkResource->pfRecvCallBack;
					if (pfRecvCallBack != NULL)
					{
						pfRecvCallBack(pNetLinkResource->deviceID, pNetLinkResource->pParam, NULL);
					}
					continue;
				}
				//对连接进行可读性检查
//				if(FD_ISSET(pNetLinkResource->sock, &tempReadSet))
                if (NAT_IsSocketInSet(pNetLinkResource->sock, &tempReadSet))
				{
					//接收缓冲区还有空间
					if(pNetLinkResource->recvBuffer.dataSize < pNetLinkResource->recvBuffer.bufferSize)
					{
						ret = RecvBuff(pNetLinkResource->deviceID, \
							pNetLinkResource->recvBuffer.pData+pNetLinkResource->recvBuffer.dataSize, \
							pNetLinkResource->recvBuffer.bufferSize - pNetLinkResource->recvBuffer.dataSize, \
							false, 0);

						if(ret > 0)
						{
							pNetLinkResource->recvBuffer.dataSize += ret;
						}
						else if(ret < 0)
						{
							pNetLinkResource->bBroken = true;
							NAT_EraseSocketFromSet(pNetLinkResource->sock, &m_fdReadSet);

							if(pNetLinkResource->pfRecvCallBack != NULL)
							{
								pNetLinkResource->pfRecvCallBack(pNetLinkResource->deviceID, \
									pNetLinkResource->pParam, NULL);
							}

							break;
						}
						else
						{
							continue;
						}
					}
				}

				if(pNetLinkResource->recvBuffer.dataSize > 0)
				{
					if(pNetLinkResource->pfRecvCallBack != NULL)
					{
						callbackRet = pNetLinkResource->pfRecvCallBack(pNetLinkResource->deviceID, \
							pNetLinkResource->pParam, \
							&pNetLinkResource->recvBuffer);
					}
					else
					{
						callbackRet = 0;
						pNetLinkResource->recvBuffer.dataSize = 0;
					}
				}

				if(pNetLinkResource->recvBuffer.dataSize < pNetLinkResource->recvBuffer.bufferSize)
				{
					//缓存中还有空间接收新的数据,并且这个连接刚接收到了数据,就不需要Sleep
					if(NAT_IsSocketInSet(pNetLinkResource->sock, &tempReadSet))
					{
						bProcessedData = true;
					}
				}
			}

			m_lstLinkLockForCallback.UnLock();

			//在此过程中并没有数据收发，则暂时挂起该线程
			if(!bProcessedData)
			{
#ifdef  WIN32
				PUB_Sleep(3);
#else
				PUB_Sleep(0);
#endif
			}
		}
		else
		{
			//select失败，没有数据可读，检查有没有之前未处理的数据，回调处理
			//对每个socket作读写检查
			m_lstLinkLockForCallback.Lock();
			list<NAT_LINK_RESOURCE *>::iterator it;
			for(it = m_listNetLink.begin(); it != m_listNetLink.end(); it++)
			{
				NAT_LINK_RESOURCE *pNetLinkResource = *it;
				if(pNetLinkResource->recvBuffer.dataSize > 0)
				{
					if(pNetLinkResource->pfRecvCallBack != NULL)
					{
						pNetLinkResource->pfRecvCallBack(pNetLinkResource->deviceID, \
							pNetLinkResource->pParam, \
							&pNetLinkResource->recvBuffer);
					}
					else
					{
						pNetLinkResource->recvBuffer.dataSize = 0;
					}
				}
			}
			m_lstLinkLockForCallback.UnLock();
		}
	}

	//线程执行结束
	printf("CNatMultiNetComm::%s exit. %s, %d\n", __FUNCTION__, __FILE__, __LINE__);
}

bool CNatMultiNetComm::GetLinkResourceByLinkID(long deviceID, NAT_LINK_RESOURCE * &pLinkResource)
{
	list<NAT_LINK_RESOURCE*>::iterator it;
	for(it = m_listNetLink.begin(); it != m_listNetLink.end(); it++)
	{
		NAT_LINK_RESOURCE *pNetLinkResource = *it;
		
		if(pNetLinkResource->deviceID == deviceID)
		{
			pLinkResource = pNetLinkResource;
			return true;
		}
	}

	return false;
}

void CNatMultiNetComm::UpdateRemoveComm()
{
	m_lstLinkLockForCallback.Lock();
	m_lstLinkLock.Lock();

	while(!m_tempDelNetLink.empty())
	{
		NAT_LINK_RESOURCE *pNetLinkResource = m_tempDelNetLink.front();
		m_tempDelNetLink.pop_front();

		list<NAT_LINK_RESOURCE*>::iterator it;
		for(it = m_listNetLink.begin(); it != m_listNetLink.end(); it++)
		{
			if(*it == pNetLinkResource)
			{
				m_listNetLink.erase(it);
 				NAT_EraseSocketFromSet(pNetLinkResource->sock, &m_fdReadSet);
                NAT_CloseSocket(pNetLinkResource->sock);
				if(NULL != pNetLinkResource->recvBuffer.pData)
				{
					delete [] pNetLinkResource->recvBuffer.pData;
				}
				delete pNetLinkResource;
				break;
			}
		}
	}
	
	m_lstLinkLock.UnLock();
	m_lstLinkLockForCallback.UnLock();
}

