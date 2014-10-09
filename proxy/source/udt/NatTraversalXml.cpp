// NatTraversalXml.cpp: implements for the handling NatTraversalXml.
//
//////////////////////////////////////////////////////////////////////

#include "NatCommon.h"
#include "NatTraversalXml.h"


#include "NatDebug.h"

bool ParseXmlChildP2PVer(NatXmlElement *parentElement, const char* childName, NAT_P2P_PROTOCOL_VER &value)
{
    static const int P2P_VER_CHAR_LEN = 16;
    char valueStr[P2P_VER_CHAR_LEN];
    bool ret = ParseXmlChildStr(parentElement, childName, valueStr, P2P_VER_CHAR_LEN);
    if (ret)
    {

        int len = (int)strlen(valueStr);
        if (len >= 3)
        {
            char *pointStr = strchr(valueStr, '.');
            if (pointStr > valueStr && pointStr < valueStr + len - 1)
            {
                char* lowStr = pointStr + 1;
                char* highStr = valueStr;
                *pointStr = 0;
                value.sVersion.ucHighVer = atoi(highStr);
                value.sVersion.ucLowVer = atoi(lowStr);
            }
            else
            {
                ret = false;
            }
        }
        else
        {
            ret = false;
        }
    }
    return ret;
}

bool AddXmlChildP2PVer(NatXmlNode *parentNode, const char* name, const NAT_P2P_PROTOCOL_VER &value )
{
    static const int P2P_VER_CHAR_LEN = 16;
    char buf[P2P_VER_CHAR_LEN] = {};
    sprintf(buf,"%d.%d",value.sVersion.ucHighVer, value.sVersion.ucLowVer);
    return AddXmlChildStr(parentNode, name, buf);
}

////////////////////////////////////////////////////////////////////////////////
// class CNatTraversalXmlParser implements

CNatTraversalXmlParser::CNatTraversalXmlParser()
:
m_xmlDoc(NULL),
m_error(PARSE_OK),
m_cmdId(NAT_ID_UNKNOW),
m_cmdElement(NULL)
{

}

CNatTraversalXmlParser::~CNatTraversalXmlParser()
{
    if (m_xmlDoc != NULL)
    {
        delete m_xmlDoc;
    }
}

CNatTraversalXmlParser::ParseError CNatTraversalXmlParser::Reset( const char* pDataBlock, int iDataBlockLen )
{
    assert(pDataBlock != NULL && iDataBlockLen > 0);

    m_cmdElement = NULL;

    if (m_xmlDoc != NULL)
    {
        delete m_xmlDoc;
    }
    m_xmlDoc = new NatXmlDocument();
    if (m_xmlDoc == NULL)
    {
        m_error = PARSE_MEMORY_USED_UP;
        return m_error;
    }

    m_xmlDoc->Parse(pDataBlock);
    m_error = PARSE_OK;
    return m_error;
}

CNatTraversalXmlParser::ParseError CNatTraversalXmlParser::CheckHeader( NatTraversalCmdId &CmdId )
{
    assert(m_cmdElement == NULL);

    if (m_error != PARSE_OK)
    {
        return m_error;
    }

    NatXmlElement *pRootElement = m_xmlDoc->RootElement();
    if (NULL == pRootElement)
    {
        m_error = PARSE_VERSION_INVLID;
        return m_error;
    }


    const char* version = pRootElement->Attribute("version");
    if (NULL == version)
    {
        m_error = PARSE_VERSION_INVLID;
        return m_error;
    }
	snprintf(m_version, sizeof(m_version), "%s", version);
    // TODO: check version support?

    m_cmdElement = pRootElement->FirstChildElement("Cmd");
    if (m_cmdElement == NULL)
    {
        m_error = PARSE_NO_CMD;
        return m_error;
    }

    const char *idStr = m_cmdElement->Attribute("id");
    if (NULL == idStr)
    {
        m_error = PARSE_UNKOWN_ID;
        return m_error;
    }
    m_cmdId =(NatTraversalCmdId)atoi(idStr);
    if (m_cmdId == NAT_ID_UNKNOW)
    {
        m_error = PARSE_UNKOWN_ID;
        return m_error;
    }
    CmdId = m_cmdId;
    return m_error;
}

CNatTraversalXmlParser::ParseError CNatTraversalXmlParser::Execute()
{
    if (m_error != PARSE_OK)
    {
        return m_error;
    }

    assert(m_cmdElement != NULL);
    switch(m_cmdId)
    {

    /* 请求命令 */
    case NAT_ID_DEVICE_REGISTER_REQ:
        m_error = Parse_DeviceRegisterReq(reinterpret_cast<NAT_DEVICE_REGISTER_REQ*>(&m_cmd));
        break;
    case NAT_ID_CLIENT_CONNECT_P2P_REQ:
        m_error = Parse_ClientConnectP2PReq(reinterpret_cast<NAT_CLIENT_CONNECT_P2P_REQ*>(&m_cmd));
        break;
    case NAT_ID_NOTIFY_CONNECT_P2P_REQ:
        m_error = Parse_NotifyConnectP2PReq(reinterpret_cast<NAT_NOTIFY_CONNECT_P2P_REQ*>(&m_cmd));
        break;
    case NAT_ID_CLIENT_CONNECT_RELAY_REQ:
        m_error = Parse_ClientConnectRelayReq(reinterpret_cast<NAT_CLIENT_CONNECT_RELAY_REQ*>(&m_cmd));
        break;
    case NAT_ID_NOTIFY_CONNECT_RELAY_REQ:
        m_error = Parse_NotifyConnectRelayReq(reinterpret_cast<NAT_NOTIFY_CONNECT_RELAY_REQ*>(&m_cmd));
        break;
    case NAT_ID_FETCH_DEVICE_REQ:
        m_error = Parse_FetchDeviceReq(reinterpret_cast<NAT_FETCH_DEVICE_REQ*>(&m_cmd));
        break;

    /* 应答命令 */
    case NAT_ID_DEVICE_REGISTER_REPLY:
        m_error = Parse_DeviceRegisterReply(reinterpret_cast<NAT_DEVICE_REGISTER_REPLY*>(&m_cmd));
        break;
    case NAT_ID_CLIENT_CONNECT_P2P_REPLY:
        m_error = Parse_ClientConnectP2PReply(reinterpret_cast<NAT_CLIENT_CONNECT_P2P_REPLY*>(&m_cmd));
        break;
    case NAT_ID_NOTIFY_CONNECT_P2P_REPLY:
        m_error = Parse_NotifyConnectP2PReply(reinterpret_cast<NAT_NOTIFY_CONNECT_P2P_REPLY*>(&m_cmd));
        break;

    case NAT_ID_CLIENT_CONNECT_RELAY_REPLY:
        m_error = Parse_ClientConnectRelayReply(reinterpret_cast<NAT_CLIENT_CONNECT_RELAY_REPLY*>(&m_cmd));
        break;
    case NAT_ID_NOTIFY_CONNECT_RELAY_REPLY:
        m_error = Parse_NotifyConnectRelayReply(reinterpret_cast<NAT_NOTIFY_CONNECT_RELAY_REPLY*>(&m_cmd));
        break;
    case NAT_ID_FETCH_DEVICE_REPLY:
        m_error = Parse_FetchDeviceReply(reinterpret_cast<NAT_FETCH_DEVICE_REPLY*>(&m_cmd));        
        break;

    case NAT_ID_HEARTBEAT:
        m_error = ParseTraversalHeartbeat(reinterpret_cast<NAT_TRAVERSAL_HEARTBEAT*>(&m_cmd));
        break;
    case NAT_ID_DEVICE_UNREGISTER:
        m_error = PARSE_UNKOWN_ID;
        break;
    default:
        m_error = PARSE_UNKOWN_ID;
        break;
    }
    return m_error;
}

NatTraversalCmdId CNatTraversalXmlParser::GetCmdId()
{
    return m_cmdId;
}

void* CNatTraversalXmlParser::GetCmd()
{
    return &m_cmd;
}

CNatTraversalXmlParser::ParseError CNatTraversalXmlParser::GetError()
{
    return m_error;
}

CNatTraversalXmlParser::ParseError CNatTraversalXmlParser::Parse_DeviceRegisterReq( NAT_DEVICE_REGISTER_REQ *req )
{
    assert(m_cmdElement != NULL);

    if(!ParseXmlChildUint32(m_cmdElement, "RequestSeq", req->stHeader.dwRequestSeq))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    if(!ParseXmlChildStr(m_cmdElement, "DeviceNo", req->caDeviceNo, sizeof(req->caDeviceNo)))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    if(!ParseXmlChildP2PVer(m_cmdElement, "P2PVersion", req->usP2PVersion))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

	ParseXmlChildUint8(m_cmdElement, "RefuseRelay", req->ucRefuseRelay, 0);

	// to be compatible version  "0.1.0.1"
	if (strcmp(m_version, NAT_TRAVERSAL_XML_VER_0_1_0_1) == 0)
	{
		NAT_DEVICE_EXTRA_INFO deviceExtraInfo;
		memset(&deviceExtraInfo, 0, sizeof(deviceExtraInfo));
		ParseXmlChildUint32(m_cmdElement, "DeviceType", deviceExtraInfo.deviceType, 0);
		ParseXmlChildStr(m_cmdElement, "DeviceVersion", deviceExtraInfo.deviceVersion, sizeof(deviceExtraInfo.deviceVersion), "");
		ParseXmlChildIpPort(m_cmdElement, "DeviceWebPort", deviceExtraInfo.deviceWebPort, 0);
		ParseXmlChildIpPort(m_cmdElement, "DeviceDataPort", deviceExtraInfo.deviceDataPort, 0);
		CNatTraversalXmlPacker::PackDeviceExtraInfo(&deviceExtraInfo, req->caExtraInfo, sizeof(req->caExtraInfo));
	}
	else
	{
		ParseXmlChildStr(m_cmdElement, "ExtraInfo", req->caExtraInfo, sizeof(req->caExtraInfo), "");
	}


    return PARSE_OK;
}

CNatTraversalXmlParser::ParseError CNatTraversalXmlParser::Parse_DeviceRegisterReply(NAT_DEVICE_REGISTER_REPLY *reply)
{
    assert(m_cmdElement != NULL);

    if(!ParseXmlChildUint32(m_cmdElement, "RequestSeq", reply->stHeader.dwRequestSeq))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    if(!ParseXmlChildUint32(m_cmdElement, "Status", reply->stHeader.dwStatus))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }


    if(reply->stHeader.dwStatus != REPLY_STATUS_OK)
    {
        reply->dwPeerIp = 0;
        reply->dwPeerPort = 0;
        return PARSE_OK; //设备异常，不需解析下面字段
    }

    if(!ParseXmlChildIpAddr(m_cmdElement, "PeerIp", reply->dwPeerIp))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }


    if(!ParseXmlChildIpPort(m_cmdElement, "PeerPort", reply->dwPeerPort))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    return PARSE_OK;
    
}

CNatTraversalXmlParser::ParseError CNatTraversalXmlParser::Parse_ClientConnectP2PReq(
    NAT_CLIENT_CONNECT_P2P_REQ *req)
{
    assert(m_cmdElement != NULL);

    if(!ParseXmlChildUint32(m_cmdElement, "RequestSeq", req->stHeader.dwRequestSeq))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    if(!ParseXmlChildStr(m_cmdElement, "DeviceNo", req->dwDeviceNo, sizeof(req->dwDeviceNo)))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    if(!ParseXmlChildUint8(m_cmdElement, "RequestPeerNat", req->ucRequestPeerNat))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }


    if(!ParseXmlChildP2PVer(m_cmdElement, "P2PVersion", req->usP2PVersion))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }


    if(!ParseXmlChildUint32(m_cmdElement, "ConnectionId", req->dwConnectionId))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    return PARSE_OK;
}

CNatTraversalXmlParser::ParseError CNatTraversalXmlParser::Parse_ClientConnectP2PReply(
    NAT_CLIENT_CONNECT_P2P_REPLY *reply)
{
    assert(m_cmdElement != NULL);

    if(!ParseXmlChildUint32(m_cmdElement, "RequestSeq", reply->stHeader.dwRequestSeq))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    if(!ParseXmlChildUint32(m_cmdElement, "Status", reply->stHeader.dwStatus))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }


    if(reply->stHeader.dwStatus != REPLY_STATUS_OK)
    {
        reply->dwDevicePeerIp = 0;
        reply->usDevicePeerPort = 0;
        reply->ucDevicePeerNat = 0;
        return PARSE_OK; //设备异常，不需解析下面字段
    }

    if(!ParseXmlChildIpAddr(m_cmdElement, "DevicePeerIp", reply->dwDevicePeerIp))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }


    if(!ParseXmlChildIpPort(m_cmdElement, "DevicePeerPort", reply->usDevicePeerPort))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }


    if(!ParseXmlChildUint8(m_cmdElement, "DevicePeerNat", reply->ucDevicePeerNat))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    return PARSE_OK;
}

CNatTraversalXmlParser::ParseError CNatTraversalXmlParser::Parse_NotifyConnectP2PReq( NAT_NOTIFY_CONNECT_P2P_REQ *req )
{
    assert(m_cmdElement != NULL);

    if(!ParseXmlChildUint32(m_cmdElement, "RequestSeq", req->stHeader.dwRequestSeq))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    if(!ParseXmlChildIpAddr(m_cmdElement, "RequestPeerIp", req->dwRequestPeerIp))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    if(!ParseXmlChildIpPort(m_cmdElement, "RequestPeerPort", req->dwRequestPeerPort))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }


    if(!ParseXmlChildUint8(m_cmdElement, "RequestPeerNat", req->ucRequestPeerNat))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    if(!ParseXmlChildP2PVer(m_cmdElement, "P2PVersion", req->usP2PVersion))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    if(!ParseXmlChildUint32(m_cmdElement, "ConnectionId", req->dwConnectionId))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    return PARSE_OK;
}

CNatTraversalXmlParser::ParseError CNatTraversalXmlParser::Parse_NotifyConnectP2PReply( NAT_NOTIFY_CONNECT_P2P_REPLY *reply )
{
    assert(m_cmdElement != NULL);

    if(!ParseXmlChildUint32(m_cmdElement, "RequestSeq", reply->stHeader.dwRequestSeq))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    if(!ParseXmlChildUint32(m_cmdElement, "Status", reply->stHeader.dwStatus))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }
    return PARSE_OK;
}

CNatTraversalXmlParser::ParseError CNatTraversalXmlParser::Parse_ClientConnectRelayReq( NAT_CLIENT_CONNECT_RELAY_REQ *req )
{
    assert(m_cmdElement != NULL);

    if(!ParseXmlChildUint32(m_cmdElement, "RequestSeq", req->stHeader.dwRequestSeq))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    if(!ParseXmlChildStr(m_cmdElement, "DeviceNo", req->dwDeviceNo, sizeof(req->dwDeviceNo)))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    return PARSE_OK;
}

CNatTraversalXmlParser::ParseError CNatTraversalXmlParser::Parse_ClientConnectRelayReply(NAT_CLIENT_CONNECT_RELAY_REPLY *reply)
{
    assert(m_cmdElement != NULL);

    if(!ParseXmlChildUint32(m_cmdElement, "RequestSeq", reply->stHeader.dwRequestSeq))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    if(!ParseXmlChildUint32(m_cmdElement, "Status", reply->stHeader.dwStatus))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }



    if(reply->stHeader.dwStatus != REPLY_STATUS_OK)
    {
        reply->dwRelayServerIp = 0;
        reply->usRelayServerPort = 0;
        reply->ucRelayConnectionId = 0;
        return PARSE_OK; //设备异常，不需解析下面字段
    }


    if(!ParseXmlChildIpAddr(m_cmdElement, "RelayServerIp", reply->dwRelayServerIp))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }


    if(!ParseXmlChildIpPort(m_cmdElement, "RelayServerPort", reply->usRelayServerPort))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }


    if(!ParseXmlChildUint32(m_cmdElement, "RelayConnectionId", reply->ucRelayConnectionId))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    return PARSE_OK;
}

CNatTraversalXmlParser::ParseError CNatTraversalXmlParser::Parse_NotifyConnectRelayReq(NAT_NOTIFY_CONNECT_RELAY_REQ *req)
{
    assert(m_cmdElement != NULL);

    if(!ParseXmlChildUint32(m_cmdElement, "RequestSeq", req->stHeader.dwRequestSeq))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    if(!ParseXmlChildIpAddr(m_cmdElement, "RelayServerIp", req->dwRelayServerIp))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    if(!ParseXmlChildIpPort(m_cmdElement, "RelayServerPort", req->dwRelayServerPort))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    if(!ParseXmlChildUint32(m_cmdElement, "RelayConnectionId", req->dwConnectionId))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    return PARSE_OK;
}

CNatTraversalXmlParser::ParseError CNatTraversalXmlParser::Parse_NotifyConnectRelayReply( NAT_NOTIFY_CONNECT_RELAY_REPLY *reply )
{
    assert(m_cmdElement != NULL);

    if(!ParseXmlChildUint32(m_cmdElement, "RequestSeq", reply->stHeader.dwRequestSeq))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    if(!ParseXmlChildUint32(m_cmdElement, "Status", reply->stHeader.dwStatus))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    return PARSE_OK;
}

CNatTraversalXmlParser::ParseError CNatTraversalXmlParser::Parse_FetchDeviceReq(NAT_FETCH_DEVICE_REQ *req)
{
    assert(m_cmdElement != NULL);

    if(!ParseXmlChildUint32(m_cmdElement, "RequestSeq", req->stHeader.dwRequestSeq))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    if(!ParseXmlChildStr(m_cmdElement, "DeviceNo", req->dwDeviceNo, sizeof(req->dwDeviceNo)))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    return PARSE_OK;
}

CNatTraversalXmlParser::ParseError CNatTraversalXmlParser::Parse_FetchDeviceReply(NAT_FETCH_DEVICE_REPLY *reply)
{
    assert(m_cmdElement != NULL);

    if(!ParseXmlChildUint32(m_cmdElement, "RequestSeq", reply->stHeader.dwRequestSeq))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    if(!ParseXmlChildUint32(m_cmdElement, "Status", reply->stHeader.dwStatus))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    if(reply->stHeader.dwStatus != REPLY_STATUS_OK)
    {
        reply->dwDevicePeerIp = 0;
        reply->usDevicePeerPort = 0;
        reply->ucCanRelay = 0;
        reply->caExtraInfo[0] = 0;
        return PARSE_OK; //设备异常，不需解析下面字段
    }

    if(!ParseXmlChildIpAddr(m_cmdElement, "DevicePeerIp", reply->dwDevicePeerIp))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }
    if(!ParseXmlChildIpPort(m_cmdElement, "DevicePeerPort", reply->usDevicePeerPort))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

    if(!ParseXmlChildUint8(m_cmdElement, "CanRelay", reply->ucCanRelay))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }

	// to be compatible version  "0.1.0.1"
	assert(strcmp(m_version, NAT_TRAVERSAL_XML_VER_0_1_0_1) != 0 && "Not support version 0.1.0.1");	
	std::string extraText = "";
	ParseXmlChildStr(m_cmdElement, "ExtraInfo", reply->caExtraInfo, sizeof(reply->caExtraInfo), "");

    return PARSE_OK;
}

CNatTraversalXmlParser::ParseError CNatTraversalXmlParser::ParseTraversalHeartbeat( NAT_TRAVERSAL_HEARTBEAT *cmd )
{

    if(!ParseXmlChildStr(m_cmdElement, "DeviceNo", cmd->uaDeviceNo, sizeof(cmd->uaDeviceNo)))
    {
        return PARSE_CMD_FORMAT_ERROR;
    }
    return PARSE_OK;
}

const char* CNatTraversalXmlParser::GetParseErrorText( ParseError error )
{
	char* ret = "Parse unknown error";
	switch(error)
	{
		
	case PARSE_OK:
		ret = "Parse OK";
		break;
	case PARSE_UNKOWN:
		break;
	case PARSE_VERSION_INVLID:
		ret = "Parse version invalid";
		break;
	case PARSE_NO_CMD:
		ret = "Parse no cmd";
		break;
	case PARSE_UNKOWN_ID:
		ret = "Parse unknown id";
		break;
	case PARSE_CMD_FORMAT_ERROR:
		ret = "Parse format error";
		break;
	case PARSE_MEMORY_USED_UP:
		ret = "Parse memory used up?";
		break;
	}
	return ret;
}

bool CNatTraversalXmlParser::ParseDeviceExtraInfo(NAT_DEVICE_EXTRA_INFO* deviceExtraInfo, const char *text)
{
	memset(deviceExtraInfo, 0, sizeof(NAT_DEVICE_EXTRA_INFO));
	NatXmlDocument doc;
	doc.Parse(text);
	NatXmlElement *rootElement = doc.RootElement();
	if (NULL == rootElement)
	{
		return false;
	}

	ParseXmlChildUint32(rootElement, "DeviceType", deviceExtraInfo->deviceType, 0);
	ParseXmlChildStr(rootElement, "DeviceVersion", deviceExtraInfo->deviceVersion, sizeof(deviceExtraInfo->deviceVersion), "");
	ParseXmlChildUint16(rootElement, "DeviceWebPort", deviceExtraInfo->deviceWebPort, 0);
	ParseXmlChildUint16(rootElement, "DeviceDataPort", deviceExtraInfo->deviceDataPort, 0);	
	return true;
}

////////////////////////////////////////////////////////////////////////////////
//class CNatTraversalXmlPacker implements 

CNatTraversalXmlPacker::CNatTraversalXmlPacker()
:
m_isPcked(false),
m_xmlDataLen(0)
{
	m_version[0] = 0;
}

CNatTraversalXmlPacker::~CNatTraversalXmlPacker()
{

}

void CNatTraversalXmlPacker::Reset()
{
    Reset(NAT_TRAVERSAL_XML_VERSION);
}


void CNatTraversalXmlPacker::Reset(const char* version)
{
	m_isPcked = false;
	m_xmlDataLen = 0;
	if (NULL == version )
	{
		version = NAT_TRAVERSAL_XML_VERSION;
	}
	assert(strlen(version) < NAT_TRAVERSAL_XML_VER_LEN_MAX);
	snprintf(m_version, NAT_TRAVERSAL_XML_VER_LEN_MAX, "%s", version);	
}

bool CNatTraversalXmlPacker::PackCmd( NatTraversalCmdId cmdId, void* cmd, int iCmdLen )
{
    assert(iCmdLen > 0);

    NatXmlDocument m_xmlDoc;
    NatXmlPrinter  printer;
    printer.SetStreamPrinting();
    NatXmlElement *rootElement;
    NatXmlElement *cmdElement;

    rootElement = AddXmlRootElement(&m_xmlDoc, "Nat");
    if (NULL == rootElement)
    {
        return false;
    }
    rootElement->SetAttribute("version", m_version);

    cmdElement = AddXmlChildElement(rootElement, "Cmd");
    if (NULL == cmdElement)
    {
        return false;
    }
    cmdElement->SetAttribute("id", cmdId);

    bool ret = false;
    switch(cmdId)
    {
        /* 请求命令 */
    case NAT_ID_DEVICE_REGISTER_REQ:
        assert(iCmdLen == sizeof(NAT_DEVICE_REGISTER_REQ));
        ret = Pack_DeviceRegisterReq(cmdElement, reinterpret_cast<NAT_DEVICE_REGISTER_REQ*>(cmd));
        break;

    case NAT_ID_CLIENT_CONNECT_P2P_REQ:
        assert(iCmdLen == sizeof(NAT_CLIENT_CONNECT_P2P_REQ));
        ret = Pack_ClientConnectP2PReq(cmdElement, reinterpret_cast<NAT_CLIENT_CONNECT_P2P_REQ*>(cmd));
        break;
    case NAT_ID_NOTIFY_CONNECT_P2P_REQ:
        assert(iCmdLen == sizeof(NAT_NOTIFY_CONNECT_P2P_REQ));
        ret = Pack_NotifyConnectP2PReq(cmdElement, reinterpret_cast<NAT_NOTIFY_CONNECT_P2P_REQ*>(cmd));
        break;

    case NAT_ID_CLIENT_CONNECT_RELAY_REQ:
        assert(iCmdLen == sizeof(NAT_CLIENT_CONNECT_RELAY_REQ));
        ret = Pack_ClientConnectRelayReq(cmdElement, reinterpret_cast<NAT_CLIENT_CONNECT_RELAY_REQ*>(cmd));
        break;
    case NAT_ID_NOTIFY_CONNECT_RELAY_REQ:
        assert(iCmdLen == sizeof(NAT_NOTIFY_CONNECT_RELAY_REQ));
        ret = Pack_NotifyConnectRelayReq(cmdElement, reinterpret_cast<NAT_NOTIFY_CONNECT_RELAY_REQ*>(cmd));
        break;
    case NAT_ID_FETCH_DEVICE_REQ:
        assert(iCmdLen == sizeof(NAT_FETCH_DEVICE_REQ));
        ret = Pack_FetchDeviceReq(cmdElement, reinterpret_cast<NAT_FETCH_DEVICE_REQ*>(cmd));        
        break;

        /* 应答命令 */
    case NAT_ID_DEVICE_REGISTER_REPLY:
        assert(iCmdLen == sizeof(NAT_DEVICE_REGISTER_REPLY));
        ret = Pack_DeviceRegisterReply(cmdElement, reinterpret_cast<NAT_DEVICE_REGISTER_REPLY*>(cmd));
        break;
    case NAT_ID_CLIENT_CONNECT_P2P_REPLY:
        assert(iCmdLen == sizeof(NAT_CLIENT_CONNECT_P2P_REPLY));
        ret = Pack_ClientConnectP2PReply(cmdElement, reinterpret_cast<NAT_CLIENT_CONNECT_P2P_REPLY*>(cmd));
        break;
    case NAT_ID_NOTIFY_CONNECT_P2P_REPLY:
        assert(iCmdLen == sizeof(NAT_NOTIFY_CONNECT_P2P_REPLY));
        ret = Pack_NotifyConnectP2PReply(cmdElement, reinterpret_cast<NAT_NOTIFY_CONNECT_P2P_REPLY*>(cmd));
        break;

    case NAT_ID_CLIENT_CONNECT_RELAY_REPLY:
        assert(iCmdLen == sizeof(NAT_CLIENT_CONNECT_RELAY_REPLY));
        ret = Pack_ClientConnectRelayReply(cmdElement, reinterpret_cast<NAT_CLIENT_CONNECT_RELAY_REPLY*>(cmd));
        break;
    case NAT_ID_NOTIFY_CONNECT_RELAY_REPLY:
        assert(iCmdLen == sizeof(NAT_NOTIFY_CONNECT_RELAY_REPLY));
        ret = Pack_NotifyConnectRelayReply(cmdElement, reinterpret_cast<NAT_NOTIFY_CONNECT_RELAY_REPLY*>(cmd));        
        break;
    case NAT_ID_FETCH_DEVICE_REPLY:
        assert(iCmdLen == sizeof(NAT_FETCH_DEVICE_REPLY));
        ret = Pack_FetchDeviceReply(cmdElement, reinterpret_cast<NAT_FETCH_DEVICE_REPLY*>(cmd));  
        break;

    case NAT_ID_HEARTBEAT:
        assert(iCmdLen == sizeof(NAT_TRAVERSAL_HEARTBEAT));
        ret = Pack_TraversalHeartbeat(cmdElement, reinterpret_cast<NAT_TRAVERSAL_HEARTBEAT*>(cmd));
        break;
    case NAT_ID_DEVICE_UNREGISTER:
        // TODO: ...
        break;
    default:
        break;
    }

    if (ret)
    {
        m_xmlDoc.Accept(&printer);
        m_xmlDataLen = (int)printer.Size() + 1;
        assert(m_xmlDataLen <= MAX_BUF_SIZE);
        memcpy(m_buf, printer.CStr(), m_xmlDataLen);
    }
    m_isPcked = ret;
    return ret;

}


bool CNatTraversalXmlPacker::IsPacked()
{
    return m_isPcked;
}

char* CNatTraversalXmlPacker::GetXmlData()
{
    if (m_isPcked)
    {
        return (char*)&m_buf;
    }
    return NULL;
}

int CNatTraversalXmlPacker::GetXmlDataLen()
{
    if (m_isPcked)
    {
        return m_xmlDataLen;
    }
    return 0;
}

bool CNatTraversalXmlPacker::Pack_DeviceRegisterReq( NatXmlElement *cmdElement,
    NAT_DEVICE_REGISTER_REQ *req)
{
    if(!AddXmlChildUint32(cmdElement, "RequestSeq", req->stHeader.dwRequestSeq))
    {
        return false;
    }
    if(!AddXmlChildStr(cmdElement, "DeviceNo", req->caDeviceNo))
    {
        return false;
    }

	if(!AddXmlChildP2PVer(cmdElement, "P2PVersion", req->usP2PVersion))
	{
		return false;
	}
	if(!AddXmlChildIpPort(cmdElement, "RefuseRelay", req->ucRefuseRelay))
	{
		return false;
	}

	assert(strcmp(m_version, NAT_TRAVERSAL_XML_VER_0_1_0_1) != 0 && "Not support version 0.1.0.1");
	if(!AddXmlChildCdata(cmdElement, "ExtraInfo", req->caExtraInfo))
	{
		return false;
	}

    return true;
}

bool CNatTraversalXmlPacker::Pack_DeviceRegisterReply(NatXmlElement *cmdElement, NAT_DEVICE_REGISTER_REPLY *reply)
{
    if(!AddXmlChildUint32(cmdElement, "RequestSeq", reply->stHeader.dwRequestSeq))
    {
        return false;
    }
    if(!AddXmlChildUint32(cmdElement, "Status", reply->stHeader.dwStatus))
    {
        return false;
    }
    if(!AddXmlChildIpAddr(cmdElement, "PeerIp", reply->dwPeerIp))
    {
        return false;
    }

    if(!AddXmlChildIpPort(cmdElement, "PeerPort", reply->dwPeerPort))
    {
        return false;
    }

    return true;
}

bool CNatTraversalXmlPacker::Pack_ClientConnectP2PReq(NatXmlElement *cmdElement, NAT_CLIENT_CONNECT_P2P_REQ *req)
{
    if(!AddXmlChildUint32(cmdElement, "RequestSeq", req->stHeader.dwRequestSeq))
    {
        return false;
    }
    if(!AddXmlChildStr(cmdElement, "DeviceNo", req->dwDeviceNo))
    {
        return false;
    }
    if(!AddXmlChildUint8(cmdElement, "RequestPeerNat", req->ucRequestPeerNat))
    {
        return false;
    }
    if(!AddXmlChildP2PVer(cmdElement, "P2PVersion", req->usP2PVersion))
    {
        return false;
    }
    if(!AddXmlChildUint32(cmdElement, "ConnectionId", req->dwConnectionId))
    {
        return false;
    }

    return true;
}

bool CNatTraversalXmlPacker::Pack_ClientConnectP2PReply( NatXmlElement *cmdElement, NAT_CLIENT_CONNECT_P2P_REPLY *reply )
{
    if(!AddXmlChildUint32(cmdElement, "RequestSeq", reply->stHeader.dwRequestSeq))
    {
        return false;
    }
    if(!AddXmlChildUint32(cmdElement, "Status", reply->stHeader.dwStatus))
    {
        return false;
    }
    if(!AddXmlChildIpAddr(cmdElement, "DevicePeerIp", reply->dwDevicePeerIp))
    {
        return false;
    }

    if(!AddXmlChildIpPort(cmdElement, "DevicePeerPort", reply->usDevicePeerPort))
    {
        return false;
    }

    if(!AddXmlChildUint8(cmdElement, "DevicePeerNat", reply->ucDevicePeerNat))
    {
        return false;
    }

    return true;
}


bool CNatTraversalXmlPacker::Pack_NotifyConnectP2PReq( NatXmlElement *cmdElement, NAT_NOTIFY_CONNECT_P2P_REQ *req )
{
    if(!AddXmlChildUint32(cmdElement, "RequestSeq", req->stHeader.dwRequestSeq))
    {
        return false;
    }
    if(!AddXmlChildIpAddr(cmdElement, "RequestPeerIp", req->dwRequestPeerIp))
    {
        return false;
    }
    if(!AddXmlChildIpPort(cmdElement, "RequestPeerPort", req->dwRequestPeerPort))
    {
        return false;
    }

    if(!AddXmlChildUint8(cmdElement, "RequestPeerNat", req->ucRequestPeerNat))
    {
        return false;
    }

    if(!AddXmlChildP2PVer(cmdElement, "P2PVersion", req->usP2PVersion))
    {
        return false;
    }
    if(!AddXmlChildUint32(cmdElement, "ConnectionId", req->dwConnectionId))
    {
        return false;
    }

    return true;
}

bool CNatTraversalXmlPacker::Pack_NotifyConnectP2PReply( NatXmlElement *cmdElement, NAT_NOTIFY_CONNECT_P2P_REPLY *reply )
{
    if(!AddXmlChildUint32(cmdElement, "RequestSeq", reply->stHeader.dwRequestSeq))
    {
        return false;
    }
    if(!AddXmlChildUint32(cmdElement, "Status", reply->stHeader.dwStatus))
    {
        return false;
    }

    return true;
}

bool CNatTraversalXmlPacker::Pack_ClientConnectRelayReq(NatXmlElement *cmdElement, NAT_CLIENT_CONNECT_RELAY_REQ *req)
{
    if(!AddXmlChildUint32(cmdElement, "RequestSeq", req->stHeader.dwRequestSeq))
    {
        return false;
    }
    if(!AddXmlChildStr(cmdElement, "DeviceNo", req->dwDeviceNo))
    {
        return false;
    }

    return true;
}

bool CNatTraversalXmlPacker::Pack_ClientConnectRelayReply( NatXmlElement *cmdElement, NAT_CLIENT_CONNECT_RELAY_REPLY *reply )
{
    if(!AddXmlChildUint32(cmdElement, "RequestSeq", reply->stHeader.dwRequestSeq))
    {
        return false;
    }
    if(!AddXmlChildUint32(cmdElement, "Status", reply->stHeader.dwStatus))
    {
        return false;
    }
    if(!AddXmlChildIpAddr(cmdElement, "RelayServerIp", reply->dwRelayServerIp))
    {
        return false;
    }

    if(!AddXmlChildIpPort(cmdElement, "RelayServerPort", reply->usRelayServerPort))
    {
        return false;
    }

    if(!AddXmlChildUint32(cmdElement, "RelayConnectionId", reply->ucRelayConnectionId))
    {
        return false;
    }
    return true;
}

bool CNatTraversalXmlPacker::Pack_NotifyConnectRelayReq( NatXmlElement *cmdElement, NAT_NOTIFY_CONNECT_RELAY_REQ *req )
{
    if(!AddXmlChildUint32(cmdElement, "RequestSeq", req->stHeader.dwRequestSeq))
    {
        return false;
    }
    if(!AddXmlChildIpAddr(cmdElement, "RelayServerIp", req->dwRelayServerIp))
    {
        return false;
    }
    if(!AddXmlChildIpPort(cmdElement, "RelayServerPort", req->dwRelayServerPort))
    {
        return false;
    }
    if(!AddXmlChildUint32(cmdElement, "RelayConnectionId", req->dwConnectionId))
    {
        return false;
    }
    return true;
}

bool CNatTraversalXmlPacker::Pack_NotifyConnectRelayReply(NatXmlElement *cmdElement, NAT_NOTIFY_CONNECT_RELAY_REPLY *reply)
{
    if(!AddXmlChildUint32(cmdElement, "RequestSeq", reply->stHeader.dwRequestSeq))
    {
        return false;
    }
    if(!AddXmlChildUint32(cmdElement, "Status", reply->stHeader.dwStatus))
    {
        return false;
    }

    return true;
}

bool CNatTraversalXmlPacker::Pack_FetchDeviceReq(NatXmlElement *cmdElement, NAT_FETCH_DEVICE_REQ *req)
{
    if(!AddXmlChildUint32(cmdElement, "RequestSeq", req->stHeader.dwRequestSeq))
    {
        return false;
    }
    if(!AddXmlChildStr(cmdElement, "DeviceNo", req->dwDeviceNo))
    {
        return false;
    }

    return true;
}

bool CNatTraversalXmlPacker::Pack_FetchDeviceReply(NatXmlElement *cmdElement, NAT_FETCH_DEVICE_REPLY *reply)
{
    if(!AddXmlChildUint32(cmdElement, "RequestSeq", reply->stHeader.dwRequestSeq))
    {
        return false;
    }
    if(!AddXmlChildUint32(cmdElement, "Status", reply->stHeader.dwStatus))
    {
        return false;
    }
    if(!AddXmlChildIpAddr(cmdElement, "DevicePeerIp", reply->dwDevicePeerIp))
    {
        return false;
    }

    if(!AddXmlChildIpPort(cmdElement, "DevicePeerPort", reply->usDevicePeerPort))
    {
        return false;
    }

    if(!AddXmlChildUint8(cmdElement, "CanRelay", reply->ucCanRelay))
    {
        return false;
    }

	// to be compatible version  "0.1.0.1"
	if (strcmp(m_version, NAT_TRAVERSAL_XML_VER_0_1_0_1) == 0)
	{
		NAT_DEVICE_EXTRA_INFO extraInfo;
		CNatTraversalXmlParser::ParseDeviceExtraInfo(&extraInfo, reply->caExtraInfo);
	
		if(!AddXmlChildUint32(cmdElement, "DeviceType", extraInfo.deviceType))
		{
			return false;
		}
		if(!AddXmlChildStr(cmdElement, "DeviceVersion", extraInfo.deviceVersion))
		{
			return false;
		}
		if(!AddXmlChildUint16(cmdElement, "DeviceWebPort", extraInfo.deviceWebPort))
		{
			return false;
		}
		if(!AddXmlChildUint16(cmdElement, "DeviceWebPort", extraInfo.deviceWebPort))
		{
			return false;
		}
		if(!AddXmlChildUint16(cmdElement, "CanDirect", 0))
		{
			return false;
		}
	}
	else
	{
		if(!AddXmlChildCdata(cmdElement, "ExtraInfo", reply->caExtraInfo))
		{
			return false;
		}
	}


    return true;
}

bool CNatTraversalXmlPacker::Pack_TraversalHeartbeat( NatXmlElement *cmdElement, NAT_TRAVERSAL_HEARTBEAT *cmd )
{
    if(!AddXmlChildStr(cmdElement, "DeviceNo", cmd->uaDeviceNo))
    {
        return false;
    }

    return true;
}


bool CNatTraversalXmlPacker::PackDeviceExtraInfo(const NAT_DEVICE_EXTRA_INFO* deviceExtraInfo, char *text, int size)
{
	assert(deviceExtraInfo != NULL && text != NULL && size > 0);
	char *buf = text;
	int retSize = 0 ;
	NatXmlPrinter  printer;
	retSize = snprintf(buf, size, "<Info>");
	size -= retSize;
	buf += retSize;
	retSize = snprintf(buf, size, "<DeviceType>%u</DeviceType>", deviceExtraInfo->deviceType);
	size -= retSize;
	buf += retSize;
	retSize = snprintf(buf, size, "<DeviceVersion>%s</DeviceVersion>", deviceExtraInfo->deviceVersion);
	size -= retSize;
	buf += retSize;
	retSize = snprintf(buf, size, "<DeviceWebPort>%u</DeviceWebPort>", deviceExtraInfo->deviceWebPort);
	size -= retSize;
	buf += retSize;
	retSize = snprintf(buf, size, "<DeviceDataPort>%u</DeviceDataPort>", deviceExtraInfo->deviceDataPort);
	size -= retSize;
	buf += retSize;
	retSize = snprintf(buf, size, "</Info>");
	size -= retSize;
	buf += retSize;
	return size > 0;
}