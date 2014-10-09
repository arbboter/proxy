// NatSimpleDataBlock.h: interface for the handling NatSimpleDataBlock.
//
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_SIMPLE_DATA_BLOCK_H_
#define _NAT_SIMPLE_DATA_BLOCK_H_

#include "NatTraversalProtocol.h"
#include "NatRunner.h"

/**
 * Simple Data Block Ð­Òé½âÎöÆ÷
 */
class CNatDataBlockParser
{
public:
    class CParserNotifier
    {
    public:
        virtual int OnRecvDataBlock(CNatDataBlockParser *parser, const char* pDataBlock, 
            int iLen) = 0;
    };
public:
    CNatDataBlockParser();
    ~CNatDataBlockParser();
    void Reset();
    void SetNotifier(CParserNotifier *notifier);
    int Parse(const char* pData, int iLen);
private:
    void NotifyOnRecvDataBlock(const char* pDataBlock, int iLen);
private:
    NAT_DATA_BLOCK m_blockData;
    CParserNotifier *m_notifier;
    bool m_parsingHeader;
    int m_recvPos;
};

class CNatDataBlockSender
{
public:
    class CSenderNotifier
    {
    public:
        virtual int OnSendDataBlock(CNatDataBlockSender *sender, const char* pDataBlock, 
            int iLen) = 0;
    };
public:
    CNatDataBlockSender();
    ~CNatDataBlockSender();
    void Reset();
    void SetNotifier(CSenderNotifier *notifer);
    NatRunResult Run();
    char* BeginSetData();
    void EndSetData(int iLen, bool bIsEncrypt);
    void SendData(const char *pBuf, int iLen, bool bIsEncrypt);
    bool IsSending();
private:
    CSenderNotifier *m_notifier;
    bool m_sending;
    NAT_DATA_BLOCK m_blockData;
    int m_sendPos;
};

#endif//_NAT_SIMPLE_DATA_BLOCK_H_