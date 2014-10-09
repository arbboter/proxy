// NatUdt.h: interface for the CNatUdt class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_UDT_H_
#define _NAT_UDT_H_

#include "NatCommon.h"
#include "NatRunner.h"
#include "NatHeapDataManager.h"
#include "NatUdtTransport.h"
#include <list>
#include <map>

#define		RTT_RESOLUTION_1

//以下两个宏一起表示当前UDT的版本号，如下表示版本:0.1
#define     P2P_VERSION_HIGH    0
#define     P2P_VERSION_LOW     1

const byte	UDP_DATA_TYPE_SYN		= 0x0;	//连接数据
const byte	UDP_DATA_TYPE_RELIABLE	= 0x1;	//可靠数据
const byte	UDP_DATA_TYPE_ACK		= 0x2;	//纯确认数据，穿透、心跳、增加乱序包确认
const byte	UDP_DATA_TYPE_RST		= 0x3;	//断开


const byte SWL_UDP_DATA_STATUS_NOSEND = 0;		//没有发送过
const byte SWL_UDP_DATA_STATUS_SENDED = 0x01;	//发送过，但是没有被确认过，最低位为0则没有发送过
const byte SWL_UDP_DATA_STATUS_TIMEOUT = 0x02;	//已经超时
const byte SWL_UDP_DATA_STATUS_BEYOND = 0x04;	//对方已经收到了超过本数据标识的包

const byte	SWL_UDP_STATE_UNCONNECTED	= 0;	
const byte SWL_UDP_STATE_RECVED		= 1;	//收到对方数据
const byte	SWL_UDP_STATE_CONNECTED		= 2;	//收到对方数据，并且知道对方收到自己这边发出的数据

const byte SWL_UDP_SENDMODE_SLOW_STARTUP = 0;		//慢启动
const byte SWL_UDP_SENDMODE_CROWDED_AVOID = 1;		//拥塞避免
const byte SWL_UDP_SENDMODE_TIMEOUT_RESEND = 2;	//超时重传
const byte SWL_UDP_SENDMODE_FAST_RESEND = 4;		//快速重传恢复
//const byte SWL_UDP_SENDMODE_RESET		= 8;	//重传条件限制

const byte SWL_UDP_ACK_STATUS_NONE = 0;		//没有确认可发
const byte SWL_UDP_ACK_STATUS_DELAY = 1;		//超时了再发，或者捎带发送
const byte SWL_UDP_ACK_STATUS_NOW = 2;			//有两个以上的新数据到了，就需要立即发送确认
const byte SWL_UDP_ACK_STATUS_DISORDER = 3;	//收到乱序包需要立即发送确认


#define UDP_DISORDER_SEND_TIMES	3		//乱序包确认最多发几次	
#define UDP_TIMEOUT				60000	//毫秒
#define UDP_CONNECT_INTERVAL	1000	//毫秒为单位，多久间隔没收到连接回复，就重发一次请求回复
#define SEND_ACK_INTERVAL		100		//每隔多长时间发送一次确认（如果有确认需要发）
#define MAX_RESEND_TIME			5000	//最大的重传间隔，以为毫秒为单位
#define UDT_HEARTBEAT_INTERVAL	30000	//UDT心跳数据包发送间隔，主要是为了保持UDT连接

#define MIN_RTT_TIME			100		
#define MIN_SSTHRESH			5
#define MIN_CWND_WIN			3

static const byte NAT_UDT_CATEGORY_INVALID = 0x00;
static const byte NAT_UDT_CATEGORY_P2P = 0x01;  /**< P2P通信分类 */
static const byte NAT_UDT_CATEGORY_P2NS = 0x02; /**< Peer-to-NatServer通信分类 */
const uint16 NAT_UDT_PROTOCOL_TAG = 0xfefe;

#pragma pack(push, 1)

typedef struct _udp_wrapper_head
{
	byte	category;			//分类，用于区分不同的通信数据
	byte	cmdId : 4;			//命令标识
	byte	reserved : 4;		//保留字段
	uint16	maxPacketLen;		//用于通知对方，自己发送的应用层包最大长度，即MTU减去IP头和UDP头
	uint32	connectionID;		//Connection ID,区分同IP同port的连接
	uint32	sendIndex;			//发送包标识，每发送一个数据就+1（包括重传，确认，心跳包）
	uint32	recvIndex;			//收到过的最大的包标识
	uint32	seqNum;				//发送的序列号,只有发的包是数据命令包时，SeqNum才加1
	uint32	ackNum;				//接收的序列号
}UDP_WRAPPER_HEAD;

#define MAX_WRAPPER_DATA_BUFFSIZE  1500
//(296 - 20 - 8) * 2 + 20 +8,为了防止被路由器当成DOS攻击，把包变成需拆分为两个包的大小
#define MTU_LEN						564 //IPV6要求链路层最小长度为1280
#define MAX_WRAPPER_DATA_LEN	(MTU_LEN - 20 - 8) //(MTU_LEN - 20 - 8) 20 IP头的最小长度 8 UDP头

#define WRAPPER_VALIDATA_LEN	(MAX_WRAPPER_DATA_LEN - sizeof(UDP_WRAPPER_HEAD))//非组合数据的有效数据最大长度

#define MAX_SEND_RELIABLE_BUFFSIZE					1024*1024

/*modified by cc : MAX_SEND_BUFF_PACKET_COUNT只依赖于MAX_SEND_RELIABLE_BUFFSIZE和WRAPPER_VALIDATA_LEN*/
const uint32 MAX_SEND_BUFF_PACKET_COUNT = MAX_SEND_RELIABLE_BUFFSIZE / WRAPPER_VALIDATA_LEN + 1;

typedef struct _udp_ack
{
	// 	uint16	nIndex;			//包标识
	// 	uint16	nRecvTimes;		//收过几次了
	uint32	ackSeq;		//确认第几号包
}UDP_ACK;

const uint32 MAX_ACK_COUNT = WRAPPER_VALIDATA_LEN / sizeof(UDP_ACK);

typedef struct _udp_syn_data_
{
	uint16	protocolTag;
	uint16	version;
}UDP_SYN_DATA;

typedef struct _udp_wrapper_data
{
	UDP_WRAPPER_HEAD head;
	union
	{
		char			data[WRAPPER_VALIDATA_LEN];
		UDP_ACK			acks[MAX_ACK_COUNT];
		UDP_SYN_DATA	syn;
	}wrapper_dat;
}UDP_WRAPPER_DATA;

typedef struct _udp_syn_packet_
{
	UDP_WRAPPER_HEAD	head;
	UDP_SYN_DATA		data;
}UDP_SYN_PACKET;

#pragma pack(pop)

typedef struct _udp_send_packet_data
{
	int32			iLen;							//最大为WRAPPER_VALIDATA_LEN
	uint32			lastSendTick;					//上一次发送的时间
	uint32			lastSendIndex;					//上一次发送时，本数据的标识
	byte			dataStatus;						//对方收到已经确认，但是这个确认被乱序确认的，不是顺序确认的，所以暂时没删除

	UDP_WRAPPER_DATA		wrapperData;
}UDP_SEND_PACKET_DATA;


// typedef struct _udp_disorder_seq
// {
// 	uint32 disorderSeq;								//乱序包的序号
// 	uint32 sendTimes;								//发送的次数
// }UDP_DISORDER_SEQ;

typedef struct _udp_reliable_send_data_info
{
	uint32								nextSeq;				//下一个包放进list应该的序号，先使用，再加一
	int32								remainedBuffSize;		//发送缓存还可使用的数据长度
	int32								remainHeapPacketNum;	//发送缓存还可使用的堆空间的包个数
	std::list<UDP_SEND_PACKET_DATA*>	listReliableSendData;	//待发送的可靠数据
	std::list<UDP_SEND_PACKET_DATA*>	listUnackedData;		//发送后未acked的数据
}R_SEND_INFO;


typedef struct _nat_udt_recv_data_
{
    int32				iPacketLen;  /**< UDT接收数据包字节长度 */
    UDP_WRAPPER_DATA	pPacket;    /**< UDT接收数据包 */

    /**
     * UDT接收数据包内所包含的数据大小（载体大小）
     */
    inline int32 PacketSize()
    {
        return iPacketLen - sizeof(pPacket.head);
    }

}NAT_UDT_RECV_DATA;

typedef std::map<uint32, NAT_UDT_RECV_DATA*> NatUdtRecvDataList;

typedef struct _udp_reliable_recv_data_info
{	
	uint32				minSeq;									//应该轮到传出去给应用层的最小数据序号
	uint32				expectedSeq;							//希望下一个接收到的顺序包
	uint32				maxPeerAck;								//对方确认到的最大的序号

	NatUdtRecvDataList	recvDataList;                       // UDT接收到的数据列表

	/*add by cc  一个字段：上一次回调给应用层的包的pos 其对应的包为编号为minSeq的包
	应用层可以不用每次把一个包都收完，pos即应用层接收了该包的实际长度*/
	int32				lastOnRecvPackPos;
}R_RECV_INFO;

typedef struct _udp_rtt_help
{
	uint32 sendIndex;								//发送索引
	uint32 sendTick;								//发送的时间
}UDP_RTT_HELP;


typedef struct _udt_remote_info_
{
	//相当于SWL_SOCKET中实际的ip和port
	uint32 ip;
	uint16 port;
	char Ip[64];
}UDT_REMOTE_INFO;

typedef struct _udt_recorded_info_
{
	uint32			lastRecvTick;					//最后一次接收到数据的tick
	uint32			lastSendTick;					//最后一次发送数据的tick
	byte			state;							//当前的连接状态
	byte			ackStatus;						//发送确认的状态 立即发送 延迟发送 没得发送
	uint32			initSendSeq;
	uint32			initPeerSendSeq;
	uint32			lastSendIndex;					//上一次发送的标识
	uint32			maxRecvIndex;					//收到的对方的最大的标识
	uint32			maxPeerRecvIndex;				//对方收到的最大的标识
}UDT_RECORDED_INFO;

//统计信息
typedef struct _udt_statistic_
{
	uint32			totalSendPacketNum;		//总发包数
	uint32			totalRecvPacketNum;		//总收包数
	uint32			reSendPacketNum;		//重发的包数
	uint32			connectTime;			//连接建立的时间
	uint64		totalSendBytes;			//发送的总字节数
	uint64		totalRecvBytes;			//接收的总字节数
	uint64		reSendBytes;			//重发的字节数
	uint16			localDataPort;			//本地发送数据的端口
}UDT_STATISTIC;

//重传和拥塞控制算法相关信息
typedef struct _udt_ctrl_algorithm_
{
	bool						bResendRestrict;		//重传限制
	uint32				nextResendIndex;		//允许开始计算是否要重传的序号

	//第几次重传第一个数据包，每重传一次第一个包就+1，每次只要知道对方收到任何新数据（index）就清0
	//这个变量是在对方老收不到数据时，用于退避超时重传
	byte				resendTime;

	//目前的发送状态：慢启动、拥塞避免、超时重传、快速重传恢复、重新恢复最小窗口慢启动
	//SWL_UDP_SENDMODE_SLOW_STARTUP  
	//SWL_UDP_SENDMODE_CROWDED_AVOID 
	//SWL_UDP_SENDMODE_TIMEOUT_RESEND		
	//SWL_UDP_SENDMODE_FAST_RESEND 
	byte				sendMode;

	//用于记录第一个包重传的次数，用于减小窗口，只要是第2次重传无论快速还是超时，则再次进超时重传
	//则参数会被再次减小
	byte				firstResend;	

	bool						bCalculateRtt;
	UDP_RTT_HELP				rttHelp;
}UDT_CTRL_ALG;

class CNatUdt;

class CUdtNotifier
{
public:
	 /**
     * 连接建立三次握手成功后的回调
     * @param pUDT UDT对象指针
     * @param iErrorCode 0 成功 其他值 失败
     */
    virtual void OnConnect(CNatUdt *pUDT, int iErrorCode) = 0;

    /**
     * 接收数据回调。
     * 当有数据接收后，触发该回调。
     * @param pUDT UDT对象指针
     * @param pBuf 数据指针，指向已接收的数据。
     * @param iLen 要接收数据的字节大小。
     * @return 返回回调后的数据已经处理的数据字节大小，该值<=iLen
     */
    virtual int OnRecv(CNatUdt *pUDT, const void* pBuf, int iLen) = 0;

	 /**
     * 发送数据回调
     * 当有真正的数据发送时，触发该回调。
     * @param pUDT UDT对象指针
     * @param pBuf 数据指针，指向要发送的数据。
     * @param iLen 要发送数据的字节大小。
     * @return 返回成功或失败 0 成功  其他值 失败
     */
    virtual int OnSendPacket(CNatUdt *pUDT, const void* pBuf, int iLen) = 0;
};

typedef struct _nat_udt_config_
{
    byte					category;               // UDT的分类
    uint32					remoteIp;               // 对方的IP地址
    uint16					remotePort;             // 对方的端口号
    uint32					connectionID;           // 连接ID
    uint32					maxRecvWindowSize;      // 最大接收窗口大小
    CNatHeapDataManager*	pRecvHeapDataManeger;  // 发送数据的缓存堆管理器，如果为NULL，则由内部根据maxSendWindowSize创建。
    uint32					maxSendWindowSize;      // 最大发送窗口大小
    CNatHeapDataManager*	pSendHeapDataManeger;  // 发送数据的缓存堆管理器，如果为NULL，则由内部根据maxSendWindowSize创建。
}NAT_UDT_CONFIG;

class CNatUdt
{
public:
    static const int RECV_PACKET_BUF_SIZE = MAX_WRAPPER_DATA_BUFFSIZE;    // 等于 1500
    static const int SEND_PACKET_BUF_SIZE = sizeof(UDP_SEND_PACKET_DATA); // 等于 552
public:
	static bool IsDatagramValid(const NAT_TRANSPORT_RECV_DATAGRAM* datagram);
	static byte GetDatagramCategory(const NAT_TRANSPORT_RECV_DATAGRAM* datagram);
    static uint32 GetDatagramConnectionId(const NAT_TRANSPORT_RECV_DATAGRAM* datagram);
    static bool IsDatagramSynCmd(const NAT_TRANSPORT_RECV_DATAGRAM* datagram);
public:
	CNatUdt();
	
	~CNatUdt();

    bool Start(const NAT_UDT_CONFIG* config);

    void Stop();

    bool IsStarted() { return m_bStarted; };

    NatRunResult Run();

	uint32 GetConnectionID();
	uint32 GetRemoteIp() {return m_RemoteInfo.ip;}
    uint16 GetRemotePort() {return m_RemoteInfo.port;}
    
    bool IsMineDatagram(const NAT_TRANSPORT_RECV_DATAGRAM* datagram);

    int NotifyRecvDatagram(const NAT_TRANSPORT_RECV_DATAGRAM* datagram);

	int Send(const void *pData, int iLen);//应用层调用此接口，发送可靠数据

	int Recv(void *pData, int iLen);//应用层主动调用此接口，获取可靠数据


	void SetNotifier(CUdtNotifier *pUdtNotifier);

    
	/**
     * 设置上层是通过回调的方式获取数据，还是主动获取数据
     */
	void SetAutoRecv(bool bAutoRecv);

	 /**
     * 获取当前可以发送数据的字节大小
     * @return 如果没有错误，则返回当前可以发送数据的字节大小，0表示发送缓冲区满了；否则，出错返回-1
     */
    int GetSendAvailable();

    /**
     * 获取可以接收数据的字节大小，主要用于主动接收数据
     * @return 如果没有错误，则返回当前可以接收数据的字节大小，0表示无数据；否则，出错返回-1
     */
    int GetRecvAvailable();
private:
    int RecvPacket(const UDP_WRAPPER_DATA *packet, int packetSize);
	int OnNotifySendPacket(const void* pBuf, int iLen);
	int OnNotifyRecvPacket(const void* pBuf, int iLen);
	void OnNotifyConnect(int iErrorCode);

	int HandleRecvReliableData(const UDP_WRAPPER_DATA *pWrapperData, int iLen, uint32 currentTick);
	int HandleRecvAckData(const UDP_WRAPPER_DATA *pData, int iLen, uint32 currentTick);
	int HandleRecvConnect(const UDP_WRAPPER_DATA *pData, int iLen, uint32 currentTick);
	int HandleRecvRST(const UDP_WRAPPER_DATA *pData, int iLen, uint32 currentTick);

	int CheckConnect(uint32 currentTick);
	int SendAckData(uint32 currentTick);
	int ReleaseReliableData(uint32 seq, uint32 index, uint32 currentTick);
	uint32 CalculateRTO(uint32 currentTick, uint32 startTick);
	int ChangeMode(byte ucMode);
	int IncreaseCwnd(int iCount = 1, bool bResend = false);
	int AdjustRecvPacket();
	int RecvFirstReliableData(char **ppData, int *pLen);
	void AutoNotifyRecvPacket();

	//1	 重传了一个数据 0 没有数据需要重传，没有数据或者没有符合发送条件的数据
	//-1 异常或者连接出错 -2 发不出去
	//-4 没有到均匀发送间隔 -5 未连接
	int ResendData(uint32 currentTick);

	//1	 表示发送了1个数据  
	//0	 表示没数据发送
	//-1 异常或者需要退出
	//-2 表示数据暂时发不出去
	//-3 超过允许发送的离第一个未确认的数据最大的范围
	//-4 没有到均匀发送间隔
	//-5 没有连接上
	//-6 窗口不够
	int SendReliableData(uint32 currentTick);

	int SendPacketData(UDP_SEND_PACKET_DATA *pPacket, uint32 currentTick);

	/**
     * 主动发送RST信号，通知对端连接关闭
     */
	void Disconnect();

    bool InsertRecvData( uint32 seqNum, const UDP_WRAPPER_DATA* packet, int dataSize  );

    inline void DestroyRecvData(NAT_UDT_RECV_DATA* data) 
    {
        assert(data != NULL);
        m_pRecvDataManager->ReleaseMemory(data);
    }
private:
	bool						m_bStarted;
	CUdtNotifier				*m_pUdtNotifier;
	UDT_RECORDED_INFO			m_RecordedInfo;
	byte						m_Category;
	bool						m_bIsAutoRecv;//上层取数据的方式是回调，还是主动拿

#ifndef UDT_WITHOUT_LOCK
	CPUB_Lock					m_RecvLock;//锁m_RRecvInfo.mapReliableRecvData和m_RRecvInfo.minSeq
	CPUB_Lock					m_RSendInfoLock;//锁m_RSendInfo
#endif

//对端IP PORT等信息
	UDT_REMOTE_INFO				m_RemoteInfo;

    //数据相关
    CNatHeapDataManager			*m_pSendDataManager;
    CNatHeapDataManager			*m_pOwnedSendDataManager; // 自己持有的SendDataManager指针，如果不为空，需要自己释放
    CNatHeapDataManager			*m_pRecvDataManager;
    CNatHeapDataManager			*m_pOwnedRecvDataManager; // 自己持有的RecvDataManager指针，如果不为空，需要自己释放
	R_SEND_INFO					m_RSendInfo;
	R_RECV_INFO					m_RRecvInfo;

	UDP_WRAPPER_DATA			m_SendTmpData;	//用于发送ACK SYN,避免再临时开辟空间

	std::list<uint32>	m_listDisorderlyPacket;	//乱序的数据序号:disorderSeq

//统计信息
	UDT_STATISTIC				m_Statistic;

//重传和拥塞算法的控制信息
	UDT_CTRL_ALG				m_CtrlAlg;
	short						m_sa;
	short						m_sv;
	uint32				m_dwRTT;
	uint32				m_dwRTO;
	int							m_iUsed;			//猜测还有几个包在网络上 （未回复并且猜测未丢弃的数据，称为在网络上的包）
	int							m_iCwnd;			//拥塞窗口，即允许有几个没有回复的数据包在网络上（一次可发几个包出去）
	int							m_iCrowdedNumerator;//拥塞时累加的分子
	int							m_iSsthresh;		//慢启动门限，拥塞窗口增大到这里时开始进行拥塞避免

//超时时间
	uint32				m_UDP_TIMEOUT;
	uint32				m_UDP_MAX_SEND_INTERVAL;
	uint32				m_LastAckTime;
	uint32				m_LastResendTime;
	uint32				m_LastRemoveUnAckPackTime;
//
	uint32				m_dwMaxRecvReliableBuffCount;//记录缓冲区允许的的最大包个数
	uint32				m_curTickCount;

};


#endif//_NAT_UDT_H_