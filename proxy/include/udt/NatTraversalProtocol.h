// NatTraversalProtocol.h: interface for the Protocol of traversal.
//
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_TRAVERSAL_PROTOCOL_H
#define _NAT_TRAVERSAL_PROTOCOL_H

////////////////////////////////////////////////////////////////////////////////
// NAT Traversal Protocol interface

/**
 * version for NAT Traversal XML
 */
#define NAT_TRAVERSAL_XML_VER_LEN_MAX  64			// 版本号最大长度
#define NAT_TRAVERSAL_XML_VER_0_1_0_1  "0.1.0.1"
#define NAT_TRAVERSAL_XML_VER_0_2_0_1  "0.2.0.1"
#define NAT_TRAVERSAL_XML_VERSION  "0.2.0.1"		// 当前默认的版本号

#define NAT_SERVER_LISTEN_PORT          8989 

static const int32 NAT_DATA_BLOCK_MAX_SIZE = 1024;

// 穿透的设备向服务器注册以后，注册的超时时间（或保持时间），单位：ms
static const int32 NAT_TRAVERSAL_DEVICE_REG_TIMEOUT = 3 * 60 * 1000; // = 3 minutes

// 穿透的设备向服务器注册以后，双方互相发心跳的间隔时间，单位：ms
static const int32 NAT_TRAVERSAL_DEVICE_REG_HEARTBEAT = 1 * 60 * 1000; // = 1 minutes

// 穿透的客户端连接服务器后，其连接的超时时间（或保持时间），单位：ms
static const int32 NAT_TRAVERSAL_CLIENT_TIMEOUT = 1 * 60 * 1000; // = 1 minutes

// 穿透的请求发出后的超时时间（或有效时间），单位：ms
static const int32 NAT_TRAVERSAL_REQ_TIMEOUT = 1 * 60 * 1000; // = 1 minutes

#pragma pack(push)
#pragma pack(1)

typedef struct _nat_version_2byte
{
    union
    {
        struct 
        {
            byte ucHighVer;
            byte ucLowVer;
        }sVersion;
        uint16 usVersionData;
    };
}NAT_VERSION_2BYTE;

typedef struct _nat_data_block_header
{
    NAT_VERSION_2BYTE usVersion;
    union 
    {
        struct 
        {
            uint16 bEncrypted : 1;
        }Options;
        uint16 OptionsData;
    };
    int32  iDataLen;    
}NAT_DATA_BLOCK_HEADER;

typedef struct _nat_data_block
{
    NAT_DATA_BLOCK_HEADER header;
    union
    {
        char caData[NAT_DATA_BLOCK_MAX_SIZE + 1];
        struct 
        {
            uint32 dwOriginalLen;     // 原文数据长度
            char caOriginalData[NAT_DATA_BLOCK_MAX_SIZE + 1 - sizeof(uint32)]; //加密后的数据
        } sOriginal;
    };
}NAT_DATA_BLOCK;


////////////////////////////////////////////////////////////////////////////////
// Traversal Protocol interface
// P2P通信的协议版本号
typedef struct _nat_p2p_protocol_ver
{
    union
    {
        struct 
        {
            byte ucHighVer;
            byte ucLowVer;
        }sVersion;
        uint16 usVersionData;   
    };
}NAT_P2P_PROTOCOL_VER;

/**
 * 穿透协议请求头
 */
typedef struct _nat_traversal_req_header
{
    uint32         dwRequestSeq;         // 请求序号
}NAT_TRAVERSAL_REQ_HEADER;

/**
 * 穿透协议应答头
 */
typedef struct _nat_traversal_reply_header 
{
    uint32   dwRequestSeq;        // 请求序号
    uint32   dwStatus;            // 应答状态，如果为非成功，则忽略以下字段
}NAT_TRAVERSAL_REPLY_HEADER;

// DevicePeer向NatServer注册
typedef struct _nat_device_register_req
{
    NAT_TRAVERSAL_REQ_HEADER    stHeader;             // 请求头，包含：请求序号
    char                        caDeviceNo[64];       // 设备的序列号
    NAT_P2P_PROTOCOL_VER        usP2PVersion;         // 设备P2P通信协议版本号
    byte                        ucRefuseRelay;        // 设备是否拒绝中继服务。为0表示不拒绝；为非0表示拒绝。默认为0
	char						caExtraInfo[512];	  // 扩展信息，必须是XML格式，内部的数据只会在设备和客户端上解析，对穿透服务器是透明的
}NAT_DEVICE_REGISTER_REQ;


// NatServer向DevicePeer返回注册应答
typedef struct _nat_device_register_reply 
{
    NAT_TRAVERSAL_REPLY_HEADER  stHeader;           // 应答头，包含：请求序号，应答状态。如果应答状态为非成功，则忽略以下字段
    uint32                      dwPeerIp;           // NatServer看到的DevicePeer的IP地址
    uint16                      dwPeerPort;         // NatServer看到的DevicePeer的端口号
}NAT_DEVICE_REGISTER_REPLY;


// ClientPeer向NatServer申请与DevicePeer建立P2P连接
typedef struct _nat_client_connect_p2p_req
{
    NAT_TRAVERSAL_REQ_HEADER    stHeader;            // 请求头，包含：请求序号
    char                        dwDeviceNo[64];      // 设备的序列号	
    byte                        ucRequestPeerNat;    // 发起请求的结点的NAT类型
    NAT_P2P_PROTOCOL_VER        usP2PVersion;        // UDT协议的版本号
    uint32                      dwConnectionId;      // 连接的Id，用于标识将要建立的P2P连接
}NAT_CLIENT_CONNECT_P2P_REQ;



// NatServer向ClientPeer返回P2P连接申请应答
typedef struct _nat_client_connect_p2p_reply 
{
    NAT_TRAVERSAL_REPLY_HEADER  stHeader;           // 应答头，包含：请求序号，应答状态。如果应答状态为非成功，则忽略以下字段
    uint32                      dwDevicePeerIp;     // 设备的出外网的ip
    uint16                      usDevicePeerPort;   // 设备的出外网的端口号
    byte                        ucDevicePeerNat;    // 发起请求的结点的NAT类型
}NAT_CLIENT_CONNECT_P2P_REPLY;

// NatServer向DevicePeer通知P2P连接申请
typedef struct _nat_notify_connect_p2p_req 
{
    NAT_TRAVERSAL_REQ_HEADER    stHeader;             // 请求头，包含：请求序号
    uint32                      dwRequestPeerIp;      // 客端的外网地址ip
    uint16                      dwRequestPeerPort;    // 客户端的外网地址port
    byte                        ucRequestPeerNat;     // 请求结点的NAT类型
    NAT_P2P_PROTOCOL_VER        usP2PVersion;         // UDT协议的版本号
    uint32                      dwConnectionId;       // 连接的Id，用于标识将要建立的P2P连接
}NAT_NOTIFY_CONNECT_P2P_REQ;


// DevicePeer向NatServer返回通知P2P连接申请的应答
typedef struct _nat_notify_connect_p2p_reply 
{
    NAT_TRAVERSAL_REPLY_HEADER  stHeader;           // 应答头，包含：请求序号，应答状态。如果应答状态为非成功，则忽略以下字段
}NAT_NOTIFY_CONNECT_P2P_REPLY;

// ClientPeer向NatServer申请与DevicePeer建立Relay连接
typedef struct _nat_client_connect_relay_req
{
    NAT_TRAVERSAL_REQ_HEADER    stHeader;             // 请求头，包含：请求序号
    char                        dwDeviceNo[64];       // 设备的序列号	
}NAT_CLIENT_CONNECT_RELAY_REQ;

// NatServer向ClientPeer返回Relay连接申请应答
typedef struct _nat_client_connect_relay_reply
{
    NAT_TRAVERSAL_REPLY_HEADER  stHeader;               // 应答头，包含：请求序号，应答状态。如果应答状态为非成功，则忽略以下字段
    uint32                      dwRelayServerIp;        // RelayServer的IP地址
    uint16                      usRelayServerPort;      // RelayServer的端口号
    uint32                      ucRelayConnectionId;    // Relay的连接号，用于标识本次申请的连接配对
}NAT_CLIENT_CONNECT_RELAY_REPLY;

// NatServer向DevicePeer通知Relay连接申请
typedef struct _nat_notify_connect_relay_req 
{
    NAT_TRAVERSAL_REQ_HEADER    stHeader;             // 请求头，包含：请求序号
    uint32                      dwRelayServerIp;      // RelayServer的IP地址
    uint16                      dwRelayServerPort;    // RelayServer的端口号
    uint32                      dwConnectionId;       // 连接的Id，用于标识将要建立的P2P连接
}NAT_NOTIFY_CONNECT_RELAY_REQ;

// DevicePeer向NatServer返回通知Relay连接申请的应答
typedef struct _nat_notify_connect_relay_reply 
{
    NAT_TRAVERSAL_REPLY_HEADER  stHeader;               // 应答头，包含：请求序号，应答状态。如果应答状态为非成功，则忽略以下字段
}NAT_NOTIFY_CONNECT_RELAY_REPLY;

// ClientPeer向NatServer获取DevicePeer的信息
typedef struct _nat_fetch_device_req 
{
    NAT_TRAVERSAL_REQ_HEADER    stHeader;             // 请求头，包含：请求序号
    char                        dwDeviceNo[64];       // 设备的序列号
}NAT_FETCH_DEVICE_REQ;

// NatServer向ClientPeer返回DevicePeer的信息
typedef struct _nat_fetch_device_reply
{
    NAT_TRAVERSAL_REPLY_HEADER  stHeader;               // 应答头，包含：请求序号，应答状态。如果应答状态为非成功，则忽略以下字段
    uint32                      dwDevicePeerIp;         // DevicePeer的IP地址，遵循“192.168.1.1”的格式
    uint16                      usDevicePeerPort;       // DevicePeer的端口号
    byte                        ucCanRelay;             // 设备是否可以使用中继连接
	char						caExtraInfo[512];		// 扩展信息，内部的数据只会在设备和客户端上解析，对穿透服务器是透明的	
}NAT_FETCH_DEVICE_REPLY;


// 穿透心跳命令
typedef struct _nat_traversal_heartbeat
{
    char                  uaDeviceNo[64];      // 设备的序列号	
}NAT_TRAVERSAL_HEARTBEAT;

/**
 * 穿透设备扩展信息
 * 标准版
 */
typedef struct _nat_device_extra_info
{
		uint32         deviceType;          /**< 设备类型，等价于实际设备中的产品型号 */
		char           deviceVersion[128];  /**< 设备版本号，以"-"为分隔符， 如"1.0.0-XXX-XX"
												 其中，第1部分为主要版本号 */
		uint16         deviceWebPort;       /**< 设备的WEB侦听端口号（TCP） */
		uint16         deviceDataPort;      /**< 设备的数据侦听端口（TCP） */
}NAT_DEVICE_EXTRA_INFO;

#pragma pack(pop)

enum NatTraversalCmdId
{
    /* 请求命令 */
    NAT_ID_DEVICE_REGISTER_REQ			   = 10001,  // DevicePeer向NatServer注册
    NAT_ID_CLIENT_CONNECT_P2P_REQ         = 10002,  // ClientPeer向NatServer申请与DevicePeer建立P2P连接
    NAT_ID_NOTIFY_CONNECT_P2P_REQ         = 10003,  // NatServer向DevicePeer通知P2P连接申请
    NAT_ID_CLIENT_CONNECT_RELAY_REQ       = 10004,  // ClientPeer向NatServer申请与DevicePeer建立Relay连接
    NAT_ID_NOTIFY_CONNECT_RELAY_REQ       = 10005,  // NatServer向DevicePeer通知Relay连接申请
    NAT_ID_FETCH_DEVICE_REQ               = 10006,  // ClientPeer向NatServer获取DevicePeer的信息


    /* 应答命令 */
    NAT_ID_DEVICE_REGISTER_REPLY           = 20001,  // NatServer向DevicePeer返回注册应答
    NAT_ID_CLIENT_CONNECT_P2P_REPLY        = 20002,  // NatServer向ClientPeer返回P2P连接申请应答
    NAT_ID_NOTIFY_CONNECT_P2P_REPLY        = 20003,  // DevicePeer向NatServer返回通知P2P连接申请的应答
    NAT_ID_CLIENT_CONNECT_RELAY_REPLY      = 20004,  // NatServer向ClientPeer返回Relay连接申请应答
    NAT_ID_NOTIFY_CONNECT_RELAY_REPLY      = 20005,  // DevicePeer向NatServer返回通知Relay连接申请的应答
    NAT_ID_FETCH_DEVICE_REPLY              = 20006,  // NatServer向ClientPeer返回DevicePeer的信息

    /* 普通命令 */
    NAT_ID_HEARTBEAT                       = 1,      // 心跳命令
    NAT_ID_DEVICE_UNREGISTER               = 2,      // 设备取消注册命令
    NAT_ID_UNKNOW                          = 0       // 未知命令
};


enum TraversalReplyStatus
{
    REPLY_STATUS_OK                  = 0,      // 成功
    REPLY_STATUS_UNKNOWN             = 1,      // 未知错误
    REPLY_STATUS_DEVICE_OFFLINE      = 2,	   // 设备不在线
    REPLY_STATUS_DEVICE_REGISTERED   = 3,      // 设备重复注册
    REPLY_STATUS_SERVER_OVERLOAD     = 4,      // 服务器负载过重，无法再提供服务
    REPLY_STATUS_DEVICE_NO_RELAY     = 5       // 设备不支持中继通信功能
};

const static int32 TRAVERSAL_REPLY_STATUS_COUNT = 6;

static const char* TRAVERSAL_REPLY_STATUS_NAMES[TRAVERSAL_REPLY_STATUS_COUNT] = {
    "Traversal reply OK",                   // REPLY_STATUS_OK
    "Traversal reply unknown",              // REPLY_STATUS_UNKNOWN
    "Traversal reply device offline",       // REPLY_STATUS_DEVICE_OFFLINE
    "Traversal reply device registered",    // REPLY_STATUS_DEVICE_REGISTERED
    "Traversal reply server overload",      // REPLY_STATUS_SERVER_OVERLOAD
    "Traversal reply device no relay"       // REPLY_STATUS_DEVICE_NO_RELAY
};


inline const char* GetTraversalReplyStatusName(int32 status)
{
    if (status >= 0 && status < TRAVERSAL_REPLY_STATUS_COUNT)
    {
        return TRAVERSAL_REPLY_STATUS_NAMES[status];
    }
    return TRAVERSAL_REPLY_STATUS_NAMES[REPLY_STATUS_UNKNOWN];
}

inline const char* GetTraversalReplyStatusName(TraversalReplyStatus status)
{
    return GetTraversalReplyStatusName((int32)status);
}

#endif//_NAT_TRAVERSAL_PROTOCOL_H