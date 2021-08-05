#pragma once
#include<Windows.h>
#include<winternl.h>
#include<map>

#include<string>
#include<tchar.h>

class CCpuUsage
{
	typedef struct tagPROCESS_INFO
	{
		TCHAR name[MAX_PATH];
		unsigned long long last_time, last_system_time;
		double cpu_usage;
	}PROCESS_INFO;

public:
	CCpuUsage();
	~CCpuUsage();

	void Update();
	void print();

	float getUsage(int pid);
private:
	unsigned long long __FileTime2Utc(const FILETIME &);
	double __GetCpuUsage(HANDLE, unsigned long long &, unsigned long long &);
	void __LoopForProcesses();
	const DWORD __GetCpuConut()const;
private:
	const DWORD dwNUMBER_OF_PROCESSORS;
	unsigned long long m_ullLastTime, m_ullLastIdleTime;
	std::map<DWORD, PROCESS_INFO> m_mapProcessMap;
	std::pair<DWORD, PROCESS_INFO> m_pairMaxProcesses[3];
};
