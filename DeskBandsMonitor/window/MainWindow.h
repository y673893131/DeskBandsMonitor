#pragma once

#include <Windows.h>
#include <string>

#include <uxtheme.h>

#include <Gdiplus.h>
using namespace Gdiplus;

#include "../resource.h"

class CSysTaskMgr;
class CPopMenu;
class CTooltipDetail;
class CDetailWindow;
class CMainWindow
{
public:
	CMainWindow();
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
	void onGdiPlusPaint(const HDC hdcIn);
	void onThemePaint(const HDC hdcIn, HTHEME hTheme);
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
	Bitmap* gdiGetFileFromResource(UINT pResourceID, LPCTSTR pResourceType);

private:
	HINSTANCE			m_hInstance;
	HWND				m_hwnd;
	std::wstring		m_className;

	BOOL				m_bCompositionEnabled;
	BOOL                m_bFocus;
	BOOL				m_bHover;
	BOOL				m_bPressTrigger;

	Bitmap* m_up;
	Bitmap* m_down;

	HIMAGELIST m_imgList;

	CPopMenu* m_pMenu;

	CTooltipDetail* m_pTooltip;

	CDetailWindow* m_pDetailWindow;

	CSysTaskMgr* m_task;
};

