#include "netdevice.h"
#include "json.h"
#include "CryptRsa.h"
#include <iostream>

CNetDevice::CNetDevice()
{
	m_pNetHandle = NULL;
	m_deviceID = 0;

	m_netStatus = NET_STATUS_LOGOUT;
	m_pDataCallBack = NULL;
	m_pUserData = NULL;

	m_lastRecvPackIndexmap.clear();
	m_lastSendPackIndexmap.clear();


	m_sendBuf.buf = new char [PACKET_MAX_SIZE];
	m_sendBuf.bufLen = PACKET_MAX_SIZE;
	m_sendBuf.dataLen = 0;


	m_bLoging = false;
	m_lastTick = 0;

	m_bNeedFirstFrame = false;
}

CNetDevice::~CNetDevice()
{
	SetNetHandle(NULL,0);	
	m_netStatus = NET_STATUS_LOGOUT;
}

bool CNetDevice::BindUser(std::string &info)
{
	m_bLoging = true;

	if (!bAgreeMent())
	{
		return false;
	}

	REPLY reply;
	reply = SendCmd(PACKAGE_TYPE_MSG,info.c_str(),info.size());

	m_bLoging = false;

	if (NULL == reply->buf)
	{
		return false;
	}

	Json::Reader reader;
	Json::Value rp;

	if (!reader.parse(reply->buf,rp))
	{
		return false;
	}

	return rp[PROXY_CODE].asString() == PROXY_OK;
}

bool CNetDevice::Login(DEVICE_INFO *pDeviceInfo)
{
	m_bLoging = true;

	if (!bAgreeMent())
	{
		return false;
	}

	Json::Value root;
	Json::Value data;
	Json::Value pushMsg;

	data[PROXY_USERNAME] = pDeviceInfo->userName;
	data[PROXY_PASSWORD] = pDeviceInfo->password;
	data[PROXY_SESSION] = pDeviceInfo->session;

	if (APPLE_PLATFORM == pDeviceInfo->pushInfo.platform)
	{
		pushMsg[PROXY_TOKEN] = pDeviceInfo->pushInfo.applePlatform.token;
		pushMsg[PROXY_MOBILE_TYPE] = pDeviceInfo->pushInfo.applePlatform.device_type;

		data[PROXY_APPLE] = pushMsg;
	}
	else if(BAIDU_PLATFORM == pDeviceInfo->pushInfo.platform)
	{
		pushMsg[PROXY_USER_ID] = pDeviceInfo->pushInfo.baiduPlatForm.user_id;
		pushMsg[PROXY_CHANNL_ID] = pDeviceInfo->pushInfo.baiduPlatForm.channel_id;
		pushMsg[PROXY_MOBILE_TYPE] = pDeviceInfo->pushInfo.baiduPlatForm.device_type;
		
		data[PROXY_BAIDU] = pushMsg;
	}

	root[PROXY_CMD] = PROXY_AUTH;
	root[PROXY_DATA] = data;

	REPLY reply;
	reply = SendCmd(PACKAGE_TYPE_MSG,root.toStyledString().c_str(),root.toStyledString().size());
	
	m_bLoging = false;

	if (NULL == reply->buf)
	{
		return false;
	}

	Json::Reader reader;
	Json::Value rp;

	if (!reader.parse(reply->buf,rp))
	{
		return false;
	}
	
	if (rp[PROXY_CODE].asString() == PROXY_OK)
	{
		m_netStatus = NET_STATUS_LOGIN;
	}

	return rp[PROXY_CODE].asString() == PROXY_OK;
}

void CNetDevice::Logout()
{
	SetNetHandle(NULL,0);	

	m_pNetHandle = NULL;
	m_deviceID = 0;

	m_netStatus = NET_STATUS_LOGOUT;
	m_pDataCallBack = NULL;
	m_pUserData = NULL;

	m_lastRecvPackIndexmap.clear();
	m_lastSendPackIndexmap.clear();

	m_lastTick = 0;

	m_recvBufMap.clear();
	m_sendBuf.dataLen = 0;
	m_msgBuf.dataLen = 0;
	m_encryptBuf.dataLen = 0;
	m_decryptBuf.dataLen = 0;

	m_bLoging = false;
	
	m_bNeedFirstFrame = false;
}

bool CNetDevice::Config(std::vector<char> &inParam,std::vector<char> &outParam)
{
	REPLY reply;
	reply = SendCmd(PACKAGE_TYPE_MSG,&inParam[0],inParam.size());
	
	outParam.clear();

	if (NULL == reply->buf)
	{
		return false;
	}
	
	if (reply->dataLen > 0)
	{
		outParam.resize(reply->dataLen);
		memcpy(&outParam[0],reply->buf,reply->dataLen);
	}
	

	return true;
}


bool CNetDevice::StartLiveChannel(long channl,bool bMainStream)
{
	Json::Value root;
	Json::Value data;
	Json::Value stream;

	if (bMainStream)
	{
		stream.append(string(PROXY_MAIN_STREAM));
	}
	else
	{
		stream.append(string(PROXY_SUB_STREAM));
	}
	

	data[PROXY_ACTION] = PROXY_OPEN;
	data[PROXY_TYPE] = stream;
	data[PROXY_CHANNL] = (int)channl;

	root[PROXY_CMD] = PROXY_LIVE;
	root[PROXY_DATA] = data;

	REPLY reply;
	reply = SendCmd(PACKAGE_TYPE_MSG,root.toStyledString().c_str(),root.toStyledString().size());	
	if (NULL == reply->buf)
	{
		return false;
	}

	Json::Reader reader;
	Json::Value rp;

	if (!reader.parse(reply->buf,rp))
	{
		return false;
	}

	return rp[PROXY_CODE].asString() == PROXY_OK;
}

void CNetDevice::StopLiveChannel(long channl,bool bMainStream)
{
	Json::Value root;
	Json::Value data;
	Json::Value stream;

	if (bMainStream)
	{
		stream.append(string(PROXY_MAIN_STREAM));
	}
	else
	{
		stream.append(string(PROXY_SUB_STREAM));
	}
	

	data[PROXY_ACTION] = PROXY_CLOSE;
	data[PROXY_CHANNL] = (int)channl;
	data[PROXY_TYPE] = stream;

	root[PROXY_CMD] = PROXY_LIVE;
	root[PROXY_DATA] = data;

	SendCmd(PACKAGE_TYPE_MSG,root.toStyledString().c_str(),root.toStyledString().size());	
}

bool CNetDevice::StartDownload(std::string fileName,long begin,long end)
{
	Json::Value root;
	Json::Value data;
	Json::Value ranges;

	root[PROXY_CMD] = PROXY_DOWNLOAD;
	
	data[PROXY_ACTION] = PROXY_OPEN;
	data[PROXY_FILE] = fileName;

	data[PROXY_RANGE][(Json::UInt)0] = (int)begin;
	data[PROXY_RANGE][(Json::UInt)1] = (int)end;

	root[PROXY_DATA] = data;

	REPLY reply;
	reply = SendCmd(PACKAGE_TYPE_MSG,root.toStyledString().c_str(),root.toStyledString().size());	
	if (NULL == reply->buf)
	{
		return false;
	}

	Json::Reader reader;
	Json::Value rp;

	if (!reader.parse(reply->buf,rp))
	{
		return false;
	}

	m_bNeedFirstFrame = true;

	return rp[PROXY_CODE].asString() == PROXY_OK;
}

void CNetDevice::DownloadFeedBack(long recvBytes)
{
	Json::Value root;
	Json::Value data;

	root[PROXY_CMD] = PROXY_DOWNLOAD_FEEDBACK;
	data[PROXY_BYTE] = (int)recvBytes;

	root[PROXY_DATA] = data;

	SendCmd(PACKAGE_TYPE_MSG,root.toStyledString().c_str(),root.toStyledString().size(),false);
}

void CNetDevice::StopDownload()
{
	Json::Value root;
	Json::Value data;
	Json::Value ranges;

	root[PROXY_CMD] = PROXY_DOWNLOAD;
	data[PROXY_ACTION] = PROXY_CLOSE;
	root[PROXY_DATA] = data;
	
	REPLY reply;
	reply = SendCmd(PACKAGE_TYPE_MSG,root.toStyledString().c_str(),root.toStyledString().size());	
	
	m_bNeedFirstFrame = true;
}

bool CNetDevice::OnSecondTimer()
{
	if(m_netStatus == NET_STATUS_LOGOUT)
	{
		return false;
	}

	unsigned long curTick = PUB_GetTickCount();

	if (0 == m_lastTick)
	{
		m_lastTick = curTick;
	}

	if ((curTick > m_lastTick) && ((curTick -  m_lastTick) > HEARTBEAT_TIME_OUT_SEC/4))
	{
		SendCmd(PACKET_TYPE_HEART_BEAT,NULL,0,false);
		m_lastTick = curTick;
	}
    return true;
}

long CNetDevice::GetNetStatus()
{
	return m_netStatus;
}

void CNetDevice::OnRecvData(RECV_DATA_BUFFER *pDataBuffer)
{
	if (pDataBuffer == NULL || pDataBuffer->pData == NULL || pDataBuffer->dataSize == 0)
	{
		m_netStatus = NET_STATUS_LOGOUT;
		std::cout<<"device disconnected"<<std::endl;
		return ;
	}

	if (!m_bLoging && (NET_STATUS_LOGOUT == m_netStatus))
	{
		return;
	}

	do 
	{
		if (pDataBuffer->dataSize < PACKET_HEAD_SIZE)
		{
			break;
		}
		
		PACKET_HEAD *pHead = reinterpret_cast<PACKET_HEAD *>(pDataBuffer->pData);
		long cmd = pHead->type;

		if (!bLegal(cmd))
		{
			assert(false);
			std::cout<<"error net data"<<std::endl;
			m_netStatus = NET_STATUS_LOGOUT;
			break;
		}

		if ((pHead->length + PACKET_HEAD_SIZE) > pDataBuffer->dataSize)
		{
			break;
		}

		NET_DEV_BUF &recvBuf = m_recvBufMap[cmd];
		int ret = NetPackParser(recvBuf);

		//缓存的数据没有处理完，暂时不接数据
		if (1 == ret)
		{
			break;
		}

		if (m_lastRecvPackIndexmap.find(cmd) == m_lastRecvPackIndexmap.end())
		{
			m_lastRecvPackIndexmap[cmd] = pHead->count+1;
		}
		else if(m_lastRecvPackIndexmap[cmd] == pHead->count)
		{
			m_lastRecvPackIndexmap[cmd] = pHead->count+1;
		}
		else
		{
			m_lastRecvPackIndexmap.erase(cmd);
			recvBuf.dataLen = 0;	
		}


		if ((recvBuf.bufLen - recvBuf.dataLen) < (pHead->length + PACKET_HEAD_SIZE))
		{
			if ((2*recvBuf.bufLen) > PACKET_MAX_LEN)
			{
				assert(false);
				break;
			}

			if (recvBuf.bufLen == 0)
			{
				recvBuf.bufLen = PACKET_HEAD_SIZE + PACKET_MAX_PAYLOAD;
				recvBuf.buf = new char[recvBuf.bufLen];
				recvBuf.dataLen = 0;
			}
			else
			{
				char *newBuf = new char[recvBuf.bufLen*2];
				memcpy(newBuf,recvBuf.buf,recvBuf.dataLen);
				delete [] recvBuf.buf;
				recvBuf.buf = newBuf;
				recvBuf.bufLen = recvBuf.bufLen*2;
			}
		}

		if (recvBuf.dataLen > 0)
		{
			memcpy(recvBuf.buf+recvBuf.dataLen,pDataBuffer->pData+PACKET_HEAD_SIZE,pHead->length);	
			recvBuf.dataLen += pHead->length;	
		}
		else
		{
			memcpy(recvBuf.buf,pDataBuffer->pData,PACKET_HEAD_SIZE+pHead->length);
			recvBuf.dataLen += PACKET_HEAD_SIZE+pHead->length;
		}

		pDataBuffer->dataSize -= PACKET_HEAD_SIZE+pHead->length;

		if (pDataBuffer->dataSize > 0)
		{
			memmove(pDataBuffer->pData, pDataBuffer->pData + PACKET_HEAD_SIZE+pHead->length, pDataBuffer->dataSize);
		}


		NetPackParser(recvBuf);

	} while (true);
}

void CNetDevice::SetFrameCallBack(DATA_CALLBACK pDataCallBack,void *pUserData)
{
	m_pDataCallBack = pDataCallBack;
	m_pUserData = pUserData;
}


void CNetDevice::SetNetHandle(CSWL_MultiNet *pNetHandle,unsigned long deviceID)
{
	m_deviceLock.Lock();
	
	if (NULL != m_pNetHandle)
	{
		m_pNetHandle->RemoveComm(m_deviceID);
	}

	m_pNetHandle = pNetHandle;
	m_deviceID = deviceID;

	m_deviceLock.UnLock();
}


bool CNetDevice::bOnLine()
{
	return NET_STATUS_LOGIN == m_netStatus;
}


bool CNetDevice::bLegal(long cmd)
{
	if(PACKET_TYPE_EXCHANGE_ENCRYPT_KEY == cmd
		|| PACKET_TYPE_HEART_BEAT == cmd
		|| PACKAGE_TYPE_MSG == cmd
		|| PACKAGE_TYPE_PUSH == cmd
		|| PACKAGE_TYPE_LIVE == cmd
		|| PACKAGE_TYPE_MP4 == cmd
		|| PACKAGE_TYPE_TALK == cmd
		|| PACKAGE_TYPE_PIC == cmd)
	{
		return true;
	}

	return false;
}

//返回值 0完成处理,1包收完但未处理完,2包未接收完
int CNetDevice::NetPackParser(NET_DEV_BUF &recvBuf)
{
	if (recvBuf.dataLen < PACKET_HEAD_SIZE)
	{
		return 2;
	}

	PACKET_HEAD *pHead = (PACKET_HEAD *)(recvBuf.buf);
	
	if ((PACKET_HEAD_SIZE + pHead->total) > recvBuf.dataLen)
	{
		assert( pHead->total < PACKET_MAX_LEN);
		return 2;
	}

	char *pData = recvBuf.buf + PACKET_HEAD_SIZE;
	long dataLen = pHead->total;

	bool bUsed = false;
	long cmd = pHead->type;

	if ((PACKET_TYPE_EXCHANGE_ENCRYPT_KEY != pHead->type) && pHead->encrypt)
	{
		if (m_decryptBuf.bufLen < pHead->total)
		{
			if (NULL != m_decryptBuf.buf)
			{
				delete [] m_decryptBuf.buf;
			}

			m_decryptBuf.buf = new char[pHead->total];
			m_decryptBuf.bufLen = pHead->total;
		}

		m_aesCrypt.Decrypt((unsigned char *)pData,dataLen,(unsigned char *)m_decryptBuf.buf);
		pData = m_decryptBuf.buf;
		dataLen = pHead->total - pHead->padding;
	}

	if (PACKAGE_TYPE_MSG == cmd || PACKAGE_TYPE_PIC == cmd)
	{
		bUsed = OnMsg(pData,dataLen);
	}
	else if(PACKAGE_TYPE_LIVE == cmd)
	{
		bUsed = OnLive(pData,dataLen);
	}
	else if(PACKAGE_TYPE_MP4 == cmd)
	{
		if (!m_bNeedFirstFrame)
		{
			if ((strlen(PROXY_DOWNLOAD_TAIL)==dataLen) && (strncmp(pData,PROXY_DOWNLOAD_TAIL,strlen(PROXY_DOWNLOAD_TAIL)) == 0))
			{
				bUsed = true;
			}
			else
			{
				bUsed = OnMp4(pData,dataLen);
			}	
		}
		else
		{
			if ((strlen(PROXY_DOWNLOAD_HEAD)==dataLen) && (strncmp(pData,PROXY_DOWNLOAD_HEAD,strlen(PROXY_DOWNLOAD_HEAD)) == 0))
			{
				m_bNeedFirstFrame = false;
			}

			bUsed = true;
		}
	}
	else if(PACKAGE_TYPE_TALK == cmd)
	{
		bUsed = OnTalk(pData,dataLen);
	}
	else if(PACKAGE_TYPE_PUSH == cmd)
	{
		bUsed = OnPush(pData,dataLen);
	}
	else if(PACKET_TYPE_EXCHANGE_ENCRYPT_KEY == cmd)
	{
		bUsed = OnMsg(pData,dataLen);
	}
	else
	{
		bUsed = true;
	}


	if (bUsed)
	{
		recvBuf.dataLen = 0;
		return 0;
	}
	else
	{
		return 1;
	}
}

bool CNetDevice::OnMsg(char *pData,long dataLen)
{
	m_msgLock.Lock();
	
	if (m_msgBuf.buf != NULL)
	{
		delete [] m_msgBuf.buf;
		m_msgBuf.buf = NULL;
	}

	if (dataLen > 0)
	{
		m_msgBuf.buf = new char[dataLen];
		m_msgBuf.bufLen = dataLen;
		m_msgBuf.dataLen = dataLen;
		memcpy(m_msgBuf.buf,pData,dataLen);
	}
	else
	{
		m_msgBuf.buf = new char[1];
		m_msgBuf.bufLen = 0;
		m_msgBuf.dataLen = 0;
	}
	
	m_msgLock.UnLock();

	return true;
}

bool CNetDevice::OnLive(char *pData,long dataLen)
{
	bool ret = true;

	DATA_CALLBACK pDataCallBack = m_pDataCallBack;
	void *pUserData = m_pUserData;
	
	if ((NULL != pDataCallBack) && (NULL != pUserData))
	{
		ret = (pDataCallBack(pUserData,m_deviceID,0,pData,dataLen,true) == 0 ? true : false);
	}
	
	return ret;
}

bool CNetDevice::OnMp4(char *pData,long dataLen)
{
	bool ret = true;

	DATA_CALLBACK pDataCallBack = m_pDataCallBack;
	void *pUserData = m_pUserData;

	if ((NULL != pDataCallBack) && (NULL != pUserData))
	{
		ret = (pDataCallBack(pUserData,m_deviceID,0,pData,dataLen,false) == 0 ? true : false);
	}

	return ret;
}

bool CNetDevice::OnPush(char *pData,long dataLen)
{
	return true;
}

bool CNetDevice::OnTalk(char *pData,long dataLen)
{
	assert(false);
	return true;
}

CNetDevice::REPLY CNetDevice::SendCmd(long msg,const char *pData,long len,bool bHasReply)
{	
	NET_DEV_BUF *pDevBuf = new NET_DEV_BUF;

	m_deviceLock.Lock();
	if (NULL == m_pNetHandle)
	{
		m_deviceLock.UnLock();
		return REPLY(pDevBuf);
	}

	if (!m_bLoging && (NET_STATUS_LOGOUT == m_netStatus))
	{
		m_deviceLock.UnLock();
		return REPLY(pDevBuf);
	}

	m_msgLock.Lock();

	if (NULL != m_msgBuf.buf)
	{
		delete [] m_msgBuf.buf;

		m_msgBuf.buf = NULL;
		m_msgBuf.dataLen = 0;
		m_msgBuf.bufLen = 0;
	}
	
	m_msgLock.UnLock();

	const char *pSendData = pData;
	long dataLen = len;
	bool bEncrypt = false;
	long padding = 0;

	if (msg == PACKAGE_TYPE_MSG)
	{
		if (m_encryptBuf.bufLen  < ((len + 0xf)&(~0xf)))
		{
			if (NULL != m_encryptBuf.buf)
			{
				delete [] m_encryptBuf.buf;
			}

			m_encryptBuf.bufLen = ((len + 0xf)&(~0xf));
			m_encryptBuf.buf = new char [m_encryptBuf.bufLen];
			m_encryptBuf.dataLen = m_encryptBuf.bufLen;
		}
	
		pSendData = m_encryptBuf.buf;
		dataLen = ((len + 0xf)&(~0xf));
		bEncrypt = true;
		padding = dataLen - len;

		m_aesCrypt.Encrypt((unsigned char *)pData,dataLen,(unsigned char *)m_encryptBuf.buf);
	}

	if (!SendPackage(msg,pSendData,dataLen,padding,bEncrypt))
	{
		m_netStatus = NET_STATUS_LOGOUT;
		std::cout<<"send data failed"<<std::endl;
		m_deviceLock.UnLock();
		return REPLY(pDevBuf);
	}

	if (!bHasReply)
	{
		m_deviceLock.UnLock();
		return REPLY(pDevBuf);
	}

	m_lastTick = PUB_GetTickCount();

	int count = 0;
	while(true)
	{
		if (NULL != m_msgBuf.buf)
		{
			m_msgLock.Lock();
			pDevBuf->buf = m_msgBuf.buf;
			pDevBuf->dataLen = m_msgBuf.dataLen;
			pDevBuf->bufLen = m_msgBuf.bufLen;
		
			m_msgBuf.buf = NULL;
			m_msgBuf.dataLen = 0;
			m_msgBuf.bufLen = 0;

			m_msgLock.UnLock();
		
			break;
		}
		else
		{
			if (!m_bLoging && (NET_STATUS_LOGIN != m_netStatus))
			{
				break;
			}

			PUB_Sleep(1);
		
			count++;
			if (count > 15000)
			{
				break;
			}
		}
	}

	m_deviceLock.UnLock();

	return REPLY(pDevBuf);
}

bool CNetDevice::SendPackage(long msg,const char *pData,long len,long padding,bool bEncrypt)
{
	bool bRet = true;
	PACKET_HEAD *pHead = reinterpret_cast<PACKET_HEAD *>(m_sendBuf.buf);
	const char *pReadPos = pData;
	long sendDataLen = 0;

	do
	{
		sendDataLen = (len <= PACKET_MAX_PAYLOAD ? len : PACKET_MAX_PAYLOAD);
		sendDataLen = (sendDataLen + 0x3) & ~0x3;

		if (sendDataLen > 0)
		{
			memcpy(m_sendBuf.buf+PACKET_HEAD_SIZE,pReadPos,sendDataLen);
		}

		pHead->magic = PACKET_MAGIC_NUM;
		pHead->type = msg;
		pHead->count = m_lastSendPackIndexmap[msg]++;
		pHead->length = sendDataLen;
		pHead->total = len;
		pHead->encrypt = bEncrypt;
		pHead->padding = padding;

		if ((PACKET_HEAD_SIZE+sendDataLen) != m_pNetHandle->SendData(m_deviceID,m_sendBuf.buf,PACKET_HEAD_SIZE+sendDataLen,true))
		{
			bRet = false;
			break;
		}	

		len -= sendDataLen;
		pReadPos += sendDataLen;
	}while(len > 0);

	return bRet;
}


bool CNetDevice::bAgreeMent()
{
	CCryptRsa rsa;
	unsigned char rsaKey[128] = {0};
	unsigned char aesKey[AES_KEY_LEN] = {0};
	rsa.Initial(true);
	rsa.GetPublicKey(rsaKey,128);

	REPLY reply = SendCmd(PACKET_TYPE_EXCHANGE_ENCRYPT_KEY,(char *)rsaKey,128,true);
	
	if (NULL == reply->buf)
	{
		return false;
	}

	int keyLen = rsa.PrivateDecrypt((unsigned char *)reply->buf,reply->dataLen,aesKey);
	assert(AES_KEY_LEN == keyLen);

	m_aesCrypt.SetCryptKey(aesKey,AES_KEY_LEN*8);
	
	return true;
}