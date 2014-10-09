// NatDeviceFetcher.cpp: implementation of the CNatDeviceFetcher class.
//
//////////////////////////////////////////////////////////////////////
#include "NatCommon.h"
#include "NatDeviceFetcher.h"
#include "NatUdtTransport.h"
#include "NatRunner.h"
#include "NatTraversalClient.h"

#ifdef	__ENVIRONMENT_LINUX__
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#include "NatDebug.h"


void NAT_ParseDeviceVer( const char* pVerStr, NAT_DEVICE_VER_INFO* pVerInfo )
{
    // set default value
    pVerInfo->verType = NAT_DEVICE_VER_3;
    if (strncmp(NAT_DEVICE_VER_PREFIX_3_0, pVerStr, strlen(NAT_DEVICE_VER_PREFIX_3_0)) == 0)
    {
        pVerInfo->verType = NAT_DEVICE_VER_3;
    } 
    else if (strncmp(NAT_DEVICE_VER_PREFIX_4_0, pVerStr, strlen(NAT_DEVICE_VER_PREFIX_4_0)) == 0)
    {
        pVerInfo->verType = NAT_DEVICE_VER_4;
    }
}

class CNatDeviceFetcherWorker: public CNatUdtTransportNotifier, CNatTraversalClient::CTraversalNotifier
{
public:
    CNatDeviceFetcherWorker(CNatDeviceFetcher &deviceFetcher);
    ~CNatDeviceFetcherWorker();
    bool Start();
    void Stop();
    bool IsStarted(); 
public:
    ////////////////////////////////////////////////////////////////////////////////
    // interface for CNatUdtTransportNotifier
    virtual void OnRecvDatagram(CNatUdtTransport *transport, const NAT_TRANSPORT_RECV_DATAGRAM* datagram);

public:
    ////////////////////////////////////////////////////////////////////////////////
    // interface for CNatTraversalClientUtil::CTraversalNotifier
        virtual void OnServerConnect();
        virtual void OnActiveCmd(NatTraversalCmdId cmdId, void* cmd) { /* ignore */};
        virtual void OnReply(NatTraversalCmdId reqId, const void* req, bool succeeded, 
            NatTraversalCmdId replyId, void* reply);
private:
    static PUB_THREAD_RESULT PUB_THREAD_CALL WorkThreadFunc(void *pParam);
    int RunWork();
    int InitTraversalClient();
    void NotifyReply(NAT_CLIENT_ERROR error, NAT_FETCH_DEVICE_REPLY *reply);
private:
    CNatDeviceFetcher &m_deviceFetcher;	
    bool m_workThreadRuning;  
    PUB_thread_t m_workThreadID;
    CNatUdtTransport m_udtTransport;
    unsigned long m_startTime;
    bool m_started;
    CNatTraversalClient m_traversalClient;
    bool m_isCompleted;
};

////////////////////////////////////////////////////////////////////////////////
// class CNatDeviceFetcherWorker implements

CNatDeviceFetcherWorker::CNatDeviceFetcherWorker(CNatDeviceFetcher &deviceFetcher)
:
m_deviceFetcher(deviceFetcher),
m_workThreadID(PUB_THREAD_ID_NOINIT),
m_workThreadRuning(false),
m_started(false),
m_isCompleted(false)
{

}

CNatDeviceFetcherWorker::~CNatDeviceFetcherWorker()
{
    if (PUB_THREAD_ID_NOINIT != m_workThreadID) 
    {
        PUB_ExitThread(&m_workThreadID, &m_workThreadRuning);
    }
}

bool CNatDeviceFetcherWorker::Start()
{

    NAT_UDT_TRANSPORT_CONFIG udtTransportConfig;
    udtTransportConfig.localIp = 0;
    udtTransportConfig.localPort = 0;

    if (m_udtTransport.Start(&udtTransportConfig) != 0)
    {
        return false;
    }
    m_udtTransport.SetNotifier(this);

    m_workThreadID = PUB_CreateThread(WorkThreadFunc, (void *)this, &m_workThreadRuning);	
    if (PUB_THREAD_ID_NOINIT == m_workThreadID) 
    {
        NAT_LOG_ERROR_SYS("CNatDeviceFetcherWorker create thread failed!\n");
        m_deviceFetcher.m_error = NAT_CLI_ERR_SYS;
        return false;
    }
    m_startTime = Nat_GetTickCount();
    m_isCompleted = false;
    m_started = true;
    return true;
}

void CNatDeviceFetcherWorker::Stop()
{
    if (PUB_THREAD_ID_NOINIT != m_workThreadID) 
    {
        PUB_ExitThread(&m_workThreadID, &m_workThreadRuning);
        m_workThreadID = PUB_THREAD_ID_NOINIT;
    }

    if (m_traversalClient.IsStarted())
    {
        m_traversalClient.Stop();
    }

    if (m_udtTransport.IsStarted())
    {
        m_udtTransport.Stop();
    }

    m_isCompleted = false;
    m_started = false;
}

bool CNatDeviceFetcherWorker::IsStarted()
{
    return m_started;
}

void CNatDeviceFetcherWorker::OnRecvDatagram(CNatUdtTransport *transport,
    const NAT_TRANSPORT_RECV_DATAGRAM* datagram)
{
    m_traversalClient.NotifyRecvDatagram(datagram);
    unsigned char category = CNatUdt::GetDatagramCategory(datagram);
}

void CNatDeviceFetcherWorker::OnServerConnect()
{
    assert(!m_traversalClient.IsRequesting());
    NAT_FETCH_DEVICE_REQ req;
    strcpy(req.dwDeviceNo, m_deviceFetcher.m_config.dwDeviceNo);
    req.stHeader.dwRequestSeq = m_traversalClient.GenNextReqSeq();
    m_traversalClient.StartRequest(NAT_ID_FETCH_DEVICE_REQ, &req, sizeof(req));
}

void CNatDeviceFetcherWorker::OnReply(NatTraversalCmdId reqId, const void* req, 
    bool succeeded, NatTraversalCmdId replyId, void* reply)
{
    if (succeeded)
    {
        assert(reqId == NAT_ID_FETCH_DEVICE_REQ && replyId == NAT_ID_FETCH_DEVICE_REPLY);
        NAT_FETCH_DEVICE_REPLY *fetchDeviceReply = reinterpret_cast<NAT_FETCH_DEVICE_REPLY*>(reply);
        NotifyReply(Nat_TraslateClientError(fetchDeviceReply->stHeader.dwStatus), fetchDeviceReply); 
        // completed
        m_workThreadRuning = false;
    }
    else
    {
        NotifyReply(NAT_CLI_ERR_TIMEOUT, NULL);
    }

}

PUB_THREAD_RESULT PUB_THREAD_CALL CNatDeviceFetcherWorker::WorkThreadFunc( void *pParam )
{
#ifdef	__ENVIRONMENT_LINUX__
    NAT_LOG_INFO("CNatUdtClientPeer thread pid = %d\n",getpid());
#endif
    assert(NULL != pParam);
    CNatDeviceFetcherWorker *worker = reinterpret_cast<CNatDeviceFetcherWorker*>(pParam);
    worker->RunWork();

    return 0;
}

int CNatDeviceFetcherWorker::RunWork()
{
    if (InitTraversalClient() != 0)
    {
        NotifyReply(NAT_CLI_ERR_SYS, NULL);
       return 0;
    }

    NatRunResult runResult = RUN_NONE;
    NatRunResult tempRet = RUN_NONE;
    unsigned long curTickCount = 0;

    while (m_workThreadRuning)
    {
        curTickCount = Nat_GetTickCount();

        if (PUB_IsTimeOutEx(m_startTime, m_deviceFetcher.m_iTimeout,curTickCount))
        {
            NAT_LOG_DEBUG("Fetch device info timeout!\n");
            NotifyReply(NAT_CLI_ERR_NETWORK, NULL);
            break;
        }

        if (m_isCompleted) break;

        tempRet = m_udtTransport.Run();
        runResult = NatRunResultMax(runResult, tempRet);
        if (runResult == RUN_DO_FAILED)
        {
            NAT_LOG_DEBUG("Fetch device info done! Udt transport run failed!\n");
            NotifyReply(NAT_CLI_ERR_NETWORK, NULL);
            break;
        }

        if (m_isCompleted) break;

        tempRet = m_traversalClient.Run(curTickCount);
        if (tempRet == RUN_DO_FAILED)
        {
            NAT_LOG_DEBUG("Fetch device info done! Traversal client run failed!\n");
            NotifyReply(NAT_CLI_ERR_NETWORK, NULL);
            break;
        }

        if (m_isCompleted) break;

        if (runResult != RUN_DO_MORE)
        {
            PUB_Sleep(10); 
        }

    }
    return 0;
}



int CNatDeviceFetcherWorker::InitTraversalClient()
{
    unsigned long ip = 0;
    NAT_DEVICE_FETCHER_CONFIG &config = m_deviceFetcher.m_config;
    if (!Nat_ParseIpByName(config.pServerIp, ip))
    {
        NAT_LOG_WARN("CNatDeviceFetcherWorker parse nat server address(%s) failed!\n", config.pServerIp);
        return -1;
    }
    NAT_LOG_DEBUG("CNatDeviceFetcherWorker NatServer Ip = %s \n", Nat_inet_ntoa(ip));

    if (!m_traversalClient.Start(ip, config.nServerPort, &m_udtTransport))
    {
        NAT_LOG_WARN("CNatDeviceFetcherWorker start traversal client failed!\n");
        return -1;
    }
    m_traversalClient.SetNotifier(this);

    return 0;
}

void CNatDeviceFetcherWorker::NotifyReply( NAT_CLIENT_ERROR error, NAT_FETCH_DEVICE_REPLY *reply )
{
    if (m_isCompleted)
    {
        return;
    }

    m_deviceFetcher.m_syncLock.Lock();
    NAT_DEVICE_INFO &m_deviceInfo = m_deviceFetcher.m_deviceInfo;
    memset(&m_deviceInfo, 0, sizeof(m_deviceInfo));
    m_deviceFetcher.m_error = error;

    NAT_LOG_DEBUG("CNatDeviceFetcher reply status=%s\n", NAT_GetClientErrorText(error));

    if (m_deviceFetcher.m_error == NAT_CLI_OK)
    {
        assert(reply != NULL);
        NAT_FETCH_DEVICE_REPLY &replyRef = *reply;
        m_deviceInfo.dwDevicePeerIp = replyRef.dwDevicePeerIp;
        m_deviceInfo.usDevicePeerPort = replyRef.usDevicePeerPort;
        m_deviceInfo.ucCanRelay = replyRef.ucCanRelay;

#ifdef __NAT_DEVICE_CONFIG_EX__
		strcpy(m_deviceInfo.caExtraInfo, replyRef.caExtraInfo);
#else
		NAT_DEVICE_EXTRA_INFO deviceExtraInfo;		
		CNatTraversalXmlParser::ParseDeviceExtraInfo(&deviceExtraInfo, replyRef.caExtraInfo);
        m_deviceInfo.dwDeviceType = deviceExtraInfo.deviceType;
        strcpy(m_deviceInfo.caDeviceVersion, deviceExtraInfo.deviceVersion);
        m_deviceInfo.usDeviceWebPort = deviceExtraInfo.deviceWebPort;
        m_deviceInfo.usDeviceDataPort = deviceExtraInfo.deviceDataPort;
#endif//__NAT_DEVICE_CONFIG_EX__
		
    }

    if (m_deviceFetcher.m_isSync)
    {
        m_deviceFetcher.m_syncCompleted = true;
    }
    else
    {
        if (m_deviceFetcher.m_error == NAT_CLI_OK)
        {
            m_deviceFetcher.NotifyCallback(&m_deviceInfo);
        }
        else
        {
            m_deviceFetcher.NotifyCallback(NULL);
        }
    }

    m_deviceFetcher.m_syncLock.UnLock();
    m_isCompleted = true;
}

////////////////////////////////////////////////////////////////////////////////
// class CNatDeviceFetcher implements

CNatDeviceFetcher* CNatDeviceFetcher::NewDeviceFetcher( const NAT_DEVICE_FETCHER_CONFIG *config )
{
    CNatDeviceFetcher *deviceFetcher = new CNatDeviceFetcher(config);
    if (deviceFetcher != NULL)
    {
        if (!deviceFetcher->Init())
        {
            delete deviceFetcher;
            deviceFetcher = NULL;            
        }
    }
    else
    {
        NAT_LOG_WARN("CNatDeviceFetcher::NewDeviceFetcher failed! The memory is used up?\n");
    }
    return deviceFetcher;
}

int CNatDeviceFetcher::Destroy()
{
    delete this;
    return 0;
}

int CNatDeviceFetcher::FetchSyn(int iTimeOut, NAT_DEVICE_INFO *pDeviceInfo)
{
    assert(!m_syncCompleted);
    assert(pDeviceInfo != NULL);
    m_iTimeout = iTimeOut;
    m_isSync = true;
    m_error = NAT_CLI_OK;

    if (!m_worker->Start())
    {
        return -1;
    }

    m_syncCompleted = false;
    while (!m_syncCompleted)
    {
        PUB_Sleep(10); // TODO: how long to wait?
    }
    
    int ret = 0;
    m_syncLock.Lock();
    if (m_error == NAT_CLI_OK)
    {
        *pDeviceInfo = m_deviceInfo;
        ret = 0;
    } 
    else
    {
        ret = -1;
    }
    m_syncLock.UnLock();

    m_worker->Stop();
    return ret;
}

int CNatDeviceFetcher::FetchAsyn( NAT_FETCH_DEVICE_CALLBACKEX pCallback, void* pObject, int iTimeOut )
{
    assert(!m_worker->IsStarted());
    m_iTimeout = iTimeOut;
    m_error = NAT_CLI_OK;
    m_isSync = false;
    m_pCallback = pCallback;
    m_pObject = pObject;
    return 0;
}

NAT_CLIENT_ERROR CNatDeviceFetcher::GetError()
{
    return m_error;
}

CNatDeviceFetcher::CNatDeviceFetcher( const NAT_DEVICE_FETCHER_CONFIG *config )
:
m_worker(NULL),
m_error(NAT_CLI_OK),
m_config(*config),
m_isSync(false),
m_syncCompleted(false),
m_pCallback(NULL),
m_pObject(NULL)
{

}

CNatDeviceFetcher::~CNatDeviceFetcher()
{
    if (m_worker != NULL)
    {
        if (m_worker->IsStarted())
        {
            m_worker->Stop();
        }
        delete m_worker;
    }
}

bool CNatDeviceFetcher::Init()
{
    m_worker = new CNatDeviceFetcherWorker(*this);
    if (m_worker == NULL)
    {
        NAT_LOG_WARN("CNatDeviceFetcher create worker failed! The memory is used up?\n");
        return false;
    }
    return true;
}

void CNatDeviceFetcher::NotifyCallback(NAT_DEVICE_INFO *pDeviceInfo)
{
    if (m_pCallback != NULL)
    {
        m_pCallback(pDeviceInfo, m_pObject, NULL);
    }
}
