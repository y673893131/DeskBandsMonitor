#include "NetUsage.h"
#include<TlHelp32.h>
#include<sstream>
#include<iomanip>
#include "../Log/Log.h"

#pragma comment(lib,"Iphlpapi.lib")
CNetUsage::CNetUsage()
	: m_up(0)
	, m_down(0)
	, m_hThread(NULL)
{
	start();
}

CNetUsage::~CNetUsage()
{
	if (m_hThread)
	{
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}

	Log(Log_Info, "%s", __FUNCTION__);
}

CNetUsage::set_string CNetUsage::getAdapterNames() {
	auto pIpAdapterInfo = (PIP_ADAPTER_INFO)malloc(sizeof(IP_ADAPTER_INFO));
	if (!pIpAdapterInfo)
	{
		return set_string();
	}

	ULONG dwSize = sizeof(IP_ADAPTER_INFO);
	DWORD dwResultCode = GetAdaptersInfo(pIpAdapterInfo, &dwSize);
	if (ERROR_BUFFER_OVERFLOW == dwResultCode) 
	{
		free(pIpAdapterInfo);
		pIpAdapterInfo = (PIP_ADAPTER_INFO)malloc(dwSize);
		if (!pIpAdapterInfo)
		{
			return set_string();
		}

		dwResultCode = GetAdaptersInfo(pIpAdapterInfo, &dwSize);
	}

	if (ERROR_SUCCESS != dwResultCode) 
	{
		free(pIpAdapterInfo);
		return{};
	}

	set_string res;
	PIP_ADAPTER_INFO cursor = pIpAdapterInfo;
	while (cursor) 
	{
		if (cursor->Type == MIB_IF_TYPE_ETHERNET || cursor->Type == IF_TYPE_IEEE80211) 
		{
			res.insert(cursor->Description);
		}

		cursor = cursor->Next;
	}

	free(pIpAdapterInfo);
	return res;
}

bool CNetUsage::exist(const CNetUsage::set_string& sets, const std::string& target) 
{
	return sets.find(target) != sets.end();
}

CNetUsage::v_net_info_t CNetUsage::getNetinfos(const CNetUsage::set_string& names) 
{
	MIB_IFTABLE* pMibIfTable = (MIB_IFTABLE*)malloc(sizeof(MIB_IFTABLE));
	if (!pMibIfTable)
	{
		return v_net_info_t();
	}

	ULONG dwSize = sizeof(MIB_IFTABLE);
	DWORD dwResultCode = GetIfTable(pMibIfTable, &dwSize, TRUE);

	if (ERROR_INSUFFICIENT_BUFFER == dwResultCode) 
	{
		free(pMibIfTable);
		pMibIfTable = (MIB_IFTABLE*)malloc(dwSize);
		if (!pMibIfTable)
		{
			return v_net_info_t();
		}
	}

	dwResultCode = GetIfTable(pMibIfTable, &dwSize, TRUE);
	if (NO_ERROR != dwResultCode) 
	{
		free(pMibIfTable);
		return v_net_info_t();
	}

	v_net_info_t res;
	for (DWORD i = 0; i < pMibIfTable->dwNumEntries; i++) 
	{
		MIB_IFROW* pRow = &pMibIfTable->table[i];
		if ((pRow->dwType == IF_TYPE_IEEE80211 || pRow->dwType == IF_TYPE_ETHERNET_CSMACD) && pRow->dwOperStatus == IF_OPER_STATUS_OPERATIONAL && exist(names, (char*)pRow->bDescr)) 
		{
			net_info_t t;
			t.strDescr = (char*)pRow->bDescr;
			t.dwIndex = pRow->dwIndex;
			t.dwType = pRow->dwType;
			t.dwInOctets = pRow->dwInOctets;
			t.dwOutOctets = pRow->dwOutOctets;
			t.dwAdminStatus = pRow->dwAdminStatus;
			t.dwOperStatus = pRow->dwOperStatus;
			t.dwSpeed = pRow->dwSpeed;
			res.push_back(t);
		}
	}

	free(pMibIfTable);

	return res;
}

DWORD WINAPI netCallback(LPVOID lpThreadParameter)
{
	auto pThis = reinterpret_cast<CNetUsage*>(lpThreadParameter);
	pThis->cb();
#ifdef _DEBUG
	return 0;
#endif
}

void CNetUsage::start()
{
	DWORD threadId = 0;
	m_hThread = CreateThread(nullptr, 0, netCallback, this, 0, &threadId);
	Log(Log_Info, "net_usage thead_id:%d", threadId);
}

void CNetUsage::pause()
{
	if (m_hThread)
	{
		SuspendThread(m_hThread);
	}
}

void CNetUsage::continue_()
{
	if (m_hThread)
	{
		ResumeThread(m_hThread);
	}
}

void CNetUsage::stop()
{
	if (m_hThread)
	{
		TerminateThread(m_hThread, 2);
		WaitForSingleObject(m_hThread, INFINITE);
	}
}

static ULONG delta(DWORD a, DWORD b) {
	if (a >= b) return a - b;
	else return (0xFFFFFFFF - b) + a;
}

void CNetUsage::loadUpDown(const CNetUsage::set_string& names, DWORD& up, DWORD&down)
{
	auto adapterTable = getNetinfos(names);

	up = 0;
	down = 0;
	for (auto it : adapterTable)
	{
		up += it.dwOutOctets;
		down += it.dwInOctets;
	}
}

void CNetUsage::cb()
{
	auto adapters = getAdapterNames();
	auto adapterTable = getNetinfos(adapters);

	DWORD dwUp = 0, dwDown = 0;
	loadUpDown(adapters, dwUp, dwDown);

	for (;;)
	{
		DWORD dwUpTmp = 0, dwDownTmp = 0;
		loadUpDown(adapters, dwUpTmp, dwDownTmp);

		m_up = delta(dwUpTmp, dwUp);
		m_down = delta(dwDownTmp, dwDown);
		dwUp = dwUpTmp;
		dwDown = dwDownTmp;

		Sleep(1000);
	}
}


DWORD CNetUsage::getUp()
{
	return m_up;
}

DWORD CNetUsage::getDown()
{
	return m_down;
}