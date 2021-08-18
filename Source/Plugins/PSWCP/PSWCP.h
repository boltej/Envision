#pragma once

#include <EnvExtension.h>

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

      // water quality inportance (Model 1) 

      int m_colS;
      int m_colS_Q;
      int m_colS_Code;

      int m_colP;
      int m_colP_Q;
      int m_colP_Code;

      int m_colM;
      int m_colM_Q;
      int m_colM_Code;

      int m_colN;
      int m_colN_Q;
      int m_colN_Code;

      int m_colPa;
      int m_colPa_Q;
      int m_colPa_Code;

      MapLayer* m_pIDULayer;
      MapLayer* m_pWQ_RP;


      bool InitWQAssessment(EnvContext*);
      bool InitWFAssessment(EnvContext*);
      bool InitHabAssessment(EnvContext*);
      bool InitHCIAssessment(EnvContext*;

      bool RunWQAssessment(EnvContext*);
      bool RunWFAssessment(EnvContext*);
      bool RunHabAssessment(EnvContext*);
      bool RunHCIAssessment(EnvContext*;


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

