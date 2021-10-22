#include "DetailWindow.h"
#include "../util/Util.h"
#include <algorithm>
#include <windowsx.h>
#include <Psapi.h>
#include <ShlObj.h>

bool CDetailWindow::m_bDesc = false;
CDetailWindow::CDetailWindow(HWND parent)
	: CBaseWindow(parent)
{
	//util::setTransparent(m_hwnd, 192);
	util::setPrivilege(SE_DEBUG_NAME, true);
	setMouseTracking(TRUE);

	initRes();
	setSize(getTableWidth(), 300);
}

CDetailWindow::~CDetailWindow()
{
	DeleteObject(m_font);
	DeleteObject(m_fontUnderLine);
}

void CDetailWindow::initRes()
{
	m_nYSCroll = 0;
	m_nSortCol = Col_Name;
	m_nHoverRow = -1;
	m_nHoverCol = -1;
	m_nRowHeight = 22;
	m_nColSpace = 10;
	m_nColWidth = { 150, 50, 150, 170 };

	m_font = CreateFont(m_nRowHeight, // nHeight
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

	m_fontUnderLine = CreateFont(m_nRowHeight, // nHeight
		0, // nWidth
		0, // nEscapement
		0, // nOrientation
		FW_NORMAL, // nWeight
		FALSE, // bItalic
		TRUE, // bUnderline
		0, // cStrikeOut
		ANSI_CHARSET, // nCharSet
		OUT_DEFAULT_PRECIS, // nOutPrecision
		CLIP_DEFAULT_PRECIS, // nClipPrecision
		DEFAULT_QUALITY, // nQuality
		DEFAULT_PITCH | FF_SWISS, // nPitchAndFamily
		L"Microsoft YaHei"); // lpszFac
}

int CDetailWindow::getRowByPos(int yPos)
{
	return yPos / m_nRowHeight;
}

int CDetailWindow::getColByPos(int xPos)
{
	int nCol = -1;
	for (auto w : m_nColWidth)
	{
		++nCol;
		xPos -= w;
		if (xPos < 0)
			break;
	}

	return nCol;
}

int CDetailWindow::getTableWidth()
{
	int width = 0;
	for (auto w : m_nColWidth)
	{
		width += w;
	}

	width += m_nColSpace * ((int)m_nColWidth.size() - 1);
	return width;
}

void CDetailWindow::onMouseMove(WPARAM /*wParam*/, LPARAM lParam)
{
	auto xPos = GET_X_LPARAM(lParam);
	auto yPos = GET_Y_LPARAM(lParam);
	auto row = getRowByPos(yPos);
	auto col = getColByPos(xPos);
	bool bUpdate = false;
	
	//wchar_t buff[128] = {};
	//wsprintf(buff, L"(%d, %d)\n", xPos, yPos);
	//OutputDebugString(buff);
	// col
	if (col != m_nHoverCol)
	{
		if (!col && row)
		{
			if (GetCapture() != m_hwnd && yPos < m_height)
				SetCapture(m_hwnd);
			::SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_HAND)));
		}
		else if (!m_nHoverCol)
		{
			::SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));
		}

		m_nHoverCol = col;
		bUpdate = true;
	}

	// row
	if (row != m_nHoverRow)
	{
		if (!row)
		{
			::SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));
		}

		m_nHoverRow = row;
		bUpdate = true;
	}
	
	if (bUpdate)
	{
		update();
	}

	if (yPos > m_height)
	{
		ReleaseCapture();
	}
}

void CDetailWindow::onMouseClick(WPARAM /*wParam*/, LPARAM lParam)
{
	auto xPos = GET_X_LPARAM(lParam);
	auto yPos = GET_Y_LPARAM(lParam);
	auto row = getRowByPos(yPos);
	auto col = getColByPos(xPos);
	// sort
	if (!row)
	{
		m_nSortCol = col;
		if (m_nSortCol == col)
		{
			m_bDesc = !m_bDesc;
		}
		sortCol();
	}

	clicked(row, col);
}

std::wstring CDetailWindow::getFullPath(HANDLE hProcess)
{
	if (!hProcess)
		return L"";
	bool bFind = false;
	BOOL bSuccess = FALSE;
	std::wstring sPath;
	TCHAR szPath[MAX_PATH + 1] = { 0 };
	do
	{
		if (NULL == hProcess)
		{
			break;
		}

		HMODULE hMod = NULL;
		DWORD cbNeeded = 0;
		if (FALSE == EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
		{
			break;
		}
		if (0 == GetModuleFileNameEx(hProcess, hMod, szPath, MAX_PATH))
		{
			break;
		}
		sPath = szPath;
		bSuccess = TRUE;

	} while (bFind);

	if (sPath.empty())
	{
		DWORD len = MAX_PATH;
		QueryFullProcessImageName(hProcess, 0, szPath, &len);
		sPath = szPath;
	}

	return std::move(sPath);
}

void CDetailWindow::clicked(int row, int col)
{
	if (col || !row)
	{
		return;
	}

	int realRow = row + m_nYSCroll - 1;
	auto details = m_net.details;
	if (details.size() <= realRow)
	{
		return;
	}

	auto& selectInfo = details[realRow];
	auto path = util::getPidPath(selectInfo.pid);
	if (path.empty())
	{
		util::reportErr(selectInfo.name.c_str());
		//util::executeCommand(L"F:\\git\\DeskBandsMonitor\\x64\\Debug\\DeskbandInstall.exe", L"-monitor");
		return;
	}

	//¹²ÓÃexplorer.exe
	CoInitialize(NULL);
	auto pidl = ILCreateFromPath(path.c_str());
	if (pidl)
	{
		SHOpenFolderAndSelectItems(pidl, 0, 0, 0);
		ILFree(pidl);
	}
	CoUninitialize();
}

void CDetailWindow::onMouseWheel(WPARAM wParam, LPARAM /*lParam*/)
{
	//auto fwKeys = GET_KEYSTATE_WPARAM(wParam);
	auto zDelta = GET_WHEEL_DELTA_WPARAM(wParam);

	if (zDelta < 0)
	{
		//down
		if (m_nYSCroll < m_net.details.size() - 1)
		{
			m_nYSCroll += 1;
			update();
		}
	}
	else
	{
		//up
		if (m_nYSCroll > 0)
		{
			m_nYSCroll -= 1;
			update();
		}
	}
}

void CDetailWindow::onLeave()
{
	m_nHoverRow = -1;
	m_nHoverCol = -1;
	show(false);
	ReleaseCapture();
}

bool CDetailWindow::sortName(const _stat_net_info_t& l, const _stat_net_info_t& r)
{
	std::wstring str1Tmp = l.name;
	std::wstring str2Tmp = r.name;
	transform(str1Tmp.begin(), str1Tmp.end(), str1Tmp.begin(), towupper);
	transform(str2Tmp.begin(), str2Tmp.end(), str2Tmp.begin(), towupper);
	return m_bDesc ? str1Tmp > str2Tmp : str1Tmp < str2Tmp;
}

bool CDetailWindow::sortPID(const _stat_net_info_t& l, const _stat_net_info_t& r)
{
	return m_bDesc ? l.pid > r.pid : l.pid < r.pid;
}

bool CDetailWindow::sortUp(const _stat_net_info_t& l, const _stat_net_info_t& r)
{
	return m_bDesc ? l.stream.up > r.stream.up : l.stream.up < r.stream.up;
}

bool CDetailWindow::sortDown(const _stat_net_info_t& l, const _stat_net_info_t& r)
{
	return m_bDesc ? l.stream.down > r.stream.down : l.stream.down < r.stream.down;
}

void CDetailWindow::setNet(const _monotor_info_t& net)
{
	m_net = net;
	sortCol();
}

void CDetailWindow::sortCol()
{
	typedef bool(*sortFunc)(const _stat_net_info_t& l, const _stat_net_info_t& r);
	sortFunc sortFuncLists[] = { sortName, sortPID, sortUp, sortDown };
	std::sort(m_net.details.begin(), m_net.details.end(), sortFuncLists[m_nSortCol]);
	update();
}

void CDetailWindow::onShow(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	if (wParam == TRUE)
	{
		int width = GetSystemMetrics(SM_CXSCREEN);
		RECT rc;
		GetWindowRect(m_parentHwnd, &rc);
		int x = (rc.right + rc.left - m_width) / 2;
		if (x + m_width > width)
		{
			x = width - m_width;
		}
		int y = rc.top - m_height;
		::MoveWindow(m_hwnd, x, y, m_width, m_height, FALSE);
	}
}

void CDetailWindow::onPaint()
{
	PAINTSTRUCT ps;
	auto hdc = BeginPaint(m_hwnd, &ps);
	if (!hdc)
	{
		return;
	}

	auto mem = CreateCompatibleDC(hdc);
	auto width = GetDeviceCaps(mem, HORZRES);
	auto height = GetDeviceCaps(mem, VERTRES);
	auto bitmap = CreateCompatibleBitmap(hdc, width, height);
	SelectObject(mem, bitmap);

	SetBkMode(mem, TRANSPARENT);
	SelectObject(mem, m_font);

	auto backBrush = CreateSolidBrush(RGB(43, 43, 43));

	auto titleBack = RGB(31, 49, 64);
	auto titlBackBrush = CreateSolidBrush(titleBack);
	auto titleColor = RGB(255, 255, 255);

	auto selectBack = RGB(206, 231, 255);
	auto selectBackBrush = CreateSolidBrush(selectBack);
	auto selectColTextColor = RGB(0, 128, 255);

	auto red = RGB(255, 0, 0);
	auto yellow = RGB(255, 255, 45);
	auto green = RGB(45, 255, 45);
	auto white = RGB(255, 255, 255);
	RECT rc;
	GetClientRect(m_hwnd, &rc);
	
	//back
	FillRect(mem, &rc, backBrush);
	DeleteObject(backBrush);

	std::wstring title[] = { L"Process", L"PID"
		, std::wstring(L"Up(") + CSysTaskMgr::toSpeedStringW(m_net.total.up) + L")"
		, std::wstring(L"Down(") + CSysTaskMgr::toSpeedStringW(m_net.total.down) + L")" };
	int row = 0;
	int col = 0;
	//title
	rc.left = 0;
	rc.top = row * m_nRowHeight;
	rc.bottom = rc.top + m_nRowHeight;

	FillRect(mem, &rc, titlBackBrush);
	DeleteObject(titlBackBrush);
	SetTextColor(mem, titleColor);
	for (int n = 0; n < sizeof(title) / sizeof(title[0]); ++n)
	{
		rc.right = rc.left + m_nColWidth[n];
		auto text = title[n];
		if (n == m_nSortCol)
		{
			text = (m_bDesc ? std::wstring(L"¡ý ") : std::wstring(L"¡ü ")) + text;
		}

		DrawText(mem, text.c_str(), (int)text.length(), &rc, DT_TOP | DT_LEFT);
		rc.left = rc.right + m_nColSpace;
	}

	++row;
	int nBeginRow = 0;

	for (auto it : m_net.details)
	{
		if (nBeginRow < m_nYSCroll)
		{
			++nBeginRow;
			continue;
		}

		rc.left = 0;
		rc.right = width;
		rc.top = row * m_nRowHeight;
		rc.bottom = rc.top + m_nRowHeight;

		if (row == m_nHoverRow)
		{
			FillRect(mem, &rc, selectBackBrush);
		}

		if (it.stream.down <= 0 && it.stream.up <= 0)
			SetTextColor(mem, white);
		else if (it.stream.down <= 0)
			SetTextColor(mem, red);
		else if (it.stream.up <= 0)
			SetTextColor(mem, yellow);
		else
			SetTextColor(mem, green);

		col = 0;
		if (row == m_nHoverRow)
		{
			if (col == m_nHoverCol)
			{
				SelectObject(mem, m_fontUnderLine);
				SetTextColor(mem, selectColTextColor);
			}
			else
			{
				SetTextColor(mem, titleBack);
			}
		}

		rc.right = rc.left + m_nColWidth[col++];
		DrawText(mem, it.name.c_str(), (int)it.name.length(), &rc, DT_TOP | DT_LEFT);

		SelectObject(mem, m_font);

		if (it.stream.down <= 0 && it.stream.up <= 0)
			SetTextColor(mem, white);
		else if (it.stream.down <= 0)
			SetTextColor(mem, red);
		else if (it.stream.up <= 0)
			SetTextColor(mem, yellow);
		else
			SetTextColor(mem, green);

		if (row == m_nHoverRow)
		{
			SetTextColor(mem, titleBack);
		}

		rc.left = rc.right + m_nColSpace;
		rc.right = rc.left + m_nColWidth[col++];
		std::wstring sPid = std::to_wstring(it.pid);
		DrawText(mem, sPid.c_str(), (int)sPid.length(), &rc, DT_TOP | DT_LEFT);

		rc.left = rc.right + m_nColSpace;
		rc.right = rc.left + m_nColWidth[col++];
		std::wstring sUp = CSysTaskMgr::toSpeedStringW(it.stream.up);
		DrawText(mem, sUp.c_str(), (int)sUp.length(), &rc, DT_TOP | DT_LEFT);

		rc.left = rc.right + m_nColSpace;
		rc.right = rc.left + m_nColWidth[col++];
		std::wstring sDown = CSysTaskMgr::toSpeedStringW(it.stream.down);
		DrawText(mem, sDown.c_str(), (int)sDown.length(), &rc, DT_TOP | DT_LEFT);

		++row;
	}

	//scroll
	paintScroll(mem);

	::BitBlt(hdc, 0, 0, width, height, mem, 0, 0, SRCCOPY);
	EndPaint(m_hwnd, &ps);
	DeleteDC(mem);
	DeleteObject(bitmap);
	DeleteObject(selectBackBrush);
}

void CDetailWindow::paintScroll(HDC__* hdc)
{
	auto yPos = (int)((m_nYSCroll * 1.0 / m_net.details.size()) * (m_height - m_nRowHeight)) + m_nRowHeight;
	if (yPos < m_nRowHeight)
		yPos = m_nRowHeight;
	auto backBrush = CreateSolidBrush(RGB(104, 104, 104));
	
	RECT rc;
	rc.left = m_width - 7;
	rc.right = rc.left + 5;
	rc.top = yPos;
	rc.bottom = rc.top + 40;

	FillRect(hdc, &rc, backBrush);
	DeleteObject(backBrush);
}