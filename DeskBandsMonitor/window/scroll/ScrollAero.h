#pragma once

#include "../base/BaseWindow.h"

class CScrollAero
{
public:
	CScrollAero(CBaseWindow* parent = nullptr);
	virtual ~CScrollAero();

	void setAreaRange(int min, int max);

private:
	CBaseWindow* m_pAreaWindow;
};

