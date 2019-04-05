#pragma once


#include <EnvExtension.h>
#include <PtrArray.h>
#include <FDATAOBJ.H>
#include <randgen/Randln.hpp>    // lognormal
#include <randgen/RandNorm.hpp>  // normal
#include <randgen/RandUnif.hpp>  // normal

// this privides a basic class definition for this
// plug-in module.
// Note that we want to override the parent methods
// for Init, InitRun, and Run.



#define _EXPORT __declspec( dllexport )

class AcuteHazards;

enum AH_STATUS { AHS_PRE_EVENT, AHS_POST_EVENT};


class AHEvent
   {
   public:
      int m_year;
      CString m_name;
      CString m_pyFunction;
      CString m_pyModulePath;
      CString m_pyModuleName;
      CString m_envOutputPath;
      CString m_envInputPath;
      CString m_hazardScenario;

      AcuteHazards *m_pAHModel;

      int m_use;          // scenario variable

      AH_STATUS m_status;

      FDataObj m_hazData;  // results table

      AHEvent() : m_status(AHS_PRE_EVENT), m_pAHModel(nullptr) {}

      bool Run(EnvContext *pEnvContext);
      bool Propagate(EnvContext *pEnvContext);
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
      bool Init(EnvContext *pEnvContext, LPCTSTR initStr);
      bool InitRun(EnvContext *pEnvContext, bool useInitialSeed);
      bool Run(EnvContext *pContext);

   protected:
      // we'll add model code here as needed
      bool InitPython();
      bool Update(EnvContext *pEnvContext);

      bool LoadXml(EnvContext*, LPCTSTR filename);

      // member data (from XML)
      CString m_pythonPath;
            
      PtrArray<AHEvent> m_events;

      RandLogNormal m_randLogNormal;
      RandNormal m_randNormal;
      RandUniform m_randUniform;

      int m_colIduImprValue;     // IMPR_VALUE
      int m_colIduRepairYrs;     // REPAIR_YRS   - years rquired to repair building, or -1 if not being restored
      int m_colIduBldgStatus;    // BLDGSTATUS  - 0=normal, 1=being restored, 2=uninhabitable
      int m_colIduRepairCost;    // REPAIRCOST  - multiples of $1000
      int m_colIduHabitable;     // HABITABLE    - 0 means building is not habitable and 1 means it can be used
      int m_colIduBldgDamage;    // BLDGDAMAGE  - damage_state - 1 means no damage; 2 - slight damage; 3 - moderate damage; 4 - extensive damage; and 5 is complete damage.

      //int m_colBldRestYr;   // BD_REST_YR

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
   };

extern "C" _EXPORT EnvExtension* Factory() { return (EnvExtension*) new AcuteHazards; }
