
/************************************************************************
***		AES对称加密算法

***		AES加密特点：
*		1、安全等级优于DES
*		2、分组加密，也称为块加密
*		3、有5种加密模式，ECB、CBC、CFB、OFB、CTR

***		这里采用应用最广泛的CBC加密模式，SSL也用的这种加密模式
*************************************************************************/

#ifndef _CRYPT_AES_H_
#define _CRYPT_AES_H_

class CCryptAes
{
public:
	CCryptAes(void);
	~CCryptAes(void);

	//生成随机128位密钥
	static void GenerateKey(unsigned char *pKey, int keyLen);

	//设置密钥，加解密都用这个密钥，userKey为密钥，bits为密钥位数
	void SetCryptKey(const unsigned char *userKey, const int bits);

	//加密数据pPlainData, length为明文的长度,不要求是16整数倍;
	//加密后的数据pEncryptData长度必须为16字节的整数倍
	bool Encrypt(const unsigned char *pPlainData, int length, unsigned char *pEncryptData);

	//解密数据pEncryptData, length为解密后的明文的长度,不要求长度是16的整数倍
	bool Decrypt(const unsigned char *pEncryptData, int length, unsigned char *pPlainData);

private:
	unsigned char			m_userKey[32];					//最多 32 * 8 = 256 bit
	int						m_keyBits;						//密钥位数
};

#endif
