#pragma once

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

//#include <python.h>

#define _EXPORT __declspec( dllexport )

class FloodDamage;


struct HazDataColInfo
   {
   CString field;
   int col;
   };


// Model class for Acute Hazards
class _EXPORT FloodDamage : public EnvModelProcess
   {
   friend class AHEvent;

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
      int m_colIduNDUs = -1;

      // exposed outputs
      //float m_annualRepairCosts;

      int m_nUninhabitableStructures = 0;
      //int m_nDUsDamaged = 0;
      //int m_nDUsDamaged1 = 0;
      //int m_nDUsDamaged2 = 0;
      //int m_nDUsDamaged3 = 0;
      //int m_nDUsDamaged4 = 0;
      //int m_nDUsDamagedAndRepaired = 0;
      //int m_nDUsRepaired = 0;
      //int m_nDUsBeingRepaired = 0;

      int m_nIDUsDamaged = 0;
      int m_nIDUsDamaged1 = 0;
      int m_nIDUsDamaged2 = 0;
      int m_nIDUsDamaged3 = 0;
      int m_nIDUsDamaged4 = 0;
      int m_nIDUsDamagedAndRepaired = 0;
      int m_nIDUsRepaired = 0;
      int m_nIDUsBeingRepaired = 0;

      int m_nInhabitableStructures = 0;
      float m_pctInhabitableStructures = 0;
      int m_totalBldgs = 0;
      int m_nFunctionalBldgs = 0;
      float m_pctFunctionalBldgs = 0;

      // life safety
      //float m_numCasSev1;
      //float m_numCasSev2;
      //float m_numCasSev3;
      //float m_numCasSev4;
      //float m_numCasTotal;


      float m_numCasualitiesEQ = 0;   //Casualties (EQ)",   "");
      float m_numFatalitiesEQ = 0;    //Fatalities (EQ)",  "");
      float m_numInjuriesEQ = 0;      //Injuries (EQ)",    );
      float m_numCasualitiesTSU = 0;  //Casualties (TSU)", , "");
      float m_numFatalitiesTSU = 0;   //Fatalities (TSU)",  "");
      float m_numInjuriesTSU = 0;     //Injuries (TSU)",   ");
      float m_numCasualities = 0;
      float m_numFatalities = 0;
      float m_numInjuries = 0;
   };
