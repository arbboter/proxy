
#include "TVT_PubDefine.h"
#include "Base64.h"

#ifdef __ENVIRONMENT_WIN32__
#include "hs_debug.h"
#else
#ifdef __DEBUG_NEW__
#include "debug_new.h"
#define new DEBUG_NEW
#endif
#endif

const unsigned char Base64IdxTab[128] =
{
	255,255,255,255,  255,255,255,255,  255,255,255,255,  255,255,255,255,
	255,255,255,255,  255,255,255,255,  255,255,255,255,  255,255,255,255,
	255,255,255,255,  255,255,255,255,  255,255,255,62,   255,255,255,63,
	52,53,54,55,      56,57,58,59,      60,61,255,255,    255,255,255,255,
	255,0,1,2,        3,4,5,6,          7,8,9,10,         11,12,13,14,
	15,16,17,18,      19,20,21,22,      23,24,25,255,     255,255,255,255,
	255,26,27,28,     29,30,31,32,      33,34,35,36,      37,38,39,40,
	41,42,43,44,      45,46,47,48,      49,50,51,255,     255,255,255,255
};

const char Base64ValTab[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

CBase64::CBase64(void)
{
	m_lineMaxLen = 0x7fffffff;
}

CBase64::~CBase64(void)
{
}

void CBase64::SetLineMaxLen(long lineMaxLen)
{
	m_lineMaxLen = lineMaxLen;
}

long CBase64::Encode(const unsigned char *pSrc, long srcLen, char *pDest, long destLen)
{
	int i = 0;
	int loop = 0;
	int remain = 0;
	int retLen = 0;
	int lineLen = 0;

	loop = srcLen/3;
	remain = srcLen%3;

	// also can encode native char one by one as decode method
	// but because all of char in native string  is to be encoded so encode 3-chars one time is easier.

	for (i=0; i<loop; i++)
	{
		unsigned char a1 = (pSrc[i*3] >> 2);
		unsigned char a2 = ( ((pSrc[i*3] & 0x03) << 4) | (pSrc[i*3+1] >> 4) );
		unsigned char a3 = ( ((pSrc[i*3+1] & 0x0F) << 2) | ((pSrc[i*3+2] & 0xC0) >> 6) );
		unsigned char a4 = (pSrc[i*3+2] & 0x3F);

		pDest[retLen++] = Base64ValTab[a1];
		if(++lineLen == m_lineMaxLen)
		{
			lineLen = 0;
			pDest[retLen++] = '\r';
			pDest[retLen++] = '\n';
		}

		pDest[retLen++] = Base64ValTab[a2];
		if(++lineLen == m_lineMaxLen)
		{
			lineLen = 0;
			pDest[retLen++] = '\r';
			pDest[retLen++] = '\n';
		}

		pDest[retLen++] = Base64ValTab[a3];
		if(++lineLen == m_lineMaxLen)
		{
			lineLen = 0;
			pDest[retLen++] = '\r';
			pDest[retLen++] = '\n';
		}

		pDest[retLen++] = Base64ValTab[a4];
		if(++lineLen == m_lineMaxLen)
		{
			lineLen = 0;
			pDest[retLen++] = '\r';
			pDest[retLen++] = '\n';
		}
	}

	if (remain == 1)
	{
		// should pad two equal sign
		i = srcLen-1;
		unsigned char a1 = (pSrc[i] >> 2);
		unsigned char a2 = ((pSrc[i] & 0x03) << 4);

		pDest[retLen++] = Base64ValTab[a1];
		if(++lineLen == m_lineMaxLen)
		{
			lineLen = 0;
			pDest[retLen++] = '\r';
			pDest[retLen++] = '\n';
		}

		pDest[retLen++] = Base64ValTab[a2];
		if(++lineLen == m_lineMaxLen)
		{
			lineLen = 0;
			pDest[retLen++] = '\r';
			pDest[retLen++] = '\n';
		}

		pDest[retLen++] = '=';
		if(++lineLen == m_lineMaxLen)
		{
			lineLen = 0;
			pDest[retLen++] = '\r';
			pDest[retLen++] = '\n';
		}

		pDest[retLen++] = '=';
		pDest[retLen] = 0x00;
	}
	else if (remain == 2)
	{
		// should pad one equal sign
		i = srcLen-2;
		unsigned char a1 = (pSrc[i] >> 2);
		unsigned char a2 = ( ((pSrc[i] & 0x03) << 4) | (pSrc[i+1] >> 4));
		unsigned char a3 = ( (pSrc[i+1] & 0x0F) << 2);

		pDest[retLen++] = Base64ValTab[a1];
		if(++lineLen == m_lineMaxLen)
		{
			lineLen = 0;
			pDest[retLen++] = '\r';
			pDest[retLen++] = '\n';
		}

		pDest[retLen++] = Base64ValTab[a2];
		if(++lineLen == m_lineMaxLen)
		{
			lineLen = 0;
			pDest[retLen++] = '\r';
			pDest[retLen++] = '\n';
		}

		pDest[retLen++] = Base64ValTab[a3];
		if(++lineLen == m_lineMaxLen)
		{
			lineLen = 0;
			pDest[retLen++] = '\r';
			pDest[retLen++] = '\n';
		}

		pDest[retLen++] = '=';
		pDest[retLen] = 0x00;
	}
	else
	{
		// just division by 3
		pDest[retLen] = 0x00;
	}

	return retLen;
}

long CBase64::Decode(const char *pSrc, long srcLen, unsigned char *pDest, long destLen)
{
	int i = 0;
	int iCnt = 0;

	unsigned char * p = pDest;

	for (i=0; i<srcLen; i++)
	{
		if (pSrc[i] > 127)
		{
			continue;
		}

		if (pSrc[i] == '=')
		{
			return p-pDest+1;
		}

		unsigned char a = Base64IdxTab[pSrc[i]];
		if (a == 255)
		{
			continue;
		}

		switch (iCnt)
		{
		case 0:
			{
				*p = a << 2;
				iCnt++;
			}
			break;

		case 1:
			{
				*p++ |= a >> 4;
				*p = a << 4;
				iCnt++;
			}
			break;

		case 2:
			{
				*p++ |= a >> 2;
				*p = a << 6;
				iCnt++;
			}
			break;

		case 3:
			{
				*p++ |= a;
				iCnt = 0;
			}
			break;
		} 
	}

	*p = 0x00;
	return p-pDest;
}
