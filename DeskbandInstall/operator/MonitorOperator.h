#pragma once

#include <iostream>
#include <set>
#include <string>
#include <Windows.h>
#include <tchar.h>

typedef HRESULT(__stdcall *RegsterFunc)();
typedef HRESULT(__stdcall *UnRegsterFunc)();
typedef HRESULT(__stdcall *IsRegsterFunc)();
typedef HRESULT(__stdcall *ShowFunc)(BOOL);
typedef HRESULT(__stdcall *IsShowFunc)();
typedef HRESULT(__stdcall *EnableLogFunc)(BOOL);

class CMonitorOperator
{
public:
	CMonitorOperator();
	~CMonitorOperator();

	void analyzeCommandLine(LPTSTR lpCmdLine);

	void analyze();

	void mkdll();
	void autoOpeartor();
	void log();
	void install();
	void uninstall();
	void help();

private:
	std::wstring currentDir();
	bool loadDLL();
	void freeDll();
	bool getDllFromResource(UINT pResourceID, HMODULE hInstance, LPCTSTR pResourceType, LPCTSTR pTargetDll);

private:
	std::set<std::wstring> m_commandLine;

	HMODULE m_hDll;
	RegsterFunc m_regsterFunc;
	UnRegsterFunc m_unregsterFunc;
	IsRegsterFunc m_isregsterFunc;
	ShowFunc m_showFunc;
	IsShowFunc m_isShowFunc;
	EnableLogFunc m_enableLogFunc;
};