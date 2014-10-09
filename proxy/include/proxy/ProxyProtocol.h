#ifndef _NET_PROTOCOL_H_
#define _NET_PROTOCOL_H_

#include <cstdint>

// 每一包的length要4字节对齐,
// 所以最后所有包的length加起来可能比total大

#pragma pack(4)
struct PACKET_HEAD 
{
	uint32_t    magic;
	uint32_t    type;
	uint16_t    encrypt : 1;
	uint16_t    count : 15;
	uint16_t    length;
	uint32_t    total : 28;
	uint32_t    padding : 4;
};
#pragma pack()

const uint32_t PACKET_MAGIC_NUM          =  0xfafafafa;
const uint32_t PACKET_MAX_SIZE           =  1280;
const uint32_t PACKET_MAX_TOTAL_LENGTH   =  0xfffffff;
const uint32_t PACKET_HEAD_SIZE			 = sizeof(PACKET_HEAD);
const uint32_t PACKET_MAX_PAYLOAD	     =  PACKET_MAX_SIZE - PACKET_HEAD_SIZE;
const uint32_t PACKET_COUNT_MAX          =  0x7fff;
const uint32_t PACKET_MAX_LEN            =  2*1024*1024;

const int HS_PROTOCOL_VER	=	0x01;

#define PACKET_TYPE(ver, t)      ((ver) << 16 | t[1] << 8 | t[0])

#ifndef UINT16_MAX
#define UINT16_MAX 65535
#endif

const int PACKET_TYPE_EXCHANGE_ENCRYPT_KEY   =   PACKET_TYPE(UINT16_MAX, "ex");
const int PACKET_TYPE_HEART_BEAT             =   PACKET_TYPE(UINT16_MAX, "bt");

const int PACKAGE_TYPE_MSG   =   PACKET_TYPE(HS_PROTOCOL_VER, "ms");
const int PACKAGE_TYPE_PUSH  =   PACKET_TYPE(HS_PROTOCOL_VER, "pu");
const int PACKAGE_TYPE_LIVE  =   PACKET_TYPE(HS_PROTOCOL_VER, "li");
const int PACKAGE_TYPE_MP4   =   PACKET_TYPE(HS_PROTOCOL_VER, "mp");
const int PACKAGE_TYPE_TALK  =   PACKET_TYPE(HS_PROTOCOL_VER, "ta");
const int PACKAGE_TYPE_PIC   =   PACKET_TYPE(HS_PROTOCOL_VER, "pc");

const int HEARTBEAT_TIME_OUT_SEC	=   10000;

const int SHORT_STR_LEN = 32;
const int LONG_STR_LEN = 64;
const int LONG_LONG_STR_LEN = 128;


const char PROXY_CMD[] = "cmd";
const char PROXY_DATA[] = "data";
const char PROXY_AUTH[] = "auth";

const char PROXY_SN[] = "device_sn";
const char PROXY_USERNAME[] = "username";
const char PROXY_PASSWORD[] = "password";
const char PROXY_SESSION[] = "session";
const char PROXY_LIST[] = "list";

const char PROXY_BIND_DEVICE_FAILED[] = "bind device failed";
const char PROXY_NOT_ONLINE[] = "device not online";
const char PROXY_UNKNOWN_ERROR[] = "unknown error";

const char PROXY_CODE[] = "code";
const char PROXY_DESC[] = "desc";

const char PROXY_OK[] = "ok";
const char PROXY_ERROR[] = "error";
const char PROXY_PARSER_JSON_FAILED[] = "parser json failed";
const char PROXY_PARAM_ERROR[] = "param error";
const char PROXY_PARAM_NONE[] = "";

const char PROXY_ACTION[] = "action";
const char PROXY_OPEN[] = "open";
const char PROXY_CLOSE[] = "close";

const char PROXY_DOWNLOAD[] = "download";
const char PROXY_FILE[] = "file";
const char PROXY_RANGE[] = "range";
const char PROXY_DOWNLOAD_HEAD[] = "download_head";
const char PROXY_DOWNLOAD_TAIL[] = "download_tail";
const char PROXY_DOWNLOAD_FEEDBACK[] = "download_feedback";
const char PROXY_BYTE[] = "byte";

const char PROXY_LIVE[] = "live";
const char PROXY_TYPE[] = "type";
const char PROXY_CHANNL[] = "channl";
const char PROXY_MAIN_STREAM[] = "video1";
const char PROXY_SUB_STREAM[] = "video2";

const char PROXY_DEV_STATUS[] = "status";
const char PROXY_DEV_ONLINE[] = "online";
const char PROXY_DEV_OFFLINE[] = "offline";
const char PROXY_DEV_NONE[] = "deviceNotAdd";

const char PROXY_APPLE[] = "apple";
const char PROXY_BAIDU[] = "baidu";
const char PROXY_GOOGLE[] = "google";
const char PROXY_MOBILE_TYPE[] = "device_type";
const char PROXY_TOKEN[] = "token";
const char PROXY_USER_ID[] = "user_id";
const char PROXY_CHANNL_ID[] = "channel_id";


#endif