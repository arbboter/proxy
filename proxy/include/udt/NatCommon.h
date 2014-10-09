// NatCommon.h: interface for the common of nat.
//
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_COMMON_H_
#define _NAT_COMMON_H_

#include "NatUserDefine.h"
#include "stdio.h"

#ifdef   __ENVIRONMENT_WIN32__

#define __printf printf

#endif

#ifdef NAT_TVT_DVR_4_0
#include "TVT_PubCom.h"
#else
#include "PUB_common.h"
#endif
#include "SWL_Public.h"
#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include "NatBaseTypes.h"
#include "NatLogger.h"


#ifdef NAT_TVT_DVR_4_0
#  ifdef __ENVIRONMENT_WIN32__
#    define Nat_GetHostByName gethostbyname
#    define Nat_ReleaseHost(host)
#  else
#    define Nat_GetHostByName GetHostbyname
#    define Nat_ReleaseHost(host) ReleaseHost(host)
#  endif
#else
#  ifdef __ENVIRONMENT_WIN32__
#    define Nat_GetHostByName gethostbyname
#    define Nat_ReleaseHost(host)
#  else
#    include "NetInterface.h"
#    define Nat_GetHostByName CNetInterface::Instance()->GetHostByName
#    define Nat_ReleaseHost(host) CNetInterface::Instance()->ReleaseHost(host)
#  endif
#endif//NAT_TVT_DVR_4_0

bool Nat_ParseIpByName(const char* addr, unsigned long &ip);

unsigned long Nat_GetTickCount();

void Nat_SRand(unsigned long seed);

void Nat_SRand();

int Nat_Rand();

unsigned short PUB_GetRandomPort();

#ifdef __NAT_PUB_IsTimeOutEx__
inline bool PUB_IsTimeOutEx(unsigned long dwStartTime, unsigned long dwDisTime, unsigned long  dwNowTime)
{
	return ((dwNowTime - dwStartTime) >= dwDisTime);
}
#endif

inline bool PUB_UNSIGNED_LONG_LT(unsigned long a, unsigned long b)
{
	return (static_cast<long>(a - b) < 0);
}

inline bool PUB_UNSIGNED_LONG_LEQ(unsigned long a, unsigned long b)
{
	return (static_cast<long>(a - b) <= 0);
}

inline bool PUB_UNSIGNED_LONG_GT(unsigned long a, unsigned long b)
{
	return (static_cast<long>(a - b) > 0);
}

inline bool PUB_UNSIGNED_LONG_GEQ(unsigned long a, unsigned long b)
{
	return (static_cast<long>(a - b) >= 0);
}

char* Nat_inet_ntoa( unsigned long ip);

unsigned long GetRandomConnectionId();

typedef CPUB_Lock CNatLock;

class CNatScopeLock
{
public:
    explicit CNatScopeLock(CNatLock *lock);
    virtual ~CNatScopeLock();
private:
    CNatLock* m_lock;
};

#endif