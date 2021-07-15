#pragma once

#include <EnvExtension.h>
#include <FDataObj.h>
#include <string>

using namespace std;


#define _EXPORT __declspec( dllexport )

struct SZCOL {
   int index;
   string label;   
   };

const int SZCOLS = 45;
SZCOL szCols[] =
   {
   {0,"Time (Years)"},
   {1,"Stability Class A (mi)"},
   {2,"Stability Class E (mi)"},
   {3,"Stability Class S (mi)"},
   {4,"Pct Shoreline Modified"},
   {5,"Pct Primary Mod"},
   {6,"Pct Secondary Mod"},
   {7,"Pct Tertiary Mod"},
   {8,"BoatRamp (mi)"},
   {9,"Conc Bulkhead (mi)"},
   {10,"Wood Bulkhead (mi)"},
   {11,"Landfill (mi)"},
   {12,"Sheet Pile (mi)"},
   {13,"RipRap (mi)"},
   {14,"Ramps (#)"},
   {15,"Piers/Docks (#)"},
   {16,"Small Slips (#)"},
   {17,"Large Slips (#)"},
   {18,"Railways (mi)"},
   {19,"Pct Riparian"},
   {20,"Riparian (mi)"},
   {21,"Intertidal Area (mi2)"},
   {22,"Verrucaria (mi)"},
   {23,"Dune Grasses (mi)"},
   {24,"Sedge (mi)"},
   {25,"Salt-Tolerant Assm (mi)"},
   {26,"Salicornia (mi)"},
   {27,"Spartina (mi)"},
   {28,"Fucus (mi)"},
   {29,"Common Barnacle (mi)"},
   {30,"Blue Mussel (mi)"},
   {31,"Ulva (mi)"},
   {32,"Gracilaria (mi)"},
   {33,"Oysters (mi)"},
   {34,"Cal Mussel (mi)"},
   {35,"Red Algae (mi)"},
   {36,"Burrowing Shrimp (mi)"},
   {37,"Soft Brown Kelps (mi)"},
   {38,"Sargassum (mi)"},
   {39,"Chocolate Brown Helps (mi)"},
   {40,"Green Surfgrass (mi)"},
   {41,"Densraster (mi)"},
   {42,"Eelgrass (mi)"},
   {43,"Bull Kelp (mi)"},
   {44,"Macrocytis Kelp (mi)"}
   };


   class _EXPORT ShoreZone : public  EnvModelProcess
      {
      public:
         ShoreZone()
            : EnvModelProcess()
            , m_dataObj(SZCOLS, 0, U_UNDEFINED)
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
         FDataObj m_dataObj;

         int  m_colChngType;
         int  m_colHabCalc;
         int  m_colSmTotPct;
         int  m_colSm1Pct;
         int  m_colSm2Pct;
         int  m_colSm3Pct;
         int  m_colSm1Ft;
         int  m_colSm2Ft;
         int  m_colSm3Ft;
         int  m_colSm1Type;
         int  m_colSm2Type;
         int  m_colSm3Type;
         int  m_colRamp;
         int  m_colPierDock;
         int  m_colSlipSm;
         int  m_colSlipLg;
         int  m_colRail;
         int  m_colRipPct;
         int  m_colRipFt;
         int  m_colItzWidth;
         int  m_colVerUnit;
         int  m_colGraUnit;
         int  m_colSedUnit;
         int  m_colTriUnit;
         int  m_colSalUnit;
         int  m_colSpaUnit;
         int  m_colFucUnit;
         int  m_colBarUnit;
         int  m_colBmuUnit;
         int  m_colUlvUnit;
         int  m_colGcaUnit;
         int  m_colOysUnit;
         int  m_colMusUnit;
         int  m_colRedUnit;
         int  m_colCalUnit;
         int  m_colSbrUnit;
         int  m_colSarUnit;
         int  m_colChbUnit;
         int  m_colSurUnit;
         int  m_colDenUnit;
         int  m_colZosUnit;
         int  m_colNerUnit;
         int  m_colMacUnit;

      };
   
   
   extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new ShoreZone; }
