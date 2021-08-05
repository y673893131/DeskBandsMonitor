#include <windows.h>
#include <uxtheme.h>
#include "DeskBand.h"
#include "monitor\SysTaskMgr.h"
#include "Log\Log.h"
#define RECTWIDTH(x)   ((x).right - (x).left)
#define RECTHEIGHT(x)  ((x).bottom - (x).top)

extern long         g_cDllRef;
extern HINSTANCE    g_hInst;

extern CLSID CLSID_DeskBandMonitor;

static const WCHAR g_szDeskBandMonitorClass[] = L"DeskBandMonitorClass";

CDeskBand::CDeskBand() :
	m_cRef(1), m_instance(NULL), m_pSite(NULL), m_fHasFocus(FALSE)
	, m_fIsDirty(FALSE), m_dwBandID(0), m_hwnd(NULL), m_hwndParent(NULL)
	, m_bHover(FALSE), m_task(NULL)
{
    InterlockedIncrement(&g_cDllRef);

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	//初始化gdi+
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

CDeskBand::~CDeskBand()
{
    if (m_pSite)
    {
        m_pSite->Release();
    }
    
	//释放gdi+
	GdiplusShutdown(m_gdiplusToken);
	
	if (m_instance)
	{
		UnregisterClassW(g_szDeskBandMonitorClass, m_instance);
		Log(Log_Info, "UnregisterClassW, %d", GetLastError());
	}

	InterlockedDecrement(&g_cDllRef);
	Log(Log_Info, "%s", __FUNCTION__);
}

//
// IUnknown
//
STDMETHODIMP CDeskBand::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = S_OK;

    if (IsEqualIID(IID_IUnknown, riid)       ||
        IsEqualIID(IID_IOleWindow, riid)     ||
        IsEqualIID(IID_IDockingWindow, riid) ||
        IsEqualIID(IID_IDeskBand, riid)      ||
        IsEqualIID(IID_IDeskBand2, riid))
    {
        *ppv = static_cast<IOleWindow *>(this);
    }
    else if (IsEqualIID(IID_IPersist, riid) ||
             IsEqualIID(IID_IPersistStream, riid))
    {
        *ppv = static_cast<IPersist *>(this);
    }
    else if (IsEqualIID(IID_IObjectWithSite, riid))
    {
        *ppv = static_cast<IObjectWithSite *>(this);
    }
    else if (IsEqualIID(IID_IInputObject, riid))
    {
        *ppv = static_cast<IInputObject *>(this);
    }
    else
    {
        hr = E_NOINTERFACE;
        *ppv = NULL;
    }

    if (*ppv)
    {
        AddRef();
    }

    return hr;
}

STDMETHODIMP_(ULONG) CDeskBand::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CDeskBand::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
    {
        delete this;
    }

    return cRef;
}

//
// IOleWindow
//
STDMETHODIMP CDeskBand::GetWindow(HWND *phwnd)
{
    *phwnd = m_hwnd;
    return S_OK;
}

STDMETHODIMP CDeskBand::ContextSensitiveHelp(BOOL)
{
    return E_NOTIMPL;
}

//
// IDockingWindow
//
STDMETHODIMP CDeskBand::ShowDW(BOOL fShow)
{
    if (m_hwnd)
    {
        ShowWindow(m_hwnd, fShow ? SW_SHOW : SW_HIDE);
    }

    return S_OK;
}

STDMETHODIMP CDeskBand::CloseDW(DWORD)
{
    if (m_hwnd)
    {
        ShowWindow(m_hwnd, SW_HIDE);
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }

    return S_OK;
}

STDMETHODIMP CDeskBand::ResizeBorderDW(const RECT *, IUnknown *, BOOL)
{
    return E_NOTIMPL;
}

//
// IDeskBand
//
STDMETHODIMP CDeskBand::GetBandInfo(DWORD dwBandID, DWORD, DESKBANDINFO *pdbi)
{
    HRESULT hr = E_INVALIDARG;

    if (pdbi)
    {
        m_dwBandID = dwBandID;

        if (pdbi->dwMask & DBIM_MINSIZE)
        {
            pdbi->ptMinSize.x = 90;
            pdbi->ptMinSize.y = 30;
        }

        if (pdbi->dwMask & DBIM_MAXSIZE)
        {
            pdbi->ptMaxSize.y = -1;
        }

        if (pdbi->dwMask & DBIM_INTEGRAL)
        {
            pdbi->ptIntegral.y = 1;
        }

        if (pdbi->dwMask & DBIM_ACTUAL)
        {
			pdbi->ptActual.x = 90;
            pdbi->ptActual.y = 30;
        }

        if (pdbi->dwMask & DBIM_TITLE)
        {
            // Don't show title by removing this flag.
            pdbi->dwMask &= ~DBIM_TITLE;
        }

        if (pdbi->dwMask & DBIM_MODEFLAGS)
        {
            pdbi->dwModeFlags = DBIMF_NORMAL | DBIMF_VARIABLEHEIGHT;
        }

        if (pdbi->dwMask & DBIM_BKCOLOR)
        {
            // Use the default background color by removing this flag.
            pdbi->dwMask &= ~DBIM_BKCOLOR;
        }

        hr = S_OK;
    }

    return hr;
}

//
// IDeskBand2
//
STDMETHODIMP CDeskBand::CanRenderComposited(BOOL *pfCanRenderComposited)
{
    *pfCanRenderComposited = TRUE;

    return S_OK;
}

STDMETHODIMP CDeskBand::SetCompositionState(BOOL fCompositionEnabled)
{
    m_fCompositionEnabled = fCompositionEnabled;

    InvalidateRect(m_hwnd, NULL, TRUE);
    UpdateWindow(m_hwnd);

    return S_OK;
}

STDMETHODIMP CDeskBand::GetCompositionState(BOOL *pfCompositionEnabled)
{
    *pfCompositionEnabled = m_fCompositionEnabled;

    return S_OK;
}

//
// IPersist
//
STDMETHODIMP CDeskBand::GetClassID(CLSID *pclsid)
{
	*pclsid = CLSID_DeskBandMonitor;
    return S_OK;
}

//
// IPersistStream
//
STDMETHODIMP CDeskBand::IsDirty()
{
    return m_fIsDirty ? S_OK : S_FALSE;
}

STDMETHODIMP CDeskBand::Load(IStream * /*pStm*/)
{
    return S_OK;
}

STDMETHODIMP CDeskBand::Save(IStream * /*pStm*/, BOOL fClearDirty)
{
    if (fClearDirty)
    {
        m_fIsDirty = FALSE;
    }

    return S_OK;
}

STDMETHODIMP CDeskBand::GetSizeMax(ULARGE_INTEGER * /*pcbSize*/)
{
    return E_NOTIMPL;
}

//
// IObjectWithSite
//
STDMETHODIMP CDeskBand::SetSite(IUnknown *pUnkSite)
{
    HRESULT hr = S_OK;

    m_hwndParent = NULL;

    if (m_pSite)
    {
        m_pSite->Release();
		m_pSite = NULL;
    }

    if (pUnkSite)
    {
        IOleWindow *pow;
        hr = pUnkSite->QueryInterface(IID_IOleWindow, reinterpret_cast<void **>(&pow));
        if (SUCCEEDED(hr))
        {
            hr = pow->GetWindow(&m_hwndParent);
            if (SUCCEEDED(hr))
            {
                WNDCLASSW wc = { 0 };
                wc.style         = CS_HREDRAW | CS_VREDRAW;
                wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
                wc.hInstance     = g_hInst;
                wc.lpfnWndProc   = WndProc;
				wc.lpszClassName = g_szDeskBandMonitorClass;
                wc.hbrBackground = CreateSolidBrush(RGB(0, 255, 255));

				RegisterClassW(&wc);
				Log(Log_Info, "RegisterClassW, %d", GetLastError());
                CreateWindowExW(0,
								g_szDeskBandMonitorClass,
                                NULL,
                                WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                0,
                                0,
                                0,
                                0,
                                m_hwndParent,
                                NULL,
                                g_hInst,
                                this);
				Log(Log_Info, "CreateWindowExW, %d", GetLastError());
                if (!m_hwnd)
                {
                    hr = E_FAIL;
                }
            }

            pow->Release();
        }

        hr = pUnkSite->QueryInterface(IID_IInputObjectSite, reinterpret_cast<void **>(&m_pSite));
    }

    return hr;
}

STDMETHODIMP CDeskBand::GetSite(REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;

    if (m_pSite)
    {
        hr =  m_pSite->QueryInterface(riid, ppv);
    }
    else
    {
        *ppv = NULL;
    }

    return hr;
}

//
// IInputObject
//
STDMETHODIMP CDeskBand::UIActivateIO(BOOL fActivate, MSG *)
{
    if (fActivate)
    {
        SetFocus(m_hwnd);
    }

    return S_OK;
}

STDMETHODIMP CDeskBand::HasFocusIO()
{
    return m_fHasFocus ? S_OK : S_FALSE;
}

STDMETHODIMP CDeskBand::TranslateAcceleratorIO(MSG *)
{
    return S_FALSE;
};

void CDeskBand::OnFocus(const BOOL fFocus)
{
    m_fHasFocus = fFocus;

    if (m_pSite)
    {
        m_pSite->OnFocusChangeIS(static_cast<IOleWindow*>(this), m_fHasFocus);
    }
}

void CDeskBand::OnHover(const BOOL bHover)
{
	m_bHover = bHover;
}

HIMAGELIST g_list = NULL;

Image* CDeskBand::GDIGetImageFromResource(UINT pResourceID, HMODULE hInstance, LPCTSTR	pResourceType)
{
	LPCTSTR pResourceName = MAKEINTRESOURCE(pResourceID);
	HRSRC hResource = FindResource(hInstance, pResourceName, pResourceType);
	if (!hResource)
	{
		wchar_t buffer[256] = {};
		wsprintf(buffer, L"[%p][%s][%d][]1", hInstance, pResourceName, GetLastError());
		::MessageBoxW(NULL, buffer, L"tips", MB_OK);
		return NULL;
	}
	
	DWORD dwResourceSize = SizeofResource(hInstance, hResource);
	if (!dwResourceSize)
	{
		::MessageBoxA(NULL, "2", "tips", MB_OK);
		return NULL;
	}

	const void* pResourceData = LockResource(LoadResource(hInstance, hResource));
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

void CDeskBand::OnPaint(const HDC hdcIn)
{
    HDC hdc = hdcIn;
    PAINTSTRUCT ps;
    static std::wstring szContent = L"Net Speed";
	std::wstring szContentUp, szContentDown;
	static int count = 0;

	auto task = m_task->getTasks();
	szContentUp = /*std::wstring(L"↑ ") +*/ CSysTaskMgr::toSpeedStringW(task.net.up);
	szContentDown = /*std::wstring(L"↓ ") + */CSysTaskMgr::toSpeedStringW(task.net.down);
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

#define TIMER_ID_TEST 1
LRESULT CALLBACK CDeskBand::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;

    CDeskBand *pDeskBand = reinterpret_cast<CDeskBand *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (uMsg)
    {
    case WM_CREATE:
        pDeskBand = reinterpret_cast<CDeskBand *>(reinterpret_cast<CREATESTRUCT *>(lParam)->lpCreateParams);
        pDeskBand->m_hwnd = hwnd;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pDeskBand));
#ifdef _M_X64
		pDeskBand->m_instance = GetModuleHandle(L"DeskbandMonitor_x64.dll");
#else
		pDeskBand->m_instance = GetModuleHandle(L"DeskbandMonitor_x86.dll");
#endif

		Log(Log_Info, "WM_CREATE: instance: 0x%x thead_id:%d", pDeskBand->m_instance, ::GetCurrentThreadId());

		pDeskBand->m_task = new CSysTaskMgr();
		SetTimer(hwnd, TIMER_ID_TEST, 1000, NULL);
		{
			pDeskBand->m_up = pDeskBand->GDIGetImageFromResource(IDB_PNG1, pDeskBand->m_instance, L"PNG");
			pDeskBand->m_down = pDeskBand->GDIGetImageFromResource(IDB_PNG2, pDeskBand->m_instance, L"PNG");
		}
		
        break;
    case WM_SETFOCUS:
        pDeskBand->OnFocus(TRUE);
        break;

    case WM_KILLFOCUS:
        pDeskBand->OnFocus(FALSE);
        break;

	case WM_MOUSEMOVE:
		if (pDeskBand->m_bHover == FALSE)
		{
			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(TRACKMOUSEEVENT);
			tme.dwFlags = TME_HOVER | TME_LEAVE;
			tme.dwHoverTime = 1; //1ms 立即显示
			tme.hwndTrack = hwnd;
			//激活WM_MOUSEHOVER消息
			TrackMouseEvent(&tme);
		}
		break;
	case WM_MOUSEHOVER:
		pDeskBand->OnHover(TRUE);
		{
			HDC hdc = GetDC(hwnd);
			pDeskBand->OnPaint(hdc);
		}
		break;

	case WM_MOUSELEAVE:
		pDeskBand->OnHover(FALSE);
		{
			HDC hdc = GetDC(hwnd);
			pDeskBand->OnPaint(hdc);
		}
		break;

    case WM_PAINT:
        pDeskBand->OnPaint(NULL);
        break;

    case WM_PRINTCLIENT:
        pDeskBand->OnPaint(reinterpret_cast<HDC>(wParam));
        break;

    case WM_ERASEBKGND:
        if (pDeskBand->m_fCompositionEnabled)
        {
            lResult = 1;
        }
        break;
	case WM_TIMER:
		if (wParam == TIMER_ID_TEST)
		{
			HDC hdc = GetDC(hwnd);
			pDeskBand->OnPaint(hdc);
		}
		break;
	case WM_DESTROY:
		pDeskBand->release();
		break;
    }

    if (uMsg != WM_ERASEBKGND)
    {
        lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return lResult;
}

void CDeskBand::release()
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

	//Log(Log_Info, "release");
}