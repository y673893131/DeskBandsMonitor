#include "BaseWindow.h"
#include "../../util/Util.h"

extern HINSTANCE g_hInst;
CBaseWindow::CBaseWindow(HWND parent)
	: m_className(L"window\\baseWindow")
	, m_width(800)
	, m_height(600)
	, m_bMouseTracking(FALSE)
	, m_bHover(FALSE)
	, m_bEraseBack(TRUE)
{
	m_ins = g_hInst;
	m_parentHwnd = parent;

	WNDCLASSW wc = { 0 };
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hInstance = m_ins;
	wc.lpfnWndProc = detailWindowCallBack;
	wc.lpszClassName = m_className.c_str();
	wc.hbrBackground = CreateSolidBrush(RGB(27, 43, 61));

	RegisterClass(&wc);

	m_hwnd = CreateWindowEx(0,
		m_className.c_str(),
		NULL,
		WS_POPUP,
		0,
		0,
		m_width,
		m_height,
		parent,
		NULL,
		m_ins,
		this);
}

CBaseWindow::~CBaseWindow()
{
	DestroyWindow(m_hwnd);
	UnregisterClassW(m_className.c_str(), m_ins);
}

void CBaseWindow::show(bool bShow)
{
	::ShowWindow(m_hwnd, bShow ? SW_SHOW : SW_HIDE);

}

void CBaseWindow::update()
{
	::InvalidateRect(m_hwnd, NULL, FALSE);
}

void CBaseWindow::setSize(int w, int h)
{
	m_width = w;
	m_height = h;
	SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, w, h, SWP_NOMOVE);
}

void CBaseWindow::setEraseBack(BOOL bEraseBack)
{
	m_bEraseBack = bEraseBack;
}

void CBaseWindow::setMouseTracking(BOOL bMouseTracking)
{
	m_bMouseTracking = bMouseTracking;
}

BOOL CBaseWindow::isHover()
{
	return m_bHover;
}

LRESULT CALLBACK CBaseWindow::detailWindowCallBack(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CBaseWindow* ptr = nullptr;
	if (uMsg == WM_CREATE)
	{
		ptr = reinterpret_cast<CBaseWindow *>(reinterpret_cast<CREATESTRUCT *>(lParam)->lpCreateParams);
		ptr->m_hwnd = hwnd;
		SetWindowLongPtr(ptr->m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ptr));
	}
	else
	{
		ptr = reinterpret_cast<CBaseWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}

	if (ptr)
		return ptr->wndCall(uMsg, wParam, lParam);
	else
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void CBaseWindow::onCreate()
{
}

void CBaseWindow::onMouseMove(WPARAM /*wParam*/, LPARAM /*lParam*/)
{

}

void CBaseWindow::onMouseClick(WPARAM /*wParam*/, LPARAM /*lParam*/)
{

}

void CBaseWindow::onMouseWheel(WPARAM /*wParam*/, LPARAM /*lParam*/)
{

}

void CBaseWindow::onHover()
{
}

void CBaseWindow::onLeave()
{
}

void CBaseWindow::onPaint()
{
}

void CBaseWindow::onTimer(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
}

void CBaseWindow::onShow(WPARAM /*wParam*/, LPARAM /*lParam*/)
{

}

LRESULT CBaseWindow::wndCall(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		onCreate();
		break;
	case WM_MOUSEMOVE:
		if (m_bMouseTracking && !m_bHover)
		{
			util::setMouseTracking(m_hwnd);
		}

		onMouseMove(wParam, lParam);
		break;
	case WM_MOUSEHOVER:
		m_bHover = TRUE;
		if (m_bMouseTracking) onHover();
		break;
	case WM_MOUSELEAVE:
		m_bHover = FALSE;
		if (m_bMouseTracking) onLeave();
		break;
	case WM_LBUTTONDOWN:
		onMouseClick(wParam, lParam);
		break;
	case WM_MOUSEWHEEL:
		onMouseWheel(wParam, lParam);
		break;
	case WM_PAINT:
		onPaint();
		break;
	case WM_TIMER:
		onTimer(wParam, lParam);
		break;
	case WM_SHOWWINDOW:
		onShow(wParam, lParam);
		break;
	default:
		break;
	}

	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}