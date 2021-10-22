#pragma once

#include <Windows.h>
#include <string>

#define MB_NUM(x) MessageBoxA(NULL, std::to_string(x).c_str(), "ÌבÊ¾(Tips)", MB_OK | MB_ICONINFORMATION);

class CBaseWindow
{
public:
	CBaseWindow(HINSTANCE ins, HWND parent);
	virtual ~CBaseWindow();

	void setMouseTracking(BOOL);
	BOOL isHover();


	void show(bool);

	virtual LRESULT wndCall(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected://event
	virtual void onCreate();
	virtual void onHover();
	virtual void onLeave();
	virtual void onPaint();
	virtual void onTimer(WPARAM wParam, LPARAM lParam);
	virtual void onShow(WPARAM wParam, LPARAM lParam);
private:
	static LRESULT CALLBACK detailWindowCallBack(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	HINSTANCE m_ins;
	HWND m_parentHwnd;
	HWND m_hwnd;
	std::wstring m_className;

	int m_width, m_height;


	BOOL m_bMouseTracking, m_bHover;
};

