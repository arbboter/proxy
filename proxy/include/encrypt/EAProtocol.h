
/********************************************************************************************
*		加密协商（Encrypt Agreement）协议
*
*		1、协商内容包括是否要加密、加密密钥传送、每一块数据是否是密文
*		2、加密协商协议透明传输上层数据，同时兼容上层数据密文传输及明文传输
*
********************************************************************************************/

#ifndef EA_PROTOCOL_H_
#define EA_PROTOCOL_H_

const unsigned int EA_HEAD_FLAG = '1111';

typedef enum _ea_frame_type
{
	EA_FRAME_PLAIN,				//明文
	EA_FRAME_AES,				//AES加密密文

	EA_FRAME_AGREEMENT	= 255,	//协商数据
}EA_FRAME_TYPE;

typedef enum _ea_agreement_cmd_
{
	EA_CMD_SET_ENCRYPT_MODE,
	EA_CMD_PUBLIC_KEY,
	EA_CMD_USER_KEY,
	EA_CMD_AGREEMENT_SUCC,
	EA_CMD_AGREEMENT_FAIL,
}EA_AGREEMENT_CMD;

typedef struct _ea_frame_head_
{
	unsigned int		headFlag;		//EA_HEAD_FLAG
	unsigned int		frameLength;	//帧长度
}EA_FRAME_HEAD;

typedef struct _ea_frame_info_
{
	EA_FRAME_HEAD		frameHead;
	unsigned char		frameType;		//EA_FRAME_TYPE
	unsigned char		alignPadding;	//为做对齐增加的字节数
	unsigned char		resv[2];
}EA_FRAME_INFO;

typedef struct _ea_cmd_head_
{
	unsigned int		cmdId;		//EA_AGREEMENT_CMD
	unsigned int		paramLen;	//参数长度
}EA_CMD_HEAD;


/************************************************   协议命令说明    **********************************************/

//EA_CMD_SET_ENCRYPT_MODE
//客户端向服务器发起，用来指定这个会话需不需要加密。如果需要，成功回复EA_CMD_PUBLIC_KEY，失败回复EA_CMD_AGREEMENT_FAIL；如果不需要，则回复EA_CMD_AGREEMENT_SUCC
//发送的数据组成为：EA_FRAME_INFO + EA_CMD_HEAD + int(加密模式，取值为EA_FRAME_TYPE)

//EA_CMD_PUBLIC_KEY
//服务器端对客户端的回应，当客户端要求加密传输时回应，参数为rsa加密的公钥
//发送的数据组成为：EA_FRAME_INFO + EA_CMD_HEAD + unsigned char [EA_CMD_HEAD.paramLen]

//EA_CMD_USER_KEY
//客户端向服务器发起，要求传送数据加密密钥给服务器，数据加密密钥用公钥加密
//发数的数据组成为：EA_FRAME_INFO + EA_CMD_HEAD + unsigned char [EA_CMD_HEAD.paramLen]

//EA_CMD_AGREEMENT_SUCC
//服务器端对客户端的回应，表示协商成功，按协商的加密及密钥进行数据传输
//发送的数据组成为：EA_FRAME_INFO + EA_CMD_HEAD

//EA_CMD_AGREEMENT_FAIL
//服务器端对客户端的回应，表示协商失败，此时要么重新协商，要么按明文传输
//发送的数据组成为：EA_FRAME_INFO + EA_CMD_HEAD

#endif

