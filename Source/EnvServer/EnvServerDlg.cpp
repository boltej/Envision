
// EnvServerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EnvServer.h"
#include "EnvServerDlg.h"
#include "afxdialogex.h"

#include <EnvAPI.h>
#include <iostream>

#include <COLORS.HPP>
#include <Path.h>

#include "RestServer.h"

#include <chrono>
#include <ctime> 

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define _CRT_SECURE_NO_WARNINGS

extern CEnvServerDlg *gpMainWnd;
extern RestServer restServer;


void SvrLogMsgProc(LPCTSTR msg, LPCTSTR hdr, int type);
void SvrStatusMsgProc(LPCTSTR msg);
int SvrPopupMsgProc(LPCTSTR hdr, LPCTSTR msg, int flags);
void NotifyProc(EM_NOTIFYTYPE type, INT_PTR, INT_PTR, INT_PTR);


// called from EnvModel while running project
void NotifyProc(EM_NOTIFYTYPE type, INT_PTR param, INT_PTR extra, INT_PTR modelID)
   {
   restServer.SendServerEvent(type, param, extra, modelID);
   }


// called whenever a Report message is invoked (e.g. Report::Log();
void SvrLogMsgProc(LPCTSTR msg, REPORT_ACTION, REPORT_TYPE type)
   {
   COLORREF color;

   switch (type)
      {
      case RT_WARNING:
         color = BLUE;
         break;

      case RT_ERROR:
      case RT_FATAL:
      case RT_SYSTEM:
         color = RED;
         break;

      default:
         color = BLACK;
         break;
      }

   gpMainWnd->Log(msg);
   restServer.SendServerEvent((EM_NOTIFYTYPE)-1, (INT_PTR)msg, type, NULL);
   TRACE(msg);

   }

void SvrStatusMsgProc(LPCTSTR msg)
   {
   std::string _msg = "Status: ";
   _msg += msg;

   gpMainWnd->Log(_msg.c_str());
   }

// called whenever a Report message is invoked (e.g. Report::Log();
int SvrPopupMsgProc(LPCTSTR msg, LPCTSTR hdr, REPORT_TYPE type, int flags, int extra)
   {
   std::string _msg = "Message: ";
   _msg += msg;

   gpMainWnd->Log(_msg.c_str());
   return 0;
   }


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CEnvServerDlg dialog



CEnvServerDlg::CEnvServerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_ENVSERVER_DIALOG, pParent)
   , m_scenarioText(_T(""))
   , m_projectFile(_T(""))
   {
   gpMainWnd = this;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
   ::EnvInitEngine(0);
}

void CEnvServerDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialogEx::DoDataExchange(pDX);
   DDX_Text(pDX, IDC_SCENARIO, m_scenarioText);
   DDX_Text(pDX, IDC_PROJECTFILE, m_projectFile);
   DDX_Control(pDX, IDC_STARTED, m_startTime);
   DDX_Control(pDX, IDC_RUNTIME, m_runTime);
   DDX_Control(pDX, IDC_SCENARIO, m_scenario);
   DDX_Control(pDX, IDC_INSTANCES, m_instances);
   DDX_Control(pDX, IDC_SESSIONS, m_sessions);
   DDX_Control(pDX, IDC_LOG, m_logList);
   }

BEGIN_MESSAGE_MAP(CEnvServerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
   ON_BN_CLICKED(IDC_STOP, &CEnvServerDlg::OnBnClickedStop)
   ON_BN_CLICKED(IDC_START, &CEnvServerDlg::OnBnClickedStart)
   ON_WM_CLOSE()
END_MESSAGE_MAP()


// CEnvServerDlg message handlers

BOOL CEnvServerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// Add scenario entry
   m_scenario.AddString("<Run All Scenarios>");
   m_scenario.SetCurSel(0);

   SYSTEMTIME tm;
   ::GetSystemTime(&tm);
   CString msg;

   msg.Format(_T("EnvServer STARTED at %i-%i-%i %i:%i:%i"), (int)tm.wMonth, (int)tm.wDay, (int)tm.wYear, (int)tm.wHour, (int)tm.wMinute, (int)tm.wSecond);
   Log(msg);

   Report::logMsgProc    = SvrLogMsgProc;
   Report::statusMsgProc = SvrStatusMsgProc;
   Report::popupMsgProc  = SvrPopupMsgProc;
   
   m_timer = SetTimer(1, 5000, NULL); // one event every 1000 ms = 1 s
   
   return TRUE;  // return TRUE  unless you set the focus to a control
}

void CEnvServerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CEnvServerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

void CEnvServerDlg::OnTimer( UINT_PTR eventID )
   {
   /////void EventHandler(void)
   /////   {
   /////   static size_t counter = 0;
   /////   const auto message = "data: event " + to_string(counter) + "\n\n";
   /////     
   /////   // check for any closed sessions, and remove them
   /////   for ( auto it=restServer.m_sessionsMap.cbegin(); it != restServer.m_sessionsMap.cend();)
   /////      {
   /////      if (it->second->is_closed())
   /////         {
   /////         string sessionID = it->first;
   /////         gpMainWnd->RemoveSession(sessionID.c_str());
   /////
   /////         restServer.m_sessionsMap.erase(it++);
   /////         }
   /////      else
   /////         ++it;
   /////      }
   /////
   /////
   /////   // send the message to any open sessions
   /////   for (auto it = restServer.m_sessionsMap.cbegin(); it != restServer.m_sessionsMap.cend();)
   /////      {
   /////      string msg = format("data: Event %i for session %s", counter, it->first.c_str());
   /////      it->second->yield(msg);
   /////
   /////      it++;
   /////      }
   /////
   /////   counter++;
   /////   }



   }


// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CEnvServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CEnvServerDlg::AddInstance(EnvisionInstance *pInst) // LPCTSTR projectFile) // , int scenario) // , const shared_ptr<Session> &pSession)
   {
   TCHAR buf[64];
   ctime_s(buf, 64, &pInst->m_start);

   CString label;
   label.Format("Envsion Instance %i STARTED %s", pInst->GetID(), buf);
   m_instances.AddString(label);
   }

////int CEnvServerDlg::LoadProject(EnvisionInstance *pInst, LPCTSTR envxFile )
////   {
////   if (pInst == NULL)
////      return -1;
////
////   return pInst->LoadProject(envxFile);
////   }




////////////////////////////////////////////////
// MESSAGE HANDLERS
////////////////////////////////////////////////


void CEnvServerDlg::OnBnClickedStop()
   {
   // TODO: Add your control notification handler code here
   }


void event_stream_handler(void);
void CEnvServerDlg::OnBnClickedStart()
   {
   this->UpdateData(1);
   
   if (this->m_projectFile.GetLength() == 0)
      {
      AfxMessageBox("You need to specify a project (.envx) file");
      return;
      }
   
   int scnIndex = this->m_scenario.GetCurSel();
   if (scnIndex < 0 || this->m_scenarioText.GetLength() == 0)
      {
      AfxMessageBox("You need to specify a scenario");
      return;
      }
   
   //AddInstance();   //_projectFile, 0); // , NULL);// m_scenario.GetCurSel() - 1);
   }


void CEnvServerDlg::OnClose()
   {
   CDialogEx::OnClose();
   }


void CEnvServerDlg::OnEnvInstanceFinished(EnvisionInstance *pInst)
   {
   // update instance box

   CString text;
   for (int i = 0; i < m_instances.GetCount(); i++)
      {
      m_instances.GetText(i, text);

      int id = atoi(text);

      if (id == pInst->GetID())
         {
         nsPath::CPath path(pInst->m_projectFile);

         TCHAR buf[64];
         ctime_s(buf, 64, &pInst->m_stop);

         long duration = (long) pInst->m_duration;
         int hours = duration / 3600;
         int mins = duration/60;
         if ( hours > 0 )
            mins = int(duration % hours);

         int secs = duration;
         if ( mins > 0 )
            secs = int(duration % mins);
         
         CString label;
         label.Format("%i:%s:%i FINISHED at %s (%ih:%ims:%is)", pInst->GetID(), (LPCTSTR)path.GetName(), pInst->m_scenario, buf, hours, mins, secs);

         m_instances.DeleteString(i);
         m_instances.InsertString(i, label);
         break;
         }
      }      
   }


int CEnvServerDlg::AddSession( int sessionID, LPCTSTR clientURL)
   {
   CString msg;
   msg.Format("Session %i (%s)", sessionID, clientURL);
   return m_sessions.AddString(msg);
   }

void CEnvServerDlg::RemoveSession(int sessionID)
   {
   int count = m_sessions.GetCount();

   CString value;
   for (int i = 0; i < count; i++)
      {
      m_sessions.GetText(i, value);

      TCHAR *p = value.GetBuffer();
      
      while (!isdigit(*p)) p++;

      int _sessionID = atoi(p);
      if (sessionID == _sessionID)
         {
         m_sessions.DeleteString(i);
         break;
         }
      }
   }

/////////////////////////////////////////////

int EnvisionInstance::m_instanceCount = 0;

EnvisionInstance::EnvisionInstance()
   : m_status(ST_INITIALIZED)
   , m_pThread(NULL)
   , m_pEnvModel(0)
   , m_scenario(-1)
   //, m_pSession(NULL)
   , m_start()
   , m_pEventSession( nullptr )
   , m_stop()
   , m_duration(0)
   {
   m_id = m_instanceCount++;

   auto start = std::chrono::system_clock::now();
   m_start = std::chrono::system_clock::to_time_t(start);
   }

EnvisionInstance::~EnvisionInstance()
   {
   if (m_pEnvModel != NULL)
      delete m_pEnvModel;

   if (m_pEventSession != NULL && m_pEventSession->is_open())
      m_pEventSession->close();

   //if (m_pThread)
   //   delete m_pThread;
   }

// This launches an Envision instance, creating
// a new thread and loading the project file into that thread.
// it will run the specified scenario on the new thread
int EnvisionInstance::LoadProject(LPCTSTR projectFile) //, int scenario)
   {
   m_projectFile = projectFile;
   //m_scenario = scenario;

   switch(m_status)
      {
      case ST_INITIALIZED:
         // run LoadProjectFn in a new thread.
         _LoadProjectThreadFn();
         //m_pThread = new std::thread(&EnvisionInstance::_LoadProjectThreadFn, std::ref(*this)); //  runs foo::bar() on object f
         m_status = ST_PROJECTLOADED;
         return m_status;

      default:
         return -(int(m_status));
      }

   //m_pThread = NULL;
   return 1;
   }

// Thread Function 
// assumes a project has been loaded, and an Envision instance is not yet created.
// creates an EnvModel instance and loads the current project file into
// the model instance
int EnvisionInstance::_LoadProjectThreadFn() //
   {
   if (m_pEnvModel != NULL)
      return -2;

   // Create a new model instance
   m_pEnvModel = ::EnvLoadProject(this->m_projectFile, 0);
   if (m_pEnvModel == NULL)
      {
      CString msg;
      msg.Format("Error: Envision Project %s did not load.  No simulation will be done.", (LPCTSTR)m_projectFile);
      Report::ErrorMsg(msg);
      return -4;
      }

   m_status = ST_PROJECTLOADED;
   m_pEnvModel->SetNotifyProc(NotifyProc, 0);

   return 1;
   }

// Requires that an EnvModel has been created with LoadProject() prior to this call.
// a new thread and loading the project file into that thread.
// it will run the specified scenario on the new thread
int EnvisionInstance::RunScenario(int scenario, int simulationLength ) //, int scenario)
   {
   m_scenario = scenario;

   switch (m_status)
      {
      case ST_PROJECTLOADED:
         m_status = ST_RUNNING;
         m_simulationLength = simulationLength;
         
         //m_pThread = NULL;
         //_RunScenarioThreadFn();         
         m_pThread = new std::thread(&EnvisionInstance::_RunScenarioThreadFn, std::ref(*this)); //  runs foo::bar() on object f
         
         return m_status;

      default:
         return -(int(m_status));
      }

   return 1;
   }

// Thread Function
int EnvisionInstance::_RunScenarioThreadFn()
   {
   ASSERT(m_pEnvModel != NULL);

   // run the scenario.
   int retVal = EnvRunScenario(m_pEnvModel, m_scenario, m_simulationLength, 0);  // m_scenario is zero-based
   m_status = ST_PROJECTLOADED;
   return retVal;
   }




 ////  // get date here, since m_pEnvModel is deleted next
 ////  // coming soon!
 ////
 ////
 ////  // finish up (this delete m_pEnvModel and the associated Map)
 ////  retVal = EnvCloseProject(m_pEnvModel, 0);
 ////
 ////  m_pEnvModel = NULL;
 ////
 ////  auto end = std::chrono::system_clock::now();
 ////  m_stop = std::chrono::system_clock::to_time_t(end);
 ////  m_duration = difftime(m_stop, m_start);   
 ////
 ////  gpMainWnd->OnEnvInstanceFinished(this);
 ////  return retVal;
 ////  }


void EnvisionInstance::Join()
   {
   if (m_pThread == NULL)
      return;

   if ( m_pThread->joinable() )
      m_pThread->join();
   }

