
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "SWL_Public.h"
#include <assert.h>

#ifdef WIN32
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#else
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#endif
#ifdef __DEBUG_NEW__
#include "debug_new.h"
#define new DEBUG_NEW
#endif


#ifdef WIN32
void PrintDebugInfo(const char *pText, DWORD dwError)
{
#if defined(_DEBUG)
	if(dwError == 0)
	{
		return;
	}
	FILE *pFile = fopen("C:\\DebugInfo.log", "ab+");
	if (NULL == pFile)
	{
		return;
	}
	char szText[256];
	sprintf(szText, "%s %d: ",pText, dwError);
	fwrite(szText, strlen(szText)+1, 1, pFile);
	LPVOID lpMsgBuf;
	int nLength = FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
		);
	fwrite((LPCTSTR)lpMsgBuf, nLength+1, 1, pFile);
	// Free the buffer.
	LocalFree( lpMsgBuf );
	strcpy(szText, "\r\n");
	fwrite(szText, strlen(szText)+1, 1, pFile);
	fclose(pFile);

#endif /////#if defined(_DEBUG)  end
}
#endif

static unsigned long s_dwInit = 0;

//return value: 0 成功 1 失败
int SWL_Init()
{
	if(s_dwInit > 0)
	{
		++s_dwInit;
		return 0;
	}

#ifdef  WIN32
	//windows上需要对WS2_32.DLL库进行初始化
	WSADATA wsaData;
	if(SOCKET_ERROR == WSAStartup(0x202,&wsaData)) 
	{
		printf("%s %d last errno = %d\n", __FILE__, __LINE__, WSAGetLastError());
		return 1;
	}
#else
	signal(SIGPIPE, SIG_IGN);
#endif
	++s_dwInit;
	return 0;
}

int SWL_Quit()
{
	if (s_dwInit > 0)
	{
		--s_dwInit;
	}
	else
	{
		return 0;
	}
#ifdef  WIN32
	//windows上需要清除WS2_32.DLL库
	if(SOCKET_ERROR == WSACleanup())
	{
		printf("%s %d last errno = %d\n", __FILE__, __LINE__, WSAGetLastError());
		return 1;
	}
#endif	
	return 0;
}

void SWL_PrintError(const char* pFile, int iLine)
{
#ifdef  WIN32
	int iErrno = WSAGetLastError();
	printf("%s %d Errno = %d\n", pFile, iLine, iErrno);
#else
	char szErrorPos[256] = {0};
	sprintf(szErrorPos, "%s %d ", pFile, iLine);
	perror(szErrorPos);
#endif 
}
 
//return value: 成功：非SWL_UNINVALID_SOCKET  
//			    失败：SWL_UNINVALID_SOCKET  
//如果失败，则不会改变 *pSocket的值
SWL_socket_t SWL_CreateSocket(int iDomain, int iType, int iProtocol)
{
	SWL_socket_t sock = socket(iDomain, iType, iProtocol);
	if (SWL_INVALID_SOCKET == sock) 
	{
#ifdef WIN32
		PrintDebugInfo("SWL_socket_t sock = socket(iDomain, iType, iProtocol)", WSAGetLastError());
#endif
		return SWL_INVALID_SOCKET;
	}
	int  opt = 1;
	if(0 != setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(int)))
	{
		SWL_PrintError(__FILE__, __LINE__);
#ifdef WIN32
		PrintDebugInfo("setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(int))", WSAGetLastError());
#endif
		return SWL_INVALID_SOCKET;
	}

#ifdef  WIN32
	unsigned long dwVal = 1; //非0为非阻塞模式
	ioctlsocket(sock, FIONBIO, &dwVal);
#else
	int iSave = fcntl(sock,F_GETFL);
	fcntl(sock, F_SETFL, iSave | O_NONBLOCK);
#endif
	return sock;
}

//return value: 0 成功 -1 失败
int SWL_CloseSocket(SWL_socket_t sock)
{
#ifdef   WIN32
	return closesocket(sock);
#else
	return close(sock);
#endif
}

//关闭已经连接上的socket
//return value: 0 成功 -1
//输入参数：iHow：	SWL_SHUT_RD 关闭时，后续的读取操作被禁止
//					SWL_SHUT_WR	关闭时，后续的发送操作被禁止
//					SWL_SHUT_BOTH 关闭时，后续的读取和发送操作被禁止
//只要调用了shutdown，不管iHow是哪个参数，连接都被断开了
//BUGS:如果iHow为both，recv的返回值是否还能为0？
//其实不调用shutdown也可以，close了，对方自然会知道，只不过，TCP是一个全双工
//的通道，关闭时选择关掉send的通道先，可以保证所有的发送出去了的数据都收到了，
//然后再关闭
int SWL_Shutdown(SWL_socket_t sSock, int iHow)
{
	return shutdown(sSock, iHow);
}

//绑定socket
//return valude: 0 成功 
//				 SWL_SOCKET_ERROR 失败
int SWL_Bind(SWL_socket_t sock, const struct sockaddr *pInfo, SWL_socklen_t len)
{
	return bind(sock, pInfo, len);
}

//监听socket
//return value: 0 成功 
//				SWL_SOCKET_ERROR 失败
int SWL_Listen(SWL_socket_t sock, int iBacklog)
{
	return listen(sock, iBacklog);
}

//return value: 成功：返回socket
//				失败：SWL_INVALID_SOCKET
int SWL_Accept(SWL_socket_t sock, struct sockaddr *pAddr, SWL_socklen_t *pLen)
{
	return accept(sock, pAddr, pLen);
}

//发送数据
//return value: >=0 发送出的数据长度， 
//              SWL_SOCKET_ERROR 失败
//输入参数： pBuf 要发送的数据的指针  iLen 要发送的数据的长度  iFlags 保留先
//BUGS: linux下的send原型为	  ssize_t send(int s, const void *buf, size_t len, int flags);
//		windows下的send原型为 int send(SOCKET s, const char FAR *buf,   int len,  int flags);
//      这里没有把windows和linux下的数据类型统一，而且直接忽略了类型，使用了int
int SWL_Send(SWL_socket_t sock, const void* pBuf, int iLen, int iFlags)
{
#ifdef            WIN32
	return send(sock, static_cast<const char*>(pBuf), iLen, 0);
#else
	return static_cast<int>(send(sock, pBuf, static_cast<size_t>(iLen), MSG_DONTWAIT | MSG_NOSIGNAL));
#endif
}

//接收数据
//return value: >0 成功收到多少数据 SWL_SOCKET_ERROR 有错误发生 0 对方socket正常关闭
//BUGS:linux下原型为 ssize_t recv(int s, void *buf, size_t len, int flags);
//     windows下原型 int recv(SOCKET s, char FAR *buf, int len, int flags);
//      这里没有把windows和linux下的数据类型统一，而且直接忽略了类型，使用了int
int SWL_Recv(SWL_socket_t sock, void* pBuf, int iLen, int iFlags)
{
#ifdef            WIN32
	return recv(sock, static_cast<char*>(pBuf), iLen, 0);
#else
	return static_cast<int>(recv(sock, pBuf, static_cast<size_t>(iLen), MSG_DONTWAIT));
#endif
}

//异步连接
//iTimeOut 以为毫秒为单位，设置connect的是否等待，为0则立即返回
int SWL_Connect(SWL_socket_t sock, const struct sockaddr *pServAddr, SWL_socklen_t len, int iTimeOut)
{
	//在外面，这个传入的socket已经设置成了异步，因此它只是触发一个连接动作
	//并没有等它真正的去连接
	if(0 != connect(sock, pServAddr, len))
	{
#ifdef         WIN32
		int iErrno = WSAGetLastError();
		if (WSAEISCONN  == iErrno) 
		{
			WSASetLastError(iErrno);
			return 0;
		}
		//多判断WSAEINVAL的理由如下：
		//Note In order to preserve backward compatibility, this error is reported as WSAEINVAL to 
		//Windows Sockets 1.1 applications that link to either Winsock.dll or Wsock32.dll.
		else if((WSAEWOULDBLOCK != iErrno) && (WSAEALREADY != iErrno) && (WSAEINVAL != iErrno))
		{
			PrintDebugInfo("connect(sock, pServAddr, len)", WSAGetLastError());
			WSASetLastError(iErrno);
			return SWL_SOCKET_ERROR;
		}	
#else
		if (EISCONN == errno) 
		{
			return 0;
		}
		else if((EINPROGRESS != errno) && (EWOULDBLOCK != errno) && (EAGAIN != errno) && (EALREADY != errno))
		{
			return SWL_SOCKET_ERROR;
		}
#endif
	}

	//检查sock是否连接上
	int error;
	socklen_t error_len;

	int iRet = 0;
	fd_set  writeFds;
	FD_ZERO(&writeFds);
	FD_SET(sock, &writeFds);

	struct timeval	struTimeout;
	if ( 0 != iTimeOut)
	{
		struTimeout.tv_sec = iTimeOut / 1000;  //取秒
		struTimeout.tv_usec = (iTimeOut % 1000 ) * 1000; //取微妙
		iRet = select(sock + 1, NULL, &writeFds, NULL, &struTimeout);
	}
	else
	{
		struTimeout.tv_sec = 0;
		struTimeout.tv_usec = 0;
		iRet = select(sock + 1, NULL, &writeFds, NULL, &struTimeout);
	}
		
	if ( 0 >= iRet ) //异常或者超时
	{
#ifdef	WIN32
		PrintDebugInfo("select(sock + 1, NULL, &writeFds, NULL, &struTimeout);", WSAGetLastError());
#endif
		return SWL_SOCKET_ERROR;
	}
	else
	{
		if(FD_ISSET(sock, &writeFds))
		{
			error_len = sizeof(error);
			if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&error, &error_len) < 0)
			{
				return SWL_SOCKET_ERROR;
			}

			if(error != 0)
			{
				return SWL_SOCKET_ERROR;
			}
			return 0;	//因为只有一个sock懒得判断是否在writeFds中了
		}
		else
		{
			assert(false);
			return SWL_SOCKET_ERROR;
		}
	}
}

//当The socket is marked as nonblocking and no connections are present to be accepted
//当非阻塞socket接收时，一会功夫又没有数据过来
//的时候返回true
bool SWL_EWOULDBLOCK()
{
#ifdef            WIN32
	if( WSAEWOULDBLOCK == WSAGetLastError())
#else
	if ( (EWOULDBLOCK == errno) || (EAGAIN == errno) )
#endif
	{
		return true;
	}
	return false;
}
