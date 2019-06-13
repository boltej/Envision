#pragma once

#include <EnvExtension.h>


#define _EXPORT __declspec( dllexport )


// this provides a basic class definition for this
// plug-in module.
// Note that we want to override the parent methods
// for Init, InitRun, and Run.

class LPJGuess : public EnvModelProcess
   {
   public:
      // constructor
      ~LPJGuess(void);

      // override API Methods
      bool Init(EnvContext *pEnvContext, LPCTSTR initStr);
      bool InitRun(EnvContext *pEnvContext, bool useInitialSeed);
      bool Run(EnvContext *pContext);

   protected:
      // we'll add model code here as needed
   };



