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
// This MFC Samples source code demonstrates using MFC Microsoft Office Fluent User Interface 
// (the "Fluent UI") and is provided only as referential material to supplement the 
// Microsoft Foundation Classes Reference and related electronic documentation 
// included with the MFC C++ library software.  
// License terms to copy, use or distribute the Fluent UI are available separately.  
// To learn more about our Fluent UI licensing program, please visit 
// http://msdn.microsoft.com/officeui.
//
// Copyright (C) Microsoft Corporation
// All rights reserved.

// MainFrm.h : interface of the CMainFrame class
//

#pragma once


//#include "OutputWnd.h"
#include <Logbook\Logbook.h>
#include <afxDockablePane.h>
class EnvModel;
class Scenario;

class CLogPane : public CDockablePane
{
public:
   CLogPane() : m_hLogbook( 0 ) { }

   CLogbook m_logbook;
   HWND m_hLogbook;
  
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()
};


class CEnvDoc;

const int MAX_APS = 199;
const int MAX_EMS = 199;
const int MAX_SCENARIOS = 99;
const int MAX_CONSTRAINTS = 99;
const int MAX_EXT_ANALYSISMODULES = 99;
const int MAX_ZOOMS = 99;

class CMainFrame : public CFrameWndEx
{
friend class CEnvDoc;

enum 
   {
   ID_AUTONOMOUS_PROCESS  = 15000,      // command id's for ribbon menu additions
   ID_EVAL_MODEL          = 15200,
   ID_SCENARIOS           = 15400,
   ID_CONSTRAINTS         = 15500,
   ID_POSTRUN_CONSTRAINTS = 15600,
   ID_EXT_ANALYSISMODULE  = 15700,
   ID_ZOOMS               = 15800
   };

protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif


protected:  // control bar embedded members
	CMFCRibbonBar        m_wndRibbonBar;
	CMFCRibbonApplicationButton m_MainButton;
	CMFCToolBarImages    m_PanelImages;
   CMFCRibbonStatusBar  m_wndStatusBar;

 	//COutputWnd        m_wndOutput;
   CLogPane            m_wndOutput;

   // Ribbon Bar elements
   CMFCRibbonButton   *m_pModulesButton;
   CMFCRibbonButton   *m_pModelsButton;
   CMFCRibbonButton   *m_pScenariosButton;
   CMFCRibbonComboBox *m_pScenariosCombo;
   CMFCRibbonComboBox *m_pConstraintsCombo;
   CMFCRibbonComboBox *m_pPostRunConstraintsCombo;
   CMFCRibbonComboBox *m_pZoomsCombo;

public:
   CMFCRibbonEdit     *m_pStartYear;
   CMFCRibbonEdit     *m_pEndYear;
   CMFCRibbonCheckBox *m_pExportCoverages; 
   CMFCRibbonEdit     *m_pExportInterval; 
   CMFCRibbonCheckBox *m_pExportOutputs; 
   CMFCRibbonCheckBox *m_pExportDeltas; 
   CMFCRibbonEdit     *m_pExportDeltaFields; 
   CMFCRibbonButton   *m_pVideoRecorders;
   
protected:
   // status bar elements
   CMFCRibbonStatusBarPane *m_pStatusMsg;
   CMFCRibbonStatusBarPane *m_pStatusMode;
   CMFCRibbonButton        *m_pZoomButton;
   CMFCRibbonProgressBar   *m_pProgressBar;
   CMFCRibbonProgressBar   *m_pMultirunProgressBar;
   CMFCRibbonSlider        *m_pZoomSlider;

   //CSplitterWnd m_splitterWnd;

public:
   void SetYearInfo( int startYear, int lengthOfRun );
   int  GetStartYear( void );
   int  GetRunLength( void );
   int  GetExportMapInterval( void );
   bool GetExportOutputs( void ) { return m_pExportOutputs->IsChecked() ? true : false; }
   //bool GetExportMaps(void) { return m_pExportMaps->IsChecked() ? true : false; }
   void SetExportDeltaFieldList( LPCTSTR fieldList );
   int  GetExportDeltaFieldList( CString &fieldList );

   void SetCaption( Scenario *pScenario );
   void SetStatusMsg ( LPCTSTR msg ) { if ( m_pStatusMsg  ) { m_pStatusMsg->SetText( msg ); m_pStatusMsg->SetAlmostLargeText( msg ); m_wndStatusBar.RecalcLayout(); m_wndStatusBar.RedrawWindow(); /* m_pStatusMsg->Redraw(); */  } }
   void SetStatusMode( LPCTSTR msg ) { if ( m_pStatusMode ) { m_pStatusMode->SetText( msg ); m_pStatusMode->Redraw(); } }
   void SetStatusZoom( int percent );  // 0-100
   int  GetZoomSliderPos() { if ( m_pZoomSlider ) return m_pZoomSlider->GetPos(); else return 0; }
   void SetModelMsg( LPCTSTR msg ) { SetStatusMode( msg ); }
   void SetMultiModelMsg( LPCTSTR msg ){ }

   void SetRunProgressPos( int count )                { m_pProgressBar->SetPos( count ); }
   void SetRunProgressRange( int start, int end )     { m_pProgressBar->SetRange( start, end ); }
   void SetMultiRunProgressPos( int count )           { m_pMultirunProgressBar->SetPos( count ); }
   void SetMultiRunProgressRange( int start, int end ){ m_pMultirunProgressBar->SetRange( start, end ); }

   void SetExportInterval( int interval ) { TCHAR buffer[ 32 ]; _itoa_s( interval, buffer, 32, 10 ); m_pExportInterval->SetEditText( buffer );  }
   
   void ShowOutputPanel( bool show=true ); // { ShowPane( &m_wndOutput, show ? 1 : 0, FALSE, TRUE ); RecalcLayout (); gpDoc->m_outputWndStatus = show==true? 1 ; 0;  } // m_wndOutput.ShowWindow( show ? 1 : 0 ); }
   bool IsOutputOpen( void ) { return m_wndOutput.IsPaneVisible() ? true : false; }
   void AddLogLine( LPCTSTR msg, LOGBOOK_TEXTTYPES type, COLORREF color ) { CString _msg( msg ); _msg.Replace( "%", "%%" ), m_wndOutput.m_logbook.AddLogLine( _msg, type, color ); m_wndOutput.RedrawWindow(); }
   void AddMemoryLine() { m_wndOutput.m_logbook.AddMemoryLine(); m_wndOutput.RedrawWindow(); }
   void AddSeparatorLine() { m_wndOutput.m_logbook.AddSeparatorLine(); m_wndOutput.RedrawWindow(); }

   int  GetCurrentlySelectedScenario( void );
   int  GetCurrentlySelectedZoom( void );

// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnApplicationLook(UINT id);
	afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);
	afx_msg void OnFilePrint();
	afx_msg void OnFilePrintPreview();
	afx_msg void OnUpdateFilePrintPreview(CCmdUI* pCmdUI);
	afx_msg void OnCommandAP(UINT id);
	afx_msg void OnCommandEM(UINT id);
	afx_msg void OnCommandExtModule(UINT id);
   afx_msg void OnSetScenario(UINT id);
   afx_msg void OnSetConstraint(UINT id);
   afx_msg void OnSetZoom(UINT id);
	afx_msg void OnUpdateScenarios(CCmdUI *pCmdUI);
	afx_msg void OnUpdateZooms(CCmdUI *pCmdUI);
	//afx_msg void OnUpdateConstraints(CCmdUI *pCmdUI);

public:
   afx_msg void OnViewOutput();

protected:
   afx_msg void OnUpdateViewOutput( CCmdUI *pCmdUI ); // { pCmdUI->SetCheck( m_wndOutput.IsWindowVisible() ? 1 : 0 ); }
   
   afx_msg void OnAnalysisExportCoverages();
   afx_msg void OnUpdateAnalysisExportCoverages( CCmdUI *pCmdUI ); // { pCmdUI->SetCheck( m_wndOutput.IsWindowVisible() ? 1 : 0 ); }
   afx_msg void OnAnalysisExportOutputs();
   afx_msg void OnUpdateAnalysisExportOutputs( CCmdUI *pCmdUI ); // { pCmdUI->SetCheck( m_wndOutput.IsWindowVisible() ? 1 : 0 ); }
   afx_msg void OnAnalysisExportInterval();
   afx_msg void OnUpdateAnalysisExportInterval( CCmdUI *pCmdUI ); // { pCmdUI->SetCheck( m_wndOutput.IsWindowVisible() ? 1 : 0 ); }
   
   afx_msg LRESULT OnCloseBalloon( WPARAM, LPARAM );
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

   DECLARE_MESSAGE_MAP()

   void AddMainPanel();
   void AddHomeCategory();
   void AddDataCategory();
   void AddMapCategory();
   void AddRunCategory();

   CMFCRibbonPanel *AddViewPanel( CMFCRibbonCategory *pCategory );
   void AddContextTab_Results();
   void AddContextTab_RtViews();
   void AddContextTab_Analytics();

   void AddAnalysisModules();   // these are added at the end of startup
   void AddModels();
   void AddScenarios();
   void AddConstraints();
   void AddZooms( int curSelection );
	void InitializeRibbon();

public:
   void InitializeStatusBar();
   void AddDynamicMenus();
   void ActivateRibbonContextCategory(UINT uiCategoryID);
   void SetRibbonContextCategory(UINT uiCategoryID);  // 0 to hide all, categoryID to show a category

protected:
   BOOL CreateDockingWindows();
   BOOL CreateOutputWnd();
   void SetDockingWindowIcons(BOOL bHiColorIcons);

   virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
};


inline
int  CMainFrame::GetExportMapInterval( void )
   {
   if (! m_pExportCoverages->IsChecked() )
      return -1;
   
   CString interval = m_pExportInterval->GetEditText();
   return atoi( interval );
   }


inline
void CMainFrame::SetYearInfo( int startYear, int lengthOfRun )
   {
   CString buffer;
   buffer.Format( "%i", startYear );
   m_pStartYear->SetEditText( buffer );

   buffer.Format( "%i", lengthOfRun );
   m_pEndYear->SetEditText( buffer );
   }


inline
int CMainFrame::GetStartYear( void )
   {
   CString year = m_pStartYear->GetEditText();
   return atoi( year );
   }


inline
int CMainFrame::GetRunLength( void )
   {
   CString year = m_pEndYear->GetEditText();
   return atoi( year );
   }


inline
void CMainFrame::SetExportDeltaFieldList( LPCTSTR fieldList )
   {
   m_pExportDeltaFields->SetEditText( fieldList);
   }


inline
int CMainFrame::GetExportDeltaFieldList( CString &fieldList )
   {
   fieldList = m_pExportDeltaFields->GetEditText();
   return (int) fieldList.GetLength();
   }
