#ifndef  __SWL_TYPEDEFINE_H__
#define  __SWL_TYPEDEFINE_H__

#ifdef WIN32

/****************************************** windows ******************************/
//#include <Winsock2.h>		//liujiang
#include <afxsock.h>		//liuhao

typedef  SOCKET     SWL_socket_t;
#define  SWL_INVALID_SOCKET  INVALID_SOCKET
typedef  int        SWL_socklen_t;

//函数返回值
#define  SWL_SOCKET_ERROR  SOCKET_ERROR

//调用SWL_Shutdown时，iHow可以填的三个值
#define SWL_SHUT_RD    SD_RECEIVE      
#define SWL_SHUT_WR    SD_SEND         
//#define SWL_SHUT_BOTH  SD_BOTH		//liujiang
#define SWL_SHUT_BOTH	2				//liuhao
/****************************************** windows ******************************/

#else

/****************************************** linux ******************************/
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include <arpa/inet.h>

typedef  int        SWL_socket_t;
#define  SWL_INVALID_SOCKET   -1
typedef  socklen_t  SWL_socklen_t;

//函数返回值
#define  SWL_SOCKET_ERROR  -1

//调用SWL_Shutdown时，iHow可以填的三个值
#define SWL_SHUT_RD    SHUT_RD      
#define SWL_SHUT_WR    SHUT_WR         
#define SWL_SHUT_BOTH  SHUT_RDWR 
/****************************************** linux ******************************/
#endif

#ifdef __TVT_NAT_SOCK__
#include "NatSocket.h"
#endif

typedef enum _server_type_
{
	NET_TRANS_COMMON	= 0,
	NET_TRANS_NAT		= 1,
}NET_TRANS_TYPE;

typedef union _tvt_socket_
{
#ifdef __TVT_NAT_SOCK__
	NatSocket		natSock;
#endif
	SWL_socket_t	swlSocket;
	_tvt_socket_(){}
	_tvt_socket_(SWL_socket_t sock)
	{
		swlSocket = sock;
	}
#ifdef __TVT_NAT_SOCK__
	_tvt_socket_(NatSocket sock)
	{
		natSock = sock;
	}
#endif
}TVT_SOCKET;

typedef struct _client_sock_info_
{
	TVT_SOCKET		tvtSock;
	char			szIP[40];		//IPv6最多4*8+7=39个字符
	unsigned short	nPort;
	unsigned short	resv;
}CLIENT_SOCK_INFO;

#endif

