#ifndef _MULTICAST_HEADER_
#define _MULTICAST_HEADER_

const unsigned long		TVT_MULTICAST_HEAD_FLAG		= V4mmioFOURCC('T', 'M', 'H', 'F');
const char * const		TVT_MULTICAST_GROUP_IP		= "234.151.151.2";
const unsigned short	TVT_MULTICAST_GROUP_PORT	= 32758;
const unsigned short    TVT_MULTICAST_SEND_PORT     = 32757;


typedef enum _tvt_multicast_cmd_
{
	TVT_MCMD_SEARCH,
	TVT_MCMD_INFO,
	TVT_MCMD_SET_IP,
	TVT_MCMD_SET_MAC,
	TVT_MCMD_UPGRADE,
}TVT_MULTICAST_CMD;

typedef struct _tvt_multicast_head_
{
	unsigned int		headFlag;		//专有标记，总是为TVT_MULTICAST_HEAD_FLAG
	unsigned int		version;		//组播版本信息
	unsigned int		cmd;			//组播命令，参考TVT_MULTICAST_CMD
	unsigned int		paramLen;		//参数长度
}TVT_MULTICAST_HEAD;


const char DEVICE_TYPE_IPC[] = "ipc";
const char DEVICE_TYPE_BOX[] = "box";

const char MULTICAST_IPV4_ADDR[]       = "ipv4Addr";
const char MULTICAST_IPV4_SUBNETMASK[] = "ipv4SubNetMask";
const char MULTICAST_IPV4_GATEWAY[]    = "ipv4GateWay";
const char MULTICAST_DATA_PORT[]       = "dataport";
const char MULTICAST_MAC_ADDR[]        = "macAddr";
const char MULTICAST_DEV_SN[]          = "deviceSN"; 
const char MULTICAST_DEV_TYPE[]		   = "deviceType";
const char MULTICAST_DEV_OWNER[]       = "deviceOwner";
const char MULTICAST_UPGRADE_PARAM[]   = "param";
const char MULTICAST_BUILD_DATE[]      = "buildDate";
const char MULTICAST_DEV_VERSION[]	   = "version";

#endif 

