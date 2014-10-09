
#include "ManagerServer.h"
#include "TVT_PubCom.h"

CManagerServer::CManagerServer()
{
	m_nPort = 0;
}

CManagerServer::~CManagerServer()
{
	
}

bool CManagerServer::Initial(std::string strDeviceSN)
{
	m_strDeviceSN = strDeviceSN;
	return true;
}

void CManagerServer::Quit()
{
	m_msInfo.clear();
}

bool CManagerServer::SetNetInfo(const std::string &strIP, const unsigned short &nPort)
{
	m_strIP = strIP;
	m_nPort = nPort;

	return true;
}

bool CManagerServer::SetInfo(const std::string &strInfoName, const std::string &strInfoValue)
{
	m_msInfo[strInfoName] = strInfoValue;
	return true;
}

bool CManagerServer::GetNetInfo(std::string &strIP, unsigned short &nPort)
{
	strIP = m_strIP;
	nPort = m_nPort;

	return true;
}

bool CManagerServer::GetInfo(const std::string &strInfoName, std::string &strInfoValue)
{
	MS_INFO_MAP::iterator it = m_msInfo.find(strInfoName);
	if(it != m_msInfo.end())
	{
		strInfoValue = it->second;
		return true;
	}

	return false;
}