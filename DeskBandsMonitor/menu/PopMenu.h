#pragma once
#include <Windows.h>

class CPopMenu
{
public:
	CPopMenu();
	~CPopMenu();

	void append(UINT nId, wchar_t* name, HBITMAP bitmap = NULL);
	BOOL enable(int, BOOL);
	void pop(HWND hwnd, int x, int y);

private:
	HMENU m_menu;
};

