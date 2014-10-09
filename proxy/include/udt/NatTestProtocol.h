// NatTraversalProtocol.h: interface for the Protocol of traversal.
//
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_TEST_PROTOCOL_H
#define _NAT_TEST_PROTOCOL_H

#include "NatBaseTypes.h"

////////////////////////////////////////////////////////////////////////////////
// Simple Data Block protocol interface

/**
 * 默认侦听端口（UDP）
 */
#define NAT_TEST_PROT_SERVICE_PORT		8993

/**
 * 测试协议版本，当前=1.0
 */
#define NAT_TEST_PROT_VER_HIGH		1
#define NAT_TEST_PROT_VER_LOW		0

/**
 * 协议起始标志
 */
static const byte NAT_TEST_PROT_START_TAG[2] = {0xEF, 0xEF};

/**
 * 测试协议类型，NAT_TEST_PORT_PROT_XX
 */
static const byte NAT_TEST_PORT_PROT_TCP = 0;
static const byte NAT_TEST_PORT_PROT_UDP = 1;


/**
 * 测试请求命令ID，NAT_TEST_ID_XX_REQ
 */
static const uint16 NAT_TEST_ID_SPEED_REQ = 0x0101;				// 测速请求命令
static const uint16 NAT_TEST_ID_PUB_IP_REQ = 0x0102;			// 测公网IP（IPv4）请求命令
static const uint16 NAT_TEST_ID_PORT_CONNECT_REQ = 0x0103;		// 测端口连接请求命令

/**
 * 测试应答命令ID，NAT_TEST_ID_XX_REPLY
 */
static const uint16 NAT_TEST_ID_SPEED_REPLY = 0x0201;			// 测速应答命令
static const uint16 NAT_TEST_ID_PUB_IP_REPLY = 0x0202;			// 测公网IP（IPv4）应答命令
static const uint16 NAT_TEST_ID_PORT_CONNECT_REPLY = 0x0203;	// 测端口连接应答命令


/**
 * 测速请求模式 NAT_TEST_SPEED_MODE_XX
 */
static const byte NAT_TEST_SPEED_MODE_ECHO = 0x01; // 回显测试数据
// 测速请求模式的其它值，表示不回显测试数据


#pragma pack(push)
#pragma pack(1)


/**
 * 穿透测试协议头
 */
typedef struct _nat_test_prot_header
{
	byte		startTag[2];	// 起始标志，非以下值的包将被丢弃：0xEFEF
	byte		versionHigh;	// 版本号高位
	byte		versionLow;		// 版本号低位
	uint16		seq;			// 请求序号，如果是请求包，取值逐个递增；如果是应答包，等于所答复的请求包的请求序号
	uint16		cmdId;			// 命令ID，小端形式  
}NAT_TEST_PROT_HEADER;


/**
 * 测速请求命令的结构
 */
typedef struct _nat_test_speed_req
{
	NAT_TEST_PROT_HEADER	header;			// 协议头
	uint32					timestamp;		// 请求的时间戳
	byte					mode;			// 请求模式。		
	char					testData[512];	// 测试数据，最大长度是512 Bytes
}NAT_TEST_SPEED_REQ;

/**
 * 测速应答命令的结构
 */
typedef NAT_TEST_SPEED_REQ NAT_TEST_SPEED_REPLY;

/**
 * 测公网IP（IPv4）请求命令的结构
 */
typedef struct _nat_test_pub_ip_req
{
	NAT_TEST_PROT_HEADER	header;			// 协议头
	/* 无数据 */
}NAT_TEST_PUB_IP_REQ;

/**
 * 测公网IP（IPv4）应答命令的结构
 */
typedef struct _nat_test_pub_ip_reply
{
	NAT_TEST_PROT_HEADER	header;			// 协议头
	uint32			ip;				// 请求方的公网IP
	uint16			port;			// 请求方的公网端口
}NAT_TEST_PUB_IP_REPLY;

/**
 * 测端口连接请求命令的结构
 */
typedef struct _nat_test_port_connect_req
{
	NAT_TEST_PROT_HEADER	header;			// 协议头
	uint16			port;			// 需要被测试端口
	byte			netProtocol;	// 测试协议类型，NAT_TEST_PORT_PROT_XX
	byte			reserved;		// 保留字段
}NAT_TEST_PORT_CONNECT_REQ;

/**
 * 测端口连接应答命令的结构
 */
typedef struct _nat_test_port_connect_reply
{
	NAT_TEST_PROT_HEADER	header;			// 协议头
	uint32			ip;				// 请求方的公网IP
	uint16			port;			// 请求方的公网端口
}NAT_TEST_PORT_CONNECT_REPLY;

#pragma pack(pop)

#endif//_NAT_TEST_PROTOCOL_H