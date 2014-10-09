// CNatClientUdtSocket.cpp: implements for the CNatClientUdtSocket class.
//
//////////////////////////////////////////////////////////////////////

#include "NatCommon.h"
#include "NatClientUdtSocket.h"
#include "NatDebug.h"

/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//class CNatUdtClientPeer implements
CNatClientUdtSocket::CNatClientUdtSocket(CNatUdt *pUdt,CNatUdtTransport *pUdtTransport)
:
m_udt(pUdt),
m_pUdtTransport(pUdtTransport),
CNatSocketBase()
{
	//获取UdtTransport的控制权
	assert(NULL != m_udt);
	assert(NULL != pUdtTransport);
	m_pUdtTransport->SetNotifier(this);
	m_udt->SetNotifier(this);
	ChangeState(STATE_CONNECTED);
}

CNatClientUdtSocket::~CNatClientUdtSocket()
{
    InternalClose();
}

NatRunResult CNatClientUdtSocket::Run()
{
	NatRunResult runResult = RUN_NONE;
	assert(m_pUdtTransport->IsStarted());
	if(IsConnected())
	{
		runResult = NatRunResultMax(runResult, m_udt->Run());
		if (RUN_DO_FAILED == runResult)
		{
			ChangeState(CNatSocketBase::STATE_DISCONNECTED);
			return RUN_DO_FAILED;
		}

		runResult = NatRunResultMax(runResult, m_pUdtTransport->Run());
		if (RUN_DO_FAILED == runResult)
		{
			ChangeState(CNatSocketBase::STATE_DISCONNECTED);
			return RUN_DO_FAILED;
		}
	}
	return runResult;
}

int CNatClientUdtSocket::Send( const void *pBuf, int iLen)
{
	int ret = -1;
	if (IsConnected())
	{
		ret = m_udt->Send(pBuf, iLen);
		if(ret < 0)
		{
			ChangeState(CNatSocketBase::STATE_DISCONNECTED);
		}
	}
	return ret;
}

int CNatClientUdtSocket::Recv( void *pBuf, int iLen )
{
	int ret = -1;
	if (IsConnected())
	{
		ret =m_udt->Recv(pBuf, iLen);
		if(ret < 0)
		{
			ChangeState(CNatSocketBase::STATE_DISCONNECTED);
		}
	}
	return ret;
}

int CNatClientUdtSocket::GetSendAvailable()
{
	if(IsConnected())
	{
		return m_udt->GetSendAvailable();
	}
	return -1;
}

int CNatClientUdtSocket::GetRecvAvailable()
{
	if(IsConnected())
	{
		return m_udt->GetRecvAvailable();
	}
	return -1;
}

int CNatClientUdtSocket::SetAutoRecv( bool bAutoRecv )
{
	if(IsConnected())
	{
		m_udt->SetAutoRecv(bAutoRecv);
	}
	return 0;
}

unsigned long CNatClientUdtSocket::GetRemoteIp()
{
	if(IsConnected())
	{
		return m_udt->GetRemoteIp();
	}
	return 0;
}

unsigned short CNatClientUdtSocket::GetRemotePort()
{
	if(IsConnected())
	{
		return m_udt->GetRemotePort();
	}
	return 0;	
}


void CNatClientUdtSocket::OnRecvDatagram( CNatUdtTransport *transport, const NAT_TRANSPORT_RECV_DATAGRAM* datagram)
{
    if(IsConnected())
    {
        if(m_udt != NULL)
        {
            if (m_udt->IsMineDatagram(datagram))
            {
               if(m_udt->NotifyRecvDatagram(datagram) < 0)
               {
                   ChangeState(CNatSocketBase::STATE_DISCONNECTED);
               }
            }
            else
            {

            }
        }
    }
}

int CNatClientUdtSocket::Close()
{
	return CNatSocketBase::Close();    
}

void CNatClientUdtSocket::OnConnect(CNatUdt *pUDT, int iErrorCode)
{

}

int CNatClientUdtSocket::OnRecv( CNatUdt *pUDT, const void* pBuf, int iLen )
{
	return NotifyOnRecv(pBuf, iLen);
}

int CNatClientUdtSocket::OnSendPacket( CNatUdt *pUDT, const void* pBuf, int iLen )
{
	return m_pUdtTransport->Send(m_udt->GetRemoteIp(), m_udt->GetRemotePort(),pBuf, iLen);
}

void CNatClientUdtSocket::InternalClose()
{
	if (m_udt != NULL)
	{
		m_udt->Stop();
		delete m_udt;
		m_udt = NULL;
    }

	if (m_pUdtTransport != NULL)
	{
		assert(m_pUdtTransport->IsStarted());	
		m_pUdtTransport->Stop();
		delete m_pUdtTransport;
		m_pUdtTransport = NULL;
	}
}