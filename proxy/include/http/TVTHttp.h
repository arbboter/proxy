/////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __TVT_HTTP_H__
#define __TVT_HTTP_H__

/*HTTP请求的结构
|------------
|请求行		|
|------------
|请求头		|
|------------
|…			|
|------------
|请求头		|
|------------
|空行		|
|------------
|消息体		|
\------------
*/

#include "TVTHttpHeader.h"
#include "TVTHttpBody.h"

#define HTTP_SSL		0
class CTVTHttp
{
public:
	CTVTHttp();
	virtual ~CTVTHttp();

	bool Get(const char * pURL, const char * pCookie=NULL, const char * pAuthInfo=NULL);
	bool Post(const char * pURL, HTTP_BODY_INFO * pBodyInfo, const char * pCookie=NULL, const char * pAuthInfo=NULL);
	
	bool PostHeader(const char * pURL, HTTP_BODY_INFO * pBodyInfo, const char * pCookie, const char * pAuthInfo);
	int PostBodyData(const char * pBD, unsigned long bdLen);
	int PostOver(unsigned long rTimeOut);

	const char * GetRetBody(){return m_httpBody.GetRetBody();}
	long GetRetBodyLen(){return m_httpBody.GetRetBodyLen();}
	long GetRetBodyType(){return m_httpBody.GetRetBodyType();}

	const char *GetHeaderValue(const char *pHeaderName){return m_httpHeader.GetHeaderValue(pHeaderName);}
	const char * GetCookie(){return m_httpHeader.GetCookie();}
	void ClearCookie(){m_httpHeader.ClearCookie();}
	int GetSocket()
	{
		if (NULL != m_pTcpSession)
		{
			return m_pTcpSession->GetSocket();
		}
		else
		{
			return SWL_INVALID_SOCKET;
		}
	}
	void Shutdown(){CloseHost();}
private:
	int SendHttpBody(HTTP_BODY_INFO * pBodyInfo, unsigned long sTimeOut);
	int RecvHttpBody(unsigned long rTimeOut);

	bool ConnectHost(const char * pHost, unsigned short port);
	bool CloseHost();
	bool GetURLInfo(const char * pURL, char * pHost, int hostBufLen, unsigned short &port, char * pURI, int urIBufLen);
protected:
	CTVTHttpHeader	m_httpHeader;
	CTVTHttpBody	m_httpBody;

	SWL_socket_t	m_sock;
	CTcpSession		*m_pTcpSession;
	SESSION_TYPE	m_httpType;
	char			m_szHost[64];
	unsigned short	m_serPort;

	bool			m_bChunked;
};

#endif
/////////////////////////////////////////////////////////////////////////////////////////////////////
