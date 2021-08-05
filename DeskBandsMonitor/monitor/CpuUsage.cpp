#include "CpuUsage.h"
#include<TlHelp32.h>
#include<sstream>
#include<iomanip>

CCpuUsage::CCpuUsage()
	: dwNUMBER_OF_PROCESSORS(__GetCpuConut())
	, m_ullLastTime(0)
	, m_ullLastIdleTime(0)
{
	Update();
}

CCpuUsage::~CCpuUsage()
{
}

float CCpuUsage::getUsage(int pid)
{
	auto it = m_mapProcessMap.find(pid);
	if (it != m_mapProcessMap.end())
		return it->second.cpu_usage;
	return 0.0f;
}

void CCpuUsage::Update()
{
	__LoopForProcesses();
}

unsigned long long CCpuUsage::__FileTime2Utc(const FILETIME &ft)
{
	LARGE_INTEGER li;
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;
	return li.QuadPart;
}

double CCpuUsage::__GetCpuUsage(HANDLE hProcess, unsigned long long&last_time, unsigned long long&last_system_time)
{
	FILETIME now, creation_time, exit_time, kernel_time, user_time;
	unsigned long long system_time, time, system_time_delta, time_delta;

	GetSystemTimeAsFileTime(&now);

	if (!GetProcessTimes(hProcess, &creation_time, &exit_time, &kernel_time, &user_time))
	{
		return -1;
	}
	system_time = __FileTime2Utc(kernel_time) + __FileTime2Utc(user_time);
	time = __FileTime2Utc(now);
	if (!last_system_time || !last_time)
	{
		last_system_time = system_time;
		last_time = time;
		return -1;
	}
	system_time_delta = system_time - last_system_time;
	time_delta = time - last_time;
	if (!time_delta)
		return -1;
	last_system_time = system_time;
	last_time = time;
	return system_time_delta*100.0 / time_delta / dwNUMBER_OF_PROCESSORS;
}

void CCpuUsage::__LoopForProcesses()
{
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hProcessSnap)
	{
		return;
	}
	BOOL b;
	PROCESSENTRY32 pe32 = { sizeof(PROCESSENTRY32) };
	std::map<DWORD, PROCESS_INFO> oldPi;
	for (auto &x : m_pairMaxProcesses)
		x.second.cpu_usage = 0.0;
	m_mapProcessMap.swap(oldPi);
	for (b = Process32First(hProcessSnap, &pe32); b; b = Process32Next(hProcessSnap, &pe32))
	{
		if (!pe32.th32ProcessID)continue;
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe32.th32ProcessID);
		if (!hProcess)
		{
			if (ERROR_ACCESS_DENIED != GetLastError())
			{
				return;
			}
		}
		else
		{
			PROCESS_INFO tp = { 0 };
			auto it = oldPi.find(pe32.th32ProcessID);
			if (it != oldPi.end())
				tp = it->second;
			else
				_tcscpy_s(tp.name, pe32.szExeFile);
			double usage = __GetCpuUsage(hProcess, tp.last_time, tp.last_system_time);
			CloseHandle(hProcess);
			if (usage >= 0)
				tp.cpu_usage = usage;
			m_mapProcessMap.insert(std::make_pair(pe32.th32ProcessID, tp));
			if (usage >= 0)
			{
				for (size_t i = 0; i < _countof(m_pairMaxProcesses); ++i)
					if (tp.cpu_usage>m_pairMaxProcesses[i].second.cpu_usage)
					{
						for (size_t j = _countof(m_pairMaxProcesses) - 1; j > i; --j)
							m_pairMaxProcesses[j] = m_pairMaxProcesses[j - 1];
						m_pairMaxProcesses[i] = std::make_pair(pe32.th32ProcessID, tp);
						break;
					}
			}
		}
	}
	CloseHandle(hProcessSnap);
}

const DWORD CCpuUsage::__GetCpuConut()const
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwNumberOfProcessors;
}

void CCpuUsage::print()
{
	for (auto it : m_mapProcessMap)
	{
		if (it.second.cpu_usage <= 0)
			continue;
		OutputDebugStringA("[");
		OutputDebugString(std::to_wstring(it.first).c_str());
		OutputDebugStringA("[");

		OutputDebugStringA("[");
		OutputDebugString(it.second.name);
		OutputDebugStringA("]");
		
		OutputDebugStringA("[");
		OutputDebugString(std::to_wstring(it.second.cpu_usage).c_str());
		OutputDebugStringA("]\n");
	}

	OutputDebugStringA("===============================CPU\n");
}