#include "PopMenu.h"

CPopMenu::CPopMenu()
{
	m_menu = CreatePopupMenu();
}

CPopMenu::~CPopMenu()
{
	if (m_menu)
	{
		DestroyMenu(m_menu);
		m_menu = NULL;
	}
}

void CPopMenu::append(UINT nId, wchar_t* name, HBITMAP /*bitmap*/)
{
	if (!m_menu)
	{
		return;
	}

	MENUITEMINFO mii = {};
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_CHECKMARKS | MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_SUBMENU | MIIM_TYPE;
	mii.dwTypeData = name;
	mii.cch = (UINT)wcslen(name);
	mii.wID = nId;

	::InsertMenuItem(m_menu, (UINT)-1, TRUE, &mii);
}

BOOL CPopMenu::enable(int index, BOOL bEnable)
{
	return ::EnableMenuItem(m_menu, index, bEnable);
}

void CPopMenu::pop(HWND hwnd, int x, int y)
{
	UINT flags = TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL;
	::TrackPopupMenu(m_menu, flags, x, y, 0, hwnd, NULL);
}