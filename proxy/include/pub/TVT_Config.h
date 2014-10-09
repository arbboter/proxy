/***********************************************************************
** Copyright (C) Tongwei Video Technology Co.,Ltd. All rights reserved.
** Author       : YSW
** Date         : 2012-03-23
** Name         : TVT_Config.h
** Version      : 1.0
** Description  :	描述DVR/DVS相关的一些基本特性
** Modify Record:
1:创建
***********************************************************************/
#ifndef __TVT_CONFIG_H__
#define __TVT_CONFIG_H__
#include "TVT_PubDefine.h"
#include "TVT_TimeZone.h"

#pragma pack(4)

typedef enum _tvt_dd_config_item_id_
{
	TVT_CONFIG_ITEM_GUI				= 0x0000,
	TVT_CONFIG_ITEM_GUI_CONFIG,				//TVT_GUI_CONFIG
	TVT_CONFIG_ITEM_STATUS_BK,				//TVT_SYSTEM_STATUS_BACKUP
	TVT_CONFIG_ITEM_GUI_END,

	TVT_CONFIG_ITEM_SYSTEM_BASE		=0x0100,
	TVT_CONFIG_ITEM_VIDEO_OUT,				//TVT_VIDEO_OUT_CONFIG
	TVT_CONFIG_ITEM_DEVICE_CONFIG,			//TVT_DEVICE_CONFIG
	TVT_CONFIG_ITEM_DATE_TIME,				//TVT_DATE_TIME_CONFIG
	TVT_CONFIG_ITEM_SYSTEM_OTHER,			//TVT_BASIC_OTHER
	TVT_CONFIG_ITEM_ALLOW_DENY_IP,			//TVT_ALLOW_DENY_IP
	TVT_CONFIG_ITEM_DISK_CONFIG,			//TVT_DISK_CONFIG
	TVT_CONFIG_ITEM_SYSTEM_END,

	TVT_CONFIG_ITEM_LIVE_BASE		= 0x0200,
	TVT_CONFIG_ITEM_LIVE_CHNN,				//TVT_LIVE_CHNN_CONFIG
	TVT_CONFIG_ITEM_LIVE_DISPLAY,			//TVT_LIVE_DISPLAY_CONFIG
	TVT_CONFIG_ITEM_LIVE_END,



	//
	TVT_CONFIG_ITEM_END
}TVT_CONFIG_ITEM_ID;


typedef struct _tvt_work_mode_info_
{
    bool   bEnableAlarm;  //报警开关  0:关闭，1：打开
    bool   bEnablePush;   //推送开关  0:关闭，1：打开
    bool   bEnableAlarmrecord;  //报警录像开关  0:关闭，1：打开
}TVT_WORK_MODE_INFO;

typedef struct _tvt_work_mode_
{
    int  workMode; // 0:在家模式 1:离家模式
    TVT_WORK_MODE_INFO  atHomeMode;      //在家模式
    TVT_WORK_MODE_INFO  leaveHomeMode;   //离家模式
}TVT_WORK_MODE;

typedef  struct  _tvt_user_info_
{
    char masterName[64];       //户主名称
    char masterPassword[64];   //户主密码
}TVT_USER_INFO; 

typedef struct _tvt_privacy_protection_
{
    int   enable;   //隐私保护 0:开启，1:关闭
}TVT_PRIVACY_PROTECTION;

typedef  struct  _tvt_wireless_info_
{
    char  wifiSsid[64];        //wifi SSID
    char  wifipassword[132];   //wifi 密码
}TVT_WIRELESS_INFO;

typedef struct  _tvt_network_ip_
{
	char			MAC[8];					    //物理地址（只读）
	unsigned short  nicTypeMask;                //支持的网卡类型掩码（只读）对应结构体TVT_NIC_TYPE
	unsigned short  supportIPV6;                //是否支持IPV6(只读)
	
	unsigned short  nicType;                    //网卡类型
	unsigned short  enable;                     //网卡是否启用。0-不启用，1-启用,

	unsigned int 	nicStatus;					//网卡状态: 0,connect;1,disconnect.	
	
	//WiFi参数，在（TVT_NIC_TYPE_WIFI == nicType）时有效
	struct  
	{
		unsigned int	channel;				//信道
		unsigned int	authMode;				//认证模式：TVT_WIFI_AUTH_MODE
		unsigned int	encrypType;				//加密方式：TVT_WIFI_ENCRYPTION_TYPE
		unsigned int	defaultKeyID;			//密码编号

		unsigned int	rate;					//频段
		unsigned int	mode;					//模式

		char			SSID[36];				//SSID(strlen() <= 32)
		char			key[132];				//无线密码（strlen() <= 128）
	}wifi;

	struct  
	{
		unsigned short	recv;
		unsigned short	useDHCP;				//是否使用动态网络地址

		char			IP[16];					//网络地址
		char			subnetMask[16];			//子网掩码
		char			gateway[16];			//网关

		char	preferredDNS[16];				//主网络域名服务器地址
		char	alternateDNS[16];				//从网络域名服务器地址
	}IPv4;

	/*IPV6的IP地址。总共16个字节，界面显示格式为X:X:X:X:X:X:X:X。
	X用16进制表示，每个X占4位16进制，也就是两个字节，总共8个X用来表示16个字节的IP地址
	IPV6的子网掩码，用长度来表示。取值为1～127。比如取64，则IP地址的前64位为网络号
	IPV6的网关。和IPV6的IP地址的字节数、表示方式一样*/
	struct  
	{
		unsigned short	recv;
		unsigned short	useDHCP;				//是否使用动态网络地址

		unsigned int	subnetMask;
		char			IP[40];		
		char			gateway[40];

		char	preferredDNS[40];				//主网络域名服务器地址
		char	alternateDNS[40];				//从网络域名服务器地址
	}IPv6;

	struct  
	{
		unsigned short	supportPPPoE;			//是否支持PPPoE功能（只读）
		unsigned short	usePPPoE;				//是否使用PPPoE
		char account[TVT_MAX_ACCOUNT_LEN+4];	//PPPoE的帐号
		char password[TVT_MAX_PASSWORD_LEN+4];	//PPPoE的密码
	}PPPoE;
}TVT_NETWORK_IP;

typedef struct _tvt_network_ip_config_
{
	unsigned char	recv;
	unsigned char	bLock;				//是否锁定

	unsigned char	bSupportNAT;		//是否支持网络穿透（只读）
	unsigned char	ethNum;				//有效网卡个数(只读)
	unsigned char	bSupportPush;		//是否支持推动(只读）

	unsigned char	bEnableCellular;	//是否开启移动蜂窝数据（3G/4G）
	unsigned char	bEnableNAT;			//开启网络穿透
	unsigned char	bEnablePush;		//是否开启推送

	TVT_NETWORK_IP	network[TVT_MAX_ETH_NUM];
}TVT_NETWORK_IP_CONFIG;

#pragma pack()
#endif //__TVT_CONFIG_H__
