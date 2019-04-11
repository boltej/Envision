
// EnvServerDlg.h : header file
//

#pragma once

#include <thread>

#include <Logbook/Logbook.h>
#include <PtrArray.h>
#include <EnvModel.h>
#include "RestServer.h"

void Notify(EM_NOTIFYTYPE, INT_PTR, INT_PTR, INT_PTR);



// CEnvServerDlg dialog
class CEnvServerDlg : public CDialogEx
{
// Construction
public:
	CEnvServerDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ENVSERVER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

public:
  void AddInstance(EnvisionInstance*);
   //int LoadProject(EnvisionInstance*, LPCTSTR);


   int AddSession(int sessionID, LPCTSTR clientURL);
   void RemoveSession(int sessionID);

   void OnEnvInstanceFinished(EnvisionInstance *pInst);
   void Log(LPCTSTR msg);
// Implementation
protected:
 
   // Dialog stuff
	HICON m_hIcon;

   //CLogbook m_logbook;
   //HWND m_hLogbook;

   int m_timer;

 
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()


public:
   afx_msg void OnBnClickedStop();
   afx_msg void OnBnClickedStart();
   void OnTimer(UINT_PTR eventID);

   CString m_scenarioText;
   CString m_projectFile;
   CEdit m_startTime;
   CEdit m_runTime;
   CComboBox m_scenario;
   afx_msg void OnClose();
   CListBox m_instances;
   CListBox m_sessions;
   CListBox m_logList;
   };


inline
void CEnvServerDlg::Log(LPCTSTR msg)
   {
   //CString _msg("\r\n");
   //_msg += msg;
   //
   //int end = m_logWnd.GetWindowTextLength();
   //m_logWnd.SetSel(end, end);
   //m_logWnd.ReplaceSel( _msg );
   m_logList.AddString(msg);
   }