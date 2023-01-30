// ExamplePlugin.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "FishPassage.h"
#include <PathManager.h>
#include <randgen/Randunif.hpp>
#include <misc.h>
#include <FDATAOBJ.H>

#include <format>

FishPassage* theModel = nullptr;




// *************************************************

FishPassage::FishPassage()
   : EnvModelProcess()
      {
      //ASSERT(theModel == NULL);  // singleton
      theModel = this;
      }


FishPassage::~FishPassage()
   {
   if (m_pOutputData != nullptr)
      delete m_pOutputData;
   }



bool FishPassage::Init(EnvContext* pEnvContext, LPCTSTR initStr)
   {
   m_pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   if (LoadXml(pEnvContext, initStr) == false)
      return false;

   // see https://geodataservices.wdfw.wa.gov/arcgis/rest/services/ApplicationServices/FP_Sites/MapServer/layers
   // for metadata
   CheckCol(m_pIDULayer, m_colIDU_FPIndex, "FP_INDEX", TYPE_INT, CC_MUST_EXIST);
   CheckCol(m_pIDULayer, m_colIDU_FPLinGain, "FP_LinGain", TYPE_INT, CC_AUTOADD);
   CheckCol(m_pIDULayer, m_colIDU_FPPriority, "FP_Priority", TYPE_INT, CC_AUTOADD);

   CheckCol(m_pFPLayer, m_colFP_FishUseCode, "FishUseCod", TYPE_INT, CC_MUST_EXIST);
   CheckCol(m_pFPLayer, m_colFP_LinealGain, "LinealGain", TYPE_INT, CC_MUST_EXIST);
   CheckCol(m_pFPLayer, m_colFP_PriorityIndex, "PriorityIn", TYPE_FLOAT, CC_MUST_EXIST);
   CheckCol(m_pFPLayer, m_colFP_OwnerTypeCode, "OwnerTypeC", TYPE_INT, CC_MUST_EXIST);
   CheckCol(m_pFPLayer, m_colFP_FPBarrierStatusCode, "FishPassag", TYPE_INT, CC_MUST_EXIST);
  
   CheckCol(m_pFPLayer, m_colFP_IDUIndex, "IDUIndex", TYPE_INT, CC_AUTOADD);
   m_pFPLayer->SetColData(m_colFP_IDUIndex, VData(-1), true);

   // copy lineal gain, priority values from FP layer to IDUs and update IDUIndex
   for (MapLayer::Iterator idu = m_pIDULayer->Begin(); idu != m_pIDULayer->End(); idu++)
      {
      int fpIndex = -1;
      m_pIDULayer->GetData(idu, m_colIDU_FPIndex, fpIndex);

      if (fpIndex >= 0) // does this IDU contain a FP Barrier point?
         {
         int linGain = 0;
         m_pFPLayer->GetData(fpIndex, m_colFP_LinealGain, linGain);
         m_pIDULayer->SetData(idu, m_colIDU_FPLinGain, linGain);

         float priority = 0;
         m_pFPLayer->GetData(fpIndex, m_colFP_PriorityIndex, priority);
         m_pIDULayer->SetData(idu, m_colIDU_FPPriority, priority);

         // set FP Layer idu index while we are at it
         m_pFPLayer->SetData(fpIndex, m_colFP_IDUIndex, (int)idu);

         if (linGain > 0)
            {
            m_milesFPBarriersInit += linGain * MI_PER_M;
            m_countFPBarriersInit += 1;
            }
         }
      else
         m_pIDULayer->SetData(idu, m_colIDU_FPLinGain, 0);
      }

   // for each fish passage restriction, locate in IDU.
   int cols = 2;

   m_pOutputData = new FDataObj(cols, 0);
   m_pOutputData->SetLabel(0, "Year");

   AddOutputVar("FP Linear Gain (mi)", m_milesFPBarriersRemoved, "");
   AddOutputVar("FP Removed Count", m_countFPBarriersRemoved, "");

   AddOutputVar("FP Linear Gain Fraction", m_fracFPBarriersRemovedMi, "");
   AddOutputVar("FP Removed Fraction", m_fracFPBarriersRemovedCount, "");

   int m_countFPBarriersRemoved = 0;
   float m_fracFPBarriersRemovedMi = 0;
   float m_fracFPBarriersRemovedCount = 0;

   return true;
   }



bool FishPassage::InitRun(EnvContext* pEnvContext, bool useInitialSeed)
   {
   m_milesFPBarriersRemoved = 0;
   m_countFPBarriersRemoved = 0;
   m_fracFPBarriersRemovedMi = 0;
   m_fracFPBarriersRemovedCount = 0;

   m_pOutputData->ClearRows();
   return true;
   }



// runs after fish passage is allocated
bool FishPassage::Run(EnvContext* pEnvContext)
   {   
   if (m_pFPLayer == nullptr)
      return false;

   DeltaArray* deltaArray = pEnvContext->pDeltaArray;

   // basic idea - after allocator has allocated Fish Passage Barrier Removal (CONSERVE=30)
   // we look through the delta array for any where that occured in the last timestep.
   // when found, we: 
   //   1) check to see if we've overwrote a prior conservation action, and if so, restore the original tag,
   //   2) set the FP_PRIORITY IDU field to -30 (to prevent further allocations)
   //   3) Update the FP coverage FISHPASSAG (fish passage barrier status code) to a value of 30

   if (deltaArray != NULL)
      {
      INT_PTR size = deltaArray->GetSize();
      if (size > 0)
         {
         for (INT_PTR i = pEnvContext->firstUnseenDelta; i < size; i++)
            {
            DELTA& delta = ::EnvGetDelta(deltaArray, i);
            if (delta.col == m_colIDU_Conserve)
               {
               int newConserve = delta.newValue.GetInt();
               int oldConserve = delta.oldValue.GetInt();

               if (newConserve == 30) // fish passage
                  {
                  if (oldConserve > 0)
                     UpdateIDU(pEnvContext, delta.cell, m_colIDU_Conserve, oldConserve, ADD_DELTA);

                  UpdateIDU(pEnvContext, delta.cell, m_colIDU_FPPriority, -30.0f);

                  int fpIndex = -1;
                  m_pIDULayer->GetData(delta.cell, m_colIDU_FPIndex, fpIndex);
                  ASSERT(fpIndex >= 0);
                  m_pFPLayer->SetData(fpIndex, m_colFP_FPBarrierStatusCode, 30);

                  float linealGain = 0;
                  m_pFPLayer->GetData(fpIndex, m_colFP_LinealGain, linealGain);

                  if (linealGain > 0)
                     {
                     m_milesFPBarriersRemoved += linealGain * MI_PER_M;
                     m_countFPBarriersRemoved += 1;

                     m_fracFPBarriersRemovedCount = float(m_countFPBarriersRemoved) / m_countFPBarriersInit;
                     m_fracFPBarriersRemovedMi = m_milesFPBarriersRemoved / m_milesFPBarriersInit;
                     }
                  }
               }
            }
         }
      }








   // go through IDU's building network nodes/links for IDU's that pass the mapping query
   //for (MapLayer::Iterator idu = m_pIDULayer->Begin(); idu != m_pIDULayer->End(); idu++)
   //for (MapLayer::Iterator i = m_pFPLayer->Begin(); i != m_pFPLayer->End(); i++)
   //   {
   //   int iduIndex = -1;
   //   m_pFPLayer->GetData(i, m_colFP_IDUIndex, iduIndex);
   //
   //   if (iduIndex >= 0)
   //      {
   //      // was this IDU tagged for restoration/conservation?
   //      int conserver = 0;
   //
   //      }
   //   }

   
   CollectData(pEnvContext->currentYear);

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


   
   //this->m_pStreamLayer = pEnvContext->pMapLayer->GetMapPtr()->GetLayer(streamLayer);
   //if (this->m_pFPLayer == NULL)
   //   {
   //   std::string msg = std::format("Unable to find stream map layer {}", fpLayer);
   //   Report::ErrorMsg(msg.c_str());
   //   return false;
   //   }

   return true;
   }