#include "Util.h"
#include <TlHelp32.h>

void util::setTransparent(HWND hwnd, int number)
{
	SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	SetLayeredWindowAttributes(hwnd, 0, (BYTE)number, LWA_ALPHA);
}

void util::setStyle(HWND hwnd, int flag)
{
	SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) | flag);
}

void util::setMouseTracking(HWND hwnd)
{
	TRACKMOUSEEVENT tme;
	tme.cbSize = sizeof(TRACKMOUSEEVENT);
	tme.dwFlags = TME_HOVER | TME_LEAVE;
	tme.dwHoverTime = 1; //1ms 立即显示
	tme.hwndTrack = hwnd;
	//激活WM_MOUSEHOVER消息
	TrackMouseEvent(&tme);
}

bool util::setPrivilege(LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)
{
	HANDLE hToken = nullptr;
	TOKEN_PRIVILEGES tp;
	auto pid = GetCurrentProcessId();
	auto handle = GetCurrentProcess();
	bool bWhile = false;
	bool bRet = false;
	do
	{
		if (handle == INVALID_HANDLE_VALUE)
		{
			handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		}

		if (!OpenProcessToken(handle, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
			break;

		if (!LookupPrivilegeValue(NULL, lpszPrivilege, &tp.Privileges[0].Luid))
			break;

		tp.PrivilegeCount = 1;
		if (bEnablePrivilege)
		{
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		}
		else
		{
			tp.Privileges[0].Attributes = 0;
		}

		if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES),
			(PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL))
		{
			break;
		}

		if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
		{
			break;
		}

		bRet = true;
	} while (bWhile);
	
	if (hToken && hToken != INVALID_HANDLE_VALUE) CloseHandle(hToken);
	if (handle && handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
	return bRet;
}

void util::reportErr(const wchar_t* title)
{
	LPVOID lpMsgBuf;
	FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&lpMsgBuf,
		0,
		NULL
		);
	wchar_t err[MAX_PATH] = {};
	wsprintf(err, L"0x%08x\n%s", GetLastError(), (char*)lpMsgBuf);
	MessageBox(NULL, err, title, MB_ICONERROR | MB_OK);
	LocalFree(lpMsgBuf);
}

std::wstring util::getPidPath(DWORD pid)
{
	auto hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		return L"";
	}

	MODULEENTRY32 me32;
	me32.dwSize = sizeof(MODULEENTRY32);
	if (!Module32First(hSnapshot, &me32))
	{
		return L"";
	}

	CloseHandle(hSnapshot);
	return me32.szExePath;
}

void util::executeCommand(const std::wstring& cmd, const std::wstring& par, int nShow, bool bAdmin)
{
	SHELLEXECUTEINFO ShExecInfo = { 0 };
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	if (bAdmin)
	{
		ShExecInfo.lpVerb = L"runas";                                 //     管理员方式启动
	}
	ShExecInfo.lpFile = cmd.c_str();                              //     调用的程序名
	ShExecInfo.lpParameters = par.c_str();                        //     调用程序的命令行参数
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = nShow;                                     //     窗口状态为隐藏
	ShExecInfo.hInstApp = NULL;
	ShellExecuteEx(&ShExecInfo);                                  //     启动新的程序
	WaitForSingleObject(ShExecInfo.hProcess, INFINITE);    //     等到该进程结束
}