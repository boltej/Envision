#pragma once


/* -------------------------
Sequence:
EnvModel:Run()
   EnvModel::UpdateAppVars()
   EnvModel::RunAutoPre()
      AcuteHazards::Run()
         if (postevent)
            AcuteHazards::Update() -> if event has run, checks for damaged bldgs
                    that are to be repaired according to repair policy (BLDGDAMAGE < 0), and resets
                    them to a repaired state if repairYrs == 0.  For multiyear repairs, reduce repairYrs
                    by one.
         if (eventScheduled)
            AHEvent::Run() - runs model, writes repairCost, repairYrs, bldgDamage

         -> DeltaArray appled here

   EnvModel::RunGlobalConstraints() -> resets global constraints
   EnvModel::ApplyMandatoryPolicies() -> selects policies within constraints
------------------- */


#include <EnvExtension.h>
#include <PtrArray.h>
#include <VDataObj.h>
#include <randgen/Randln.hpp>    // lognormal
#include <randgen/RandNorm.hpp>  // normal
#include <randgen/RandUnif.hpp>  // normal

// this privides a basic class definition for this
// plug-in module.
// Note that we want to override the parent methods
// for Init, InitRun, and Run.

#include <python.h>

#define _EXPORT __declspec( dllexport )

class AcuteHazards;

enum AH_STATUS { AHS_PRE_EVENT, AHS_POST_EVENT };


class AHEvent
   {
   public:
      int m_year;
      CString m_name;
      CString m_pyFunction;
      CString m_pyModulePath;
      CString m_pyModuleName;
      CString m_envOutputPath;
      CString m_earthquakeInputPath;
      CString m_tsunamiInputPath;
      CString m_earthquakeScenario;
      CString m_tsunamiScenario;
      //CString m_pyTransModulePath;
      //CString m_pyTransModuleName;
      //CString m_pyTransFunction;

      AcuteHazards* m_pAHModel;

      int m_use;          // scenario variable

      AH_STATUS m_status;

      VDataObj m_earthquakeData;  // results table
      VDataObj m_tsunamiData;  // results table

      AHEvent()
         : m_year(-1)
         , m_status(AHS_PRE_EVENT)
         , m_pAHModel(nullptr)
         , m_use(1)
         , m_earthquakeData(U_UNDEFINED)
         , m_tsunamiData(U_UNDEFINED) {}

      bool Run(EnvContext* pEnvContext);
      bool Propagate(EnvContext* pEnvContext);
   };



struct HazDataColInfo
   {
   CString field;
   int col;
   };


// Model class for Acute Hazards
class _EXPORT AcuteHazards : public EnvModelProcess
   {
   friend class AHEvent;

   public:
      // constructor
      AcuteHazards(void);
      ~AcuteHazards(void);

      // override API Methods
      bool Init(EnvContext* pEnvContext, LPCTSTR initStr);
      bool InitRun(EnvContext* pEnvContext, bool useInitialSeed);
      bool Run(EnvContext* pContext);

   protected:
      // we'll add model code here as needed

      void UpdateBldgType(EnvContext*, bool useDelta);
      bool InitPython();
      bool Update(EnvContext* pEnvContext);

      bool LoadXml(EnvContext*, LPCTSTR filename);

      // member data (from XML)
      CString m_pythonPath;

      PtrArray<AHEvent> m_events;

      RandLogNormal m_randLogNormal;
      RandNormal m_randNormal;
      RandUniform m_randUniform;

      int m_colBldgType = -1;
      int m_colIduImprValue = -1;     // IMPR_VALUE
      int m_colIduRepairYrs = -1;     // REPAIR_YRS   - years rquired to repair building, or -1 if not being restored
      int m_colIduBldgStatus = -1;    // BLDGSTATUS  - 0=normal, 1=being restored, 2=uninhabitable
      int m_colIduRepairCost = -1;    // REPAIRCOST  - multiples of $1000
      int m_colIduHabitable = -1;     // HABITABLE    - 0 means building is not habitable and 1 means it can be used
      int m_colIduBldgDamage = -1;    // BLDGDAMAGE  - combined damage_state - 1 means no damage; 2 - slight damage; 3 - moderate damage; 4 - extensive damage; and 5 is complete damage.
      int m_colIduBldgDamageEq = -1;  // BLDGDMGEQ  - earthquake damage_state - 1 means no damage; 2 - slight damage; 3 - moderate damage; 4 - extensive damage; and 5 is complete damage.
      int m_colIduBldgDamageTsu = -1; // BLDGDMGTS  - tsunami damage_state - 1 means no damage; 2 - slight damage; 3 - moderate damage; 4 - extensive damage; and 5 is complete damage.
      int m_colIduRemoved = -1;       // REMOVED - buildings removed
      //int m_colBldRestYr;   // BD_REST_YR
      int m_colIduCasualties = -1;
      int m_colIduInjuries = -1;
      int m_colIduFatalities = -1;

      // exposed outputs
      //float m_annualRepairCosts;

      int m_nUninhabitableStructures;
      int m_nDamaged;
      int m_nDamaged2;
      int m_nDamaged3;
      int m_nDamaged4;
      int m_nDamaged5;
      int m_bldgsRepaired;
      int m_bldgsBeingRepaired;

      int m_nInhabitableStructures;
      float m_pctInhabitableStructures;
      int m_totalBldgs;
      int m_nFunctionalBldgs;
      float m_pctFunctionalBldgs;

      // life safety
      //float m_numCasSev1;
      //float m_numCasSev2;
      //float m_numCasSev3;
      //float m_numCasSev4;
      //float m_numCasTotal;
      float m_numCasualities = 0;
      float m_numFatalities = 0;
      float m_numInjuries = 0;
   };
