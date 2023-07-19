// ExamplePlugin.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "FishPassage.h"
#include <PathManager.h>
#include <randgen/Randunif.hpp>
#include <misc.h>
#include <FDATAOBJ.H>
#include <PathManager.h>
#include <EnvConstants.h>
#include <format>

FishPassage* theModel = nullptr;

const float gMaxLengthGain = 100000.0f;    //  units?

// *************************************************

FishPassage::FishPassage()
   : EnvModelProcess()
      {
      //ASSERT(theModel == NULL);  // singleton
      theModel = this;
      }


FishPassage::~FishPassage()
   {
   //if (m_pOutputData != nullptr)
   //   delete m_pOutputData;
   }



bool FishPassage::Init(EnvContext* pEnvContext, LPCTSTR initStr)
   {
   m_pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   if (LoadXml(pEnvContext, initStr) == false)
      return false;

   // see https://geodataservices.wdfw.wa.gov/arcgis/rest/services/ApplicationServices/FP_Sites/MapServer/layers
   // for metadata

   // IDU columns
   CheckCol(m_pIDULayer, m_colIDU_FPIndex, "FP_INDEX", TYPE_INT, CC_MUST_EXIST);
   CheckCol(m_pIDULayer, m_colIDU_FPLinealGain, "FP_LIN_GN", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(m_pIDULayer, m_colIDU_FPAreaGain, "FP_AREA_GN", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(m_pIDULayer, m_colIDU_FPPriority, "FP_Priorit", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(m_pIDULayer, m_colIDU_Conserve, "Conserve", TYPE_INT, CC_MUST_EXIST);

   // Fish Passage layer columns
   //CheckCol(m_pFPLayer, m_colFP_FishUseCode, "FishUseCod", TYPE_INT, CC_MUST_EXIST);
   //CheckCol(m_pFPLayer, m_colFP_LinealGain, "LinealGain", TYPE_INT, CC_MUST_EXIST);
   CheckCol(m_pFPLayer, m_colFP_Priority, "R_PRIORITY", TYPE_FLOAT, CC_MUST_EXIST);
   //CheckCol(m_pFPLayer, m_colFP_OwnerTypeCode, "OwnerTypeC", TYPE_INT, CC_MUST_EXIST);
   CheckCol(m_pFPLayer, m_colFP_BarrierStatusCode, "FishPass_1", TYPE_INT, CC_MUST_EXIST);
   CheckCol(m_pFPLayer, m_colFP_BarrierType, "FishPassag", TYPE_INT, CC_MUST_EXIST);
   CheckCol(m_pFPLayer, m_colFP_Significant, "Significan", TYPE_INT, CC_MUST_EXIST);
  
   CheckCol(m_pFPLayer, m_colFP_IDUIndex, "IDU_Index", TYPE_INT, CC_MUST_EXIST);
   CheckCol(m_pFPLayer, m_colFP_StreamOrder, "StrOrder", TYPE_INT, CC_MUST_EXIST);
   CheckCol(m_pFPLayer, m_colFP_UpArea, "UpStrArea", TYPE_FLOAT, CC_MUST_EXIST);
   //CheckCol(m_pFPLayer, m_colFP_AreaGain, "AreaGain", TYPE_FLOAT, CC_MUST_EXIST);  // check units
   //CheckCol(m_pFPLayer, m_colFP_LengthGain, "LengthGain", TYPE_FLOAT, CC_MUST_EXIST);
  
   // WRONG!!!!  See above, need to fix data generation algorthm for area, lengths
   CheckCol(m_pFPLayer, m_colFP_AreaGain, "AreaSqKM", TYPE_FLOAT, CC_MUST_EXIST);  // check units
   CheckCol(m_pFPLayer, m_colFP_LengthGain, "LengthKM", TYPE_FLOAT, CC_MUST_EXIST);

   
   CheckCol(m_pFPLayer, m_colFP_Conserve, "Conserve", TYPE_INT, CC_AUTOADD);
   CheckCol(m_pFPLayer, m_colFP_Removed, "Removed", TYPE_INT, CC_AUTOADD);

   m_pFPLayer->SetColData(m_colFP_Conserve, VData(0), true);

   int maxOrder = 0;
   float maxArea = 0;
   float maxAreaGain = 0;
   float maxLengthGain = 0;

   // get scaling factors
   for (MapLayer::Iterator fpPt = m_pFPLayer->Begin(); fpPt != m_pFPLayer->End(); fpPt++)
      {
      int statusCode = 0, type = 0;
      float lengthGain = 0;
      m_pFPLayer->GetData(fpPt, m_colFP_BarrierStatusCode, statusCode);
      m_pFPLayer->GetData(fpPt, m_colFP_BarrierType, type);
      m_pFPLayer->GetData(fpPt, m_colFP_LengthGain, lengthGain);

      if (statusCode == 10 && type != 5 && lengthGain < gMaxLengthGain)  // is it a barrier?, 
         {
         int streamOrder = 0;
         float upArea = 0;
         float areaGain = 0;

         m_pFPLayer->GetData(fpPt, m_colFP_StreamOrder, streamOrder);
         m_pFPLayer->GetData(fpPt, m_colFP_UpArea, upArea);
         m_pFPLayer->GetData(fpPt, m_colFP_AreaGain, areaGain);
         m_pFPLayer->GetData(fpPt, m_colFP_LengthGain, lengthGain);

         if (streamOrder > maxOrder)
            maxOrder = streamOrder;

         if (upArea > maxArea)
            maxArea = upArea;

         if (areaGain > maxAreaGain)
            maxAreaGain = areaGain;

         if (lengthGain > maxLengthGain)
            maxLengthGain = lengthGain;
         }
      }

   // iterate through fish passage points, 
   for (MapLayer::Iterator fpPt = m_pFPLayer->Begin(); fpPt != m_pFPLayer->End(); fpPt++)
      {
      int statusCode=0, type=0;
      float lengthGain = 0;
      m_pFPLayer->GetData(fpPt, m_colFP_BarrierStatusCode, statusCode);
      m_pFPLayer->GetData(fpPt, m_colFP_BarrierType, type);
      m_pFPLayer->GetData(fpPt, m_colFP_LengthGain, lengthGain);

      int iduIndex = -1;
      m_pFPLayer->GetData(fpPt, m_colFP_IDUIndex, iduIndex);

      if (statusCode == 10 && type != 5 && lengthGain < gMaxLengthGain)  // is it a barrier?  and not a natural barrier?
         {
         this->m_nElgibleBarriers++;

         if (iduIndex >= 0) // does this FPpt contain an IDU Barrier point?
            {
            int streamOrder = 0;
            float upArea = 0;
            float areaGain = 0;
            int significant = 0;
            
            m_pFPLayer->GetData(fpPt, m_colFP_StreamOrder, streamOrder);
            m_pFPLayer->GetData(fpPt, m_colFP_UpArea, upArea);
            m_pFPLayer->GetData(fpPt, m_colFP_AreaGain, areaGain);
            m_pFPLayer->GetData(fpPt, m_colFP_Significant, significant);

            this->m_lengthElgibleBarriers += lengthGain;
            this->m_areaElgibleBarriers += areaGain;

            if (significant == 10)
               significant = 1;
            else
               significant = 0;

            float priority = 0.2f * ((float(streamOrder)/ maxOrder)) 
                           + 0.2f * (upArea / maxArea)
                           + 0.2f * (areaGain / maxAreaGain)
                           + 0.2f * (lengthGain / maxLengthGain)
                           + 0.2f * significant;

            m_pFPLayer->SetData(fpPt, m_colFP_Priority, priority);
            m_pIDULayer->SetData(iduIndex, m_colIDU_FPPriority, priority);
            }
         }// end  of: if ( statusCode == 10 and type != 5)
      else
         {
         m_pFPLayer->SetData(fpPt, m_colFP_Priority, 0);
         m_pIDULayer->SetData(iduIndex, m_colIDU_FPPriority, 0);
         }
      }

   AddOutputVar("FP Linear Gain (mi)", m_milesFPBarriersRemoved, "");
   AddOutputVar("FP Area Gain (mi2)", m_miles2FPBarriersRemoved, "");
   AddOutputVar("FP Removed Count", m_countFPBarriersRemoved, "");

   AddOutputVar("FP Linear Gain Fraction", m_fracFPBarriersRemovedMi, "");
   AddOutputVar("FP Area Gain Fraction", m_fracFPBarriersRemovedMi2, "");
   AddOutputVar("FP Removed Fraction", m_fracFPBarriersRemovedCount, "");

   this->m_countFPBarriersRemoved = 0;
   this->m_fracFPBarriersRemovedMi = 0;
   this->m_fracFPBarriersRemovedCount = 0;

   return true;
   }



bool FishPassage::InitRun(EnvContext* pEnvContext, bool useInitialSeed)
   {
   this->m_milesFPBarriersRemoved = 0;
   this->m_miles2FPBarriersRemoved = 0;
   this->m_countFPBarriersRemoved = 0;
   
   this->m_fracFPBarriersRemovedMi = 0;
   this->m_fracFPBarriersRemovedMi2 = 0;
   this->m_fracFPBarriersRemovedCount = 0;

   //m_pOutputData->ClearRows();
   return true;
   }



// runs after fish passage is allocated
bool FishPassage::Run(EnvContext* pEnvContext)
   {   
   m_milesFPBarriersRemoved = 0;
   m_miles2FPBarriersRemoved = 0;
   m_countFPBarriersRemoved = 0;

   m_fracFPBarriersRemovedMi = 0;
   m_fracFPBarriersRemovedMi2 = 0;
   m_fracFPBarriersRemovedCount = 0; 
   
   
   if (m_pFPLayer == nullptr)
      return false;

   for (MapLayer::Iterator fpPt = m_pFPLayer->Begin(); fpPt != m_pFPLayer->End(); fpPt++)
      {
      int statusCode = 0, type = 0;
      float lengthGain = 0;

      m_pFPLayer->GetData(fpPt, m_colFP_BarrierStatusCode, statusCode);
      m_pFPLayer->GetData(fpPt, m_colFP_BarrierType, type);
      m_pFPLayer->GetData(fpPt, m_colFP_LengthGain, lengthGain);

      //if (statusCode == 10 && type != 5)  // is it a barrier?
      if (lengthGain < gMaxLengthGain)
         {
         int conserve = 0;
         m_pFPLayer->GetData(fpPt, m_colFP_Conserve, conserve);

         if (conserve == 30)
            {
            float areaGain = 0;
            m_pFPLayer->GetData(fpPt, m_colFP_AreaGain, areaGain);

            m_milesFPBarriersRemoved += lengthGain;
            m_miles2FPBarriersRemoved += areaGain;
            m_countFPBarriersRemoved += 1;

            int iduIndex = 0;
            m_pFPLayer->GetData(fpPt, m_colFP_IDUIndex, iduIndex);

            m_pFPLayer->SetData(fpPt, m_colFP_Removed, 1);

            this->UpdateIDU(pEnvContext, iduIndex, m_colIDU_FPLinealGain, lengthGain);
            this->UpdateIDU(pEnvContext, iduIndex, m_colIDU_FPAreaGain, areaGain);
            this->UpdateIDU(pEnvContext, iduIndex, m_colIDU_Conserve, 30);
            }
         }
      }

   m_fracFPBarriersRemovedMi = m_milesFPBarriersRemoved/ m_lengthElgibleBarriers;  // km
   m_fracFPBarriersRemovedMi2 = m_miles2FPBarriersRemoved / m_areaElgibleBarriers; // km2
   m_fracFPBarriersRemovedCount = float(m_countFPBarriersRemoved) / m_nElgibleBarriers;

   // convert to mi, m2
   m_milesFPBarriersRemoved *= MI_PER_KM;
   m_miles2FPBarriersRemoved *= MI2_PER_KM2;

   //CollectData(pEnvContext->currentYear);
   CString outDir = PathManager::GetPath(PM_OUTPUT_DIR);
   CString filename;
   filename.Format("%sFishPassage/FP_NHD_V4_%i.shp", outDir, pEnvContext->currentYear);

   CString msg("Exporting map layer: ");
   msg += filename;
   Report::Log(msg);
   m_pFPLayer->SaveShapeFile(filename, false, 0, 1);

   msg.Format("Barriers Removed- Count:%i, Length Stream Opened: %.0fmi, Area Opened: %.0fmi2",
      m_countFPBarriersRemoved, m_milesFPBarriersRemoved, m_miles2FPBarriersRemoved);
   Report::Log(msg);

   return true;
   }


bool FishPassage::CollectData(int year) 
   {




   /*
   int fieldCount = (int) m_fields.GetSize();
   int fc = GetActiveFieldCount( theModel->m_id);

   float* data = new float[1+(2*fc)];

   data[0] = (float) year;
   int j = 1;
   for (int i = 0; i < fieldCount; i++)
      {
      if (IsFieldInModel( m_fields[i], theModel->m_id ))
         {
         data[j++] = (float)m_fields[i]->m_count;
         data[j++] = (float)m_fields[i]->m_totalArea;
         }
      }

   m_pOutputData->AppendRow(data, 1+(2*fc));

   delete [] data;  */
   return true;
   }

bool FishPassage::LoadXml(EnvContext *pEnvContext, LPCTSTR filename)
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

   LPTSTR fpLayer = NULL, streamLayer=NULL;
   int    shuffleIDUs = 1;
   XML_ATTR rattrs[] = { // attr               type           address       isReq checkCol
                      { "fish_passage_layer",  TYPE_STRING,   &fpLayer,       true, 0 },
                      //{ "stream_layer",        TYPE_STRING,   &streamLayer,   true, 0 },
                      { NULL,                  TYPE_NULL,     NULL,           false, 0 } };

   if (TiXmlGetAttributes(pXmlRoot, rattrs, path, NULL) == false)
      return false;

   if (fpLayer != nullptr)
      {
      this->m_fpLayerName = fpLayer;
      this->m_pFPLayer = m_pIDULayer->GetMapPtr()->GetLayer(fpLayer);
      }

   if (this->m_pFPLayer == nullptr)
      {
      std::string msg = std::format("Unable to find fish passage map layer {}", fpLayer);
      Report::ErrorMsg(msg.c_str());
      return false;
      }

   return true;
   }