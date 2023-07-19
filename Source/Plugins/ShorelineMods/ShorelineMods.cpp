// ExamplePlugin.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ShorelineMods.h"
#include <PathManager.h>
#include <randgen/Randunif.hpp>
#include <misc.h>
#include <FDATAOBJ.H>
#include <PathManager.h>
#include <EnvConstants.h>
#include <format>

ShorelineMods* theModel = nullptr;


// *************************************************

ShorelineMods::ShorelineMods()
   : EnvModelProcess()
      {
      //ASSERT(theModel == NULL);  // singleton
      theModel = this;
      }


ShorelineMods::~ShorelineMods()
   {
   //if (m_pOutputData != nullptr)
   //   delete m_pOutputData;
   }



bool ShorelineMods::Init(EnvContext* pEnvContext, LPCTSTR initStr)
   {
   m_pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   if (LoadXml(pEnvContext, initStr) == false)
      return false;

   // IDU columns
   //CheckCol(m_pIDULayer, m_colIDU_SAIndex, "FP_INDEX", TYPE_INT, CC_MUST_EXIST);
   CheckCol(m_pIDULayer, m_colIDU_SALength, "SA_REM_MI", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(m_pIDULayer, m_colIDU_Conserve, "Conserve", TYPE_INT, CC_MUST_EXIST);

   // Fish Passage layer columns
   CheckCol(m_pArmorLayer, m_colSA_IDUIndex, "IDU_Index", TYPE_INT, CC_MUST_EXIST);
   CheckCol(m_pArmorLayer, m_colSA_Length, "Length", TYPE_FLOAT, CC_MUST_EXIST);
   CheckCol(m_pArmorLayer, m_colSA_Conserve, "Conserve", TYPE_INT, CC_AUTOADD);
   CheckCol(m_pArmorLayer, m_colSA_Removed, "Removed", TYPE_INT, CC_AUTOADD);

   m_pArmorLayer->SetColData(m_colSA_Conserve, VData(0), true);

   this->m_milesEligibleArmor = 0;

   // get scaling factors
   for (MapLayer::Iterator i=m_pArmorLayer->Begin(); i != m_pArmorLayer->End(); i++)
      {
      float length = 0;
      m_pArmorLayer->GetData(i, this->m_colSA_Length, length);   // units=meters
      
      this->m_milesEligibleArmor += length;
      }

   this->m_milesEligibleArmor  *= MI_PER_M;

   AddOutputVar("Armor Removed (mi)", m_milesArmorRemoved, "");
   AddOutputVar("Armor Removed Fraction", m_fracArmorRemoved, "");

   return true;
   }

bool ShorelineMods::InitRun(EnvContext* pEnvContext, bool useInitialSeed)
   {
   return true;
   }



// runs after fish passage is allocated
bool ShorelineMods::Run(EnvContext* pEnvContext)
   {   
   this->m_milesArmorRemoved = 0;
   
   if (m_pArmorLayer == nullptr)
      return false;

   for (MapLayer::Iterator i = m_pArmorLayer->Begin(); i != m_pArmorLayer->End(); i++)
      {
      int conserve = 0;
      m_pArmorLayer->GetData(i, m_colSA_Conserve, conserve);
      if (conserve == 45)
         {
         float length = 0;
         m_pArmorLayer->GetData(i, this->m_colSA_Length, length);

         this->m_milesArmorRemoved += length;

         int iduIndex = 0;
         m_pArmorLayer->GetData(i, m_colSA_IDUIndex, iduIndex);
         m_pArmorLayer->SetData(i, m_colSA_Removed, 1);

         this->UpdateIDU(pEnvContext, iduIndex, m_colIDU_SALength, length);
         this->UpdateIDU(pEnvContext, iduIndex, m_colIDU_Conserve, 45);
         }
      }

   // convert to mi, m2
   this->m_milesArmorRemoved *= MI_PER_KM;
   this->m_fracArmorRemoved = this->m_milesArmorRemoved / this->m_milesEligibleArmor;   // note: haven converted to MILES yet

   CString outDir = PathManager::GetPath(PM_OUTPUT_DIR);
   CString filename;
   filename.Format("%sShorelineArmor/BS_ShorelineArmor_%i.shp", outDir, pEnvContext->currentYear);

   CString msg("Exporting map layer: ");
   msg += filename;
   Report::Log(msg);
   m_pArmorLayer->SaveShapeFile(filename, false, 0, 1);
     
   msg.Format("Armor Removed - Length: %.0fmi of %.0fmi", this->m_milesArmorRemoved, this->m_milesEligibleArmor);
   Report::Log(msg);

   return true;
   }



bool ShorelineMods::LoadXml(EnvContext *pEnvContext, LPCTSTR filename)
   {
   // search for file along path
   CString path;
   int result = PathManager::FindPath(filename, path);  // finds first full path that contains passed in path if relative or filename, return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 

   if (result < 0)
      {
      CString msg("Unable to find input file ");
      msg += filename;
      Report::ErrorMsg(msg);
      return false;
      }

   TiXmlDocument doc;
   bool ok = doc.LoadFile(path);

   if (!ok)
      {
      Report::ErrorMsg(doc.ErrorDesc());
      return false;
      }

   CString msg("Loading input file ");
   msg += path;
   Report::Log(msg);

   // start interating through the nodes
   TiXmlElement* pXmlRoot = doc.RootElement();  // field_calculator

   LPTSTR mapLayer = NULL;
   int    shuffleIDUs = 1;
   XML_ATTR rattrs[] = { // attr               type           address       isReq checkCol
                      { "armor_layer",  TYPE_STRING,   &mapLayer,      true, 0 },
                      { NULL,           TYPE_NULL,     NULL,           false, 0 } };

   if (TiXmlGetAttributes(pXmlRoot, rattrs, path, NULL) == false)
      return false;

   if (mapLayer != nullptr)
      {
      MapLayer* m_pArmorLayer = nullptr;

      this->m_armorLayerName = mapLayer;
      this->m_pArmorLayer = m_pIDULayer->GetMapPtr()->GetLayer(mapLayer);
      }

   if (this->m_pArmorLayer == nullptr)
      {
      std::string msg = std::format("Unable to find armor map layer {}", mapLayer);
      Report::ErrorMsg(msg.c_str());
      return false;
      }

   return true;
   }