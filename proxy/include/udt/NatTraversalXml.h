// NatTraversalXml.h: interface for handling NatTraversalXml.
//
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_TRAVERSAL_XML_H_
#define _NAT_TRAVERSAL_XML_H_

#include "NatTraversalProtocol.h"
#include "NatRunner.h"
#include "NatXml.h"

bool ParseXmlChildP2PVer(NatXmlElement *parentElement, const char* childName, NAT_P2P_PROTOCOL_VER &value);

bool AddXmlChildP2PVer(NatXmlNode *parentNode, const char* name, const NAT_P2P_PROTOCOL_VER &value );

class CNatTraversalXmlParser
{
public:
    static const int MAX_BUF_SIZE = NAT_DATA_BLOCK_MAX_SIZE;
public:
    enum ParseError
    {
        PARSE_OK                = 0,
        PARSE_UNKOWN            = 1,
        PARSE_VERSION_INVLID    = 2,
        PARSE_NO_CMD            = 3,
        PARSE_UNKOWN_ID         = 4,
        PARSE_CMD_FORMAT_ERROR  = 5,
        PARSE_MEMORY_USED_UP    = 6
    };
	static const char* GetParseErrorText(ParseError error);
	static bool ParseDeviceExtraInfo(NAT_DEVICE_EXTRA_INFO* deviceExtraInfo, const char *text);
public:
    CNatTraversalXmlParser();
	~CNatTraversalXmlParser();
	ParseError Reset(const char* pDataBlock, int iDataBlockLen);
    ParseError CheckHeader(NatTraversalCmdId &CmdId);
    ParseError Execute();
    NatTraversalCmdId GetCmdId();
    void* GetCmd();
    ParseError GetError();
	const char* GetVersion() {return m_version;};
private:    
    ParseError Parse_DeviceRegisterReq(NAT_DEVICE_REGISTER_REQ *req);
    ParseError Parse_DeviceRegisterReply(NAT_DEVICE_REGISTER_REPLY *reply);

    ParseError Parse_ClientConnectP2PReq(NAT_CLIENT_CONNECT_P2P_REQ *req);
    ParseError Parse_ClientConnectP2PReply(NAT_CLIENT_CONNECT_P2P_REPLY *reply);

    ParseError Parse_NotifyConnectP2PReq(NAT_NOTIFY_CONNECT_P2P_REQ *req);
    ParseError Parse_NotifyConnectP2PReply(NAT_NOTIFY_CONNECT_P2P_REPLY *reply);

    ParseError Parse_ClientConnectRelayReq(NAT_CLIENT_CONNECT_RELAY_REQ *req);
    ParseError Parse_ClientConnectRelayReply(NAT_CLIENT_CONNECT_RELAY_REPLY *reply);

    ParseError Parse_NotifyConnectRelayReq(NAT_NOTIFY_CONNECT_RELAY_REQ *req);
    ParseError Parse_NotifyConnectRelayReply(NAT_NOTIFY_CONNECT_RELAY_REPLY *reply);

    ParseError Parse_FetchDeviceReq(NAT_FETCH_DEVICE_REQ *req);
    ParseError Parse_FetchDeviceReply(NAT_FETCH_DEVICE_REPLY *reply);
    
    ParseError ParseTraversalHeartbeat(NAT_TRAVERSAL_HEARTBEAT *cmd);        
private:
    NatXmlDocument *m_xmlDoc;
    ParseError m_error;
    NatTraversalCmdId m_cmdId;
    char m_cmd[MAX_BUF_SIZE];
    int m_cmdSize;
	NatXmlElement *m_cmdElement;
	char m_version[NAT_TRAVERSAL_XML_VER_LEN_MAX];
};


class CNatTraversalXmlPacker
{
public:
    static const int MAX_BUF_SIZE = NAT_DATA_BLOCK_MAX_SIZE;
public:
	static bool PackDeviceExtraInfo(const NAT_DEVICE_EXTRA_INFO* deviceExtraInfo, char *text, int size);
public:
    CNatTraversalXmlPacker();
	~CNatTraversalXmlPacker();
	void Reset();
	void Reset(const char* version);
    bool PackCmd(NatTraversalCmdId cmdId, void* cmd, int iCmdLen);
    bool IsPacked();
    char* GetXmlData();
    int GetXmlDataLen();
private:
    bool Pack_DeviceRegisterReq(NatXmlElement *cmdElement, NAT_DEVICE_REGISTER_REQ *req);
    bool Pack_DeviceRegisterReply(NatXmlElement *cmdElement, NAT_DEVICE_REGISTER_REPLY *reply);    

    bool Pack_ClientConnectP2PReq(NatXmlElement *cmdElement, NAT_CLIENT_CONNECT_P2P_REQ *req);
    bool Pack_ClientConnectP2PReply(NatXmlElement *cmdElement, NAT_CLIENT_CONNECT_P2P_REPLY *reply);

    bool Pack_NotifyConnectP2PReq(NatXmlElement *cmdElement, NAT_NOTIFY_CONNECT_P2P_REQ *req);
    bool Pack_NotifyConnectP2PReply(NatXmlElement *cmdElement, NAT_NOTIFY_CONNECT_P2P_REPLY *reply);

    bool Pack_ClientConnectRelayReq(NatXmlElement *cmdElement, NAT_CLIENT_CONNECT_RELAY_REQ *req);
    bool Pack_ClientConnectRelayReply(NatXmlElement *cmdElement, NAT_CLIENT_CONNECT_RELAY_REPLY *reply);

    bool Pack_NotifyConnectRelayReq(NatXmlElement *cmdElement, NAT_NOTIFY_CONNECT_RELAY_REQ *req);
    bool Pack_NotifyConnectRelayReply(NatXmlElement *cmdElement, NAT_NOTIFY_CONNECT_RELAY_REPLY *reply);

    bool Pack_FetchDeviceReq(NatXmlElement *cmdElement, NAT_FETCH_DEVICE_REQ *req);
    bool Pack_FetchDeviceReply(NatXmlElement *cmdElement, NAT_FETCH_DEVICE_REPLY *reply);
    
    
    bool Pack_TraversalHeartbeat(NatXmlElement *cmdElement, NAT_TRAVERSAL_HEARTBEAT *cmd);
    
private:
    char m_buf[MAX_BUF_SIZE];
    bool m_isPcked;
    int m_xmlDataLen;
	char m_version[NAT_TRAVERSAL_XML_VER_LEN_MAX];
};

#endif//_NAT_TRAVERSAL_XML_H_