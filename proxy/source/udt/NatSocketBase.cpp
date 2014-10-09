// NatSocketBase.cpp: implements for the CNatSocketBase class.
//
//////////////////////////////////////////////////////////////////////

#include "NatCommon.h"
#include "NatSocketBase.h"

#include "NatDebug.h"

////////////////////////////////////////////////////////////////////////////////
//class CNatSocketBase implements

CNatSocketBase::CNatSocketBase( )
:
m_commNotifier(NULL),
m_socketState(STATE_CLOSED),
m_bIsUsing(false)
{

}

CNatSocketBase::~CNatSocketBase()
{

}


bool CNatSocketBase::IsInited()
{
    return GetState() != STATE_CLOSED;
}

bool CNatSocketBase::IsConnected()
{
    return GetState() == CNatSocketBase::STATE_CONNECTED;
}

void CNatSocketBase::SetCommNotifier( CNatSocketCommNotifier* notifier )
{
    CNatScopeLock lock(&m_notifierLock);
    m_commNotifier = notifier;
}

int CNatSocketBase::Close()
{
    m_bIsUsing = false;
    return 0;
}

int CNatSocketBase::NotifyOnRecv( const void* pBuf, int iLen )
{
    CNatScopeLock lock(&m_notifierLock);
    if (m_commNotifier != NULL)
    {
        return m_commNotifier->OnRecv(this, pBuf, iLen);
    }
    return 0;
}

CNatSocketBase::SocketState CNatSocketBase::GetState()
{
    return m_socketState;
}

void CNatSocketBase::ChangeState( SocketState state )
{
    m_socketState = state;
}

void CNatSocketBase::Use()
{
	m_bIsUsing = true;
}

bool CNatSocketBase::IsUsing()
{
	return m_bIsUsing;
}