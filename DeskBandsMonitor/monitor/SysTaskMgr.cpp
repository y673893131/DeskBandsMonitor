#include "SysTaskMgr.h"
#include "NetUsage.h"
#include <Psapi.h>
#include <sstream>
#include "../Log/Log.h"
#include "../../NetMonitor/NetManager.h"

CSysTaskMgr::CSysTaskMgr()
{
	//m_pNetTask = new CNetUsage();
	m_pNetManager = new CNetManager();
}

CSysTaskMgr::~CSysTaskMgr()
{
	Log(Log_Info, "%s", __FUNCTION__);
	//m_pNetTask->stop();
	//delete m_pNetTask;
	m_pNetManager->stop();
	delete m_pNetManager;
}

std::string CSysTaskMgr::toSpeedString(unsigned long dwBytes) {
	std::ostringstream os;
	if (dwBytes < 1024) {
		os << (double)dwBytes << " B";
	}
	else if (dwBytes < 1024 * 1024) {
		os << floor((double)dwBytes / 1024 * 100) / 100 << " KB";
	}
	else {
		os << floor((double)dwBytes / 1024 / 1024 * 100) / 100 << " MB";
	}
	return os.str();
}

std::wstring CSysTaskMgr::toSpeedStringW(unsigned long dwBytes)
{
	std::wostringstream os;
	if (dwBytes < 1024) {
		os << (double)dwBytes << L" B";
	}
	else if (dwBytes < 1024 * 1024) {
		os << floor((double)dwBytes / 1024 * 100) / 100 << L" KB";
	}
	else {
		os << floor((double)dwBytes / 1024 / 1024 * 100) / 100 << L" MB";
	}
	return os.str();
}

_monotor_info_t CSysTaskMgr::getTasks()
{
	return m_pNetManager->getStatInfo();
}