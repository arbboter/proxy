
#include "NatCommon.h"

#include "RelayObj.h"
#include "NatLogger.h"
#include <algorithm>

#include "NatDebug.h"

using namespace std;

CRelayObj::CRelayObj(unsigned long connectionID, unsigned long serverIp, unsigned short serverPort, unsigned char role)
:
m_recvHeader(true),
m_recvDataPos(0),
m_dataSize(0)

{
	m_RelayInfo.connectionID = connectionID;
	m_RelayInfo.serverIp = serverIp;
	m_RelayInfo.serverPort = serverPort;
	m_RelayInfo.role = role;
	m_RelayInfo.pNofitier = NULL;
	m_bAutoRecv = true;
	m_FirstConnectTime = 0;
	FD_ZERO(&m_fdSet);
	memset(&m_LinkResource,0,sizeof(NET_LINK_RESOURCE));
	NAT_LOG_DEBUG("CRelayObj::CRelayObj()\n");
}

CRelayObj::~CRelayObj()
{
	if(m_LinkResource.recvBuffer.pData)
	{
		delete []m_LinkResource.recvBuffer.pData;
		m_LinkResource.recvBuffer.pData = NULL;
	}

	if (m_LinkResource.sock != SWL_INVALID_SOCKET)
	{
		SWL_CloseSocket(m_LinkResource.sock);
		m_LinkResource.sock = SWL_INVALID_SOCKET;
	}
	NAT_LOG_DEBUG("CRelayObj::~CRelayObj()\n");
}

bool CRelayObj::Initial()
{
	int ret = 0;
	
	m_LinkResource.sock = SWL_CreateSocket(AF_INET, SOCK_STREAM, 0);
	if (SWL_INVALID_SOCKET == m_LinkResource.sock)
	{
		NAT_LOG_ERROR_SOCKET("CRelayObj::Initial()");
		return false;
	}

    m_recvHeader = true;
    m_recvDataPos = 0;
    m_dataSize = 0;
	ChangeState(STATE_INIT);

	return true;
}


void CRelayObj::SetRelayNotifier(CRelayNotifier *pNotifier)
{
	m_RelayInfo.pNofitier = pNotifier;
}

NatRunResult CRelayObj::Run()
{
	if (m_socketState == STATE_INIT)
	{
		ConnectServer();
		unsigned long firstConnectTime = m_FirstConnectTime;
  		if (PUB_IsTimeOutEx(firstConnectTime, CONNECT_TIMEOUT, Nat_GetTickCount()))
  		{
			NAT_LOG_DEBUG("Relay connection connect to server timeout!\n");
  			ChangeState(STATE_DISCONNECTED);
  		}
		return RUN_NONE;
	}
	else if(m_socketState == STATE_DISCONNECTED)
	{
		return RUN_DO_FAILED;
	}
	else if(m_socketState == STATE_CLOSED)
	{
		return RUN_NONE;
	}

	int ret = 0;

	timeval timeOut;
	fd_set tempReadSet;


	tempReadSet = m_fdSet;

	timeOut.tv_sec = 0;
	timeOut.tv_usec = 0;

	ret = select(m_maxSockFd+1, &tempReadSet, NULL, NULL, &timeOut);
	if(ret > 0)
	{
		//在此次读检查过程中，有连接进行了读操作，bProcessedData便置为true，以便马上进入下一次读检查
		bool bProcessedData = false;

		m_BuffLock.Lock();
		//接收缓冲区还有空间
		if(m_LinkResource.recvBuffer.dataSize <m_LinkResource.recvBuffer.bufferSize)
		{
			ret = RecvBuff(m_LinkResource.recvBuffer.pData+m_LinkResource.recvBuffer.dataSize, \
				m_LinkResource.recvBuffer.bufferSize - m_LinkResource.recvBuffer.dataSize);

			if(ret > 0)
			{
				m_LinkResource.recvBuffer.dataSize += ret;
				//if (m_bAutoRecv)
				{
					HandleRecv(&m_LinkResource.recvBuffer);
				}
				ret = RUN_DO_MORE;
			}
			else if(ret < 0)
			{
				ChangeState(STATE_DISCONNECTED);
				//TODO断开连接
				ret = RUN_DO_FAILED;
			}
			else
			{
				ret = RUN_NONE;
			}
		}
        else
        {
            HandleRecv(&m_LinkResource.recvBuffer);
            ret = RUN_NONE;
        }
		m_BuffLock.UnLock();
	}
	else if(ret == 0)
	{
		m_BuffLock.Lock();
		HandleRecv(&m_LinkResource.recvBuffer);
		m_BuffLock.UnLock();
		ret = RUN_NONE;
	}
	else//select < 0
	{
		ChangeState(STATE_DISCONNECTED);
		//if (m_bAutoRecv)
		{
			m_BuffLock.Lock();
			HandleRecv(&m_LinkResource.recvBuffer);
			m_BuffLock.UnLock();
		}
		ret = RUN_DO_FAILED;
	}

	return static_cast<NatRunResult>(ret);
}

int CRelayObj::SendBuff( const void *pBuf, int iLen )
{
	if (!pBuf || iLen < 0 || m_LinkResource.sock == SWL_INVALID_SOCKET)
	{
		return 0;
	}
	int iRet = 0;
	int iLeft = iLen;    //还要发送的数据长度
	const char *pTmp = static_cast<const char*>(pBuf);


	assert(0 < iLeft);
	assert(iLeft <= iLen);
	assert(pTmp >= pBuf);
	assert(pTmp <= (static_cast<const char*>(pBuf) + iLen));

	iRet = SWL_Send(m_LinkResource.sock, pTmp , iLeft, 0);

	if ( 0 < iRet)
	{
        // send succeed!
	}
	else if( SWL_SOCKET_ERROR == iRet)
	{
		//如果是暂时没有数据
		if(SWL_EWOULDBLOCK())
		{
            iRet = 0;							
		}
		else
		{
			NAT_LOG_ERROR_SOCKET("Relay send error!");
			return -1;
		}
	}
	else	// 0 == iRet
	{
		//不会发送长度为0的数据，所以不可能有为0的返回值 ??
		NAT_LOG_DEBUG("Relay Connection send return 0?\n");
	}
	
	return iRet;
}

int CRelayObj::Send( const void *pBuf, int iLen )
{
	int ret = -1;
	if (IsConnected() && pBuf && iLen >= 0)
	{
		int tmpRet = 0;
		if (m_Ctrl.sendRemainSize == 0)//需要发送头信息
		{
			m_CmdHeader.cmdID = RELAY_CMD_DATA;
			m_CmdHeader.cmdDataSize = iLen;
			ret = SendBuff(&m_CmdHeader, sizeof(RELAY_CMD_HEADER));
			if (ret < 0)
			{
				NAT_LOG_INFO("Relay connection change to disconnected: socket send data failed! RelayServer=(%s:%d), ConnectionId=%d\n",
					m_RelayInfo.serverIp, m_RelayInfo.serverPort);
				ChangeState(STATE_DISCONNECTED);
				return ret;
			}
			else if(ret == 0)
			{
				//NAT_LOG_DEBUG("%s %d send blocked\n", __FILE__, __LINE__);
				return 0;
			}
			while(ret != sizeof(RELAY_CMD_HEADER))//头一旦发了一半就一定要等待发完
			{
				tmpRet = SendBuff((char*)&m_CmdHeader + ret, sizeof(RELAY_CMD_HEADER) - ret);
				if (tmpRet < 0)
				{
					NAT_LOG_INFO("Relay connection change to disconnected: socket send data failed! RelayServer=(%s:%d), ConnectionId=%d\n",
						m_RelayInfo.serverIp, m_RelayInfo.serverPort);
					ChangeState(STATE_DISCONNECTED);
					return tmpRet;
				}
				ret += tmpRet;
			}
			m_Ctrl.sendRemainSize = iLen;
		}

		assert(iLen >= m_Ctrl.sendRemainSize);
		if (iLen < m_Ctrl.sendRemainSize)
		{
			NAT_LOG_INFO("Relay connection change to disconnected: input send data len invalid! RelayServer=(%s:%d), ConnectionId=%d\n",
				m_RelayInfo.serverIp, m_RelayInfo.serverPort);
			ChangeState(STATE_DISCONNECTED);
			return ret;
		}

		ret = SendBuff(pBuf, m_Ctrl.sendRemainSize);

		if (ret < 0)
		{
			NAT_LOG_INFO("Relay connection change to disconnected: socket send data failed! RelayServer=(%s:%d), ConnectionId=%d\n",
				m_RelayInfo.serverIp, m_RelayInfo.serverPort);
			ChangeState(STATE_DISCONNECTED);
			return ret;
		}
		else
		{
			m_Ctrl.sendRemainSize -= ret;
		}
	}

	return ret;
}


int CRelayObj::RecvBuff( void *pBuf, int iLen )
{
	if (!pBuf || iLen <= 0)
	{
		return 0;
	}

	int iRet = 0;
	int iLeft = iLen;
	char *pTmp = reinterpret_cast<char*>(pBuf);

	iRet = SWL_Recv(m_LinkResource.sock, pTmp, iLeft, 0);
	if (0 < iRet)
	{
		pTmp += iRet;
		iLeft -= iRet;
	}
	else if( SWL_SOCKET_ERROR == iRet)
	{
		if(SWL_EWOULDBLOCK())
		{

		}
		else
		{
			NAT_LOG_ERROR_SOCKET("Relay Connection recv error!");
			return -1;
		}			
	}
	else //0 == iRet
	{
		NAT_LOG_DEBUG("Relay Connection has been closed gracefully?");
		return -1;
	}
	
	return iLen - iLeft;
}

int CRelayObj::HandleRecv( RELAY_RECV_DATA_BUFFER *pDataBuffer )
{
	if (!pDataBuffer)//连接已断开
	{
		// TODO
		return 0;
	}

	int ret = 0;

	while(pDataBuffer->dataSize != 0)
	{
		if (m_recvHeader)
        {
            if (pDataBuffer->dataSize >= sizeof(RELAY_CMD_HEADER))
            {
                memcpy(&m_header, pDataBuffer->pData, sizeof(RELAY_CMD_HEADER));
                memmove(pDataBuffer->pData, pDataBuffer->pData + sizeof(RELAY_CMD_HEADER), 
                    pDataBuffer->dataSize - sizeof(RELAY_CMD_HEADER));
                pDataBuffer->dataSize -= sizeof(RELAY_CMD_HEADER);
                // TODO: 判断header的有效性（包括命令ID，版本号）
                m_recvHeader = false;
                m_recvDataPos = 0;
            }
            else
            {
                break;
            }

        }
        else
        {
            if (RELAY_CMD_CONNETCT_ACK == m_header.cmdID)
            {
                if (pDataBuffer->dataSize >= sizeof(unsigned long))
                {

                    unsigned long *pAckStatus = (unsigned long *)(pDataBuffer->pData);
                    
                    // 处理 连接应答
                    NAT_LOG_INFO("Onconnect %lu   ID:%d\n", *pAckStatus, m_RelayInfo.connectionID);
                    if (*pAckStatus == 0)
                    {
						ChangeState(STATE_CONNECTED);//状态转变要保证在回调的前面
                        m_RelayInfo.pNofitier->RelayOnConnect(this, 0);
                    }
                    else
                    {
						ChangeState(STATE_DISCONNECTED);
                        NAT_LOG_ERROR_SOCKET("Relay Connect Err");
                    }

					memmove(pDataBuffer->pData, pDataBuffer->pData + sizeof(unsigned long), 
						pDataBuffer->dataSize  - sizeof(unsigned long));

					pDataBuffer->dataSize -= sizeof(unsigned long);

                    m_recvHeader = true;
                }
                else
                {
                    break;
                }

            }
            else if (RELAY_CMD_DATA == m_header.cmdID)
            {
                int dataFreeSize = RECV_DATA_BUF_SIZE - m_dataSize;
                if (dataFreeSize > 0)
                {
                    int recvLen = min((int)(m_header.cmdDataSize - m_recvDataPos), (int)pDataBuffer->dataSize);
                    recvLen = min(recvLen, dataFreeSize);

                    memcpy(m_data + m_dataSize, pDataBuffer->pData, recvLen);

                    memmove(pDataBuffer->pData, pDataBuffer->pData + recvLen, pDataBuffer->dataSize  - recvLen);
                    pDataBuffer->dataSize -= recvLen;
                    m_recvDataPos += recvLen;
                    m_dataSize += recvLen;

                    if (m_header.cmdDataSize == m_recvDataPos)
                    {
                        m_recvHeader = true;
                        m_recvDataPos = 0;
                    }
                }
                else
                {
                    // data buf is full!
                    break;
                }
            }
			else
			{
				// error cmd
				// ignore
				m_recvHeader = true;
				m_recvDataPos = 0;
			}

        }

    }

	return 0;
}

int CRelayObj::Recv( void *pBuf, int iLen )
{
	if (!pBuf || iLen <= 0)
	{
		return 0;
	}

	if (!IsConnected())
	{
		return -1;
	}

	int ret = 0;

	m_BuffLock.Lock();
	if(m_dataSize > 0)
	{
        assert(m_data != NULL);
        ret = min(iLen, m_dataSize);
		memcpy(pBuf, m_data, ret);
		memmove(m_data, m_data + ret, m_dataSize - ret);
		m_dataSize -= ret;
	}
	m_BuffLock.UnLock();

	return ret;
}

int CRelayObj::GetSendAvailable()
{
	if (!IsConnected())
	{
		return -1;
	}
	timeval timeOut;
	timeOut.tv_sec = 0;
	timeOut.tv_usec = 0;

	fd_set tempReadSet;
	tempReadSet = m_fdSet;

	return select(m_maxSockFd+1, NULL, &tempReadSet, NULL, &timeOut);
}

int CRelayObj::GetRecvAvailable()
{
	if (!IsConnected())
	{
		return -1;
 	}

	return m_dataSize;
}

int CRelayObj::SetAutoRecv( bool bAutoRecv )
{
	m_bAutoRecv = bAutoRecv;

	return 0;
}

int CRelayObj::Close()
{
	NAT_LOG_DEBUG("CRelayObj::Close()\n");

	CNatSocketBase::Close();

	return 0;
}

unsigned long CRelayObj::GetRemoteIp()
{
	return m_RelayInfo.serverIp;
}

unsigned short CRelayObj::GetRemotePort()
{
	return m_RelayInfo.serverPort;
}

int CRelayObj::ConnectServer()
{
	struct sockaddr_in sockAddr;
	memset(&sockAddr, 0, sizeof(sockaddr_in));
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(m_RelayInfo.serverPort);
	sockAddr.sin_addr.s_addr = m_RelayInfo.serverIp;

	if (0 == m_FirstConnectTime)
	{
		m_FirstConnectTime = Nat_GetTickCount();
	}
	if (0 != SWL_Connect(m_LinkResource.sock, (const struct sockaddr *)&sockAddr, sizeof(struct sockaddr_in), 0))
	{
		return 0;
	}

	ChangeState(STATE_CONNECTING);
	m_maxSockFd = m_LinkResource.sock;
	FD_SET(m_LinkResource.sock, &m_fdSet);

	m_LinkResource.recvBuffer.pData = new char[RELAY_RECV_BUF_LEN];
	m_LinkResource.recvBuffer.bufferSize = RELAY_RECV_BUF_LEN;
	m_LinkResource.recvBuffer.dataSize = 0;
	memset(m_LinkResource.recvBuffer.pData, 0, RELAY_RECV_BUF_LEN);

	int sendLen = sizeof(RELAY_CMD_HEADER)+sizeof(RELAY_CMD_CONNECT_DATA);
	char *pConnctCmd = new char[sendLen];
	RELAY_CMD_HEADER *pCmdHeader = (RELAY_CMD_HEADER *)pConnctCmd;
	RELAY_CMD_CONNECT_DATA *pConnectData = (RELAY_CMD_CONNECT_DATA *)(pConnctCmd + sizeof(RELAY_CMD_HEADER));
	pCmdHeader->cmdID = RELAY_CMD_CONNETCT;
	pCmdHeader->cmdDataSize = sizeof(RELAY_CMD_CONNECT_DATA);
	pConnectData->connectionID =m_RelayInfo.connectionID;
	pConnectData->deviceNo = m_RelayInfo.deviceNo;
	pConnectData->role = m_RelayInfo.role;

	int ret = SendBuff(pConnctCmd, sendLen);
	if (ret != sendLen)//因为连接刚建立，则协议栈肯定能放下sizeof(RELAY_CONNECTION_CMD)
	{
		assert(false);
		delete []pConnctCmd;
		pConnctCmd = NULL;
		return 0;
	}
	delete []pConnctCmd;
	pConnctCmd = NULL;

	m_Ctrl.recvRemainSize = 0;
	m_Ctrl.sendRemainSize = 0;
    return 1;
}

void CRelayObj::ChangeState(SocketState state)
{
	if (m_socketState != state)
	{
		NAT_LOG_DEBUG("Old State: %d   New State: %d\n", m_socketState, state);
		SocketState oldState = m_socketState;
		CNatSocketBase::ChangeState(state);
		if (state == STATE_DISCONNECTED && (oldState == STATE_CONNECTING || oldState == STATE_INIT))
		{
			//只有在没有成功回调的情况下，才回调-1
			assert(m_maxSockFd != 0);
			FD_CLR(m_LinkResource.sock, &m_fdSet);
			m_maxSockFd = 0;
			m_RelayInfo.pNofitier->RelayOnConnect(this, -1);
		}
	}
}
