// NatClientPeerManager.cpp: implements for the NatClientPeerManager class.
//
//////////////////////////////////////////////////////////////////////

#include "NatCommon.h"
#include "NatClientPeerManager.h"
#include "NatSockManger.h"
#include "NatDebug.h"

static CNatClientPeerManager*  g_instance=NULL;
static CPUB_Lock m_instanceLock;

CNatClientPeerManager* CNatClientPeerManager::instance()
{
    if (NULL == g_instance)
    {
        m_instanceLock.Lock();
        if (NULL == g_instance)
        {
            g_instance = new CNatClientPeerManager();
        }
        m_instanceLock.UnLock();
    }
    return g_instance;
}

void CNatClientPeerManager::DestroyInstance()
{
    m_instanceLock.Lock();
    if (NULL != g_instance)
    {
        delete g_instance;
        g_instance = NULL;
    }
    m_instanceLock.UnLock();
}

bool CNatClientPeerManager::IsInstanceInited()
{
    return NULL != g_instance;
}

CNatClientPeerManager::CNatClientPeerManager()
: m_workThreadRunning(false)
{
    m_workThreadID = PUB_CreateThread(WorkThreadFunc, (void *)this, &m_workThreadRunning);	
}

CNatClientPeerManager::~CNatClientPeerManager()
{
    if (PUB_THREAD_ID_NOINIT != m_workThreadID) 
    {		
        PUB_ExitThread(&m_workThreadID, &m_workThreadRunning);
    }	

    if (!m_clientSockets.empty())
    {
        // TODO: print debug: has some nat sockets leak
        CNatScopeLock lock(&m_socketsLock);
        std::vector<CNatSocketBase*>::iterator i;
        for (i = m_clientSockets.begin(); i != m_clientSockets.end(); i++)
        {
             CNatSocketBase *pNatSock = *i;
             if (pNatSock->IsUsing())
             {
                 CNatSockManager::GetSocketManager()->Disable(pNatSock);
             }
             delete *i;
        }
        m_clientSockets.clear();
    }

}

NatSocket CNatClientPeerManager::AddConnectedSocket( CNatSocketBase* sockBase )
{
    NatSocket sock;
    sock = CNatSockManager::GetSocketManager()->Add(sockBase);
    if (sock != NAT_SOCKET_INVALID)
    {
        sockBase->Use();
        CNatScopeLock lock(&m_socketsLock);
        m_clientSockets.push_back(sockBase);
    }
    return sock;
}

int CNatClientPeerManager::GetSocketCount()
{
    return (int)m_clientSockets.size();
}

CNatSocketBase* CNatClientPeerManager::GetSocket( int index )
{
    return m_clientSockets[index];
}

PUB_THREAD_RESULT PUB_THREAD_CALL CNatClientPeerManager::WorkThreadFunc( void *pParam )
{
#ifdef	__ENVIRONMENT_LINUX__
    NAT_LOG_INFO("CNatClientPeerManager thread pid = %d\n",getpid());
#endif
    assert(NULL != pParam);
    CNatClientPeerManager *manager = reinterpret_cast<CNatClientPeerManager*>(pParam);
    manager->RunWork();

    return 0;
}

int CNatClientPeerManager::RunWork()
{
    bool needRunMore = false;
    NatRunResult runResult;
    std::vector<CNatSocketBase*>::iterator it;
    
    while (m_workThreadRunning)
    {
        needRunMore = false;
        runResult = ClearClosedSockets();
        if (runResult == RUN_DO_MORE)
        {
            needRunMore = true;
        }

        // run all clients
        if (!m_clientSockets.empty())
        {
            CNatScopeLock lock(&m_socketsLock);
            for (it = m_clientSockets.begin(); it != m_clientSockets.end(); it++)
            {
                CNatSocketBase* client = *it;
                runResult = client->Run();
                if (runResult == RUN_DO_MORE)
                {
                    needRunMore = true;
                }
            }
        }


        if (!needRunMore)
        {
            PUB_Sleep(1); // TODO: 10ms ?
        }

    }
    return 0;
}

NatRunResult CNatClientPeerManager::ClearClosedSockets()
{
    if (!m_clientSockets.empty())
    {
        std::vector<CNatSocketBase*>::iterator it;
        CNatScopeLock lock(&m_socketsLock);
        for (it = m_clientSockets.begin(); it != m_clientSockets.end(); )
        {
            CNatSocketBase* client = *it;
            if (!client->IsUsing() && (client->GetState() != CNatSocketBase::STATE_INIT) && (client->GetState() != CNatSocketBase::STATE_CONNECTING))
            {
                it = m_clientSockets.erase(it);
                delete client;
            }
            else
            {
                it++;
            }
        }
    }

    return RUN_NONE;
}
