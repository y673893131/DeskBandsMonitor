#include "MainWindow.h"
#include "../DeskBand.h"
#include "../Log/Log.h"
#include "../monitor/SysTaskMgr.h"
#include "../menu/PopMenu.h"
#include "../export.h"

#define MB(x) ::MessageBox(NULL, x, L"Tips", MB_OK);
#define ST(x) ::SetTimer(m_hwnd, x, 1000, NULL)
#define KT(x) ::KillTimer(m_hwnd, x)
#define RECTWIDTH(x)   ((x).right - (x).left)
#define RECTHEIGHT(x)  ((x).bottom - (x).top)
#define TIMER_ID_TEST 1
#define TIMER_ID_DBCLICK 2
#define TIMER_ID_HOVER 3

#define MENU_ID_FIRST 0x1000
#define MENU_ID_CLOSE 0x1000
#define MENU_ID_UNREG 0x1001


CMainWindow::CMainWindow(CDeskBand* parent)
	: m_pDeskBand(parent)
	, m_className(L"DeskBandMonitorClass")
	, m_hInstance(NULL)
	, m_hwnd(NULL)
	, m_bCompositionEnabled(FALSE)
	, m_bFocus(FALSE)
	, m_bHover(FALSE)
	, m_up(NULL)
	, m_down(NULL)
	, m_bPressTrigger(FALSE)
	, m_task(NULL)
	, m_pMenu(NULL)
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
			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(TRACKMOUSEEVENT);
			tme.dwFlags = TME_HOVER | TME_LEAVE;
			tme.dwHoverTime = 1; //1ms ������ʾ
			tme.hwndTrack = hwnd;
			//����WM_MOUSEHOVER��Ϣ
			TrackMouseEvent(&tme);
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
	}

	m_pMenu = new CPopMenu();
	m_pMenu->append(MENU_ID_CLOSE, L"close");
	m_pMenu->append(MENU_ID_UNREG, L"unregster");
}

void CMainWindow::onFocus(BOOL fFocus)
{
	m_bFocus = fFocus;
	m_pDeskBand->setFocus(m_bFocus);
}

void CMainWindow::onHover(BOOL bHover)
{
	m_bHover = bHover;
	if (m_bHover)
		ST(TIMER_ID_HOVER);
	else
		KT(TIMER_ID_HOVER);
}

Image* CMainWindow::gdiGetFileFromResource(UINT pResourceID, LPCTSTR pResourceType)
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
	Gdiplus::Image *pImage = NULL;
	if (CreateStreamOnHGlobal(hResourceBuffer, FALSE, &pIStream) == S_OK)
	{
		pImage = Gdiplus::Image::FromStream(pIStream);
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

void CMainWindow::onPaint(const HDC hdcIn)
{
	HDC hdc = hdcIn;
	PAINTSTRUCT ps;
	static std::wstring szContent = L"Net Speed";
	std::wstring szContentUp, szContentDown;
	static int count = 0;

	auto task = m_task->getTasks();
	szContentUp = /*std::wstring(L"�� ") +*/ CSysTaskMgr::toSpeedStringW(task.net.up);
	szContentDown = /*std::wstring(L"�� ") + */CSysTaskMgr::toSpeedStringW(task.net.down);
	if (!hdc)
	{
		hdc = BeginPaint(m_hwnd, &ps);
	}

	auto pContentUp = szContentUp.c_str();
	int lenUp = (int)szContentUp.length();

	auto pContentDown = szContentDown.c_str();
	int lenDown = (int)szContentDown.length();

	if (hdc)
	{
		RECT rc;
		GetClientRect(m_hwnd, &rc);
		{
			auto mem = CreateCompatibleDC(hdc);
			auto width = GetDeviceCaps(mem, HORZRES);
			auto height = GetDeviceCaps(mem, VERTRES);
			auto bitmap = CreateCompatibleBitmap(hdc, width, height);

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
			g.DrawString(pContentUp, lenUp, &f, r2, &format, &b);
			g.DrawString(pContentDown, lenDown, &f, r3, &format, &b);

			::BitBlt(hdc, 0, 0, width, height, mem, 0, 0, SRCCOPY);
			g.ReleaseHDC(mem);

			DeleteDC(mem);
			DeleteObject(bitmap);
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
	case TIMER_ID_HOVER:
		KT(TIMER_ID_HOVER);
		Log(Log_Info, "HOVER TIMEOUT");
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
}

void CMainWindow::onCommand(WPARAM wParam, LPARAM lParam)
{
	auto action = LOWORD(wParam);
	auto type = HIWORD(wParam);
	if (type == 0)
		onMenu(action, lParam);//�˵�
	else if (type == 1)
		onNotify(action, lParam);//���ټ�
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
	ShExecInfo.lpVerb = L"runas";                                 //     ����Ա��ʽ����
	ShExecInfo.lpFile = cmd.c_str();                              //     ���õĳ�����
	ShExecInfo.lpParameters = par.c_str();                        //     ���ó���������в���
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = nShow;                                     //     ����״̬Ϊ����
	ShExecInfo.hInstApp = NULL;
	ShellExecuteEx(&ShExecInfo);                                  //     �����µĳ���
	//WaitForSingleObject(ShExecInfo.hProcess, INFINITE);    //     �ȵ��ý��̽���
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