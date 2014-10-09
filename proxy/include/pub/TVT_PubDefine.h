/***********************************************************************
** 深圳同为数码科技有限公司保留所有权利。
** 作者			: 袁石维
** 日期         : 2012-03-22
** 名称         : TVT_PubDefine.h
** 版本号		: 1.0
** 描述			:
** 修改记录		:
***********************************************************************/
#ifndef __TVT_PUB_DEFINE_H__
#define __TVT_PUB_DEFINE_H__

#ifdef WIN32
#include <afxwin.h>
#include <io.h>
#include <winsock2.h>
#include "windows.h"
#include "time.h"
#include "winbase.h"
#include <ws2tcpip.h>
#else //////Linux下面
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <pthread.h> 
#include <time.h> 
#include <signal.h>
#include <fcntl.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#endif

#include <assert.h> 
#include <iostream>
#include <fstream>
#include <string.h>
#include <list>
#include <map>

#include "TVT_Product.h"

#include "TVT_DevDefine.h"

using namespace std;

#pragma pack(4)
//////////////////////////////////////////////////////////////////////////

typedef enum _tvt_encryption_type_
{
    TVT_ENCRYPTION_NONE,
    TVT_ENCRYPTION_SSL,
    TVT_ENCRYPTION_TLS
}TVT_ENCRYPTION_TYPE;

typedef enum _tvt_date_mode_
{
	TVT_DATE_MODE_YYMMDD	= 0x01,		//年-月-日模式
	TVT_DATE_MODE_MMDDYY	= 0x02,		//月-日-年模式
	TVT_DATE_MODE_DDMMYY	= 0x03,		//日-月-年模式
	TVT_DATE_MODE_Y_M_D	= 0x04,		//年/月/日模式
	TVT_DATE_MODE_M_D_Y	= 0x05,		//月/日/年模式
	TVT_DATE_MODE_D_M_Y	= 0x06,		//日/月/年模式
	TVT_DATE_MODE_YMD		= 0x07,		//年.月.日模式
	TVT_DATE_MODE_MDY		= 0x08,		//月.日.年模式
	TVT_DATE_MODE_DMY		= 0x09		//日.月.年模式
}TVT_DATE_MODE;

typedef enum _tvt_time_mode_
{
	TVT_TIME_MODE_12		= 0x01,		//12小时制
	TVT_TIME_MODE_24		= 0x02,		//24小时制 默认
}TVT_TIME_MODE;

//事件类型
typedef enum _tvt_event_type_
{
	TVT_EVENT_MOTION		= 0x01,			//移动侦测
    TVT_EVENT_PIR        = 0x02,          //人体感应
    TVT_EVENT_SENSOR		= 0x03,			//传感器报警
    TVT_EVENT_SHELTER		= 0x04,			//遮挡报警
    TVT_EVENT_VLOSS      =  0x05,			//视频丢失
	TVT_EVENT_BEHAVIOR	= 0x06,		    //智能报警
}TVT_EVENT_TYPE;


typedef enum _tvt_alarm_type_
{
	TVT_ALARM_MOTION =0 ,		//移动侦测报警
    TVT_ALARM_PIR,           //人体感应报警
    TVT_ALARM_SENSOR,			//传感器报警
	TVT_ALARM_SHELTER,		//遮挡报警
	TVT_ALARM_VLOSS,			//视频丢失
    TVT_ALARM_BEHAVIOR,	    //智能报警
}TVT_ALARM_TYPE;

typedef enum _tvt_generate_log_type_
{
	TVT_GENERATE_LOG_USER   = 0x01,
	TVT_GENERATE_LOG_CAMERA = 0x02,
	TVT_GENERATE_LOG_DEVICE = 0x03,
}TVT_GENERATE_LOG_TYPE; //生成日志类型,由谁产生的日志

//高位两字节对应contentID，地位两字节对应type。
typedef enum _tvt_log_type_
{
	//报警（Alarm）
	TVT_LOG_ALARM			= 0x00010000,
	TVT_LOG_MOTION_START,				//移动侦测开始
	TVT_LOG_MOTION_END,				//移动侦测结束
    TVT_LOG_PIR_START,				//人体感应报警开始
    TVT_LOG_PIR_END,				    //人体感应报警结束
	TVT_LOG_SHELTER_START,			//遮挡报警开始
	TVT_LOG_SHELTER_END,				//遮挡报警结束
	TVT_LOG_BEHAVIOR_START,			//智能报警开始
	TVT_LOG_BEHAVIOR_END,				//智能报警结束
	TVT_LOG_ALARM_IN_START,			//报警输入开始
	TVT_LOG_ALARM_IN_END,				//报警输入结束
}TVT_LOG_TYPE;

typedef  enum _tvt_log_ext_id_
{
	//报警（Alarm）
	TVT_LOG_ID_ALARM		= 0x00010000,   //TVT_LOG_ALARM
	TVT_LOG_ID_MOTION_START,		        //移动侦测开始			TVT_LOG_MOTION_START,			TVT_LOG_EXT_MOTION
	TVT_LOG_ID_MOTION_END,					//移动侦测结束			TVT_LOG_MOTION_END,				TVT_LOG_EXT_MOTION

    TVT_LOG_ID_SHELTER_START,				//遮挡报警开始			TVT_LOG_SHELTER_START,			TVT_LOG_EXT_SHELTER
	TVT_LOG_ID_SHELTER_END,					//遮挡报警结束			TVT_LOG_SHELTER_END,			TVT_LOG_EXT_SHELTER
	TVT_LOG_ID_BEHAVIOR_START,				//智能报警开始			TVT_LOG_BEHAVIOR_START,			none
	TVT_LOG_ID_BEHAVIOR_END,				//智能报警结束			TVT_LOG_BEHAVIOR_END,			none
	TVT_LOG_ID_ALARM_IN_START,				//报警输入开始			TVT_LOG_ALARM_IN_START,			TVT_LOG_EXT_SENSOR
	TVT_LOG_ID_ALARM_IN_END,				//报警输入结束			TVT_LOG_ALARM_IN_END,			TVT_LOG_EXT_SENSOR
	TVT_LOG_ID_ALARM_OUT_START,				//报警输出开始			TVT_LOG_ALARM_OUT_START,		TVT_LOG_EXT_ALARM_OUT
	TVT_LOG_ID_ALARM_OUT_END,				//报警输出结束			TVT_LOG_ALARM_OUT_END,			none

}TVT_LOG_EXT_ID;

//只能从零开始排布，不可中间赋值简单连续性
typedef enum _tvt_language_
{
	//东亚
	TVT_LANGUAGE_CHINESE_S=0,			//简体中文
	TVT_LANGUAGE_CHINESE_B,			//繁体中文
	TVT_LANGUAGE_JAPANESE,			//日本
	TVT_LANGUAGE_KOREA,				//韩文

	//东南亚
	TVT_LANGUAGE_INDONESIA,			//印度尼西亚
	TVT_LANGUAGE_THAI,				//泰文
	TVT_LANGUAGE_VIETNAMESE,		//越南

	//欧美
	TVT_LANGUAGE_ENGLISH_US,		//英文（美国）
	TVT_LANGUAGE_ENGLISH_UK,		//英文（英国）
	TVT_LANGUAGE_PORTUGUESE,		//葡萄牙
	TVT_LANGUAGE_GREECE,			//希腊
	TVT_LANGUAGE_SPANISH,			//西班牙
	TVT_LANGUAGE_RUSSIAN,			//俄文	
	TVT_LANGUAGE_NORWAY,			//挪威
	TVT_LANGUAGE_ITALY,				//意大利
	TVT_LANGUAGE_CZECH,				//捷克
	TVT_LANGUAGE_GERMAN,			//德文
	TVT_LANGUAGE_DUTCH,				//荷兰
	TVT_LANGUAGE_FINNISH,			//芬兰
	TVT_LANGUAGE_SWEDISH,			//瑞士
	TVT_LANGUAGE_DANISH,			//丹麦
	TVT_LANGUAGE_FRENCH,			//法文
	TVT_LANGUAGE_POLISH,			//波兰
	TVT_LANGUAGE_BULGARIAN,			//保加利亚
	TVT_LANGUAGE_HUNGARY,			//匈牙利

	//中东
	TVT_LANGUAGE_HEBREW,			//希伯来
	TVT_LANGUAGE_TURKEY,			//土耳其
	TVT_LANGUAGE_PERSIAN,			//波斯文
	TVT_LANGUAGE_ARABIC,			//阿拉伯文
	TVT_LANGUAGE_MAX_COUNT,
}TVT_LANGUAGE;


//最多只能定义6个
typedef enum _tvt_default_color_type_
{
	TVT_DEFAULT_COLOR_STANDARD	= 0,
	TVT_DEFAULT_COLOR_INDOOR,
	TVT_DEFAULT_COLOR_LOW_LIGHT,
	TVT_DEFAULT_COLOR_OUTDOOR,
	TVT_DEFAULT_COLOR_REVS1,
	TVT_DEFAULT_COLOR_REVS2,

	TVT_DEFAULT_COLOR_AUTO		= 0x80000000,	//自动切换
	TVT_DEFAULT_COLOR_CHANGE,					//昼夜转换
	TVT_DEFAULT_COLOR_DAY,						//白天
	TVT_DEFAULT_COLOR_NIGHT,					//夜晚
	TVT_DEFAULT_COLOR_OFF		= 0xFFFFFFFF,	//关闭情景模式
}TVT_DEFAULT_COLOR_TYPE;

typedef enum _tvt_nic_type_
{
	TVT_NIC_TYPE_10M_HALF_DUPIEX					= 0x0001,
	TVT_NIC_TYPE_10M_FULL_DUPIEX					= 0x0002,
	TVT_NIC_TYPE_100M_HALF_DUPIEX					= 0x0004,
	TVT_NIC_TYPE_100M_FULL_DUPIEX					= 0x0008,	
	TVT_NIC_TYPE_1000M_FULL_DUPIEX					= 0x0010,
	TVT_NIC_TYPE_10_100_1000M_AUTO_NEGOTIATION		= 0x0020,
	TVT_NIC_TYPE_WIFI								= 0x0040,
	TVT_NIC_TYPE_BLUETOOTH						= 0x0080
}TVT_NIC_TYPE;

//////////////////////////////////////////////////////////////////////////

typedef struct _tvt_device_param_
{
	/*
	硬件接口
	*/
	unsigned char	localAudioNum;			//本地音频输入数目
	unsigned char	localVideoInNum;		//本地视频输入数目
	unsigned char	lineInNum;				//声音输入数目（等于零表示使用第一通道音频对讲）
	unsigned char	lineOutNum;				//声音输出数目

	unsigned char	localSensorInNum;		//本地传感器数目
	unsigned char	localRelayOutNum;		//本地继电器输出数目
	unsigned char	diskCtrlNum;			//本地磁盘接口数目
	unsigned char	eSataNum;				//本地E-sata接口数目

	unsigned char	networkPortNum;			//网络接口数目
	unsigned char	rs485Mode;              //0-半双工，1全双工；目前没有用
	unsigned char	rs232Num;				//RS232接口数目
	unsigned char	rs485Num;				//RS485接口数目

	unsigned char	hdmiNum;				//HDMI接口数目
	unsigned char	usbNum;					//USB接口数目

	/*网络指标*/
	unsigned char	netVideoInNum;  		//网络视频输入数目
	unsigned char	netSensorInNum;			//网络传感器数目
	unsigned char	netRelayOutNum;			//网络继电器输出数目
	unsigned char	maxOnlineUserNum;		//支持的最大在线用户数目（只读）
	
	/*
	设备基本信息
	*/
	unsigned int	deviceID;									//设备ID
	unsigned int	videoFormat;								//视频制式
	unsigned int	deviceLanguage;								//系统语言

	char			mac[8];										//MAC地址
	char			deviceName [TVT_MAX_TITLE_LEN+4];			//设备名称
	
	char			deviceSN[TVT_MAX_TITLE_LEN+4];				//设备序列号
	char			firmwareVersion [TVT_MAX_TITLE_LEN+4];		//固件版本
	char			firmwareBuildDate [TVT_MAX_TITLE_LEN+4];	//固件编译日期
	char			hardwareVersion [TVT_MAX_TITLE_LEN+4];		//硬件版本
	char			kernelVersion [TVT_MAX_TITLE_LEN+4];		//内核版本
	char			mcuVersion [TVT_MAX_TITLE_LEN+4];			//单片机软件版本

	unsigned char	 language[TVT_MAX_LANGUAGE_NUM];				//语言支持	
	char			productionDate[TVT_MAX_TITLE_LEN+4];		//生产日期
	char			manufacturer[TVT_MAX_TITLE_LEN+4];			//生产厂家
}TVT_DEVICE_PARAM;


typedef struct _tvt_system_version_
{
	char			deviceSN[TVT_MAX_TITLE_LEN+4];				//设备序列号
	char			firmwareVersion [TVT_MAX_TITLE_LEN+4];		//固件版本
	char			firmwareBuildDate [TVT_MAX_TITLE_LEN+4];	//固件编译日期
	char			hardwareVersion [TVT_MAX_TITLE_LEN+4];		//硬件版本
	char			kernelVersion [TVT_MAX_TITLE_LEN+4];		//内核版本
	char			mcuVersion [TVT_MAX_TITLE_LEN+4];			//单片机软件版本
}TVT_SYSTEM_VERSION;

typedef struct _tvt_network_status_
{
	unsigned short		bSupportNTP;
	unsigned short		bEnableNTP;

	unsigned short		httpPort;
	unsigned short		dataPort;

	unsigned int		ethNum;
	
	struct  
	{
		unsigned short		recv;
		unsigned short		port;
		char				IP[40];					//多播地址（兼容IPV4和IPV6）
	}multiCast;

	struct  
	{
		char			MAC [8];					//物理地址
		unsigned int	nicStatus;					//网卡状态: 0,connect;1,disconnect.

		struct  
		{
			unsigned short	enable;					//是否启用
			unsigned short	getIPMode;				//获取地址的方式：0-DHCP，1-静态地址，2-PPPoE

			char			IP[16];					//网络地址
			char			subnetMask[16];			//子网掩码
			char			gateway[16];			//网关

			char	preferredDNS[16];				//主网络域名服务器地址
			char	alternateDNS[16];				//从网络域名服务器地址
		}IPv4;

		struct  
		{
			unsigned short	enable;					//是否启用
			unsigned short	getIPMode;				//获取地址的方式：0-DHCP，1-静态地址，2-PPPoE

			unsigned int	subnetMask;
			char			IP[40];		
			char			gateway[40];

			char	preferredDNS[40];			//主网络域名服务器地址
			char	alternateDNS[40];			//从网络域名服务器地址
		}IPv6;

	}ethernet[TVT_MAX_ETH_NUM];

}TVT_NETWORK_STATUS;

#pragma pack()

//////////////////////////////////////////////////////////////////////////
// const unsigned int		TVT_ONE_DAY_SECOND_NUM		= 86400;
// const unsigned int		TVT_LOCAL_DEVICE_ID			= 0;
// const unsigned int		TVT_INVALID_NET_DEVICE_ID	= (~0x0);
// const unsigned int		TVT_LOCAL_CLIENT_ID			= TVT_LOCAL_DEVICE_ID;
// const unsigned int		TVT_INVALID_CLIENT_ID		= (~0x0);
// const unsigned short	TVT_HOLDTIME_ALWAYS			= (~0x0);
//////////////////////////////////////////////////////////////////////////
#endif //__TVT_PUB_DEFINE_H__

