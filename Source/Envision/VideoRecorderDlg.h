/*
This file is part of Envision.

Envision is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Envision is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Envision.  If not, see <http://www.gnu.org/licenses/>

Copywrite 2012 - Oregon State University

*/
#pragma once
#include "afxwin.h"


// VideoRecorderDlg dialog

class VideoRecorderDlg : public CDialogEx
{
	DECLARE_DYNAMIC(VideoRecorderDlg)

public:
	VideoRecorderDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~VideoRecorderDlg();

// Dialog Data
	enum { IDD = IDD_VIDEORECORDER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   void LoadVR( void );
   void EnableControls();

   bool m_dirty;

   //int AddWindow( LPCTSTR name, CWnd *pWnd );
	DECLARE_MESSAGE_MAP()
public:
   CCheckListBox m_recorders;

   int m_frameRate;
   int m_quality;
   CString m_path;
   int m_timerInterval;
   BOOL m_useTimer;
   BOOL m_useMessage;
   BOOL m_useApplication;

   virtual BOOL OnInitDialog();
   afx_msg void OnLbnSelchangeWindows();
   afx_msg void OnEnChangeFramerate();
   afx_msg void OnEnChangeQuality();
   afx_msg void OnBnClickedCaptureinterval();
   afx_msg void OnEnChangeInterval();
   afx_msg void OnBnClickedMessage();
   afx_msg void OnBnClickedApplication();
   afx_msg void OnEnChangePath();
   afx_msg void OnCheckRecorders();
   };
