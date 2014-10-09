
// NatLogger.h: interface for the Logger.
//
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_LOGGER_H_
#define _NAT_LOGGER_H_

#if defined(DEBUG) || defined(__DEBUG) || defined(_DEBUG_) || defined(__DEBUG__)
#if !defined(_DEBUG)
#define _DEBUG
#endif
#endif

#define NAT_LOG_LEVEL_NONE   0
#define NAT_LOG_LEVEL_FATAL  1
#define NAT_LOG_LEVEL_ERROR  2
#define NAT_LOG_LEVEL_WARN   3
#define NAT_LOG_LEVEL_INFO   4
#define NAT_LOG_LEVEL_DEBUG  5

#define NAT_LOG_LEVEL_ALL    NAT_LOG_LEVEL_DEBUG

#ifndef NAT_LOG_LEVEL


#ifdef _DEBUG

#define NAT_LOG_LEVEL NAT_LOG_LEVEL_DEBUG

#else

#define NAT_LOG_LEVEL NAT_LOG_LEVEL_INFO

#endif//_DEBUG

#endif//NAT_LOG_LEVEL

#define NAT_LOG_TITLE_FATAL  "FATAL"
#define NAT_LOG_TITLE_ERROR  "ERROR"
#define NAT_LOG_TITLE_WARN   "WARN"
#define NAT_LOG_TITLE_INFO   "INFO"
#define NAT_LOG_TITLE_DEBUG  "DEBUG"


#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)

#ifdef   __ENVIRONMENT_WIN32__

#define __NAT_LOG_PRINTF__(format, ...) TRACE(format, ## __VA_ARGS__)

#define __NAT_LOG_SYS_ERROR_DETAIL__(message, errorCode) __NAT_LOG_PRINTF__(message ": ErrorNo=%d\n", errorCode)

#elif defined __ENVIRONMENT_LINUX__

#define  NAT_MAX_LOG_LEN   1024

#define __NAT_LOG_PRINTF__(format, ...) NAT_LOG_TIME_PRINTF(format, ## __VA_ARGS__)

#define __NAT_LOG_SYS_ERROR_DETAIL__(message, errorCode) perror(message)

#endif

#define __NAT_LOG__(title, format, ...) __NAT_LOG_PRINTF__(__FILE__ "("__STR1__(__LINE__)"): " title ": " format, ## __VA_ARGS__)

#define __NAT_LOG_SYS_ERROR_COMMON__(message, errorCode) __NAT_LOG_SYS_ERROR_DETAIL__(__FILE__ "("__STR1__(__LINE__)"): "\
    NAT_LOG_TITLE_ERROR ": [SYS]" message, errorCode)

#ifdef   __ENVIRONMENT_WIN32__

#define __NAT_LOG_SYS_ERROR__(message) __NAT_LOG_SYS_ERROR_COMMON__(message, GetLastError())

#define __NAT_LOG_ERROR_SOCKET__(message) __NAT_LOG_SYS_ERROR_COMMON__(message, WSAGetLastError())

#elif defined __ENVIRONMENT_LINUX__

#define __NAT_LOG_SYS_ERROR__(message) __NAT_LOG_SYS_ERROR_COMMON__(message, errno)

#define __NAT_LOG_ERROR_SOCKET__(message) __NAT_LOG_SYS_ERROR_COMMON__(message, errno)

#endif

// LOG FATAL
#if NAT_LOG_LEVEL >= NAT_LOG_LEVEL_FATAL

#define NAT_LOG_FATAL(format, ...) __NAT_LOG__(NAT_LOG_TITLE_FATAL, format, ## __VA_ARGS__)

#else

#define NAT_LOG_FATAL(format, ...)

#endif


// LOG ERROR
#if NAT_LOG_LEVEL >= NAT_LOG_LEVEL_ERROR

#define NAT_LOG_ERROR(format, ...) __NAT_LOG__(NAT_LOG_TITLE_ERROR, format, ## __VA_ARGS__)

#define NAT_LOG_ERROR_SYS(message) __NAT_LOG_SYS_ERROR__(message)

#define NAT_LOG_ERROR_SOCKET(message) __NAT_LOG_ERROR_SOCKET__(message)

#else

#define NAT_LOG_ERROR(format, ...)

#define NAT_LOG_ERROR_SYS(message)

#endif

// LOG WARN
#if NAT_LOG_LEVEL >= NAT_LOG_LEVEL_WARN

#define NAT_LOG_WARN(format, ...) __NAT_LOG__(NAT_LOG_TITLE_WARN, format, ## __VA_ARGS__)

#else

#define NAT_LOG_WARN(format, ...)

#endif

// LOG INFO
#if NAT_LOG_LEVEL >= NAT_LOG_LEVEL_INFO

#define NAT_LOG_INFO(format, ...) __NAT_LOG__(NAT_LOG_TITLE_INFO, format, ## __VA_ARGS__)

#else

#define NAT_LOG_INFO(format, ...)

#endif

// LOG DEBUG
#if NAT_LOG_LEVEL >= NAT_LOG_LEVEL_DEBUG

#define NAT_LOG_DEBUG(format, ...) __NAT_LOG__(NAT_LOG_TITLE_DEBUG, format, ## __VA_ARGS__)

#else

#define NAT_LOG_DEBUG(format, ...)

#endif
#ifdef __ENVIRONMENT_LINUX__
int  NAT_LOG_TIME_PRINTF(const char *pszFmt, ...);
#endif

#endif//_NAT_LOGGER_H_