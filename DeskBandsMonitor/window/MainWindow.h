#pragma once

#include <Windows.h>
#include <string>

#include <Gdiplus.h>
using namespace Gdiplus;

#include "../resource.h"

class CDeskBand;
class CSysTaskMgr;
class CPopMenu;
class CMainWindow
{
public:
	CMainWindow(CDeskBand* parent);
	~CMainWindow();

	BOOL regster(HINSTANCE hInstance);
	void unRegster();

	BOOL create(HINSTANCE hInstance, HWND parent);

	HWND getWindow();
	void show(BOOL);
	void close();
	void setCompositionState(BOOL bCompositionEnabled);
	BOOL getCompositionState();
	void setFocus();
	BOOL getFocus();

private:
	static LRESULT CALLBACK windowCallBack(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void onCreate();
	void onFocus(BOOL fFocus);
	void onHover(BOOL bHover);
	void onPaint(const HDC hdcIn);
	void onLeftClick(WPARAM);
	void onLeftDubleClick();
	void onRightClick(LPARAM);
	void onTimer(UINT);
	void onDestroy();
	void onCommand(WPARAM, LPARAM);
	void onMenu(WPARAM, LPARAM);
	void onNotify(WPARAM, LPARAM);

	void onMenuClose();
	void onMenuUnreg();
private:
	Image* gdiGetFileFromResource(UINT pResourceID, LPCTSTR pResourceType);

private:
	CDeskBand*			m_pDeskBand;
	HINSTANCE			m_hInstance;
	HWND				m_hwnd;
	std::wstring		m_className;

	BOOL				m_bCompositionEnabled;
	BOOL                m_bFocus;
	BOOL				m_bHover;
	BOOL				m_bPressTrigger;

	Image* m_up;
	Image* m_down;
	CPopMenu* m_pMenu;
	
	CSysTaskMgr* m_task;
};

