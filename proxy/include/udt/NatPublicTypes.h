// NatPublicTypes.h: interface for the public types of nat.
//
//////////////////////////////////////////////////////////////////////
#ifndef __NAT_PUBLIC_TYPES_H__
#define __NAT_PUBLIC_TYPES_H__

#include "NatUserDefine.h"

#ifdef NAT_TVT_DVR_4_0
#include "TVT_PubCom.h"
#else
#include "PUB_common.h"
#endif//NAT_TVT_DVR_4_0

#include "SWL_Public.h"
/**
 * 穿透服务器信息，主要作为穿透服务器列表项
 */
typedef struct __nat_server_info__
{
    char                szAddr[64];     // 服务器地址
    unsigned short      sPort;          // 穿透端口号
    unsigned short      sTestPort;      // 穿透测试端口号 
}NAT_SERVER_INFO;


#endif//__NAT_PUBLIC_TYPES_H__