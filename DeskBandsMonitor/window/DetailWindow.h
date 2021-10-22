#pragma once

#include "Base/BaseWindow.h"
#include "../monitor/SysTaskMgr.h"
#include <vector>

class CDetailWindow : public CBaseWindow
{
public:
	CDetailWindow(HWND parent = nullptr);
	virtual ~CDetailWindow();

	void setNet(const _monotor_info_t& net);

private:
	enum Table_Col
	{
		Col_Name,
		Col_Pid,
		Col_Up,
		Col_Down
	};

private:
	void onPaint() override;
	void paintScroll(HDC__* hdc);

	void onMouseMove(WPARAM wParam, LPARAM lParam) override;
	void onMouseClick(WPARAM wParam, LPARAM lParam) override;
	void clicked(int row, int col);

	void onMouseWheel(WPARAM wParam, LPARAM lParam) override;
	void onLeave() override;
	void onShow(WPARAM wParam, LPARAM lParam) override;

private:
	void initRes();

	static bool sortName(const _stat_net_info_t& l, const _stat_net_info_t& r);
	static bool sortPID(const _stat_net_info_t& l, const _stat_net_info_t& r);
	static bool sortUp(const _stat_net_info_t& l, const _stat_net_info_t& r);
	static bool sortDown(const _stat_net_info_t& l, const _stat_net_info_t& r);
	
	void sortCol();
	int getRowByPos(int yPos);
	int getColByPos(int xPos);
	int getTableWidth();
private:
	_monotor_info_t m_net;

	HFONT m_font, m_fontUnderLine;
	int m_nRowHeight;
	int m_nColSpace;
	int m_nHoverRow;
	int m_nHoverCol;
	int m_nSortCol;
	int m_nYSCroll;
	std::vector<int> m_nColWidth;
	static bool m_bDesc;
};

