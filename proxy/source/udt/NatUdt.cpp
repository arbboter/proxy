// NatDevicePeer.cpp: implements for the CNatUdt class.
//
//////////////////////////////////////////////////////////////////////


#ifdef TEST_PROFILE
#include "Profile.h"
#endif

#include "NatCommon.h"
#include "NatUdt.h"
#include <assert.h>

#include "NatDebug.h"
bool g_bool=0;


bool CNatUdt::IsDatagramValid(const NAT_TRANSPORT_RECV_DATAGRAM* datagram)
{
	return datagram != NULL && datagram->dataSize >= sizeof(UDP_WRAPPER_HEAD) && datagram->dataSize <= MAX_WRAPPER_DATA_BUFFSIZE;
}

byte CNatUdt::GetDatagramCategory( const NAT_TRANSPORT_RECV_DATAGRAM* datagram )
{
    assert(datagram != NULL);
	if (IsDatagramValid(datagram))
	{
		return (reinterpret_cast<const UDP_WRAPPER_HEAD *>(datagram->data))->category;
	}
	return NAT_UDT_CATEGORY_INVALID;
}

uint32 CNatUdt::GetDatagramConnectionId(const NAT_TRANSPORT_RECV_DATAGRAM* datagram)
{
    assert(datagram != NULL);
	if (IsDatagramValid(datagram))
	{
		return (reinterpret_cast<const UDP_WRAPPER_HEAD *>(datagram->data))->connectionID;
	}
	return 0;
}

bool CNatUdt::IsDatagramSynCmd(const NAT_TRANSPORT_RECV_DATAGRAM* datagram)
{
    assert(datagram != NULL);

	if (IsDatagramValid(datagram))
	{
		return (reinterpret_cast<const UDP_WRAPPER_HEAD *>(datagram->data))->cmdId == UDP_DATA_TYPE_SYN;
	}
	return false;
}

CNatUdt::CNatUdt()
:
m_pSendDataManager(NULL),
m_pOwnedSendDataManager(NULL),
m_pRecvDataManager(NULL),
m_pOwnedRecvDataManager(NULL),
m_curTickCount(0)
{
	m_bStarted = false;
	m_UDP_TIMEOUT = UDP_TIMEOUT;
	m_UDP_MAX_SEND_INTERVAL = UDT_HEARTBEAT_INTERVAL;
}


CNatUdt::~CNatUdt()
{

}



uint32 CNatUdt::GetConnectionID()
{
	return m_RecordedInfo.initSendSeq;;
}

bool CNatUdt::Start(const NAT_UDT_CONFIG* config)
{
	if (m_bStarted)
	{
		return true;
	}
	m_bStarted = true;
	m_curTickCount = Nat_GetTickCount();

	m_Category = config->category;
	m_RemoteInfo.ip = config->remoteIp;
	m_RemoteInfo.port = config->remotePort;
	struct in_addr tmp_Addr;
	tmp_Addr.s_addr = config->remoteIp;
	memset(m_RemoteInfo.Ip, 0, sizeof(m_RemoteInfo.Ip));
	strcpy(m_RemoteInfo.Ip, inet_ntoa(tmp_Addr));
	m_pUdtNotifier = NULL;

    assert(m_pSendDataManager == NULL && m_pOwnedSendDataManager == NULL);
	if (NULL == config->pSendHeapDataManeger)
	{
		m_pOwnedSendDataManager = new CNatHeapDataManager(SEND_PACKET_BUF_SIZE, config->maxSendWindowSize);
		if (NULL == m_pOwnedSendDataManager)
		{
			NAT_LOG_WARN("CNatUdt create send data manager failed!\n");
			return false;
		}
		m_pSendDataManager = m_pOwnedSendDataManager;
	}
	else
	{
		m_pSendDataManager = config->pSendHeapDataManeger;
    }


    assert(m_pRecvDataManager == NULL && m_pOwnedRecvDataManager == NULL);
    if (NULL == config->pRecvHeapDataManeger)
    {
        m_pOwnedRecvDataManager = new CNatHeapDataManager(RECV_PACKET_BUF_SIZE, config->maxRecvWindowSize);
        if (NULL == m_pOwnedRecvDataManager)
        {
            NAT_LOG_ERROR("CNatUdt Create recv data manager failed!\n");
            return false;
        }
        m_pRecvDataManager = m_pOwnedRecvDataManager;
    }
    else
    {
        m_pRecvDataManager = config->pRecvHeapDataManeger;
    }
    m_dwMaxRecvReliableBuffCount = config->maxRecvWindowSize;

	m_RSendInfo.remainHeapPacketNum = MAX_SEND_BUFF_PACKET_COUNT;
	m_RSendInfo.remainedBuffSize = MAX_SEND_RELIABLE_BUFFSIZE;

	memset(&m_RecordedInfo, 0, sizeof(UDT_RECORDED_INFO));
	memset(&m_Statistic, 0, sizeof(UDT_STATISTIC));
	memset(&m_CtrlAlg, 0, sizeof(UDT_CTRL_ALG));

	m_LastAckTime = 0;
	m_bIsAutoRecv = false;

	//RTT RTO计算相关的初始化
	m_sa = MIN_RTT_TIME << 3;
	m_sv = 0;
	m_dwRTT = MIN_RTT_TIME;
	m_dwRTO = 2 * m_dwRTT;	//这个值只影响第一次的超时时间
	m_CtrlAlg.sendMode = SWL_UDP_SENDMODE_SLOW_STARTUP;

	m_RecordedInfo.ackStatus = SWL_UDP_ACK_STATUS_NONE;

	m_iUsed = 0;
	m_iCwnd = 1;
	m_iCrowdedNumerator = 0;
	m_iSsthresh = 0x7fffffff;

	m_LastResendTime = 0;
	m_LastRemoveUnAckPackTime = 0;
	//连接之前的初始化
	m_RecordedInfo.initSendSeq = config->connectionID;
	m_RecordedInfo.initPeerSendSeq = 0;
	m_RecordedInfo.lastRecvTick = m_curTickCount;
	m_RecordedInfo.lastSendTick = 0;
	m_RecordedInfo.lastSendIndex = m_RecordedInfo.initSendSeq - 1;
	m_RecordedInfo.maxPeerRecvIndex = m_RecordedInfo.initSendSeq - 1;
	m_RecordedInfo.state = SWL_UDP_STATE_UNCONNECTED;
	m_RSendInfo.nextSeq = m_RecordedInfo.initSendSeq + 1;
	m_RRecvInfo.expectedSeq = 0;
	m_RRecvInfo.lastOnRecvPackPos = 0;
	m_RRecvInfo.maxPeerAck = 0;
	m_RRecvInfo.minSeq = 0;

	NAT_LOG_DEBUG("[category: %d Peer Ip:%s  port:%d  ConnectionID:%lu]  CNatUdt::Start\n", m_Category, m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID());
	return true;
}

void CNatUdt::Stop()
{
	if (!m_bStarted)
	{
		return ;
	}
	m_bStarted = false;

	//TODO 发送RST，但是由于上层会ChangeDisconnected，导致RST发布出去，待解决
	Disconnect();

#ifndef UDT_WITHOUT_LOCK
	m_RSendInfoLock.Lock();
#endif
	if (m_pOwnedSendDataManager != NULL)
	{
		delete m_pOwnedSendDataManager;
        m_pOwnedSendDataManager = NULL;
    }
    m_pSendDataManager = NULL;
	m_RSendInfo.listReliableSendData.clear();
	m_RSendInfo.listUnackedData.clear();
#ifndef UDT_WITHOUT_LOCK
	m_RSendInfoLock.UnLock();
#endif


#ifndef UDT_WITHOUT_LOCK
	m_RecvLock.Lock();
#endif


	NatUdtRecvDataList::iterator it;
	for (it = m_RRecvInfo.recvDataList.begin(); it!=m_RRecvInfo.recvDataList.end(); ++it)
	{
		DestroyRecvData((*it).second);
	}
	m_RRecvInfo.recvDataList.clear();

    if (m_pOwnedRecvDataManager != NULL)
    {
        delete m_pOwnedRecvDataManager;
        m_pOwnedRecvDataManager = NULL;
    }
    m_pRecvDataManager = NULL;
#ifndef UDT_WITHOUT_LOCK
	m_RecvLock.UnLock();
#endif

#ifdef TEST_PROFILE
	PrintProfileResult();
	NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu]  Real Resend Times %lu\n", m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID(),  m_Statistic.reSendPacketNum);
#endif
	NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu]  NatStop\n", m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID());
}

NatRunResult CNatUdt:: Run()
{
#ifdef TEST_PROFILE
static FunctionProfile __funcProfile(__FUNCTION__);
ProfilerStackFrame __funcFrame(&__funcProfile);
#endif

	if (!m_bStarted)
	{
		return RUN_NONE;
	}

	m_curTickCount = Nat_GetTickCount();
	NatRunResult ret = RUN_NONE;
	int	iRet = 0;
	bool bSendAck = false;
	do 
	{
		if(-1 == CheckConnect(m_curTickCount))
		{		
			ret = RUN_DO_FAILED;
			break;
		}

#ifndef UDT_WITHOUT_LOCK
		m_RSendInfoLock.Lock();
#endif
		if (((int)m_RSendInfo.listUnackedData.size() > m_iUsed) || PUB_UNSIGNED_LONG_GT(m_curTickCount - 5, m_LastResendTime))
		{
			m_LastResendTime = m_curTickCount;
			do 
			{
				iRet = ResendData(m_curTickCount);
			} while(iRet == 1);
		}
#ifndef UDT_WITHOUT_LOCK
		m_RSendInfoLock.UnLock();
#endif
		if ((-4 == iRet) || (-6 == iRet))
		{
			//如果符合这几个条件，就不用发送新数据了
			//这里啥都不做，到后面去发送确认数据

			//ret = 
		}
		else if ((-2 == iRet) || (-5 == iRet))
		{
			//如果符合这几个条件，就不用发送新数据了
			//也不用发确认了
			
			//ret = 
			break;
		}
		else if (-1 == iRet)
		{
			ret = RUN_DO_FAILED;
			break;
		}
		else if(0 == iRet)
		{
			//没有数据重传就发送新的数据
			do 
			{
				iRet = SendReliableData(m_curTickCount);
				if (-1 == iRet)
				{
					ret = RUN_DO_FAILED;
					break;
				}
				else if(1 == iRet)
				{
					
				}
				else
				{

				}
				ret = RUN_NONE;
			} while(iRet == 1);
		}

		//有乱序包 或者 确认时间超时，则回复确认	
		if(PUB_IsTimeOutEx(m_LastAckTime, SEND_ACK_INTERVAL, m_curTickCount))
		{
			//如果SEND_ACK_INTERVAL毫秒没有发送过一次ack，则检查是否有需要ack
			//的数据，如果有需要ack的则回复ack

			//不管是否有需要ack的数据都重新给dwAckTimer赋值，本次都重新赋值
			//因为如果
			//（1）没有数据需要ack，那么再过SEND_ACK_INTERVAL后再检查是否需要ack，
			//（2）如果有数据需要ack，后面m_RecordedInfo.ackStatus的状态不为SWL_UDP_ACK_STATUS_NONE，就会发送ack			
			m_LastAckTime = m_curTickCount;

			//判断是否有需要ack的数据
			if (SWL_UDP_ACK_STATUS_NONE != m_RecordedInfo.ackStatus)
			{				
				bSendAck = true;
			}
		}
		else
		{
			if((SWL_UDP_ACK_STATUS_NOW == m_RecordedInfo.ackStatus) 
				|| (SWL_UDP_ACK_STATUS_DISORDER == m_RecordedInfo.ackStatus))
			{
				//现在ack了，下次连续SEND_ACK_INTERVAL毫秒没有ack再检查是否有需要ack的数据
				m_LastAckTime = m_curTickCount;
				bSendAck = true;
			}
		}

		//回复确认
		if (bSendAck)
		{
			iRet = SendAckData(m_curTickCount);
			if(iRet == -1)
			{				
				ret = RUN_DO_FAILED;
			}
			else if(iRet == -2)
			{
				ret = RUN_NONE;
			}
			else
			{
				ret = RUN_DO_MORE;
			}
		}

	} while(0);
	
	if (ret == RUN_DO_FAILED)
	{
		NAT_LOG_DEBUG("CNatUdt Run failed!\n");
		Disconnect();
	}

	return ret; 
}

void CNatUdt::SetNotifier(CUdtNotifier *pUDTInterface)
{
	m_pUdtNotifier = pUDTInterface;
}

int CNatUdt::RecvPacket(const UDP_WRAPPER_DATA *packet, int packetSize)
{
	if (!m_bStarted)
	{
		NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu]  OnRecvPacke is invalid(udt not start)\n", m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID());
		return -1;
	}

	if (!packet)
	{
		NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu]  CNatUdt::OnRecvPacke\n", m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID());
		return -1;
	}

#ifdef TEST_PROFILE
	static FunctionProfile __funcProfile(__FUNCTION__);
	ProfilerStackFrame __funcFrame(&__funcProfile);
#endif

	assert(packetSize <= MAX_WRAPPER_DATA_BUFFSIZE);

	if (m_RecordedInfo.state != SWL_UDP_STATE_CONNECTED && packet->head.cmdId != UDP_DATA_TYPE_SYN)
	{
		NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu]  cmdId err:%d\n", m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID(), packet->head.cmdId);
		return 0;
	}

	assert(packet->head.connectionID == GetConnectionID());

	int ret = 0;
	uint32 currTick = m_curTickCount;
	switch(packet->head.cmdId)
	{
	case UDP_DATA_TYPE_SYN:
		ret = HandleRecvConnect(packet, packetSize, currTick);
		break;

	case UDP_DATA_TYPE_RELIABLE:
		m_Statistic.totalRecvPacketNum++;
		m_Statistic.totalRecvBytes += packetSize;
		ret = HandleRecvReliableData(packet, packetSize, currTick);
		m_RecordedInfo.lastRecvTick = currTick;
		break;

	case UDP_DATA_TYPE_ACK:
		ret = HandleRecvAckData(packet, packetSize, currTick);
		m_RecordedInfo.lastRecvTick = currTick;
		break;

	case UDP_DATA_TYPE_RST:
		HandleRecvRST(packet, packetSize, currTick);
		ret = -1;
		break;
	default:
		NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu]  cmdId err:%d\n", m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID(), packet->head.cmdId);
		ret = -1;
		break;
	}

	if (ret == -1)
	{		
		Disconnect();
	}

	return ret;
}

int CNatUdt::Send(const void *pData, int iLen)
{
#ifdef TEST_PROFILE
	static FunctionProfile __funcProfile(__FUNCTION__);
ProfilerStackFrame __funcFrame(&__funcProfile);
#endif

	UDP_SEND_PACKET_DATA	*pPacket = NULL;
	int totalLen = iLen;

	if (m_RecordedInfo.state != SWL_UDP_STATE_CONNECTED)
	{
		return -1;
	}
#ifndef UDT_WITHOUT_LOCK
	m_RSendInfoLock.Lock();
#endif
	if(m_RSendInfo.remainedBuffSize < iLen || m_RSendInfo.remainHeapPacketNum < 1)
	{
#ifndef UDT_WITHOUT_LOCK
		m_RSendInfoLock.UnLock();
#endif
		return 0;
	}
	
	while(iLen > 0)
	{
		pPacket = static_cast<UDP_SEND_PACKET_DATA*>(m_pSendDataManager->GetMemory());
		if (!pPacket)
		{
            //NAT_LOG_DEBUG("NatUdt Warning: Send Data failed! Send Heap Data is used up!\n");
			break;
		}
		m_RSendInfo.remainHeapPacketNum--;
		pPacket->wrapperData.head.seqNum = m_RSendInfo.nextSeq++;
#ifndef UDT_WITHOUT_LOCK
		m_RSendInfoLock.UnLock();
#endif

		pPacket->wrapperData.head.category = m_Category;
		pPacket->wrapperData.head.cmdId = UDP_DATA_TYPE_RELIABLE;
		pPacket->wrapperData.head.maxPacketLen = MAX_WRAPPER_DATA_LEN;		
		

		pPacket->lastSendTick = 0;
		pPacket->dataStatus = SWL_UDP_DATA_STATUS_NOSEND;	
		pPacket->iLen = (iLen > static_cast<int32>(WRAPPER_VALIDATA_LEN)) ? WRAPPER_VALIDATA_LEN : iLen;
		iLen -= pPacket->iLen;
		memcpy(&(pPacket->wrapperData.wrapper_dat), pData, pPacket->iLen);
		pData = (const char*)pData + pPacket->iLen;		
#ifndef UDT_WITHOUT_LOCK
		m_RSendInfoLock.Lock();
#endif
		m_RSendInfo.listReliableSendData.push_back(pPacket);	
	}

	m_RSendInfo.remainedBuffSize -= (totalLen - iLen);
#ifndef UDT_WITHOUT_LOCK
	m_RSendInfoLock.UnLock();
#endif
	return totalLen - iLen;
}

int CNatUdt::CheckConnect( uint32 currentTick )
{
	if (PUB_IsTimeOutEx(m_RecordedInfo.lastRecvTick, m_UDP_TIMEOUT, currentTick))
	{	
		NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu]  UDP_TIMEOUT\n", m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID());        
		return -1;
	}

	if(SWL_UDP_STATE_CONNECTED != m_RecordedInfo.state)
	{		
		if ((0 == m_RecordedInfo.lastSendTick) 
			|| PUB_IsTimeOutEx(m_RecordedInfo.lastSendTick, UDP_CONNECT_INTERVAL, currentTick))
		{
			UDP_SYN_PACKET synPacket;
			synPacket.head.category = m_Category;
			synPacket.head.cmdId = UDP_DATA_TYPE_SYN;
			synPacket.head.maxPacketLen = MAX_WRAPPER_DATA_LEN;
			synPacket.head.seqNum = m_RecordedInfo.initSendSeq;
			synPacket.head.connectionID = GetConnectionID();
			synPacket.data.protocolTag = NAT_UDT_PROTOCOL_TAG;
			//synPacket.data.version

			if(SWL_UDP_STATE_RECVED == m_RecordedInfo.state)
			{
				synPacket.head.ackNum = m_RecordedInfo.initPeerSendSeq;
			}
			else if(SWL_UDP_STATE_UNCONNECTED == m_RecordedInfo.state)
			{
				synPacket.head.ackNum = 0;
			}
			else
			{
				assert(false);
				return -1;
			}
			//更新最后一次发送时间
			m_RecordedInfo.lastSendTick = currentTick;
			OnNotifySendPacket(&synPacket, sizeof(UDP_SYN_PACKET));
		}
	}
	else
	{
		if (PUB_IsTimeOutEx(m_RecordedInfo.lastSendTick, m_UDP_MAX_SEND_INTERVAL, currentTick))
		{
			SendAckData(currentTick);		
		}	
	}

	return 0;
}

int CNatUdt::SendAckData( uint32 currentTick )
{
	int ret = -1;

	//m_SendTmpData.head.seqNum = m_RSendInfo.nextSeq - 1;	//liujiang 这个值对方没有用
	m_SendTmpData.head.category = m_Category;
	m_SendTmpData.head.cmdId = UDP_DATA_TYPE_ACK;
	m_SendTmpData.head.maxPacketLen = MAX_WRAPPER_DATA_LEN;
	m_SendTmpData.head.sendIndex = m_RecordedInfo.lastSendIndex;
	m_SendTmpData.head.recvIndex = m_RecordedInfo.maxRecvIndex;
	m_SendTmpData.head.ackNum = m_RRecvInfo.expectedSeq - 1;
	m_SendTmpData.head.connectionID = GetConnectionID();		

	std::list<uint32>::iterator it;			
	uint32 i = 0;	
	//乱序包只回复一次
	for (i = 0, it = m_listDisorderlyPacket.begin(); 
		it != m_listDisorderlyPacket.end() && (i < MAX_ACK_COUNT);
		it = m_listDisorderlyPacket.begin(), ++i)
	{		
		m_SendTmpData.wrapper_dat.acks[i].ackSeq = (*it);		
		m_listDisorderlyPacket.pop_front();
	}

	int iLen = sizeof(UDP_WRAPPER_HEAD) + sizeof(uint32) * i;


	ret = OnNotifySendPacket(&m_SendTmpData, iLen);
	if (0 == ret)
	{
		//更新最后一次发送时间
		m_RecordedInfo.lastSendTick = currentTick;
		m_RecordedInfo.ackStatus = SWL_UDP_ACK_STATUS_NONE;
	}
	else if(-1 == ret)
	{

	}
	else//socket blocked
	{
		ret = -2;
	}
	

	return ret;
}

int CNatUdt::ReleaseReliableData( uint32 seq, uint32 index, uint32 currentTick )
{
#ifdef TEST_PROFILE
	static FunctionProfile __funcProfile(__FUNCTION__);
	ProfilerStackFrame __funcFrame(&__funcProfile);
#endif

	UDP_SEND_PACKET_DATA	*pPacket = NULL;
	std::list<UDP_SEND_PACKET_DATA*>::iterator it;
	bool	bCompareIndex = false;
	bool	bDeleteData = false;
	uint32	lastIndex = 0;

	
	//更新对方收到本地发送出的最大标识
	if (PUB_UNSIGNED_LONG_GT(index, m_RecordedInfo.maxPeerRecvIndex))
	{		
		lastIndex = m_RecordedInfo.maxPeerRecvIndex;
		m_RecordedInfo.maxPeerRecvIndex = index;		
		bCompareIndex = true;

		if(m_CtrlAlg.bResendRestrict)
		{
			if(PUB_UNSIGNED_LONG_GEQ(index, m_CtrlAlg.nextResendIndex))
			{
				m_CtrlAlg.bResendRestrict = false;
			}
		}

		//只要知道对方能收到包，就把重传次数清0
		m_CtrlAlg.resendTime = 0;	

		//计算 RTT 和 RTO
#ifdef	RTT_RESOLUTION_1
		//RTT方法一：
		if (m_CtrlAlg.bCalculateRtt)
		{
			if (PUB_UNSIGNED_LONG_GEQ(m_RecordedInfo.maxPeerRecvIndex, m_CtrlAlg.rttHelp.sendIndex))
			{
				m_CtrlAlg.bCalculateRtt = false;
				CalculateRTO(currentTick, m_CtrlAlg.rttHelp.sendTick);
			}
		}
#else
		//RTT方法二：
		uint32 dwStartTick = 0;
		uint32 sendIndex = 0;		
		std::list<UDP_RTT_HELP>::iterator it;
		//在3516的板子上，list的size函数，list里的现有结点越多，效率越低,以前这里是循环判断size大小
		//现在改掉了
		for(it = m_listRttHelp.begin(); it != m_listRttHelp.end(); it = m_listRttHelp.begin())
		{
			sendIndex = (*it).sendIndex;
			int iDis = static_cast<int>(sendIndex - m_dwIndexPeerRecv);
			if (0 > iDis)
			{
				m_listRttHelp.pop_front();
			}
			else if(0 == iDis)
			{
				dwStartTick = (m_listRttHelp.front()).sendTick;
				m_listRttHelp.pop_front();
				break;
			}
			else
			{
				assert(false);
			}			
		}
		CalculateRTO(currentTick, dwStartTick);
#endif			
	}

	//更新对方确认的最大数据，删除本地未确认的发送数据
	if(PUB_UNSIGNED_LONG_GT(seq, m_RRecvInfo.maxPeerAck))
	{
		//有确认新的数据

		bDeleteData = true;
		m_RRecvInfo.maxPeerAck = seq;

		//有新的顺序数据被确认了，则第一个包重传的次数清零
		m_CtrlAlg.firstResend = 0;

		//重传结束，记录限制重传的变量，并且切换状态
		if (SWL_UDP_SENDMODE_TIMEOUT_RESEND == m_CtrlAlg.sendMode)
		{			
			m_CtrlAlg.bResendRestrict = true;
			m_CtrlAlg.nextResendIndex = m_RecordedInfo.lastSendIndex;
			ChangeMode(SWL_UDP_SENDMODE_SLOW_STARTUP);
		}
		else if(SWL_UDP_SENDMODE_FAST_RESEND == m_CtrlAlg.sendMode)
		{			
			m_CtrlAlg.bResendRestrict = true;
			m_CtrlAlg.nextResendIndex = m_RecordedInfo.lastSendIndex;
			ChangeMode(SWL_UDP_SENDMODE_CROWDED_AVOID);
		}
	}
#ifndef UDT_WITHOUT_LOCK
	m_RSendInfoLock.Lock();
#endif
	for (it = m_RSendInfo.listUnackedData.begin(); it != m_RSendInfo.listUnackedData.end(); ) 
	{	
		pPacket = *it;

		//找到每个回复对应的发送数据，释放资源
		if (bDeleteData)
		{
			if(PUB_UNSIGNED_LONG_GEQ(seq, pPacket->wrapperData.head.seqNum))
			{				
				//如果是SWL_UDP_DATA_STATUS_BEYOND、SWL_UDP_DATA_STATUS_TIMEOUT的，则以前减过用户窗口了
				if (pPacket->dataStatus == SWL_UDP_DATA_STATUS_SENDED)
				{										
					IncreaseCwnd(1, false);
				}

				m_RSendInfo.remainedBuffSize += pPacket->iLen;					
				m_pSendDataManager->ReleaseMemory(pPacket);
				m_RSendInfo.remainHeapPacketNum++;
				it = m_RSendInfo.listUnackedData.erase(it);	//bug 这里是主要效率低的地方			
				continue;
			}
			else
			{
				//如果现在找到的数据的序号已经比接收的更大，那么后面的肯定都更大了
				bDeleteData = false;
			}
		}

		if (bCompareIndex)
		{
			//等于的不用判断，后面会删掉   以前还没有处理过的标识
			if (PUB_UNSIGNED_LONG_GT(index, pPacket->lastSendIndex))
			{					
				assert(pPacket->lastSendIndex != index);

				//[dwLastIndex + 1, dwIndexPeerRecv) 这个区间的标识是需要处理的
				//dwIndexPeerRecv的包会被删掉，这里不处理，如果处理了，窗口就不会增加
				//这里标记了的包都不会增加窗口大小

				//没有设置SWL_UDP_DATA_STATUS_TIMEOUT和SWL_UDP_DATA_STATUS_BEYOND
				//更大的标识对方都收到了，设置SWL_UDP_DATA_STATUS_BEYOND，用于判断快速重传
				if (SWL_UDP_DATA_STATUS_SENDED == pPacket->dataStatus)
				{
					pPacket->dataStatus |= SWL_UDP_DATA_STATUS_BEYOND;
					IncreaseCwnd(1, true);
				}					
			}
			else
			{
				// >=dwIndexPeerRecv 
				// > dwIndexPeerRecv 的部分数据对方还没有收到，暂时先不处理
				// = 的包要么在前面的一个删除的处理中删除了，要么在外面乱序包的时候会删除

				//fixed by cc  因为listUnackedData中的packet，其lastSendIndex是递增的
				bCompareIndex = false;
			}			

  			if (!bDeleteData && !bCompareIndex)
  			{
  				break;
  			}
			
		}		
		++it;
	}
#ifndef UDT_WITHOUT_LOCK
	m_RSendInfoLock.UnLock();
#endif
	return 0;
}

uint32 CNatUdt::CalculateRTO( uint32 currentTick, uint32 startTick )
{
/* diff between this shouldn't exceed 32K since this are tcp timer ticks
	and a round-trip shouldn't be that long... */ 
	uint32 dwLastRTO = m_dwRTO;		 
	m_dwRTT = currentTick - startTick;
	short max = static_cast<short>(m_dwRTT);
	max = MIN_RTT_TIME > max ? MIN_RTT_TIME : max;
	short m = max;
		
	/* This is taken directly from VJs original code in his paper */
	m = m - (m_sa >> 3);
	m_sa += m;
	if (m < 0)
	{
		m = -m;
	}
	m = m - (m_sv >> 2);
	m_sv += m;
	m_dwRTO = static_cast<uint32>((m_sa >> 3) + m_sv);

#ifdef __ENVIRONMENT_LINUX__
	if(2 == g_bool)
	{
		NAT_LOG_DEBUG("dwRTT = %lu dwRTO = %lu m_sa = %hu m_sv = %hu\n", m_dwRTT, m_dwRTO,m_sa,m_sv);
	}
#endif


/*modified by cc  下面几行自行对[jacobson 1988/1990]的修改，会导致如下情况：
**虽然本次RTO能快速反应到RTT的2倍，但是会影响下一次RTO的计算，
**如果下一次RTO比较大，那计算出的下一次RTO值可能比下一次的RTT值还小,会增加重传次数
	//这里修改了在实际rtt已经很小，但是以前rtt比较大的时候，导致rto比较大，
	//短时间内rto变化太慢跟不上来的问题，
	//这里限制RTO不许超过RTT的两倍
	if (m_dwRTO > static_cast<uint32>(max << 1))
	{
		m_dwRTO = (max << 1);
	}	

	//这里限制RTO不许超过上次RTO的两倍
	if(m_dwRTO > (dwLastRTO<< 1))
	{
		m_dwRTO = (dwLastRTO<< 1);
	}
*/

//	if((m_dwRTO<200) || (m_dwRTO>400))
//	{
//		printf("dwRTT = %lu dwRTO = %lu m_sa = %hu m_sv = %hu\n", m_dwRTT, m_dwRTO,m_sa,m_sv);
//	}
	assert(m_dwRTO != 0);
	return m_dwRTO;
}

int CNatUdt::ChangeMode( byte ucMode )
{
	//printf("curMode :%d    Changeto :%d   Cwnd :%d   SSthresh :%d\n", m_CtrlAlg.sendMode, ucMode, m_iCwnd, m_iSsthresh);

	if (SWL_UDP_SENDMODE_TIMEOUT_RESEND == ucMode)
	{		
		if(SWL_UDP_SENDMODE_FAST_RESEND == m_CtrlAlg.sendMode)//正在快速恢复时，要切换到慢启动
		{				
			//m_iSsthresh 不变，因为进入快速恢复的时候已经变过一次了
			m_iCwnd = (m_iSsthresh << 2) / 5;
		}
		else if((SWL_UDP_SENDMODE_CROWDED_AVOID == m_CtrlAlg.sendMode) || (SWL_UDP_SENDMODE_SLOW_STARTUP == m_CtrlAlg.sendMode))
		{			
			m_iSsthresh = m_iCwnd;			
			m_iCwnd = (m_iCwnd << 2) / 5;	// 4/5
		}
		else if(SWL_UDP_SENDMODE_TIMEOUT_RESEND == m_CtrlAlg.sendMode)
		{			
			m_iSsthresh = m_iCwnd;			
			m_iCwnd = (m_iCwnd << 2) / 5;	// 4/5						
		}

		if (m_iSsthresh < MIN_SSTHRESH)
		{
			m_iSsthresh = MIN_SSTHRESH;
		}

		if(m_iCwnd < MIN_CWND_WIN)
		{
			m_iCwnd = MIN_CWND_WIN;
		}

		m_CtrlAlg.sendMode = SWL_UDP_SENDMODE_TIMEOUT_RESEND;
	}
	//转到慢启动状态
	else if (SWL_UDP_SENDMODE_SLOW_STARTUP == ucMode)
	{
		//目前只有一种发送模式可以切换进来
		assert(SWL_UDP_SENDMODE_TIMEOUT_RESEND == m_CtrlAlg.sendMode);
		m_CtrlAlg.sendMode = SWL_UDP_SENDMODE_SLOW_STARTUP;
	}
	//转到拥塞避免状态
	else if(SWL_UDP_SENDMODE_CROWDED_AVOID == ucMode)
	{
		if (SWL_UDP_SENDMODE_SLOW_STARTUP == m_CtrlAlg.sendMode)
		{
			//什么都不做
		}		
		else if(SWL_UDP_SENDMODE_FAST_RESEND == m_CtrlAlg.sendMode)
		{			
			//m_iSsthresh 不变，因为进入快速恢复的时候已经变过一次了
			m_iCwnd = m_iSsthresh;
		}
		else
		{
			//如果本来就是拥塞避免的状态，外面又调用进入拥塞避免状态，
			//那么逻辑上肯定有问题
			//如果是超时重传也不可能切换到拥塞避免状态
			assert(false);
		}
		m_iCrowdedNumerator = 0;
		m_CtrlAlg.sendMode = SWL_UDP_SENDMODE_CROWDED_AVOID;		
	}
	//转到快速恢复状态
	else if(SWL_UDP_SENDMODE_FAST_RESEND == ucMode)
	{
		assert(m_CtrlAlg.sendMode != SWL_UDP_SENDMODE_FAST_RESEND);

		//这一行会出现是因为，device端断开连接后，NATServer端还没断开，NATServer端发出的包没应答，模式切换到SWL_UDP_SENDMODE_TIMEOUT_RESEND
		//而此时device端又重新连接，device端数据的SendIndex由于是GetTickCount计算出来的，肯定会比之前的大，这导致unAckList中的packet被标记为了SWL_UDP_DATA_STATUS_BEYOND,
		//现在RecvPacket，对连接还没建立时的packet忽略，不会出现该断言了
//		assert(m_CtrlAlg.sendMode != SWL_UDP_SENDMODE_TIMEOUT_RESEND);

		m_iSsthresh = ((m_iCwnd << 3) - m_iCwnd) >> 3;		// 7/8
		if (m_iSsthresh < MIN_SSTHRESH)
		{
			m_iSsthresh = MIN_SSTHRESH;
		}		
		m_CtrlAlg.sendMode = SWL_UDP_SENDMODE_FAST_RESEND;
	}
	else
	{
		assert(false);
	}
	return 0;
}

int CNatUdt::IncreaseCwnd( int iCount /*= 1*/, bool bResend /*= false*/ )
{
	m_iUsed -= iCount;
	assert(m_iUsed >= 0);

	if (!bResend)
	{
		for (int i = 0; i < iCount; ++i)
		{
			//拥塞避免
			if(SWL_UDP_SENDMODE_CROWDED_AVOID == m_CtrlAlg.sendMode)
			{
				//每次要收到 拥塞窗口数量 个包才给拥塞窗口加1
				++m_iCrowdedNumerator;
				if (m_iCrowdedNumerator >= m_iCwnd)
				{
					++m_iCwnd;
					m_iCrowdedNumerator = 0;
				}		
			}
			//超时重传
			else if(SWL_UDP_SENDMODE_TIMEOUT_RESEND == m_CtrlAlg.sendMode) 
			{
				//超时重传时，每收到一个包，只会再发一个包，不会像慢启动一样
				//每收一个包发两个包
			}
			//快速恢复
			else if(SWL_UDP_SENDMODE_FAST_RESEND == m_CtrlAlg.sendMode)
			{
				//快速恢复时，每收到一个包，只会再发一个包，不会像慢启动一样
				//每收一个包发两个包
			}
			else//慢启动
			{				
				++m_iCwnd;
				if (m_iCwnd > m_iSsthresh)
				{
					ChangeMode(SWL_UDP_SENDMODE_CROWDED_AVOID);
				}								
			}		
		}	
	}

	return 0;
}

int CNatUdt::AdjustRecvPacket()
{
#ifdef TEST_PROFILE
	static FunctionProfile __funcProfile(__FUNCTION__);
	ProfilerStackFrame __funcFrame(&__funcProfile);
#endif
#ifndef UDT_WITHOUT_LOCK
	m_RecvLock.Lock();
#endif
	NatUdtRecvDataList::iterator it;
	for(it = m_RRecvInfo.recvDataList.find(m_RRecvInfo.expectedSeq);
		it != m_RRecvInfo.recvDataList.end();
		it = m_RRecvInfo.recvDataList.find(m_RRecvInfo.expectedSeq))
	{		
		++(m_RRecvInfo.expectedSeq);
	}
#ifndef UDT_WITHOUT_LOCK
	m_RecvLock.UnLock();
#endif
	//删除没有乱序的包序号
	std::list<uint32>::iterator itex;	
	for (itex = m_listDisorderlyPacket.begin(); itex != m_listDisorderlyPacket.end();)
	{	
		//不可能有等于的包
		assert( (*itex) != m_RRecvInfo.expectedSeq);
		if (PUB_UNSIGNED_LONG_LT((*itex), m_RRecvInfo.expectedSeq))
		{
			itex = m_listDisorderlyPacket.erase(itex);
		}
		else
		{
			++itex;
		}
	}

	return 0;
}


int CNatUdt::OnNotifySendPacket( const void* pBuf, int iLen )
{
#ifdef TEST_PROFILE
	static FunctionProfile __funcProfile(__FUNCTION__);
	ProfilerStackFrame __funcFrame(&__funcProfile);
#endif

	int ret = -1;
	if (m_pUdtNotifier)
	{
		ret = m_pUdtNotifier->OnSendPacket(this, pBuf, iLen);
	}
	else
	{
		NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu]  UdtNotifier Err\n", m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID());
	}

	if (0 == ret)
	{
		NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu]  UdtNotifier Blocked\n", m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID());
		ret = 1;
	}
	else if(iLen == ret)
	{
		ret = 0;
	}
	else
	{
		NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu]  sock err\n", m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID());
		ret = -1;
	}

	return ret;
}

int CNatUdt::OnNotifyRecvPacket( const void* pBuf, int iLen )
{
#ifdef TEST_PROFILE
	static FunctionProfile __funcProfile(__FUNCTION__);
	ProfilerStackFrame __funcFrame(&__funcProfile);
#endif

	int ret = -1;
	if (m_pUdtNotifier)
	{
		ret = m_pUdtNotifier->OnRecv(this, pBuf, iLen);
	}

	return ret;
}

void CNatUdt::OnNotifyConnect( int iErrorCode )
{
	NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu]  OnNotifyConnect : %d\n", m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID(), iErrorCode);
	if (m_pUdtNotifier)
	{
		m_pUdtNotifier->OnConnect(this, iErrorCode);
	}
}

int CNatUdt::HandleRecvReliableData(const UDP_WRAPPER_DATA *pWrapperData, int iLen, uint32 currentTick )
{
#ifdef TEST_PROFILE
	static FunctionProfile __funcProfile(__FUNCTION__);
	ProfilerStackFrame __funcFrame(&__funcProfile);
#endif

	/********************************** 根据对方的确认，标识进行相关处理 **************************************************/
	ReleaseReliableData(pWrapperData->head.ackNum, pWrapperData->head.recvIndex, currentTick);

	/********************************** 根据对方的发送序号进行相关处理 ****************************************************/
	// 更新收到对方数据的最大标识
	if (PUB_UNSIGNED_LONG_GT(pWrapperData->head.sendIndex, m_RecordedInfo.maxRecvIndex))
	{
		m_RecordedInfo.maxRecvIndex = pWrapperData->head.sendIndex;

		// 标识更新也要发送确认，这里懒得做仔细的判断，简单的处理一下
		// 没有判断标识增大的各种可能情况，例如：标识变大，但是没有收到新数据，有可能是对发重传了这边收到过的数据
		if (SWL_UDP_ACK_STATUS_NONE == m_RecordedInfo.ackStatus)
		{
			m_RecordedInfo.ackStatus = SWL_UDP_ACK_STATUS_DELAY;
		}
		else if(SWL_UDP_ACK_STATUS_DELAY == m_RecordedInfo.ackStatus)//收过两个新的数据，就需要立即发送确认
		{
			m_RecordedInfo.ackStatus = SWL_UDP_ACK_STATUS_NOW;
		}
		else
		{
			// 本来就是 SWL_UDP_ACK_STATUS_NOW，或者已经是更高级别的 SWL_UDP_ACK_STATUS_DISORDER ，就不改属性了 
		}
	}

	//注意，这里iDisSeq必须用有符号的数字，并且发送序号溢出的情况也能正常处理
    int	iDisSeq = pWrapperData->head.seqNum - m_RRecvInfo.expectedSeq;
    int iBufDisSeq = pWrapperData->head.seqNum - m_RRecvInfo.minSeq;
#ifndef UDT_WITHOUT_LOCK
	m_RecvLock.Lock();
#endif
    if(iBufDisSeq > (static_cast<int>(m_dwMaxRecvReliableBuffCount) - 1))	// 超过缓冲区大小的包
    {
        // NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu]  buffer over flow\n", m_RemoteInfo.Ip, m_RemoteInfo.port, m_RecordedInfo.initPeerSendSeq);
        // NAT_LOG_DEBUG("map size:%lu\n", m_RRecvInfo.mapReliableRecvData.size());
        // BUG这里程序里最好限制一下发送，不要超过缓冲区，如果以后版本升级，把收发双方分开，则可以取消这个限制
        // 两端对其都要做防呆处理，包括拥塞窗口如何变化
        // 在这个期望收到的数据之前，可能还有数据没有被应用程序收走，如果应用程序老不收，那么可能就会内存耗尽
        //		assert(false);
#ifndef UDT_WITHOUT_LOCK
        m_RecvLock.UnLock();
#endif
        return 0;
    }
	//这个包正好是在等的包
	else if (0 == iDisSeq)
	{
		//就算以前收到过（后面添加返回了-1），也要设置这个标志，因为可能对方没有收到确认，是重发	
		if (SWL_UDP_ACK_STATUS_NONE == m_RecordedInfo.ackStatus)
        {
			m_RecordedInfo.ackStatus = SWL_UDP_ACK_STATUS_DELAY;
		}
		else if(SWL_UDP_ACK_STATUS_DELAY == m_RecordedInfo.ackStatus) //收过两个新的数据，就需要立即发送确认
		{
			m_RecordedInfo.ackStatus = SWL_UDP_ACK_STATUS_NOW;
		}
		else
		{
			//本来就是SWL_UDP_ACK_STATUS_NOW，或者已经是更高级别的SWL_UDP_ACK_STATUS_DISORDER，就不改属性了
		}

		// 如果这个序号的包以前已经收到了，就会添加失败        
        if(!InsertRecvData(pWrapperData->head.seqNum, pWrapperData, iLen))
		{		
#ifndef UDT_WITHOUT_LOCK
			m_RecvLock.UnLock();
#endif
			return -1;
		}
	}
	else if(iDisSeq < 0)//过时包
	{
		//printf("UDT recv overtime packet\n");

		//不降低回复确认的级别就好了
		if (SWL_UDP_ACK_STATUS_DISORDER != m_RecordedInfo.ackStatus)
		{
			//收到过时包，可能是对方没有收到确认，因此马上重发确认
			m_RecordedInfo.ackStatus = SWL_UDP_ACK_STATUS_NOW;
		}
#ifndef UDT_WITHOUT_LOCK
		m_RecvLock.UnLock();
#endif
		return 0;
	}
	else //乱序包 这里的else就等于if(iDisSeq > 0 && iDisSeq <= m_dwMaxRecvReliableBuffCount - (m_RRecvInfo.expectedSeq - m_RRecvInfo.minSeq) + 1)
	{
		//就算以前收到过（后面添加返回了-1），也要设置这个标志，因为可能对方没有收到确认，是重发		
		m_RecordedInfo.ackStatus = SWL_UDP_ACK_STATUS_DISORDER;

		//以前是删除掉重复的乱序序号，并把新来乱序序号放在最前面，现在不去重，直接放在链表后面，只会造成可能多发ack
		m_listDisorderlyPacket.push_back(pWrapperData->head.seqNum);

        //如果这个序号的包以前已经收到了，就会添加失败
        InsertRecvData(pWrapperData->head.seqNum, pWrapperData, iLen);

#ifndef UDT_WITHOUT_LOCK
		m_RecvLock.UnLock();
#endif
		return 0;
	}

	//走到这里了，肯定是收到顺序的包了

	//更新期望接收到的下一个包的序号，删除没有乱序的包序号
	++m_RRecvInfo.expectedSeq;
	uint32 dwNextExpected = m_RRecvInfo.expectedSeq;

	AdjustRecvPacket();

	//如果收到本数据还把缓冲中的乱序数据连续几个一起确认了，则需要立即发送ACK
	if (PUB_UNSIGNED_LONG_GT(m_RRecvInfo.expectedSeq, dwNextExpected))
	{
		if (SWL_UDP_ACK_STATUS_DISORDER != m_RecordedInfo.ackStatus)
		{
			m_RecordedInfo.ackStatus = SWL_UDP_ACK_STATUS_NOW;
		}
	}


	if (m_bIsAutoRecv)
	{
		AutoNotifyRecvPacket();
	}
#ifndef UDT_WITHOUT_LOCK
	m_RecvLock.UnLock();
#endif
	return 0;
}

int CNatUdt::HandleRecvAckData( const UDP_WRAPPER_DATA *pData, int iLen, uint32 currentTick )
{
#ifdef TEST_PROFILE
	static FunctionProfile __funcProfile(__FUNCTION__);
	ProfilerStackFrame __funcFrame(&__funcProfile);
#endif

	UDP_SEND_PACKET_DATA	*pPacket = NULL;
	std::list<UDP_SEND_PACKET_DATA*>::iterator it;

	assert( 0 == ((iLen - sizeof(UDP_WRAPPER_HEAD)) % sizeof(uint32)));

	ReleaseReliableData(pData->head.ackNum, pData->head.recvIndex, currentTick);

	int i = 0;
	int iFoundCount = 0;

#ifdef __ENVIRONMENT_LINUX__
	if (g_bool == 2)
	{
		int iLenex = iLen;
		//printf("dwRecvSeq = %lu, dwRecvIndex = %lu, currentTick = %lu\n",pData->head.dwRecvSeq, pData->head.dwRecvIndex, //currentTick);
		//for (i = 0, iLenex -= sizeof(UDP_WRAPPER_HEAD); iLenex > 0; iLenex -= sizeof(UDP_ACK), ++i)
		//{
		//	printf("%lu ", pData->wrapper_dat.acks[i].ackSeq);
		//}
		//printf("i = %d iLen = %d\n",i,iLen);
	}
#endif
#ifndef UDT_WITHOUT_LOCK
	m_RSendInfoLock.Lock();
#endif
	//bug 这里的循环和erase都会导致效率低，主要是erase
	for (i = 0, iLen -= sizeof(UDP_WRAPPER_HEAD); iLen > 0; iLen -= sizeof(UDP_ACK), ++i)
	{			
		//找到每个回复对应的发送数据，释放资源

		//先找unacked队列
		for(it = m_RSendInfo.listUnackedData.begin(); it != m_RSendInfo.listUnackedData.end();)
		{			
			pPacket = *it;				

			int iDisSeq = pData->wrapper_dat.acks[i].ackSeq - pPacket->wrapperData.head.seqNum;
			if (0 == iDisSeq)
			{	
				if (pPacket->dataStatus == SWL_UDP_DATA_STATUS_SENDED)
				{					
					++iFoundCount;
				}				

				m_RSendInfo.remainedBuffSize += pPacket->iLen;
				m_pSendDataManager->ReleaseMemory(pPacket);
				m_RSendInfo.remainHeapPacketNum++;
				it = m_RSendInfo.listUnackedData.erase(it);
				break;
			}
			else if(0 > iDisSeq)
			{		
				break;
			}
			else //(0 < iDisSeq)
			{				
				//继续比较下一个更大的序号
				++it;
			}				
		}	
	}
#ifndef UDT_WITHOUT_LOCK
	m_RSendInfoLock.UnLock();
#endif
	//之所以把增大窗口放在后面，是因为要先判断是否切换到快速恢复
	if (0 != iFoundCount)
	{
		IncreaseCwnd(iFoundCount, false);
	}

	return 0;
}

int CNatUdt::HandleRecvConnect( const UDP_WRAPPER_DATA *pData, int iLen, uint32 currentTick )
{
	//printf("HandleRecvConnect ConnectionID %lu  sendSeq%lu\n", m_RecordedInfo.initPeerSendSeq, pData->head.sendSeqNum);

	if ( iLen < sizeof(UDP_SYN_PACKET) 
		|| pData->wrapper_dat.syn.protocolTag != NAT_UDT_PROTOCOL_TAG/* || pData->wrapper_dat.syn.version*/)
	{
		NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu] Connect Syn: wrong protocol syn!\n", \
			m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID());
		return 0;
	}

	int ret = 0;
	bool bSendAck = false;

	switch(m_RecordedInfo.state)
	{
	case SWL_UDP_STATE_UNCONNECTED:
		if (0 == pData->head.ackNum)
		{
			NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu] Connect Syn: [1] recv dest peer 1st syn!\n", \
				m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID());	

			m_RecordedInfo.state = SWL_UDP_STATE_RECVED;
			bSendAck = true;
		}
		else if(m_RecordedInfo.initSendSeq == pData->head.ackNum)
		{
			NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu] Connect Syn: [2] dest peer has recv syn, but do not known if this peer have recv syn!\n", \
				m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID());

			m_RecordedInfo.state = SWL_UDP_STATE_CONNECTED;
			OnNotifyConnect(0);
			bSendAck = true;
		}
		else
		{
			//忽略
			NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu] Connect Syn: useless syn!\n", \
				m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID());
		}
		break;
	case SWL_UDP_STATE_RECVED:
		if (m_RecordedInfo.initSendSeq + 1 == pData->head.ackNum)
		{
			NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu] Connect Syn: [4] dest peer has been in connected state!\n", \
				m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID());

			m_RecordedInfo.state = SWL_UDP_STATE_CONNECTED;
			OnNotifyConnect(0);
		}
		else if(0 == pData->head.ackNum)
        {
            NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu] Connect Syn: recv syn[2] again, just reply!\n", \
                m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID());
			bSendAck = true;
		}
		else if(m_RecordedInfo.initSendSeq == pData->head.ackNum)
		{
			//双向打开的情况，此时对方收到我方的第一次握手包了，也认为连接建立
            NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu] Connect Syn: [3] dest peer has recv syn, and known this peer have recv syn!\n", \
                m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID());
			m_RecordedInfo.state = SWL_UDP_STATE_CONNECTED;
			OnNotifyConnect(0);
			bSendAck = true;
		}
		else
		{
			//错误的SYN  忽略
			NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu] Connect Syn: recv wrong syn ack!\n", \
				m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID());
		}
		break;
	case SWL_UDP_STATE_CONNECTED:
		assert(m_RecordedInfo.initPeerSendSeq != 0);
		if (m_RecordedInfo.initSendSeq == pData->head.ackNum)
		{
            //第三次握手对方没收到，重发
            NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu] Connect Syn: this peer has connected, just reply syn!\n", \
                m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID());
			bSendAck = true;
		}
		else if(pData->head.ackNum == 0)
		{
			//assert(false);//可能已经连接上了，第一个SYN包才到达
			//对方超时或异常，企图重连
			if (m_RecordedInfo.initPeerSendSeq == pData->head.seqNum)
			{
				//同一个peer发过来的，应该不存在这样的情况
				NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu] Connect Syn: useless syn!\n", \
					m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID());
			}
			else
			{
				//不同peer
				NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu] Connect Syn: differnt syn\n", \
					m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID());
			}
		}
		else if (m_RecordedInfo.initSendSeq + 1 == pData->head.ackNum)
		{
			//连接已经建立，忽略
		}
		break;
	default:
		assert(false);
		break;
	}

	if (bSendAck)
	{
		m_RecordedInfo.initPeerSendSeq = pData->head.seqNum;
		m_RRecvInfo.maxPeerAck = m_RecordedInfo.initSendSeq;	
		m_RRecvInfo.expectedSeq = pData->head.seqNum + 1;
		m_RRecvInfo.minSeq = m_RRecvInfo.expectedSeq;

		m_RecordedInfo.maxRecvIndex = pData->head.sendIndex;

		UDP_SYN_PACKET synPacket;
		synPacket.head.category = m_Category;
		synPacket.head.cmdId = UDP_DATA_TYPE_SYN;
		synPacket.head.maxPacketLen = MAX_WRAPPER_DATA_LEN;

		//SYN类型的数据，这两个变量没有用
		synPacket.head.sendIndex = m_RecordedInfo.lastSendIndex;
		synPacket.head.recvIndex = m_RecordedInfo.maxRecvIndex;

		synPacket.head.seqNum = m_RecordedInfo.initSendSeq;
		synPacket.head.connectionID = GetConnectionID();
		synPacket.data.protocolTag = NAT_UDT_PROTOCOL_TAG;
		//synPacket.data.version

		if (SWL_UDP_STATE_CONNECTED == m_RecordedInfo.state)
		{
			synPacket.head.ackNum = pData->head.seqNum + 1;
		}
		else if(SWL_UDP_STATE_RECVED == m_RecordedInfo.state)
		{
			synPacket.head.ackNum = pData->head.seqNum;
		}	

		//更新最后一次发送时间
		m_RecordedInfo.lastSendTick = currentTick;

		//记录缓冲区允许的的最大包个数
		//m_dwMaxRecvReliableBuffCount = (MAX_RECV_RELIABLE_BUFFSIZE / pData->head.maxPacketLen) + 1;

		OnNotifySendPacket(&synPacket, sizeof(UDP_SYN_PACKET));
	}

	return ret;	
}

int CNatUdt::HandleRecvRST( const UDP_WRAPPER_DATA *pData, int iLen, uint32 currentTick )
{
	NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu]  HandleRecvRST\n", m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID());

	m_RecordedInfo.state = SWL_UDP_STATE_UNCONNECTED;

	return 0;
}

int CNatUdt::ResendData( uint32 currentTick )
{
	if (SWL_UDP_STATE_CONNECTED != m_RecordedInfo.state)
	{
		return -5;
	}

#ifdef TEST_PROFILE
	static FunctionProfile __funcProfile(__FUNCTION__);
	ProfilerStackFrame __funcFrame(&__funcProfile);
#endif


	UDP_SEND_PACKET_DATA *pPacket = NULL;
	std::list<UDP_SEND_PACKET_DATA*>::iterator it = m_RSendInfo.listUnackedData.begin();
	uint32 dwResendTime = m_dwRTO << m_CtrlAlg.resendTime;//指数退避
	dwResendTime = dwResendTime > MAX_RESEND_TIME ? MAX_RESEND_TIME : dwResendTime;

	//每次进来都从头到尾遍历，效率比较低	
	//bug 这段代码是否可以考虑有其他的实现方式代替，当m_RSendInfo.listUnackedData里面数据比较多的时候
	//效率会比较低
	for(; it != m_RSendInfo.listUnackedData.end(); ++it)
	{
		pPacket = *it;						
		if (SWL_UDP_DATA_STATUS_SENDED == pPacket->dataStatus)
		{
			if (PUB_IsTimeOutEx(pPacket->lastSendTick, dwResendTime, currentTick))
			{
				IncreaseCwnd(1, true);
				pPacket->dataStatus |= SWL_UDP_DATA_STATUS_TIMEOUT;
			}
		}		
	}


	if (m_iUsed >= m_iCwnd)
	{
		return -6;
	}

	bool bFirst = true;
	//bug 循环和SendPacketData都有可能降低效率，仔细研究一下怎么提高这个代码的效率
	//当会循环很多次，重发后面的某个数据时，循环的效率比较低
	for(it = m_RSendInfo.listUnackedData.begin(); it != m_RSendInfo.listUnackedData.end(); ++it, bFirst = false)
	{
		//重传过一次后，没有index更新，则第2次开始每次只重传第一个包
		//这种情况当窗口不够来处理
		if (!bFirst)
		{
			if(m_CtrlAlg.resendTime > 1)
			{
				return -6;
			}
		}
		pPacket = *it;
		if ((SWL_UDP_DATA_STATUS_TIMEOUT & pPacket->dataStatus)
			|| (SWL_UDP_DATA_STATUS_BEYOND & pPacket->dataStatus))
		{
			byte dataStatus = pPacket->dataStatus;
#ifdef	RTT_RESOLUTION_1
			//RTT方法一：
			//因为RTT方法一不如方法二准，如果某个包正在用于计算RTT，可是发生了重传，
			//则这个包当前时间用于重新计算RTT

			//modified by cc  应该为pPacket->lastSendIndex
			if (m_CtrlAlg.bCalculateRtt && m_CtrlAlg.rttHelp.sendIndex == pPacket->lastSendIndex)
			{
				//这里置为false，是要在SendPacketData中重新更新rtthelp的发送时间
				m_CtrlAlg.bCalculateRtt = false;
			}
#else
#endif
			m_Statistic.reSendPacketNum++;
			m_Statistic.reSendBytes += pPacket->iLen+sizeof(UDP_WRAPPER_HEAD);

			int iRet = SendPacketData(pPacket, currentTick);
			if(0 == iRet)
			{	
				if (bFirst)
				{
					do 
					{
						//重传限制时，没有超过m_dwNextResendIndex则不会再次切换某种重传状态
						if(m_CtrlAlg.bResendRestrict && PUB_UNSIGNED_LONG_LEQ(pPacket->lastSendIndex, m_CtrlAlg.nextResendIndex))
						{
							break;
						}

						//再次进重传
						if (0 != m_CtrlAlg.firstResend)
						{
							ChangeMode(SWL_UDP_SENDMODE_TIMEOUT_RESEND);
						}
						else	//首次重传
						{
							//超时重传
							if (SWL_UDP_DATA_STATUS_TIMEOUT & dataStatus)
							{
								ChangeMode(SWL_UDP_SENDMODE_TIMEOUT_RESEND);
							}
							//快速重传
							else if(SWL_UDP_DATA_STATUS_BEYOND & dataStatus)
							{
								ChangeMode(SWL_UDP_SENDMODE_FAST_RESEND);
							}
							else
							{
								assert(false);
							}
						}

						//重传次数+1

						++m_CtrlAlg.resendTime;
						++m_CtrlAlg.firstResend;
					} while(false);				
				}			

				++m_iUsed;
				return 1;
			}
			else if(-1 == iRet)
			{		
				return -1;
			}
			else //if(1 == iRet) socket EWOULDBLOCK
			{
				return -2;
			}
		}
	}

	return 0;
}

int CNatUdt::SendPacketData( UDP_SEND_PACKET_DATA *pPacket, uint32 currentTick )
{
#ifdef TEST_PROFILE
	static FunctionProfile __funcProfile(__FUNCTION__);
	ProfilerStackFrame __funcFrame(&__funcProfile);
#endif

	int iLen = 0;

	pPacket->wrapperData.head.category = m_Category;
	pPacket->wrapperData.head.cmdId = UDP_DATA_TYPE_RELIABLE;
	pPacket->wrapperData.head.maxPacketLen = MAX_WRAPPER_DATA_LEN;
	pPacket->wrapperData.head.sendIndex = m_RecordedInfo.lastSendIndex + 1;
	pPacket->wrapperData.head.recvIndex = m_RecordedInfo.maxRecvIndex;
	pPacket->wrapperData.head.seqNum = pPacket->wrapperData.head.seqNum;
	pPacket->wrapperData.head.ackNum = m_RRecvInfo.expectedSeq - 1;
	pPacket->wrapperData.head.connectionID = GetConnectionID();

	if (0 != pPacket->iLen)
	{
		iLen = sizeof(UDP_WRAPPER_HEAD) + pPacket->iLen;			
	}
	else
	{
		iLen = sizeof(UDP_WRAPPER_HEAD);
	}

	int iRet = OnNotifySendPacket(&(pPacket->wrapperData), iLen);

	if(0 == iRet)
	{
		++m_RecordedInfo.lastSendIndex;

		//更新最后一次发送时间
		m_RecordedInfo.lastSendTick = currentTick;

		//捎带，可以把DELAY和NOW清除
		if((SWL_UDP_ACK_STATUS_DELAY == m_RecordedInfo.ackStatus)
			||(SWL_UDP_ACK_STATUS_NOW == m_RecordedInfo.ackStatus))
		{
			m_RecordedInfo.ackStatus = SWL_UDP_ACK_STATUS_NONE;
		}

		//标记数据发送过
		pPacket->dataStatus = SWL_UDP_DATA_STATUS_SENDED;

		pPacket->lastSendIndex = m_RecordedInfo.lastSendIndex;

		//修改计时时间
		pPacket->lastSendTick = currentTick;
#ifdef	RTT_RESOLUTION_1
		//RTT方法一：
		if (!m_CtrlAlg.bCalculateRtt) 
		{
			m_CtrlAlg.rttHelp.sendIndex = m_RecordedInfo.lastSendIndex;
			m_CtrlAlg.rttHelp.sendTick = currentTick;
			m_CtrlAlg.bCalculateRtt = true;
		}
#else
		//RTT方法二：
		UDP_RTT_HELP udpRttHelp;
		udpRttHelp.sendIndex = m_dwIndexSendLast;
		udpRttHelp.sendTick = currentTick;
		m_listRttHelp.push_back(udpRttHelp);
#endif
	}
	return iRet;
}

int CNatUdt::SendReliableData( uint32 currentTick )
{
	if (SWL_UDP_STATE_CONNECTED != m_RecordedInfo.state)
	{
		return -5;
	}

	//窗口满了
	if (m_iUsed >= m_iCwnd)			
	{
		return -6;
	}

#ifdef TEST_PROFILE
	static FunctionProfile __funcProfile(__FUNCTION__);
	ProfilerStackFrame __funcFrame(&__funcProfile);
#endif
#ifndef UDT_WITHOUT_LOCK
	m_RSendInfoLock.Lock();
#endif
	int ret = 0;
	UDP_SEND_PACKET_DATA *pPacket = NULL;
	do  
	{
		if (m_RSendInfo.listReliableSendData.empty())
		{
			break;
		}
		
		pPacket = m_RSendInfo.listReliableSendData.front();
		assert(pPacket);

		//已经有 MAX_SEND_RELIABLE_WINDOW 个包没有确认了，则暂时先不发数据了
        // -- 陈信华：由于没有发送窗口大小的动态调整，此处判断先暂时忽略。
// 		if(PUB_UNSIGNED_LONG_GEQ(pPacket->wrapperData.head.seqNum, m_RRecvInfo.maxPeerAck + MAX_SEND_RELIABLE_WINDOW))
// 		{
// 			ret = -3;
// 			break;
// 		}

		int iRet = SendPacketData(pPacket, currentTick);
		if(0 == iRet)
		{
			//成功发送了一个数据，发送出去的在网络上的数据数+1
			++m_iUsed;
			ret = 1;
			
			m_Statistic.totalSendPacketNum++;
			m_Statistic.totalSendBytes += pPacket->iLen+sizeof(UDP_WRAPPER_HEAD);

			//发数据换个队列
			m_RSendInfo.listUnackedData.push_back(pPacket);
			m_RSendInfo.listReliableSendData.pop_front();
			break;
		}
		else if(-1 == iRet)
		{
			ret = -1;		
			break;
		}
		else//OnSendPacket是否要定义一个缓冲区满的情况，用在这里
		{				
			ret = -2;
			break;
		}
	}while(0);
#ifndef UDT_WITHOUT_LOCK
	m_RSendInfoLock.UnLock();
#endif
	return ret;
}

//可能与其他外部调用的接口都不在一个线程中
//发送RST，但没有接收RST的ACK，不保证对方一定能收到RST
void CNatUdt::Disconnect()
{

	if(m_RecordedInfo.state != SWL_UDP_STATE_CONNECTED)
    {
        m_RecordedInfo.state = SWL_UDP_STATE_UNCONNECTED;
		return;
	}
	m_RecordedInfo.state = SWL_UDP_STATE_UNCONNECTED;

	NAT_LOG_DEBUG("[Peer Ip:%s  port:%d  ConnectionID:%lu]  Send Disconnect\n", m_RemoteInfo.Ip, m_RemoteInfo.port, GetConnectionID());

	UDP_SYN_PACKET synPacket;
	synPacket.head.category = m_Category;
	synPacket.head.cmdId = UDP_DATA_TYPE_RST;
	synPacket.head.maxPacketLen = MAX_WRAPPER_DATA_LEN;
	synPacket.head.sendIndex = m_RecordedInfo.lastSendIndex;
	synPacket.head.recvIndex = m_RecordedInfo.maxRecvIndex;
	synPacket.head.seqNum = 0;//没用
	synPacket.head.ackNum = 0;//没用
	synPacket.head.connectionID = GetConnectionID();
	synPacket.data.protocolTag = NAT_UDT_PROTOCOL_TAG;
	//synPacket.data.version

	OnNotifySendPacket(&synPacket, sizeof(UDP_WRAPPER_HEAD));
}

void CNatUdt::SetAutoRecv( bool IsCallBack )
{
#ifndef UDT_WITHOUT_LOCK
	m_RecvLock.Lock();
#endif
	m_bIsAutoRecv = IsCallBack;
#ifndef UDT_WITHOUT_LOCK
	m_RecvLock.UnLock();
#endif
}

int CNatUdt::Recv( void *pData, int iLen )
{
#ifdef TEST_PROFILE
	static FunctionProfile __funcProfile(__FUNCTION__);
ProfilerStackFrame __funcFrame(&__funcProfile);
#endif

	if (m_RecordedInfo.state != SWL_UDP_STATE_CONNECTED)
	{
		return -1;
	}

	int ret = 0, dataLen = 0;
	UDP_WRAPPER_DATA *pWrapperData = NULL;
	NatUdtRecvDataList::iterator it;
#ifndef UDT_WITHOUT_LOCK
	m_RecvLock.Lock();
#endif
	do
	{
		if (m_bIsAutoRecv)
		{
			ret = -1;
			break;
		}

		it = m_RRecvInfo.recvDataList.find(m_RRecvInfo.minSeq);
		while (it != m_RRecvInfo.recvDataList.end() && iLen > 0)
		{
			pWrapperData = &(*it).second->pPacket;
			dataLen = (*it).second->PacketSize() - m_RRecvInfo.lastOnRecvPackPos;

			assert(dataLen > 0);

			if (dataLen <= iLen)
			{
				memcpy((char *)pData+ret, pWrapperData->wrapper_dat.data + m_RRecvInfo.lastOnRecvPackPos, dataLen);
				ret += dataLen;
				iLen -= dataLen;
				m_RRecvInfo.lastOnRecvPackPos = 0;
			}
			else
			{
				memcpy((char*)pData+ret, pWrapperData->wrapper_dat.data + m_RRecvInfo.lastOnRecvPackPos, iLen);
				ret += iLen;
				m_RRecvInfo.lastOnRecvPackPos += iLen;
				break;
			}
			
			//取下一个包
            DestroyRecvData((*it).second);
			m_RRecvInfo.recvDataList.erase(it);
			++(m_RRecvInfo.minSeq);
			it = m_RRecvInfo.recvDataList.find(m_RRecvInfo.minSeq);
		}

	}while(0);
#ifndef UDT_WITHOUT_LOCK
	m_RecvLock.UnLock();
#endif
	return ret;
}

void CNatUdt::AutoNotifyRecvPacket()
{
	//数据回调给应用层
	int	dataLen = 0, heapDataLen = 0;
	int ret = 0;
	UDP_WRAPPER_DATA *pWrapperData = NULL;
	NatUdtRecvDataList::iterator it;
	it = m_RRecvInfo.recvDataList.find(m_RRecvInfo.minSeq);
	while(it != m_RRecvInfo.recvDataList.end())
	{
		pWrapperData = &(*it).second->pPacket;
		dataLen = (*it).second->PacketSize() - m_RRecvInfo.lastOnRecvPackPos;
		assert(dataLen > 0);

		ret = OnNotifyRecvPacket(pWrapperData->wrapper_dat.data + m_RRecvInfo.lastOnRecvPackPos, dataLen);

		if (ret == dataLen)
		{
			m_RRecvInfo.lastOnRecvPackPos = 0;
			DestroyRecvData((*it).second);
			m_RRecvInfo.recvDataList.erase(it);

			++(m_RRecvInfo.minSeq);
			it = m_RRecvInfo.recvDataList.find(m_RRecvInfo.minSeq);
		}
		else if(ret > 0)
		{
			m_RRecvInfo.lastOnRecvPackPos += ret;
			break;
		}
		else//ret<=0
		{
			break;
		}
	}
}

int CNatUdt::GetSendAvailable()
{
	int ret = 0;
#ifndef UDT_WITHOUT_LOCK
	m_RSendInfoLock.Lock();
#endif
	ret = m_RSendInfo.remainedBuffSize;
#ifndef UDT_WITHOUT_LOCK
	m_RSendInfoLock.UnLock();
#endif
	return ret;
}

int CNatUdt::GetRecvAvailable()
{

#ifdef TEST_PROFILE
	static FunctionProfile __funcProfile(__FUNCTION__);
	ProfilerStackFrame __funcFrame(&__funcProfile);
#endif

	int ret = 0;
	NatUdtRecvDataList::iterator it;
#ifndef UDT_WITHOUT_LOCK
	m_RecvLock.Lock();
#endif
	it = m_RRecvInfo.recvDataList.find(m_RRecvInfo.minSeq);
	if (it != m_RRecvInfo.recvDataList.end())
	{
		ret = (*it).second->PacketSize() - m_RRecvInfo.lastOnRecvPackPos;
	}
#ifndef UDT_WITHOUT_LOCK
	m_RecvLock.UnLock();
#endif

	return ret;
}

bool CNatUdt::IsMineDatagram(const NAT_TRANSPORT_RECV_DATAGRAM* datagram)
{
	if (!IsDatagramValid(datagram))
	{
		return false;
	}

    const UDP_WRAPPER_HEAD *packetHeader = reinterpret_cast<const UDP_WRAPPER_HEAD *>(datagram->data);
    return packetHeader->category == this->m_Category
        && packetHeader->connectionID == GetConnectionID()
        && datagram->fromIp == m_RemoteInfo.ip 
        && datagram->fromPort== m_RemoteInfo.port;
}


int CNatUdt::NotifyRecvDatagram(const NAT_TRANSPORT_RECV_DATAGRAM* datagram)
{
	if (!IsDatagramValid(datagram))
	{
		return -1;
	}
	return RecvPacket((const UDP_WRAPPER_DATA *)datagram->data, datagram->dataSize);
}

bool CNatUdt::InsertRecvData( uint32 seqNum, const UDP_WRAPPER_DATA* packet, int dataSize )
{
    NatUdtRecvDataList::iterator it = m_RRecvInfo.recvDataList.find(seqNum);
    if(it != m_RRecvInfo.recvDataList.end())
    {		
        NAT_LOG_DEBUG("Udt recv packet repeat!\n");
        return false;
    }

    NAT_UDT_RECV_DATA* recvData = (NAT_UDT_RECV_DATA*)m_pRecvDataManager->GetMemory();
    memcpy(&recvData->pPacket, packet, dataSize);
    recvData->iPacketLen = dataSize;
    m_RRecvInfo.recvDataList.insert(NatUdtRecvDataList::value_type(seqNum, recvData));
    return true;

}