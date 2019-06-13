// LPJGuess.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#pragma hdrstop

#include "LPJGuess.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


extern "C" _EXPORT EnvExtension* Factory() { return (EnvExtension*) new LPJGuess; }


// constructor
LPJGuess::~LPJGuess(void)
   {
   }

// override API Methods
bool LPJGuess::Init(EnvContext *pEnvContext, LPCTSTR initStr)
   {
   return TRUE;
   }

bool LPJGuess::InitRun(EnvContext *pEnvContext, bool useInitialSeed)
   {
   return TRUE;
   }

bool LPJGuess::Run(EnvContext *pContext)
   {
   return TRUE;
   }
