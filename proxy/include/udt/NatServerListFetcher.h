// NatServerListFetcher.h: interface for the CNatServerListFetcher class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __NAT_SERVER_LIST_FETCHER__
#define __NAT_SERVER_LIST_FETCHER__

#include "NatPublicTypes.h"
#include "tinyxml.h"
#include "TVTHttp.h"
#include <vector>

class CNatServerListFetcher
{
public:
	CNatServerListFetcher();
	virtual ~CNatServerListFetcher();
public:
    /**
     * 设置默认服务器地址，通常为域名
     */
    void SetDefaultServerAddr(const char* addr);
    /**
     * 设置服务器列表
     */
    void SetServerList(const NAT_SERVER_INFO* pServerList, int iCount);
    /**
     * 设置是否启用测速，默认为 false
     */
    void SetTestSpeed(bool bTestSpeed);

    /**
     * 开始执行获取过程
     * 内部会启动处理线程
     */
    bool Start();

    /**
     * 是否已完成
     * 初始状态为true；当执行Start()后，为false；在处理完成后，为true，此时内部线程会自动退出
     */
    bool IsCompleted();

    // 取消执行过程，并等待真正完成后返回
    void Cancel();
    
	int GetServerCount();
    /**
     * 获取服务器列表
     * 应满足 IsCompleted() 或 IsTestParted() 以后，才可以获取
     */
    bool GetServerInfo(NAT_SERVER_INFO *pServerInfo, int index);

    bool IsTestSpeed();

private:

    static PUB_THREAD_RESULT PUB_THREAD_CALL WorkThreadFunc(void *pParam);
    int RunWork();
private:
	typedef struct __SERVER_SPEED_INFO__
	{
		NAT_SERVER_INFO serverInfo;
		unsigned long remoteIp;
		unsigned long lastSendTime;
		int sendCount;
		bool hasReceived;
	}SERVER_SPEED_INFO;

	typedef std::vector<NAT_SERVER_INFO> NatServerVector;
	typedef std::list<SERVER_SPEED_INFO> NatServerList;
private:
	bool InitSpeedTestList(NatServerVector &serverVector);
	//下载服务器列表
	bool FetchListFromServ(NatServerVector &serverVector, const char *pServerAddr);
	//解析服务器列表
	bool ParseServList(NatServerVector &serverVector, const char* pXmlSrc, int nXmlType = 0);
	//解析服务器信息
	int ParseServInfo(TiXmlElement* pXmlDoc, NAT_SERVER_INFO* pServInfo);
	//清空
	void Clear();

	void ProcTestSpeed();
private:
	bool m_isCompleted;
	//结果服务器列表
	NatServerVector m_retServerVector;
	//临时服务器列表
	NatServerVector m_cacheServerVector;
	// 下载服务器列表
	NatServerVector m_downloadServerVector;
	// 测速列表
	NatServerList   m_speedTestList;
	CTVTHttp *m_pHttp;
	CPUB_Lock m_lock;
	char m_szDefaultServerAddr[256];
    bool m_isTestSpeed;

    PUB_thread_t m_workThreadID;
    bool m_workThreadRunning;

};
#endif//__NAT_SERVER_LIST_FETCHER__