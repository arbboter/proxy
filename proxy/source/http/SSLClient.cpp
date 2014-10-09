#include "SSLClient.h"
#if HTTP_SSL
#include "openssl/conf.h"
#include "openssl/x509v3.h"

#include <string.h>
#include <assert.h>
#include <vector>
#ifdef __DEBUG_NEW__
#include "debug_new.h"
#define new DEBUG_NEW
#endif


unsigned long CSSLClient::s_objectCount = 0;
CPUB_Lock CSSLClient::s_Lock;
bool CSSLClient::s_bHasInitial = false;

static std::vector<CPUB_Lock*> s_vecLock;

//That's the global context structure which is created by a server or client once per program life-time
//and which holds mainly default values for the SSL structures which are later created for the connections.
SSL_CTX	*s_SSLCTX = NULL;

CSSLClient::CSSLClient()
{
	m_pSSL			= NULL;
}

CSSLClient::~CSSLClient()
{
}

bool CSSLClient::Initial(unsigned long encryptionType)
{
	//初始SSL
	s_Lock.Lock();
	if (0 == s_objectCount)
	{
		SSL_library_init();
		const SSL_METHOD * pSSLMethod = NULL;
		if (TVT_ENCRYPTION_SSL == encryptionType)
		{
			pSSLMethod = TLSv1_client_method();
		}
		else if(TVT_ENCRYPTION_TLS == encryptionType)
		{
			pSSLMethod = SSLv3_client_method();
		}

		SSL_load_error_strings();
		s_SSLCTX = SSL_CTX_new(pSSLMethod);
		if(NULL == s_SSLCTX)
		{
			int errNum = ERR_get_error();
			printf("%s:%s:%d, ssl connect error:%s\n", __FUNCTION__, __FILE__, __LINE__, ERR_reason_error_string(errNum));
			printf("%s:%s:%d, error function=%s\n", __FUNCTION__, __FILE__, __LINE__, ERR_func_error_string(errNum));
			s_Lock.UnLock();
			return false;
		}
	/*	if (SSL_CTX_use_certificate_file(s_SSLCTX, CA_CERT_FILE, SSL_FILETYPE_PEM) <= 0)
		{
			printf("Error: %s\n", ERR_reason_error_string(ERR_get_error())); 
			s_Lock.UnLock();
			return false;
		}

		if (SSL_CTX_use_PrivateKey_file_pass(s_SSLCTX, CA_KEY_FILE, "123456") <= 0)
		{
			printf("use_PrivateKey_file err\n");
			s_Lock.UnLock();
			return false;
		}
		*/
		for (int i=0; i < CRYPTO_num_locks(); ++i)
		{
			CPUB_Lock *pLock = new CPUB_Lock;
			s_vecLock.push_back(pLock);
		}

		CRYPTO_set_id_callback(GetThreadId);
		CRYPTO_set_locking_callback(pthreads_locking_callback);

		s_bHasInitial = true;
	}

	s_objectCount++;

	s_Lock.UnLock();

	return true;
}

void CSSLClient::Quit()
{
	if (s_objectCount == 0)
	{
		return;
	}

	printf("CSSLClient::Quit()\n");
	FreeRsource();
	
	s_Lock.Lock();
	s_objectCount--;
	if (0 == s_objectCount)
	{
		for (int i=0; i<CRYPTO_num_locks(); ++i)
		{
			if (!s_vecLock.empty())
			{
				std::vector<CPUB_Lock*>::iterator iter = s_vecLock.begin();
				for (; iter != s_vecLock.end(); ++iter)
				{
					delete (*iter);
					(*iter) = NULL;
				}
				s_vecLock.clear();
			}
		}

		CRYPTO_set_locking_callback(NULL);
		if (NULL != s_SSLCTX)
		{
			SSL_CTX_free(s_SSLCTX);
			s_SSLCTX = NULL;
		}
		s_bHasInitial = false;
		printf("CSSLClient::Quit ALL\n");
	}
	s_Lock.UnLock();
}

//设置使用的加密套件
bool CSSLClient::SSLSetCipher(const char *pCipherName)
{
	if (!s_bHasInitial)
	{
		assert(false);
		printf("%s:%s:%d, s_bHasInitial=false\n", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}
	
	if (NULL == s_SSLCTX)
	{
		assert(false);
		printf("%s:%s:%d, s_SSLCTX=NULL\n", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	if (NULL == pCipherName)
	{
		assert(false);
		printf("%s:%s:%d, pCipherName = NULL\n", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}

	//保存老的加密组建
	const char *pOldCipher = SSL_get_cipher(m_pSSL);
	char *pOldCipherTemp = NULL;
	if (NULL != pOldCipher)
	{
		printf("%s:%s:%d, Old cipher=%s\n", __FUNCTION__, __FILE__, __LINE__, pOldCipher);
		unsigned long len = strlen(pOldCipher) + 10;
		pOldCipherTemp = new char [len];
		if (NULL == pOldCipherTemp)
		{
			printf("%s:%s:%d, NEW %d char error\n", __FUNCTION__, __FILE__, __LINE__, len);
			return false;
		}
		else
		{
			memset(pOldCipherTemp, 0, len);
			memcpy(pOldCipherTemp, pOldCipher, strlen(pOldCipher));
		}
	}

	bool bChanged = false;
	int retVal = SSL_set_cipher_list(m_pSSL, pCipherName);
	if (0 == retVal)
	{
		printf("%s:%s:%d, pCipherName=%s\n", __FUNCTION__, __FILE__, __LINE__, pCipherName);
		printf("%s:%s:%d, New cipher=%s\n", __FUNCTION__, __FILE__, __LINE__, SSL_get_cipher(m_pSSL));
		bChanged = true;
	}
	else
	{
		SSL_set_cipher_list(m_pSSL, pOldCipherTemp);
	}

	if (NULL != pOldCipherTemp)
	{
		delete pOldCipherTemp;
		pOldCipherTemp = NULL;
	}
	
	return bChanged;
}

//int	CSSLClient::SSLBindSock(SWL_socket_t sockfd)
//{
//	if (NULL != m_pSSL)
//	{
//		int retVal = SSL_set_fd(m_pSSL, sockfd);
//		return 0;
//	}
//	else
//	{
//		return -1;
//	}
//}

int	CSSLClient::SSLConnect(SWL_socket_t sockfd, bool bVerifyPeer, const char *pHost, const char *pCertFile, \
						   const char *pPriteKeyFile, const char *pCAPath, const char *pCAFile, const char *pPassWd)
{
	s_Lock.Lock();
	if (!s_bHasInitial)
	{
		printf("ssl not initial\n");
		s_Lock.UnLock();
		return -1;
	}
	s_Lock.UnLock();
	
	if (pCertFile!=NULL && pPriteKeyFile!=NULL)
	{
		if (pCAPath)
		{
			if(SSL_CTX_load_verify_locations(s_SSLCTX, pCAFile, NULL) <= 0)
			{
				printf("Failed to set CA location...\n");
				return -1;
			}
		}
		if (SSL_CTX_use_certificate_file(s_SSLCTX, pCertFile, SSL_FILETYPE_PEM) <= 0) 
		{
			printf("%s %s %d failed\n", __FILE__, __FUNCTION__, __LINE__);
			int errNum = ERR_get_error();
			printf("%s:%s:%d, ssl connect error:%s\n", __FUNCTION__, __FILE__, __LINE__, ERR_reason_error_string(errNum));
			printf("%s:%s:%d, error function=%s\n", __FUNCTION__, __FILE__, __LINE__, ERR_func_error_string(errNum));
			return -1;
		}
		if (pPassWd)
		{
			SSL_CTX_set_default_passwd_cb_userdata(s_SSLCTX, (void*)pPassWd);
		}
		if (SSL_CTX_use_PrivateKey_file(s_SSLCTX, pPriteKeyFile, SSL_FILETYPE_PEM) <= 0) 
		{
			printf("%s %s %d failed\n", __FILE__, __FUNCTION__, __LINE__);
			int errNum = ERR_get_error();
			printf("%s:%s:%d, ssl connect error:%s\n", __FUNCTION__, __FILE__, __LINE__, ERR_reason_error_string(errNum));
			printf("%s:%s:%d, error function=%s\n", __FUNCTION__, __FILE__, __LINE__, ERR_func_error_string(errNum));
			return -1;
		}

		if (!SSL_CTX_check_private_key(s_SSLCTX)) 
		{
			printf("%s %s %d failed\n", __FILE__, __FUNCTION__, __LINE__);
			int errNum = ERR_get_error();
			printf("%s:%s:%d, ssl connect error:%s\n", __FUNCTION__, __FILE__, __LINE__, ERR_reason_error_string(errNum));
			printf("%s:%s:%d, error function=%s\n", __FUNCTION__, __FILE__, __LINE__, ERR_func_error_string(errNum));
			return -1;
		}
	}

	if (NULL == m_pSSL)
	{
		if (bVerifyPeer)
		{
			SSL_CTX_set_verify(s_SSLCTX, SSL_VERIFY_PEER, VerifyCallBack);
		}
		m_pSSL = SSL_new(s_SSLCTX);
		if (NULL == m_pSSL)
		{
			printf("ssl new err\n");
			return -1;
		}
	}

	if (1 != SSL_set_fd(m_pSSL, sockfd))
	{
		int errNum = ERR_get_error();
		printf("%s:%s:%d, ssl connect error:%s\n", __FUNCTION__, __FILE__, __LINE__, ERR_reason_error_string(errNum));
		printf("%s:%s:%d, error function=%s\n", __FUNCTION__, __FILE__, __LINE__, ERR_func_error_string(errNum));
		return -1;
	}
	SSL_set_connect_state(m_pSSL);

	unsigned long reTry = 0; 
	int ret = 0, err = 0;
	while(reTry++ < SSL_SOCKET_CONNECT_RETRIES)
	{
		if((ret = SSL_connect(m_pSSL)) == 1)
		{
			break;
		}
		err = SSL_get_error(m_pSSL, ret);
		if(SSL_ERROR_WANT_CONNECT == err || SSL_ERROR_WANT_READ == err || SSL_ERROR_WANT_WRITE == err)
		{
			PUB_Sleep(10); // wait a while
		}
		else
		{
			printf("SSL_connect Err : %d\n", err);
			int errNum = ERR_get_error();
			printf("%s:%s:%d, ssl connect error:%s\n", __FUNCTION__, __FILE__, __LINE__, ERR_reason_error_string(errNum));
			printf("%s:%s:%d, error function=%s\n", __FUNCTION__, __FILE__, __LINE__, ERR_func_error_string(errNum));
			FreeRsource();
			return -1;
		}
	}
	if (reTry >= SSL_SOCKET_CONNECT_RETRIES)
	{
		printf("reTry >= SSL_SOCKET_CONNECT_RETRIES\n");
		FreeRsource();
		return -1;
	}

	if (pHost)
	{
		if (PostConnectCheck(m_pSSL, pHost) < 0)
		{
			return -1;
		}
	}
	
	return 0;
}


int CSSLClient::SSLRead(void *pBuf, int bufLen)
{
	if (NULL == pBuf || bufLen == 0 || NULL == m_pSSL)//
	{
		return 0;
	}

	unsigned long reTry = 0; 
	int ret = 0, err = 0;
	int tmpRecvLen = 0;
	while((reTry++ < SSL_SOCKET_CONNECT_RETRIES) && tmpRecvLen <= bufLen)
	{
		if((ret = SSL_read(m_pSSL, (void *)((char *)pBuf + tmpRecvLen), bufLen - tmpRecvLen)) <= 0)
		{
			err = SSL_get_error(m_pSSL, ret);
			if(SSL_ERROR_WANT_READ == err)
			{
				PUB_Sleep(1); // wait a while
			}
			else
			{
				printf("SSL_read Err, Cur read Len:%d  Need Len : %d\n", tmpRecvLen, bufLen);
				FreeRsource();
				return -1;
			}
		}
		else
		{
			tmpRecvLen += ret;
			break;
		}
	}

	return tmpRecvLen;
}

int	CSSLClient::SSLWrite(const void * pDataBuf, int dataLen)
{
	if (pDataBuf == NULL)
	{
		printf("SSLWrite pDataBuf NULL\n");
		return -1;
	}
	if (dataLen == 0)
	{
		printf("SSLWrite dataLen 0\n");
		return 0;
	}
	if (m_pSSL == NULL)
	{
		printf("SSLWrite m_pssl NULL\n");
		return -1;
	}
	if (m_pSSL->method == NULL || m_pSSL->packet == NULL || m_pSSL->wbio == NULL)
	{
		printf("m_pssl contains NULL\n");
		return -1;
	}

	unsigned long reTry = 0; 
	int ret = 0, err = 0;
	int tmpSendLen = 0;;
	while((reTry++ < SSL_SOCKET_CONNECT_RETRIES) && tmpSendLen != dataLen)
	{
		if((ret = SSL_write(m_pSSL, (const void *)((char *)pDataBuf + tmpSendLen), dataLen - tmpSendLen)) <= 0)
		{
			err = SSL_get_error(m_pSSL, ret);
			if(SSL_ERROR_WANT_WRITE == err)
			{
				PUB_Sleep(1); // wait a while
			}
			else
			{
				printf("SSL_write Err, Cur Send Len:%d  Need Len : %d\n", tmpSendLen, dataLen);
				FreeRsource();
				return -1;
			}
		}
		else
		{
			tmpSendLen += ret;
		}
	}

	return tmpSendLen;
}

void CSSLClient::FreeRsource()
{
	//printf("FreeRsource begin\n");
	if (NULL != m_pSSL)
	{
		int ret = 0 , err = 0;
		//printf("SSL_shutdown begin\n");
		
		ret = SSL_shutdown(m_pSSL);
		if (ret < 0)
		{
			err = SSL_get_error(m_pSSL, ret);
		}

		SSL_free(m_pSSL);
		m_pSSL = NULL;
	}
	//printf("FreeRsource End\n");
}

int CSSLClient::SSL_CTX_use_PrivateKey_file_pass(SSL_CTX *ctx,const char *pFileName,char *pPass)
{
	EVP_PKEY     *pkey = NULL;
	BIO          *pBIO = NULL;

	pBIO = BIO_new(BIO_s_file());
	BIO_read_filename(pBIO, pFileName);

	pkey = PEM_read_bio_PrivateKey(pBIO, NULL, NULL, pPass);

	if(pkey == NULL)
	{
		printf("PEM_read_bio_PrivateKey err");
		return -1;
	}

	if (SSL_CTX_use_PrivateKey(ctx,pkey) <= 0)
	{
		printf("SSL_CTX_use_PrivateKey err\n");
		return -1;
	}

	BIO_free(pBIO);

	return 1;

}


unsigned long CSSLClient::GetThreadId()
{
#ifndef WIN32
	pthread_t pid = pthread_self();
#else
	DWORD pid = ::GetCurrentThreadId();
#endif
	return (unsigned long)pid;
}

void CSSLClient::pthreads_locking_callback(int mode, int type, const char *file, int line)
{
	s_Lock.Lock();
	if (0 == s_objectCount)
	{
		s_Lock.UnLock();
		return;
	}
	s_Lock.UnLock();
	if (type >= CRYPTO_num_locks())
	{
		return;
	}
	if (mode & CRYPTO_LOCK)
	{
		s_vecLock[type]->Lock();
	}
	else
	{
		s_vecLock[type]->UnLock();
	}
}


int CSSLClient::VerifyCallBack(int ok, X509_STORE_CTX *store)
{
	char data[256] = {0};
	bool ret = false;
	if (!ok)
	{
		X509 *cert = X509_STORE_CTX_get_current_cert(store);
		int depth = X509_STORE_CTX_get_error_depth(store);
		int err = X509_STORE_CTX_get_error(store);
		printf("-Error with certificate at depth: %i\n",
			depth);
		X509_NAME_oneline(X509_get_issuer_name(cert), data, 256);
		printf(" issuer = %s\n", data);
		X509_NAME_oneline(X509_get_subject_name(cert), data, 256);
		printf(" subject = %s\n", data);
		printf(" err %i:%s\n", err,	X509_verify_cert_error_string(err));
	
	}
	return ok;
}

//extention to SSL_get_verify_result
int CSSLClient::PostConnectCheck(SSL *ssl, const char *pHost)
{
	X509 *cert;
	X509_NAME *subj;
	char data[256];
	int extcount;
	int ret = 0;

	do 
	{
		if (!(cert = SSL_get_peer_certificate(ssl)) || !pHost)
		{
			break;
		}
		if ((extcount = X509_get_ext_count(cert)) > 0)
		{
			int i;
			for (i = 0; i < extcount; i++)
			{
				const char *extstr;
				X509_EXTENSION *ext;
				ext = X509_get_ext(cert, i);
				extstr = OBJ_nid2sn(OBJ_obj2nid(X509_EXTENSION_get_object(ext)));
				if (!strcmp(extstr, SN_subject_alt_name))
				{
					int j;
					const unsigned char *pData;
					STACK_OF(CONF_VALUE) *val;
					CONF_VALUE *nval;
					const X509V3_EXT_METHOD *pExtMethod;
					if (!(pExtMethod = X509V3_EXT_get(ext)))
					{
						break;
					}
					pData = ext->value->data;
					val = pExtMethod->i2v(pExtMethod, pExtMethod->d2i(NULL, &pData, ext->value->length), NULL);
					for (j = 0; j < sk_CONF_VALUE_num(val); j++)
					{
						nval = sk_CONF_VALUE_value(val, j);
						if (!strcmp(nval->name, "DNS") && !strcmp(nval->value, pHost))
						{
							ret = 1;
							break;
						}
					}
				}
				if (ret)
				{
					break;
				}
			}//end of for(; extentcount; )
		}
		if (!ret && (subj = X509_get_subject_name(cert)) && X509_NAME_get_text_by_NID(subj, NID_commonName, data, 256) > 0)
		{
			data[255] = 0;
			if (strcmp(data, pHost) != 0)
			{
				break;
			}
		}

		X509_free(cert);
		ret = SSL_get_verify_result(ssl);
		return ret;

	} while(0);

	if (cert)
	{
		X509_free(cert);
	}
	return -1;
}
#endif