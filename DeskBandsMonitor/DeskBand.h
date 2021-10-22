#pragma once

#include <windows.h>
#include <shlobj.h> // for IDeskband2, IObjectWithSite, IPesistStream, and IInputObject

#include <string>
#include <commctrl.h>
#include <assert.h>
#include <Gdiplus.h>


#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Gdiplus.lib")

using namespace Gdiplus;

class CMainWindow;
class CDeskBand : public IDeskBand2,
                  public IPersistStream,
                  public IObjectWithSite,
                  public IInputObject
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IOleWindow
    STDMETHODIMP GetWindow(HWND *phwnd);
    STDMETHODIMP ContextSensitiveHelp(BOOL);

    // IDockingWindow
    STDMETHODIMP ShowDW(BOOL fShow);
    STDMETHODIMP CloseDW(DWORD);
    STDMETHODIMP ResizeBorderDW(const RECT *, IUnknown *, BOOL);

    // IDeskBand (needed for all deskbands)
    STDMETHODIMP GetBandInfo(DWORD dwBandID, DWORD, DESKBANDINFO *pdbi);

    // IDeskBand2 (needed for glass deskband)
    STDMETHODIMP CanRenderComposited(BOOL *pfCanRenderComposited);
    STDMETHODIMP SetCompositionState(BOOL fCompositionEnabled);
    STDMETHODIMP GetCompositionState(BOOL *pfCompositionEnabled);

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pclsid);

    // IPersistStream
    STDMETHODIMP IsDirty();
    STDMETHODIMP Load(IStream *pStm);
    STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty);
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize);

    // IObjectWithSite
    STDMETHODIMP SetSite(IUnknown *pUnkSite);
    STDMETHODIMP GetSite(REFIID riid, void **ppv);

    // IInputObject
    STDMETHODIMP UIActivateIO(BOOL fActivate, MSG *);
    STDMETHODIMP HasFocusIO();
    STDMETHODIMP TranslateAcceleratorIO(MSG *);

    CDeskBand();
	void setFocus(BOOL);

protected:
    ~CDeskBand();

private:
    LONG                m_cRef;                 // ref count of deskband
    IInputObjectSite   *m_pSite;                // parent site that contains deskband
    BOOL                m_fIsDirty;             // whether deskband setting has changed
    DWORD               m_dwBandID;             // ID of deskband
    HWND                m_hwndParent;           // parent window of deskband

	ULONG_PTR			m_gdiplusToken;
	CMainWindow*		m_pMainWindow;
};

