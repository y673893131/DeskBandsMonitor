#pragma once

//#include <TlHelp32.h>
#include <string>
#include "../../NetMonitor/NetManager.h"

//class CNetUsage;
class CNetManager;
class CSysTaskMgr
{
public:
	CSysTaskMgr();
	~CSysTaskMgr();

	_monotor_info_t getTasks();

	static std::string CSysTaskMgr::toSpeedString(unsigned long dwBytes);
	static std::wstring CSysTaskMgr::toSpeedStringW(unsigned long dwBytes);

private:
	CSysTaskMgr(const CSysTaskMgr&) = delete;

private:
	//CNetUsage* m_pNetTask;
	CNetManager* m_pNetManager;
};

