//////////////////////////////////////////////////////////////////////////////////////
#ifndef _SSL_CLIENT_H_
#define _SSL_CLIENT_H_
#if HTTP_SSL
#include "SWL_TypeDefine.h"
#include "TVT_PubCom.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "SSLType.h"

const unsigned long SSL_SOCKET_CONNECT_RETRIES = 2000;

class CSSLClient
{
public:
	CSSLClient();
	~CSSLClient();

	bool Initial(unsigned long encryptionType = TVT_ENCRYPTION_SSL);
	void Quit();

	//设置加密套件，SSL支持的加密套件查看头文件SSLType.h
	bool SSLSetCipher(const char *pCipherName);
	//int	 SSLBindSock(SWL_socket_t sockfd);
	int	SSLConnect(SWL_socket_t sockfd, bool bVerifyPeer = false,  const char *pHost = NULL, const char *pCertFile = NULL, const char *pPriteKeyFile = NULL,\
		const char *pCAPath = NULL,  const char *pCAFile = NULL, const char *pPassWd = NULL);
	void SSLShutdown();

	int	 SSLRead(void *pBuf, int bufLen);
	int	 SSLWrite(const void * pDataBuf, int dataLen);

protected:
private:
	void FreeRsource();
	int SSL_CTX_use_PrivateKey_file_pass(SSL_CTX *ctx,const char *pFileName,char *pPass);
	static unsigned long GetThreadId();
	static void pthreads_locking_callback(int mode, int type, const char *file, int line);
	static int VerifyCallBack(int ok, X509_STORE_CTX *store);
	int PostConnectCheck(SSL *ssl, const char *pHost = NULL);

private:
	static unsigned long s_objectCount;
	static CPUB_Lock s_Lock;
	static bool s_bHasInitial;

	//That's the main SSL/TLS structure which is created by a server or client per established connection
	SSL					*m_pSSL;
	

};
#endif
#endif	/*_SSL_CLIENT_H_*/
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
