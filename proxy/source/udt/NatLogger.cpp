// NatLogger.cpp: implements for the logger of nat.
//
//////////////////////////////////////////////////////////////////////
#include "NatCommon.h"
#include "NatLogger.h"

#include "NatDebug.h"

#include <stdarg.h>
#ifdef __ENVIRONMENT_LINUX__
int  NAT_LOG_TIME_PRINTF(const char *pszFmt, ...)
{
    va_list vaList;
    int   iLen =0;
    int lBufLen= NAT_MAX_LOG_LEN;
    char buf[64]={0};
    char szBuf[NAT_MAX_LOG_LEN]={0};
    struct tm *t;
    time_t tt;
    time(&tt);
    t=localtime(&tt);
    sprintf(buf,"%4d-%02d-%02d %02d:%02d:%02d ",t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);

    iLen = snprintf(szBuf, lBufLen, "%s ",buf);
    va_start(vaList, pszFmt);
    iLen += vsnprintf(szBuf+iLen, lBufLen-iLen, pszFmt, vaList);
    va_end(vaList);

   // iLen += snprintf(szBuf + iLen, lBufLen - iLen, "\n");
    printf("%s",szBuf);
    return 0;
}
#endif