// NatXml.h: interface for the xml of nat.
//
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_XML_H_
#define _NAT_XML_H_

#include "tinyxml.h"

#ifdef  TIXML_STRING_INCLUDED
#define NAT_XML_NOTE_TEXT TiXmlNode::TINYXML_TEXT
#else
#define NAT_XML_NOTE_TEXT  TiXmlNode::TEXT
#endif

typedef TiXmlElement  NatXmlElement;
typedef TiXmlNode     NatXmlNode;
typedef TiXmlDocument NatXmlDocument;
typedef TiXmlPrinter  NatXmlPrinter;

////////////////////////////////////////////////////////////////////////////////
//Parse Xml interface

bool ParseXmlChildStr(NatXmlElement *parentElement, const char* childName, const char** value);

void ParseXmlChildStr(NatXmlElement *parentElement, const char* childName, const char** value, const char *defaultValue);

bool ParseXmlChildStr(NatXmlElement *parentElement, const char* childName, char* outValue, int maxLen);

void ParseXmlChildStr(NatXmlElement *parentElement, const char* childName, char* outValue, int maxLen, const char *defaultValue);

bool ParseXmlChildInt32(NatXmlElement *parentElement, const char* childName, int32 &value);

bool ParseXmlChildUint32(NatXmlElement *parentElement, const char* childName, uint32 &value);

void ParseXmlChildUint32(NatXmlElement *parentElement, const char* childName, uint32 &value, const uint32 defaultValue);

bool ParseXmlChildUint16(NatXmlElement *parentElement, const char* childName, uint16 &value);

void ParseXmlChildUint16(NatXmlElement *parentElement, const char* childName, uint16 &value, const uint16 defaultValue);

bool ParseXmlChildUint8(NatXmlElement *parentElement, const char* childName, uint8 &value);

void ParseXmlChildUint8(NatXmlElement *parentElement, const char* childName, uint8 &value, const uint8 defaultValue);

bool ParseXmlChildIpAddr(NatXmlElement *parentElement, const char* childName, uint32 &value);

bool ParseXmlChildIpPort(NatXmlElement *parentElement, const char* childName, uint16 &value);

void ParseXmlChildIpPort(NatXmlElement *parentElement, const char* childName, uint16 &value, const uint16 defaultValue);

bool ParseXmlChildOuterText(NatXmlElement *parentElement, const char* childName, std::string &outValue, bool append = true);

////////////////////////////////////////////////////////////////////////////////
// Create Xml interface

NatXmlElement* AddXmlChildElement(NatXmlNode *parentNode, const char* name );

NatXmlElement* AddXmlRootElement(NatXmlDocument *m_xmlDoc, const char* name );

bool AddXmlChildStr(NatXmlNode *parentNode, const char* name, char* value );

bool AddXmlChildInt32(NatXmlNode *parentNode, const char* name, int32 value );

bool AddXmlChildUint32(NatXmlNode *parentNode, const char* name, uint32 value );

bool AddXmlChildUint16(NatXmlNode *parentNode, const char* name, uint16 value );

bool AddXmlChildUint8(NatXmlNode *parentNode, const char* name, uint8 value );

bool AddXmlChildIpAddr(NatXmlNode *parentNode, const char* name, uint32 value );

bool AddXmlChildIpPort(NatXmlNode *parentNode, const char* name, uint16 value );

bool AddXmlChildCdata(NatXmlNode *parentNode, const char* name, char* value );

#endif//_NAT_XML_H_