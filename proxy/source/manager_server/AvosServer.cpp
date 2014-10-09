#include "AvosServer.h"
#include "json.h"
#include "TVTHttp.h"
#include "Base64.h"

unsigned char g_auth_code_seed[20] = {0xBE, 0xCD, 0xAC, 0xCE, 0xAA, 0xBC, 0xD2, 0xCD, 0xA5, 0xBB, 0xA5, 0xC1, 0xAA, 0xD6, 0xD0, 0xD0, 0xC4, 0xDC};

CAvosServer::CAvosServer(void)
{
	CBase64 base64;
	base64.Encode(g_auth_code_seed, 18, m_authCode, 28);
}


CAvosServer::~CAvosServer(void)
{
}

CAvosServer *CAvosServer::Instance()
{
	static CAvosServer s_instance;
	return &s_instance;
}

bool CAvosServer::SaveInfo()
{
	bool bRet = false;
	Json::Value req;

	req["cmd"] = "device_register";
	req["data"];
	req["data"]["device_sn"] = m_strDeviceSN;
	req["data"]["ip"] = m_strIP;
	req["data"]["port"] = m_nPort;
	req["data"]["auth_code"] = m_authCode;

	MS_INFO_MAP::iterator it = m_msInfo.begin();
	for(; it != m_msInfo.end(); it++)
	{
		req["data"][it->first] = it->second;
	}

	Json::FastWriter writer;
	std::string s = writer.write(req);

	CTVTHttp http;
	HTTP_BODY_INFO body;

	body.iBodyType = HTTP_BODY_TYPE_APP_JSON;
	body.iBodyBufLen = s.size();
	body.iBodyLen = s.size();
	body.iTotalBodyLen = s.size();
	body.pBodyBuf = &s[0];

	if (http.Post(AVOS_ACTION_URL, &body))
	{
		Json::Value res;
		Json::Reader reader;
		if (reader.parse(http.GetRetBody(), http.GetRetBody() + http.GetRetBodyLen(), res))
		{
			int code = res["code"].asInt();
			if(code == 0)
			{
				bRet = true;
			}
		}
		else
		{
		}
	}
	else
	{
	}

	body.pBodyBuf = NULL;

	return bRet;
}

bool CAvosServer::ReadInfo()
{
	bool bRet = false;
	Json::Value req;

	req["cmd"] = "device_query";
	req["data"];
	req["data"]["device_sn"] = m_strDeviceSN;

	Json::FastWriter writer;
	std::string s = writer.write(req);

	CTVTHttp http;
	HTTP_BODY_INFO body;

	body.iBodyType = HTTP_BODY_TYPE_APP_JSON;
	body.iBodyBufLen = s.size();
	body.iBodyLen = s.size();
	body.iTotalBodyLen = s.size();
	body.pBodyBuf = &s[0];

	if (http.Post(AVOS_ACTION_URL, &body))
	{
		Json::Value res;
		Json::Reader reader;
		if (reader.parse(http.GetRetBody(), http.GetRetBody() + http.GetRetBodyLen(), res))
		{
			int code = res["code"].asInt();
			if(0 == code)
			{
				Json::Value::Members members = res["data"].getMemberNames();
				m_nPort = res["data"]["port"].asInt();
				m_strIP = res["data"]["ip"].asString();
				
				for(int i = 0; i < members.size(); i++)
				{
					if((members[i] != "port") && (members[i] != "ip") && (members[i] != "device_sn"))
					{
						m_msInfo[members[i]] = res["data"][members[i]].asString();
					}
				}

				bRet = true;
			}
			else
			{
				printf("error code is %d\n", code);
			}
		}
		else
		{
			printf("parse json error : %s\n", http.GetRetBody());
		}
	}
	else
	{
	}

	body.pBodyBuf = NULL;

	return bRet;
}

int CAvosServer::CheckAuth(const std::string &account, const std::string &password)
{
	int nRet = -1;
	Json::Value req;

	req["cmd"] = "check_device_auth";
	req["data"];
	req["data"]["account"] = account;
	req["data"]["password"] = password;
	req["data"]["device_sn"] = m_strDeviceSN;

	Json::FastWriter writer;
	std::string s = writer.write(req);

	CTVTHttp http;
	HTTP_BODY_INFO body;

	body.iBodyType = HTTP_BODY_TYPE_APP_JSON;
	body.iBodyBufLen = s.size();
	body.iBodyLen = s.size();
	body.iTotalBodyLen = s.size();
	body.pBodyBuf = &s[0];

	if (http.Post(AVOS_ACTION_URL, &body))
	{
		Json::Value res;
		Json::Reader reader;
		if (reader.parse(http.GetRetBody(), http.GetRetBody() + http.GetRetBodyLen(), res))
		{
			int code = res["code"].asInt();
			std::string auth = res["data"]["auth"].asString();
			if((0 == code) && (auth == "auth_owner"))
			{
				nRet = 0;
			}
			else
			{
				printf("error code is %d, auth is %s\n", code, auth);
			}
		}
		else
		{
			printf("parse json error : %s\n", http.GetRetBody());
		}
	}
	else
	{
	}

	body.pBodyBuf = NULL;

	return nRet;
}

int CAvosServer::BindOwner(const std::string &strAccount, const std::string &password)
{
	int nRet = -1;
	Json::Value req;

	req["cmd"] = "add_device";
	req["data"];
	req["data"]["account"] = strAccount;
	req["data"]["password"] = password;
	req["data"]["device_sn"] = m_strDeviceSN;

	Json::FastWriter writer;
	std::string s = writer.write(req);

	CTVTHttp http;
	HTTP_BODY_INFO body;

	body.iBodyType = HTTP_BODY_TYPE_APP_JSON;
	body.iBodyBufLen = s.size();
	body.iBodyLen = s.size();
	body.iTotalBodyLen = s.size();
	body.pBodyBuf = &s[0];

	if (http.Post(AVOS_ACTION_URL, &body))
	{
		Json::Value res;
		Json::Reader reader;
		if (reader.parse(http.GetRetBody(), http.GetRetBody() + http.GetRetBodyLen(), res))
		{
			int code = res["code"].asInt();
			if(0 == code)
			{
				nRet = 0;
			}
			else
			{
				printf("error code is %d\n", code);
			}
		}
		else
		{
		}
	}
	else
	{
	}

	body.pBodyBuf = NULL;

	return nRet;
}
