/***********************************************************************
** 深圳同为数码科技有限公司保留所有权利。
** 作者			: 袁石维
** 日期         : 2012-04-23
** 名称         : TVT_Error.h
** 版本号		: 1.0
** 描述			:
** 修改记录		:
***********************************************************************/
#ifndef __TVT_ERROR_H__
#define __TVT_ERROR_H__

//////////////////////////////////////////////////////////////////////////
typedef enum _tvt_msg_ack_
{
	TVT_MSG_ACK_SUCC,								//成功


	TVT_MSG_ACK_PROTOCOL_NOT_MATCH		= 0x0300,	//协议不匹配
	TVT_MSG_ACK_DEVICE_UNINITIAL,					//设备未初始化
	TVT_MSG_ACK_CONNECT_OVERLOAD,					//网络连接超载
	TVT_MSG_ACK_SYSTEM_BUSY,						//系统忙
	TVT_MSG_ACK_NO_CAPACITY,						//系统能力不足


	TVT_MSG_ACK_ID_PW_WRONG 		     = 0x0400,	//用户名或者密码错误
	TVT_MSG_ACK_USER_BLOCK,							//用户被屏蔽:黑白名单,MAC地址,被踢出
	TVT_MSG_ACK_USER_MAX,							//达到网络最大用户个数
	TVT_MSG_ACK_HIDE_CHANNEL,						//通道被隐藏:请求的通道都无法操作,全部都被隐藏通道屏蔽
	TVT_MSG_ACK_NO_AUTHORITY,						//无操作权限:请求的通道都无法操作,包括部分被隐藏屏蔽
	TVT_MSG_ACK_NO_FULL_AUTHORITY,					//只有部分权限:请求的通道,只要有一个及以上可以操作
	TVT_MSG_ACK_NO_FUNC,							//不支持该功能
	TVT_MSG_ACK_GET_CONFIG_FAIL,					//获取配置失败
	TVT_MSG_ACK_SET_CONFIG_FAIL,					//保存配置失败
	TVT_MSG_ACK_GET_DATA_FAIL,						//获取数据失败
	TVT_MSG_ACK_NO_CHG_TIME_FOR_DISK,               //正在格式化磁盘,不能修改时间.请稍后再试
	TVT_MSG_ACK_NO_CHG_TIME_FOR_READ,               //正在读磁盘,不能修改时间.请稍后再试
	TVT_MSG_ACK_NO_CHG_TIME_FOR_TALK,               //正在对讲,不能修改时间.请稍后再试


	TVT_MSG_ACK_TIME_OUT                 = 0x0500,	//网络等待超时
	TVT_MSG_ACK_CONNECT_SERVER_FAIL,				//连接服务器错误
	TVT_MSG_ACK_SEND_FAIL,							//send命令发送数据失败
	TVT_MSG_ACK_DDNS_REGISTER_FAIL,					//DDNS注册失败
	TVT_MSG_ACK_DDNS_HOSTDOMAIN_ERR,				//DDNS用户名、密码或域名错误
	TVT_MSG_ACK_TALK_LINE_BUSY,	                    //对讲占线
	TVT_MSG_ACK_DEVICE_DOWN,						//网络未连接,未插网线,网卡被禁用
	TVT_MSG_ACK_DHCP,						        //DHCP不可用
	TVT_MSG_ACK_DHCP6,						        //ipv6的DHCP不可用
	TVT_MSG_ACK_IP_CONFLICT,						//静态设置的IP冲突
	TVT_MSG_ACK_NETWORK_PARAM_FORMAT_ERR,			//网络参数格式错误
	TVT_MSG_ACK_NETWORK_UNREACHABLE,				//网络不可达
	TVT_MSG_ACK_CANNT_PREVIEW,						//不能预览（原因可能有密码错误，无权限等）

	TVT_MSG_ACK_PTZ_WRITE_COM_FAIL       = 0x0600,	//写串口失败
	TVT_MSG_ACK_PTZ_OPERATE_FAIL,					//操作云台失败

	TVT_MSG_ACK_GET_DISK_FAIL            = 0x0700,  //获取磁盘信息失败
	TVT_MSG_ACK_NETDISK_NO_SUPPORT,                 //该机器不支持网络磁盘
	TVT_MSG_ACK_NETDISK_CONNECT_LOST,				//网络磁盘操作连接丢失
	TVT_MSG_ACK_NETDISK_PASSWD_ERR,					//网络磁盘操作用户名密码错误
	TVT_MSG_ACK_NETDISK_RELOGIN_ERR,				//网络磁盘操作重复登录
	TVT_MSG_ACK_NETDISK_ADDINDEX_ERR,				//该机器支持的网络磁盘个数已满
	TVT_MSG_ACK_DISK_FORMATING,					    //正在格式化磁盘。
	TVT_MSG_ACK_DISK_FORMAT_FAIL,                   //格式化失败	
	TVT_MSG_ACK_CHANGEDISK_PROPERTY_ERR,            //修改磁盘属性失败。
	TVT_MSG_ACK_CHANGE_GROUP_ERR,                   //分组错误。原因：修改群组要遵循下面的要求：至少要有一个群组；为每个磁盘都分配了群组；为每个通道到配置了群组。
	TVT_MSG_ACK_GROUP_HAVE_BE_CHANGE,               //群组有变化。请先保存群组。
	TVT_MSG_ACK_DISK_SPACE_ERR,                     //磁盘容量要大于10G。
	TVT_MSG_ACK_INVALID_PATH,                       //无效路径
	TVT_MSG_ACK_NO_ENOUGH_SPACE,                    //无足够磁盘空间
	TVT_MSG_ACK_DISK_READING,                       //磁盘正在备份或回放中
	TVT_MSG_ACK_REMOVEDISK_WRITE_ERR,				//可移动磁盘(U盘)容量不够,或掉线
	TVT_MSG_ACK_UPGRADE_NO_SPACE,                   //升级用的U盘剩余容量要大于500M，且U盘支持可读写
	TVT_MSG_ACK_UPGRADE_FILE_ERR,                   //无效的文件
	TVT_MSG_ACK_UPGRADE_FAIL,						//升级失败

	TVT_MSG_ACK_CREATE_THREAD_FAIL			= 0x0800,
	TVT_MSG_ACK_ALLOC_MEM_FAIL,
	

	TVT_MSG_ACK_PARAM_ERROR              = 0x1000,	//参数错误
	TVT_MSG_ACK_UNKNOW_ERROR,						//未知错误


}TVT_MSG_ACK;
#endif //__TVT_ERROR_H__
