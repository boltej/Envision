#pragma once

#include <EnvExtension.h>
//#include <PtrArray.h>
//#include <VDataObj.h>
//#include <randgen/Randln.hpp>    // lognormal
//#include <randgen/RandNorm.hpp>  // normal
//#include <randgen/RandUnif.hpp>  // normal

// this provides a basic class definition for this
// plug-in module.
// Note that we want to override the parent methods
// for Init, InitRun, and Run.

//#include <python.h>

#define _EXPORT __declspec( dllexport )


struct ColMap
   {
   CString iduField;
   int iduCol = -1;
   CString resultsField;
   int resultCol = -1;
   };

// Model class for Acute Hazards
class _EXPORT RunProcess : public EnvModelProcess
   {
   public:
      // constructor
      RunProcess(void);
      ~RunProcess(void);

      // override API Methods
      bool Init(EnvContext* pEnvContext, LPCTSTR initStr);
      bool InitRun(EnvContext* pEnvContext, bool useInitialSeed);
      bool Run(EnvContext* pContext);

   protected:
      // we'll add model code here as needed
      CString m_processPath;
      CString m_iduFile;
      CString m_resultsFile;

      bool m_use = true;  // if true, this column will be used in the run

      std::vector<ColMap> m_colMaps;

      bool LoadXml(EnvContext*, LPCTSTR filename);

   };
