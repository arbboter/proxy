
#include "TVT_PubCom.h"
#include "CryptRsa.h"
#include "openssl/rand.h"

CCryptRsa::CCryptRsa(void)
{
	m_pRsa			= NULL;
	m_rsaSize		= -1;
}

CCryptRsa::~CCryptRsa(void)
{
}

bool CCryptRsa::Initial(bool bServer, int keyBits /* = 1024 */, const unsigned char *pPublicKey /* = NULL */)
{
	m_bServer = bServer;
	m_keyBits = keyBits;

	if(bServer)
	{
		TVT_DATE_TIME dateTime = GetCurrTime();
		char seed[4];
		memcpy(seed, &dateTime.microsecond, 3);
		unsigned long tickCount = PUB_GetTickCount();
		memcpy(seed+3, &tickCount, 1);
		RAND_seed(seed, 4);

		m_pRsa = RSA_new();
		BIGNUM *bne = BN_new();
		bne = BN_bin2bn((const unsigned char *)"\x01\x00\x01", 3, bne);
		RSA_generate_key_ex(m_pRsa,m_keyBits,bne,NULL);
		BN_free(bne);
	}
	else
	{
		assert(NULL != pPublicKey);
		m_pRsa = RSA_new();
		m_pRsa->n = BN_bin2bn(pPublicKey, keyBits/8, m_pRsa->n);
		m_pRsa->e = BN_bin2bn((const unsigned char *)"\x01\x00\x01", 3, m_pRsa->e);
	}
	m_rsaSize = RSA_size(m_pRsa);

	return true;
}

bool CCryptRsa::GetPublicKey(unsigned char *pPublicKey, int keyLen)
{
	assert(m_bServer);
	assert(NULL != m_pRsa);
	assert(NULL != pPublicKey);
	assert(keyLen >= (m_keyBits/8));
	if(!m_bServer || (NULL == m_pRsa) || (NULL == pPublicKey) || (keyLen < m_keyBits/8))
	{
		return false;
	}

	BN_bn2bin(m_pRsa->n, pPublicKey);
	return true;
}

int CCryptRsa::GetEncryptSize()
{
	return m_rsaSize;
}

int CCryptRsa::PublicEncryt(unsigned char *pPlainData, int dataLen, unsigned char *pEncryptData)
{
	assert(NULL != m_pRsa);
	assert(NULL != pPlainData);
	assert(NULL != pEncryptData);
	assert(dataLen <= (m_rsaSize-11));

	if((NULL == m_pRsa) || (NULL == pPlainData) || (NULL == pEncryptData) || (dataLen > (m_rsaSize - 11)))
	{
		return -1;
	}

	return RSA_public_encrypt(dataLen, pPlainData, pEncryptData, m_pRsa, RSA_PKCS1_PADDING);
}

int CCryptRsa::PrivateDecrypt(unsigned char *pEncryptData, int dataLen, unsigned char *pPlainData)
{
	assert(m_bServer);
	assert(NULL != m_pRsa);
	assert(NULL != pPlainData);
	assert(NULL != pEncryptData);
	assert(dataLen == m_rsaSize);

	if(!m_bServer || (NULL == m_pRsa) || (NULL == pPlainData) || (NULL == pEncryptData) || (dataLen != m_rsaSize))
	{
		return -1;
	}
	
	return RSA_private_decrypt(dataLen, pEncryptData, pPlainData, m_pRsa, RSA_PKCS1_PADDING);
}


void CCryptRsa::Quit()
{
	if(NULL != m_pRsa)
	{
		RSA_free(m_pRsa);
		m_pRsa = NULL;
	}
}

