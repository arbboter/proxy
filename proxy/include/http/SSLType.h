//////////////////////////////////////////////////////////////////
#ifndef _SSL_TYPE_H_
#define _SSL_TYPE_H_

#ifdef  WIN32
const char SERVER_KEY_FILE[]		= "./server.key";
const char SERVER_REQ_CERT_FILE[]	= "./server.pem";
const char SERVER_CERT_FILE[]		= "./server.crt";

const char CA_KEY_FILE[]			= "D:/work/4.0.0/xFalcon/debug/ca.key";
const char CA_CERT_FILE[]			= "D:/work/4.0.0/xFalcon/debug/ca.crt";
#else
const char SERVER_KEY_FILE[]		= "/mnt/mtd/sslCfg/server.key";
const char SERVER_REQ_CERT_FILE[]	= "/mnt/mtd/sslCfg/server.pem";
const char SERVER_CERT_FILE[]		= "/mnt/mtd/sslCfg/server.crt";

const char CA_KEY_FILE[]			= "/mnt/mtd/sslCfg/ca.key";
const char CA_CERT_FILE[]			= "/mnt/mtd/sslCfg/ca.crt";
#endif

const	unsigned long	DEFAULT_BITS	= 1024;

//ssl client 请求的加密套件
//ALL表示添加所有的加密套件
//但不包含身份认证方式为DH、ECDSA、DSS、ECDH、PSK、的加密套件
//不包含密钥交换方式为ECDH的加密套件
//STRENGTH表示对选取的加密套件按安全级别由高到低排序，这样SSL客户端和SSL服务器协商加密套件时会从最高安全级别开始协商
const char	TVT_SSL_CIPHER0[] = "ALL:!ADH:!AECDSA:!ADSS:!AECDH:!APSK:!KECDH:!KPSK:@STRENGTH";
const char	TVT_SSL_CIPHER1[] = "ARSA:KRSA:@STRENGTH";

//SSLV3支持的加密套件，openssl ciphers -v -ssl3 > cipher
const char	TVT_SSL_CIPHER[][128] = {
//	加密套件名称								 密钥交换方式	认证方式	数据加密方式		信息摘要方式
	"ECDHE-RSA-AES256-SHA",				// SSLv3 Kx=ECDH		Au=RSA		Enc=AES(256)		Mac=SHA1
	"ECDHE-ECDSA-AES256-SHA",			// SSLv3 Kx=ECDH		Au=ECDSA	Enc=AES(256)		Mac=SHA1
	"DHE-RSA-AES256-SHA",				// SSLv3 Kx=DH			Au=RSA		Enc=AES(256)		Mac=SHA1
	"DHE-DSS-AES256-SHA",				// SSLv3 Kx=DH			Au=DSS		Enc=AES(256)		Mac=SHA1
	"DHE-RSA-CAMELLIA256-SHA",			// SSLv3 Kx=DH			Au=RSA		Enc=Camellia(256)	Mac=SHA1
	"DHE-DSS-CAMELLIA256-SHA",			// SSLv3 Kx=DH			Au=DSS		Enc=Camellia(256)	Mac=SHA1
	"ECDH-RSA-AES256-SHA",				// SSLv3 Kx=ECDH/RSA	Au=ECDH		Enc=AES(256)		Mac=SHA1
	"ECDH-ECDSA-AES256-SHA",			// SSLv3 Kx=ECDH/ECDSA	Au=ECDH		Enc=AES(256)		Mac=SHA1
	"AES256-SHA",						// SSLv3 Kx=RSA			Au=RSA		Enc=AES(256)		Mac=SHA1
	"CAMELLIA256-SHA",					// SSLv3 Kx=RSA			Au=RSA		Enc=Camellia(256)	Mac=SHA1
	"PSK-AES256-CBC-SHA",				// SSLv3 Kx=PSK			Au=PSK		Enc=AES(256)		Mac=SHA1
	"ECDHE-RSA-DES-CBC3-SHA",			// SSLv3 Kx=ECDH		Au=RSA		Enc=3DES(168)		Mac=SHA1
	"ECDHE-ECDSA-DES-CBC3-SHA",			// SSLv3 Kx=ECDH		Au=ECDSA	Enc=3DES(168)		Mac=SHA1
	"EDH-RSA-DES-CBC3-SHA",				// SSLv3 Kx=DH			Au=RSA		Enc=3DES(168)		Mac=SHA1
	"EDH-DSS-DES-CBC3-SHA",				// SSLv3 Kx=DH			Au=DSS		Enc=3DES(168)		Mac=SHA1
	"ECDH-RSA-DES-CBC3-SHA",			// SSLv3 Kx=ECDH/RSA	Au=ECDH		Enc=3DES(168)		Mac=SHA1
	"ECDH-ECDSA-DES-CBC3-SHA",			// SSLv3 Kx=ECDH/ECDSA	Au=ECDH		Enc=3DES(168)		Mac=SHA1
	"DES-CBC3-SHA",						// SSLv3 Kx=RSA			Au=RSA		Enc=3DES(168)		Mac=SHA1
	"PSK-3DES-EDE-CBC-SHA",				// SSLv3 Kx=PSK			Au=PSK		Enc=3DES(168)		Mac=SHA1
	"ECDHE-RSA-AES128-SHA",				// SSLv3 Kx=ECDH		Au=RSA		Enc=AES(128)		Mac=SHA1
	"ECDHE-ECDSA-AES128-SHA",			// SSLv3 Kx=ECDH		Au=ECDSA	Enc=AES(128)		Mac=SHA1
	"DHE-RSA-AES128-SHA",				// SSLv3 Kx=DH			Au=RSA		Enc=AES(128)		Mac=SHA1
	"DHE-DSS-AES128-SHA",				// SSLv3 Kx=DH			Au=DSS		Enc=AES(128)		Mac=SHA1
	"DHE-RSA-SEED-SHA",					// SSLv3 Kx=DH			Au=RSA		Enc=SEED(128)		Mac=SHA1
	"DHE-DSS-SEED-SHA",					// SSLv3 Kx=DH			Au=DSS		Enc=SEED(128)		Mac=SHA1
	"DHE-RSA-CAMELLIA128-SHA",			// SSLv3 Kx=DH			Au=RSA		Enc=Camellia(128)	Mac=SHA1
	"DHE-DSS-CAMELLIA128-SHA",			// SSLv3 Kx=DH			Au=DSS		Enc=Camellia(128)	Mac=SHA1
	"ECDH-RSA-AES128-SHA",				// SSLv3 Kx=ECDH/RSA	Au=ECDH		Enc=AES(128)		Mac=SHA1
	"ECDH-ECDSA-AES128-SHA",			// SSLv3 Kx=ECDH/ECDSA	Au=ECDH		Enc=AES(128)		Mac=SHA1
	"AES128-SHA",						// SSLv3 Kx=RSA			Au=RSA		Enc=AES(128)		Mac=SHA1
	"SEED-SHA",							// SSLv3 Kx=RSA			Au=RSA		Enc=SEED(128)		Mac=SHA1
	"CAMELLIA128-SHA",					// SSLv3 Kx=RSA			Au=RSA		Enc=Camellia(128)	Mac=SHA1
	"IDEA-CBC-SHA",						// SSLv3 Kx=RSA			Au=RSA		Enc=IDEA(128)		Mac=SHA1
	"PSK-AES128-CBC-SHA",				// SSLv3 Kx=PSK			Au=PSK		Enc=AES(128)		Mac=SHA1
	"ECDHE-RSA-RC4-SHA",				// SSLv3 Kx=ECDH		Au=RSA		Enc=RC4(128)		Mac=SHA1
	"ECDHE-ECDSA-RC4-SHA",				// SSLv3 Kx=ECDH		Au=ECDSA	Enc=RC4(128)		Mac=SHA1
	"ECDH-RSA-RC4-SHA",					// SSLv3 Kx=ECDH/RSA	Au=ECDH		Enc=RC4(128)		Mac=SHA1
	"ECDH-ECDSA-RC4-SHA",				// SSLv3 Kx=ECDH/ECDSA	Au=ECDH		Enc=RC4(128)		Mac=SHA1
	"RC4-SHA",							// SSLv3 Kx=RSA			Au=RSA		Enc=RC4(128)		Mac=SHA1
	"RC4-MD5",							// SSLv3 Kx=RSA			Au=RSA		Enc=RC4(128)		Mac=MD5 
	"PSK-RC4-SHA",						// SSLv3 Kx=PSK			Au=PSK		Enc=RC4(128)		Mac=SHA1
	"EDH-RSA-DES-CBC-SHA",				// SSLv3 Kx=DH			Au=RSA		Enc=DES(56)			Mac=SHA1
	"EDH-DSS-DES-CBC-SHA",				// SSLv3 Kx=DH			Au=DSS		Enc=DES(56)			Mac=SHA1
	"DES-CBC-SHA",						// SSLv3 Kx=RSA			Au=RSA		Enc=DES(56)			Mac=SHA1
	"EXP-EDH-RSA-DES-CBC-SHA",			// SSLv3 Kx=DH(512)		Au=RSA		Enc=DES(40)			Mac=SHA1 export
	"EXP-EDH-DSS-DES-CBC-SHA",			// SSLv3 Kx=DH(512)		Au=DSS		Enc=DES(40)			Mac=SHA1 export
	"EXP-DES_CBC_SHA",					// SSLv3 Kx=RSA(512)	Au=RSA		Enc=DES(40)			Mac=SHA1 export
	"EXP-RC2-CBC-MD5",					// SSLv3 Kx=RSA(512)	Au=RSA		Enc=RC2(40)			Mac=MD5  export
	"EXP-RC4-MD5",						// SSLv3 Kx=RSA(512)	Au=RSA		Enc=RC4(40)			Mac=MD5  export
};

typedef enum _tvt_cipher_type_
{
	ECDHE_RSA_AES256_SHA	= 0,
	ECDHE_ECDSA_AES256_SHA	= 1,
	DHE_RSA_AES256_SHA		= 2,
	DHE_DSS_AES256_SHA		= 3,
	DHE_RSA_CAMELLIA256_SHA	= 4,
	DHE_DSS_CAMELLIA256_SHA = 5,
	ECDH_RSA_AES256_SHA		= 6,
	ECDH_ECDSA_AES256_SHA	= 7,
	AES256_SHA				= 8,
	CAMELLIA256_SHA			= 9,
	PSK_AES256_CBC_SHA		= 10,
	ECDHE_RSA_DES_CBC3_SHA	= 11,
	ECDHE_ECDSA_DES_CBC3_SHA= 12,
	EDH_RSA_DES_CBC3_SHA	= 13,
	EDH_DSS_DES_CBC3_SHA	= 14,
	ECDH_RSA_DES_CBC3_SHA	= 15,
	ECDH_ECDSA_DES_CBC3_SHA	= 16,
	DES_CBC3_SHA			= 17,
	PSK_3DES_EDE_CBC_SHA	= 18,
	ECDHE_RSA_AES128_SHA	= 19,
	ECDHE_ECDSA_AES128_SHA	= 20,
	DHE_RSA_AES128_SHA		= 21,
	DHE_DSS_AES128_SHA		= 22,
	DHE_RSA_SEED_SHA		= 23,
	DHE_DSS_SEED_SHA		= 24,
	DHE_RSA_CAMELLIA128_SHA	= 25,
	DHE_DSS_CAMELLIA128_SHA	= 26,
	ECDH_RSA_AES128_SHA		= 27,
	ECDH_ECDSA_AES128_SHA	= 28,
	AES128_SHA				= 29, 
	SEED_SHA				= 30,
	CAMELLIA128_SHA			= 31,
	IDEA_CBC_SHA			= 32,
	PSK_AES128_CBC_SHA		= 33,
	ECDHE_RSA_RC4_SHA		= 34,
	ECDHE_ECDSA_RC4_SHA		= 35, 
	ECDH_RSA_RC4_SHA		= 36, 
	ECDH_ECDSA_RC4_SHA		= 37, 
	RC4_SHA					= 38, 
	RC4_MD5					= 39,
	PSK_RC4_SHA				= 40,
	EDH_RSA_DES_CBC_SHA		= 41,
	EDH_DSS_DES_CBC_SHA		= 42,
	DES_CBC_SHA				= 43,
	EXP_EDH_RSA_DES_CBC_SHA	= 44,
	EXP_EDH_DSS_DES_CBC_SHA	= 45,
	EXP_DES_CBC_SHA			= 46,
	EXP_RC2_CBC_MD5			= 47,
	EXP_RC4_MD5				= 48,
}TVT_CIPHER_TYPE;

#endif  /*_SSL_TYPE_H_*/
//////////////////////////////////////////////////////////////////
