#pragma once

//#include <TlHelp32.h>
#include <string>

//net speed(bit/s)
struct _net_info_t
{
	unsigned long up;
	unsigned long down;
};

struct _monotor_info_t
{
	_net_info_t net;
};

class CNetUsage;
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
	CNetUsage* m_pNetTask;
};

