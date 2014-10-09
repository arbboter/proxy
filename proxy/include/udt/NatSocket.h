// NatSocket.h: interface for the NatSocket type.
//
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_SOCKET_H_
#define _NAT_SOCKET_H_

#include "NatUserDefine.h"

#include <set>

typedef void* NatSocket;

#define NAT_SOCKET_INVALID NULL

typedef std::set<NatSocket> NatSocketSet;

/**
 * NAT客户端错误
 */
enum NAT_CLIENT_ERROR
{
    NAT_CLI_OK		= 0,               // 成功
    NAT_CLI_ERR_UNKNOWN,                // 未知错误
    NAT_CLI_ERR_TIMEOUT,               // 客户端超时
    NAT_CLI_ERR_DEVICE_OFFLINE,	       // 设备不在线
    NAT_CLI_ERR_SERVER_OVERLOAD,       // 服务器负载过重，无法再提供服务
    NAT_CLI_ERR_DEVICE_NO_RELAY,       // 设备不支持中继通信功能
    NAT_CLI_ERR_SYS,                   // 系统错误
    NAT_CLI_ERR_NETWORK                // 网络错误
};

/**
 * NAT客户端状态
 */
enum NAT_CLIENT_STATE
{
	NAT_CLI_STATE_INIT		= 0,		// 本地系统出错
	NAT_CLI_STATE_P2P_SERVER,			// P2P穿透模式下，与服务器通信的过程
	NAT_CLI_STATE_P2P_DEVICE,			// P2P穿透模式下，与设备通信的过程
	NAT_CLI_STATE_RELAY_SERVER,			// RELAY模式下，与服务器通信的过程
	NAT_CLI_STATE_RELAY_DEVICE,			// RELAY模式下，与设备通信的过程
};

const char* NAT_GetClientErrorText(NAT_CLIENT_ERROR error);

typedef struct _nat_socket_info_
{
    unsigned long  remoteIp;
    unsigned short remotePort;
}NAT_SOCKET_INFO;

/**
 * NAT模块的初始化，需要在程序开始运行时调用
 * 非线程安全
 */
int NAT_Init();

/**
 * NAT模块的终止化，释放NAT模块所占用的全局资源，需要在程序结束运行时调用
 * 非线程安全
 */
int NAT_Quit();

int NAT_CloseSocket(NatSocket sock);

int NAT_GetSocketInfo(NatSocket sock, NAT_SOCKET_INFO &info);

int NAT_Select(NatSocketSet *readSet, NatSocketSet *writeSet, int iTimeout);

int NAT_Send(NatSocket sock, const void *pBuf, int iLen);

int NAT_Recv(NatSocket sock, void *pBuf, int iLen);

/**
 * 获取当前可以发送数据的字节大小
 * @return 如果没有错误，则返回当前可以发送数据的字节大小，0表示发送缓冲区满了；否则，出错返回-1
 */
int NAT_GetSendAvailable(NatSocket sock);

/**
 * 获取可以接收数据的字节大小，主要用于主动接收数据
 * @return 如果没有错误，则返回当前可以接收数据的字节大小，0表示无数据；否则，出错返回-1
 */
int NAT_GetRecvAvailable(NatSocket sock);

bool NAT_IsSocketInSet(NatSocket sock, NatSocketSet *socketSet);

void NAT_EraseSocketFromSet(NatSocket sock, NatSocketSet *socketSet);

void NAT_InsertSocketToSet(NatSocket sock, NatSocketSet *socketSet);

#endif//_NAT_SOCKET_H_