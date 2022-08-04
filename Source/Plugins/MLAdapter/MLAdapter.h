#pragma once

#include <EnvExtension.h>
#include <PtrArray.h>

#define _EXPORT __declspec( dllexport )


class _EXPORT MLAdapter : public  EnvModelProcess
   {
   public:
      MLAdapter() 
         : EnvModelProcess()
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
      //PtrArray<Hazard> m_hazardArray;

      bool LoadXml(EnvContext* pEnvContext, LPCTSTR filename);

   };


extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new MLAdapter; }

