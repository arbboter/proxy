// NatUdtTransport.cpp: implements for the CNatUdtTransport class.
//
//////////////////////////////////////////////////////////////////////

#include "NatCommon.h"
#include "NatUdtTransport.h"
#include "NatHeapDataManager.h"

#include "NatDebug.h"

#ifdef __ENVIRONMENT_WIN32__

// MS Transport Provider IOCTL to control
// reporting PORT_UNREACHABLE messages
// on UDP sockets via recv/WSARecv/etc.
// Path TRUE in input buffer to enable (default if supported),
// FALSE to disable.
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12)

#ifndef ENETUNREACH
#define ENETUNREACH             WSAENETUNREACH
#endif//WSAENETUNREACH

#endif //__ENVIRONMENT_WIN32__

inline int Nat_GetLastSockError()
{
#ifdef            __ENVIRONMENT_WIN32__
    return WSAGetLastError();
#elif defined     __ENVIRONMENT_LINUX__
    return errno;
#endif
}


////////////////////////////////////////////////////////////////////////////////
// global function implements

unsigned short GenRandomPort()
{
    return static_cast<unsigned short>(Nat_Rand()) % 55535 + 10000;
}

SWL_socket_t NatCreateUdpSocket()
{
    SWL_socket_t sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (SWL_INVALID_SOCKET == sock) 
    {
        NAT_LOG_ERROR_SYS("NatCreateUdpSocket failed!");
        return SWL_INVALID_SOCKET;
    }

//     int  opt = 1;
//     if(0 != setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(int)))
    //     {
//         NAT_LOG_ERROR_SOCKET("NatCreateUdpSocket failed!\n");
//         SWL_CloseSocket(sock);
//         return SWL_INVALID_SOCKET;
//     }

#ifdef  __ENVIRONMENT_WIN32__
    DWORD dwBytesReturned = 0;
    BOOL bNewBehavior = FALSE;

    // disable  new behavior using
    // IOCTL: SIO_UDP_CONNRESET
    if (0 != WSAIoctl(sock, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior),
        NULL, 0, &dwBytesReturned, NULL, NULL))
    {
        NAT_LOG_ERROR_SOCKET("NatCreateUdpSocket failed!\n");
        SWL_CloseSocket(sock);
        return SWL_INVALID_SOCKET;
    }
#endif

#ifdef  __ENVIRONMENT_LINUX__
    int iSave = fcntl(sock,F_GETFL);
    fcntl(sock, F_SETFL, iSave | O_NONBLOCK);
#elif defined  __ENVIRONMENT_WIN32__
    unsigned long dwVal = 1; //非0为非阻塞模式
    ioctlsocket(sock, FIONBIO, &dwVal);
#endif

    return sock;
}

SWL_socket_t CreateAndBindUdpSock( unsigned long ip, unsigned short int port)
{
    SWL_socket_t sock = NatCreateUdpSocket();
    if (SWL_INVALID_SOCKET == sock) 
    {
        return SWL_INVALID_SOCKET;
    }

    struct sockaddr_in struLocalInfo;
    SWL_socklen_t  InfoLen = sizeof(struLocalInfo);

    //为绑定socket填写信息
    struLocalInfo.sin_family = AF_INET;
    struLocalInfo.sin_addr.s_addr = ip;
    struLocalInfo.sin_port = htons(port); 	//这里要转成网络序
    memset(&(struLocalInfo.sin_zero), '\0', 8); // zero the rest of the struct

#ifdef __ENVIRONMENT_LINUX__
//     if(0 != setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, pIfName, strlen(pIfName) + 1))
//     {
//         NAT_LOG_ERROR_SOCKET("setsockopt failed");
//         SWL_CloseSocket(sock);		
//         return SWL_INVALID_SOCKET;
//     }
#endif

    //绑定socket
    if(0 != SWL_Bind(sock, (sockaddr*)&struLocalInfo, InfoLen))
    {
        NAT_LOG_ERROR_SOCKET("NAT UDP Bind Error"); 
        SWL_CloseSocket(sock);
        return SWL_INVALID_SOCKET;
    }

    int opt = MAX_RECVBUFF;
    int optlen = sizeof(int);
    int iRet = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char*>(&opt), optlen);
    if (iRet != 0)
    {
        NAT_LOG_ERROR_SOCKET("NAT setsockopt MAX_RECVBUFF Error"); 
    }

    opt = MAX_SENDBUFF;
    iRet = setsockopt(sock, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&opt), optlen);
    if (iRet != 0)
    {
        NAT_LOG_ERROR_SOCKET("NAT setsockopt MAX_SENDBUFF Error");  
    }

    return sock;
}

int NatUdpSendTo(SWL_socket_t sock, const void *pData, int iLen, unsigned long dwIP, unsigned short nPort)
{
    int ret;
    struct sockaddr_in struTo;
    SWL_socklen_t  toLen = sizeof(struTo);
    struTo.sin_family = AF_INET;
    struTo.sin_addr.s_addr = dwIP;
    struTo.sin_port = htons(nPort); 
    memset(&(struTo.sin_zero), '\0', 8); // zero the rest of the struct
#ifdef __ENVIRONMENT_LINUX__
    ret = sendto(sock, pData, iLen, MSG_DONTWAIT | MSG_NOSIGNAL, reinterpret_cast<sockaddr*>(&struTo), toLen);
#elif defined __ENVIRONMENT_WIN32__
    ret = sendto(sock, reinterpret_cast<const char*>(pData), iLen, 0, reinterpret_cast<sockaddr*>(&struTo), toLen);
#endif
    if (SWL_SOCKET_ERROR == ret)    
    {
        if (SWL_EWOULDBLOCK())
        {
            NAT_LOG_DEBUG("NatUdpSendTo Block, the sys buf is full!\n");  
            return 0;
        }
        else if (ENETUNREACH == Nat_GetLastSockError())
        {
            // UDP is connectionless, ignore the connect error!
            return 0;
        }        
        else
        {
            NAT_LOG_ERROR_SOCKET("NatUdpSendTo Error");  
            return -1;
        }

    }
    return ret;
}

int NatUdpRecvfrom(SWL_socket_t sock, void* pBuff, int iLen, unsigned long *pIP, 
                     unsigned short *pPort, unsigned long dwTimeOut)
{
    int iRet = 0;
//     if (0 != dwTimeOut)
//            //检查sock是否连接上	
        fd_set  rdFds;
        FD_ZERO(&rdFds);
        FD_SET(sock, &rdFds);

        struct timeval	struTimeout;		
        struTimeout.tv_sec = dwTimeOut / 1000;  //取秒
        struTimeout.tv_usec = (dwTimeOut % 1000 ) * 1000; //取微妙
        iRet = select((int)sock + 1, &rdFds, NULL, NULL, &struTimeout);		
        if ( 0 > iRet ) //异常或者超时
        {
            NAT_LOG_ERROR_SOCKET("NatUdpRecvfrom select Error");  
            return -1;
        }
        else if(0 == iRet)
        {
            return 0;	
        }
        //		else 就往下走
//    }

    struct sockaddr_in struFrom;
    SWL_socklen_t  fromLen = sizeof(struFrom);
    iRet = recvfrom(sock, reinterpret_cast<char*>(pBuff), iLen, 0, reinterpret_cast<sockaddr*>(&struFrom), &fromLen);
    if(-1 == iRet)
    {
        if (SWL_EWOULDBLOCK())
        {
            return 0;
        }
        else
        {
            NAT_LOG_ERROR_SOCKET("NatUdpRecvfrom recvfrom Error");            
            return -1;
        }		
    }

    if (NULL != pIP)
    {
        *pIP = struFrom.sin_addr.s_addr;
    }

    if (NULL != pPort)
    {
        *pPort = ntohs(struFrom.sin_port);
    }	
    return iRet;	
}

//////////////////////////////////////////////////////////////////////////
//CNatUdtTransport

CNatUdtTransport::CNatUdtTransport()
:
m_started(false),
m_config(),
m_notifier(NULL),
m_sock(SWL_INVALID_SOCKET),
m_randInited(false)
{
    memset(&m_recvDatagram, 0, sizeof(m_recvDatagram)); 
}

CNatUdtTransport::~CNatUdtTransport()
{
    InternalStop();
}

int CNatUdtTransport::Start( const NAT_UDT_TRANSPORT_CONFIG* config )
{
    if(m_started)
    {
        return 0;
    }

    m_config = *config;

    if (0 != m_config.localPort)
    {
        m_sock = CreateAndBindUdpSock(m_config.localIp, m_config.localPort);
        if (SWL_INVALID_SOCKET == m_sock )
        {
            InternalStop();
            return -1;
        }
    }

#ifdef USE_UDT_TRANSPORT_STAT
    memset(&stat, 0, sizeof(stat));
    updater.startTimeSec= time(NULL);
    updater.lastUpdateTimeSec = updater.startTimeSec;
    memset(&updater.lastStat, 0, sizeof(updater.lastStat));
#endif

    m_randInited = false;
    m_started = true;
    return 0;
}

void CNatUdtTransport::Stop()
{
    if (m_started)
    {
        InternalStop();
        m_started = false;
    }
}

NatRunResult CNatUdtTransport::Run()
{
    if (!m_started)
    {
        return RUN_NONE;
    }

#ifdef USE_UDT_TRANSPORT_STAT
    UpdateStat();
#endif

    if (0 == m_config.localPort && SWL_INVALID_SOCKET == m_sock)
    {
        static const int RAND_BIND_TIMES_MAX = 3;
        int localPort;
        int randBindTimes = 0;
        if (!m_randInited)
        {
            Nat_SRand();
            m_randInited = true;
        }
        while(SWL_INVALID_SOCKET == m_sock && randBindTimes < RAND_BIND_TIMES_MAX)
        {
            localPort = GenRandomPort();
            m_sock = CreateAndBindUdpSock(m_config.localIp, localPort);
            randBindTimes++;
        }
        if (SWL_INVALID_SOCKET == m_sock)
        {
            return RUN_NONE;
        }
        NAT_LOG_DEBUG("Udt Transport bind random port ok! LocalPort = %d\n",localPort);
        m_config.localPort = localPort;
    }

	if(SWL_INVALID_SOCKET != m_sock)
	{
		return RecvDatagram();
	}
    return RUN_NONE;
}

int CNatUdtTransport::Send(unsigned long destIP, unsigned short destPort, 
                           const void* packet, int iLen)
{
    if (!m_started)
    {
        return -1;
    }

    if (SWL_INVALID_SOCKET == m_sock)
    {
        if (0 == m_config.localPort)
        {
            return 0;
        } 
        else
        {
            return -1;
        }
    }

    int ret = NatUdpSendTo(m_sock, packet, iLen, destIP, destPort);

#ifdef USE_UDT_TRANSPORT_STAT
    if (ret > 0)
    {
        stat.totalSendPacket++;
        stat.totalSendSize += iLen;
    }
#endif
    return ret;
}

unsigned long CNatUdtTransport::GetLocalIp()
{
    return m_config.localIp;
}

unsigned short CNatUdtTransport::GetLocalPort()
{
    return m_config.localPort;
}

CNatUdtTransportNotifier* CNatUdtTransport::GetNotifier()
{
    return m_notifier;
}

void CNatUdtTransport::SetNotifier(CNatUdtTransportNotifier* notifier)
{
    m_notifier = notifier;
}

bool CNatUdtTransport::IsStarted()
{
    return m_started;
}

void CNatUdtTransport::InternalStop()
{
    if (SWL_INVALID_SOCKET != m_sock)
    {
        SWL_CloseSocket(m_sock);
    }
}

NatRunResult CNatUdtTransport::RecvDatagram()
{
    NatRunResult runResult = RUN_NONE;

    int ret = NatUdpRecvfrom(m_sock, m_recvDatagram.data, sizeof(m_recvDatagram.data), 
        &m_recvDatagram.fromIp, &m_recvDatagram.fromPort, 0);
    if (ret > 0)
    {
#ifdef USE_UDT_TRANSPORT_STAT
        stat.totalRecvPacket++;
        stat.totalRecvSize += ret;
#endif
        assert(ret <= sizeof(m_recvDatagram.data));
        m_recvDatagram.dataSize = ret;
        runResult = RUN_DO_MORE;
        NotifyOnRecvDatagram();
    }
    else if (ret < 0)
    {
        runResult = RUN_DO_FAILED;
    }
    return runResult;
}

void CNatUdtTransport::NotifyOnRecvDatagram()
{
    if (NULL != m_notifier)
    {
        m_notifier->OnRecvDatagram(this, &m_recvDatagram);
    }
}


#ifdef USE_UDT_TRANSPORT_STAT
void CNatUdtTransport::UpdateStat()
{
    unsigned long long curTime = time(NULL);
    int disTime = curTime - updater.lastUpdateTimeSec;
    if (disTime > STAT_UPDATE_INTERVAL)
    {
        long long totalTime = curTime - updater.startTimeSec;
        NAT_LOG_DEBUG("CNatUdtTransport: Stat: **************************************");
        NAT_LOG_DEBUG("CNatUdtTransport: Stat: TotalSendPacket=%llu", stat.totalSendPacket);
        NAT_LOG_DEBUG("CNatUdtTransport: Stat: TotalRecvPacket=%llu", stat.totalRecvPacket);
        NAT_LOG_DEBUG("CNatUdtTransport: Stat: TotalSendSize=%llu", stat.totalSendSize);
        NAT_LOG_DEBUG("CNatUdtTransport: Stat: TotalRecvSize=%llu", stat.totalRecvSize);
        NAT_LOG_DEBUG("CNatUdtTransport: Stat: TotalTime=%lld s", totalTime);
        assert(totalTime != 0);
        NAT_LOG_DEBUG("CNatUdtTransport: Stat: AvgSendPacket=%f pack/s", stat.totalSendPacket / (double)totalTime);
        NAT_LOG_DEBUG("CNatUdtTransport: Stat: AvgRecvPacket=%f pack/s", stat.totalRecvPacket / (double)totalTime);
        NAT_LOG_DEBUG("CNatUdtTransport: Stat: AvgSendSize=%f KB/s", stat.totalSendSize / (double)totalTime / 1024.0);
        NAT_LOG_DEBUG("CNatUdtTransport: Stat: AvgRecvSize=%f KB/s", stat.totalRecvSize / (double)totalTime / 1024.0);
        assert(disTime != 0);
        NAT_LOG_DEBUG("CNatUdtTransport: Stat: CurSendPacket=%f pack/s", 
            (stat.totalSendPacket - updater.lastStat.totalSendPacket) / (double)disTime);
        NAT_LOG_DEBUG("CNatUdtTransport: Stat: CurRecvPacket=%f pack/s",
            (stat.totalRecvPacket - updater.lastStat.totalRecvPacket) / (double)disTime);
        NAT_LOG_DEBUG("CNatUdtTransport: Stat: CurSendSize=%f KB/s",
            (stat.totalSendSize - updater.lastStat.totalSendSize) / (double)totalTime / 1024.0);
        NAT_LOG_DEBUG("CNatUdtTransport: Stat: CurRecvSize=%f KB/s",
            (stat.totalRecvSize - updater.lastStat.totalRecvSize) / (double)totalTime / 1024.0);

        updater.lastStat = stat;
        updater.lastUpdateTimeSec = curTime;
    }
}
#endif