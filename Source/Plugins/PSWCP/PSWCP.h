#pragma once

#include <EnvExtension.h>

#include <vdataobj.h>
#include <fdataobj.h>
#include <AttrIndex.h>

#define _EXPORT __declspec( dllexport )

struct LULCINFO {
   LPCTSTR label; 
   int minImpactValue; 
   int maxImpactValue; 
   LULCINFO(LPCTSTR _label, int _min, int _max) : label(label), minImpactValue(_min), maxImpactValue(_max) {};
   };


enum TABLE {
   NULL_TABLE=-1,
   WQ_DB_TABLE,
   WQ_M1_TABLE,
   WQ_RP_TABLE,

   WF_DB1_TABLE,
   WF_DB2_TABLE,
   WF_M1_TABLE,
   WF_M2_TABLE,
   WF_RP_TABLE,

   HAB_TERR_TABLE,

   HCI_VALUE_TABLE,
   HCI_CATCH_TABLE
   };

struct TABLECOL {
   TABLE table;
   VDataObj* pTable;
   int   col;
   LPCTSTR field;
   };

enum TU_OPS {
   TUOP_AREA,
   TUOP_AREAWTMEAN
   };

// TABLE_UPDATES define PSWCP Table variables and how to calculate them from
// IDU values for each AU (Analysis Unit)
// basic idea of table lookups:
//   1) Initially, we track: 1) table values for the TABLE_UPDATE variable
//                           2) corresponding (accumulative) ID values for the AU for the TABLE_UPDATE variable
//   2) During Run(), we: 1) compute the (accumulative) IDU values for an AU and store in the TABLE_LOOKUP's iduCurrentValues array
//                        2) scale the IDU value to a table value by linear scaling to baseline values and store in model table
//                                     iduCurrent    tableCurrent
//                                     ----------- = ------------       tc = tb (ic/ib)
//                                     iduBaseline   tableBaseline
//                        3) run the PSWCP Model using model tables
//                        4) store results in IDUs and output data object

struct TABLE_UPDATE {
   LPCTSTR tableVar;   // name of PSWCP table variable
   TABLE table;        // PSWCP table containing this variable;
   CString iduQuery;   // single string=column name, valid query = query
   int     iduCol;     // -1 if query, idu col otherwise
   Query* pQuery;      // NULL if iduCol, not NULL if query
   TU_OPS op;

   CArray<float> iduBaselineValues;    // values=baseline value for the AU from IDUs
   CArray<float> tableBaselineValues;  // values=cumulative value for the AU from table
   CArray<float> iduCurrentValues;     // values=current value for the AU
   };

struct HAB_SCORE {
   int col;
   int valOld;
   int valNew;
   float score;
   };

enum LSGROUP { LG_NULL=-1,LG_COASTAL=0, LG_LOWLAND, LG_MOUNTAINOUS, LG_LAKE, LG_DELTA, LG_COUNT };


class _EXPORT PSWCP : public  EnvModelProcess
   {
   public:
      PSWCP();
      ~PSWCP();

      virtual bool Init(EnvContext* pEnvContext, LPCTSTR initStr);
      virtual bool InitRun(EnvContext* pEnvContext, bool useInitialSeed);
      virtual bool Run(EnvContext* pContext);

      bool LoadXml(EnvContext* pEnvContext, LPCTSTR filename);

      // idu columns
      MapLayer* m_pIDULayer;
      int m_col_IDU_AUW_ID;    // water AU index

      // water quality restoration prioirity codes
      int m_col_IDU_WQS_rp;
      int m_col_IDU_WQP_rp;
      int m_col_IDU_WQMe_rp;
      int m_col_IDU_WQN_rp;
      int m_col_IDU_WQPa_rp;

      int m_col_IDU_WQS_m1_cal;
      int m_col_IDU_WQP_m1_cal;
      int m_col_IDU_WQMe_m1_cal;
      int m_col_IDU_WQN_m1_cal;
      int m_col_IDU_WQPa_m1_cal;

      // water flow importance
      int m_col_IDU_WF1_DE;     // importance to delivery
      int m_col_IDU_WF1_SS;      // importance to surface storage
      int m_col_IDU_WF1_R;       // importance to recharge
      int m_col_IDU_WF1_DI;      // importance of discharge
      int m_col_IDU_WF1_GW;
      int m_col_IDU_WF_M1_CAL;  // calibrated score for model 1 importance

      // water flow degradation
      int m_col_IDU_WF2_IMP;     // IMP impervious surface indicator(urban)
      int m_col_IDU_WF2_IMPPCT;
      int m_col_IDU_WF2_DE;     // degradation to delivery
      int m_col_IDU_WF2_SS;     // degradation to surface storage
      int m_col_IDU_WF2_R;      // degradation to recharge
      int m_col_IDU_WF2_DI;     // degradation to discharge
      int m_col_IDU_WF2_GW;     // degradation to groundwater

      int m_col_IDU_WF_M2_CAL;
      
      int m_col_IDU_WF_RP;

      // habitat
      int m_col_IDU_AUH_ID;            // hab AU ID
      int m_col_IDU_Hab_IntIndex;      // habitat integrity index
      int m_col_IDU_Hab_PHS;           // priority habitat for sppecies of interest
      int m_col_IDU_Hab_OakGrove;         // oak grove habitat
      int m_col_IDU_Hab_OverallIndex;  // overall (combined) index
      int m_col_IDU_LULC_A;
      int m_col_IDU_LULC_B;
      int m_col_IDU_CONSERVE;
      int m_col_IDU_AREA;

      int m_col_IDU_HCI_Dist;
      int m_col_IDU_HCI_Shed;
      int m_col_IDU_HCI_Value;
      int m_numHCICat;

      // AU coverage for geometry
      MapLayer* m_pAUWLayer;
      MapLayer* m_pAUHLayer;   // not currently used
      MapLayer* m_pTerrHabLayer;

      // data tables with model data
      VDataObj* m_pWqDbTable;
      VDataObj* m_pWqM1Table;
      VDataObj* m_pWqRpTable;
      VDataObj* m_pWfDb1Table;
      VDataObj* m_pWfDb2Table;
      VDataObj* m_pWfM1Table;
      VDataObj* m_pWfM2Table;
      VDataObj* m_pWfRpTable;
      VDataObj* m_pHabTerrTable;
      //data table to store HCI values
      VDataObj* m_pHCITable;
      // lookup table of lulc and hpc
      VDataObj*  m_pHPCTable;
      
      // outputs 
      FDataObj* m_pOutputData;      // a set of useful outputs for each AU, written every 10 years
      FDataObj* m_pOutputInitValues;

      //CArray<float> startWQM1Values;
      //CArray<float> startWFM1Values;
      //CArray<float> startWFM2Values;
      //CArray<float> startHabValues;
      int m_aboveCountWQM1;
      int m_belowCountWQM1;
      int m_aboveCountWfM1;
      int m_belowCountWfM1;
      int m_aboveCountWfM2;
      int m_belowCountWfM2;
      int m_aboveCountHab;
      int m_belowCountHab;

      // various indexes/maps
      CMap<int, int, int, int> m_AUIndex_Tables;
      AttrIndex m_AUWIndex_IDU;  // for IDUs, key=AUWIndex,value=IDU rows containing key  
      AttrIndex m_AUHIndex_IDU;  // for IDUs, key=AUHIndex,value=IDU rows containing key  
      AttrIndex m_HCIIndex_IDU;

      // water flow assessment 

      // habitat assessment


      // HCI assessment

      //
       

      // methods

      // copy values from IDUs to local tables before running assessment
      bool UpdateTablesFromIDULayer(EnvContext*);
      void PopulateTableUpdatesFromIDUs(bool init);

      // Initialization routines
      bool InitOutputData(EnvContext*);
      bool InitColumns(EnvContext*);
      bool InitTableUpdates(EnvContext* pEnvContext);
      FDataObj* InitOutputDataObj(LPCTSTR name);

      bool InitWaterAssessments(EnvContext*);
      bool InitHabAssessments(EnvContext*);
      bool InitHCIAssessment(EnvContext*);

      // run various assessments
      bool RunWFAssessment(EnvContext*);
      bool RunWQAssessment(EnvContext*);
      bool RunHabAssessment(EnvContext*);
      bool RunHCIAssessment(EnvContext*);


      //virtual bool EndRun(EnvContext *pContext) { return true; }
      //virtual bool Setup(EnvContext *pContext, HWND hWnd) { return false; }
      //virtual bool GetConfig(std::string &configStr) { return false; }
      //virtual bool SetConfig(EnvContext*, LPCTSTR configStr) { return false; }
      //virtual int  InputVar(int id, MODEL_VAR** modelVar);
      //virtual int  OutputVar(int id, MODEL_VAR** modelVar);

   protected:
      int SolveWqM1Sed();
      int SolveWqM1Phos();
      int SolveWqM1Metals();
      int SolveWqM1Nit();
      int SolveWqM1Path();

      int SolveWfM1WatDel();
      int SolveWfM1SurfStorage();
      int SolveWfM1RechargeDischarge();
      int SolveWfM1Combined();

      int SolveWfM2WatDel();
      int SolveWfM2SurfStorage();
      int SolveWfM2Recharge();    // NEEDS WORK, DOCS UNCLEAR
      int SolveWfM2Discharge();
      int SolveWfM2EvapTrans();
      int SolveWfM2Combined();

      int SolveHabTerr(EnvContext*);

      int SolveHCI();

      void UpdateIDUs(EnvContext*);

      bool LoadTables();

      VDataObj* LoadTable(TABLE, LPCTSTR filename);
      bool GetTableValue(TABLE t, LPCTSTR field, int row, int& value);
      bool GetTableValue(TABLE t, LPCTSTR field, int row, float& value);
      bool GetTableValue(TABLE t, LPCTSTR field, int row, double& value);
      bool GetTableValue(TABLE t, LPCTSTR field, int row, CString& value);
      bool SetTableValue(TABLE t, LPCTSTR field, int row, int value)    { VData v(value); return SetTableValue(t, field, row, v); }
      bool SetTableValue(TABLE t, LPCTSTR field, int row, float value)  { VData v(value); return SetTableValue(t, field, row, v); }
      bool SetTableValue(TABLE t, LPCTSTR field, int row, LPCTSTR value){ VData v(value); return SetTableValue(t,field,row,v); }

      bool SetTableValue(TABLE, LPCTSTR field, int row, VData& v);

      LSGROUP GetLSGroupIndex(int row);

      bool CollectOutput(FDataObj*, EnvContext* pEnvContext);

   };


extern "C" _EXPORT EnvExtension * Factory(EnvContext*);

