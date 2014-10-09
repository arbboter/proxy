// NatXml.cpp: implements for the NatXml.
//
//////////////////////////////////////////////////////////////////////

#include "NatCommon.h"
#include "NatXml.h"

#include "NatDebug.h"

bool ParseXmlChildStr(NatXmlElement *parentElement, const char* childName, const char** value)
{
    NatXmlElement *childElement = parentElement->FirstChildElement(childName);

    if (NULL == childElement)
    {
        return false;
    }
    TiXmlNode *valueNode = childElement->FirstChild();

    if (NULL == valueNode)
    {
        return false;
    }
    if(valueNode->Type() != NAT_XML_NOTE_TEXT)
    {
        return false;
    }

    *value = valueNode->ToText()->Value();
    if (NULL == value)
    {
        return false;
    }    
    return true;
}

void ParseXmlChildStr(NatXmlElement *parentElement, const char* childName, const char** value, const char *defaultValue)
{
    if (!ParseXmlChildStr(parentElement, childName, value))
    {
        *value = defaultValue;
    }
}

bool ParseXmlChildStr(NatXmlElement *parentElement, const char* childName, char* outValue, int maxLen)
{
    const char *valueStr = NULL;
    bool ret = ParseXmlChildStr(parentElement, childName, &valueStr);
    if (ret)
    {
        snprintf(outValue, maxLen, "%s", valueStr);
    } 
    return ret;
}

void ParseXmlChildStr(NatXmlElement *parentElement, const char* childName, char* outValue, int maxLen, const char *defaultValue)
{
    if (!ParseXmlChildStr(parentElement, childName, outValue, maxLen))
    {
        assert((int)strlen(defaultValue) < maxLen);
        strcpy(outValue, defaultValue);
    }
}

bool ParseXmlChildInt32(NatXmlElement *parentElement, const char* childName, int32 &value)
{        
    const char *valueStr = NULL;
    bool ret = ParseXmlChildStr(parentElement, childName, &valueStr);
    if (ret)
    {
        value = atoi(valueStr);
    }
    return ret;
}


bool ParseXmlChildUint32(NatXmlElement *parentElement, const char* childName, uint32 &value)
{
    const char *valueStr = NULL;
    bool ret = ParseXmlChildStr(parentElement, childName, &valueStr);
    if (ret)
    {
        value = strtoul(valueStr, NULL, 10);
    }
    return ret;
}

void ParseXmlChildUint32(NatXmlElement *parentElement, const char* childName, uint32 &value, const uint32 defaultValue)
{
	if(!ParseXmlChildUint32(parentElement, childName, value))
	{
		value = defaultValue;
	}
}

bool ParseXmlChildUint16(NatXmlElement *parentElement, const char* childName, uint16 &value)
{
    const char *valueStr = NULL;
    bool ret = ParseXmlChildStr(parentElement, childName, &valueStr);
    if (ret)
    {
        value = (uint16)strtoul(valueStr, NULL, 10);
    }
    return ret;
}

void ParseXmlChildUint16(NatXmlElement *parentElement, const char* childName, uint16 &value, const uint16 defaultValue)
{
	if(!ParseXmlChildUint16(parentElement, childName, value))
	{
		value = defaultValue;
	}
}

bool ParseXmlChildUint8(NatXmlElement *parentElement, const char* childName, uint8 &value)
{
    const char *valueStr = NULL;
    bool ret = ParseXmlChildStr(parentElement, childName, &valueStr);
    if (ret)
    {
        value = (uint8)strtoul(valueStr, NULL, 10);
    }
    return ret;
}

void ParseXmlChildUint8(NatXmlElement *parentElement, const char* childName, uint8 &value, const uint8 defaultValue)
{
    if(!ParseXmlChildUint8(parentElement, childName, value))
    {
        value = defaultValue;
    }
}

bool ParseXmlChildIpAddr(NatXmlElement *parentElement, const char* childName, uint32 &value)
{
    const char *valueStr = NULL;
    bool ret = ParseXmlChildStr(parentElement, childName, &valueStr);
    if (ret)
    {
        value = inet_addr(valueStr);
    }
    return ret;
}

bool ParseXmlChildIpPort(NatXmlElement *parentElement, const char* childName, uint16 &value)
{
    return ParseXmlChildUint16(parentElement, childName, value);
}

void ParseXmlChildIpPort(NatXmlElement *parentElement, const char* childName, uint16 &value, const uint16 defaultValue)
{
    if(!ParseXmlChildIpPort(parentElement, childName, value))
    {
        value = defaultValue;
    }
}

bool ParseXmlChildOuterText(NatXmlElement *parentElement, const char* childName, std::string &outValue, bool append/* = true*/)
{
	NatXmlElement *childElement = parentElement->FirstChildElement(childName);

	if (NULL == childElement)
	{
		return false;
	}
	NatXmlPrinter printer;
	printer.SetStreamPrinting();
	childElement->Accept(&printer);
	if (append)
	{
		outValue += printer.CStr();
	}
	else
	{
		outValue = printer.CStr();
	}
  
	return true;
}

////////////////////////////////////////////////////////////////////////////////
// Create Xml implements

NatXmlElement* AddXmlChildElement(TiXmlNode *parentNode, const char* name )
{
    NatXmlElement* newElement = NULL;
    TiXmlNode* newNode = parentNode->InsertEndChild(NatXmlElement(name));
    if (newNode != NULL)
    {
        //assert(newNode );
        newElement = dynamic_cast<NatXmlElement*>(newNode);
    }

    return newElement;
}

NatXmlElement* AddXmlRootElement(NatXmlDocument *m_xmlDoc, const char* name )
{
    return AddXmlChildElement(m_xmlDoc, name);
}


bool AddXmlChildStr(TiXmlNode *parentNode, const char* name, char* value )
{
    bool ret = false;
    NatXmlElement* xmlElement = AddXmlChildElement(parentNode, name);
    if (xmlElement != NULL)
    {
        TiXmlNode *xmlNode = xmlElement->InsertEndChild(TiXmlText(value));
        ret = xmlNode != NULL;
    }
    return ret;
}

bool AddXmlChildInt32(TiXmlNode *parentNode, const char* name, int32 value )
{
    static const int32 MAX_BUF_SIZE = 64;
    char buf[MAX_BUF_SIZE] = {0};
    snprintf(buf, MAX_BUF_SIZE-1, "%d", value);
    return AddXmlChildStr(parentNode, name, buf);
}

bool AddXmlChildUint32(TiXmlNode *parentNode, const char* name, uint32 value )
{
    static const int32 MAX_BUF_SIZE = 64;
    char buf[MAX_BUF_SIZE] = {0};
    snprintf(buf, MAX_BUF_SIZE-1, "%u", value);
    return AddXmlChildStr(parentNode, name, buf);
}

bool AddXmlChildUint16(TiXmlNode *parentNode, const char* name, uint16 value )
{
    return AddXmlChildUint32(parentNode, name, value);
}

bool AddXmlChildUint8(TiXmlNode *parentNode, const char* name, uint8 value )
{
    return AddXmlChildUint32(parentNode, name, value);
}

bool AddXmlChildIpAddr(TiXmlNode *parentNode, const char* name, uint32 value )
{    
    return AddXmlChildStr(parentNode, name, Nat_inet_ntoa(value));
}

bool AddXmlChildIpPort(TiXmlNode *parentNode, const char* name, uint16 value )
{
    return AddXmlChildInt32(parentNode, name, value);
}

bool AddXmlChildCdata(TiXmlNode *parentNode, const char* name, char* value )
{
	bool ret = false;
	NatXmlElement* xmlElement = AddXmlChildElement(parentNode, name);
	if (xmlElement != NULL)
	{
		TiXmlText textNode = TiXmlText(value);
		textNode.SetCDATA(true);
		TiXmlNode *xmlNode = xmlElement->InsertEndChild(textNode);
		ret = xmlNode != NULL;
	}
	return ret;
}

