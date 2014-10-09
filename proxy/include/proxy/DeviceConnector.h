#ifndef _DEVICE_CONNECTOR_H_
#define _DEVICE_CONNECTOR_H_

#include "NatConnectProc.h"
#include "SWL_ConnectProcEx.h"
#include "SWL_MultiNet.h"

typedef struct _connect_unit_
{
	CSWL_MultiNet *pMultiNet;
	TVT_SOCKET sock;
}CONNECT_UNIT;

enum CONNECT_MODE
{
	CONNECT_MODE_NONE,
	CONNECT_MODE_TCP,
	CONNECT_MODE_UDT,
	CONNECT_MODE_MIX
};

class CDeviceConnector
{
public:
	CDeviceConnector(CSWL_MultiNet *pTcpMultiNet,CSWL_MultiNet *pNatMultiNet,std::string serialNo);
	~CDeviceConnector();

	bool ConnectDevice(CONNECT_UNIT *pUnit);
	void OnConnect(TVT_SOCKET sock,bool bTcp);

	CONNECT_MODE InitConnetMode();

private:
	bool ConnectTcpMode(CONNECT_UNIT *pUnit);
	bool ConnectUdtMode(CONNECT_UNIT *pUnit);
	bool ConnectMixMode(CONNECT_UNIT *pUnit);
	
	void WaitForConnect(int timeouts);

	void InitUdtConnectProc();
	void InitTcpConnectProc();

private:
	CSWL_MultiNet	*m_pTcpMultiNet;
	CSWL_ConnectProcEx *m_pTcpConnectProc;

	CSWL_MultiNet	*m_pNatMultiNet;
	CNatConnectProc	*m_pNatConnectProc;

	std::string		m_serialNo;
	std::string		m_addr;
	unsigned short	m_port;

	CPUB_Lock		m_lock;
	TVT_SOCKET		m_sock;
	bool			m_bConnected;
	bool			m_bTcp;
};

#endif