#pragma once

#include <EnvExtension.h>
#include <PtrArray.h>

#define _EXPORT __declspec( dllexport )


class Hazard
   {
   public:
      CString m_name;
      CString m_hazardField;
      CString m_acmPctileField;
      CString m_hazPctileField;
      CString m_vulnIndexField;
      CStringArray m_adaptCapFields;

      int m_colHazard;
      int m_colAcmPctile;
      int m_colHazPctile;
      int m_colVulnIndex;

      CArray<int, int> m_colsAdaptCap;

      CString m_query;
      Query* m_pQuery;
     
   public:
      Hazard()
         : m_hazardField()
         , m_colHazard(-1)
         , m_colAcmPctile(-1)
         , m_colHazPctile(-1)
         , m_colVulnIndex(-1)
         , m_adaptCapFields()
         , m_colsAdaptCap()
         , m_query()
         , m_pQuery(NULL)
         {}

      Hazard(Hazard &h)
         : m_hazardField( h.m_hazardField)
         , m_colHazard( h.m_colHazard)
         //, m_adaptCapFields( h.m_adaptCapFields)
         //, m_colsAdaptCap(h.m_colsAdaptCap)
         {}
   };


class _EXPORT CVA : public  EnvModelProcess
   {
   public:
      CVA() 
         : EnvModelProcess()
         , m_hazardArray()
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
      PtrArray<Hazard> m_hazardArray;

      bool LoadXml(EnvContext* pEnvContext, LPCTSTR filename);
      void ToPercentiles(float values[], float ranks[], int n);

   };


extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new CVA; }

