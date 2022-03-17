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

// EnvView.h : interface of the CEnvView class
//


#include "LulcInfoDialog.h"
#include "PolEditor.h"

#include "ResultsPanel.h"
#include "MapPanel.h"
#include "DataPanel.h"
#include "RtViewPanel.h"
#include "InputPanel.h"
#include "AnalyticsPanel.h"

#include "Visualizer.h"

#include <PtrArray.h>
#include <VideoRecorder.h>


// "standard" video recorders
enum PANEL_TYPE { INPUT_PANEL, MAP_PANEL,  DATA_PANEL, RTVIEW_PANEL, RESULTS_PANEL, ANALYTICS_PANEL };

enum VRTYPE { VRT_MAIN=1, VRT_MAPPANEL=2, VRT_RESULTSPANEL=3, VRT_RTVIEWPANEL=4, VRT_INPUTPANEL=5, VRT_MAINMAP=6, VRT_END=7 };

class CEnvDoc;

class ZoomInfo
{
public:
   CString m_name;
   
   REAL m_xMin;
   REAL m_xMax;
   REAL m_yMin;
   REAL m_yMax;

   ZoomInfo( LPCTSTR name, REAL xMin, REAL yMin, REAL xMax, REAL yMax )
      : m_name( name )
      , m_xMin( xMin )
      , m_xMax( xMax )
      , m_yMin( yMin )
      , m_yMax( yMax )
      { }
};



class CEnvView : public CView
{
protected: // create from serialization only
	CEnvView();
	DECLARE_DYNCREATE(CEnvView)

// Attributes
public:
	CEnvDoc* GetDocument() const;
   void AddResult( CWnd *pWnd );

   // views embedded within this view
   InputPanel     m_inputPanel;
   MapPanel       m_mapPanel;
   RtViewPanel    m_viewPanel;
   DataPanel      m_dataPanel;
   ResultsPanel   m_resultsPanel;
   AnalyticsPanel m_analyticsPanel;


   // modeless dialogs
   LulcInfoDialog m_lulcInfoDialog;
   PolEditor      m_policyEditor;

   // additional information
   VizManager     m_vizManager;  // manages windows associated with visualizer views

protected:
   int m_mode;    // same as Map's MAP_MODE enum
                  // enum MAP_MODE  { MM_NORMAL, MM_ZOOMIN, MM_ZOOMOUT, MM_PAN, MM_INFO, MM_SELECT, MM_QUERYDISTANCE };
public:
   CWnd *GetActivePanel();

   PANEL_TYPE   m_activePanel;      // see enums above.  These must correspond exactly to the tabs in Envision.

// Operations
public:
   //void ChangeTabSelection( int index );
   void SetMode( int mode ) { m_mode = mode; }
   int  GetMode() { return m_mode; }

   void ActivatePanel( int index );
   void UpdateUI( int flags, INT_PTR extra );     // flags 1=map, 2=year, 3=process 

   void EndRun(); // called at the end of each Envision timestep


// Overrides
	// ClassWizard generated virtual function overrides
public:
   virtual void OnDraw(CDC* pDC);  // overridden to draw this view
   virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);

// Implementation
public:
	virtual ~CEnvView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

   // video recorders
protected:
   PtrArray< VideoRecorder > m_vrArray;
   int m_vrFrameRate;      // default for new recorders

public:
   int AddVideoRecorder( VideoRecorder *pVR ) { return (int) m_vrArray.Add( pVR ); }
   int GetVideoRecorderCount()     { return (int) m_vrArray.GetSize(); }
   VideoRecorder *GetVideoRecorder( int i ) { return m_vrArray[ i ]; }
   bool AddStandardRecorders();

   void StartVideoRecorders();
   void UpdateVideoRecorders();
   void StopVideoRecorders(); // writes associated AVIs

   // Zooms
protected:
   PtrArray< ZoomInfo > m_zoomInfoArray;

public:
   ZoomInfo *m_pDefaultZoom;      // default zoom

   int AddZoom( LPCTSTR name, REAL xMin, REAL yMin, REAL xMax, REAL yMax ) { return (int) m_zoomInfoArray.Add( new ZoomInfo( name, xMin, yMin, xMax, yMax ) ); }
   int GetZoomCount( void ) { return (int) m_zoomInfoArray.GetSize(); }
   ZoomInfo *GetZoomInfo( int i ) { return m_zoomInfoArray.GetAt( i ); }
   void SetZoom( int zoomIndex );

protected:
   // views
   afx_msg void OnViewInput();
   afx_msg void OnViewMap();
   afx_msg void OnViewTable();
   afx_msg void OnViewRtViews();
   afx_msg void OnViewResults();
   afx_msg void OnViewAnalytics();
   afx_msg void OnViewOutput();

   // data menu
   afx_msg void OnDataLulcInfo();
   afx_msg void OnDataIduInfo();

   // Analysis Menu handlers
   afx_msg void OnEditPolicies();
   afx_msg void OnEditActors();
   afx_msg void OnEditLulc();
   afx_msg void OnEditScenarios();
   afx_msg void OnAnalysisSetupRun();
   afx_msg void OnAnalysisSetupModels() { }
   
public:
   afx_msg void OnExportCoverages();
   afx_msg void OnExportOutputs();
   afx_msg void OnExportDeltas();
   afx_msg void OnExportDeltaFields();
   afx_msg void OnVideoRecorders();
 
protected:
   afx_msg void OnUpdateExportCoverages(CCmdUI *pCmdUI);
   afx_msg void OnUpdateExportInterval(CCmdUI *pCmdUI);
   afx_msg void OnUpdateExportOutputs(CCmdUI *pCmdUI);
   afx_msg void OnUpdateExportDeltas(CCmdUI *pCmdUI);
   afx_msg void OnUpdateExportDeltaFields(CCmdUI *pCmdUI);
   afx_msg void OnUpdateStartYear(CCmdUI *pCmdUI);
   afx_msg void OnUpdateStopTime(CCmdUI *pCmdUI);

   // Window messages
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnUpdateResultsTileCascade(CCmdUI* pCmdUI);
   afx_msg void OnResultsCascade();
	afx_msg void OnResultsTile();
	afx_msg void OnResultsClear();
   //afx_msg void OnResultsCapture();
   //afx_msg void OnUpdateResultsCapture(CCmdUI* pCmdUI);
   afx_msg void OnResultsDeltas();
   afx_msg void OnUpdateResultsDeltas(CCmdUI* pCmdUI);
   afx_msg void OnResultsSyncMaps();
   afx_msg void OnUpdateResultsSyncMaps( CCmdUI *pCmdUI );

   afx_msg void OnUpdateAnalyticsTileCascade(CCmdUI* pCmdUI);
   afx_msg void OnAnalyticsCascade();
	afx_msg void OnAnalyticsTile();
	afx_msg void OnAnalyticsClear();
   afx_msg void OnAnalyticsCapture();
   afx_msg void OnUpdateAnalyticsCapture( CCmdUI *pCmdUI );
   afx_msg void OnAnalyticsAdd();
   afx_msg void OnUpdateAnalyticsAdd( CCmdUI *pCmdUI );

   afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
   afx_msg void OnFilePrint();
   afx_msg void OnUpdateFilePrint(CCmdUI *pCmdUI);
   afx_msg void OnClose();

   afx_msg void OnZoomSlider();
   afx_msg void OnZoomButton();

   afx_msg void OnEditCopy();
	afx_msg void OnEditCopyLegend();
	afx_msg void OnZoomin();
	afx_msg void OnZoomout();
	afx_msg void OnZoomfull();
	afx_msg void OnZoomToQuery();
   afx_msg void OnSelect();
	afx_msg void OnClearSelection();
	afx_msg void OnQuerydistance();
	afx_msg void OnPan();
	afx_msg void OnViewPolygonedges();
	afx_msg void OnViewShowMapScale();
	afx_msg void OnViewShowAttribute();
	afx_msg void OnUpdateIsMapWnd(CCmdUI* pCmdUI);
   afx_msg void OnUpdateIsSelection( CCmdUI *pCmdUI );
	afx_msg void OnUpdateViewPolygonedges(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewShowMapScale(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewShowAttribute(CCmdUI* pCmdUI);
	afx_msg void OnAnalysisShowactorterritory();
	afx_msg void OnAnalysisShowactors();
	afx_msg void OnUpdateData(CCmdUI* pCmdUI);
 	
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnDbtable();

   //afx_msg void OnDataExportdata();
   afx_msg void OnSaveShapefile();
   afx_msg void OnUpdateSaveShapefile(CCmdUI *pCmdUI);
//   afx_msg void OnConnectToHost();
//   afx_msg void OnUpdateConnectToHost(CCmdUI *pCmdUI);
//   afx_msg void OnHostSession();
//   afx_msg void OnUpdateHostSession(CCmdUI *pCmdUI);
   afx_msg void OnAnalysisGoalspace();
   afx_msg void OnAnalysisShowdeltalist();
   afx_msg void OnUpdateAnalysisShowdeltalist(CCmdUI *pCmdUI);
   afx_msg void OnAnalysisSetuppolicymetaprocess();
   afx_msg void OnUpdateAnalysisSetuppolicymetaprocess(CCmdUI *pCmdUI);
   afx_msg void OnDataStorerundata();
   afx_msg void OnDataLoadrundata();
   afx_msg void OnRunquery();
   virtual void OnInitialUpdate();
   afx_msg void OnUpdateEditCopy(CCmdUI *pCmdUI);
   afx_msg void OnEditSetpolicycolors();  

   void RandomizeLulcLevel( int level );
   virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
};



#ifndef _DEBUG  // debug version in EnvView.cpp
inline CEnvDoc* CEnvView::GetDocument() const
   { return reinterpret_cast<CEnvDoc*>(m_pDocument); }
#endif

