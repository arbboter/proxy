// SWL_ConnectProcEx.h: interface for the CSWL_ConnectProcEx class.
//
//////////////////////////////////////////////////////////////////////
/*
	一个CSWL_ConnectProc对象只能连接一个指定IP地址的一个端口，但是可以从这个
	对象中创建多个连接

一次连接：ConnectSyn尝试连接一次，如果连接不上立即返回

异步连接：ConnectAsyn提供异步连接，调用这个函数的时候如果会启动异步连接线程，当
		  连接成功后会触发回调函数，并退出线程，下次调用ConnectAsyn的时候要join,
		  或者Destroy对象的时候join，因为uclibc不支持线程的detach属性,所以才如此
		  处理
		  当上一次异步发送没有发送完，则本次的请求会失败

注意：  1、本类的对象只能用NewConnectProc从堆里创建对象，用Destroy销毁，每个对象必须有这一对
		   对称的操作，否则出错
		2、不考虑Destroy和别的函数的同步问题，Destroy相当于对该对象的delete，就好像没有必要
		   考虑delete和一个对象函数的同步问题
		3、本类的析构函数去掉了虚函数，如果要继承，则需要把它变成虚函数。
		   由于只能用Destroy销毁对象，因此析构函数为私有
*/

#ifndef _SWL_CONNECT_PROC_EX_H_
#define _SWL_CONNECT_PROC_EX_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TVT_PubCom.h"
#include "SWL_TypeDefine.h"
#include "SWL_Public.h"

//当有客户端连接后，accept了以后调用该函数
//pParam1 填注册回调函数时，外部传进去的参数
//pParam2 触发事件时的一些参数信息
typedef int (*GET_CONNECT_LINK_CALLBACK)(SWL_socket_t sock, void *pObject, void *pParam);

//IPV6 的地址最大为XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX
//#define		SWL_MAX_IP_LENGTH	40


class CSWL_ConnectProcEx
{
public:
	//CreateMethod
	static CSWL_ConnectProcEx* NewConnectProc(const char *pIPAddr, unsigned short nPort);
	int Destroy();
	
	//尝试连接一次,以毫秒为超时单位
	SWL_socket_t ConnectSyn(int timeOut);
				
	//连接，当连接上触发回调函数,超时以毫秒为单位
	int ConnectAsyn(GET_CONNECT_LINK_CALLBACK pCallback, void* pObject, int iTimeOut);	

private:
	CSWL_ConnectProcEx(const char *pIPAddr, unsigned short nPort);	
	~CSWL_ConnectProcEx();

	//connect线程，启动ConnectThreadRun
	static PUB_THREAD_RESULT PUB_THREAD_CALL StartConnect(void *pParam);
	int ConnectThreadRun();

private:
//	char					*m_pszIP;			//本对象要连接的IP地址
	unsigned long			m_dwIP;			//转换成无符号整形的IP地址
	const unsigned short	m_nPort;		//本对象要连接的端口
	int						m_iTimeOut;		//异步连接的时候的超时时间，以为毫秒为单位
	SWL_socket_t			m_Sock;			//用于连接的socket
	
	GET_CONNECT_LINK_CALLBACK m_pConnectCallback;
	void					  *m_pConnectCallbackParam;

	PUB_thread_t			m_ConnectThreadID;   //异步连接线程的ID
	bool					m_bConnectThreadRun; //异步连接线程是否在执行
	bool					m_bDestroy;
	
	PUB_lock_t				m_ConnectSynLock; 
	PUB_lock_t				m_ConnectAsynLock; 

	struct addrinfo			*m_pAddrInfo;

};

#endif
