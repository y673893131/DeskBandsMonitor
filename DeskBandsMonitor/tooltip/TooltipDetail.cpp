#include "TooltipDetail.h"
#include <CommCtrl.h>

CTooltipDetail::CTooltipDetail(HINSTANCE ins, HWND parent)
{
	m_ins = ins;
	m_parentHwnd = parent;
	m_hwnd = CreateWindowEx(WS_EX_TOPMOST,
		TOOLTIPS_CLASS,
		NULL,
		WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		parent,
		NULL,
		ins,
		NULL);
}

CTooltipDetail::~CTooltipDetail()
{
}

void CTooltipDetail::tooltip(LPWSTR info)
{
	TOOLINFO ti = { sizeof(TOOLINFO) };
	ti.uFlags = TTF_SUBCLASS;
	ti.hinst = m_ins;
	ti.hwnd = m_parentHwnd;
	ti.lpszText = info;

	GetClientRect(m_parentHwnd, &ti.rect);
	SendMessage(m_hwnd, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
}