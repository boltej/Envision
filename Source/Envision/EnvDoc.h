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

// EnvisionDoc.h : interface of the CEnvDoc class
//

#pragma once
#pragma warning( disable : 4995 )

#include <afxtempl.h>
#include <EnvModel.h>
#include <Maplayer.h>
#include <tixml.h>
#include <PtrArray.h>
#include <iostream>


// following used for SetChanged( int flag ), UnSetChanged( int flag ), IsChanged( int flag )
enum { CHANGED_PROJECT=1, CHANGED_POLICIES=2, CHANGED_ACTORS=4, CHANGED_COVERAGE=8, CHANGED_SCENARIOS=16, CHANGED_LULC=32, CHANGED_OTHER=64 };


// note:  types are  vt types as defined typedefs.h
// display flags :  see bin flags in maplayer.h


/////////////////// CEnvDoc class /////////////////////////////////

class CEnvDoc : public CDocument
{
protected: // create from serialization only
	CEnvDoc();
	DECLARE_DYNCREATE(CEnvDoc)

// Attributes
public:
   // global parameters
   CString m_userName;        // user logged onto this session

   // INI file settings
   static CString m_iniFile;                    // ini file name
   static CString m_databaseName;               // path/file to current evoland database
   static CStringArray m_fieldInfoFileArray;    // field information files (only user specified - not autoloaded) - no default

   bool    m_coupleSliders;
   int     m_loadSharedPolicies;

   int     m_errors;    // keep track of what's been reported in the logBook
   int     m_warnings;
   int     m_infos;

   int     m_cmdRunScenario;     // -1 for no autorun, 0=run all scenarios and exit, >0 = run the given scenario (one based) and exit
   int     m_cmdMultiRunCount;   // 1 for single run, > 1 for multiruns
   bool    m_cmdNoExit;          // avoid terminating after run/multirun 
   CString m_cmdLoadProject;     // name of project file passed in on command line (empty of not passed via command line)

   // OPTION settings stored in the registry
   //bool    m_showTips;    // startup tips

   DWORD   m_outputWndStatus;    // outputWnd opened or closed on startup? 0=closed, 1=open
   
   //FieldInfoArray m_fieldInfoArray;
   EnvModel m_model;

   bool m_runEval;
   int  m_exitCode;    // see Envision.cpp for codes;
   
protected:
   // application-defined variables
   PtrArray< EnvAnalysisModule > m_analysisModuleArray;
   PtrArray< EnvVisualizer> m_visualizerArray;
   PtrArray< EnvDataModule > m_dataModuleArray;
   CStringArray m_constraintArray; 

public:
   int GetConstraintCount() { return (int) m_constraintArray.GetCount(); }
   CString &GetConstraint( int i ) { return m_constraintArray[ i ]; }
   void RemoveConstraints() { m_constraintArray.RemoveAll(); }
   void AddConstraint( LPCTSTR constraint ) { m_constraintArray.Add( constraint ); }

public:
   EnvAnalysisModule *GetAnalysisModule( int i ) { return m_analysisModuleArray[ i ]; }
   int  GetAnalysisModuleCount( ) { return (int) m_analysisModuleArray.GetSize(); }
   void RemoveAnalysisModule( int i ) { m_analysisModuleArray.RemoveAt( i ); }
   int  AddAnalysisModule( EnvAnalysisModule *pInfo ) { return (int) m_analysisModuleArray.Add( pInfo ); }
   
   EnvVisualizer *GetVisualizer( int i ) { return m_visualizerArray[ i ]; }
   int  GetVisualizerCount( ) { return (int) m_visualizerArray.GetSize(); }
   void RemoveVisualizer( int i ) { m_visualizerArray.RemoveAt( i ); }
   int  AddVisualizer( EnvVisualizer *pInfo ) { return (int) m_visualizerArray.Add( pInfo ); }
   
   EnvDataModule *GetDataModule( int i ) { return m_dataModuleArray[ i ]; }
   int  GetDataModuleCount( ) { return (int) m_dataModuleArray.GetSize(); }
   void RemoveDataModule( int i ) { m_dataModuleArray.RemoveAt( i ); }
   int  AddDataModule( EnvDataModule *pInfo ) { return (int) m_dataModuleArray.Add( pInfo ); }


protected:
   int  m_isChanged;         // 1=policies, 2 = actors, 16 everything else or'd together (see enum above)

// Operations
public:
   void Clear();           // clears out CEnvDoc in prep for a new document load
   bool InitDoc( void );
   int  StartUp();
   int  OpenDocXml( LPCTSTR filename );

   void SetChanged  ( int flag ) { m_isChanged |= flag; }  // set enum above
   void UnSetChanged( int flag ) { m_isChanged &= ~flag; }
   int  IsChanged( int flag ) { return m_isChanged & flag; }

   //void InitModels( void );
   //void InitVisualizers( void );

protected:
   bool CheckForUpdates();     // return true to update, false otherwise

   void RunModel();
   void RunMultiple();

public:
   //int  LoadDB( LPCTSTR filename );
   //int  LoadDBWeb( LPCTSTR url );

   //int  LoadFieldInfoXml( MapLayer *pLayer, LPCTSTR filename ); // note: pLayer=NULL means find layer name in XML file and fix up layer ptr
   //int  LoadFieldInfoXml( TiXmlElement *pXmlElement, MAP_FIELD_INFO *pParent, MapLayer *pLayer, LPCTSTR filename ); // note: pLayer=NULL means find layer name in XML file and fix up layer ptr
   int  SaveFieldInfoXML( MapLayer *pLayer, LPCTSTR filename );
   bool SaveFieldXml( std::ofstream &file, MAP_FIELD_INFO *pInfo, MapLayer *pLayer, int tabCount );

   //int  LoadLayer( LPCTSTR name, LPCTSTR path, int type,  int includeData, int red, int green, int blue, int extraCols, int records, LPCTSTR initField, int overlayFlags, bool loadFieldInfo );
   int  SaveAs( LPCTSTR filename );     // save v5+ xml file
   //bool SaveIni( LPCTSTR filename );    // saves older ini file (deprecated)
   //bool LoadModel( LPCTSTR name, LPCTSTR path, int modelID, int use, int showInResults, /*int decisionUse,*/ int useCol, LPCTSTR fieldname, LPCTSTR initFnInfo, LPCTSTR dependencyNames, int initRunOnStartup, float gain, float offset );
   //bool LoadAP( LPCTSTR name, LPCTSTR path, int apID, int use, int timing, int sandbox, int useCol, LPCTSTR fieldname, LPCTSTR initFnInfo, LPCTSTR dependencyNames, int initRunOnStartup );
   bool LoadExtension( LPCTSTR name, LPCTSTR path, int modelID, int use, int showInResults, int decisionUse, int useCol, LPCTSTR fieldname, LPCTSTR initFnInfo, float gain, float offset );
   bool LoadVisualizer( LPCTSTR name, LPCTSTR path, int use, int type, LPCTSTR initInfo );

protected:
   int  LoadExtensions();
   void LoadDatabase(); // deprecated
   //bool LoadActors();
   //bool LoadPolicies();
   //bool VerifyIDULayer( void );  // checks that lulc and area columns contain reasonable data
   //bool CheckValidFieldNames( MapLayer* );
   MapLayer *AddMapLayer( LPCTSTR filename, int type, int extraCols, int includeData, int records, bool loadFieldInfo );

   // ReconcileDataTypes() ensures ExtraCols added to MapLayer DB for Autonomous & Evaluative Models
   // are correct DataType and that DataType of FieldInfoArray fields do not contradict the DataType in the DB.
   int   ReconcileDataTypes(void);   
   void  SetDependencies( void );
   int   CompilePolicies( void );

public:
   //int   ModelNumberToDbGoalIDWeb( CArray<int> & goalIDArray );

protected:
   int   LoadActorFromIduCoverage(int actorDecisionFrequency); // requires .ini file record.
   int   LoadActorUniform( int actorDecisionFrequency, char *args ); // requires .ini file record.  Args is a comma-delimited list of model goal values.
   int   SaveDB( LPCTSTR filename );

   //int   SavePolicyRecordsWeb();

   bool  PopulateLulcFields( void );
   bool  PopulateLulcLevel( int level, int thisCol, int parentCol );   // helper function for PopulateLulcFields();
   void  InitActorValuesToCells();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEnvDoc)
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CEnvDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	afx_msg void OnFileLoadmap();
	afx_msg void OnUpdateIsDocOpen(CCmdUI* pCmdUI);   // update handler
	afx_msg void OnUpdateIsDocChanged(CCmdUI* pCmdUI);   // update handler
	afx_msg void OnUpdateAnalysisRun(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()

public:
  afx_msg void OnImportPolicies();
  afx_msg void OnImportActors();
  afx_msg void OnImportLulc();
  afx_msg void OnImportScenarios();
  afx_msg void OnImportMultirunDataset();

  afx_msg void OnUpdateImportMultirunDataset(CCmdUI *pCmdUI);

public:
	afx_msg void OnFileSavedatabase();
   afx_msg void OnFileExport();
   afx_msg void OnExportActors();
   afx_msg void OnExportPolicies();
   afx_msg void OnExportLulc();
   afx_msg void OnExportScenarios();
   afx_msg void OnExportFieldInfo();
   afx_msg void OnExportPoliciesashtmldocument();
   afx_msg void OnExportModelOutputs();
   afx_msg void OnExportMultirunDataset();
   afx_msg void OnExportDecadalcelllayers();
   afx_msg void OnExportFieldInfoAsHtml();
   afx_msg void OnExportDeltaArray();

   afx_msg void OnUpdateFileExport(CCmdUI *pCmdUI);
   afx_msg void OnUpdateExportModelOutputs(CCmdUI *pCmdUI);
   afx_msg void OnUpdateExportMultirunDataset(CCmdUI *pCmdUI);
   afx_msg void OnUpdateExportDecadalcelllayers(CCmdUI *pCmdUI);
   afx_msg void OnUpdateExportDeltaArray(CCmdUI *pCmdUI);


   afx_msg void OnOptions();

   afx_msg void OnAnalysisRun();
   afx_msg void OnAnalysisStop();
   afx_msg void OnUpdateAnalysisStop(CCmdUI *pCmdUI);
   afx_msg void OnAnalysisReset();
   afx_msg void OnUpdateAnalysisReset(CCmdUI *pCmdUI);
   afx_msg void OnAnalysisRunmultiple();
	afx_msg void OnUpdateAnalysisRunMultiple(CCmdUI* pCmdUI);
   afx_msg void OnDatapreparationDiststr();

   afx_msg void OnEditFieldInformation();
   afx_msg void OnEditIniFile();

   // data menu handlers
   afx_msg void OnDataMerge();
   afx_msg void OnDataChangeType();
   afx_msg void OnDataAddCol();
   afx_msg void OnDataCalculateField();
   afx_msg void OnDataRemoveCol();
   afx_msg void OnDataBuildnetworktree();
   afx_msg void OnDataArea();
   afx_msg void OnDataVerifyCellLayer();
   afx_msg void OnDataInitializePolicyEfficacies();
   afx_msg void OnDataDistanceTo();
   afx_msg void OnDataNextTo();
   afx_msg void OnDataLulcFields();
   afx_msg void OnDataEvaluativeModelLearning();
   afx_msg void OnDataProjectTo3d();
   afx_msg void OnDataHistogram();
   afx_msg void OnDataElevation();
   afx_msg void OnDataLinearDensity();
   afx_msg void OnDataTextToID();
   afx_msg void OnDataIntersect();
   afx_msg void OnDataReclass();
   afx_msg void OnDataPopulateIduIndex();
   afx_msg void OnDataGenerateFieldDocumentation();

   afx_msg void OnMapAreaSummary();
   afx_msg void OnMapAddDotDensityMap();
   afx_msg void OnMapConvert();

   // analysis menu handlers
   afx_msg void OnEditPolicies();
   afx_msg void OnEditScenarios();
   afx_msg void OnSensitivityAnalysis();

};

