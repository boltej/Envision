#pragma once

#ifdef _WINDOWS

#ifdef BUILD_ENVISION
#define ENVAPI

#elif BUILD_ENGINE
#define ENVAPI __declspec( dllexport )

#elif USE_ENGINE
#define ENVAPI __declspec( dllimport )

#else 
#define ENVAPI __declspec( dllimport )
#endif

#include <MAP.h>
#include <PtrArray.h>

class EnvModel;
class DELTA;
class DeltaArray;
class EnvContext;
class Policy;
class Scenario;
class EnvEvaluator;
class EnvModelProcess;

enum ENV_CP_FLAGS   // EnvCloseProject() flags;
   { ENVCP_PRESERVE_MAP=1 };

#ifdef __cplusplus
extern "C" {
#endif

   // High-level (Dynamic) interface functions
   typedef int (PASCAL *INITENGINEFN)(int);
   typedef EnvModel* (PASCAL *LOADPROJECTFN)(LPCTSTR, int);
   typedef int (PASCAL *RUNSCENARIOFN)(EnvModel*, int, int);
   typedef int (PASCAL *CLOSEPROJECTFN)(EnvModel*, int);
   typedef int (PASCAL *CLOSEENGINEFN)();

   ENVAPI int       PASCAL EnvInitEngine(int initFlags);
   ENVAPI EnvModel* PASCAL EnvLoadProject(LPCTSTR envxFile, int initFlags);
   ENVAPI int       PASCAL EnvRunScenario(EnvModel *pModel, int scenario, int simulationLength, int runFlags);  // scenario is zero-based, -1=run all
   ENVAPI int       PASCAL EnvCloseProject(EnvModel *pModel, int closeFlags);  // scenario is zero-based, -1=run all



   ENVAPI int       PASCAL EnvCloseEngine();

   ////////////////////////////////////////////////////////////////
   // class Map/MapLayer methods
   //ENVAPI bool GetIDUPath( CString &path )'
   /*
   ENVAPI int  PASCAL EnvGetLayerIndex(MapLayer*, LPCTSTR name);
   ENVAPI bool PASCAL EnvGetLayerName(MapLayer*, int index, LPTSTR name, int maxLength);
   */

   ENVAPI DELTA&  PASCAL EnvGetDelta(DeltaArray*, INT_PTR index);
   ENVAPI int     PASCAL EnvApplyDeltaArray(EnvModel*);
   /*
   ENVAPI bool PASCAL EnvGetDataFloat(EnvModel *pModel, int layer, int row, int col, float *value);
   ENVAPI bool PASCAL EnvGetDataInt(EnvModel *pModel, int layer, int row, int col, int *value);
   ENVAPI bool PASCAL EnvGetDataBool(EnvModel *pModel, int layer, int row, int col, bool *value);
   ENVAPI bool PASCAL EnvGetDataString(EnvModel *pModel, int layer, int row, int col, LPTSTR value, int maxLength);
   ENVAPI bool PASCAL EnvGetDataDouble(EnvModel *pModel, int layer, int row, int col, double *value);

   ENVAPI bool PASCAL EnvSetDataFloat(EnvModel *pModel, int layer, int row, int col, float value);
   ENVAPI bool PASCAL EnvSetDataInt(EnvModel *pModel, int layer, int row, int col, int value);
   ENVAPI bool PASCAL EnvSetDataBool(EnvModel *pModel, int layer, int row, int col, bool value);
   ENVAPI bool PASCAL EnvSetDataString(EnvModel *pModel, int layer, int row, int col, LPCTSTR value);
   ENVAPI bool PASCAL EnvSetDataDouble(EnvModel *pModel, int layer, int row, int col, double value);
   */
   // class EnvModel methods
   ENVAPI int             PASCAL EnvGetEvaluatorCount(EnvModel*);
   ENVAPI EnvEvaluator* PASCAL EnvGetEvaluatorInfo(EnvModel*, int i);
   ENVAPI EnvEvaluator* PASCAL EnvFindEvaluatorInfo(EnvModel*, LPCTSTR name);

   ENVAPI int               PASCAL EnvGetAutoProcessCount(EnvModel*);
   ENVAPI EnvModelProcess* PASCAL EnvGetAutoProcessInfo(EnvModel*, int i);

   ENVAPI int PASCAL EnvChangeIDUActor(EnvContext*, int idu, int actorGroup, bool randomize);

   // PolicyManager methods
   ENVAPI int     PASCAL EnvGetPolicyCount(EnvModel*);
   ENVAPI int     PASCAL EnvGetUsedPolicyCount(EnvModel*);
   ENVAPI Policy* PASCAL EnvGetPolicy(EnvModel*, int i);
   ENVAPI Policy* PASCAL EnvGetPolicyFromID(EnvModel*, int id);


   // ScenarioManager methods
   ENVAPI int        PASCAL EnvGetScenarioCount(EnvModel*);
   ENVAPI Scenario * PASCAL EnvGetScenario(EnvModel*, int i);
   ENVAPI Scenario * PASCAL EnvGetScenarioFromName(EnvModel*, LPCTSTR name, int *index);

   // Standard Path Information
   //ENVAPI int PASCAL EnvStandardOutputFilename(LPTSTR filename, LPCTSTR pathAndFilename, int maxLength);
   ENVAPI int PASCAL EnvCleanFileName(LPTSTR filename);

#ifdef __cplusplus
   }
#endif




#endif
