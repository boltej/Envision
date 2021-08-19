#pragma once

#include <EnvExtension.h>

#include <vdataobj.h>
#include <AttrIndex.h>

#define _EXPORT __declspec( dllexport )


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


      // water quality inportance (Model 1) 
      // WQ_RP.csv columns
      int m_col_WQ_AU;
      int m_col_WQ_S;
      int m_col_WQ_P;
      int m_col_WQ_Me;
      int m_col_WQ_N;
      int m_col_WQ_Pa;
      
      int m_col_WQ_S_rp;  // priority codes
      int m_col_WQ_P_rp;
      int m_col_WQ_Me_rp;
      int m_col_WQ_N_rp;
      int m_col_WQ_Pa_rp;
      
      int m_col_WQ_S_Q;
      int m_col_WQ_P_Q;
      int m_col_WQ_Me_Q;
      int m_col_WQ_N_Q;
      int m_col_WQ_Pa_Q;

      VDataObj* m_pWqRpTable;
      VDataObj* m_pWfRpTable;
      AttrIndex m_index_IDU;  // for IDUs, key=AUIndex,value=IDU rows containing key  

      // water flow assessment 

      // habitat assessment

      // HCI assessment



      // methods
      bool InitWQAssessment(EnvContext*);
      bool InitWFAssessment(EnvContext*);
      bool InitHabAssessment(EnvContext*);
      bool InitHCIAssessment(EnvContext*);

      bool RunWQAssessment(EnvContext*);
      bool RunWFAssessment(EnvContext*);
      bool RunHabAssessment(EnvContext*);
      bool RunHCIAssessment(EnvContext*);


      //virtual bool EndRun(EnvContext *pContext) { return true; }
      //virtual bool Setup(EnvContext *pContext, HWND hWnd) { return false; }
      //virtual bool GetConfig(std::string &configStr) { return false; }
      //virtual bool SetConfig(EnvContext*, LPCTSTR configStr) { return false; }
      //virtual int  InputVar(int id, MODEL_VAR** modelVar);
      //virtual int  OutputVar(int id, MODEL_VAR** modelVar);

   protected:
      int m_counter;
   };


extern "C" _EXPORT EnvExtension * Factory(EnvContext*);

