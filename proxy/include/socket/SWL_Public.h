// SWL_Public.h: interface for the CSWL_Public class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SWL_PUBLIC_H__3834396E_DB0F_48F9_B5A2_DA88C1EA87A4__INCLUDED_)
#define AFX_SWL_PUBLIC_H__3834396E_DB0F_48F9_B5A2_DA88C1EA87A4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SWL_TypeDefine.h"

#ifdef  WIN32
//#include <Winsock2.h>		//liujiang
#include <afxsock.h>		//liuhao
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif


//sssssssssssssssssssssssssss
#define   _DEBUG_SWL
#ifdef   _DEBUG_SWL
#include  <stdio.h>
#define   SWL_PRINTF(format,argc) printf(format, ##argc)
#else
#define   SWL_PRINTF(format,argc)
#endif


//初始化网络socket的库
int SWL_Init();	//非线程安全

//网络socket的库
int SWL_Quit();	//非线程安全

//socket层的出错打印信息
void SWL_PrintError(const char* pFile, int iLine);

//创建一个socket,创建的所有socket都设置了非阻塞属性
SWL_socket_t SWL_CreateSocket(int iDomain, int iType, int iProtocol);

//释放socket资源
int SWL_CloseSocket(SWL_socket_t sSock);

//关闭已经连接上的socket
int SWL_Shutdown(SWL_socket_t sSock, int iHow);

//绑定socket
int SWL_Bind(SWL_socket_t sSock, const struct sockaddr *name, SWL_socklen_t len);

//监听socket
int SWL_Listen(SWL_socket_t sSock, int iBacklog);

//accept socket
int SWL_Accept(SWL_socket_t sSock, struct sockaddr *pAddr, SWL_socklen_t *pLen);

//发送数据
int SWL_Send(SWL_socket_t sSock, const void* pBuf, int iLen, int iFlags);

//接收数据
int SWL_Recv(SWL_socket_t sSock, void* pBuf, int iLen, int iFlags);

//异步连接
int SWL_Connect(SWL_socket_t sSock, const struct sockaddr *pServAddr, SWL_socklen_t len, int iTimeOut);

//当The socket is marked as nonblocking and no connections are present to be accepted
//当非阻塞socket接收时，一会功夫又没有数据过来
//的时候返回true
bool SWL_EWOULDBLOCK();

#endif // !defined(AFX_SWL_PUBLIC_H__3834396E_DB0F_48F9_B5A2_DA88C1EA87A4__INCLUDED_)

