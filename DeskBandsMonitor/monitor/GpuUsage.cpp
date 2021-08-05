#include "GpuUsage.hpp"
#include <tchar.h>
#include <Psapi.h>
#include <tlhelp32.h>
#include <Wtsapi32.h>
#pragma comment(lib, "Wtsapi32.lib")
#pragma comment(lib, "Psapi.lib")

#pragma warning(disable:4996)

#define kIsDebuging FALSE
#if kIsDebuging
#define PRINTF_DEBUG_MSG(msg) \
	cout << msg << endl;
#else
#define PRINTF_DEBUG_MSG(msg)
#endif
// GPU使用率空闲以及忙碌阈值
const float kIdleThreshold = 0.5f;
const float kBusyThreshold = 0.7f;

//操作系统版本是否为XP或更旧
bool isXPorEarlier()
{
	OSVERSIONINFO os_version;
	bool is_XP_or_later = FALSE;
	memset(&os_version, 0, sizeof(OSVERSIONINFO));
	os_version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&os_version);
	return (os_version.dwMajorVersion < 6) ? true : false;
}

wstring& replaceall(wstring& str, const wchar_t* old_value, const wchar_t* new_value)
{
	int n1 = (int)wcslen(old_value);
	int n2 = (int)wcslen(new_value);
	for (string::size_type pos(0); pos != string::npos; pos += n2)
	{
		if ((pos = str.find(old_value, pos)) != string::npos)
			str.replace(pos, n1, new_value);

		else
			break;
	}
	return str;
}

CGpuUsage::CGpuUsage()
	:_is_initialized(false)
	, _usage_lock()
	,_query_statistics()
	, XD_D3DKMTQueryStatistics(NULL)
	, _gdi32_hmodule(NULL)
	, _current_usage(0.0)
	, _running_time(0)
{
	_performance_counter.QuadPart = 0;
}

CGpuUsage::~CGpuUsage()
{
	uninitalizeAdapter();
}

bool CGpuUsage::initializeAdapter()
{
	if (isXPorEarlier()) return false;
	if (_is_initialized) return true;

	LUID luid = { 20 };
	TOKEN_PRIVILEGES privs = { 1, { luid, SE_PRIVILEGE_ENABLED } };
	HANDLE hProcess = GetCurrentProcess();
	if (NULL == hProcess) {
		PRINTF_DEBUG_MSG("initializeAdapter, failed to get module.");
		return false;
	}

	HANDLE hToken;
	BOOL bRet = OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken);
	if (!bRet) {
		CloseHandle(hToken);
		CloseHandle(hProcess);
		PRINTF_DEBUG_MSG("initializeAdapter, failed to open process token.");
		return false;
	}
	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &privs.Privileges[0].Luid);
	bRet = AdjustTokenPrivileges(hToken, FALSE, &privs, sizeof(privs), NULL, NULL);
	if (!bRet) {
		CloseHandle(hToken);
		CloseHandle(hProcess);
		PRINTF_DEBUG_MSG("initializeAdapter, failed to adjust privileges.");
		return false;
	}

	getGpuIDpath(sDeviceName);
	sDeviceName = replaceall(sDeviceName, TEXT("\\"), TEXT("#"));
	sDeviceName.insert(0, TEXT("\\\\?\\"));
	sDeviceName.append(TEXT("#{1ca05180-a699-450a-9a0c-de4fbe3ddd89}"));
	device_name.pDeviceName = sDeviceName.c_str();

	_gdi32_hmodule = LoadLibraryA("gdi32.dll");
	if (!_gdi32_hmodule) {
		CloseHandle(hToken);
		CloseHandle(hProcess);
		PRINTF_DEBUG_MSG("initializeAdapter, failed to load gdi32.dll");
		return false;
	}
	PFND3DKMT_OPENADAPTERFROMDEVICENAME D3DKMTOpenAdapterFromDeviceName =
		(PFND3DKMT_OPENADAPTERFROMDEVICENAME)GetProcAddress(_gdi32_hmodule, "D3DKMTOpenAdapterFromDeviceName");
	if (!D3DKMTOpenAdapterFromDeviceName) {
		CloseHandle(hToken);
		CloseHandle(hProcess);
		FreeLibrary(_gdi32_hmodule);
		PRINTF_DEBUG_MSG("initializeAdapter, failed to get openAdapter address.");
		return false;
	}
	NTSTATUS nt_status = D3DKMTOpenAdapterFromDeviceName(&device_name);
	if (0 > nt_status || 63 < nt_status) {
		CloseHandle(hToken);
		CloseHandle(hProcess);
		FreeLibrary(_gdi32_hmodule);
		PRINTF_DEBUG_MSG("initializeAdapter, failed to open adapter.");
		return false;
	}

	XD_D3DKMTQueryStatistics = (PFND3DKMT_QUERYSTATISTICS)GetProcAddress(_gdi32_hmodule, "D3DKMTQueryStatistics");
	if (!XD_D3DKMTQueryStatistics) {
		CloseHandle(hToken);
		CloseHandle(hProcess);
		FreeLibrary(_gdi32_hmodule);
		PRINTF_DEBUG_MSG("initializeAdapter, failed to get queryStatistics address.");
		return false;
	}
	CloseHandle(hToken);
	CloseHandle(hProcess);
	_is_initialized = true;
	return true;
}

void CGpuUsage::uninitalizeAdapter()
{
	FreeLibrary(_gdi32_hmodule);
	_gdi32_hmodule = NULL;
	_is_initialized = false;
	
}

bool CGpuUsage::isGpuIdle()
{
	AutoLock auto_lock(&_usage_lock);
	return (_is_initialized && _current_usage < kIdleThreshold) ? true : false;
}

float CGpuUsage::getUsage(/*DWORD PID*/HANDLE hProcess)
{
	float fRet = 0.0;
	//HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, TRUE, PID);
	if (!hProcess) {
		PRINTF_DEBUG_MSG("initializeAdapter, failed to open process.");
		return fRet;
	}
	
	_query_statistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS_NODE;
	_query_statistics.AdapterLuid = device_name.AdapterLuid;
	_query_statistics.hProcess = hProcess;
	_query_statistics.QueryNode.NodeId = 0;
	
	if (!XD_D3DKMTQueryStatistics) {
		PRINTF_DEBUG_MSG("D3DKMTQueryStatistics function address is null.");
		return fRet;
	}

	LARGE_INTEGER clock_frequency = { 0 };
	QueryPerformanceFrequency(&clock_frequency);
	LARGE_INTEGER counter = { 0 };
	QueryPerformanceCounter(&counter);
	LONGLONG delta_counter = counter.QuadPart - _performance_counter.QuadPart;
	// 逝去的时间，单位为纳秒
	double elapsed_time = static_cast<double>(delta_counter * 10000000) / clock_frequency.QuadPart;
	NTSTATUS status = XD_D3DKMTQueryStatistics(&_query_statistics);
	if (0 > status || 63 < status)
	{
		return fRet;
	}
	LONGLONG running_time = _query_statistics.QueryResult.NodeInformation.GlobalInformation.RunningTime.QuadPart;
	printf("running_time=%llx\n", running_time);
	LONGLONG delta_time = running_time - _running_time;
	if (0 < _performance_counter.QuadPart && 0 < _running_time) {
		float usage = 0.0f;
		if (0 < elapsed_time) {
			usage = static_cast<float>(static_cast<double>(delta_time) / elapsed_time);
		}
		_usage_lock.Lock();
		_current_usage = usage;
		printf("gpu usage %.4f\n", usage);
		_usage_lock.Unlock();
		checkBusyState(usage);
		fRet = usage * 100;
	}
	_performance_counter = counter;
	_running_time = running_time;
	//CloseHandle(hProcess);
	return fRet;
}

void CGpuUsage::checkBusyState(const float current_gpu)
{
	if (current_gpu >= kBusyThreshold) {
		//TODO:做相关操作
	}
}

void CGpuUsage::ViewProcessList(std::map<DWORD, float> &mProGPU)
{
	HANDLE hProcessSnap = NULL;
	try
	{
		hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hProcessSnap == INVALID_HANDLE_VALUE)
			throw GetLastError();

		PROCESSENTRY32 pe32 = { 0 };
		pe32.dwSize = sizeof(PROCESSENTRY32);
		if (!Process32First(hProcessSnap, &pe32))
			throw GetLastError();

		do
		{
			float usage = GetGpuUsage(pe32.th32ProcessID);
			if (usage)
			{
				usage = (float)(((usage + 0.005) * 100) / 100);
				mProGPU.insert(std::make_pair(pe32.th32ProcessID, usage));
			}

		} while (Process32Next(hProcessSnap, &pe32));

	}
	catch (...) {};
	if (hProcessSnap != NULL)
		CloseHandle(hProcessSnap);
}

float CGpuUsage::GetGpuUsage(DWORD dID)
{
	float fRet = 0.0;
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, TRUE, dID);
	if (!hProcess) {
		PRINTF_DEBUG_MSG("initializeAdapter, failed to open process.");
		return fRet;
	}

/*
	if (!XD_D3DKMTQueryStatistics) {
		PRINTF_DEBUG_MSG("D3DKMTQueryStatistics function address is null.");
		return fRet;
	}*/
	_query_statistics.Type = D3DKMT_QUERYSTATISTICS_PROCESS_NODE;
	_query_statistics.AdapterLuid = device_name.AdapterLuid;
	_query_statistics.hProcess = hProcess;
	_query_statistics.QueryNode.NodeId = 0;

	LARGE_INTEGER clock_frequency = { 0 };
	QueryPerformanceFrequency(&clock_frequency);
	LARGE_INTEGER counter = { 0 };
	QueryPerformanceCounter(&counter);
	auto itsCounter = mCounter.find(dID);
	if (itsCounter == mCounter.end())
	{
		LARGE_INTEGER largeID;
		largeID.QuadPart = 0;
		mCounter.insert(std::make_pair(dID, largeID));
		itsCounter = mCounter.find(dID);
		if (itsCounter == mCounter.end())
		{
			PRINTF_DEBUG_MSG("mCounter not exsits.");
			CloseHandle(hProcess);
			_query_statistics.hProcess = NULL;
			return fRet;
		}
	}
	
	LONGLONG delta_counter = counter.QuadPart - itsCounter->second.QuadPart;
	// 逝去的时间，单位为纳秒
	double elapsed_time = static_cast<double>(delta_counter * 10000000) / clock_frequency.QuadPart;
	
	NTSTATUS status = XD_D3DKMTQueryStatistics(&_query_statistics);
	
	if (0 > status || 63 < status)
	{
		CloseHandle(hProcess);
		_query_statistics.hProcess = NULL;
		return fRet;
	}
	LONGLONG running_time = _query_statistics.QueryResult.NodeInformation.GlobalInformation.RunningTime.QuadPart;
	//printf("running_time=%llx\n", running_time);

	auto itsRuntime = mRunningtime.find(dID);
	if (itsRuntime == mRunningtime.end())
	{
		mRunningtime.insert(std::make_pair(dID, 0));
		itsRuntime = mRunningtime.find(dID);
		if (itsRuntime == mRunningtime.end())
		{
			CloseHandle(hProcess);
			_query_statistics.hProcess = NULL;
			PRINTF_DEBUG_MSG("mRunningtime not exsits.");
			return fRet;
		}
	}
	
	LONGLONG delta_time = running_time - itsRuntime->second;

	if (0 < itsCounter->second.QuadPart && 0 < itsRuntime->second) {
		float usage = 0.0f;
		if (0 < elapsed_time) {
			usage = static_cast<float>(static_cast<double>(delta_time) / elapsed_time);
		}
		_usage_lock.Lock();
		_current_usage = usage;
		//printf("gpu usage %.4f\n", usage);
		_usage_lock.Unlock();
		checkBusyState(usage);
		fRet = usage * 100;
	}
	itsCounter->second = counter;
	itsRuntime->second = running_time;

	CloseHandle(hProcess);
	_query_statistics.hProcess = NULL;

	return fRet;
}

void CGpuUsage::getGpuIDpath(std::wstring &pPathID)
{
	HRESULT hres;

	hres = CoInitializeEx(0, COINIT_MULTITHREADED);// COINIT_APARTMENTTHREADED
	if (FAILED(hres))
	{
		cout << "Failed to initialize COM library. Error code = 0x"
			<< hex << hres << endl;
		return;
	}

	hres = CoInitializeSecurity(
		NULL,
		-1,                          // COM authentication  
		NULL,                        // Authentication services  
		NULL,                        // Reserved  
		RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication   
		RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation    
		NULL,                        // Authentication info  
		EOAC_NONE,                   // Additional capabilities   
		NULL                         // Reserved  
		);


	if (!(SUCCEEDED(hres) || RPC_E_TOO_LATE == hres))
	{
		cout << "Failed to initialize security. Error code = 0x"
			<< hex << hres << endl;
		CoUninitialize();
		return;
	}

	IWbemLocator *pLoc = NULL;
	hres = CoCreateInstance(
		CLSID_WbemLocator,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator, (LPVOID *)&pLoc);

	if (FAILED(hres))
	{
		cout << "Failed to create IWbemLocator object."
			<< " Err code = 0x"
			<< hex << hres << endl;
		CoUninitialize();
		return;
	}

	IWbemServices *pSvc = NULL;
	hres = pLoc->ConnectServer(
		_bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace  
		NULL,                    // User name. NULL = current user  
		NULL,                    // User password. NULL = current  
		0,                       // Locale. NULL indicates current  
		NULL,                    // Security flags.  
		0,                       // Authority (for example, Kerberos)  
		0,                       // Context object   
		&pSvc                    // pointer to IWbemServices proxy  
		);

	if (FAILED(hres))
	{
		cout << "Could not connect. Error code = 0x"
			<< hex << hres << endl;
		pLoc->Release();
		CoUninitialize();
		return;                // Program has failed.  
	}

	//cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;

	hres = CoSetProxyBlanket(
		pSvc,                        // Indicates the proxy to set  
		RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx  
		RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx  
		NULL,                        // Server principal name   
		RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx   
		RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx  
		NULL,                        // client identity  
		EOAC_NONE                    // proxy capabilities   
		);

	if (FAILED(hres))
	{
		cout << "Could not set proxy blanket. Error code = 0x"
			<< hex << hres << endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return;
	}

	IEnumWbemClassObject* pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT * FROM Win32_VideoController"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);

	if (FAILED(hres))
	{
		cout << "Query for operating system name failed."
			<< " Error code = 0x"
			<< hex << hres << endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return;
	}

	IWbemClassObject *pclsObj = NULL;
	ULONG uReturn = 0;

	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
			&pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}

		VARIANT vtProp;
		hr = pclsObj->Get(L"PNPDeviceID", 0, &vtProp, 0, 0);
		pPathID = (wchar_t *)vtProp.bstrVal;

		VariantClear(&vtProp);
		VariantClear(&vtProp);

		pclsObj->Release();
	}

	pSvc->Release();
	pLoc->Release();
	pEnumerator->Release();
	CoUninitialize();

	return;   // Program successfully completed.  
}

