
#ifndef _MANAGER_SERVER_H_
#define _MANAGER_SERVER_H_

#include <string>
#include <map>

/***********************************************************
该类提供设备或客户端向管理平台存取设备信息的接口。
由于平台的多样性，造成了对不同平台有不同的具体存取方式，
所以这里仅提供统一的存取接口，具体平台的存取需要在这个类的基础上具体化。
************************************************************/
typedef std::map<std::string, std::string> MS_INFO_MAP;
class CManagerServer
{
public:
	CManagerServer();
	virtual ~CManagerServer();

	//初始化，传入设备的序列号
	virtual bool Initial(std::string strDeviceSN);
	virtual void Quit();

	//设置设备的外网地址及端口信息，这里只是暂存在本地，并没有存到管理服务器
	virtual bool SetNetInfo(const std::string &strIP, const unsigned short &nPort);
	//设置设备的其他信息，每条信息由信息名及信息值组成，信息值限定为字符串
	virtual bool SetInfo(const std::string &strInfoName, const std::string &strInfoValue);

	//获取设备的外网地址及端口信息，这里不直接从服务器取，而是从这个类的对象中取
	virtual bool GetNetInfo(std::string &strIP, unsigned short &nPort);
	//获取指定名称的设备信息
	virtual bool GetInfo(const std::string &strInfoName, std::string &strInfoValue);

	//保存到服务器
	virtual bool SaveInfo() = 0;
	//从服务器读取
	virtual bool ReadInfo() = 0;

	//////////////////////////////////////////////////////////////////////////
	//以下为管理服务器的用户设备管理相关的接口

	//检查指定用户对该设备的访问权限，返回-1表示没有权限，0表示管理员权限，大于0表示受限的权限
	virtual int CheckAuth(const std::string &strAccount, const std::string &password){return -1;}
	virtual int BindOwner(const std::string &strAccount, const std::string &password){return -1;}

protected:
	std::string		m_strDeviceSN;
	std::string		m_strIP;
	unsigned short	m_nPort;
	MS_INFO_MAP		m_msInfo;
};

#endif