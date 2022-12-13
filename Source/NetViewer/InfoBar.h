#pragma once
#include <afxwin.h>

// InfoBar

const int infoBarHeight = 20;


class InfoBar : public CWnd
{
	DECLARE_DYNAMIC(InfoBar)

public:
	InfoBar();
	virtual ~InfoBar();

protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
};


