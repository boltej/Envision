#pragma once

#include <EnvExtension.h>

#include <VdataObj.h>
#include <FdataObj.h>
#include <MovingWindow.h>

//#include <PtrArray.h>

#define _EXPORT __declspec( dllexport )

class _EXPORT Risk : public  EnvEvaluator
   {
   public:
      Risk() : EnvEvaluator()
         , m_movingWindowPotentialLoss(5)
         , m_movingWindowActualLoss(5)
         {}

      ~Risk() { if (m_pOutputData) delete m_pOutputData; }

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
      int m_colArea = -1;
      int m_colFlameLen = -1;
      int m_colPFlameLen = -1;

      int m_colPotentialLossPerHa = -1;
      int m_colActualLossPerHa = -1;
      int m_colRisk = -1;

      int m_colActualExpHousingLoss = -1;
      int m_colPotentialExpHousingLoss = -1;

      int m_colDamageHousing = -1;
      int m_colPotentialDamageHousing = -1;
      int m_colDamageFracHousing = -1;

      int m_colDamageTimber = -1;
      int m_colPotentialDamageTimber = -1;
      int m_colDamageFracTimber = -1;

      int m_colLookupHousing = -1;
      int m_colLookupTimber = -1;

      int m_colNDU = -1;
      int m_colImprVal = -1;
      int m_colFirewise = -1;
      int m_colTimberVol = -1;

      float m_potentialWt = 0.07f;
      float m_timberValueMBF = 10;  // value/THOUSAND BOARD FEET
            
      float m_firewiseReductionFactor = 0.15f;
      float m_flamelenThreshold = 0.01f;

      float m_maxDamagePotentialPerHa = 1000000;
      float m_maxDamageActualPerHa = 1000000;

      CString m_queryStr;
      Query* pQuery = nullptr;

      CString m_lossTableFileHousing;
      CString m_lossTableFileTimber;

      VDataObj* m_pLossTableHousing = nullptr;
      VDataObj* m_pLossTableTimber = nullptr;
      CMap<int, int, int, int> m_luMapHousing; // key=lulc, value=index
      CMap<int, int, int, int> m_luMapTimber; // key=lulc, value=index

      FDataObj* m_pOutputData = nullptr;

      //MovingWindow m_movingWindowActualHousingLoss;
      //MovingWindow m_movingWindowActualTimberLoss;

      //MovingWindow m_movingWindowPotentialHousingLoss;
      //MovingWindow m_movingWindowPotentialTimberLoss;

      MovingWindow m_movingWindowPotentialLoss;
      MovingWindow m_movingWindowActualLoss;
      //MovingWindow m_movingWindowCombinedRisk;


      PtrArray<MovingWindow> iduMovingWindowsPotentialLoss;
      PtrArray<MovingWindow> iduMovingWindowsActualLoss;


      // Methods 

      bool LoadXml(EnvContext* pEnvContext, LPCTSTR filename);

      void GetLoss(MapLayer*, float flameLen, int lu, bool doTimber, int firewise, float& lossFrac);
      void ToPercentiles(std::vector<float>& values, std::vector<float>& pctiles);
   };


extern "C" _EXPORT EnvExtension * Factory(EnvContext*) { return (EnvExtension*) new Risk; }

