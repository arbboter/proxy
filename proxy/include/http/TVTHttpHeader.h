#ifndef __TVT_HTTP_HEADER_H__
#define __TVT_HTTP_HEADER_H__

#include "TVTHttpDef.h"
#include "TcpSession.h"

class CTVTHttpHeader
{
public:
	CTVTHttpHeader();
	~CTVTHttpHeader();
	
	bool Initial(const char * pHost, const char * pURI, const char * pCookie, const char * pAuthInfo, bool bPost=false, HTTP_BODY_INFO * pBodyInfo=NULL);
	void Quit();

	int SendHeader(CTcpSession * pTcpSession);
	int RecvHeader(CTcpSession * pTcpSession);

	int ContentType()const{return m_contentType;}
	int ContentLength()const{return m_contentLength;}
	bool Closed(){return m_bClosed;}
	bool IsChunkedContentData()const{return m_bChunked;}
	const char * ContentData(char * pBuf, int &iBufLen)const;
	const char * ContentData(int &iLength)const;
	const char *GetHeaderValue(const char *pHeaderName);
	const char *GetCookie(){return m_szCookie;}
	void ClearCookie();
protected:
	//组装HTTP头
	bool AddStartLine(const char *pMethod, const char *pRequestURI, const char *pHTTPVersion);
	bool AddMsgHeaderLine(const char *pHeaderName, const char *pHeaderValue);
	bool AddMsgHeaderLine(const char *pHeaderName, int value);
	bool AddEndLine();
	void SetCookie(const char * pNewCookie);
private:
	const char *GetOneLine(CTcpSession * pTcpSession, int &len, unsigned long rTimeOut);
	void ClearMSGList();
private:
	char *	m_pHeaderBuf;
	int		m_iHeaderBufLen;
	int		m_iHeaderLen;
	int		m_iOperatePos;

	char	m_szCookie[MAX_COOKIE_LEN];
	std::list<char *>	m_cookieList;
	MSG_HEADER_LIST		m_msgList;		//应答中的头信息

	int		m_contentLength;
	bool	m_bChunked;
	bool	m_bClosed;
	int		m_contentType;
};
#endif
