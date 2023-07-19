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




class _EXPORT ShorelineMods : public  EnvModelProcess
   {
   public:
      ShorelineMods();
      ~ShorelineMods();

      virtual bool Init(EnvContext* pEnvContext, LPCTSTR initStr);

      virtual bool InitRun(EnvContext* pEnvContext, bool useInitialSeed);
      
      virtual bool Run(EnvContext* pEnvContext);

   public:
      MapLayer* m_pIDULayer = nullptr;
      MapLayer* m_pArmorLayer = nullptr;
     
   protected:
      std::string m_armorLayerName;
      
      int m_colIDU_SAIndex= -1;
      int m_colIDU_SALength = -1;
      int m_colIDU_Conserve = -1;

      int m_colSA_IDUIndex= -1;
      int m_colSA_Length = -1;
      int m_colSA_Conserve = -1;
      int m_colSA_Removed = -1;

      float m_milesEligibleArmor = 0;

      // output variables
      float m_milesArmorRemoved = 0;
      float m_fracArmorRemoved = 0;
      bool LoadXml(EnvContext*, LPCTSTR);
   };


extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new ShorelineMods; }

