// NatUserDefine.h: interface for the custom define for nat user.
//
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_USER_DEFINE_H_
#define _NAT_USER_DEFINE_H_

/************************************************************************/
/* This header file is used for custom define.                          */
/* All of the public headers must include this header first.            */
/************************************************************************/

/**
 * DVR平台的预定义配置
 * NAT_TVT_DVR_4_0 表示DVR4.0平台
 * NAT_TVT_DVR_3_0 表示DVR3.0平台
 *
 * 默认（即如果不配置）是NAT_TVT_DVR_3_0
 */
#define NAT_TVT_DVR_4_0
// or 
// #define NAT_TVT_DVR_3_0

#ifdef NAT_TVT_DVR_4_0
// declare the platform for nat 
#  if defined(WIN32) || defined(_WIN32)
#     ifndef __ENVIRONMENT_WIN32__
#         define __ENVIRONMENT_WIN32__
#     endif
#  else 
#     ifndef __ENVIRONMENT_LINUX__
#         define __ENVIRONMENT_LINUX__
#     endif
#  endif
#endif//NAT_TVT_DVR_4_0

/**
 * 穿透设备信息是否使用扩展版：定义，表示扩展版；不定义，表示标准版。
 * 标准版，则设备的注册信息，使用通用的字段信息；
 * 扩展版，则设备的注册信息，除了主要信息（如设备序列号），其它信息需要使用者以XML格式封装到“ExtraInfo”中
 */
#undef __NAT_DEVICE_CONFIG_EX__

/**
 * 穿透模块是否需要定义函数PUB_IsTimeOutEx()
 */
#define __NAT_PUB_IsTimeOutEx__

#endif//_NAT_USER_DEFINE_H_