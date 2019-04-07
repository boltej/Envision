
// EnvServerDlg.h : header file
//

#pragma once

#include <thread>

#include <Logbook/Logbook.h>
#include <PtrArray.h>
#include <EnvModel.h>
#include "RestServer.h"

void Notify(EM_NOTIFYTYPE, INT_PTR, INT_PTR, INT_PTR);

enum STATUS { ST_INITIALIZED, ST_PROJECTLOADED, ST_RUNNING, ST_PAUSED, ST_STOPPED };

//--------------------------------------------------------------------
// An EnvisionInstance represents one instance of an Envision Model.
// It supports functionality for a) loading, and b) running an Envision
// model, managing the interface between the REST services and the
// Envision API.
// When loading and running an Envision model, each function is run 
// on its own thread.
//--------------------------------------------------------------------

class EnvisionInstance
   {
   friend class CEnvServerDlg;

   public:
      CString m_projectFile;  // envx associated with this instance
      EnvModel *m_pEnvModel;  // EnvEngine instance ptr (NULL until model loaded)
      int m_scenario;         // zero-based (-1=run all)
      int m_simulationLength;

      time_t m_start;   // start time;
      shared_ptr<Session> m_pEventSession;  // EventSource session

   protected:
      int m_id;   // unique identifier (incrementing m_count)
      std::thread *m_pThread;  //  currently execting thread
      STATUS m_status; 
      time_t m_stop;
      double m_duration;   // seconds

      static int m_instanceCount;

   // API Entry points
   public:
      int LoadProject(LPCTSTR envxFile );  // Runs ::EnvLoadProject
      int RunScenario(int scenario, int simulationLength);      // Runs ::EnvRunScenario()

   // Thread functions.
   protected:
      int _RunScenarioThreadFn();
      int _LoadProjectThreadFn();

   public:
      EnvisionInstance();
      ~EnvisionInstance();

      int GetID() { return m_id;  }
      
      void Join();
   };


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
   EnvisionInstance *AddInstance(); // , int scenario);// , const shared_ptr<Session>&);  // note scenario is zero-based, -1 to run all
   //int LoadProject(EnvisionInstance*, LPCTSTR);


   int AddSession(LPCTSTR sessionID);
   void RemoveSession(LPCTSTR sessionID);

   EnvModel *GetEnvModelFromSession(const shared_ptr<Session> &);
   const shared_ptr<Session> & GetSessionFromEnvModel(EnvModel*);

   EnvisionInstance *GetEnvisionInstanceFromModelID(int modelID);

   void OnEnvInstanceFinished(EnvisionInstance *pInst);
   void Log(LPCTSTR msg);
// Implementation
protected:
   PtrArray<EnvisionInstance> m_instArray;

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