// NatSimpleDataBlock.cpp: implements for handling NatSimpleDataBlock.
//
//////////////////////////////////////////////////////////////////////

#include "NatCommon.h"
#include "NatSimpleDataBlock.h"


#include "NatDebug.h"


CNatDataBlockParser::CNatDataBlockParser()
:
m_notifier(NULL),
m_parsingHeader(true),
m_recvPos(0)
{

}

CNatDataBlockParser::~CNatDataBlockParser()
{

}

void CNatDataBlockParser::Reset()
{
    m_parsingHeader = true;
    m_recvPos = 0;
}

void CNatDataBlockParser::SetNotifier( CParserNotifier *notifier )
{
    m_notifier = notifier;
}

int CNatDataBlockParser::Parse( const char* pData, int iLen )
{
    int originalDataPos = 0;
    bool running = true;
    int recvLen = 0;
    char* recvData = NULL;
    while(originalDataPos < iLen) 
    {
        if (m_parsingHeader)
        {
            recvLen = min(iLen - originalDataPos, (int)sizeof(NAT_DATA_BLOCK_HEADER) - m_recvPos);
            recvData = (char*)&m_blockData;
            memcpy(recvData + m_recvPos, pData + originalDataPos, recvLen);
            m_recvPos += recvLen;
            originalDataPos += recvLen;
            if (m_recvPos == sizeof(NAT_DATA_BLOCK_HEADER))
            {
				// m_blockData.header.iDataLen == 0 ?? need handle?
                if (m_blockData.header.iDataLen > 0 && m_blockData.header.iDataLen < NAT_DATA_BLOCK_MAX_SIZE)
                {
                    m_parsingHeader = false;
                    m_recvPos = 0;
                }
                else
                {
                    NotifyOnRecvDataBlock(NULL, -1);
                    break;
                }
            }
        }
        else // recv data
        {
            recvLen = min(iLen - originalDataPos, m_blockData.header.iDataLen - m_recvPos);
			if (recvLen > 0)
			{
				assert(recvLen < NAT_DATA_BLOCK_MAX_SIZE);
				recvData = (char*)&m_blockData.caData;
				memcpy(recvData + m_recvPos, pData + originalDataPos, recvLen);
				m_recvPos += recvLen;
				originalDataPos += recvLen;
			}
            if (m_recvPos == m_blockData.header.iDataLen)
            {
                m_blockData.caData[m_recvPos] = 0;
                m_parsingHeader = true;
                m_recvPos = 0;
                // TODO: handle encrypt??
                NotifyOnRecvDataBlock((char*)&m_blockData.caData, m_blockData.header.iDataLen);
            }

        } 
    }

    return originalDataPos;
}

void CNatDataBlockParser::NotifyOnRecvDataBlock( const char* pDataBlock, int iLen )
{
    if (m_notifier != NULL)
    {
        m_notifier->OnRecvDataBlock(this, pDataBlock, iLen);
    }
}

////////////////////////////////////////////////////////////////////////////////
// class CNatDataBlockSender implements

CNatDataBlockSender::CNatDataBlockSender()
:
m_notifier(NULL),
m_sending(false),
m_sendPos(0)
{

}

CNatDataBlockSender::~CNatDataBlockSender()
{

}

void CNatDataBlockSender::Reset()
{
    m_sending = false;
    m_sendPos = 0;
}

void CNatDataBlockSender::SetNotifier( CSenderNotifier *notifer )
{
    m_notifier = notifer;
}

NatRunResult CNatDataBlockSender::Run()
{
    NatRunResult ret = RUN_NONE;
    if (m_sending)
    {
        int sendLen = sizeof(NAT_DATA_BLOCK_HEADER) + m_blockData.header.iDataLen - m_sendPos;
        int sendRet = 0;
        char* sendData = (char*) &m_blockData + m_sendPos;
        if (m_notifier != NULL)
        {
            sendRet = m_notifier->OnSendDataBlock(this, sendData, sendLen);
            if (sendRet > 0)
            {
                m_sendPos += sendRet;
            }
            else if(sendRet < 0)
            {
                ret = RUN_DO_FAILED;
            }
        }
        if (m_sendPos == sizeof(NAT_DATA_BLOCK_HEADER) + m_blockData.header.iDataLen)
        {
            m_sendPos = 0;
            m_sending = false;
            // TODO: need? ret = RUN_DO_MORE;
        }
    }
    return ret;
}

char* CNatDataBlockSender::BeginSetData()
{
    assert(!m_sending);
    return (char*)&m_blockData.caData;
}

void CNatDataBlockSender::EndSetData( int iLen, bool isEncrypt)
{
    assert(!m_sending);
    assert(iLen <= NAT_DATA_BLOCK_MAX_SIZE);
    m_blockData.header.usVersion.sVersion.ucHighVer = 0;
    m_blockData.header.usVersion.sVersion.ucLowVer = 1; 
    m_blockData.header.iDataLen = iLen;
    // TODO: handle encrypt    
    m_sending = true;
}


void CNatDataBlockSender::SendData( const char *pBuf, int iLen, bool bIsEncrypt )
{
    assert(!m_sending);
    assert(iLen <= NAT_DATA_BLOCK_MAX_SIZE);
    memcpy(&m_blockData.caData, pBuf, iLen);
    m_blockData.header.usVersion.sVersion.ucHighVer = 0;
    m_blockData.header.usVersion.sVersion.ucLowVer = 1; 
    m_blockData.header.iDataLen = iLen;
    // TODO: handle encrypt    
    m_sending = true;
}

bool CNatDataBlockSender::IsSending()
{
    return m_sending;
}