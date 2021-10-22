#include "MainWindow.h"
#include "../Log/Log.h"
#include "../monitor/SysTaskMgr.h"
#include "../menu/PopMenu.h"
#include "../tooltip/TooltipDetail.h"
#include "DetailWindow.h"
#include "../export.h"
#include "../util/Util.h"

#include <Vsstyle.h>

#define MB(x) ::MessageBox(NULL, x, L"Tips", MB_OK);
#define ST(x, y) ::SetTimer(m_hwnd, x, y, NULL)
#define KT(x) ::KillTimer(m_hwnd, x)
#define RECTWIDTH(x)   ((x).right - (x).left)
#define RECTHEIGHT(x)  ((x).bottom - (x).top)
#define TIMER_ID_TEST 1
#define TIMER_ID_DBCLICK 2
#define TIMER_ID_LEAVE 3

#define MENU_ID_FIRST 0x1000
#define MENU_ID_CLOSE 0x1000
#define MENU_ID_UNREG 0x1001


CMainWindow::CMainWindow()
	: m_className(L"DeskBandMonitorClass")
	, m_hInstance(NULL)
	, m_hwnd(NULL)
	, m_bCompositionEnabled(FALSE)
	, m_bFocus(FALSE)
	, m_bHover(FALSE)
	, m_up(NULL)
	, m_down(NULL)
	, m_imgList(NULL)
	, m_bPressTrigger(FALSE)
	, m_task(NULL)
	, m_pMenu(NULL)
	, m_pTooltip(NULL)
	, m_pDetailWindow(NULL)
{
}

CMainWindow::~CMainWindow()
{
	unRegster();
}

BOOL CMainWindow::regster(HINSTANCE hInstance)
{
	WNDCLASSW wc = { 0 };
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hInstance = hInstance;
	wc.lpfnWndProc = windowCallBack;
	wc.lpszClassName = m_className.c_str();
	wc.hbrBackground = CreateSolidBrush(RGB(0, 255, 255));

	RegisterClassW(&wc);

	m_hInstance = hInstance;
	return TRUE;
}

void CMainWindow::unRegster()
{
	UnregisterClassW(m_className.c_str(), m_hInstance);
	Log(Log_Info, "UnregisterClassW, %d", GetLastError());
}

BOOL CMainWindow::create(HINSTANCE hInstance, HWND parent)
{
	CreateWindowExW(0,
		m_className.c_str(),
		NULL,
		WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		0,
		0,
		0,
		0,
		parent,
		NULL,
		hInstance,
		this);
	Log(Log_Info, "CreateWindowExW, %d", GetLastError());
	if (!m_hwnd)
	{
		return FALSE;
	}

	return TRUE;
}

HWND CMainWindow::getWindow()
{
	return m_hwnd;
}

void CMainWindow::show(BOOL bShow)
{
	if (m_hwnd)
	{
		ShowWindow(m_hwnd, bShow ? SW_SHOW : SW_HIDE);
	}
}

void CMainWindow::close()
{
	if (m_hwnd)
	{
		ShowWindow(m_hwnd, SW_HIDE);
		DestroyWindow(m_hwnd);
		m_hwnd = NULL;
	}
}

void CMainWindow::setCompositionState(BOOL bCompositionEnabled)
{
	m_bCompositionEnabled = bCompositionEnabled;
	InvalidateRect(m_hwnd, NULL, TRUE);
	UpdateWindow(m_hwnd);
}

BOOL CMainWindow::getCompositionState()
{
	return m_bCompositionEnabled;
}

void CMainWindow::setFocus()
{
	SetFocus(m_hwnd);
}

BOOL CMainWindow::getFocus()
{
	return m_bFocus;
}

LRESULT CALLBACK CMainWindow::windowCallBack(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult = 0;

	auto pThis = reinterpret_cast<CMainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	switch (uMsg)
	{
	case WM_CREATE:
		pThis = reinterpret_cast<CMainWindow *>(reinterpret_cast<CREATESTRUCT *>(lParam)->lpCreateParams);
		pThis->m_hwnd = hwnd;
		pThis->onCreate();
		break;

	case WM_SETFOCUS:
		pThis->onFocus(TRUE);
		break;

	case WM_KILLFOCUS:
		pThis->onFocus(FALSE);
		break;

	case WM_MOUSEMOVE:
		if (pThis->m_bHover == FALSE)
		{
			util::setMouseTracking(pThis->m_hwnd);
		}
		break;
	case WM_MOUSEHOVER:
		pThis->onHover(TRUE);
		{
			HDC hdc = GetDC(hwnd);
			pThis->onPaint(hdc);
		}
		break;

	case WM_MOUSELEAVE:
		pThis->onHover(FALSE);
		{
			HDC hdc = GetDC(hwnd);
			pThis->onPaint(hdc);
		}
		break;
	case WM_LBUTTONDOWN:
		pThis->onLeftClick(wParam);
		break;
	case WM_LBUTTONDBLCLK:
		pThis->onLeftDubleClick();
		break;

	case WM_RBUTTONUP:
		pThis->onRightClick(lParam);
		return TRUE;

	case WM_PAINT:
		pThis->onPaint(NULL);
		break;

	case WM_PRINTCLIENT:
		pThis->onPaint(reinterpret_cast<HDC>(wParam));
		break;

	case WM_ERASEBKGND:
		if (pThis->m_bCompositionEnabled)
		{
			lResult = 1;
		}
		break;
	case WM_TIMER:
		pThis->onTimer(static_cast<UINT>(wParam));
		break;

	case WM_DESTROY:
		pThis->onDestroy();
		break;
	case WM_COMMAND:
		pThis->onCommand(wParam, lParam);
		break;
	case WM_MENUCOMMAND:
		break;
	}

	if (uMsg != WM_ERASEBKGND)
	{
		lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return lResult;
}

void CMainWindow::onCreate()
{
	Log(Log_Info, "WM_CREATE: instance: 0x%x thead_id:%d", m_hInstance, ::GetCurrentThreadId());
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

	m_task = new CSysTaskMgr();
	SetTimer(m_hwnd, TIMER_ID_TEST, 1000, NULL);
	{
		m_up = gdiGetFileFromResource(IDB_PNG1, L"PNG");
		m_down = gdiGetFileFromResource(IDB_PNG2, L"PNG");

		m_imgList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 2, 0);
		HBITMAP bmp_up, bmp_down;
		Gdiplus::Color c(0, 255, 255, 255);
		m_up->GetHBITMAP(c, &bmp_up);
		m_down->GetHBITMAP(c, &bmp_down);
		ImageList_AddMasked(m_imgList, bmp_up, RGB(255, 255, 255));
		ImageList_AddMasked(m_imgList, bmp_down, RGB(255, 255, 255));
	}

	m_pMenu = new CPopMenu();
	m_pMenu->append(MENU_ID_CLOSE, L"close");
	m_pMenu->append(MENU_ID_UNREG, L"unregster");

	m_pTooltip = new CTooltipDetail(m_hInstance, m_hwnd);
	m_pDetailWindow = new CDetailWindow(m_hwnd);
}

void CMainWindow::onFocus(BOOL fFocus)
{
	m_bFocus = fFocus;
}

void CMainWindow::onHover(BOOL bHover)
{
	m_bHover = bHover;
	KT(TIMER_ID_LEAVE);
	if (m_bHover)
	{
		m_pDetailWindow->show(true);
	}
	else
	{
		ST(TIMER_ID_LEAVE, 500);
	}
}

Bitmap* CMainWindow::gdiGetFileFromResource(UINT pResourceID, LPCTSTR pResourceType)
{
	LPCTSTR pResourceName = MAKEINTRESOURCE(pResourceID);
	HRSRC hResource = FindResource(m_hInstance, pResourceName, pResourceType);
	if (!hResource)
	{
		wchar_t buffer[256] = {};
		wsprintf(buffer, L"[%p][%s][%d][]1", m_hInstance, pResourceName, GetLastError());
		::MessageBoxW(NULL, buffer, L"tips", MB_OK);
		return NULL;
	}

	DWORD dwResourceSize = SizeofResource(m_hInstance, hResource);
	if (!dwResourceSize)
	{
		::MessageBoxA(NULL, "2", "tips", MB_OK);
		return NULL;
	}

	const void* pResourceData = LockResource(LoadResource(m_hInstance, hResource));
	if (!pResourceData)
	{
		::MessageBoxA(NULL, "3", "tips", MB_OK);
		return NULL;
	}

	HGLOBAL	hResourceBuffer = GlobalAlloc(GMEM_MOVEABLE, dwResourceSize);
	if (!hResourceBuffer)
	{
		::MessageBoxA(NULL, "4", "tips", MB_OK);
		return NULL;
	}

	void* pResourceBuffer = GlobalLock(hResourceBuffer);
	if (!pResourceBuffer)
	{
		GlobalFree(hResourceBuffer);
		::MessageBoxA(NULL, "5", "tips", MB_OK);
		return NULL;
	}

	CopyMemory(pResourceBuffer, pResourceData, dwResourceSize);

	IStream* pIStream = NULL;
	Gdiplus::Bitmap *pImage = NULL;
	if (CreateStreamOnHGlobal(hResourceBuffer, FALSE, &pIStream) == S_OK)
	{
		pImage = Gdiplus::Bitmap::FromStream(pIStream);
		pIStream->Release();
	}
	else
	{
		::MessageBoxA(NULL, "6", "tips", MB_OK);
	}
	GlobalUnlock(hResourceBuffer);
	GlobalFree(hResourceBuffer);

	return pImage;
}

void CMainWindow::onGdiPlusPaint(const HDC hdcIn)
{
	RECT rc;
	GetClientRect(m_hwnd, &rc);
	auto mem = CreateCompatibleDC(hdcIn);
	auto width = GetDeviceCaps(mem, HORZRES);
	auto height = GetDeviceCaps(mem, VERTRES);
	auto bitmap = CreateCompatibleBitmap(hdcIn, width, height);

	SelectObject(mem, bitmap);

	auto hBrush = CreateSolidBrush(NULL_BRUSH);
	SetBkMode(mem, TRANSPARENT);
	FillRect(mem, &rc, hBrush);
	DeleteObject(hBrush);

	Gdiplus::Graphics g(mem);
	if (m_bHover)
	{
		Gdiplus::SolidBrush b(Gdiplus::Color(220, 0x32, 0x40, 0x53));
		Gdiplus::Rect r(rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc));
		g.FillRectangle(&b, r);
	}
	
	Gdiplus::Rect r0(5, (RECTHEIGHT(rc) / 2 - m_up->GetHeight()) / 2, m_up->GetWidth(), m_up->GetHeight());
	g.DrawImage(m_up, r0);

	Gdiplus::Rect r1(5, (RECTHEIGHT(rc) / 2 - m_down->GetHeight()) / 2 + RECTHEIGHT(rc) / 2, m_down->GetWidth(), m_down->GetHeight());
	g.DrawImage(m_down, r1);

	Gdiplus::FontFamily fontFamily(L"Microsoft YaHei");
	Gdiplus::Font f(&fontFamily, 10, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
	Gdiplus::StringFormat format;
	format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
	Gdiplus::SolidBrush b(Gdiplus::Color(255, 255, 255));

	float h = f.GetHeight(&g);
	float y = (RECTHEIGHT(rc) - 2 * h) / 3;
	Gdiplus::RectF r2((float)r0.GetRight() + 5, y, (float)RECTWIDTH(rc) - r0.GetRight() + 5, h);
	Gdiplus::RectF r3((float)r0.GetRight() + 5, y * 2 + h, (float)RECTWIDTH(rc) - r0.GetRight() + 5, h);

	g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

	auto task = m_task->getTasks();
	m_pDetailWindow->setNet(task);
	std::wstring szContentUp, szContentDown;
	szContentUp = CSysTaskMgr::toSpeedStringW(task.total.up);
	szContentDown = CSysTaskMgr::toSpeedStringW(task.total.down);
	
	auto pContentUp = szContentUp.c_str();
	int lenUp = (int)szContentUp.length();

	auto pContentDown = szContentDown.c_str();
	int lenDown = (int)szContentDown.length();

	g.DrawString(pContentUp, lenUp, &f, r2, &format, &b);
	g.DrawString(pContentDown, lenDown, &f, r3, &format, &b);

	::BitBlt(hdcIn, 0, 0, width, height, mem, 0, 0, SRCCOPY);
	g.ReleaseHDC(mem);

	DeleteDC(mem);
	DeleteObject(bitmap);
}

void CMainWindow::onThemePaint(const HDC hdcIn, HTHEME hTheme)
{
	RECT rc;
	GetClientRect(m_hwnd, &rc);
	HDC hdcPaint = NULL;
	HPAINTBUFFER hBufferedPaint = BeginBufferedPaint(hdcIn, &rc, BPBF_TOPDOWNDIB, NULL, &hdcPaint);
	
	DrawThemeParentBackground(m_hwnd, hdcPaint, &rc);

	if (m_bHover)
	{
		Gdiplus::Graphics g(hdcPaint);
		Gdiplus::SolidBrush b(Gdiplus::Color(220, 0x32, 0x40, 0x53));
		Gdiplus::Rect r(rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc));
		g.FillRectangle(&b, r);
		g.ReleaseHDC(hdcPaint);
	}
	// up.png	
	RECT r0;
	r0.left = 5;
	r0.top = 2;
	r0.right = 5 + 16;
	r0.bottom = r0.top + 16;
	DrawThemeIcon(hTheme, hdcPaint, BP_PUSHBUTTON, PBS_DISABLED, &r0, m_imgList, 0);

	r0.top = r0.bottom + 5;
	r0.bottom = r0.top + 16;
	DrawThemeIcon(hTheme, hdcPaint, BP_PUSHBUTTON, PBS_DISABLED, &r0, m_imgList, 1);

	auto task = m_task->getTasks();
	m_pDetailWindow->setNet(task);
	std::wstring szContentUp, szContentDown;
	szContentUp = /*std::wstring(L"↑ ") +*/ CSysTaskMgr::toSpeedStringW(task.total.up);
	szContentDown = /*std::wstring(L"↓ ") + */CSysTaskMgr::toSpeedStringW(task.total.down);
	auto pContentUp = szContentUp.c_str();
	int lenUp = (int)szContentUp.length();
	auto pContentDown = szContentDown.c_str();
	//int lenDown = (int)szContentDown.length();

	SIZE size;
	GetTextExtentPointW(hdcIn, pContentUp, lenUp, &size);
	int space = (RECTHEIGHT(rc) - 2 * size.cy) / 3;

	RECT r2;
	r2.left = 5 + r0.right;
	r2.top = space;
	r2.right = RECTWIDTH(rc) - 5;
	r2.bottom = r2.top + space + size.cy;

	RECT r3;
	r3.left = r2.left;
	r3.top = r2.bottom + space;
	r3.right = RECTWIDTH(rc) - 5;
	r3.bottom = rc.bottom;

	DTTOPTS dttOpts = { sizeof(dttOpts) };
	dttOpts.dwFlags = DTT_COMPOSITED | DTT_TEXTCOLOR | DTT_GLOWSIZE;
	dttOpts.crText = RGB(255, 255, 255);
	dttOpts.iGlowSize = 0;

	static auto font = CreateFont(18, // nHeight
		0, // nWidth
		0, // nEscapement
		0, // nOrientation
		FW_NORMAL, // nWeight
		FALSE, // bItalic
		FALSE, // bUnderline
		0, // cStrikeOut
		ANSI_CHARSET, // nCharSet
		OUT_DEFAULT_PRECIS, // nOutPrecision
		CLIP_DEFAULT_PRECIS, // nClipPrecision
		DEFAULT_QUALITY, // nQuality
		DEFAULT_PITCH | FF_SWISS, // nPitchAndFamily
		L"Microsoft YaHei"); // lpszFac

	SelectObject(hdcPaint, font);
	DrawThemeTextEx(hTheme, hdcPaint, 0, 0, pContentUp, -1, 0, &r2, &dttOpts);
	DrawThemeTextEx(hTheme, hdcPaint, 0, 0, pContentDown, -1, 0, &r3, &dttOpts);

	EndBufferedPaint(hBufferedPaint, TRUE);
	CloseThemeData(hTheme);
}

void CMainWindow::onPaint(const HDC hdcIn)
{
	HDC hdc = hdcIn;
	PAINTSTRUCT ps;
	if (!hdc)
	{
		hdc = BeginPaint(m_hwnd, &ps);
	}

	if (hdc)
	{
		RECT rc;
		GetClientRect(m_hwnd, &rc);
		if (getCompositionState())
		{
			HTHEME hTheme = OpenThemeData(NULL, L"BUTTON");
			if (hTheme)
			{
				onThemePaint(hdc, hTheme);
			}
			else
			{
				onGdiPlusPaint(hdc);
			}
		}
	}

	if (!hdcIn)
	{
		EndPaint(m_hwnd, &ps);
	}

	DeleteDC(hdc);
}

void CMainWindow::onLeftClick(WPARAM wParam)
{
	if (wParam & 0x8000)
	{
		//MB(L"OnLeftClick");
	}
	else
	{
		m_bPressTrigger = TRUE;
		auto ms = GetDoubleClickTime();
		SetTimer(m_hwnd, TIMER_ID_DBCLICK, ms, NULL);
	}
}

void CMainWindow::onLeftDubleClick()
{
	m_bPressTrigger = FALSE;
	Log(Log_Info, "left double click trigger.");
}

void CMainWindow::onRightClick(LPARAM /*lParam*/)
{
	POINT pt;
	::GetCursorPos(&pt);
	m_pMenu->pop(m_hwnd, pt.x, pt.y);
}

void CMainWindow::onTimer(UINT id)
{
	switch (id)
	{
	case TIMER_ID_TEST:
	{
		HDC hdc = GetDC(m_hwnd);
		onPaint(hdc);
	}break;
	case TIMER_ID_DBCLICK:
	{
		KT(TIMER_ID_DBCLICK);
		if (m_bPressTrigger)
		{
			PostMessage(m_hwnd, WM_LBUTTONDOWN, MK_LBUTTON | 0x8000, 0);
		}
	}break;
	case TIMER_ID_LEAVE:
		KT(TIMER_ID_LEAVE);
		//m_pTooltip->tooltip(L"hahahahahahahahahha");
		if (!m_pDetailWindow->isHover())
			m_pDetailWindow->show(false);
		break;
	default:
		break;
	}
}

void CMainWindow::onDestroy()
{
	KillTimer(m_hwnd, TIMER_ID_TEST);
	//CoFreeUnusedLibraries();

	if (m_up)
	{
		delete m_up;
		m_up = NULL;
	}

	if (m_down)
	{
		delete m_down;
		m_down = NULL;
	}

	if (m_imgList)
	{
		ImageList_Destroy(m_imgList);
		m_imgList = NULL;
	}

	if (m_task)
	{
		delete m_task;
		m_task = NULL;
	}

	if (m_pMenu)
	{
		delete m_pMenu;
		m_pMenu = NULL;
	}

	if (m_pTooltip)
	{
		delete m_pTooltip;
		m_pTooltip = NULL;
	}

	if (m_pDetailWindow)
	{
		delete m_pDetailWindow;
		m_pDetailWindow = NULL;
	}
}

void CMainWindow::onCommand(WPARAM wParam, LPARAM lParam)
{
	auto action = LOWORD(wParam);
	auto type = HIWORD(wParam);
	if (type == 0)
		onMenu(action, lParam);//菜单
	else if (type == 1)
		onNotify(action, lParam);//加速键
}

void CMainWindow::onMenu(WPARAM action, LPARAM /*lParam*/)
{
	switch (action)
	{
	case MENU_ID_CLOSE:
		onMenuClose();
		break;
	case MENU_ID_UNREG:
		onMenuUnreg();
		break;
	default:
		break;
	}
}

void CMainWindow::onNotify(WPARAM, LPARAM)
{

}

void CMainWindow::onMenuClose()
{
	Log(Log_Info, "Menu Close trigger.");
	DllShowMonitor(FALSE);
}

void executeCommand(const std::wstring& cmd, const std::wstring& par, int nShow)
{
	SHELLEXECUTEINFO ShExecInfo = { 0 };
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = L"runas";                                 //     管理员方式启动
	ShExecInfo.lpFile = cmd.c_str();                              //     调用的程序名
	ShExecInfo.lpParameters = par.c_str();                        //     调用程序的命令行参数
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = nShow;                                     //     窗口状态为隐藏
	ShExecInfo.hInstApp = NULL;
	ShellExecuteEx(&ShExecInfo);                                  //     启动新的程序
	//WaitForSingleObject(ShExecInfo.hProcess, INFINITE);    //     等到该进程结束
}

void CMainWindow::onMenuUnreg()
{
	Log(Log_Info, "Menu Unregster trigger.");
	
	wchar_t path[1024] = {};
	GetModuleFileName(m_hInstance, path, 1024);
	auto *pos = wcsrchr(path, '\\');
	if (pos) *pos = 0;
	std::wstring sExe = std::wstring(path) + L"\\DeskbandInstall";

	executeCommand(sExe, L"-uninstall", SW_HIDE);
}