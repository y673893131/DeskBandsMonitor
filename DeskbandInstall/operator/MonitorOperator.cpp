#include "MonitorOperator.h"
#include "..\resource.h"
#include <stdlib.h>

#ifdef _M_X64
#define MONITOR_DLL L"DeskbandMonitor_x64.dll"
#else
#define MONITOR_DLL L"DeskbandMonitor_x86.dll"
#endif

CMonitorOperator::CMonitorOperator()
	: m_hDll(NULL)
	, m_regsterFunc(NULL)
	, m_unregsterFunc(NULL)
	, m_isregsterFunc(NULL)
	, m_showFunc(NULL)
	, m_isShowFunc(NULL)
	, m_enableLogFunc(NULL)
{
}

CMonitorOperator::~CMonitorOperator()
{
}

void CMonitorOperator::analyzeCommandLine(LPTSTR lpCmdLine)
{
	std::wstring sCmdLine(lpCmdLine);
	int pos = 0;
	do
	{
		pos = (int)sCmdLine.find(L" ");
		if (pos < 0)
			break;
		m_commandLine.insert(sCmdLine.substr(0, pos));
		sCmdLine = sCmdLine.substr(pos + 1, sCmdLine.length() - pos);
	} while (true);

	if (!sCmdLine.empty())
	{
		m_commandLine.insert(sCmdLine);
	}
}

void CMonitorOperator::analyze()
{
	if (!loadDLL())
		return;

	if (m_commandLine.empty())
	{
		autoOpeartor();
		return;
	}

	if (m_commandLine.find(L"-log") != m_commandLine.end())
	{
		log();
	}

	if (m_commandLine.find(L"-install") != m_commandLine.end())
	{
		install();
	}
	else if (m_commandLine.find(L"-uninstall") != m_commandLine.end())
	{
		uninstall();
	}
	else
	{
		help();
	}

	freeDll();
}

bool CMonitorOperator::getDllFromResource(UINT pResourceID, HMODULE hInstance, LPCTSTR pResourceType, LPCTSTR pTargetDll)
{
	LPCTSTR pResourceName = MAKEINTRESOURCE(pResourceID);
	HRSRC hResource = FindResource(hInstance, pResourceName, pResourceType);
	if (!hResource)
	{
		return false;
	}

	DWORD dwResourceSize = SizeofResource(hInstance, hResource);
	if (!dwResourceSize)
	{
		return false;
	}

	const void* pResourceData = LockResource(LoadResource(hInstance, hResource));
	if (!pResourceData)
	{
		return false;
	}

	HGLOBAL	hResourceBuffer = GlobalAlloc(GMEM_MOVEABLE, dwResourceSize);
	if (!hResourceBuffer)
	{
		return false;
	}

	void* pResourceBuffer = GlobalLock(hResourceBuffer);
	if (!pResourceBuffer)
	{
		GlobalFree(hResourceBuffer);
		return false;
	}

	CopyMemory(pResourceBuffer, pResourceData, dwResourceSize);

	auto fHandle = CreateFile(pTargetDll
		, GENERIC_READ | GENERIC_WRITE
		, FILE_SHARE_READ
		, NULL
		, CREATE_NEW
		, FILE_FLAG_WRITE_THROUGH
		, NULL);
	if (!fHandle)
	{
		GlobalUnlock(hResourceBuffer);
		GlobalFree(hResourceBuffer);
		return false;
	}

	DWORD dwWrite = 0;
	WriteFile(fHandle, pResourceBuffer, dwResourceSize, &dwWrite, NULL);
	if (dwWrite != dwResourceSize)
	{
		CloseHandle(fHandle);
		GlobalUnlock(hResourceBuffer);
		GlobalFree(hResourceBuffer);
		return false;
	}

	CloseHandle(fHandle);
	GlobalUnlock(hResourceBuffer);
	GlobalFree(hResourceBuffer);

	return true;
}

std::wstring CMonitorOperator::currentDir()
{
	wchar_t buff[1024] = {};
	GetModuleFileName(NULL, buff, 1024);
	auto p = _tcsrchr(buff, '\\');
	*p = 0;

	return buff;
}

void CMonitorOperator::mkdll()
{
	auto path = currentDir() + L"\\" + MONITOR_DLL;
	if (_waccess(path.c_str(), 0) == 0)
		return;

	auto hInstance = GetModuleHandle(NULL);
	getDllFromResource(IDR_DLL1, hInstance, L"DLL", path.c_str());
}

bool CMonitorOperator::loadDLL()
{
	if (m_hDll)
		return true;

	mkdll();
	m_hDll = LoadLibrary(MONITOR_DLL);
	if (!m_hDll)
	{
		::MessageBox(NULL, L"¼ÓÔØDLLÊ§°Ü£¡", L"´íÎó", MB_OK);
		return false;
	}

	m_regsterFunc = (RegsterFunc)GetProcAddress(m_hDll, "DllRegisterServer");
	m_unregsterFunc = (UnRegsterFunc)GetProcAddress(m_hDll, "DllUnregisterServer");
	m_isregsterFunc = (IsRegsterFunc)GetProcAddress(m_hDll, "DLLIsRegster");
	m_showFunc = (ShowFunc)GetProcAddress(m_hDll, "DllShowMonitor");
	m_isShowFunc = (IsShowFunc)GetProcAddress(m_hDll, "DllIsShow");
	m_enableLogFunc = (EnableLogFunc)GetProcAddress(m_hDll, "DllEnableLog");

	bool bRet = false;
	if (!m_regsterFunc)
	{
		::MessageBox(NULL, L"DllRegisterServer ¼ÓÔØÊ§°Ü£¡", L"´íÎó", MB_OK);
		goto load_ret;
	}

	if (!m_unregsterFunc)
	{
		::MessageBox(NULL, L"DllUnregisterServer ¼ÓÔØÊ§°Ü£¡", L"´íÎó", MB_OK);
		goto load_ret;
	}

	if (!m_isregsterFunc)
	{
		::MessageBox(NULL, L"DLLIsRegster ¼ÓÔØÊ§°Ü£¡", L"´íÎó", MB_OK);
		goto load_ret;
	}

	if (!m_showFunc)
	{
		::MessageBox(NULL, L"DllShowMonitor ¼ÓÔØÊ§°Ü£¡", L"´íÎó", MB_OK);
		goto load_ret;
	}

	if (!m_isShowFunc)
	{
		::MessageBox(NULL, L"DllIsShow ¼ÓÔØÊ§°Ü£¡", L"´íÎó", MB_OK);
		goto load_ret;
	}

	if (!m_enableLogFunc)
	{
		::MessageBox(NULL, L"DllEnableLog ¼ÓÔØÊ§°Ü£¡", L"´íÎó", MB_OK);
		goto load_ret;
	}

	bRet = true;
load_ret:
	if (!bRet) FreeLibrary(m_hDll);
	return bRet;
}

void CMonitorOperator::freeDll()
{
	if (m_hDll)
	{
		FreeLibrary(m_hDll);
		m_hDll = NULL;
	}
}

void CMonitorOperator::autoOpeartor()
{
	HRESULT hr = m_isregsterFunc();
	if (hr != S_OK)
	{
		install();
	}
	else
	{
		uninstall();
	}
}

void CMonitorOperator::log()
{
	m_enableLogFunc(TRUE);
}

void CMonitorOperator::install()
{
	HRESULT hr = m_isregsterFunc();
	if (hr != S_OK)
	{
		hr = m_regsterFunc();
		if (hr == S_OK)
		{
			hr = m_showFunc(TRUE);
		}
	}
	else
	{
		hr = m_showFunc(TRUE);
	}

	if (hr == S_OK)
	{
		HWND hTaskbarWnd = FindWindow(TEXT("Shell_TrayWnd"), NULL);
		if (hTaskbarWnd)
			PostMessage(hTaskbarWnd, WM_TIMER, 24, 0);
	}
	else
	{
		::MessageBox(NULL, L"install failed.", L"error", MB_OK);
	}
}

void CMonitorOperator::uninstall()
{
	HRESULT hr = m_isregsterFunc();
	if (hr != S_OK)
	{
	}
	else
	{
		m_showFunc(FALSE);
		hr = m_unregsterFunc();
		if (hr != S_OK)
		{
			::MessageBox(NULL, L"uninstall failed.", L"error", MB_OK);
		}
		else
		{
			HWND hTaskbarWnd = FindWindow(TEXT("Shell_TrayWnd"), NULL);
			if (hTaskbarWnd)
				PostMessage(hTaskbarWnd, WM_TIMER, 24, 0);
		}
	}

	freeDll();
	auto path = currentDir() + L"\\" + MONITOR_DLL;
	DeleteFile(path.c_str());
}

void CMonitorOperator::help()
{
	std::string sHelp;
	sHelp += "´ËÃüÁîÓï·¨£¨Command):\n";
	sHelp += "	[-install | -uninstall]:\n";
	printf(sHelp.c_str());
	MessageBoxA(NULL, sHelp.c_str(), "ÌáÊ¾(Tips)", MB_OK | MB_ICONINFORMATION);
}