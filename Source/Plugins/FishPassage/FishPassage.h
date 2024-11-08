#pragma once

#include <EnvExtension.h>

#include <MapLayer.h>
#include <Typedefs.h>
#include <VData.h>
#include <queryengine.h>
#include <tixml.h>
#include <MapExprEngine.h>
#include <PtrArray.h>


#define _EXPORT __declspec( dllexport )

//class RandUniform;




class _EXPORT FishPassage : public  EnvModelProcess
   {
   public:
      FishPassage();
      ~FishPassage();

      virtual bool Init(EnvContext* pEnvContext, LPCTSTR initStr);

      virtual bool InitRun(EnvContext* pEnvContext, bool useInitialSeed);
      
      virtual bool Run(EnvContext* pEnvContext);


      //virtual bool EndRun(EnvContext *pContext) { return true; }
      //virtual bool Setup(EnvContext *pContext, HWND hWnd) { return false; }
      //virtual bool GetConfig(std::string &configStr) { return false; }
      //virtual bool SetConfig(EnvContext*, LPCTSTR configStr) { return false; }
      //virtual int  InputVar(int id, MODEL_VAR** modelVar);
      //virtual int  OutputVar(int id, MODEL_VAR** modelVar);

   public:
      MapLayer* m_pIDULayer = nullptr;
      MapLayer* m_pFPLayer = nullptr;
      //MapLayer* m_pStreamLayer = nullptr;

   protected:
      static bool m_initialized;
      static int m_colArea;

      std::string m_fpLayerName;
      
      int m_colIDU_FPIndex= -1;
      int m_colIDU_FPLinealGain = -1;
      int m_colIDU_FPAreaGain = -1;
      int m_colIDU_FPPriority= -1;
      int m_colIDU_Conserve = -1;

      int m_colFP_IDUIndex = -1;
      int m_colFP_StreamOrder = -1;
      int m_colFP_UpArea = -1;
      int m_colFP_AreaGain = -1;
      int m_colFP_LengthGain = -1;
      int m_colFP_Conserve = -1;
      int m_colFP_Removed = -1;
      int m_colFP_Priority = -1;
      int m_colFP_Significant = -1;
      int m_colFP_BarrierStatusCode = -1;
      int m_colFP_BarrierType = -1;

      //int m_colFP_FishUseCode = -1;
      //int m_colFP_LinealGain = -1;
      //int m_colFP_OwnerTypeCode = -1;




      int m_nElgibleBarriers = 0;
      float m_areaElgibleBarriers = 0;
      float m_lengthElgibleBarriers = 0;

      // output variables
      float m_milesFPBarriersRemoved = 0;
      float m_miles2FPBarriersRemoved = 0;
      int m_countFPBarriersRemoved = 0;

      float m_fracFPBarriersRemovedMi = 0;
      float m_fracFPBarriersRemovedMi2 = 0;
      float m_fracFPBarriersRemovedCount = 0;
      
      //float m_milesFPBarriersInit = 0;
      //int m_countFPBarriersInit = 0;

      FDataObj* m_pOutputData;

      bool CollectData(int year);
      bool LoadXml(EnvContext*, LPCTSTR);

   };


extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new FishPassage; }

