#pragma once

#include <EnvExtension.h>

#define _EXPORT __declspec( dllexport )


class _EXPORT ExampleModel : public  EnvModelProcess
   {
   public:
      ExampleModel() 
         : EnvModelProcess() 
         , m_counter(0)
         {}

      virtual bool Init(EnvContext *pEnvContext, LPCTSTR initStr) 
         { 
         AddOutputVar("Counter", m_counter, "");
         return true; 
         }

      //virtual bool InitRun(EnvContext *pEnvContext, bool useInitialSeed) { return true; }
      
      virtual bool Run(EnvContext *pContext) 
         { 
         m_counter++;  
         return true; 
         }

      //virtual bool EndRun(EnvContext *pContext) { return true; }
      //virtual bool Setup(EnvContext *pContext, HWND hWnd) { return false; }
      //virtual bool GetConfig(std::string &configStr) { return false; }
      //virtual bool SetConfig(EnvContext*, LPCTSTR configStr) { return false; }
      //virtual int  InputVar(int id, MODEL_VAR** modelVar);
      //virtual int  OutputVar(int id, MODEL_VAR** modelVar);

   protected:
      int m_counter;
   };


extern "C" _EXPORT EnvExtension* Factory() { return (EnvExtension*) new ExampleModel; }

