/////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __TVT_HTTP_BODY_H__
#define __TVT_HTTP_BODY_H__

#include "TVTHttpHeader.h"

class CTVTHttpBody
{
public:
	CTVTHttpBody();
	~CTVTHttpBody();

	bool Initial(const CTVTHttpHeader * pBodyHeader);
	void Quit();

	int SendBody(CTcpSession * pTcpSession);
	int RecvBody(CTcpSession * pTcpSession);

	const char * GetRetBody(){return (0 != m_iBodyLen) ? m_pBodyBuf : NULL;}
	long GetRetBodyLen(){return m_iBodyLen;}
	long GetRetBodyType(){return m_iBodyType;}
protected:
private:
	int ParseChunckedData(const char * pBody, int iLength);
	int ParseChunckedDataEx(const char * pBody, int iLength);
	int ParseChunckedDataLen(const char * pBody, int iLength);
private:
	char *	m_pBodyBuf;
	int		m_iBodyBufLen; 
	int		m_iBodyLen;
	int		m_iOperatePos;
	int		m_iTotalBodyLen;
	int		m_iBodyType;

	bool	m_bChunked;
	int		m_iCurChunkedLen;
	int		m_iCurChunkedRecv;
	bool	m_bNeedParseLength;
	bool	m_bRecvOver;
};

#endif
