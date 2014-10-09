// NatDevicePeer.cpp: implements for the CNatUdt class.
//
//////////////////////////////////////////////////////////////////////

#include "NatCommon.h"

#include "NatDebug.h"


bool Nat_ParseIpByName(const char* addr, unsigned long &ip)
{
    ip = inet_addr(addr);
    if (INADDR_NONE != ip)
    {
        return true;
    }
    struct hostent *pServerHost = Nat_GetHostByName(addr);
    if (NULL != pServerHost)
    {
        struct in_addr *theAddr = (struct in_addr *)(pServerHost->h_addr);
        ip = theAddr->s_addr;
        Nat_ReleaseHost(pServerHost);
        return true;	
    }

    return false; 
}

#ifdef	__ENVIRONMENT_LINUX__
unsigned long Nat_GetTickCount()
{
    static CPUB_Lock s_lock;
    s_lock.Lock();
    unsigned long dwTick = 0;
    struct timespec tp;

    clock_gettime(CLOCK_MONOTONIC, &tp);
    dwTick = tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
    while(0 == dwTick)
    {
        clock_gettime(CLOCK_MONOTONIC, &tp);
        dwTick = tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
    }

    s_lock.UnLock();
    return dwTick;
}
#endif

#ifdef __ENVIRONMENT_WIN32__
unsigned long Nat_GetTickCount()
{
    unsigned long dwTick = GetTickCount();
    while(0 == dwTick)
    {		
        dwTick = GetTickCount();
    }
    return dwTick;
}
#endif


static CPUB_Lock	 s_natRandLock;

void Nat_SRand( unsigned long seed )
{
    s_natRandLock.Lock();
    srand (seed);	
    s_natRandLock.UnLock();
}

void Nat_SRand()
{
    Nat_SRand(Nat_GetTickCount());
}

int Nat_Rand()
{
    s_natRandLock.Lock();
    int ret = rand ();
    s_natRandLock.UnLock();
    return ret;
}


unsigned long GetRandomConnectionId()
{
	return Nat_GetTickCount();
}
char* Nat_inet_ntoa( unsigned long ip )
{
    struct in_addr tempAddr;
    tempAddr.s_addr = ip;
    return inet_ntoa(tempAddr);
}


CNatScopeLock::CNatScopeLock( CNatLock *lock )
:
m_lock(lock)
{
    m_lock->Lock();
}

CNatScopeLock::~CNatScopeLock()
{
    m_lock->UnLock();
}