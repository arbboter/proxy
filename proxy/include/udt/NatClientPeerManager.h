// NatClientPeerManager.h: interface for the CNatClientPeerManager class.
//
//////////////////////////////////////////////////////////////////////

#include "NatSocketBase.h"
#include <vector>

#ifndef _NAT_CLIENT_PEER_MANAGER_H_
#define _NAT_CLIENT_PEER_MANAGER_H_


class CNatClientPeerManager
{
public:
    static CNatClientPeerManager* instance();
    static void DestroyInstance();
    static bool IsInstanceInited();
private:
//     static CNatClientPeerManager* g_instance;
//     static CPUB_Lock m_instanceLock;
public:
    virtual ~CNatClientPeerManager();

    NatSocket AddConnectedSocket(CNatSocketBase* sockBase);

    int GetSocketCount();

    CNatSocketBase* GetSocket(int index);

private:
    CNatClientPeerManager();
private:
    static PUB_THREAD_RESULT PUB_THREAD_CALL WorkThreadFunc(void *pParam);
    int RunWork();
private:
    NatRunResult ClearClosedSockets();
private:
    std::vector<CNatSocketBase*> m_clientSockets;
    bool m_workThreadRunning;
    PUB_thread_t m_workThreadID;
    CPUB_Lock m_socketsLock;
};

#endif//_NAT_CLIENT_PEER_MANAGER_H_