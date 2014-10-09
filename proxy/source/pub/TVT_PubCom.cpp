/***********************************************************************
** 深圳同为数码科技有限公司保留所有权利。
** 作者			: 袁石维
** 日期         : 2011-11-07
** 名称         : TVT_PubCom.cpp
** 版本号		: 1.0
** 描述			:
** 修改记录		:
***********************************************************************/
#include "TVT_PubCom.h"
#include "TVT_Config.h"
#ifdef __DEBUG_NEW__
#include "debug_new.h"
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////////
/***********************_tvt_date_time_**********************************/
_tvt_date_time_::_tvt_date_time_()
{
	seconds			= 0;
	microsecond		= 0;
}

_tvt_date_time_ & _tvt_date_time_::operator=(tm & time)
{
	{
		assert((100 < time.tm_year) && (time.tm_year < 137));
		assert((0 <= time.tm_mon) && (time.tm_mon < 12));
		assert((0 < time.tm_mday) && (time.tm_mday < 32));

		assert((0 <= time.tm_hour) && (time.tm_hour < 24));
		assert((0 <= time.tm_min) && (time.tm_min < 60));
		assert((0 <= time.tm_sec) && (time.tm_sec < 60));
	}

	tm tm_time = time;
	tm_time.tm_isdst	= -1;	//让系统自动判定是否处于夏令时

	seconds			= (int)mktime(&tm_time);
	microsecond		= 0;

	return *this;
}

_tvt_date_time_ & _tvt_date_time_::operator=(const timeval & time)
{
	seconds			= time.tv_sec;
	microsecond		= time.tv_usec;

	return *this;
}

_tvt_date_time_ & _tvt_date_time_::operator=(const _tvt_local_time_ & time)
{
	{
		tm tm_time;
		memset(&tm_time, 0, sizeof(tm));

		tm_time.tm_year		= time.year - 1900;
		tm_time.tm_mon		= time.month - 1;
		tm_time.tm_mday		= time.mday;

		tm_time.tm_hour		= time.hour;
		tm_time.tm_min		= time.minute;
		tm_time.tm_sec		= time.second;

		tm_time.tm_isdst	= -1;	//让系统自动判定是否处于夏令时

		{
			assert((100 < tm_time.tm_year) && (tm_time.tm_year < 137));
			assert((0 <= tm_time.tm_mon) && (tm_time.tm_mon < 12));
			assert((0 < tm_time.tm_mday) && (tm_time.tm_mday < 32));

			assert((0 <= tm_time.tm_hour) && (tm_time.tm_hour < 24));
			assert((0 <= tm_time.tm_min) && (tm_time.tm_min < 60));
			assert((0 <= tm_time.tm_sec) && (tm_time.tm_sec < 60));
		}

		seconds			= (int)mktime(&tm_time);
	}
	
	microsecond		= time.microsecond;

	return *this;
}

tm _tvt_date_time_::TM() const
{
	tm time;
	memset(&time, 0, sizeof(tm));

	time_t SecNum = seconds;

#ifdef WIN32
	if (0 != localtime_s(&time, &SecNum))
#else
	if (NULL == localtime_r(&SecNum, &time))
#endif
	{
		assert(false);
	}

	return time;
}

timeval _tvt_date_time_::TimeVal() const
{
	timeval time;

	time.tv_sec		= seconds;
	time.tv_usec	= microsecond;

	return time;
}

_tvt_local_time_ _tvt_date_time_::LocalTime() const
{
	_tvt_local_time_ time;

	tm tm_time;
	memset(&tm_time, 0, sizeof(tm));

	time_t SecNum = seconds;

#ifdef WIN32
	if (0 != localtime_s(&tm_time, &SecNum))
#else
	if (NULL == localtime_r(&SecNum, &tm_time))
#endif
	{
		assert(false);
		memset(&time, 0, sizeof(_tvt_local_time_));
	}
	else
	{
		time.year	= tm_time.tm_year + 1900;
		time.month	= tm_time.tm_mon + 1;
		time.mday	= tm_time.tm_mday;

		time.hour	= tm_time.tm_hour;
		time.minute	= tm_time.tm_min;
		time.second	= tm_time.tm_sec;

		time.microsecond	= microsecond;
	}

	return time;
}

_tvt_date_time_ & _tvt_date_time_::operator+=(int usec)
{
	if (usec < 0)
	{
		usec = labs(usec);
		(*this) -= usec;
	}
	else//0 <= usec
	{
		int sum = microsecond + usec;
		
		//需要进位
		if (1000000 <= sum)
		{
			seconds += (sum / 1000000);
		}

		microsecond = (sum % 1000000);
	}

	return *this;
}

_tvt_date_time_ & _tvt_date_time_::operator-=(int usec)
{
	if (usec < 0)
	{
		usec = labs(usec);
		(*this) += usec;
	}
	else//0 <= usec
	{
		seconds -= (usec / 1000000);

		int u = (usec % 1000000);
		//需要借位
		if (microsecond < u)
		{
			-- seconds;
			microsecond = (1000000 + microsecond - u);
		}
		else
		{
			microsecond -= u;
		}
	}

	return *this;
}

//返回微秒
long long _tvt_date_time_::Dec(const _tvt_date_time_ & time)
{
	long long	usec = seconds;
	usec = (1000000*usec) + microsecond;

	long long	usec1 = time.seconds;
	usec1 = (1000000*usec1) + time.microsecond;

	return (usec - usec1);
}

//返回秒
int _tvt_date_time_::operator-(const _tvt_date_time_ & time)
{
	return this->seconds - time.seconds;
}

bool _tvt_date_time_::operator!=(const _tvt_date_time_ & time) const
{
	return (0 == memcmp(this, &time, sizeof(TVT_DATE_TIME))) ? false : true;
}

bool _tvt_date_time_::operator==(const _tvt_date_time_ & time) const
{
	return (0 == memcmp(this, &time, sizeof(TVT_DATE_TIME))) ? true : false;
}

bool _tvt_date_time_::operator<(const _tvt_date_time_ & time) const
{
	if (this->seconds < time.seconds)
	{
		return true;
	}
	else
	{
		if (this->seconds == time.seconds)
		{
			return (this->microsecond < time.microsecond);
		}
		else
		{
			return false;
		}
	}
}

bool _tvt_date_time_::operator<=(const _tvt_date_time_ & time) const
{
	if (this->seconds < time.seconds)
	{
		return true;
	}
	else
	{
		if (this->seconds == time.seconds)
		{
			return (this->microsecond <= time.microsecond);
		}
		else
		{
			return false;
		}
	}
}

bool _tvt_date_time_::operator>(const _tvt_date_time_ & time) const
{
	if (this->seconds > time.seconds)
	{
		return true;
	}
	else
	{
		if (this->seconds == time.seconds)
		{
			return (this->microsecond > time.microsecond);
		}
		else
		{
			return false;
		}
	}
}

bool _tvt_date_time_::operator>=(const _tvt_date_time_ & time) const
{
	if (this->seconds > time.seconds)
	{
		return true;
	}
	else
	{
		if (this->seconds == time.seconds)
		{
			return (this->microsecond >= time.microsecond);
		}
		else
		{
			return false;
		}
	}
}

/***********************_tvt_data_frame_**********************************/
bool _tvt_data_frame_::operator!=(const _tvt_data_frame_ & frame) const
{
	return (0 == memcmp(this, &frame, sizeof(TVT_DATA_FRAME))) ? false : true;
}

bool _tvt_data_frame_::operator==(const _tvt_data_frame_ & frame) const
{
	return (0 == memcmp(this, &frame, sizeof(TVT_DATA_FRAME))) ? true : false;
}


//////////////////////////////////////////////////////////////////////////
int  GetTime()
{
	int time;

#ifdef WIN32
	SYSTEMTIME systemtime;
	GetLocalTime(&systemtime);

	tm tm_time;

	tm_time.tm_year		= systemtime.wYear - 1900;
	tm_time.tm_mon		= systemtime.wMonth - 1;
	tm_time.tm_mday		= systemtime.wDay;

	tm_time.tm_wday		= systemtime.wDayOfWeek;
	tm_time.tm_isdst	= -1; //让系统自动判定是否处于夏令时

	tm_time.tm_hour		= systemtime.wHour;
	tm_time.tm_min		= systemtime.wMinute;
	tm_time.tm_sec		= systemtime.wSecond;

	time = (int)mktime(&tm_time);
#else
	timeval tv;
	//timezone *tz;
	if (0 == gettimeofday(&tv, NULL))
	{
		time = tv.tv_sec;
	}
#endif

	return time;
}

TVT_DATE_TIME GetCurrTime()
{
	TVT_DATE_TIME time;

#ifdef WIN32
	SYSTEMTIME systemtime;
	GetLocalTime(&systemtime);

	tm tm_time;
	memset(&tm_time, 0, sizeof(tm));

	tm_time.tm_year		= systemtime.wYear - 1900;
	tm_time.tm_mon		= systemtime.wMonth - 1;
	tm_time.tm_mday		= systemtime.wDay;

	tm_time.tm_hour		= systemtime.wHour;
	tm_time.tm_min		= systemtime.wMinute;
	tm_time.tm_sec		= systemtime.wSecond;

	time				= tm_time;
	time.microsecond	= systemtime.wMilliseconds*1000;
#else
	timeval tv;
	//timezone *tz;
	if (0 == gettimeofday(&tv, NULL))
	{
		time = tv;
	}
#endif

	return time;
}

TVT_LOCAL_TIME GetLocalTime()
{
	TVT_LOCAL_TIME time;

#ifdef WIN32
	SYSTEMTIME systemtime;
	GetLocalTime(&systemtime);

	time.year		= systemtime.wYear;
	time.month		= systemtime.wMonth;
	time.mday		= systemtime.wDay;
	
	time.hour		= systemtime.wHour;
	time.minute		= systemtime.wMinute;
	time.second		= systemtime.wSecond;

	time.microsecond= systemtime.wMilliseconds*1000;
#else
	timeval tv;
	//timezone *tz;
	if (0 == gettimeofday(&tv, NULL))
	{
		tm tm_time;
		memset(&tm_time, 0, sizeof(tm));

		time_t SecNum = tv.tv_sec;

		if (NULL == localtime_r(&SecNum, &tm_time))
		{
			time.year	= 2012;
			time.month	= 1;
			time.mday	= 1;
			time.hour	= 0;
			time.minute	= 0;
			time.second	= 0;
			time.microsecond	= 0;

			assert(false);
		}
		else
		{
			time.year	= tm_time.tm_year + 1900;
			time.month	= tm_time.tm_mon + 1;
			time.mday	= tm_time.tm_mday;
			time.hour	= tm_time.tm_hour;
			time.minute	= tm_time.tm_min;
			time.second	= tm_time.tm_sec;
			time.microsecond	= tv.tv_usec;
		}

		return time;
	}
#endif

	return time;
}

//输入参数：start_routine 线程的执行函数
//			pParam  线程执行时传入的参数
//			pRun    线程是否执行的bool量的指针，如果为NULL，则不理会此参数
//return value: 成功：返回线程ID    *pRun = true
//				失败：返回PUB_CREATE_THREAD_FAIL  *pRun的值不变
PUB_thread_t PUB_CreateThread(PUB_start_routine start_routine, void* pParam, bool *pRun)
{	
	PUB_thread_t threadID;
	if (NULL != pRun) 
	{
		*pRun = true;
	}
#ifdef WIN32
	threadID = CreateThread(0, 0, start_routine, (void *)pParam, 0, NULL);
	if ( (PUB_CREATE_THREAD_FAIL == threadID) && (NULL != pRun) ) {
		*pRun = false;
	}
#else
	int iRet = 0;
	if((iRet = pthread_create(&threadID, 0, start_routine, pParam)) != 0){
		threadID = PUB_CREATE_THREAD_FAIL;
		if (NULL != pRun)
		{
			*pRun = false;
		}		
	}
	errno = iRet;	//线程创建不会设置错误码，而是直接返回错误码
#endif
	return threadID;
}

//输入参数：start_routine 线程的执行函数
//			pParam  线程执行时传入的参数
//			pRun    线程是否执行的bool量的指针，如果为NULL，则不理会此参数
//			policy  线程调度优先级
//			return value: 成功：返回线程ID    *pRun = true
//				失败：返回PUB_CREATE_THREAD_FAIL  *pRun的值不变
PUB_thread_t PUB_CreateThreadEx(PUB_start_routine start_routine, void* pParam, bool *pRun, int policy)
{	
	PUB_thread_t threadID;
	if (NULL != pRun) 
	{
		*pRun = true;
	}
#ifdef WIN32
	threadID = CreateThread(0, 0, start_routine, (void *)pParam, 0, NULL);
	if ( (PUB_CREATE_THREAD_FAIL == threadID) && (NULL != pRun) ) {
		*pRun = false;
	}
#else
	int iRet = 0;
	pthread_attr_t thread_attr;
	struct sched_param thread_param;

	pthread_attr_init(&thread_attr);
	pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
	thread_param.sched_priority = policy;
	pthread_attr_setschedparam(&thread_attr, &thread_param);

	iRet = pthread_create(&threadID, &thread_attr, start_routine, pParam);
	if (0 != iRet)
	{
		threadID = PUB_CREATE_THREAD_FAIL;
		if (NULL != pRun)
		{
			*pRun = false;
		}		
	}

	errno = iRet;	//线程创建不会设置错误码，而是直接返回错误码
#endif
	return threadID;
}
//ThreadID 线程ID
//pRun     线程是否执行的bool量的指针
//return value: 退出线程后 *pThreadID = PUB_THREAD_ID_NOINIT，*pRun = false
void PUB_ExitThread(PUB_thread_t *pThreadID, bool *pRun)
{
	assert(NULL != pThreadID);
	if (PUB_THREAD_ID_NOINIT == *pThreadID)
	{
		return;
	}

	if (NULL != pRun) 
	{
		*pRun = false;
	}	
#ifdef WIN32
	WaitForSingleObject(*pThreadID, INFINITE);
	CloseHandle(*pThreadID);
#else
	if(pthread_self() == *pThreadID)
		CThreadManager::ExitThread(*pThreadID);
	else
		pthread_join(*pThreadID, NULL);
#endif
	*pThreadID = PUB_THREAD_ID_NOINIT;
}
//以为毫秒为单位的睡眠，
//BUGS：
//这里忽略了liunx下usleep是会返回非0的，并且没有了linux下不能睡眠微秒了
//linux下:SUSv2 version usleep 被信号中断，或者设置休息的时间大于一秒则会返回-1
//		  BSD version   usleep 无返回值
//		  dwMillisecond超过6分钟左右好像有问题 估计unsigned int不支持这么大的数字
void PUB_Sleep(unsigned int dwMillisecond)
{
#ifdef WIN32
	if (0 == dwMillisecond) {
		Sleep(10);
	}
	else{
		Sleep(dwMillisecond);
	}

#else
	int iSec = dwMillisecond / 1000;
	int	iMicSec = (dwMillisecond % 1000) * 1000;

	//大于一秒的时间用这个睡眠
	if (iSec > 0) 
	{
		do 
		{
			iSec = sleep(iSec);
		} while(iSec > 0); 
	}

	if(0 != usleep(iMicSec))
	{
		if (EINTR == errno) 
		{
			printf("the usleep Interrupted by a signal. pid = %d\n", getpid());
		}
		else if (EINVAL == errno) 
		{
			assert(false);
			printf("the usleep param is not smaller than 1000000");
		}
	}
	//	usleep(dwMillisecond*1000);
#endif
}

//打印致命错误
void PUB_PrintError(const char* pFile, int iLine)
{
	char szErrorSource[200] = {0};
	sprintf(szErrorSource, "%s %d ", pFile, iLine);
	perror(szErrorSource);
}

//初始化锁，初始化后是可嵌套的锁
void PUB_InitLock(PUB_lock_t *pLock)
{
	assert(NULL != pLock);
#ifdef WIN32
	InitializeCriticalSection(pLock);
#else
	pthread_mutexattr_t mutexAttr;
	pthread_mutexattr_init(&mutexAttr);
	pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(pLock, &mutexAttr);
	pthread_mutexattr_destroy(&mutexAttr);
#endif
}

//销毁锁
void PUB_DestroyLock(PUB_lock_t *pLock)
{
	assert(NULL != pLock);
#ifdef WIN32
	DeleteCriticalSection(pLock);
#else
	pthread_mutex_destroy(pLock);
#endif
}

CPUB_LockAction::CPUB_LockAction(PUB_lock_t* pLock)
{
	assert(NULL != pLock);
	m_pLock = pLock;
#ifdef WIN32
	EnterCriticalSection(pLock);
#else
#ifdef __DEBUG_LOCK__
	//这种锁无法使用宏替换的方式传入死锁代码位置
	int ret = 0;
	unsigned long count = 0;

	do
	{
		ret = pthread_mutex_trylock(pLock);
		count++;

		if(count > 1000)
		{
			//出现死锁
			printf("**** LockAction time out!pthread=%d,(%s,%d)\n", ret, __FILE__, __LINE__);

			count = 0;
		}

		if(0 != ret)
		{
			//没有获取锁
			usleep(100);
		}
	}while(0 != ret);
#else
	pthread_mutex_lock(pLock);
#endif
#endif
}

CPUB_LockAction::~CPUB_LockAction()
{
#ifdef WIN32
	LeaveCriticalSection(m_pLock);
#else
	pthread_mutex_unlock(m_pLock);
#endif
}

CPUB_Lock::CPUB_Lock()
{
#ifdef WIN32
	InitializeCriticalSection(&m_Lock);
#else
	pthread_mutexattr_t mutexAttr;
	pthread_mutexattr_init(&mutexAttr);
	pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_Lock, &mutexAttr);
	pthread_mutexattr_destroy(&mutexAttr);
	pthread_cond_init(&m_cond, NULL);
#endif

#ifdef __DEBUG_LOCK__
	memset(m_szLockFile, 0, sizeof(m_szLockFile));
#endif
}

CPUB_Lock::~CPUB_Lock()
{
#ifdef WIN32
	DeleteCriticalSection(&m_Lock);
#else
	pthread_cond_destroy(&m_cond);
	pthread_mutex_destroy(&m_Lock);
#endif
}

#undef Lock

void CPUB_Lock::Lock()
{
#ifdef WIN32
	EnterCriticalSection(&m_Lock);
#else
	pthread_mutex_lock(&m_Lock);
#endif
}

#ifdef __DEBUG_LOCK__
void CPUB_Lock::TestLock(unsigned long line, const char * pFileName)
{
#ifdef WIN32
	EnterCriticalSection(&m_Lock);
#else
	int ret = 0;
	unsigned long count = 0;

	do
	{
		ret = pthread_mutex_trylock(&m_Lock);
		count++;

		if(count > 1000)
		{
			//出现死锁
			printf("**** Lock time out!result=%d,pid=%d,(%s,%d), locked:pid=%d,(%s,%d)\n", ret, syscall(__NR_gettid), pFileName, line, m_lockThread, m_szLockFile, m_iLockLine);

			count = 0;
		}

		if(0 != ret)
		{
			//没有获取锁
			usleep(1000);
		}
	}while(0 != ret);

	strncpy(m_szLockFile, pFileName, 127);
	m_iLockLine = line;
	m_lockThread = syscall(__NR_gettid);
#endif
}
#endif

void CPUB_Lock::UnLock()
{
#ifdef WIN32
	LeaveCriticalSection(&m_Lock);
#else
	pthread_mutex_unlock(&m_Lock);
#endif
}

#ifndef	WIN32
void CPUB_Lock::CondWait()
{
	pthread_cond_wait(&m_cond, &m_Lock);
}

void CPUB_Lock::CondSignal()
{
	pthread_cond_signal(&m_cond);
}

//return value: 0 成功 -1 加锁失败
int CPUB_Lock::TryLock()
{
	if(0 == pthread_mutex_trylock(&m_Lock))
	{
		return 0;
	}
	return -1;
}

//return value:	0 成功得到锁 1 超时 2 其他错误
//BUGS:这个在uclibc上好像没有用，或者是自己就写了一个bug
int CPUB_Lock::TimeLock(int iMicrosecond)
{
	struct timeval     timeNow;
	struct timespec    Timeout;
	gettimeofday(&timeNow, NULL);
	Timeout.tv_sec = timeNow.tv_sec;
	Timeout.tv_nsec = (timeNow.tv_usec + iMicrosecond) * 1000;              
	int iRet = pthread_mutex_timedlock(&m_Lock, &Timeout);
	if (0 == iRet) 
	{
		return 0;
	}
	else if (-1 == iRet) 
	{
		if (ETIMEDOUT == errno) 
		{
			return 1;
		}
		else
		{
			assert(false);
			return 2;
		}
	}
	return 0;
}

CThreadManager g_ThreadManager;
CThreadManager::CThreadManager()
{
	m_ThreadManagerExit=false;
	m_ThreadManagerThread= PUB_CreateThread(ThreadManagerProc, this, NULL);
}

CThreadManager::~CThreadManager()
{
	m_ThreadManagerExit=true;
	//	PUB_ExitThread(&m_ThreadManagerThread, NULL);
}

void CThreadManager::ExitThread(unsigned int handle)
{
	g_ThreadManager.m_ThreadListLock.Lock();
	g_ThreadManager.m_ThreadList.push_back(handle);
	g_ThreadManager.m_ThreadListLock.UnLock();
}

PUB_THREAD_RESULT PUB_THREAD_CALL CThreadManager::ThreadManagerProc(void * lpParameter)
{
	CThreadManager *pManager=(CThreadManager*)lpParameter;
	pManager->ThreadManagerFunc();
	return 0;
}

void CThreadManager::ThreadManagerFunc()
{
	PUB_thread_t handle=0;
	while(!m_ThreadManagerExit)
	{
		handle=0;

		m_ThreadListLock.Lock();
		if(m_ThreadList.size() > 0)
		{
			handle = m_ThreadList.front();
			m_ThreadList.pop_front();
		}
		m_ThreadListLock.UnLock();

		if(handle)
			PUB_ExitThread(&handle, NULL);
		PUB_Sleep(100);
	}
}

#endif

/************************************************************************/
/* CRC32 校验                                                           */
/************************************************************************/

const unsigned int g_tvt_crc32_table[] = {
#ifdef DVR_BIG_ENDIAN
	0x00000000L, 0x96300777L, 0x2c610eeeL, 0xba510999L, 0x19c46d07L,
	0x8ff46a70L, 0x35a563e9L, 0xa395649eL, 0x3288db0eL, 0xa4b8dc79L,
	0x1ee9d5e0L, 0x88d9d297L, 0x2b4cb609L, 0xbd7cb17eL, 0x072db8e7L,
	0x911dbf90L, 0x6410b71dL, 0xf220b06aL, 0x4871b9f3L, 0xde41be84L,
	0x7dd4da1aL, 0xebe4dd6dL, 0x51b5d4f4L, 0xc785d383L, 0x56986c13L,
	0xc0a86b64L, 0x7af962fdL, 0xecc9658aL, 0x4f5c0114L, 0xd96c0663L,
	0x633d0ffaL, 0xf50d088dL, 0xc8206e3bL, 0x5e10694cL, 0xe44160d5L,
	0x727167a2L, 0xd1e4033cL, 0x47d4044bL, 0xfd850dd2L, 0x6bb50aa5L,
	0xfaa8b535L, 0x6c98b242L, 0xd6c9bbdbL, 0x40f9bcacL, 0xe36cd832L,
	0x755cdf45L, 0xcf0dd6dcL, 0x593dd1abL, 0xac30d926L, 0x3a00de51L,
	0x8051d7c8L, 0x1661d0bfL, 0xb5f4b421L, 0x23c4b356L, 0x9995bacfL,
	0x0fa5bdb8L, 0x9eb80228L, 0x0888055fL, 0xb2d90cc6L, 0x24e90bb1L,
	0x877c6f2fL, 0x114c6858L, 0xab1d61c1L, 0x3d2d66b6L, 0x9041dc76L,
	0x0671db01L, 0xbc20d298L, 0x2a10d5efL, 0x8985b171L, 0x1fb5b606L,
	0xa5e4bf9fL, 0x33d4b8e8L, 0xa2c90778L, 0x34f9000fL, 0x8ea80996L,
	0x18980ee1L, 0xbb0d6a7fL, 0x2d3d6d08L, 0x976c6491L, 0x015c63e6L,
	0xf4516b6bL, 0x62616c1cL, 0xd8306585L, 0x4e0062f2L, 0xed95066cL,
	0x7ba5011bL, 0xc1f40882L, 0x57c40ff5L, 0xc6d9b065L, 0x50e9b712L,
	0xeab8be8bL, 0x7c88b9fcL, 0xdf1ddd62L, 0x492dda15L, 0xf37cd38cL,
	0x654cd4fbL, 0x5861b24dL, 0xce51b53aL, 0x7400bca3L, 0xe230bbd4L,
	0x41a5df4aL, 0xd795d83dL, 0x6dc4d1a4L, 0xfbf4d6d3L, 0x6ae96943L,
	0xfcd96e34L, 0x468867adL, 0xd0b860daL, 0x732d0444L, 0xe51d0333L,
	0x5f4c0aaaL, 0xc97c0dddL, 0x3c710550L, 0xaa410227L, 0x10100bbeL,
	0x86200cc9L, 0x25b56857L, 0xb3856f20L, 0x09d466b9L, 0x9fe461ceL,
	0x0ef9de5eL, 0x98c9d929L, 0x2298d0b0L, 0xb4a8d7c7L, 0x173db359L,
	0x810db42eL, 0x3b5cbdb7L, 0xad6cbac0L, 0x2083b8edL, 0xb6b3bf9aL,
	0x0ce2b603L, 0x9ad2b174L, 0x3947d5eaL, 0xaf77d29dL, 0x1526db04L,
	0x8316dc73L, 0x120b63e3L, 0x843b6494L, 0x3e6a6d0dL, 0xa85a6a7aL,
	0x0bcf0ee4L, 0x9dff0993L, 0x27ae000aL, 0xb19e077dL, 0x44930ff0L,
	0xd2a30887L, 0x68f2011eL, 0xfec20669L, 0x5d5762f7L, 0xcb676580L,
	0x71366c19L, 0xe7066b6eL, 0x761bd4feL, 0xe02bd389L, 0x5a7ada10L,
	0xcc4add67L, 0x6fdfb9f9L, 0xf9efbe8eL, 0x43beb717L, 0xd58eb060L,
	0xe8a3d6d6L, 0x7e93d1a1L, 0xc4c2d838L, 0x52f2df4fL, 0xf167bbd1L,
	0x6757bca6L, 0xdd06b53fL, 0x4b36b248L, 0xda2b0dd8L, 0x4c1b0aafL,
	0xf64a0336L, 0x607a0441L, 0xc3ef60dfL, 0x55df67a8L, 0xef8e6e31L,
	0x79be6946L, 0x8cb361cbL, 0x1a8366bcL, 0xa0d26f25L, 0x36e26852L,
	0x95770cccL, 0x03470bbbL, 0xb9160222L, 0x2f260555L, 0xbe3bbac5L,
	0x280bbdb2L, 0x925ab42bL, 0x046ab35cL, 0xa7ffd7c2L, 0x31cfd0b5L,
	0x8b9ed92cL, 0x1daede5bL, 0xb0c2649bL, 0x26f263ecL, 0x9ca36a75L,
	0x0a936d02L, 0xa906099cL, 0x3f360eebL, 0x85670772L, 0x13570005L,
	0x824abf95L, 0x147ab8e2L, 0xae2bb17bL, 0x381bb60cL, 0x9b8ed292L,
	0x0dbed5e5L, 0xb7efdc7cL, 0x21dfdb0bL, 0xd4d2d386L, 0x42e2d4f1L,
	0xf8b3dd68L, 0x6e83da1fL, 0xcd16be81L, 0x5b26b9f6L, 0xe177b06fL,
	0x7747b718L, 0xe65a0888L, 0x706a0fffL, 0xca3b0666L, 0x5c0b0111L,
	0xff9e658fL, 0x69ae62f8L, 0xd3ff6b61L, 0x45cf6c16L, 0x78e20aa0L,
	0xeed20dd7L, 0x5483044eL, 0xc2b30339L, 0x612667a7L, 0xf71660d0L,
	0x4d476949L, 0xdb776e3eL, 0x4a6ad1aeL, 0xdc5ad6d9L, 0x660bdf40L,
	0xf03bd837L, 0x53aebca9L, 0xc59ebbdeL, 0x7fcfb247L, 0xe9ffb530L,
	0x1cf2bdbdL, 0x8ac2bacaL, 0x3093b353L, 0xa6a3b424L, 0x0536d0baL,
	0x9306d7cdL, 0x2957de54L, 0xbf67d923L, 0x2e7a66b3L, 0xb84a61c4L,
	0x021b685dL, 0x942b6f2aL, 0x37be0bb4L, 0xa18e0cc3L, 0x1bdf055aL,
	0x8def022dL
#else
	0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
	0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
	0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
	0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
	0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
	0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
	0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
	0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
	0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
	0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
	0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
	0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
	0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
	0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
	0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
	0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
	0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
	0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
	0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
	0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
	0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
	0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
	0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
	0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
	0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
	0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
	0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
	0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
	0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
	0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
	0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
	0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
	0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
	0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
	0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
	0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
	0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
	0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
	0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
	0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
	0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
	0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
	0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
	0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
	0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
	0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
	0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
	0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
	0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
	0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
	0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
	0x2d02ef8dL
#endif
};

#define TVT_CRC32_CHECK(CH, CRC) (g_tvt_crc32_table[((CRC) ^ (CH)) & 0xff] ^ ((CRC) >> 8))

unsigned int Crc32(const unsigned char* buff, unsigned int buff_len, unsigned int seed)
{
	/*
	CRC32 变种, 只能用于检验数据完整性
	此算法只适用于校验数据量小, 精度要求高
	buff		: 待检验的数据
	buff_len	: 待检验的数据长度
	seed		: 种子, [一般为时间 或默认0]
	返回		: 校验值
	*/
	unsigned int crc = ~static_cast<unsigned int>(0);

	for(unsigned int i = 0; i < buff_len; i++)
	{
		unsigned char ch = buff[i];
		crc = TVT_CRC32_CHECK(ch, crc);
	}

	/* 异或seed, 是为了降低得到一致CRC32值的概率 */
	return ((~crc) ^ seed);
}

//Get Host By Name

//获取DNS对应的ip地址
struct hostent * GetHostbyname(const char *pDNS)
{
#ifdef WIN32
	return GetHostbyname(pDNS, "8.8.8.8");
#else
	//获得DNS服务器的IP地址
	FILE * pFile = fopen(PATH_DNS, "r");
	if (NULL == pFile)
	{
		printf("%s:%s:%d, Open file %s error:%s\n", __FUNCTION__, __FILE__, __LINE__, PATH_DNS, strerror(errno));
		return NULL;
	}

	if (0 != fseek(pFile, 0, SEEK_END))
	{
		printf("%s:%s:%d, Seek file %s error:%s\n", __FUNCTION__, __FILE__, __LINE__, PATH_DNS, strerror(errno));
		fclose(pFile);
		return NULL;
	}

	long long fileLen = ftell(pFile);
	if (-1 == fileLen)
	{
		printf("%s:%s:%d, file %s length error:%s\n", __FUNCTION__, __FILE__, __LINE__, PATH_DNS, strerror(errno));
		fclose(pFile);
		return NULL;
	}

	char * pBuf = new char [fileLen + 10];
	if (NULL == pBuf)
	{
		fclose(pFile);
		assert(false);
		return NULL;
	}
	else
	{
		memset(pBuf, 0, fileLen + 10);
	}

	if (0 != fseek(pFile, 0, SEEK_SET))
	{
		printf("%s:%s:%d, Seek file %s error:%s\n", __FUNCTION__, __FILE__, __LINE__, PATH_DNS, strerror(errno));
		fclose(pFile);
		return NULL;
	}

	if (0 >= fread(pBuf, 1, fileLen, pFile))
	{
		printf("%s:%s:%d, read file %s error:%s\n", __FUNCTION__, __FILE__, __LINE__, PATH_DNS, strerror(errno));
		fclose(pFile);
		pFile = NULL;

		delete [] pBuf;
		pBuf = NULL;

		assert(false);
		return NULL;
	}

	fclose(pFile);
	pFile = NULL;

	char * pTemp = pBuf;
	hostent * pHost = NULL;

	while (NULL != pTemp)
	{
		char * p = strstr(pTemp, "nameserver");
		if ( NULL != p)
		{
			char dnsServerIP[40] = {0};
			sscanf(p,"nameserver %s",dnsServerIP);
			//printf("%s:%s:%d, dnsserverip=%s\n", __FUNCTION__, __FILE__, __LINE__, dnsServerIP);
			pHost = GetHostbyname(pDNS, dnsServerIP);
			if (NULL != pHost)
			{
				break;
			}

			pTemp = p + strlen("nameserver");
		}
		else
		{
			break;
		}
	}

	delete [] pBuf;
	pBuf = NULL;

	return pHost;
#endif
}

#ifdef WIN32
/**********************************************
*   FUNC：  判断一个IP地址是否合法
*   INPUT:    IP地址字符串
***********************************************/
bool PUB_AssertIPAddr(const char *IpAddr)
{
	assert(IpAddr != NULL);

	int Value[4] = {-1, -1, -1, -1};
	int ret = sscanf(IpAddr, "%d.%d.%d.%d", &Value[0], &Value[1], &Value[2], &Value[3]);
	if(ret != 4)
		return false;

	int i = 0;
	for(i=0; i<4; i++)
	{
		if(Value[i]>=0 && Value[i]<=255)
		{
			continue;
		}
		else
			return false;
	}
	return true;
}
#endif

struct hostent * GetHostbyname(const char * pDNS, char * pDnsServer)
{
	struct hostent * pHost = new hostent;
	if (NULL == pHost)
	{
		printf("%s:%s:%d, New error\n", __FUNCTION__, __FILE__, __LINE__);
		assert(false);
		return NULL;
	}
	else
	{
		memset(pHost, 0, sizeof(struct hostent));
		pHost->h_addr_list = new char*[4];
		assert(NULL != pHost->h_addr_list);
		for(int index1 = 0; index1 < 4; ++index1)
		{
			pHost->h_addr_list[index1] = new char[40];
			assert(NULL != pHost->h_addr_list[index1]);
			memset(pHost->h_addr_list[index1], 0, 40);
		}
	}

#ifdef WIN32
	if(PUB_AssertIPAddr(pDNS))
	{
		char addr_list[10] = {0};
		memset(addr_list, 0, sizeof(addr_list));
		sscanf(pDNS, "%d.%d.%d.%d", &addr_list[0], &addr_list[1], &addr_list[2], &addr_list[3]);
		memcpy(&pHost->h_addr_list[0][3],addr_list+3,1);
		memcpy(&pHost->h_addr_list[0][2],addr_list+2,1);
		memcpy(&pHost->h_addr_list[0][1],addr_list+1,1);
		memcpy(&pHost->h_addr_list[0][0],addr_list,1);
		pHost->h_addrtype = AF_INET;
		memset(addr_list, 0, sizeof(addr_list));
		return pHost;
	}
#else
	if (inet_aton(pDNS, (struct in_addr*)pHost->h_addr))
	{
		//请求的域名本身就是IP地址
		return pHost;
	}
#endif
	//解析域名为DNS请求的格式
	size_t lengthParseName = strlen(pDNS) + 2;		//解析后字符串长度
	char * pParseName = new char[lengthParseName];	//用于存储解析后的域名
	assert(NULL != pParseName);
	memset(pParseName, 0, lengthParseName);
	strcpy(pParseName, pDNS);
	if (!ParseDomainName(pParseName, lengthParseName)) 
	{
		delete [] pParseName;
		pParseName = NULL;
		ReleaseHost(pHost);

		return NULL;
	}

	//建构DNS报文头部信息	 格式为[标示(2字节)][标志(2字节)][问题数(2字节)][应答资源数(2字节)][授权资源数][额外资源数]{查询问题}{应答}{授权}{额外}
	size_t lengthDatagram = 12 + lengthParseName + 4;
	char * pDadagram = new char[lengthDatagram];
	assert(NULL != pDadagram);
	memcpy(pDadagram, DNS_REQ_DATA_HEAD,12);							//考贝协议头
	memcpy(pDadagram + 12, pParseName, lengthParseName);				//考贝要解析的域名
	memcpy(pDadagram + 12 + lengthParseName, DNS_REQ_DATA_TAIL, 4);		//考贝协议尾
	delete [] pParseName;
	pParseName = NULL;

	//获得DNS服务器的IP地址
	struct sockaddr_in their_addr;
	memset(&their_addr, 0, sizeof(their_addr));
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(DNS_PORT);
	their_addr.sin_addr.s_addr = inet_addr(pDnsServer);
	if (INADDR_NONE == their_addr.sin_addr.s_addr)
	{
		delete [] pDadagram;
		pDadagram = NULL;

		ReleaseHost(pHost);

		return NULL;
	}

	//创建socket和DNS服务器通信
#ifdef WIN32
	SOCKET	sockfd = -1;
#else
	int sockfd = -1;
#endif
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) 
	{
		printf("(%s %d) Failed: open socket failed,%s!\n",__FILE__,__LINE__,strerror(errno));
		delete [] pDadagram;
		pDadagram = NULL;
		ReleaseHost(pHost);

		return NULL;
	}

	timeval timeout;
	timeout.tv_sec	= 5;	//超时时间为5s
	timeout.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout))) 
	{
#ifdef WIN32
		closesocket(sockfd);
#else
		close(sockfd);
#endif
		printf("(%s %d) Failed: setsockopt failed,%s\n",__FILE__,__LINE__,strerror(errno));
		delete [] pDadagram;
		pDadagram = NULL;
		ReleaseHost(pHost);
		return NULL;
	}

	//发送DNS数据并接收服务器回应
	char revBuffer[1024] = {0};
	int  revLength = 0;

	bool ret =  false;
	//进行5次通信
	for (int timer = 0; !ret && timer < 3; timer ++) 
	{
		revLength = sendto(sockfd, (const char*)pDadagram, lengthDatagram, 0, (struct sockaddr *)&their_addr, sizeof(struct sockaddr));
		if (-1 == revLength)
		{
			printf("%s, %s %d Failed: send DNS data failed, timer=%d, %s!\n",__FUNCTION__, __FILE__,__LINE__,timer, strerror(errno));
			continue;
		}

		memset(revBuffer, 0, sizeof(revBuffer));
		socklen_t addr_len = sizeof(their_addr);

		revLength = recvfrom(sockfd, revBuffer, sizeof(revBuffer), 0, (sockaddr*)&their_addr, &addr_len);	
		if(12 < revLength)
		{
			//比较DNS id
			if(0 == memcmp(revBuffer, DNS_REQ_DATA_HEAD, 2)) 
			{
				//在回应报文中可能有多个返回地址, 得到其个数
				short temp = 0;
				memcpy(&temp, revBuffer + 6, sizeof(short));
				short nameNum = ntohs(temp);

				char * pAnser = revBuffer;
				pAnser = pAnser+ 12 + lengthParseName + 4;	//12为DNS协议的头长度//4为query字段的:type, class ,各为一个short类型
				int useLen = 12 + lengthParseName + 4;
				int index = 0;
				while( (nameNum > 0) && (useLen + 12 <= revLength) )
				{
					//得到其后内容的类型
					memcpy(&temp, pAnser + 2, sizeof(short));
					short nameType = ntohs(temp);
					memcpy(&temp, pAnser + 10, sizeof(short));
					short nameLength = ntohs(temp);

					//如果是IP地址, 返回
					if( (1==nameType) && (useLen + 16 <= revLength) )
					{
						memcpy(&pHost->h_addr_list[index][3],(pAnser+15),1);
						memcpy(&pHost->h_addr_list[index][2],(pAnser+14),1);
						memcpy(&pHost->h_addr_list[index][1],(pAnser+13),1);
						memcpy(&pHost->h_addr_list[index][0],(pAnser+12),1);
						pHost->h_addrtype = AF_INET;
						index ++;
						ret  = true;
						if (2 == index)
						{
							break;
						}
					}
					pAnser = pAnser + 12 + nameLength;
					useLen += 12 + nameLength;
					nameNum --;
				}			
			}
		}
	}

#ifdef WIN32
	closesocket(sockfd);
#else
	close(sockfd);
#endif
	delete [] pDadagram;
	pDadagram  = NULL;

	if (ret) 
	{		
		return pHost;
	}
	else
	{
		ReleaseHost(pHost);

		return NULL;
	}
}

void ReleaseHost(struct hostent *pHost)
{
	if (NULL != pHost)
	{
		delete[] pHost->h_addr_list[0];
		delete[] pHost->h_addr_list[1];
		delete[] pHost->h_addr_list[2];
		delete[] pHost->h_addr_list[3];
		delete[] pHost->h_addr_list;
		delete pHost;
	}
}

bool ParseDomainName(char * pDNS, size_t &length)
{
	if (length < strlen(pDNS) + 2)
	{
		printf("%s:%s:%d, the length is too short\n", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	char *pDNSTemp = new char [length];
	if (NULL == pDNSTemp)
	{
		printf("%s:%s:%d, New error\n", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}
	else
	{
		memcpy(pDNSTemp, pDNS, length);
		memset(pDNS, 0, length);
		strcpy(pDNS+1, pDNSTemp);
		delete [] pDNSTemp;
		pDNSTemp = NULL;
	}

	char * pCurrent	= pDNS+1;
	char * pMark	= pDNS;

	*pMark = 0;
	while ('\0' != *pCurrent) 
	{
		//只要当前字符不为字符串的结束标志
		//如果当前字符是 .
		if ('.' == *pCurrent)
		{
			//如果连着两个都是 . ,则表示输入格式错, 返回错误
			if ('.' == *(pCurrent-1)) 
			{
				printf("%s:%s:%d, The dns %s is invalid\n", __FUNCTION__, __FILE__, __LINE__, pDNS);
				assert(false);
				return false;
			}
			pMark = pCurrent;
			*pMark = 0;
		}
		else
		{
			(*pMark)++;	//计数
		}
		pCurrent++;		//指针后移
	}

	length = strlen(pDNS) + 1;

	return true;
}


/**************************************************************/

const unsigned int MAX_UNSIGNED_32BIT_NUM = 0x80000000 + 0x7fffffff;
unsigned int PUB_GetTickCount()
{
#ifdef WIN32
	return GetTickCount();
#else
	struct timespec tp;
	if(!clock_gettime(CLOCK_MONOTONIC, &tp) < 0){assert(false);}
	return tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
#endif
}

//是否从dwStartTime毫秒开始计算，过了dwDisTime毫秒
//BUGS: 超了两圈以上就没办法了
bool PUB_IsTimeOut(unsigned long dwStartTime, unsigned long dwDisTime, unsigned long *pCurTime)
{
	unsigned int  dwNowTime = PUB_GetTickCount();
	if (NULL != pCurTime)
	{
		*pCurTime = dwNowTime;
	}

	if (dwNowTime >= dwStartTime)
	{
		if((dwNowTime - dwStartTime) >= dwDisTime) 
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		if(((MAX_UNSIGNED_32BIT_NUM - dwStartTime) + 1 + dwNowTime) >= dwDisTime)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}

//返回编译时间
TVT_LOCAL_TIME GetBuildDate()
{
	const char *pDate = __DATE__;
	const char *pTime = __TIME__;

	TVT_LOCAL_TIME time;
	memset(&time, 0, sizeof(TVT_LOCAL_TIME));

	//year
	time.year = atoi(pDate+7);

	//mon
	if(strncmp(pDate,"Jan",3)==0)
	{
		time.month = 1;
	}
	if(strncmp(pDate,"Feb",3)==0)
	{
		time.month = 2;	
	}
	if(strncmp(pDate,"Mar",3)==0)
	{
		time.month = 3;	
	}
	if(strncmp(pDate,"Apr",3)==0)
	{
		time.month = 4;	
	}
	if(strncmp(pDate,"May",3)==0)
	{
		time.month = 5;	
	}
	if(strncmp(pDate,"Jun",3)==0)
	{
		time.month = 6;		
	}
	if(strncmp(pDate,"Jul",3)==0)
	{
		time.month = 7;	
	}
	if(strncmp(pDate,"Aug",3)==0)
	{
		time.month = 8;	
	}
	if(strncmp(pDate,"Sep",3)==0)
	{
		time.month = 9;	
	}
	if(strncmp(pDate,"Oct",3)==0)
	{
		time.month = 10;	
	}
	if(strncmp(pDate,"Nov",3)==0)
	{
		time.month = 11;	
	}
	if(strncmp(pDate,"Dec",3)==0)
	{
		time.month = 12;		
	}

	//mday
	time.mday  = atoi(pDate+4);

	//hour
	time.hour 	= atoi(pTime);

	//min
	time.minute	= atoi(pTime+3);

	//sec
	time.second	= atoi(pTime+6);

	return time;
}

unsigned int PUB_RecFrameCheck(TVT_DATA_FRAME * pFrameInfo)
{

	unsigned int  sum = 0;

	unsigned int  uxor = 0;
	unsigned int  *ptime32=(unsigned int *)(&(pFrameInfo->frame.time.seconds));

	sum+= pFrameInfo->length;
	uxor^= pFrameInfo->length;

	sum  += pFrameInfo->frame.streamType;
	uxor ^= pFrameInfo->frame.streamType;
	
	sum+= pFrameInfo->frame.length;
	uxor^= pFrameInfo->frame.length;

	sum+= pFrameInfo->frame.width;
	uxor^= pFrameInfo->frame.width;

	sum+= pFrameInfo->frame.height;
	uxor^= pFrameInfo->frame.height;

	sum+= pFrameInfo->frame.channel;
	uxor^= pFrameInfo->frame.channel;

	sum+= ptime32[0];
	uxor^= ptime32[0];

	sum+= ptime32[1];
	uxor^= ptime32[1];

	sum= sum ^uxor;
	return sum;	
}

int PUB_GetDiskNoByFilePath(const char *pPath)
{
	int diskNo = -1;
	char pLeaveStr[50] = "";
	int diskPos = strlen(pPath) - strlen("00/data00/00000000.dat");
	const char *p = pPath+diskPos;
	int ret = sscanf(p, "%d%s", &diskNo, pLeaveStr);
	if ((2 != ret) || (strlen(pLeaveStr) != strlen("/data00/00000000.dat")))
	{
		printf("[PUB_GetDiskNoByFilePath]:Can not get diskNo by file:%s\n", pPath);
		assert(false);
		return -1;
	}

	return diskNo;
}

CWaitEvent::CWaitEvent()
{

}

CWaitEvent::~CWaitEvent()
{

}

void CWaitEvent::CreateWaitEvent()
{
#ifdef WIN32
	m_Event = CreateEvent(NULL, true, false, NULL);  
#else
	pthread_mutex_init(&m_lock, NULL);
	pthread_cond_init(&m_Event, NULL);
#endif
}

unsigned int CWaitEvent::WaitForCond(unsigned int dwMilliseconds)
{
#ifdef WIN32
	unsigned int ret = WaitForSingleObject(m_Event, dwMilliseconds);
	ResetEvent(m_Event);

	if (ret == WAIT_OBJECT_0)
	{
		return 0;
	}
	else
	{
		return 1;
	}
#else
	unsigned int ret = 0;
	if(dwMilliseconds == INFINITE)
	{
		pthread_mutex_lock(&m_lock);
		ret = pthread_cond_wait(&m_Event, &m_lock);
		pthread_mutex_unlock(&m_lock);
	}
	else
	{
		struct timespec ts;
		Gettimespec(&ts, dwMilliseconds);
		pthread_mutex_lock(&m_lock);
		ret = pthread_cond_timedwait(&m_Event, &m_lock, &ts);
		pthread_mutex_unlock(&m_lock);
	}

	return ret;
#endif	
}

void CWaitEvent::SetEvent()
{
#ifdef	WIN32 
	::SetEvent(m_Event);
#else
	pthread_cond_signal(&m_Event);
#endif
}

void CWaitEvent::PulseEvent()
{
#ifdef WIN32
	::PulseEvent(m_Event);  
#else
	pthread_cond_broadcast(&m_Event);
#endif
}

#ifndef WIN32
void CWaitEvent::Gettimespec(struct timespec* ptimespec, int offset) 
{ 
	struct timeval   now; 
	struct timespec  *ptime = (struct timespec *)ptimespec; 

	gettimeofday(&now, NULL); 
	ptime->tv_sec = now.tv_sec; 

	int tmp = now.tv_usec + offset * 1000; ////tmp是us级别的
	ptime->tv_sec = ptime->tv_sec + (tmp/(1000 * 1000));
	ptime->tv_nsec = (tmp % (1000 * 1000)) * 1000; 
}
#endif

void CWaitEvent::Close()
{
#ifdef  WIN32
	CloseHandle(m_Event);
	m_Event = NULL;
#else
	pthread_cond_destroy(&m_Event);
	pthread_mutex_destroy(&m_lock);
#endif
}

//end
