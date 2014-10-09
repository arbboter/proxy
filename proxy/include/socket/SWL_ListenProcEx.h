// SWL_ListenProcEx.h: interface for the CSWL_ListenProcEx class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SWL_LISTENPROCEX_H__8A2C9646_247E_4B30_94A7_FD3F66A78973__INCLUDED_)
#define AFX_SWL_LISTENPROCEX_H__8A2C9646_247E_4B30_94A7_FD3F66A78973__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "SWL_TypeDefine.h"
#include "SWL_Public.h"
#include "TVT_PubCom.h"

//当有客户端连接后，accept了以后调用该函数
//pParam1 填注册回调函数时，外部传进去的参数
//pParam2 触发事件时的一些参数信息
typedef int (*GET_ACCEPT_LINK_CALLBACK)(CLIENT_SOCK_INFO * pClientSockInfo, void *pParam1, void *pParam2);

class CSWL_ListenProcEx  
{
public:
	//pCallback函数要尽量快返回，因为程序还要接着listen
	//如果pCallback里没有把生成的socket接管来则会导致资源泄露
	CSWL_ListenProcEx(GET_ACCEPT_LINK_CALLBACK pCallback, void* pParam);
	~CSWL_ListenProcEx();

	int StartListen(unsigned short int nPort);     //开始监听端口，检查是否有网络接入
	int StopListen();      //停止监听端口

private:
	//禁止使用默认的拷贝构造函数和=运算符
	CSWL_ListenProcEx(const CSWL_ListenProcEx&);
	CSWL_ListenProcEx& operator=(const CSWL_ListenProcEx&);
	
	//accept线程，启动AcceptThreadRun,为了在代码上分开环境间的差异
	static PUB_THREAD_RESULT PUB_THREAD_CALL StartAccept(void *pParam);
	int AcceptThreadRun();	   //accept线程,accept的run函数
	int AcceptThread6Run();		//IPV6

	SWL_socket_t CreateIPV4Socket(unsigned short port);
	SWL_socket_t CreateIPV6Socket(unsigned short port);

private:
	bool				m_bAcceptThreadRun;    //accept线程是否运行
	bool				m_bListenStart;     	//是否正在listen
	GET_ACCEPT_LINK_CALLBACK m_pAcceptCallback;    //有客户端connect上来，accept后调用此回调函数
	void				*m_pParam;             //回调函数的
	SWL_socket_t		m_SocketListen;       //用于监听的socket
	PUB_thread_t		m_AcceptThreadID;	    //accept线程的ID
	PUB_lock_t			m_ListenLock;      	//开始监听和停止监听的锁

	CLIENT_SOCK_INFO	m_clientSockInfo;
};

#endif // !defined(AFX_SWL_LISTENPROCEX_H__8A2C9646_247E_4B30_94A7_FD3F66A78973__INCLUDED_)
