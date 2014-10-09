
/************************************************************************
***		RSA非对称加密算法

***		RSA加密特点：
*		1、算法比较简单，但同等安全级别下，比DES加密的算法要慢1000倍
*		2、算法基本思路：
			找两素数p和q 
			取n=p*q 
			取t=(p-1)*(q-1) 
			取任何一个数e,要求满足e取d*e%t==1 
			这样最终得到三个数： n d e 
			
			我们这里取n为公钥，d为私钥，e随意取一个

***		这里采用应用最广泛的CBC加密模式，SSL也用的这种加密模式
*************************************************************************/
#ifndef _CRYPT_RSA_H_
#define _CRYPT_RSA_H_

#include "openssl/rsa.h"

class CCryptRsa
{
public:
	CCryptRsa(void);
	~CCryptRsa(void);

	//bServer为true表示是密钥的生成方，这时需要填充的字段为keyBits,keyBits取值要求在1024以上
	//bServer为false表示要发送数据方，需要传入keyBits和publickey来加密数据
	bool Initial(bool bServer, int keyBits = 1024, const unsigned char *pPublicKey = NULL);

	//对于密钥生成方，需要把公钥取出来发给对方,传入的内存大小需要不小于keyBits/8个字节
	bool GetPublicKey(unsigned char *pPublicKey, int keyLen);

	//加密后的数据长度
	int GetEncryptSize();

	//公钥加密，dataLen <= GetEncryptSize() - 11
	//返回值为加密后的长度，-1表示加密失败
	int PublicEncryt(unsigned char *pPlainData, int dataLen, unsigned char *pEncryptData);

	//私钥解密,dataLen == GetEncryptSize()
	//返回值为解密后的明文长度，-1表示解密失败
	int PrivateDecrypt(unsigned char *pEncryptData, int dataLen, unsigned char *pPlainData);

	void Quit();

private:
	bool			m_bServer;
	int				m_keyBits;
	RSA				*m_pRsa;
	int				m_rsaSize;
};

#endif
