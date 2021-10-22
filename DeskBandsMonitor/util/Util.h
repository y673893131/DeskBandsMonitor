#pragma once

#include <Windows.h>
#include <string>
namespace util
{
	//mumber: alpha (0-255)
	void setTransparent(HWND hwnd, int number);

	void setStyle(HWND hwnd, int flag);

	void setMouseTracking(HWND hwnd);

	bool setPrivilege(LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);

	void reportErr(const wchar_t* title);

	std::wstring getPidPath(DWORD pid);

	void executeCommand(const std::wstring& cmd, const std::wstring& par, int nShow = SW_HIDE, bool bAdmin = false);
};

