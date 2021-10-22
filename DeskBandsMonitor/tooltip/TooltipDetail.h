#pragma once
#pragma once

#include <Windows.h>

class CTooltipDetail
{
public:
	CTooltipDetail(HINSTANCE ins, HWND parent);
	virtual ~CTooltipDetail();

	void tooltip(LPWSTR);
private:
	HINSTANCE m_ins;
	HWND m_parentHwnd;
	HWND m_hwnd;
};

