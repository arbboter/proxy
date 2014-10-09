#include "proxyserver.h"
#include "json.h"
#include <sstream>
#include <vector>
#include <algorithm>
#include <iostream>

const unsigned long WEB_SER_RECV_BUF_LEN = 4*1024;
const unsigned long WEB_SER_HTTP_REQ_LEN = 4*1024;
const unsigned long WEB_SER_HTTP_REP_LEN = 12*1024;
const unsigned long MAX_DEVICE_TIME_OUT = 20000;


const char g_msg_404[] = "<html><head><title>404 Not Found</title></head><body bgcolor=\"white\"><center><h1>404 Not Found</h1></center><hr><center>TVT</center></body></html>";
const char g_m3u8_header[] = "#EXTM3U\n#EXT-X-TARGETDURATION:1\n#EXT-X-ALLOW-CACHE:NO\n#EXT-X-VERSION:3\n#EXT-X-MEDIA-SEQUENCE:0\n#EXT-X-PLAYLIST-TYPE:VOD\n#EXTINF:1,\nlive.ts\n#EXT-X-ENDLIST\n";
const char g_msg_200[] = "HTTP/1.1 200 OK\r\nContent-Length:%d\r\nContent-Type:%s\r\nAccept-Ranges:bytes\r\nConnection:keep-alive\r\n\r\n";
const char g_msg_200_chunked[] = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nContent-Type: video/mp2t\r\nConnection: Keep-Alive\r\n\r\n";
const char g_msg_206[] = "HTTP/1.1 206 Partial Content\r\nDate: Fri, 20 Jun 2014 09:41:09 GMT\r\nLast-Modified: Fri, 20 Jun 2014 09:40:58 GMT\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: close\r\nContent-Range: bytes %d-%d/%d\r\n\r\n";

//处理新的用户请求
static int AcceptLinkCallback(CLIENT_SOCK_INFO *pClientSockInfo, void *pParam1, void *pParam2)
{
	assert(NULL != pParam1);
	CProxyServer *pThis = reinterpret_cast<CProxyServer *>(pParam1);
	pThis->OnAccept(pClientSockInfo);
	return 0;
}


//处理客户端请求
static PUB_THREAD_RESULT PUB_THREAD_CALL ProcCmdThread(void *pParam)
{
	assert(NULL != pParam);
	CProxyServer *pThis = reinterpret_cast<CProxyServer *>(pParam);
	pThis->ProcCmd();
	return 0;
}


//回复客户端
static PUB_THREAD_RESULT PUB_THREAD_CALL ProcReplyThread(void *pParam)
{
	assert(NULL != pParam);
	CProxyServer *pThis = reinterpret_cast<CProxyServer *>(pParam);
	pThis->ProcReply();
	return 0;
}

//网络数据接收回调函数
static int WebServerDataCallback(long netSessionID, void* pParam, RECV_DATA_BUFFER *pDataBuffer)
{
	assert(NULL != pParam);
	CProxyServer *pThis = reinterpret_cast<CProxyServer *>(pParam);
	pThis->OnRecvData(netSessionID, pDataBuffer);
	return 0;
}

static std::string GetField(const char* buf, const char* key)
{
	std::string strKey(key);
	strKey += ": ";
	std::string strValue("");

	// 找到字段的开始:起始位置或者是新一行的起始位置
	const char* pszStart = strstr(buf, strKey.c_str());
	if(pszStart == NULL || (pszStart != buf && *(pszStart - 1) != '\n'))
	{
		return strValue;
	}

	pszStart += strKey.size();

	// 找到字段结束
	const char* pszEnd = strstr(pszStart, "\r\n");
	if(pszEnd == NULL) return strValue;

	strValue.assign(pszStart, pszEnd - pszStart);
	return strValue;
}


static void ParserParam(std::string &param,std::vector<string> &paraVec)
{
	std::replace(param.begin(),param.end(),'/',' ');
	istringstream isStream(param);
	std::string tmp;
	while(isStream>>tmp)
	{
		paraVec.push_back(tmp);
	}
}


bool CProxyServer::Init(unsigned short port)
{
	m_port = port;
	m_pListenProc = NULL;
	
	m_hProcCmdThreadID = PUB_CREATE_THREAD_FAIL;
	m_bProcCmdRun = false;

	m_hProcReplyThreadID = PUB_CREATE_THREAD_FAIL;
	m_bProcReplyRun = false;

	m_pMultiNetComm = NULL;

	m_netDevMan.Init();

	return true;
}


void CProxyServer::Quit()
{
	m_netDevMan.Quit();
}


bool CProxyServer::Start()
{
	if (!CDeviceSearcher::Instance().StartSearchThread())
	{
		goto ERROR_RET_PROC;
	}

	m_pMultiNetComm = new CSWL_MultiNetCommEx;
	if (!m_pMultiNetComm->Init())
	{
		goto ERROR_RET_PROC;
	}

	m_pListenProc = new CSWL_ListenProcEx(AcceptLinkCallback, this);
	if(NULL == m_pListenProc)
	{
		goto ERROR_RET_PROC;
	}

	if(m_pListenProc->StartListen(m_port) != 0)
	{
		goto ERROR_RET_PROC;
	}
	
	m_hProcCmdThreadID = PUB_CreateThread(ProcCmdThread,this,&m_bProcCmdRun);
	if (PUB_CREATE_THREAD_FAIL == m_hProcCmdThreadID)
	{
		goto ERROR_RET_PROC;
	}


	m_hProcReplyThreadID = PUB_CreateThread(ProcReplyThread,this,&m_bProcReplyRun);
	if (PUB_CREATE_THREAD_FAIL == m_bProcReplyRun)
	{
		goto ERROR_RET_PROC;
	}

	if (!m_netDevMan.Start())
	{
		goto ERROR_RET_PROC;
	}

	return true;

ERROR_RET_PROC:
	
	if (NULL != m_pMultiNetComm)
	{
		m_pMultiNetComm->Destroy();
		delete m_pMultiNetComm;
		m_pMultiNetComm =  NULL;
	}

	if (NULL != m_pListenProc)
	{
		m_pListenProc->StopListen();
		delete m_pListenProc;
		m_pListenProc = NULL;
	}

	if (PUB_CREATE_THREAD_FAIL != m_hProcCmdThreadID)
	{
		PUB_ExitThread(&m_hProcCmdThreadID,&m_bProcCmdRun);
		m_hProcCmdThreadID = PUB_CREATE_THREAD_FAIL;
	}

	if (PUB_CREATE_THREAD_FAIL != m_hProcReplyThreadID)
	{
		PUB_ExitThread(&m_hProcReplyThreadID,&m_bProcReplyRun);
		m_hProcReplyThreadID = PUB_CREATE_THREAD_FAIL;
	}
	
	m_netDevMan.Stop();

	CDeviceSearcher::Instance().StopSearchThread();

	return false;
}

void CProxyServer::Stop()
{
	if (PUB_CREATE_THREAD_FAIL != m_hProcCmdThreadID)
	{
		PUB_ExitThread(&m_hProcCmdThreadID,&m_bProcCmdRun);
		m_hProcCmdThreadID = PUB_CREATE_THREAD_FAIL;
	}

	if (PUB_CREATE_THREAD_FAIL != m_hProcReplyThreadID)
	{
		PUB_ExitThread(&m_hProcReplyThreadID,&m_bProcReplyRun);
		m_hProcReplyThreadID = PUB_CREATE_THREAD_FAIL;
	}

	if (NULL != m_pMultiNetComm)
	{
		m_pMultiNetComm->Destroy();
		delete m_pMultiNetComm;
		m_pMultiNetComm =  NULL;
	}

	if (NULL != m_pListenProc)
	{
		m_pListenProc->StopListen();
		delete m_pListenProc;
		m_pListenProc = NULL;
	}

	m_netDevMan.Stop();

	CDeviceSearcher::Instance().StopSearchThread();
}


unsigned long AllocateProxyClientID()
{
	static unsigned long s_clientID = 1;
	unsigned long newClientID = s_clientID++;

	//无符号长整数溢出，重新从1开始
	if(0 == s_clientID)
	{
		s_clientID = 1;
	}
	
	return newClientID;
}

void CProxyServer::OnAccept(CLIENT_SOCK_INFO *pClientSockInfo)
{
	HTTP_CLIENT_INFO *pClientInfo = new HTTP_CLIENT_INFO;
	memset(pClientInfo,0,sizeof(*pClientInfo));

	pClientInfo->sockfd = pClientSockInfo->tvtSock;
	pClientInfo->netSessionID = AllocateProxyClientID();
	
	pClientInfo->httpReq.buf = new char[WEB_SER_HTTP_REQ_LEN];
	pClientInfo->httpReq.bufLen = WEB_SER_HTTP_REQ_LEN;
	pClientInfo->httpReq.dataLen = 0;
	assert(NULL != pClientInfo->httpReq.buf);

	/*pClientInfo->httpRep.buf = new char[WEB_SER_HTTP_REP_LEN];
	pClientInfo->httpRep.bufLen = WEB_SER_HTTP_REP_LEN;
	pClientInfo->httpReq.dataLen = 0;
	assert(NULL != pClientInfo->httpRep.buf);*/

	m_pMultiNetComm->AddComm(pClientInfo->netSessionID,pClientInfo->sockfd);
	m_pMultiNetComm->SetRecvBufferLen(pClientInfo->netSessionID,WEB_SER_RECV_BUF_LEN);
	m_pMultiNetComm->SetAutoRecvCallback(pClientInfo->netSessionID,WebServerDataCallback,this);

	pClientInfo->lastTimeRecv = PUB_GetTickCount();
	pClientInfo->lastTimeSend = PUB_GetTickCount();

	std::cout<<"new client:"<<pClientInfo->netSessionID<<std::endl;
	AddClient(pClientInfo);
}

void CProxyServer::OnRecvData(long netSessionID, RECV_DATA_BUFFER *pDataBuffer)
{
	if (NULL == pDataBuffer)
	{
		std::cout<<"disconnect by client:"<<netSessionID<<std::endl;
		DelClient(netSessionID);
	}
	else
	{
		HTTP_CLIENT_INFO *pClient = GetClientRefer(netSessionID);
		
		if (NULL == pClient)
		{
			return;
		}

		long readPos = 0;
		HTTP_REQUEST_MSG &httpReq = pClient->httpReq;

		if (HTTP_STATE_HEAD == httpReq.recvState)
		{
			for (;readPos < pDataBuffer->dataSize; readPos++)
			{
				httpReq.buf[httpReq.dataLen++] = pDataBuffer->pData[readPos];
				
				assert(httpReq.dataLen < httpReq.bufLen);
				
				if ((httpReq.dataLen > 4) && (0 == strncmp(httpReq.buf+httpReq.dataLen-4,"\r\n\r\n",4)))
				{
					httpReq.headLen = httpReq.dataLen;
					httpReq.contextLen = atoi(GetField(httpReq.buf,"Content-Length").c_str());
					httpReq.recvState = (httpReq.contextLen == 0 ? HTTP_STATE_FINISH : HTTP_STATE_BODY);
					readPos++;
					break;
				}
			}
		}
			
		if (HTTP_STATE_BODY == httpReq.recvState)
		{
			assert((httpReq.contextLen+httpReq.headLen) < httpReq.bufLen);
			for (;readPos < pDataBuffer->dataSize; readPos++)
			{
				httpReq.buf[httpReq.dataLen++] = pDataBuffer->pData[readPos];
				if (httpReq.dataLen == (httpReq.contextLen+httpReq.headLen))
				{
					httpReq.recvState = HTTP_STATE_FINISH;
					break;
				}
			}
		}
		
		if (HTTP_STATE_FINISH == httpReq.recvState && readPos > 0)
		{
			pClient->httpReq.buf[pClient->httpReq.dataLen] = 0;
			std::cout<<pClient->httpReq.buf<<std::endl;
			pClient->lastTimeRecv = PUB_GetTickCount();

			m_httpReqLock.Lock();
			m_httpReqList.push_back(pClient->netSessionID);
			m_httpReqLock.UnLock();
		}
		
		pDataBuffer->dataSize -= readPos;
		if (pDataBuffer->dataSize != 0 && readPos != 0)
		{
			memmove(pDataBuffer->pData, pDataBuffer->pData + readPos, pDataBuffer->dataSize);
		}

		RelClientRefer(pClient->netSessionID);
	}
}


void CProxyServer::ProcCmd()
{
	HTTP_CLIENT_INFO *pClient = NULL;
	long netSessionID = 0;
	HTTP_RESPONSE_MSG *pMsg = NULL;
	std::list<long> httpReqList;

	while(m_bProcCmdRun)
	{
		m_httpReqLock.Lock();
		httpReqList.assign(m_httpReqList.begin(),m_httpReqList.end());
		m_httpReqList.clear();
		m_httpReqLock.UnLock();

		for (std::list<long>::iterator iter = httpReqList.begin(); iter != httpReqList.end(); ++iter)
		{
			pClient=GetClientRefer(*iter);

			if (pClient != NULL)
			{
				netSessionID = pClient->netSessionID;

				pMsg = new HTTP_RESPONSE_MSG;
				memset(pMsg,0,sizeof(HTTP_RESPONSE_MSG));

				pMsg->netSessionID = *iter;

				string result;
				HTTP_MSG_REQUEST msgType = ParserHttpRequest(pClient,result);

				switch(msgType)
				{
				case MSG_REQUEST_ADD_DEV:
					OnAddDev(pClient,result,pMsg);
					break;
				case MSG_REQUEST_DEL_DEV:
					OnDelDev(pClient,result,pMsg);
					break;
				case MSG_REQUEST_DOWNLOAD:
					OnDownLoad(pClient,result,pMsg);
					break;
				case MSG_REQUEST_REQ_M3U8:
					OnM3U8(pClient,result,pMsg);
					break;
				case MSG_REQUEST_START_LIVE:
					OnLive(pClient,result,pMsg);
					break;
				case MSG_REQUEST_CONFIG:
					OnConfig(pClient,result,pMsg);
					break;
				case MSG_REQUEST_LOCAL_DEVICES:
					OnGetAllDevice(pClient,result,pMsg);
					break;
				case MSG_REQUEST_DEV_INFO:
					OnGetDeviceInfo(pClient,result,pMsg);
					break;
				case MSG_REQUEST_DEV_BIND:
					OnBindDevOwner(pClient,result,pMsg);
					break;
				case MSG_REQUEST_CL_DEV:
					OnCleanDev(pClient,result,pMsg);
					break;
				case MSG_REQUEST_DEV_NET_STATUS:
					OnGetDeviceNetStatus(pClient,result,pMsg);
					break;
				case MSG_REQUEST_UPGRADE:
					OnUpGrade(pClient,result,pMsg);
					break;
				default:
					OnError(pClient,result,pMsg);
					break;
				}

				PutReplyMsg(pMsg);
			}
		
			RelClientRefer(netSessionID);
		}
	
		if (httpReqList.empty())
		{
			PUB_Sleep(1);
		}
	}
}

void CProxyServer::ProcReply()
{
	bool bIdle  = true;


	while(m_bProcReplyRun)
	{
		bIdle = true;

		bIdle |= SendMsg();
		
		bIdle |= SendStream();

		ClearDevice();
	
		if (bIdle)
		{
			PUB_Sleep(1);
		}
	}
}

CProxyServer::HTTP_MSG_REQUEST CProxyServer::ParserHttpRequest(HTTP_CLIENT_INFO *pClient,std::string &result)
{
	assert(NULL != pClient && NULL != pClient->httpReq.buf);
	char *pFirstLineEnd = strstr(pClient->httpReq.buf,"\r\n");
	HTTP_MSG_REQUEST ret = MSG_REQUEST_ERROR;

	if (NULL == pFirstLineEnd)
	{
		return ret;
	}
	
	std::string request(pClient->httpReq.buf,pFirstLineEnd);
	std::istringstream istring(request);
	std::vector<std::string> strVec;

	std::string word;
	while(istring>>word)
	{
		strVec.push_back(word);	
	}

	if (strVec.size() < 2)
	{
		return ret;
	}

	if (strVec[0] == "GET")
	{
		std::string::size_type pos = strVec[1].find_last_of('.');
		std::string reqStr;

		if (pos != std::string::npos)
		{
			reqStr = strVec[1].substr(pos);
		}

		if(pos == string::npos)
		{
			ret = MSG_REQUEST_ERROR;
		}
		else if(reqStr == ".m3u8")
		{
			ret = MSG_REQUEST_REQ_M3U8;
		}
		else if(reqStr == ".ts")
		{
			ret = MSG_REQUEST_START_LIVE;
		}
		else if (reqStr == ".mp4")
		{
			ret = MSG_REQUEST_DOWNLOAD;
		}
		else
		{
			ret = MSG_REQUEST_ERROR;
		}
	}
	else if(strVec[0] == "POST")
	{
		if (strVec[1] == "/dev/add")
		{
			ret = MSG_REQUEST_ADD_DEV;
		}
		else if(strVec[1] == "/dev/del")
		{
			ret = MSG_REQUEST_DEL_DEV;
		}
		else if(strVec[1] == "/dev/clean")
		{
			ret = MSG_REQUEST_CL_DEV;
		}
		else if (strVec[1] == "/devices")
		{
			ret = MSG_REQUEST_LOCAL_DEVICES;
		}
		else if(strVec[1] == "/upgrade")
		{
			ret = MSG_REQUEST_UPGRADE;
		}
		else
		{
			std::string::size_type pos = strVec[1].find_last_of('/');
			std::string reqStr;

			if (pos != std::string::npos)
			{
				reqStr = strVec[1].substr(pos);
			}

			if(pos == string::npos)
			{
				ret = MSG_REQUEST_ERROR;
			}
			else if (reqStr == "/config")
			{
				ret = MSG_REQUEST_CONFIG;
			}
			else if(reqStr == "/bind")
			{
				ret = MSG_REQUEST_DEV_BIND;
			}
			else if(reqStr == "/info")
			{
				ret = MSG_REQUEST_DEV_INFO;
			}
			else if(reqStr == "/net_status")
			{
				ret = MSG_REQUEST_DEV_NET_STATUS;
			}
			else
			{
				ret = MSG_REQUEST_ERROR;
			}	
		}
	}
	
	result = strVec[1];

	return ret;
}


void CProxyServer::BuildHttpResponse(int httpCode,std::vector<char> &context,HTTP_RESPONSE_MSG *pResponseMsg)
{
	std::ostringstream osStream;

	if (200 == httpCode)
	{
		osStream<<"HTTP/1.1 200 OK\r\n";	
	}
	else
	{
		osStream<<"HTTP/1.1 404 Not Found\r\n";
	}

	osStream<<"Content-Type: text/html\r\n"
		    <<"Content-Length: "<<context.size()<<"\r\n\r\n";

	std::string  headStr = osStream.str();
	pResponseMsg->dataLen = headStr.size()+context.size();
	pResponseMsg->buf = new char[pResponseMsg->dataLen];
	pResponseMsg->usePos = 0;
	memcpy(pResponseMsg->buf,headStr.c_str(),headStr.size());
	if (!context.empty())
	{
		memcpy(pResponseMsg->buf+headStr.size(),&context[0],context.size());
	}
}

void CProxyServer::OnAddDev(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg)
{	
	std::string req;
	req.assign(pClient->httpReq.buf+pClient->httpReq.headLen,pClient->httpReq.contextLen);
	
	Json::Reader reader;
	Json::Value root;

	Json::Value jsonReponse;

	do 
	{
		if (!reader.parse(req,root))
		{
			jsonReponse[PROXY_CODE] = PROXY_ERROR;
			jsonReponse[PROXY_DESC] = PROXY_PARSER_JSON_FAILED;

			break;
		}

		Json::Value &deviceList = root[PROXY_LIST];
		int devNum = deviceList.size();
		DEVICE_INFO devInfo;

		for (int i = 0; i < devNum; i++)
		{
			std::string sn		 = deviceList[i][PROXY_SN].asString();
			std::string username = deviceList[i][PROXY_USERNAME].asString();
			std::string password = deviceList[i][PROXY_PASSWORD].asString();
			std::string session	 = deviceList[i][PROXY_SESSION].asString();

			if (sn.empty() || username.empty() || (password.empty() && session.empty()))
			{
				continue;
			}
			
			memset(&devInfo,0,sizeof(devInfo));
			strncpy(devInfo.serialNo,sn.c_str(),sizeof(devInfo.serialNo)-1);
			strncpy(devInfo.userName,username.c_str(),sizeof(devInfo.userName)-1);
			
			if (!session.empty())
			{
				strncpy(devInfo.session,session.c_str(),sizeof(devInfo.session)-1);
			}
			if (!password.empty())
			{
				strncpy(devInfo.password,password.c_str(),sizeof(devInfo.password)-1);
			}

			if (deviceList[i].isMember(PROXY_BAIDU))
			{
				std::string channl_id = deviceList[i][PROXY_CHANNL_ID].asString();
				std::string user_id = deviceList[i][PROXY_USER_ID].asString();
				int device_type = deviceList[i][PROXY_MOBILE_TYPE].asInt();

				devInfo.pushInfo.platform = BAIDU_PLATFORM;
				strncpy(devInfo.pushInfo.baiduPlatForm.channel_id,channl_id.c_str(),sizeof(devInfo.pushInfo.baiduPlatForm.channel_id)-1);
				devInfo.pushInfo.baiduPlatForm.device_type = device_type;
				strncpy(devInfo.pushInfo.baiduPlatForm.user_id,user_id.c_str(),sizeof(devInfo.pushInfo.baiduPlatForm.user_id)-1);
			}
			else if(deviceList[i].isMember(PROXY_APPLE))
			{
				std::string token = deviceList[i][PROXY_TOKEN].asString();
				int device_type = deviceList[i][PROXY_MOBILE_TYPE].asInt();

				devInfo.pushInfo.platform = APPLE_PLATFORM;
				devInfo.pushInfo.applePlatform.device_type = device_type;
				strncpy(devInfo.pushInfo.applePlatform.token,token.c_str(),sizeof(devInfo.pushInfo.applePlatform.token)-1);
			}

			m_netDevMan.AddDevice(&devInfo);
		}
		
		jsonReponse[PROXY_CODE] = PROXY_OK;
		jsonReponse[PROXY_DESC] = PROXY_PARAM_NONE;
	} while (false);

    std::vector<char> response;
	response.resize(jsonReponse.toStyledString().size());
	memcpy(&response[0],jsonReponse.toStyledString().c_str(),response.size());
	
	BuildHttpResponse(200,response,pResponseMsg);
}

void CProxyServer::OnDelDev(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg)
{
	std::string req;
	req.assign(pClient->httpReq.buf+pClient->httpReq.headLen,pClient->httpReq.contextLen);

	Json::Value root;
	Json::Reader reader;

	Json::Value jsonReponse;

	do 
	{
		if (!reader.parse(req,root))
		{
			jsonReponse[PROXY_CODE] = PROXY_ERROR;
			jsonReponse[PROXY_DESC] = PROXY_PARSER_JSON_FAILED;
			break;
		}

		std::string sn = root[PROXY_SN].asString();
		if (sn.empty())
		{
			jsonReponse[PROXY_CODE] = PROXY_ERROR;
			jsonReponse[PROXY_DESC] = PROXY_PARAM_ERROR;
			break;
		}

		m_netDevMan.DelDevice(sn);


		jsonReponse[PROXY_CODE] = PROXY_OK;
		jsonReponse[PROXY_DESC] = PROXY_PARAM_NONE;
	} while (false);

	std::vector<char> response;
	response.resize(jsonReponse.toStyledString().size());
	memcpy(&response[0],jsonReponse.toStyledString().c_str(),response.size());
	
	BuildHttpResponse(200,response,pResponseMsg);
}


void CProxyServer::OnCleanDev(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg)
{
	m_netDevMan.DelAllDevice();

	Json::Value jsonReponse;
	jsonReponse[PROXY_CODE] = PROXY_OK;
	jsonReponse[PROXY_DESC] = PROXY_PARAM_NONE;

	std::vector<char> response;
	response.resize(jsonReponse.toStyledString().size());
	memcpy(&response[0],jsonReponse.toStyledString().c_str(),response.size());

	BuildHttpResponse(200,response,pResponseMsg);
}

void CProxyServer::OnDownLoad(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg)
{
	std::string ranges = GetField(pClient->httpReq.buf,"Range");
	std::vector<std::string> strVec;
	ParserParam(param,strVec);

	if ((strVec.size() != 3) || (!m_netDevMan.bOnLine(strVec[0])))
	{
		std::vector<char> response;
		response.resize(strlen(g_msg_404));
		memcpy(&response[0],g_msg_404,response.size());

		BuildHttpResponse(404,response,pResponseMsg);
		pResponseMsg->replyStatus = MSG_REPLY_DOWNLOAD_FAILED;
		return;
	}
	
	char *pHttpHead = new char[1024];
	memset(pHttpHead,0,1024);
	long fileLen = atoi(strVec[1].c_str());
	long beginPos = 0;
	long endPos = 0;

	if (ranges.empty())
	{
		sprintf(pHttpHead,g_msg_200,fileLen,"video/mp4");
		beginPos = 0;
		endPos = fileLen - 1;
	}
	else
	{
		long contentLen = 0;
		sscanf(ranges.c_str(),"bytes=%d-%d",&beginPos,&endPos);

		if (endPos == 0)
		{
			endPos = fileLen-1;
		}

		assert(endPos >= beginPos);
		contentLen = endPos - beginPos + 1;

		sprintf(pHttpHead,g_msg_206,"video/mp2t",contentLen,beginPos,endPos,fileLen);
	}

	pResponseMsg->dataLen = strlen(pHttpHead);
	pResponseMsg->buf = pHttpHead;
	pResponseMsg->usePos = 0;

	HTTP_STREAM *pHttpStream = new HTTP_STREAM;
	memset(pHttpStream,0,sizeof(HTTP_STREAM));

	pHttpStream->bLive = false;
	pHttpStream->beginpos = beginPos;
	pHttpStream->endpos = endPos;
	strncpy(pHttpStream->serialNo,strVec[0].c_str(),SHORT_STR_LEN);
	strncpy(pHttpStream->filename,strVec[2].c_str(),SHORT_STR_LEN);

	pResponseMsg->param = pHttpStream;
	pResponseMsg->replyStatus = MSG_REPLY_DOWNLOAD_SUC;
}


void CProxyServer::OnM3U8(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg)
{
	char *pHttpHead = new char[1024];
	const char *pm3u8 = g_m3u8_header;

	memset(pHttpHead,0,1024);

	sprintf(pHttpHead, g_msg_206,"application/x-mpegURL",strlen(pm3u8),0,strlen(pm3u8)-1,strlen(pm3u8));

	memcpy(pHttpHead+strlen(pHttpHead),pm3u8,strlen(pm3u8));	
	
	pResponseMsg->dataLen = strlen(pHttpHead);
	pResponseMsg->buf = pHttpHead;
	pResponseMsg->usePos = 0;

	pResponseMsg->replyStatus = MSG_REPLY_M3U8_SUC;
}

void CProxyServer::OnLive(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg)
{
	std::vector<std::string> strVec;
	ParserParam(param,strVec);
	
	if ((4 == strVec.size()) && m_netDevMan.bOnLine(strVec[0]) && ((strVec[2]==PROXY_MAIN_STREAM) || (strVec[2]==PROXY_SUB_STREAM)))
	{
		pResponseMsg->dataLen = strlen(g_msg_200_chunked);
		pResponseMsg->buf = new char[pResponseMsg->dataLen];
		pResponseMsg->usePos = 0;
		memcpy(pResponseMsg->buf,g_msg_200_chunked,pResponseMsg->dataLen);

		HTTP_STREAM *pHttpStream = new HTTP_STREAM;
		memset(pHttpStream,0,sizeof(HTTP_STREAM));

		pHttpStream->bLive = true;
		pHttpStream->bMainStream = (strVec[2]==PROXY_MAIN_STREAM);
		pHttpStream->channl = atoi(strVec[1].c_str());
		strncpy(pHttpStream->serialNo,strVec[0].c_str(),SHORT_STR_LEN);

		pResponseMsg->param = pHttpStream;
		pResponseMsg->replyStatus = MSG_REPLY_LIVE_SUC;
	}
	else
	{
		pResponseMsg->replyStatus = MSG_REPLY_LIVE_FAILED;

		std::vector<char> response;
		response.resize(strlen(PROXY_ERROR));
		memcpy(&response[0],PROXY_ERROR,response.size());
		
		BuildHttpResponse(404,response,pResponseMsg);
	}
}

void CProxyServer::OnConfig(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg)
{
	std::vector<char> req(pClient->httpReq.buf+pClient->httpReq.headLen,pClient->httpReq.buf+pClient->httpReq.headLen+pClient->httpReq.contextLen);

	std::vector<std::string> strVec;
	ParserParam(param,strVec);

	Json::Value jsonReponse;
	 
	if (strVec.size() != 2)
	{
		jsonReponse[PROXY_CODE] = PROXY_ERROR;
		jsonReponse[PROXY_DESC] = PROXY_PARAM_ERROR;
       
		std::vector<char> response;
		response.resize(jsonReponse.toStyledString().size());
		memcpy(&response[0],jsonReponse.toStyledString().c_str(),response.size());
		BuildHttpResponse(200,response,pResponseMsg);

		return;
	}

	if(!m_netDevMan.bOnLine(strVec[0]))
	{
		jsonReponse[PROXY_CODE] = PROXY_ERROR;
		jsonReponse[PROXY_DESC] = PROXY_NOT_ONLINE;

		std::vector<char> response;
		response.resize(jsonReponse.toStyledString().size());
		memcpy(&response[0],jsonReponse.toStyledString().c_str(),response.size());
		BuildHttpResponse(200,response,pResponseMsg);

		return;
	}
	

	jsonReponse[PROXY_CODE] = PROXY_ERROR;
	jsonReponse[PROXY_DESC] = PROXY_UNKNOWN_ERROR;

	std::vector<char> response;
	response.resize(jsonReponse.toStyledString().size());
	memcpy(&response[0],jsonReponse.toStyledString().c_str(),response.size());
	m_netDevMan.Config(strVec[0],req,response);
	BuildHttpResponse(200,response,pResponseMsg);
}

void CProxyServer::OnError(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg)
{
	std::vector<char> response;
	response.resize(strlen(g_msg_404));
	memcpy(&response[0],g_msg_404,response.size());
	BuildHttpResponse(404,response,pResponseMsg);
}

void CProxyServer::OnGetAllDevice(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg)
{
	TVT_DEV_LIST devList;
	m_netDevMan.GetAllLocalDevice(devList);
	
	Json::Value devArr;
	for (TVT_DEV_LIST::iterator iter = devList.begin(); iter != devList.end(); ++iter)
	{
		Json::Value dev;
		dev[MULTICAST_IPV4_ADDR] = (*iter).ip;
		dev[MULTICAST_IPV4_SUBNETMASK] = (*iter).netmask;
		dev[MULTICAST_IPV4_GATEWAY] = (*iter).gateway;
		dev[MULTICAST_DATA_PORT] = (*iter).port;
		dev[MULTICAST_MAC_ADDR] = (*iter).mac;
		dev[MULTICAST_DEV_SN] = (*iter).sn;
		dev[MULTICAST_DEV_OWNER] = (*iter).owner;
		dev[MULTICAST_DEV_TYPE] = (*iter).devType;
		dev[MULTICAST_DEV_VERSION] = (*iter).version;
		dev[MULTICAST_BUILD_DATE] = (*iter).buildDate;

		devArr.append(dev);
	}
	
	Json::Value jsonReponse;
	jsonReponse[PROXY_CODE] = PROXY_OK;
	jsonReponse[PROXY_DESC] = PROXY_PARAM_NONE;
	jsonReponse[PROXY_DATA] =  devArr;

	std::vector<char> response;
	response.resize(jsonReponse.toStyledString().size());
	memcpy(&response[0],jsonReponse.toStyledString().c_str(),response.size());
	BuildHttpResponse(200,response,pResponseMsg);
}

void CProxyServer::OnGetDeviceInfo(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg)
{
	std::vector<std::string> strVec;
	ParserParam(param,strVec);

	Json::Value dev;
	dev[PROXY_DEV_STATUS] = PROXY_DEV_NONE;

	if ((2 == strVec.size()))
	{		
		if (m_netDevMan.bOnLine(strVec[0]))
		{
			dev[PROXY_DEV_STATUS] = PROXY_DEV_ONLINE;
		}
		else if(m_netDevMan.bAdd(strVec[0]))
		{
			dev[PROXY_DEV_STATUS] = PROXY_DEV_OFFLINE;
		}

		TVT_DISCOVER_DEVICE devInfo;
		if (m_netDevMan.GetLocalDevice(strVec[0],devInfo))
		{
			dev[MULTICAST_IPV4_ADDR] = devInfo.ip;
			dev[MULTICAST_IPV4_GATEWAY] = devInfo.gateway;
			dev[MULTICAST_DATA_PORT] = devInfo.port;
			dev[MULTICAST_MAC_ADDR] = devInfo.mac;
			dev[MULTICAST_DEV_SN] = devInfo.sn;
			dev[MULTICAST_DEV_OWNER] = devInfo.owner;
			dev[MULTICAST_DEV_TYPE] = devInfo.devType;
			dev[MULTICAST_BUILD_DATE] = devInfo.buildDate;
			dev[MULTICAST_DEV_VERSION] = devInfo.version;
		}
	}

	Json::Value jsonReponse;
	jsonReponse[PROXY_CODE] = PROXY_OK;
	jsonReponse[PROXY_DESC] = PROXY_PARAM_NONE;
	jsonReponse[PROXY_DATA] =  dev;
	

	std::vector<char> response;
	response.resize(jsonReponse.toStyledString().size());
	memcpy(&response[0],jsonReponse.toStyledString().c_str(),response.size());
	BuildHttpResponse(200,response,pResponseMsg);
}

void CProxyServer::OnGetDeviceNetStatus(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg)
{
	std::vector<std::string> strVec;
	ParserParam(param,strVec);
	Json::Value dev;
	
	dev[PROXY_DEV_STATUS] = PROXY_DEV_OFFLINE;

	if (m_netDevMan.bReachable(strVec[0]))
	{
		dev[PROXY_DEV_STATUS] = PROXY_DEV_ONLINE;
	}

	Json::Value jsonReponse;
	jsonReponse[PROXY_CODE] = PROXY_OK;
	jsonReponse[PROXY_DESC] = PROXY_PARAM_NONE;
	jsonReponse[PROXY_DATA] =  dev;

	std::vector<char> response;
	response.resize(jsonReponse.toStyledString().size());
	memcpy(&response[0],jsonReponse.toStyledString().c_str(),response.size());
	BuildHttpResponse(200,response,pResponseMsg);
}

void CProxyServer::OnBindDevOwner(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg)
{
	std::string req;
	req.assign(pClient->httpReq.buf+pClient->httpReq.headLen,pClient->httpReq.contextLen);

	std::vector<std::string> strVec;
	ParserParam(param,strVec);

	Json::Value jsonReponse;

	if (strVec.size() != 2)
	{
		jsonReponse[PROXY_CODE] = PROXY_ERROR;
		jsonReponse[PROXY_DESC] = PROXY_PARAM_ERROR;
		std::vector<char> response;
		response.resize(jsonReponse.toStyledString().size());
		memcpy(&response[0],jsonReponse.toStyledString().c_str(),response.size());
		BuildHttpResponse(200,response,pResponseMsg);

		return;
	}

	if(m_netDevMan.BindUser(strVec[0],req))
	{
		jsonReponse[PROXY_CODE] = PROXY_OK;
		jsonReponse[PROXY_DESC] = PROXY_PARAM_NONE;

		std::vector<char> response;
		response.resize(jsonReponse.toStyledString().size());
		memcpy(&response[0],jsonReponse.toStyledString().c_str(),response.size());
		BuildHttpResponse(200,response,pResponseMsg);

		return;
	}


	jsonReponse[PROXY_CODE] = PROXY_ERROR;
	jsonReponse[PROXY_DESC] = PROXY_BIND_DEVICE_FAILED;
	std::vector<char> response;
	response.resize(jsonReponse.toStyledString().size());
	memcpy(&response[0],jsonReponse.toStyledString().c_str(),response.size());
	BuildHttpResponse(200,response,pResponseMsg);
}


void CProxyServer::OnUpGrade(HTTP_CLIENT_INFO *pClient,std::string param,HTTP_RESPONSE_MSG *pResponseMsg)
{
	std::string req;
	req.assign(pClient->httpReq.buf+pClient->httpReq.headLen,pClient->httpReq.contextLen);

	Json::Reader reader;
	Json::Value val;

	Json::Value jsonReponse;

	if (reader.parse(req,val))
	{
		std::string sn = val[PROXY_DATA][PROXY_SN].asString();
		std::string param = val[PROXY_DATA][MULTICAST_UPGRADE_PARAM].asString();
		m_netDevMan.UpGradeLocalDevice(sn,param);
	
		jsonReponse[PROXY_CODE] = PROXY_OK;
	}
	else
	{
		jsonReponse[PROXY_CODE] = PROXY_ERROR;
		jsonReponse[PROXY_DESC] = PROXY_PARAM_ERROR;
	}

	std::vector<char> response;
	response.resize(jsonReponse.toStyledString().size());
	memcpy(&response[0],jsonReponse.toStyledString().c_str(),response.size());
	BuildHttpResponse(200,response,pResponseMsg);
}

bool CProxyServer::SendMsg()
{
	bool bIdle = true;
	std::set<long> netInfoList;
	
	GetAllClient(netInfoList);

	if (netInfoList.empty())
	{
		return bIdle;
	}

	HTTP_CLIENT_INFO *pClient = NULL;
	HTTP_RESPONSE_MSG *pMsg = NULL;
	
	for (std::set<long>::iterator iter = netInfoList.begin(); iter != netInfoList.end(); ++iter)
	{
		pClient = GetClientRefer((*iter));
		if (NULL == pClient)
		{
			continue;
		}

		//假如没有数据，就从消息列表里面拿消息
		if (pClient->httpRep.dataLen <= 0)
		{
			pMsg = GetReplyMsg(pClient->netSessionID);
			
			if (pMsg != NULL)
			{
				if (NULL != pClient->httpRep.buf)
				{
					delete [] pClient->httpRep.buf;
					pClient->httpRep.buf = NULL;
				}

				pClient->httpRep.buf = pMsg->buf;
				pClient->httpRep.dataLen = pMsg->dataLen;
				pClient->httpRep.bufLen = pMsg->dataLen;
				pClient->httpRep.replyStatus = pMsg->replyStatus;
				pClient->httpRep.usePos = 0;
				pClient->httpRep.param = pMsg->param;
			}

			if (NULL != pMsg)
			{
				delete pMsg;
			}
		}
	
		//假如没有消息，发送下一个客户端的数据
		if (pClient->httpRep.dataLen <= 0)
		{
			RelClientRefer(pClient);
			continue;
		}
		
		//假如有数据，则发送
		long sendLen = SendData(pClient);

		//假如客户端断开，就删除
		if (sendLen < 0)
		{
			std::cout<<"net error:"<<pClient->netSessionID<<std::endl;
			RelClientRefer(pClient->netSessionID);
			DelClient(pClient->netSessionID);
			continue;
		}

		bIdle |= (sendLen > 0);
		
		pClient->httpRep.usePos += sendLen;
		if (pClient->httpRep.usePos < pClient->httpRep.dataLen)
		{
			RelClientRefer(pClient);
			continue;
		}

		if (pClient->httpRep.usePos > pClient->httpRep.dataLen)
		{
			assert(false);
			std::cout<<"send error:"<<pClient->netSessionID<<std::endl;
			RelClientRefer(pClient->netSessionID);
			DelClient(pClient->netSessionID);
			continue;
		}


		//消息发送完成，做一些处理
		pClient->httpRep.dataLen = 0;
		pClient->httpRep.usePos = 0;
	
		if (MSG_REPLY_STREAM != pClient->httpRep.replyStatus)
		{
			std::cout<<pClient->httpRep.buf<<std::endl;
		}


		switch(pClient->httpRep.replyStatus)
		{
		case MSG_REPLY_LIVE_SUC:
		case MSG_REPLY_DOWNLOAD_SUC:
			pClient->httpReq.recvState = HTTP_STATE_HEAD;
			AddStream((HTTP_STREAM *)pClient->httpRep.param,pClient->netSessionID);
			RelClientRefer(pClient);
			break;
		case MSG_REPLY_STREAM:
			pClient->httpReq.recvState = HTTP_STATE_HEAD;
			RelClientRefer(pClient);
			break;
		case MSG_REPLY_M3U8_SUC:
			pClient->httpReq.recvState = HTTP_STATE_HEAD;
			RelClientRefer(pClient);
			break;
		case MSG_REPLY_ERROR:
		case MSG_REPLY_ADD_DEV:
		case MSG_REPLY_M3U8_FAILED:
		case MSG_REPLY_LIVE_FAILED:
		case MSG_REPLY_DOWNLOAD_FAILED:
		default:
			std::cout<<"disconnect by proxy:"<<pClient->netSessionID<<std::endl;
			RelClientRefer(pClient);
			DelClient(pClient->netSessionID);
			break;
		}
	}

	return bIdle;
}

bool CProxyServer::SendStream()
{
	bool bIdle = true;

	HTTP_STREAM_SEND_INFO *pSendInfo = NULL;

	for (HTTP_STREAM_SEND_INFO_LIST::iterator sendStreamIter = m_streamList.begin(); sendStreamIter != m_streamList.end(); )
	{
		pSendInfo = (*sendStreamIter);

		//没有客户端请求这个流，关闭流
		HTTP_STREAM_SEND_STATUS_LIST::iterator sendStatusIter = pSendInfo->statusList.begin();
		HTTP_CLIENT_INFO *pClient = NULL;
		while(sendStatusIter != pSendInfo->statusList.end())
		{
			pClient = GetClientRefer((*sendStatusIter).netSessionID);
			if (NULL == pClient)
			{
				sendStatusIter = pSendInfo->statusList.erase(sendStatusIter);
				continue;
			}
			
			RelClientRefer(pClient);
			sendStatusIter++;
		}
		
		if (pSendInfo->statusList.empty())
		{
			if (pSendInfo->bLive)
			{
				m_netDevMan.StopLiveChannel(pSendInfo->serialNo,pSendInfo->channl,pSendInfo->bMainStream);
			}
			else
			{
				m_netDevMan.StopDownload(pSendInfo->serialNo);
			}
		
			pSendInfo->statusList.clear();
			delete pSendInfo;pSendInfo = NULL;

			sendStreamIter = m_streamList.erase(sendStreamIter);
			continue;
		}
		

		//数据发送完毕就重新获取
		if (NULL == pSendInfo->pData || 0 == pSendInfo->dataLen)
		{
			if (m_netDevMan.GetDataBlock(pSendInfo->serialNo,pSendInfo->channl,pSendInfo->bLive,pSendInfo->bMainStream,&(pSendInfo->pData),&(pSendInfo->dataLen)))
			{
				for (HTTP_STREAM_SEND_STATUS_LIST::iterator sIter = pSendInfo->statusList.begin();sIter != pSendInfo->statusList.end();++sIter)
				{
					(*sIter).bSended = false;
				}
			}
		}

		if (NULL == pSendInfo->pData || 0 == pSendInfo->dataLen)
		{
			sendStreamIter++;
			continue;
		}


		//发送流数据
		sendStatusIter = pSendInfo->statusList.begin();
		pClient = NULL;
		while(sendStatusIter != pSendInfo->statusList.end())
		{
			pClient = GetClientRefer((*sendStatusIter).netSessionID);
			if (NULL == pClient)
			{
				sendStatusIter = pSendInfo->statusList.erase(sendStatusIter);
				continue;
			}

			if (!((*sendStatusIter).bSended) && (0==pClient->httpRep.dataLen))
			{
				PackStream(pClient,pSendInfo->pData,pSendInfo->dataLen,pSendInfo->bLive);

				(*sendStatusIter).bSended = true;
			}
		
			if (pClient->httpRep.dataLen <= 0)
			{
				sendStatusIter++;
				RelClientRefer(pClient);
				continue;
			}

			long sendLen = SendData(pClient);
			if (sendLen < 0)
			{
				std::cout<<"net error:"<<pClient->netSessionID<<std::endl;
				RelClientRefer(pClient);
				DelClient(pClient->netSessionID);
				sendStatusIter++;
				continue;
			}


			pClient->httpRep.usePos += sendLen;
			if (pClient->httpRep.usePos == pClient->httpRep.dataLen)
			{
				pClient->httpRep.dataLen = 0;
				pClient->httpRep.usePos = 0;
			}
			RelClientRefer(pClient);

			bIdle = (sendLen > 0);
			
			sendStatusIter++;
		}
			
		//如果发送完流数据，释放当前块
		bool bAllSended = true;
		sendStatusIter = pSendInfo->statusList.begin();
		while (sendStatusIter != pSendInfo->statusList.end())
		{
			if (!(*sendStatusIter).bSended)
			{
				bAllSended = false;
				break;
			}
		
			sendStatusIter++;
		}

		if (bAllSended)
		{
			m_netDevMan.ReleaseDataBlock(pSendInfo->serialNo,pSendInfo->channl,pSendInfo->bLive,pSendInfo->bMainStream,pSendInfo->pData,pSendInfo->dataLen);
			pSendInfo->pData = NULL;
			pSendInfo->dataLen = 0;
		}
		else
		{
			//发送另外一个流的数据
			sendStreamIter++;
		}
		
	}

	return bIdle;
}


void CProxyServer::ClearDevice()
{
	std::set<long> netInfoListTemp;
	GetAllClient(netInfoListTemp);
    std::list<long> netInfoList(netInfoListTemp.begin(),netInfoListTemp.end());

	unsigned long curTime = PUB_GetTickCount();
	HTTP_CLIENT_INFO *pClient = NULL;
	
	bool bRecvTimeOut = false;
	bool bSendTimeOut = false;

	for (std::list<long>::iterator iter = netInfoList.begin(); iter != netInfoList.end();)
	{
		pClient = GetClientRefer((*iter));
		if (NULL == pClient)
		{
			 ++iter;
			continue;
		}

		bRecvTimeOut = false;
		bSendTimeOut = false;

		if ((curTime > pClient->lastTimeRecv) && ((curTime - pClient->lastTimeRecv) > MAX_DEVICE_TIME_OUT))
		{
			bRecvTimeOut = true;
		}
	
		if ((curTime > pClient->lastTimeSend) && ((curTime - pClient->lastTimeSend) > MAX_DEVICE_TIME_OUT))
		{
			bSendTimeOut = true;
		}

	
		if (bRecvTimeOut && bSendTimeOut)
		{
			std::cout<<"client timeout:"<<pClient->netSessionID<<std::endl;
			RelClientRefer(pClient);
			DelClient(pClient->netSessionID);
			iter = netInfoList.erase(iter); 
		}
		else
		{
			RelClientRefer(pClient);
			++iter;
		}
	}
}


void CProxyServer::GetAllClient(std::set<long> &netInfoList)
{
	netInfoList.clear();
	m_clientLock.Lock();
	for (HTTP_CLIENT_INFO_LIST::iterator iter = m_clientList.begin(); iter != m_clientList.end(); ++iter)
	{
		netInfoList.insert((*iter)->netSessionID);
	}
	m_clientLock.UnLock();
}

void CProxyServer::AddClient(HTTP_CLIENT_INFO *pClient)
{
	m_clientLock.Lock();
	m_clientList.push_back(pClient);
	m_clientLock.UnLock();
}

CProxyServer::HTTP_CLIENT_INFO* CProxyServer::GetClientRefer(long netSessionID)
{
	HTTP_CLIENT_INFO *pClient = NULL;

	m_clientLock.Lock();

	for (HTTP_CLIENT_INFO_LIST::iterator iter = m_clientList.begin(); iter != m_clientList.end(); ++iter)
	{
		if (netSessionID == (*iter)->netSessionID)
		{
			pClient = (*iter);
			pClient->count++;
			break;
		}
	}

	m_clientLock.UnLock();

	return pClient;
}

void CProxyServer::RelClientRefer(long netSessionID)
{
	m_clientLock.Lock();

	for (HTTP_CLIENT_INFO_LIST::iterator iter = m_clientList.begin(); iter != m_clientList.end(); ++iter)
	{
		if (netSessionID == (*iter)->netSessionID)
		{
			(*iter)->count--;
			assert((*iter)->count >= 0);
			break;
		}
	}

	m_clientLock.UnLock();
}

void CProxyServer::RelClientRefer(HTTP_CLIENT_INFO *pClient)
{
	if (NULL == pClient)
	{
		return;
	}

	RelClientRefer(pClient->netSessionID);
}

void CProxyServer::DelClient(long netSessionID)
{
	HTTP_CLIENT_INFO *pClientInfo = NULL;

	while(true)
	{
		pClientInfo = NULL;

		m_clientLock.Lock();
		for (HTTP_CLIENT_INFO_LIST::iterator iter = m_clientList.begin(); iter != m_clientList.end(); ++iter)
		{
			if (netSessionID == (*iter)->netSessionID)
			{
				pClientInfo = (*iter);
				break;
			}
		}
	
		if (NULL == pClientInfo)
		{
			m_clientLock.UnLock();
			break;
		}
		
		if (0 == pClientInfo->count )
		{
			m_clientList.remove(pClientInfo);	
			m_clientLock.UnLock();
			break;
		}
		
		m_clientLock.UnLock();
		PUB_Sleep(1);
	}

	if (NULL != pClientInfo)
	{
		DelMsg(netSessionID);

		if (NULL != pClientInfo->httpReq.buf)
		{
			delete [] pClientInfo->httpReq.buf;
		}

		if (NULL != pClientInfo->httpRep.buf)
		{
			delete [] pClientInfo->httpRep.buf;
		}

		SWL_CloseSocket(pClientInfo->sockfd.swlSocket);
		m_pMultiNetComm->RemoveComm(pClientInfo->netSessionID);

		delete  pClientInfo;
	}
}


void CProxyServer::PutReplyMsg(HTTP_RESPONSE_MSG * pMsg)
{
	m_responseLock.Lock();
	m_responseList.push_back(pMsg);
	m_responseLock.UnLock();
}

CProxyServer::HTTP_RESPONSE_MSG * CProxyServer::GetReplyMsg(long netSessionID)
{
	HTTP_RESPONSE_MSG *pMsg = NULL;

	m_responseLock.Lock();
	for(HTTP_RESPONSE_MSG_LIST::iterator iter = m_responseList.begin(); iter != m_responseList.end(); ++iter)
	{
		if ((*iter)->netSessionID == netSessionID)
		{
			pMsg = (*iter);
			m_responseList.erase(iter);
			break;
		}
	}
	m_responseLock.UnLock();

	return pMsg;
}


void CProxyServer::DelMsg(long netSessionID)
{
	m_responseLock.Lock();
	for(HTTP_RESPONSE_MSG_LIST::iterator iter = m_responseList.begin(); iter != m_responseList.end();)
	{
		if ((*iter)->netSessionID == netSessionID)
		{
		
			if ((MSG_REPLY_LIVE_SUC == (*iter)->replyStatus) || (MSG_REPLY_DOWNLOAD_SUC == (*iter)->replyStatus))
			{
				assert(NULL != (*iter)->param);
				delete (HTTP_STREAM *)((*iter)->param);
			}

			delete (*iter);

			iter = m_responseList.erase(iter);
			continue;
		}
		
		++iter;
	}
	m_responseLock.UnLock();
}


long CProxyServer::SendData(HTTP_CLIENT_INFO *pClient)
{
	long deviceID = pClient->netSessionID;
	char *pData = pClient->httpRep.buf + pClient->httpRep.usePos;
	long dataLen = pClient->httpRep.dataLen - pClient->httpRep.usePos;

	long sendLen = m_pMultiNetComm->SendData(deviceID,pData,dataLen,false);
	
	if (sendLen > 0)
	{
		pClient->lastTimeSend = PUB_GetTickCount();
	}

	return sendLen;
}

void CProxyServer::AddStream(HTTP_STREAM *pHttpStream,long netSessionID)
{
	assert(NULL != pHttpStream);
	
	if (pHttpStream->bLive)
	{
		HTTP_STREAM_SEND_INFO *pStreamInfo = NULL;
		HTTP_STREAM_SEND_INFO_LIST::iterator iter = m_streamList.begin();

		for (; iter != m_streamList.end(); ++iter)
		{
			if ((0 == strcmp((*iter)->serialNo,pHttpStream->serialNo))
				&& ((*iter)->channl == pHttpStream->channl)
				&& ((*iter)->bMainStream == pHttpStream->bMainStream))
			{
				pStreamInfo = (*iter);
				break;
			}
		}

		if (NULL == pStreamInfo)
		{
			pStreamInfo = new HTTP_STREAM_SEND_INFO;

			pStreamInfo->bLive = pHttpStream->bLive;
			pStreamInfo->channl = pHttpStream->channl;
			pStreamInfo->bMainStream = pHttpStream->bMainStream;
			pStreamInfo->pData = NULL;
			pStreamInfo->dataLen = 0;
			memset(pStreamInfo->serialNo,0,sizeof(pStreamInfo->serialNo));
			strcpy(pStreamInfo->serialNo,pHttpStream->serialNo);
			m_streamList.push_back(pStreamInfo);

			m_netDevMan.StartLiveChannel(pStreamInfo->serialNo,pStreamInfo->channl,pStreamInfo->bMainStream);
		}

		HTTP_STREAM_SEND_STATUS sendStatus;
		sendStatus.bSended = true;
		sendStatus.netSessionID = netSessionID;

		pStreamInfo->statusList.push_back(sendStatus);
	}
	else
	{
		HTTP_STREAM_SEND_INFO *pStreamInfo = NULL;
		HTTP_STREAM_SEND_INFO_LIST::iterator iter = m_streamList.begin();

		for (; iter != m_streamList.end(); ++iter)
		{
			if ((0 == strcmp((*iter)->serialNo,pHttpStream->serialNo))
			 && (0 == strcmp((*iter)->fileName,pHttpStream->filename)))
			{
				pStreamInfo = (*iter);
				break;
			}
		}
	
		if (pStreamInfo != NULL)
		{
			m_netDevMan.StopDownload(pStreamInfo->serialNo);
			m_netDevMan.StartDownload(pStreamInfo->serialNo,pStreamInfo->fileName,pHttpStream->beginpos,pHttpStream->endpos);

			pStreamInfo->bLive = false;
			pStreamInfo->bMainStream = false;
			pStreamInfo->statusList.clear();
			pStreamInfo->pData = 0;
			pStreamInfo->dataLen = 0;
			pStreamInfo->channl = 0;

			HTTP_STREAM_SEND_STATUS sendStatus;
			sendStatus.bSended = true;
			sendStatus.netSessionID = netSessionID;

			pStreamInfo->statusList.push_back(sendStatus);
		}
		else
		{
			pStreamInfo = new HTTP_STREAM_SEND_INFO;

			pStreamInfo->bLive = false;
			pStreamInfo->pData = NULL;
			pStreamInfo->dataLen = 0;
			memset(pStreamInfo->serialNo,0,sizeof(pStreamInfo->serialNo));
			strncpy(pStreamInfo->serialNo,pHttpStream->serialNo,SHORT_STR_LEN);
			memset(pStreamInfo->fileName,0,sizeof(pStreamInfo->fileName));
			strncpy(pStreamInfo->fileName,pHttpStream->filename,SHORT_STR_LEN);

			m_streamList.push_back(pStreamInfo);

			HTTP_STREAM_SEND_STATUS sendStatus;
			sendStatus.bSended = true;
			sendStatus.netSessionID = netSessionID;

			pStreamInfo->statusList.push_back(sendStatus);
			pStreamInfo->channl = 0;

			m_netDevMan.StopDownload(pStreamInfo->serialNo);
			m_netDevMan.StartDownload(pStreamInfo->serialNo,pStreamInfo->fileName,pHttpStream->beginpos,pHttpStream->endpos);
		}
	}

	delete pHttpStream;
}

void CProxyServer::PackStream(HTTP_CLIENT_INFO *pClient,char *pData,long len,bool bLive)
{
	if (bLive)
	{
		char tmp[10] = {0};
		sprintf(tmp,"%X\r\n",len);
		int tmpLen = strlen(tmp);

		if (pClient->httpRep.bufLen < (tmpLen + 2 + len))
		{
			delete [] pClient->httpRep.buf;
			pClient->httpRep.buf = new char[tmpLen + 2 + len];
			pClient->httpRep.bufLen = tmpLen + 2 + len;
			assert(NULL != pClient->httpRep.buf);
		}

		memcpy(pClient->httpRep.buf,tmp,tmpLen);
		memcpy(pClient->httpRep.buf+tmpLen,pData,len);
		memcpy(pClient->httpRep.buf+tmpLen+len,"\r\n",2);
		pClient->httpRep.replyStatus = MSG_REPLY_STREAM;
		pClient->httpRep.dataLen = tmpLen + 2 + len;
		pClient->httpRep.usePos = 0;
	}
	else
	{
		if (pClient->httpRep.bufLen < len)
		{
			delete [] pClient->httpRep.buf;
			pClient->httpRep.buf = new char[len];
			pClient->httpRep.bufLen = len;
			assert(NULL != pClient->httpRep.buf);
		}

		memcpy(pClient->httpRep.buf,pData,len);
		pClient->httpRep.replyStatus = MSG_REPLY_STREAM;
		pClient->httpRep.dataLen = len;
		pClient->httpRep.usePos = 0;
	}
}
