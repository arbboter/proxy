// SWL_MultiNetComm.cpp: implementation of the CSWL_MultiNet class.
//
//////////////////////////////////////////////////////////////////////
#include "SWL_MultiNet.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSWL_MultiNet::CSWL_MultiNet()
{

}

CSWL_MultiNet::~CSWL_MultiNet()
{
}

bool CSWL_MultiNet::Init()
{
 	return true;
}

void CSWL_MultiNet::Destroy()
{
}
/******************************************************************************
*
*
*
******************************************************************************/
int CSWL_MultiNet::AddComm(long deviceID, TVT_SOCKET sock)
{
	return 0;
}

/******************************************************************************
*
*
*
******************************************************************************/
int CSWL_MultiNet::RemoveComm(long deviceID)
{
	return 0;
}

int CSWL_MultiNet::RemoveAllConnect()
{
	return 0;
}

/******************************************************************************
*
*	采用同步发送数据的方式，通过此接口直接调用socket发送接口发送数据
*
*	返回值：
*	发送成功，返回实际发送成功的字节数
	发送失败，返回-1
*
*	参数：bBlock: 如果设置为true，即此接口为阻塞模式，则会等待数据发送完毕或超时为止
*
******************************************************************************/
int CSWL_MultiNet::SendData(long deviceID, const void *pBuf, int iLen, bool bBlock)
{
	return -1;
}

