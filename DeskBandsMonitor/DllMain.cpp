#include <windows.h>
#include <strsafe.h> // for StringCchXXX functions
#include <olectl.h> // for SELFREG_E_CLASS
#include <shlobj.h> // for ICatRegister
#include "ClassFactory.h" // for the class factory
#include "Log\Log.h"
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

const wchar_t szAppName[] = L"___DeskBandsMonitor";
const wchar_t szName[] = L"Net Speed";

// {BF57B3D6-AB54-4D35-8639-448539E2B894}
CLSID CLSID_DeskBandMonitor = { 0xBF57B3D6, 0xAB54, 0x4D35, { 0x86, 0x39, 0x44, 0x85, 0x39, 0xE2, 0xB8, 0x94 } };

HINSTANCE   g_hInst     = NULL;
long        g_cDllRef   = 0;
HWND		m_notifyHend = NULL;
BOOL		m_bFound = FALSE;

STDAPI_(BOOL) DllMain(HINSTANCE hInstance, DWORD dwReason, void *)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		g_hInst = hInstance;
		//DisableThreadLibraryCalls(hInstance);
		break;
	case DLL_PROCESS_DETACH:
		break;
	default:
		break;
	}

    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;

	if (IsEqualCLSID(CLSID_DeskBandMonitor, rclsid))
    {
        hr = E_OUTOFMEMORY;

        CClassFactory *pClassFactory = new CClassFactory();
        if (pClassFactory)
        {
            hr = pClassFactory->QueryInterface(riid, ppv);
            pClassFactory->Release();
        }
    }

	Log(Log_Info, "DllGetClassObject[%d][%d]", hr, g_cDllRef);
    return hr;
}

STDAPI DllCanUnloadNow()
{
	Log(Log_Info, "DllCanUnloadNow[%d]", g_cDllRef);
    return g_cDllRef > 0 ? S_FALSE : S_OK;
}

HRESULT RegisterServer()
{
    WCHAR szCLSID[MAX_PATH];
    StringFromGUID2(CLSID_DeskBandMonitor, szCLSID, ARRAYSIZE(szCLSID));

    WCHAR szSubkey[MAX_PATH];
    HKEY hKey;

    HRESULT hr = StringCchPrintfW(szSubkey, ARRAYSIZE(szSubkey), L"CLSID\\%s", szCLSID);
    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
        if (ERROR_SUCCESS == RegCreateKeyExW(HKEY_CLASSES_ROOT,
                                             szSubkey,
                                             0,
                                             NULL,
                                             REG_OPTION_NON_VOLATILE,
                                             KEY_WRITE,
                                             NULL,
                                             &hKey,
                                             NULL))
        {
            
            if (ERROR_SUCCESS == RegSetValueExW(hKey,
                                                NULL,
                                                0,
                                                REG_SZ,
                                                (LPBYTE) szName,
                                                sizeof(szName)))
            {
                hr = S_OK;
            }

            RegCloseKey(hKey);
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = StringCchPrintfW(szSubkey, ARRAYSIZE(szSubkey), L"CLSID\\%s\\InprocServer32", szCLSID);
        if (SUCCEEDED(hr))
        {
            hr = HRESULT_FROM_WIN32(RegCreateKeyExW(HKEY_CLASSES_ROOT, szSubkey,
                 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL));
            if (SUCCEEDED(hr))
            {
                WCHAR szModule[MAX_PATH];
                if (GetModuleFileNameW(g_hInst, szModule, ARRAYSIZE(szModule)))
                {
                    DWORD cch = lstrlen(szModule);
                    hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, NULL, 0, REG_SZ, (LPBYTE) szModule, cch * sizeof(szModule[0])));
                }

                if (SUCCEEDED(hr))
                {
                    WCHAR const szModel[] = L"Apartment";
                    hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, L"ThreadingModel", 0,  REG_SZ, (LPBYTE) szModel, sizeof(szModel)));
                }

                RegCloseKey(hKey);
            }
        }
    }

    return hr;
}

HRESULT RegisterComCat(BOOL bRegster)
{
    ICatRegister *pcr;
	CoInitialize(NULL);
    HRESULT hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pcr));
    if (SUCCEEDED(hr))
    {
        CATID catid = CATID_DeskBand;
		if (bRegster)
			hr = pcr->RegisterClassImplCategories(CLSID_DeskBandMonitor, 1, &catid);
		else
			hr = pcr->UnRegisterClassImplCategories(CLSID_DeskBandMonitor, 1, &catid);
        pcr->Release();
    }

	CoUninitialize();
    return hr;
}

STDAPI DllRegisterServer()
{
    // Register the deskband object.
    HRESULT hr = RegisterServer();
    if (SUCCEEDED(hr))
    {
        // Register the component category for the deskband object.
        hr = RegisterComCat(TRUE);
    }

	Log(Log_Info, "DllRegisterServer[%d]", hr);
    return SUCCEEDED(hr) ? S_OK : SELFREG_E_CLASS;
}

STDAPI DllUnregisterServer()
{
	WCHAR szCLSID[MAX_PATH];
	StringFromGUID2(CLSID_DeskBandMonitor, szCLSID, ARRAYSIZE(szCLSID));

	WCHAR szSubkey[MAX_PATH];
	HRESULT hr = StringCchPrintfW(szSubkey, ARRAYSIZE(szSubkey), L"CLSID\\%s", szCLSID);
	if (SUCCEEDED(hr))
	{
		if (ERROR_SUCCESS != RegDeleteTreeW(HKEY_CLASSES_ROOT, szSubkey))
		{
			hr = E_FAIL;
		}
	}

	Log(Log_Info, "DllUnregisterServer[%d]", hr);
	return SUCCEEDED(hr) ? S_OK : SELFREG_E_CLASS;
}

STDAPI DLLIsRegster()
{
	wchar_t szCLSID[] = L"CLSID\\";
	wchar_t szSubKey[_countof(szCLSID) - 1 + 39 - 1 + 1] = { 0 };
	wcscpy_s(szSubKey, szCLSID);
	StringFromGUID2(CLSID_DeskBandMonitor, szSubKey + _countof(szCLSID) - 1, 39);
	HKEY hKey;
	LONG l = RegOpenKey(HKEY_CLASSES_ROOT, szSubKey, &hKey);
	if (!l)
	{
		RegCloseKey(hKey);
		return S_OK;
	}
	else if (2 == l)
		return S_FALSE;
	else
		return E_FAIL;
}

BOOL CALLBACK enumWindowCallBack(HWND hwnd, LPARAM l)
{
	if (m_bFound)
		return FALSE;

	TCHAR szResult[MAX_PATH] = { 0 };
	if (IsWindow(hwnd))
	{
		GetClassName(hwnd, szResult, MAX_PATH);
		if (0 == lstrcmp(szResult, L"#32770"))
		{
			GetWindowText(hwnd, szResult, MAX_PATH);
			if (0 == lstrcmp(szResult, szName))
			{
				m_notifyHend = hwnd;
				m_bFound = TRUE;
				return FALSE;
			}
		}

		EnumChildWindows(hwnd, enumWindowCallBack, l);
	}
	return TRUE;
}

HWND FindDeskBandWindow()
{
	m_bFound = FALSE;
	EnumWindows(enumWindowCallBack, NULL);
	return m_notifyHend;
}

bool AutoClickNotifyButton()
{
	auto hwnd = FindDeskBandWindow();
	if (!hwnd)
	{
		return false;
	}

	HWND hAffirmWindow = FindWindowEx(hwnd, NULL, L"DirectUIHWND", NULL);
	if (hAffirmWindow)
	{
		HWND hSink = nullptr;
		int nRetryTimes = 1000;
		do
		{
			hSink = FindWindowEx(hAffirmWindow, hSink, L"CtrlNotifySink", NULL);
			if (hSink)
			{
				HWND hButton = FindWindowEx(hSink, NULL, L"Button", NULL);
				if (hButton)
				{
					WCHAR lsCaption[MAX_PATH] = { 0 };
					GetWindowText(hButton, lsCaption, MAX_PATH);
					if (0 == lstrcmp(lsCaption, L"是(&Y)"))
					{
						SendMessage(hSink, WM_COMMAND, BN_CLICKED, (LPARAM)hButton);
						ShowWindow(hAffirmWindow, SW_HIDE);
						return true;
					}
				}
			}
		} while (hSink != nullptr && nRetryTimes--);
	}

	return false;
}

LRESULT CALLBACK CallWndRetProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	LPCWPRETSTRUCT lpMsg = (LPCWPRETSTRUCT)lParam;
	if (HC_ACTION == nCode&&lpMsg&&WM_INITDIALOG == lpMsg->message&&lpMsg->hwnd)
	{
		int titleLen = GetWindowTextLengthW(lpMsg->hwnd);
		wchar_t* lpStr = new wchar_t[titleLen + 1]();
		GetWindowTextW(lpMsg->hwnd, lpStr, titleLen + 1);
		if (!wcscmp(lpStr, szAppName))
		{
			HWND hD = FindWindowEx(lpMsg->hwnd, NULL, TEXT("DirectUIHWND"), NULL);
			if (hD)
			{
				for (HWND hc = FindWindowEx(hD, NULL, TEXT("CtrlNotifySink"), NULL);
					hc; hc = FindWindowEx(hD, hc, TEXT("CtrlNotifySink"), NULL))
				{
					HWND hb = FindWindowEx(hc, NULL, WC_BUTTON, NULL);
					if (hb&&GetWindowLongPtr(hb, GWL_STYLE)&BS_DEFPUSHBUTTON)
					{
						SendMessage(hc, WM_COMMAND, BN_CLICKED, (LPARAM)hb);
						HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, TRUE, szAppName);
						SetEvent(hEvent);
						break;
					}
				}
			}
		}
		delete[] lpStr;
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

STDAPI DllShowMonitor(BOOL bShow)
{
	ITrayDeskBand *pTrayDeskBand = NULL;
	CoInitialize(NULL);
	HRESULT hr = CoCreateInstance(CLSID_TrayDeskBand, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pTrayDeskBand));
	// Vista and higher operating system
	if (SUCCEEDED(hr))
	{
		if (TRUE == bShow)
		{
			hr = pTrayDeskBand->DeskBandRegistrationChanged();
			if (SUCCEEDED(hr))
			{
				int i = 5;
				for (hr = pTrayDeskBand->IsDeskBandShown(CLSID_DeskBandMonitor);
					FAILED(hr) && i > 0;
					--i, hr = pTrayDeskBand->IsDeskBandShown(CLSID_DeskBandMonitor))
					Sleep(100);
				if (SUCCEEDED(hr) && (S_FALSE == hr))
				{
					//HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, szAppName);
					//HHOOK hhk = SetWindowsHookEx(WH_CALLWNDPROCRET, CallWndRetProc, g_hInst, 0);
					//hr = pTrayDeskBand->ShowDeskBand(CLSID_DeskBandMonitor);
					//WaitForSingleObject(hEvent, 5000/*INFINITE*/);
					//UnhookWindowsHookEx(hhk);
					//CloseHandle(hEvent);
					
					hr = pTrayDeskBand->ShowDeskBand(CLSID_DeskBandMonitor);
					Sleep(100);
					if (SUCCEEDED(hr))
					{
						for (i = 0; i < 100; ++i)
						{
							if (AutoClickNotifyButton())
							{
								break;
							}
							else
							{
								hr = pTrayDeskBand->IsDeskBandShown(CLSID_DeskBandMonitor);
								if (SUCCEEDED(hr))
								{
									break;
								}
							}
							Sleep(10);
						}
					}
				}
			}
		}
		else
		{
			hr = pTrayDeskBand->IsDeskBandShown(CLSID_DeskBandMonitor);
			if (SUCCEEDED(hr) && (S_OK == hr))
			{
				hr = pTrayDeskBand->HideDeskBand(CLSID_DeskBandMonitor);
			}
		}
		pTrayDeskBand->Release();
	}
	else
	{
		if (TRUE == bShow)
		{
			WCHAR *pBuf = new WCHAR[49];       //很配存储文字串的空间
			StringFromGUID2(CLSID_DeskBandMonitor, pBuf, 39);
			if (!GlobalFindAtom(pBuf))
				GlobalAddAtom(pBuf);
			// Beware! SHLoadInProc is not implemented under Vista and higher.
			hr = SHLoadInProc(CLSID_DeskBandMonitor);
		}
		else
		{
			IBandSite* spBandSite = nullptr;
			hr = CoCreateInstance(CLSID_TrayBandSiteService, NULL, CLSCTX_ALL, IID_PPV_ARGS(&spBandSite));
			if (SUCCEEDED(hr))
			{
				DWORD dwBandID = 0;
				const UINT nBandCount = spBandSite->EnumBands((UINT)-1, &dwBandID);

				for (UINT i = 0; i < nBandCount; ++i)
				{
					spBandSite->EnumBands(i, &dwBandID);

					IPersist* spPersist;
					hr = spBandSite->GetBandObject(dwBandID, IID_PPV_ARGS(&spPersist));
					if (SUCCEEDED(hr))
					{
						CLSID clsid = CLSID_NULL;
						hr = spPersist->GetClassID(&clsid);
						spPersist->Release();
						if (SUCCEEDED(hr) && ::IsEqualCLSID(clsid, CLSID_DeskBandMonitor))
						{
							hr = spBandSite->RemoveBand(dwBandID);
							break;
						}
					}
				}
				spBandSite->Release();
			}
		}
	}
	CoUninitialize();

	Log(Log_Info, "DllShowMonitor[0x%x][%d]", hr, bShow);
	return hr;
}

STDAPI DllIsShow()
{
	ITrayDeskBand *pTrayDeskBand = NULL;
	CoInitialize(NULL);
	HRESULT hr = CoCreateInstance(CLSID_TrayDeskBand, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pTrayDeskBand));
	if (SUCCEEDED(hr))
	{
		hr = pTrayDeskBand->IsDeskBandShown(CLSID_DeskBandMonitor);
	}

	CoUninitialize();
	return SUCCEEDED(hr) ? S_OK : S_FALSE;
}

STDAPI DllEnableLog(BOOL bEnableLog)
{
	LogEnable(bEnableLog);
	return S_OK;
}