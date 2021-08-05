#pragma once

#include <iostream>  
#include <map>
using namespace std;
#include <comdef.h>  
#include <Wbemidl.h> 
#pragma comment(lib, "wbemuuid.lib")  

//#include "D3dkmthk.h"
#include "d3dkmt.h"

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&);               \


class SectionLock {
private:
	// make copy constructor and assignment operator inaccessible
	DISALLOW_COPY_AND_ASSIGN(SectionLock);
	CRITICAL_SECTION _critical_section;

public:
	SectionLock() {
		InitializeCriticalSection(&_critical_section);
	};

	~SectionLock() {
		DeleteCriticalSection(&_critical_section);
	};

	void Lock() {
		EnterCriticalSection(&_critical_section);
	};

	void Unlock() {
		LeaveCriticalSection(&_critical_section);
	};
};

class AutoLock {
private:
	// make copy constructor and assignment operator inaccessible
	DISALLOW_COPY_AND_ASSIGN(AutoLock);

protected:
	SectionLock * _section_lock;

public:
	AutoLock(SectionLock * plock) {
		_section_lock = plock;
		_section_lock->Lock();
	};

	~AutoLock() {
		_section_lock->Unlock();
	};
};

class CGpuUsage
{
private:
	// make copy constructor and assignment operator inaccessible
	CGpuUsage(const CGpuUsage &refGpuUsage);
	CGpuUsage &operator=(const CGpuUsage &refGpuUsage);
public:
	CGpuUsage();
	~CGpuUsage();

public:
	bool initializeAdapter();
	void uninitalizeAdapter();
	float getUsage(/*DWORD PID*/HANDLE hProcess);
	bool isGpuIdle();
	bool isInit(){ return _is_initialized; }

	//private:
	void checkBusyState(const float current_gpu);
	void getGpuIDpath(std::wstring &pPathID);
	float GetGpuUsage(DWORD dID);
	void ViewProcessList(std::map<DWORD, float> &mProGPU);

private:
	bool _is_initialized;
	SectionLock _usage_lock;
	HMODULE _gdi32_hmodule;
	PFND3DKMT_QUERYSTATISTICS XD_D3DKMTQueryStatistics;
	D3DKMT_QUERYSTATISTICS _query_statistics;
	D3DKMT_OPENADAPTERFROMDEVICENAME device_name;
	std::wstring sDeviceName;
	float _current_usage;
	LARGE_INTEGER _performance_counter;
	LONGLONG _running_time;

	std::map<DWORD, LARGE_INTEGER> mCounter;
	std::map<DWORD, LONGLONG> mRunningtime;
};

