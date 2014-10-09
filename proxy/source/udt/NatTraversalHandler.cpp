// NatTraversalHandler.cpp: implements for the class NatTraversalHandler.
//
//////////////////////////////////////////////////////////////////////

#include "NatCommon.h"
#include "NatTraversalHandler.h"


#include "NatDebug.h"

CNatTraversalHandler::CNatTraversalHandler()
:
m_xmlPacker(NULL),
m_xmlParser(NULL),
m_ownedXmlPacker(NULL),
m_ownedXmlParser(NULL),
m_notifier(NULL),
m_isInitialized(false)
{
    m_dataBlockParser.SetNotifier(this);
    m_dataBlockSender.SetNotifier(this);
}

CNatTraversalHandler::~CNatTraversalHandler()
{
    Clear();
}

bool CNatTraversalHandler::Initialize()
{
    return Initialize(NULL, NULL);
}

bool CNatTraversalHandler::Initialize( CNatTraversalXmlPacker *xmlPacker, CNatTraversalXmlParser *xmlParser )
{
    assert(!m_isInitialized);
    if (xmlPacker != NULL)
    {
        m_xmlPacker = xmlPacker;
    }
    else
    {
        m_ownedXmlPacker = new CNatTraversalXmlPacker();
        if (m_ownedXmlPacker == NULL)
        {
            Clear();
            return false;
        }
        m_xmlPacker = m_ownedXmlPacker;
    }

    if (xmlParser != NULL)
    {
        m_xmlParser = xmlParser;
    }
    else
    {
        m_ownedXmlParser = new CNatTraversalXmlParser();
        if (m_ownedXmlParser == NULL)
        {
            Clear();
            return false;
        }
        m_xmlParser = m_ownedXmlParser;
    }
    m_isInitialized = true;
    return true;
}


void CNatTraversalHandler::Finalize()
{
    Clear();
    m_isInitialized = false;
}

bool CNatTraversalHandler::IsInitialized()
{
    return m_isInitialized;
}

bool CNatTraversalHandler::RecvTraversalData( const char* pData, int iLen )
{
    if(m_dataBlockParser.Parse(pData, iLen) < 0)
    {
        NAT_LOG_DEBUG("CNatTraversalHandler recv wrong traversal data!\n");
        return false;
    }
    return true;
}

bool CNatTraversalHandler::SendCmd( NatTraversalCmdId cmdId, void *cmd, int cmdLen )
{
    return SendCmd(cmdId, cmd, cmdLen, NULL);
}

bool CNatTraversalHandler::SendCmd(NatTraversalCmdId cmdId, void *cmd, int cmdLen, const char* version)
{
	if (!m_dataBlockSender.IsSending())
	{
		m_xmlPacker->Reset(version);
		if(m_xmlPacker->PackCmd(cmdId, cmd, cmdLen))
		{
			m_dataBlockSender.SendData(m_xmlPacker->GetXmlData(), m_xmlPacker->GetXmlDataLen(), false);            
		}
		else
		{
			NAT_LOG_ERROR("CNatTraversalHandler pack traversal xml error! CmdId=%d", cmdId);
		}

		return true;
	}
	return false;
}

NatRunResult CNatTraversalHandler::RunSend()
{
    return m_dataBlockSender.Run();
}

bool CNatTraversalHandler::IsSending()
{
    return m_dataBlockSender.IsSending();
}

void CNatTraversalHandler::SetNotifier( CHandlerNotifier *notifier )
{
    m_notifier = notifier;
}

int CNatTraversalHandler::OnRecvDataBlock( CNatDataBlockParser *parser, const char* pDataBlock, int iLen )
{
    if (iLen < 0)
    {
        NAT_LOG_DEBUG("CNatTraversalHandler parse data block wrong! Len=%d", iLen);
        return -1;
    }

    m_xmlParser->Reset(pDataBlock, iLen);
    NatTraversalCmdId cmdId = NAT_ID_UNKNOW;
    CNatTraversalXmlParser::ParseError parseError = CNatTraversalXmlParser::PARSE_OK;

    parseError = m_xmlParser->CheckHeader(cmdId);
    if (CNatTraversalXmlParser::PARSE_OK != parseError)
    {
        // ignore the error
        NAT_LOG_INFO("Parse XML Header Error: %d\n", parseError);
        return -1;
    }

    bool isHandled = true;
    if (m_notifier != NULL)
    {
        m_notifier->OnBeforeRecvCmd(m_xmlParser->GetVersion(), cmdId, isHandled);
    }
    if (!isHandled)
    {
        return 0;
    }

    parseError = m_xmlParser->Execute();
    if (CNatTraversalXmlParser::PARSE_OK != parseError)
    {
        // ignore the error
        NAT_LOG_INFO("Parse XML Execute Error: %d\n", parseError);
        return -1;
    }

    if (m_notifier != NULL)
    {
        m_notifier->OnRecvCmd(m_xmlParser->GetVersion(), cmdId, m_xmlParser->GetCmd());
    }
    return 0;
}

int CNatTraversalHandler::OnSendDataBlock( CNatDataBlockSender *sender, const char* pDataBlock, int iLen )
{
    if (m_notifier != NULL)
    {
        return m_notifier->OnSendTraversalData(pDataBlock, iLen);
    }
    return -1;
}

void CNatTraversalHandler::Clear()
{
    if (m_ownedXmlParser != NULL)
    {
        delete m_ownedXmlParser;
        m_ownedXmlParser = NULL;
    }
    m_xmlParser = NULL;

    if (m_ownedXmlPacker != NULL)
    {
        delete m_ownedXmlPacker;
        m_ownedXmlPacker = NULL;
    }
    m_xmlPacker = NULL;
    m_dataBlockParser.Reset();
    m_dataBlockSender.Reset();
}
