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

#ifndef NO_MFC
#include <afxtempl.h>
#endif
#include <FDATAOBJ.H>
#include <Vdataobj.h>
#include <IDATAOBJ.H>

#include "DeltaArray.h"
#include "EnvModel.h"
//#include "resultsPanel.h"

#include "EnvAPI.h"

//extern ResultsPanel    *gpResultsPanel;

enum AREA_FACTOR
   {
   AF_IGNORE = 0,
   AF_MULTIPLY,   // multiply value by area, for density-based columns
   AF_AREAWEIGHT  // return an area-weighted results (cell area/total area)
   };


class DataObjArray : public CArray< DataObj*, DataObj* >
   {
   public:
      //DataObjectArray( int size ) : CArray< DataObj*, DataObj* > { SetSize( size ); }
      ~DataObjArray() { RemoveAllData(); }

      void RemoveAllData() { for (int i = 0; i < GetSize(); i++) delete GetAt(i);  RemoveAll(); }
   };


// DataObjectMatrix is an array of DataObjArray ptrs.  Each DataObjectArray stored (row) represents one run
class DataObjMatrix : public CArray< DataObjArray*, DataObjArray* >
   {
   public:
      ~DataObjMatrix() { RemoveAllData(); }

      FDataObj* GetDataObj(int run, int year);
      bool      SetDataObj(int run, int year, FDataObj* pDataObj);

      void RemoveAllData() { for (int i = 0; i < GetSize(); i++) delete GetAt(i);  RemoveAll(); }
   };


/*------------------------------------------------------------------------------------------
 * Data Objects created during each SINGLE RUN:
 *   1) Eval Model Statistics - rows=year, col for each model that reports results (scaled and raw) (FDataObj)
 *   2) Eval Model/Autonomous Process exposed outputs - rows=year, col for each exposed variable, includes global app-defined AppVars
 *   3) Actor Value Trajectories - rows=year, col for each actor value and actor group
 *   4) Policy Initial Area Results Summary - rows=policy Index, col (only one) is potential application area
 *   4) Policy Application Summary - rows=policy index, cols=policy application stats.  This is calculated at the end of a run
 *   5) Global Constraints values
 *
 * These are stored in m_dataObjs, which stores arrays of dataobj ptrs for each type of data
 *     (each array has a data object of the specified type for each run)
 *
 * Additionally, if models return FDataObjs, they are stored in the m_modelDataObjMatrices array - an array of DataObjMatrix's.
 *   Each matrix is a collection of FDataObjs - row=run, col=year.  An element in the m_modelDataObjartices array is allocated for each
 *   model that reports results, but that element may be nullptr if the model doesn't return FDataObjs
 * The m_modelDataObjMatrices is an array of pointers to DataObjMatrix's.  It's length is the # of models that report results.
 * If a model reports back DataObjs, then its corresponding m_dataObjArray entry is a pntr to a DataObjMatrix, otherwise it's nullptr.
 *
 * m_modelDataObjMatrices looks like:
 *
 * |DataObjMatrix* for Model 0|DataObjMatrix* for Model 1|......|DataObjMatrix* for Model N| (entries may be nullptr )
 *
 * Each models DataObjectMatrix looks like this:
 *
 * [ run 0 DataObjArray ---> [ year 1 ] [ year 2 ] ... [ year N ] ]
 * [ run 1 DataObjArray ---> [ year 1 ] [ year 2 ] ... [ year N ] ]
 *                                        . . .
 * [ run M DataObjArray ---> [ year 1 ] [ year 2 ] ... [ year N ] ]
 *
 * Additionally, if models return FDataObjs, they are stored in the m_modelDataObjs array - an array of DataObj Matrices.
 *   Each matrix is a collection of FDataObjs - row=run, col=year.  An element in the m_modelDataObj array is allocated for each
 *   model that reports results, but that element may be nullptr if the model doesn't return FDataObjs
 *
 * The following methods return dataobjs of the indicated type
 *
 * FDataObj *CalculateLulcATrends( int run = -1 );   // returns a newed FDataObj
 * FDataObj *CalculateLulcTrends( int run = -1 );    // returns a newed FDataObj (deprecated)
 * IDataObj *CalculatePolicyApps( int run = -1 );  // returns a newed FDataObj
 * FDataObj *CalculatePolicyEffectivenessTrendDynamic( int modelIndexX, int modelIndexY, int run = -1 );  // returns a newed FDataObj
 * FDataObj *CalculatePolicyEffectivenessTrendStatic( int run = -1 );  // returns a newed FDataObj
 * VDataObj *CalculatePolicySummary( int run = -1 );  // returns a newed VDataObj
 * FDataObj *CalculateLulcTransTable( int run = -1, int level = 1, int year = -1, bool areaBasis = false, bool includeAlps = true );  // returns a newed FDataObj
 * FDataObj *CalculateTrendsWeightedByLulcA( int trendCol, double scaler = 1.0, int run = -1 );
 * VDataObj *CalculatePolicyByActorArea( run=-1 )
 * VDataObj *CalculatePolicyByActorCount( run=-1 )
 * -------------------------------------------------------------------------------------------
 * Data Objects created during MULTIRUNS:
 *   1) Multirun Landscape Evaluative Statistics - rows=runs, col for each evaluative model that reports results
 *   2) Multirun Scenario Settings - rows=runs, col for each scenario variable
 *
 * These are stored in m_multiRunDataObjs, which stores arrays of dataobjs for each type of data
 *     (each array has a data object of the specified type for each multirun)
 * the m_multiRunDataObj Array is a fixed length = # of types data being collected.
 *
 * Additionally, the following methods return dataobjs of the indicated type
 *
 * FDataObj *CalculateScenarioParametersFreq( int multiRun );
 * FDataObj *CalculateScenarioVulnerability( int multiRun );
 * FDataObj *CalculateEvalFreq( int multiRun );
 * //FDataObj *CalculateFreqTable( FDataObj *pSourceData, int binCount, bool ignoreFirstCol=true );
 *
 *
 * NOTE REGARDING DATAOBJ SIZING and time representation:
 *
 * For those dataobjs that represent a time series, the rows in the dataobj represent the progression
 * of time. In general, the time stored in a dataObj reflects the start of the year condition, e.g.
 * time=0 represent initial conditions, time=1 represents conditions at the start of year 1, etc.
 * This implies that that year depicted in the data object is the "envision" year + 1
 * So, for example, an attribute generated during the first (0) year in Envision would be
 * tagged with a time of 1. However, there are two possibilities.
 *   1) DataObj that include initial conditions - rowcount of dataobj will be years+1.  These are
 *       used to represent state information
 *   2) DataObj that include don't initial conditions - rowcount of dataobj will be years. These
 *       are used to represent change
 *
----------------------------------------------------------------------------------------------*/



// note: following types apply to TIME SERIES data for ONE RUN only - these are needed for each dataobj type stored 
// internally by the DataManager
enum DM_TYPE {
   DT_EVAL_SCORES = 0,     // landscape goal scores trajectories
   DT_EVAL_RAWSCORES,      // landscape goal scores trajectories (raw scores)
   DT_MODEL_OUTPUTS,       // output variables from eval and autonous process models, + App vars
   DT_ACTOR_WTS,           // actor group data
   DT_ACTOR_COUNTS,        // actor counts by actor group
   DT_POLICY_SUMMARY,      // policy summary information (only initial areas)
   DT_GLOBAL_CONSTRAINTS,  // Global Constraints summary outputs
   DT_POLICY_STATS,        // info on policy stats (intrumentation)
   DT_LAST                 // NOTE: This must ALWAYS be last
   };

enum DM_MULTITYPE {
   DT_MULTI_EVAL_SCORES = 0,     // landscape goal scores trajectories
   DT_MULTI_EVAL_RAWSCORES,      // landscape goal scores trajectories (raw scores)
   DT_MULTI_PARAMETERS,          // parameter values used in a scenario
   DT_MULTI_LAST                 // NOTE: This must ALWAYS be last
   };


struct RUN_INFO
   {
   Scenario* pScenario;
   int scenarioRun;     // 0-based

   int startYear;    // calendar year
   int endYear;      // calendar year

   RUN_INFO() : pScenario(nullptr), scenarioRun(-1), startYear(0), endYear(0) { }
   RUN_INFO(Scenario* _pScenario, int scenarioRun, int _startYear, int _endYear) : pScenario(_pScenario), scenarioRun(scenarioRun), startYear(_startYear), endYear(_endYear) { }
   RUN_INFO& operator = (const RUN_INFO& m) { pScenario = m.pScenario; scenarioRun = m.scenarioRun; startYear = m.startYear; endYear = m.endYear; return *this; }
   RUN_INFO(const RUN_INFO& m) { *this = m; }
   };


struct MULTIRUN_INFO
   {
   int runFirst;              // number of first run in this multirun
   int runLast;               // number of last run in this multirun
   Scenario* pScenario;

   MULTIRUN_INFO() : runFirst(-1), runLast(-1), pScenario(nullptr) { }
   MULTIRUN_INFO(int first, int last, Scenario* _pScenario) : runFirst(first), runLast(last), pScenario(_pScenario) { }
   MULTIRUN_INFO& operator = (const MULTIRUN_INFO& m) { runFirst = m.runFirst; runLast = m.runLast; pScenario = m.pScenario; return *this; }
   MULTIRUN_INFO(const MULTIRUN_INFO& m) { *this = m; }
   };


//----------------------------------------------------------------------------------
// DataManager class
//
// This class is responsible for
//   1) Maintaining dataobjects and deltaArrays for the current run
//   2) Maintaining dataobjects and deltaArrays across multiple runs
//   3) For datasets NOT collected during the run, generate needed data on request
//          (typically by processing the appropriate deltaArrays)
//----------------------------------------------------------------------------------

class ENVAPI DataManager
   {
   friend class EnvModel;

   public:
      DataManager(EnvModel*);
      ~DataManager(void);

   protected:
      EnvModel* m_pEnvModel;

      // "Current" DataObjs (for the current run)
      DataObj* m_currentDataObjs[DT_LAST];

      // arrays of data objects (from multiple runs)
      DataObjArray m_dataObjs[DT_LAST];    // note: this is an array of dataobj ptr arrays arrays.  Each DataObj is deleted when the array is deleted

      // Current Multirun DataObjs (for the current multirun)
      DataObj* m_currentMultiRunDataObjs[DT_MULTI_LAST];

      // arrays of Mulitrun data objects (from multiple runs)
      DataObjArray m_multiRunDataObjs[DT_MULTI_LAST];


      //DataObjArray m_sessionDataObjs;  // collection of data objs for each output 

      // FDataObj's returned from Eval models  
      DataObjMatrix** m_modelDataObjMatrices;        // array of length [# of models] - each element corresponds to a model
      // if the array element is nullptr, no DataObjs are returned from the model
// current delta array;
      DeltaArray* m_pDeltaArray;

      // array of deltaArrays (from multiple runs)
      CArray< DeltaArray*, DeltaArray* > m_deltaArrayArray;

      PtrArray< DataObj > m_modelOutputDataObjArray;   // arrays of various types of data object used for model output - we management memory here

   public:
      // NOTE: All CalculateXXXX() methods returns a new'ed dataobj. deletion is the responsibility of the caller!!!
      FDataObj* CalculateLulcTrends(int level, int run = -1);   // level = 1/2/3, returns a newed FDataObj

      //FDataObj *CalculateVulnerability( int run = -1 );
      FDataObj* CalculateEvalFreq(int multiRun);
      IDataObj* CalculatePolicyApps(int run = -1);  // returns a newed FDataObj
      FDataObj* CalculatePolicyEffectivenessTrendDynamic(int modelIndexX, int modelIndexY, int run = -1);  // returns a newed FDataObj
      FDataObj* CalculatePolicyEffectivenessTrendStatic(int run = -1);  // returns a newed FDataObj
      VDataObj* CalculatePolicySummary(int run = -1);  // returns a newed VDataObj
      VDataObj* CalculatePolicyByActorArea(int run = -1);  // returns a newed FDataObj
      VDataObj* CalculatePolicyByActorCount(int run = -1);  // returns a newed IDataObj

      FDataObj *CalculateLulcTransTable( int run=-1, int level = 1, int year = -1, bool areaBasis = true, bool includeAlps = true );  // returns a newed FDataObj
      //FDataObj *CalculateTrendsWeightedByLulcA( int trendCol, double scaler = 1.0, AREA_FACTOR af=AF_IGNORE, int run=-1, bool includeTotal=true );
      FDataObj* CalculateScenarioParametersFreq(int multiRun);
      //FDataObj* CalculateScenarioVulnerability(int multiRun);
      //FDataObj *CalculateDecadalLUStatusVariance( int intMultiRun );
      FDataObj* CalculateFreqTable(FDataObj* pSourceData, int binCount, bool ignoreFirstCol = true);
      //VDataObj *CalculateEvalScoresByLulcA( int year, int run=-1 );

      // the following methods are used by the View Windows
      //void CalculateLulcTrends( int level, FDataObj **ppData, int fromYearClosed, int toYearOpen, int run = -1 ); // Endpoint Statistics
      //void CalculateAppliedPolicyScores( FDataObj **ppData, int fromYearClosed, int toYearOpen, int run = -1 ); // Interval Statistics
      //void CalculateAppliedPolicyCounts( FDataObj **ppData, int fromYearClosed, int toYearOpen, int run = -1 ); // Interval Statistics
      //void CalculateAreaQuery( IDataObj **ppData, int fromYearClosed, int toYearOpen, int col, VData &value, int run /*= -1*/ );

      // return existing data object.  Caller should not delete!!!
      DataObj* GetDataObj(DM_TYPE type, int run = -1);
      DataObj* GetMultiRunDataObj(DM_MULTITYPE type, int multirun = -1);
      DataObjMatrix* GetDataObjMatrix(int resultModelIndex, int run = -1);

      DeltaArray* GetDeltaArray(int run = -1) { if (run < 0) return m_pDeltaArray; return m_deltaArrayArray[run]; }

      bool SubsetData(DM_TYPE type, int run, int col, int start, int end, CArray<float, float>& data);

      bool m_collectMultiRunOutput = false;
      bool m_collectActorData;
      bool m_collectLandscapeScoreData;
      //bool m_collectPolicyPriorityData;
      int multiRunDecadalMapsModulus;

   public:
      // data management - run data
      void Clear();     // deletes everything
      bool CreateDataObjects();
      bool AppendRows(int rows);
      void SetDataSize(int rows);
      bool CollectData(int year);
      void EnableCollection(DM_TYPE, bool enable = true);
      void EndRun(bool discardDeltaArray);
      int GetDeltaArrayCount() { return (int)m_deltaArrayArray.GetSize(); }
      bool ClearRunData(int run = -1);

      //int CleanFileName ( CString &filename );

      int ImportMultiRunData(LPCTSTR path, int multirun = -1);  // loads all data associated with a previously saved multirun.
      bool FixSubdividePointers(DeltaArray* pDA);
      bool ExportMultiRunData(LPCTSTR path, int multirun = -1);  // saves all data associated with a multirun.
      //bool ExportRunData( LPCTSTR path, int flags /*1=maps, 2=outputs*/, int run );
      bool ExportDataObject(DataObj* pData, LPCTSTR path, LPCTSTR filename = nullptr);

      //bool ExportRunMaps( LPCTSTR path, int run=-1, int interval=10 );  // path should include terminal '\'
      bool ExportRunDeltaArray(LPCTSTR path, int run = -1, int startYear = -1, int endYear = -1, bool selectedOnly = false, bool includeDuplicates = false, LPCTSTR fieldList = nullptr);
      bool ExportRunData(LPCTSTR path, int run = -1);
      bool ExportRunMap(LPCTSTR path, int run = -1); // int year );   // note: run currently ignored
      bool ExportRunBmps(LPCTSTR path, int run = -1); // int year );  // note: run currently ignored



      // Store/Load Current Run
      bool SaveRun(LPCTSTR fileName, int run = -1);
      bool LoadRun(LPCTSTR fileName);
      bool SaveDataObj(FILE* fp, DataObj& dataObj);
      bool LoadDataObj(FILE* fp, DataObj& dataObj);
      bool SaveCString(FILE* fp, CString& string);
      bool LoadCString(FILE* fp, CString& string);

      // data management - multirun data
      bool CreateMultiRunDataObjects(void);
      bool CollectMultiRunData(int run);

      // delta array management
      //bool CreateDeltaArray( void );
      //int  ApplyDeltaArray( int currentYear );
      int  AddDeltaArray(DeltaArray* delta) { return (int)m_deltaArrayArray.Add(delta); }
      void DiscardDeltaArray(int run) { if (m_deltaArrayArray[run] != nullptr) { delete m_deltaArrayArray[run]; m_deltaArrayArray[run] = nullptr; } }
      //void SetFirstUnevaluatedDelta() { m_pDeltaArray->SetFirstUnevaluated(); }

   protected:
      void RemoveAllData(void);

   protected:
      CArray< RUN_INFO, RUN_INFO& > m_runInfoArray;
      CArray< MULTIRUN_INFO, MULTIRUN_INFO& > m_multirunInfoArray;

   public:
      int GetRunCount() { return (int)m_runInfoArray.GetCount(); }
      RUN_INFO& GetRunInfo(int run) { if (run < 0) run = GetRunCount() - 1; return m_runInfoArray[run]; }

      int GetMultiRunCount() { return (int)m_multirunInfoArray.GetCount(); }
      MULTIRUN_INFO& GetMultiRunInfo(int multirun) { return m_multirunInfoArray[multirun]; }

      // publisher interface
      CString m_publishRootPath;

      bool publishMaps;
      bool publishOutputs;
      bool publishDeltas;

   };
