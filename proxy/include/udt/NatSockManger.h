
#ifndef  __NAT_SOCK_MANAGER__
#define __NAT_SOCK_MANAGER__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <map>
#include "NatSocketBase.h"


//上层应用通过key访问CNatSocketBase,弱化上层和底层之间的关系
class CNatSockManager
{
public:
    CNatSockManager();
    ~CNatSockManager();

    static CNatSockManager* GetSocketManager();
  
    NatSocket Add(CNatSocketBase *pSockBase);

    int Remove(NatSocket sock);

    void Disable(CNatSocketBase *pSockBase);
    
    int CloseSocket(NatSocket sock);

    int GetSocketInfo(NatSocket sock, NAT_SOCKET_INFO &info);

    int Select(NatSocketSet *readSet, NatSocketSet *writeSet, int iTimeout);

    int Send(NatSocket sock, const void *pBuf, int iLen);

    int Recv(NatSocket sock, void *pBuf, int iLen);

    /**
     * 获取当前可以发送数据的字节大小
     * @return 如果没有错误，则返回当前可以发送数据的字节大小，0表示发送缓冲区满了；否则，出错返回-1
     */
    int GetSendAvailable(NatSocket sock);

    /**
     * 获取可以接收数据的字节大小，主要用于主动接收数据
     * @return 如果没有错误，则返回当前可以接收数据的字节大小，0表示无数据；否则，出错返回-1
     */
    int GetRecvAvailable(NatSocket sock);

private:
    CNatSocketBase* FindSock(NatSocket sock);
private:
    typedef std::map<void*, CNatSocketBase*> NatSocketMap;
private:
    CPUB_Lock		  m_lock;
    NatSocketMap      m_sockMap;	

};

#endif//__NAT_SOCK_MANAGER__
