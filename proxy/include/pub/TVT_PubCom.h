/***********************************************************************
** 深圳同为数码科技有限公司保留所有权利。
** 作者			: 袁石维
** 日期         : 2011-11-02
** 名称         : TVT_PubCom.h
** 版本号		: 1.0
** 描述			:
** 修改记录		:
***********************************************************************/
#ifndef __TVT_PUBLIC_COM_H__
#define __TVT_PUBLIC_COM_H__

#include "TVT_Error.h"
#include "TVT_PubDefine.h"
#include "TVT_DevDefine.h"
#ifdef   WIN32
typedef  HANDLE						PUB_thread_t;			//线程ID
typedef  LPTHREAD_START_ROUTINE		PUB_start_routine;		//线程函数
typedef  CRITICAL_SECTION			PUB_lock_t;				//锁
typedef	 HANDLE						PUB_sem_t;				//信号量
typedef	 DWORD						PUB_THREAD_RESULT;		//线程函数返回结果
#define	 PUB_THREAD_CALL			WINAPI					//函数调用方式
#define  PUB_CREATE_THREAD_FAIL		NULL					//创建线程失败
//#define  printf                     TRACE
#define  snprintf                   _snprintf
#else			
typedef  pthread_t					PUB_thread_t;			//线程ID
typedef  void *(*start_routine)(void*);
typedef  start_routine				PUB_start_routine;		//线程函数
typedef  pthread_mutex_t			PUB_lock_t;				//锁
typedef	 sem_t						PUB_sem_t;				//信号量
typedef	 void*						PUB_THREAD_RESULT;		//线程函数返回结果
#define	 PUB_THREAD_CALL									//函数调用方式
#define  PUB_CREATE_THREAD_FAIL		0						//创建线程失败

typedef	 unsigned long long			ULONGLONG;
#endif

#define PUB_THREAD_ID_NOINIT  PUB_CREATE_THREAD_FAIL    //没有初始化的线程ID
//////////////////////////////////////////////////////////////////////////
#define IP_ADDRESS(ip1, ip2, ip3, ip4) ((ip1) + (ip2<<8) + (ip3<<16) + (ip4<<24))
#ifndef V4mmioFOURCC
#define V4mmioFOURCC( ch0, ch1, ch2, ch3 ) \
	( (unsigned int)(unsigned char)(ch0) | ( (unsigned int)(unsigned char)(ch1) << 8 ) |	\
	( (unsigned int)(unsigned char)(ch2) << 16 ) | ( (unsigned int)(unsigned char)(ch3) << 24 ) )
#endif

const char PATH_DNS[]	= "/etc/resolv.conf";
const unsigned char DNS_REQ_DATA_HEAD[12] = {0x2b,0x50,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00};
const unsigned char DNS_REQ_DATA_TAIL[4]  = {0x00,0x01,0x00,0x01};
const unsigned short DNS_PORT = 53;
//////////////////////////////////////////////////////////////////////////
int GetTime();
TVT_DATE_TIME	GetCurrTime();
TVT_LOCAL_TIME GetLocalTime();
#define PRINT_TIME(time) printf("Time: %d-%d-%d %d:%d:%d %d\n", time.year, time.month, time.mday, time.hour, time.minute, time.second, time.microsecond);
#define PRINT_LOG(str) printf("Log: %s, %d, %s\n", __FILE__, __LINE__, str);
#define PRINT_DEBUG(str) printf("Debug: %s, %d, %s\n", __FILE__, __LINE__, str);

//获取DNS对应的ip地址
bool ParseDomainName(char * pDNS, size_t &length);
#ifdef WIN32
bool PUB_AssertIPAddr(const char *IpAddr);
#endif
struct hostent * GetHostbyname(const char * pDNS, char * pDnsServer);
struct hostent * GetHostbyname(const char * pDNS);
void ReleaseHost(struct hostent *pHost);

TVT_LOCAL_TIME GetBuildDate();

//创建线程
PUB_thread_t PUB_CreateThread(PUB_start_routine start_routine, void* pParam, bool *pRun);
PUB_thread_t PUB_CreateThreadEx(PUB_start_routine start_routine, void* pParam, bool *pRun, int policy);
//销毁线程（会阻塞等待线程结束）
void PUB_ExitThread(PUB_thread_t *pThreadID, bool *pRun);

//以为毫秒为单位的睡眠，
void PUB_Sleep(unsigned int dwMillisecond);

//打印致命错误
void PUB_PrintError(const char* pFile, int iLine);

//初始化锁，初始化后是可递归的锁
void PUB_InitLock(PUB_lock_t *pLock);

//销毁锁
void PUB_DestroyLock(PUB_lock_t *pLock);

bool PUB_IsTimeOut(unsigned long dwStartTime, unsigned long dwDisTime, unsigned long *pCurTime);
unsigned int PUB_GetTickCount();

TVT_LOCAL_TIME GetBuildDate();

unsigned int PUB_RecFrameCheck(TVT_DATA_FRAME * pFrameInfo);
//通过.dat文件的完整路径查找所属磁盘编号
int PUB_GetDiskNoByFilePath(const char *pPath);

//利用构造和析构函数自动加锁解锁
class CPUB_LockAction
{
public:
	explicit CPUB_LockAction(PUB_lock_t* pLock);
	~CPUB_LockAction();

private:
	PUB_lock_t* m_pLock;
};

class CPUB_Lock
{
public:
	CPUB_Lock();
	~CPUB_Lock();

	void Lock();	//加锁
	void UnLock();	//解锁

#ifdef __DEBUG_LOCK__
	void TestLock(unsigned long line, const char * pFileName);
#endif

#ifndef	WIN32
	void CondWait();
	void CondSignal();
	int TimeLock(int iMicrosecond);	//BUGS在arm-uclibc-linux-g++上编译，执行这个函数有问题，不能超时
	int TryLock();
#endif
private:
	PUB_lock_t m_Lock;
#ifdef __DEBUG_LOCK__
	char		m_szLockFile[128];
	int			m_iLockLine;
	pid_t		m_lockThread;
	
#endif
#ifndef	WIN32
	pthread_cond_t m_cond;
#endif
};

#ifdef __DEBUG_LOCK__
#undef Lock
#define Lock() TestLock(__LINE__, __FILE__)
#endif

#ifndef WIN32
class CThreadManager
{
public:
	CThreadManager();
	~CThreadManager();
	static void ExitThread(unsigned int handle);
	static PUB_THREAD_RESULT PUB_THREAD_CALL ThreadManagerProc(void * lpParameter);
	void ThreadManagerFunc();
private:
	bool m_ThreadManagerExit;
	unsigned int m_ThreadManagerThread;
	std::list<unsigned int> m_ThreadList;
	CPUB_Lock m_ThreadListLock;
};
#endif

#define INFINITE            0xFFFFFFFF// Infinite timeout

class CWaitEvent
{
public:
	CWaitEvent();
	~CWaitEvent();
	void CreateWaitEvent();
	unsigned int WaitForCond(unsigned int dwMilliseconds = INFINITE);  /////等待信号 默认参数为
	void SetEvent();  /////设置为有信号
	void PulseEvent();
	void Close();
private:

#ifdef WIN32
	HANDLE           m_Event;
#else
	void Gettimespec(struct timespec* ptimespec, int offset);
	pthread_mutex_t  m_lock;
	pthread_cond_t   m_Event;
#endif
};

#endif//__TVT_PUBLIC_COM_H__
