// SWL_ListenProcEx.cpp: implementation of the CSWL_ListenProcEx class.
//
//////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <string.h>

#ifndef	WIN32
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#endif

#include "SWL_Public.h"
#include "SWL_ListenProcEx.h"
#ifdef __DEBUG_NEW__
#include "debug_new.h"
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSWL_ListenProcEx::CSWL_ListenProcEx(GET_ACCEPT_LINK_CALLBACK pCallback, void* pParam)
{
	//在accept之后，没有用回调函数做处理，程序会资源泄露，因此回调函数不能为空
	assert(NULL != pCallback);
	m_pAcceptCallback = pCallback;
	m_pParam = pParam;
	m_SocketListen = SWL_INVALID_SOCKET;
	m_AcceptThreadID = PUB_THREAD_ID_NOINIT;
	m_bAcceptThreadRun = false;
	m_bListenStart = false;
	PUB_InitLock(&m_ListenLock);
}

CSWL_ListenProcEx::~CSWL_ListenProcEx()
{
	PUB_DestroyLock(&m_ListenLock);
}
 
//创建socket,并开始监听端口,创建accept线程，accept线程结束的时候socket被销毁
//return value: 0 成功  1 创建socket失败 2 获取主机名或者获取主机IP地址失败
//				3 绑定socket失败  4 监听失败  5 创建监听线程失败
//				6 已经开始了监听
int CSWL_ListenProcEx::StartListen(unsigned short int nPort)
{
	//本函数StartListen的返回结果
	int iReturn = 0;
	int iLoop = 0;
	CPUB_LockAction   FunctionLock(&m_ListenLock); //给函数加了锁

	//如果已经开始了监听，则直接返回
	if (m_bListenStart)
	{
		iReturn = 6;
		goto StartListen_end;
	}

#ifdef __CUSTOM_IPV6__
	m_SocketListen = CreateIPV6Socket(nPort);
#else
	m_SocketListen = CreateIPV4Socket(nPort);
#endif
	if (SWL_INVALID_SOCKET == m_SocketListen)
	{
		iReturn = 1;
		goto StartListen_end;
	}
	
	//监听
	
	while(0 != SWL_Listen(m_SocketListen, 5/*SOMAXCONN*/))
	{
#ifdef WIN32
		if ( (WSAEINPROGRESS == WSAGetLastError()) && (iLoop < 3) )
#else
		if(0)
#endif
		{
			SWL_PrintError(__FILE__,__LINE__);	
			PUB_Sleep(10000);
		}
		else
		{
			SWL_PrintError(__FILE__,__LINE__);		
			iReturn = 4;
			goto StartListen_end_close_sock;
		}

		++iLoop;
	}

	//启动accept的线程
	m_AcceptThreadID = PUB_CreateThread(StartAccept, (void *)this, &m_bAcceptThreadRun);
	if( PUB_CREATE_THREAD_FAIL == m_AcceptThreadID )
	{
		PUB_PrintError(__FILE__,__LINE__);
		iReturn = 5;
		goto StartListen_end_close_sock;
	}

	//启动监听成功了
	iReturn = 0;
	m_bListenStart = true;
	goto StartListen_end;

	//在创建socket后，后面的阶段出错则要把创建的socket销毁
StartListen_end_close_sock:    
	SWL_CloseSocket(m_SocketListen);
	m_SocketListen = SWL_INVALID_SOCKET;
	
StartListen_end:
	return iReturn;
}

//停止监听端口,销毁socket,socket的销毁放在了accept线程的结束
//return value: 0 成功
//				1 已经停止
int CSWL_ListenProcEx::StopListen()
{
	CPUB_LockAction   FunctionLock(&m_ListenLock);
	
	//如果已经停止了监听，则直接返回
	if (!m_bListenStart) 
	{
		return 1;
	}

	//退出accept线程
	PUB_ExitThread(&m_AcceptThreadID, &m_bAcceptThreadRun);
	m_bListenStart = false;
	return 0;
}

//accept线程，启动AcceptThreadRun
//BUGS：对于线程的退出状态没有做处理
PUB_THREAD_RESULT CSWL_ListenProcEx::StartAccept(void *pParam)
{
#ifndef	WIN32
	pid_t pid = getpid();
	pid_t tid = syscall(__NR_gettid);
	printf("%s, %d, pid = %d, tid=%d\n", __FILE__, __LINE__, pid, tid);
#endif
	assert(NULL != pParam);
	CSWL_ListenProcEx *pListenProc = reinterpret_cast<CSWL_ListenProcEx*>(pParam);
#ifdef __CUSTOM_IPV6__
	pListenProc->AcceptThread6Run();
#else
	pListenProc->AcceptThreadRun();
#endif
	
	return 0;
}

//accept线程,accept的run函数
//return value:0 线程正常
//			   1 线程退出时关闭socket失败
//			   2 accept出错
int CSWL_ListenProcEx::AcceptThreadRun()
{	
	SWL_socket_t  sClientLinkSock;
	struct sockaddr struAddr;
	SWL_socklen_t len = sizeof(struAddr);

	while(m_bAcceptThreadRun)
	{		
		sClientLinkSock = SWL_Accept(m_SocketListen, (struct sockaddr *)&struAddr, &len);

		if(SWL_INVALID_SOCKET == sClientLinkSock) 
		{
			if(SWL_EWOULDBLOCK())
			{
#ifdef __CHIP3515__
				PUB_Sleep(100);
#else
				PUB_Sleep(100);
#endif
				continue;
			}
			else
			{
				SWL_PrintError(__FILE__, __LINE__);
				assert(false);//xg 如果出错，不好处理，退出该线程后，新的客户端就进不来了
				return 2;
			}
		}

		m_clientSockInfo.tvtSock.swlSocket = sClientLinkSock;
		struct sockaddr_in * sockAddr = (struct sockaddr_in*)&struAddr;
		strcpy(m_clientSockInfo.szIP, inet_ntoa(sockAddr->sin_addr));
		m_clientSockInfo.nPort = sockAddr->sin_port;

		//调用对象构造时注册的回调函数
		m_pAcceptCallback(&m_clientSockInfo, m_pParam, NULL);		
	}

	if ( 0 != SWL_CloseSocket(m_SocketListen) )
	{
		return 1;
	}
	return 0;
}

int CSWL_ListenProcEx::AcceptThread6Run()
{
	SWL_socket_t  sClientLinkSock;
	struct sockaddr_in6 structAddrIn6;
	SWL_socklen_t len = sizeof(structAddrIn6);

	memset(&m_clientSockInfo, 0, sizeof(CLIENT_SOCK_INFO));

	while(m_bAcceptThreadRun)
	{		
		sClientLinkSock = SWL_Accept(m_SocketListen, (struct sockaddr *)&structAddrIn6, &len);

		if(SWL_INVALID_SOCKET == sClientLinkSock) 
		{
			if(SWL_EWOULDBLOCK())
			{
#ifdef __CHIP3515__
				PUB_Sleep(100);
#else
				PUB_Sleep(100);
#endif
				continue;
			}
			else
			{
				SWL_PrintError(__FILE__, __LINE__);
				assert(false);//xg 如果出错，不好处理，退出该线程后，新的客户端就进不来了
				return 2;
			}
		}

		m_clientSockInfo.tvtSock.swlSocket = sClientLinkSock;
		m_clientSockInfo.nPort = structAddrIn6.sin6_port;

		//如果是IPV4的IP地址，记录IP信息。对于IPV6的地址设置为255.255.255.255
 		unsigned char * pIpAddr = structAddrIn6.sin6_addr.s6_addr;
 		if (0x00 == pIpAddr[0] && 0x00 == pIpAddr[1] && 0x00 == pIpAddr[2] &&
 			0x00 == pIpAddr[3] && 0x00 == pIpAddr[4] && 0x00 == pIpAddr[5] &&
 			0x00 == pIpAddr[6] && 0x00 == pIpAddr[7] && 0x00 == pIpAddr[8] &&
 			0x00 == pIpAddr[9] && 0xFF == pIpAddr[10] && 0xFF == pIpAddr[11])
 		{
			sprintf(m_clientSockInfo.szIP, "%d.%d.%d.%d", pIpAddr[12], pIpAddr[13], pIpAddr[14], pIpAddr[15]);
 			//表示这是个IPV4的客户端
//  			unsigned char * pIp = (char *)&m_clientSockInfo.dwIP;
//  			pIp[0] = pIpAddr[12];
//  			pIp[1] = pIpAddr[13];
//  			pIp[2] = pIpAddr[14];
//  			pIp[3] = pIpAddr[15];
 		}
		else
		{
#ifdef WIN32
			memset(m_clientSockInfo.szIP, 0, sizeof(m_clientSockInfo.szIP));
			bool bSkiped = false;
			for(int i = 0; i < 16; i++)
			{
				if((0 == (i%2)) && (0x00 == pIpAddr[i]) && !bSkiped)
				{
					do 
					{
						i++;
					} while(0x00 == pIpAddr[i]);
					strcat(m_clientSockInfo.szIP, ":");
					bSkiped = true;
					i--;
				}
				else
				{
					if((1 == (i%2)) && (i != 15))
					{
						sprintf(m_clientSockInfo.szIP+strlen(m_clientSockInfo.szIP), "%02x:", pIpAddr[i]);
					}
					else
					{
						sprintf(m_clientSockInfo.szIP+strlen(m_clientSockInfo.szIP), "%02x", pIpAddr[i]);
					}
				}
			}
#else
			inet_ntop(AF_INET6, &structAddrIn6.sin6_addr, m_clientSockInfo.szIP, sizeof(m_clientSockInfo.szIP));
#endif
		}

		printf("%s:%s:%d, get accept for ipv6 client\n", __FUNCTION__, __FILE__, __LINE__);
		//调用对象构造时注册的回调函数
		m_pAcceptCallback(&m_clientSockInfo, m_pParam, NULL);		
	}

	if ( 0 != SWL_CloseSocket(m_SocketListen) )
	{
		return 1;
	}
	return 0;
}

SWL_socket_t CSWL_ListenProcEx::CreateIPV4Socket(unsigned short port)
{
	SWL_socket_t sockfd = SWL_INVALID_SOCKET;
	//创建一个socket，创建的所有的socket都是非阻塞的
	sockfd = SWL_CreateSocket(AF_INET, SOCK_STREAM, 0);
	if(SWL_INVALID_SOCKET == sockfd)
	{
		printf("%s:%s:%d, Create socket error\n", __FUNCTION__, __FILE__, __LINE__);
		assert(false);
		return sockfd;
	}

	struct sockaddr_in sockaddrIn4;
	memset(&sockaddrIn4, 0, sizeof(struct sockaddr_in));

	SWL_socklen_t  sockAddrLen = sizeof(struct sockaddr_in);

	sockaddrIn4.sin_family = AF_INET;
	sockaddrIn4.sin_addr.s_addr = htonl(INADDR_ANY);;
	sockaddrIn4.sin_port = htons(port); 		//这里要转成网络序
	memset(&(sockaddrIn4.sin_zero), '\0', 8);	// zero the rest of the struct

	int reuse = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int));

	//绑定socket
	int	iLoop = 0;
	while(0 != SWL_Bind(sockfd, (sockaddr*)&sockaddrIn4, sizeof(sockaddr_in)))
	{
#ifdef WIN32
		if ( (WSAEINPROGRESS == WSAGetLastError()) && (iLoop < 3) )
#else
		if(0)
#endif
		{
			SWL_PrintError(__FILE__,__LINE__);				
			PUB_Sleep(2000);
		}
		else
		{
			SWL_PrintError(__FILE__,__LINE__);
			SWL_CloseSocket(sockfd);
			sockfd = SWL_INVALID_SOCKET;
			break;
		}

		++iLoop;
	}

	return sockfd;
}

SWL_socket_t CSWL_ListenProcEx::CreateIPV6Socket(unsigned short port)
{
	SWL_socket_t  sockfd = SWL_INVALID_SOCKET;
	sockfd = SWL_CreateSocket(PF_INET6, SOCK_STREAM, 0);
	if (SWL_INVALID_SOCKET == sockfd)
	{
		printf("%s:%s:%d, Create socket error\n", __FUNCTION__, __FILE__, __LINE__);
		assert(false);
		return SWL_INVALID_SOCKET;
	}

	struct sockaddr_in6 sockaddrIn6;
	memset(&sockaddrIn6, 0, sizeof(struct sockaddr_in6));
	sockaddrIn6.sin6_family = PF_INET6;
	sockaddrIn6.sin6_port   = htons(port);
	//sockaddrIn6.sin6_addr	= in6addr_any;

	int reuse = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int));

	//reuse = 1;
	//setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &reuse, sizeof(int));
	//setsockopt(sockfd, IPPROTO_IPV6, 26, &reuse, sizeof(int));

	int	iLoop = 0;
	while(0 != SWL_Bind(sockfd, (sockaddr*)&sockaddrIn6, sizeof(struct sockaddr_in6)))
	{
#ifdef WIN32
		if ( (WSAEINPROGRESS == WSAGetLastError()) && (iLoop < 3) )
#else
		if(0)
#endif
		{
			SWL_PrintError(__FILE__,__LINE__);				
			PUB_Sleep(2000);
		}
		else
		{
			SWL_PrintError(__FILE__,__LINE__);
			SWL_CloseSocket(sockfd);
			sockfd = SWL_INVALID_SOCKET;
			break;
		}

		++iLoop;
	}

	return sockfd;
}


