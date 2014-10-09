#include "NatCommon.h"
#include <string.h>

#ifdef   __ENVIRONMENT_LINUX__
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#endif

#include "NatSockManger.h"
#include "NatSocket.h"

#include "NatDebug.h"

CNatSockManager::CNatSockManager()
{
}

CNatSockManager::~CNatSockManager()
{
    if (!m_sockMap.empty())
    {
        NAT_LOG_WARN("There are some NatSockets forgot to remove!\n");
        NatSocketMap::iterator it;
        for(it=m_sockMap.begin();it!=m_sockMap.end();it++)
        {
            CNatSocketBase *sockBase = it->second;
            if (sockBase != NULL)
            {
                sockBase->Close();
            }
            int *sockKey = (int*)it->first;
            delete sockKey;
        }
    }
   m_sockMap.clear();
}

CNatSockManager* CNatSockManager::GetSocketManager()
{
    static CNatSockManager s_NatSockManager;
    return &s_NatSockManager;
}
NatSocket CNatSockManager::Add(CNatSocketBase *pSockBase)
{
    if(pSockBase == NULL)
    {
        return NAT_SOCKET_INVALID;
    }

    NatSocket sock = (NatSocket)(new int);
    if(sock == NAT_SOCKET_INVALID)
    {
        return NAT_SOCKET_INVALID;
    }

    CNatScopeLock lock(&m_lock);
    m_sockMap.insert(NatSocketMap::value_type(sock, pSockBase));
   return sock;
}

int CNatSockManager::Remove(NatSocket sock)
{
    assert(NAT_SOCKET_INVALID != sock);
    int ret = 0;
    CNatScopeLock lock(&m_lock);

    NatSocketMap::iterator it = m_sockMap.find(sock);

    if(it != m_sockMap.end())
    {
        CNatSocketBase *sockBase = it->second;
        if (sockBase != NULL)
        {
            sockBase->Close();
        }
        int *sockKey = (int*)it->first;
        delete sockKey;
        m_sockMap.erase(it);
    }else
    {
        ret  = -1;
    }
    return ret;
}

void CNatSockManager::Disable(CNatSocketBase *pSockBase)
{
    if(pSockBase == NULL)
    {
        return;
    }

    CNatScopeLock lock(&m_lock);

    NatSocketMap::iterator it;
    for(it=m_sockMap.begin();it!=m_sockMap.end();it++)
    {
        if(it->second == pSockBase)
        {
            it->second = NULL;
           break;
        }
    }
    return;
}


int CNatSockManager::CloseSocket(NatSocket sock)
{
    if (sock != NAT_SOCKET_INVALID)
    {
        CNatScopeLock lock(&m_lock);
        CNatSockManager::GetSocketManager()->Remove(sock);
        return 0;
    }
    return -1;
}

int CNatSockManager::GetSocketInfo( NatSocket sock, NAT_SOCKET_INFO &info )
{
    if (sock != NAT_SOCKET_INVALID)
    {
        CNatScopeLock lock(&m_lock);
        CNatSocketBase* socketBase = FindSock(sock);
        if(socketBase != NULL && socketBase->IsConnected())
        {
            info.remoteIp = socketBase->GetRemoteIp();
            info.remotePort = socketBase->GetRemotePort();
            return 0;
        }
    }
    return -1;
}

int CNatSockManager::Send(NatSocket sock, const void *pBuf, int iLen)
{
    int ret = -1;
    if (sock != NAT_SOCKET_INVALID)
    {
        CNatScopeLock lock(&m_lock);
        CNatSocketBase* socketBase = FindSock(sock);
        if(socketBase != NULL)
        {
            ret = socketBase->Send(pBuf, iLen);
        }
    }
    return ret;
}

int CNatSockManager::Recv(NatSocket sock, void *pBuf, int iLen)
{
    int ret = -1;
    if (sock != NAT_SOCKET_INVALID)
    {
        CNatScopeLock lock(&m_lock);
        CNatSocketBase* socketBase = FindSock(sock);
        if(socketBase != NULL)
        {
            ret = socketBase->Recv(pBuf, iLen);
        }
    }
    return ret;
}

int CNatSockManager::GetSendAvailable(NatSocket sock)
{
    int ret = -1;
    if (sock != NAT_SOCKET_INVALID)
    {
        CNatScopeLock lock(&m_lock);
        CNatSocketBase* socketBase = FindSock(sock);
        if(socketBase != NULL)
        {
            ret = socketBase->GetSendAvailable();
        }
    }
    return ret;
}

int CNatSockManager::GetRecvAvailable(NatSocket sock)
{
    int ret = -1;
    if (sock != NAT_SOCKET_INVALID)
    {
        CNatScopeLock lock(&m_lock);
        CNatSocketBase* socketBase = FindSock(sock);
        if(socketBase != NULL)
        {
            ret = socketBase->GetRecvAvailable();
        }
    }
    return ret;
}

CNatSocketBase *CNatSockManager::FindSock(NatSocket sock)
{
    CNatSocketBase *pSockBase = NULL;
    NatSocketMap::iterator it;
    it = m_sockMap.find(sock);
    if(it != m_sockMap.end())
    {
       pSockBase =(*it).second;
    }
    return pSockBase;
}
