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
   std::string iduField;
   int iduCol = -1;
   std::string resultsField;
   int resultCol = -1;
   };

// Model class for Acute Hazards
class _EXPORT FloodDamage : public EnvModelProcess
   {
   public:
      // constructor
      FloodDamage(void);
      ~FloodDamage(void);

      // override API Methods
      bool Init(EnvContext* pEnvContext, LPCTSTR initStr);
      bool InitRun(EnvContext* pEnvContext, bool useInitialSeed);
      bool Run(EnvContext* pContext);

   protected:
      // we'll add model code here as needed
      CString m_processPath;
      CString m_iduFile;
      CString m_resultsFile;

      std::vector<ColMap> m_colMaps;

      bool LoadXml(EnvContext*, LPCTSTR filename);

   };
