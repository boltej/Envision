#pragma once

#include <EnvExtension.h>

#include <vdataobj.h>
#include <AttrIndex.h>

#define _EXPORT __declspec( dllexport )

enum TABLE {
   NULL_TABLE=-1,
   WQ_DB_TABLE,
   WQ_M1_TABLE,
   WQ_RP_TABLE,

   WF_DB1_TABLE,
   WF_DB2_TABLE,
   WF_M1_TABLE,
   WF_M2_TABLE,
   WF_RP_TABLE
   };

struct TABLECOL {
   TABLE table;
   VDataObj* pTable;
   int   col;
   LPCTSTR field;
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
      int m_col_IDU_AUIndex;
      int m_col_IDU_WqS_rp;
      int m_col_IDU_WqP_rp;
      int m_col_IDU_WqMe_rp;
      int m_col_IDU_WqN_rp;
      int m_col_IDU_WqPa_rp;

      // data tables with model data
      VDataObj* m_pWqDbTable;
      VDataObj* m_pWqM1Table;
      VDataObj* m_pWqRpTable;
      VDataObj* m_pWfDb1Table;
      VDataObj* m_pWfDb2Table;
      VDataObj* m_pWfM1Table;
      VDataObj* m_pWfM2Table;
      VDataObj* m_pWfRpTable;

      AttrIndex m_index_IDU;  // for IDUs, key=AUIndex,value=IDU rows containing key  

      // water flow assessment 

      // habitat assessment

      // HCI assessment



      // methods
      bool InitAssessment(EnvContext*);
      bool InitHCIAssessment(EnvContext*);

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

      int SolveWfM2WatDel();
      int SolveWfM2SurfStorage();
      int SolveWfM2Recharge();    // NEEDS WORK, DOCS UNCLEAR
      int SolveWfM2Discharge();
      int SolveWfM2EvapTrans();

      int LoadTable(TABLE);
      bool GetTableValue(TABLE t, LPCTSTR field, int row, int& value);
      bool GetTableValue(TABLE t, LPCTSTR field, int row, float& value);
      bool GetTableValue(TABLE t, LPCTSTR field, int row, CString& value);
      bool SetTableValue(TABLE t, LPCTSTR field, int row, int value)    { VData v(value); return SetTableValue(t, field, row, v); }
      bool SetTableValue(TABLE t, LPCTSTR field, int row, float value)  { VData v(value); return SetTableValue(t, field, row, v); }
      bool SetTableValue(TABLE t, LPCTSTR field, int row, LPCTSTR value){ VData v(value); return SetTableValue(t,field,row,v); }

      bool SetTableValue(TABLE, LPCTSTR field, int row, VData& v);

      LSGROUP GetLSGroupIndex(int row);

   };


extern "C" _EXPORT EnvExtension * Factory(EnvContext*);

