#pragma once

#include <EnvExtension.h>
//#include <PtrArray.h>

#define _EXPORT __declspec( dllexport )




class _EXPORT Risk : public  EnvEvaluator
   {
   public:
      Risk() : EnvEvaluator()
         {}

      virtual bool Init(EnvContext* pEnvContext, LPCTSTR initStr);

      virtual bool InitRun(EnvContext* pEnvContext, bool useInitialSeed);
      
      virtual bool Run(EnvContext* pContext);
      //virtual bool EndRun(EnvContext *pContext) { return true; }
      //virtual bool Setup(EnvContext *pContext, HWND hWnd) { return false; }
      //virtual bool GetConfig(std::string &configStr) { return false; }
      //virtual bool SetConfig(EnvContext*, LPCTSTR configStr) { return false; }
      //virtual int  InputVar(int id, MODEL_VAR** modelVar);
      //virtual int  OutputVar(int id, MODEL_VAR** modelVar);

   protected:
      int m_colArea = -1;
      int m_colHazard = -1;
      int m_colImpact = -1;
      int m_colRisk = -1;

      float m_upperBound = 1;
      float m_lowerBound = 0;

      CString m_queryStr;
      Query* pQuery = nullptr;

      bool LoadXml(EnvContext* pEnvContext, LPCTSTR filename);
      //void ToPercentiles(float values[], float ranks[], int n);

   };


extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new Risk; }

