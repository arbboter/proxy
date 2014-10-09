#ifndef _WEB_SERVER_H_
#define _WEB_SERVER_H_
#include "SWL_ListenProcEx.h"
#include "SWL_MultiNetCommEx.h"
#include "netdeviceman.h"
#include <list>
#include <vector>
#include <string>
#include <set>

class CProxyServer
{
	typedef enum _http_recv_state_
	{
		HTTP_STATE_HEAD,
		HTTP_STATE_BODY,
		HTTP_STATE_FINISH,
	}HTTP_RECV_STATE;
	
	typedef enum _http_msg_req_
	{
		MSG_REQUEST_ERROR,
		MSG_REQUEST_ADD_DEV,
		MSG_REQUEST_DEL_DEV,
		MSG_REQUEST_DEV_BIND,
		MSG_REQUEST_DOWNLOAD,
		MSG_REQUEST_REQ_M3U8,
		MSG_REQUEST_START_LIVE,
		MSG_REQUEST_CONFIG,
		MSG_REQUEST_DEV_INFO,
		MSG_REQUEST_LOCAL_DEVICES,
		MSG_REQUEST_CL_DEV,
		MSG_REQUEST_DEV_NET_STATUS,
		MSG_REQUEST_UPGRADE,
	}HTTP_MSG_REQUEST;

	typedef enum _http_msg_reply_
	{
		MSG_REPLY_ERROR,
		MSG_REPLY_STREAM,
		MSG_REPLY_ADD_DEV,
		MSG_REPLY_DEV_BIND_SUC,
		MSG_REPLY_DEV_BIND_FAILED,
		MSG_REPLY_M3U8_SUC,
		MSG_REPLY_M3U8_FAILED,
		MSG_REPLY_LIVE_SUC,
		MSG_REPLY_LIVE_FAILED,
		MSG_REPLY_DOWNLOAD_SUC,
		MSG_REPLY_DOWNLOAD_FAILED,
		MSG_REPLY_DEV_INFO,
		MSG_REPLY_LOCAL_DEVICES,
		MSG_REPLY_CL_DEV,
		MSG_REPLY_DEV_NET_STATUS,
		MSG_RELY_UPGRADE,
	}HTTP_MSG_REPLY;

	typedef struct _http_request_
	{
		char *buf;
		long bufLen;
		long dataLen;
		long headLen;
		long contextLen;
		long recvState;   //HTTP_RECV_STATE
	}HTTP_REQUEST_MSG;
	
	typedef struct _http_response_msg_
	{
		char *buf;
		long bufLen;
		long dataLen;
		long usePos;
		long netSessionID;
		void *param;
		long replyStatus; //HTTP_MSG_REQ_RESULT
	}HTTP_RESPONSE_MSG;

	typedef struct _http_client_info_
	{
		long netSessionID;
		long count;
		TVT_SOCKET sockfd;
		HTTP_REQUEST_MSG httpReq;
		HTTP_RESPONSE_MSG httpRep;

		unsigned long lastTimeRecv;
		unsigned long lastTimeSend;
	}HTTP_CLIENT_INFO;

	typedef struct _http_stream_send_status_
	{
		long netSessionID;
		bool bSended;
	}HTTP_STREAM_SEND_STATUS;

	typedef std::list<HTTP_STREAM_SEND_STATUS> HTTP_STREAM_SEND_STATUS_LIST;

	typedef struct _http_stream_
	{
		int channl;
		bool bLive;
		bool bMainStream;
		unsigned long beginpos;
		unsigned long endpos;
		char filename[SHORT_STR_LEN+1];
		char serialNo[SHORT_STR_LEN+1];
	}HTTP_STREAM;

	typedef struct _http_stream_send_info_
	{
		long channl;
		bool bLive;
		bool bMainStream;
		char *pData;
		unsigned long dataLen;
		char serialNo[SHORT_STR_LEN+1];
		char fileName[SHORT_STR_LEN+1];
		HTTP_STREAM_SEND_STATUS_LIST statusList;
	}HTTP_STREAM_SEND_INFO;

	typedef std::list<HTTP_RESPONSE_MSG *> HTTP_RESPONSE_MSG_LIST;
	typedef std::list<HTTP_CLIENT_INFO *> HTTP_CLIENT_INFO_LIST;
	typedef std::list<HTTP_STREAM_SEND_INFO*> HTTP_STREAM_SEND_INFO_LIST;  	
	
public:
	bool Init(unsigned short port);
	void Quit();

	bool Start();
	void Stop();

public:
	void OnAccept(CLIENT_SOCK_INFO *pClientSockInfo);
	void OnRecvData(long netSessionID, RECV_DATA_BUFFER *pDataBuffer);

	void ProcCmd();
	void ProcReply();

private:
	HTTP_MSG_REQUEST ParserHttpRequest(HTTP_CLIENT_INFO *pClient,std::string &result);

	void BuildHttpResponse(int httpCode,std::vector<char> &context,HTTP_RESPONSE_MSG *pResponseMsg);

	void OnAddDev(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg);
	void OnDelDev(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg);
	void OnCleanDev(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg);

	void OnDownLoad(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg);

	void OnM3U8(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg);
	void OnLive(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg);
	
	void OnConfig(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg);

	void OnError(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg);

	void OnGetAllDevice(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg);
	void OnGetDeviceInfo(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg);
	void OnGetDeviceNetStatus(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg);

	void OnBindDevOwner(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg);

	void OnUpGrade(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg);

private:

	bool SendMsg();

	bool SendStream();

	void ClearDevice(); 

	void GetAllClient(std::set<long> &netInfoList);
	void AddClient(HTTP_CLIENT_INFO *pClient);
	HTTP_CLIENT_INFO* GetClientRefer(long netSessionID);
	void RelClientRefer(long netSessionID);
	void RelClientRefer(HTTP_CLIENT_INFO *pClient);
	void DelClient(long netSessionID);
	
	void PutReplyMsg(HTTP_RESPONSE_MSG *pMsg);
	HTTP_RESPONSE_MSG * GetReplyMsg(long netSessionID);
	void DelMsg(long netSessionID);

	long SendData(HTTP_CLIENT_INFO *pClient);

	void AddStream(HTTP_STREAM *pHttpStream,long netSessionID);
	
	void PackStream(HTTP_CLIENT_INFO *pClient,char *pData, long len,bool bLive);

private:
	unsigned short		m_port;	
	CSWL_ListenProcEx	*m_pListenProc;
	CSWL_MultiNetCommEx *m_pMultiNetComm;

	PUB_thread_t		m_hProcCmdThreadID;
	bool				m_bProcCmdRun;

	PUB_thread_t		m_hProcReplyThreadID;
	bool				m_bProcReplyRun;

	HTTP_CLIENT_INFO_LIST	m_clientList;
	CPUB_Lock				m_clientLock;

	std::list<long>			m_httpReqList;
	CPUB_Lock				m_httpReqLock;

	HTTP_RESPONSE_MSG_LIST	m_responseList;
	CPUB_Lock				m_responseLock;

	HTTP_STREAM_SEND_INFO_LIST m_streamList;

	CNetDeviceMan		m_netDevMan;

};

#endif