// NatTraversalHandler.h: interface for the class CNatTraversalHandler.
//
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_TRAVERSAL_HANDLER_H
#define _NAT_TRAVERSAL_HANDLER_H

#include "NatSimpleDataBlock.h"
#include "NatTraversalXml.h"

class CNatTraversalHandler: public CNatDataBlockParser::CParserNotifier,
    CNatDataBlockSender::CSenderNotifier
{
public:
    class CHandlerNotifier
    {
    public:
        /**
         * 发送数据回调通知
         * 当需要发送的穿透命令经转化处理为可发送的数据后，回调通知。
         *
         */
        virtual int OnSendTraversalData(const char* pData, int iLen) = 0;

        /**
         * 在接收命令前回调通知
		 * 主要是给调用者选择是否处理该命令并回调通知该命令。
		 * @param[in] version 版本号
		 * @param[in] cmdId 命令ID
         * @param[in] isHandled 是否处理该命令，默认是true；如果设置为false，将忽略该命令的处理。
         */
        virtual void OnBeforeRecvCmd(const char* version, const NatTraversalCmdId &cmdId, bool &isHandled) = 0;

        /**
		* 接收命令回调通知
		* @param[in] version 版本号
         * @param[in] cmdId 命令ID
         * @param[in] cmd 命令的数据体
         */
        virtual void OnRecvCmd(const char* version, const NatTraversalCmdId &cmdId, void* cmd) = 0;

		/**
		 * 接收命令出错
		 * 主要是解析错误
		 * @param[in] error 解析错误
		 */
		virtual void OnRecvCmdError(CNatTraversalXmlParser::ParseError error) = 0;
    };
public:
    CNatTraversalHandler();

    ~CNatTraversalHandler();

    bool Initialize();

    bool Initialize(CNatTraversalXmlPacker *xmlPacker, CNatTraversalXmlParser *xmlParser);

    void Finalize();

    bool IsInitialized();

    bool RecvTraversalData(const char* pData, int iLen);

	bool SendCmd(NatTraversalCmdId cmdId, void *cmd, int cmdLen);

	bool SendCmd(NatTraversalCmdId cmdId, void *cmd, int cmdLen, const char* version);

    NatRunResult RunSend();

    bool IsSending();

    void SetNotifier(CHandlerNotifier *notifier);
public:
    ////////////////////////////////////////////////////////////////////////////////
    // CNatDataBlockParser::CNotifier public interface

    virtual int OnRecvDataBlock(CNatDataBlockParser *parser, const char* pDataBlock, 
        int iLen);
public:
    ////////////////////////////////////////////////////////////////////////////////
    // CNatDataBlockSender::CSenderNotifier public interface

    virtual int OnSendDataBlock(CNatDataBlockSender *sender, const char* pDataBlock, 
        int iLen);
private:
    void Clear();
private:
    CNatTraversalXmlPacker *m_xmlPacker;
    CNatTraversalXmlParser *m_xmlParser;
    CNatTraversalXmlPacker *m_ownedXmlPacker;
    CNatTraversalXmlParser *m_ownedXmlParser;
    CNatDataBlockParser m_dataBlockParser;
    CNatDataBlockSender m_dataBlockSender;
    CHandlerNotifier *m_notifier;
    bool m_isInitialized;
};

#endif//_NAT_TRAVERSAL_HANDLER_H