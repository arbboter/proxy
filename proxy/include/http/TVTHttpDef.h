/////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __TVT_HTTP_DEF_H__
#define __TVT_HTTP_DEF_H__

#include <list>
#include <string.h>
#include <assert.h>

#include "TVT_PubCom.h"

const char HTTP_VER_1_1[]="HTTP/1.1";
const char HTTP_VER_1_0[]="HTTP/1.0";

#define  HTTP_HEAD_BUF_LEN		128 * 1024
#define  HTTP_BODY_BUF_LEN		512 * 1024
#define  HTTP_RECV_BUF_LEN		40960 * 2
#define  HTTP_SEND_BUF_LEN		4096 * 1024
#define  MAX_COOKIE_LEN			20480
#define  MAX_SYNC_TIME			4000		//最大收发数据等待时间


#ifdef WIN32
#define strncasecmp strnicmp
#define strcasecmp stricmp
#endif

typedef enum _header_type_
{
	HEADER_GENERAL	= 0,
	HEADER_REQUEST	= 1,
	HEADER_RESPONSE	= 2,
	HEADER_ENTITY	= 3,
}HEADER_TYPE;

typedef struct _msg_header_
{
	char *pName;
	char *pValue;
	HEADER_TYPE type;
}MSG_HEADER;

typedef enum __http_body_type__
{
	HTTP_BODY_TYPE_NONE,
	HTTP_BODY_TYPE_APP_JSON,
	HTTP_BODY_TYPE_APP_RRLENCODED,
	HTTP_BODY_TYPE_MUL_FORM_DATA,
	HTTP_BODY_TYPE_IMAGE_JPEG,
	HTTP_BODY_TYPE_HTML,
	HTTP_BODY_TYPE_XML,
}HTTP_BODY_TYPE;

typedef struct __http_body_info__
{
	char * pBodyBuf;
	int iBodyBufLen; 
	int iBodyLen;
	int iOperatePos;
	int iTotalBodyLen;
	int iBodyType;
	__http_body_info__()
	{
		pBodyBuf = NULL;
		iBodyBufLen = 0;
		iBodyLen = 0;
		iOperatePos = 0;
		iTotalBodyLen = 0;
		iBodyType = 0;
	}

	~__http_body_info__()
	{
		if (NULL != pBodyBuf)
		{
			delete [] pBodyBuf;
			pBodyBuf = NULL;
		}

		iBodyBufLen = 0;
		iBodyLen = 0;
		iBodyType = 0;
	}

}HTTP_BODY_INFO;

typedef struct __http_head_info__
{
	char * pHeadBuf;
	int iHeadBufLen;
	int iHeadLen;
	int iOperatePos;
	__http_head_info__() : pHeadBuf(NULL), iHeadBufLen(0), iHeadLen(0), iOperatePos(0){;}
}HTTP_HEAD_INFO;

typedef std::list<MSG_HEADER> MSG_HEADER_LIST;
typedef std::list<MSG_HEADER>::iterator MSG_HEADER_LIST_ITER;

#endif


