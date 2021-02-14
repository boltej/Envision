/*
This file is part of Envision.

Envision is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Envision is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Envision.  If not, see <http://www.gnu.org/licenses/>

Copywrite 2012 - Oregon State University

*/
#include "StdAfx.h"

#pragma hdrstop

#include "FarmModel.h"
#include "F2R.h"
#include <Maplayer.h>
#include <tixml.h>
#include <UNITCONV.H>
#include <misc.h>
#include <DATE.HPP>
#include <MAP.h>
#include <omp.h>

#include <randgen\Randunif.hpp>
#include <LULCTREE.H>
#include <FDATAOBJ.H>
#include <PathManager.h>
#include <misc.h>

#include <EnvisionAPI.h>

int FarmModel::m_colFarmID = -1;
int FarmModel::m_colFarmHQ = -1;
int FarmModel::m_colFieldID = -1;
int FarmModel::m_colArea = -1;
int FarmModel::m_colRotation = -1;
int FarmModel::m_colRotIndex = -1;
int FarmModel::m_colLulc = -1;
int FarmModel::m_colRegion = -1;
int FarmModel::m_colCLI = -1;
int FarmModel::m_colFarmType = -1;
int FarmModel::m_colCropAge = -1;
int FarmModel::m_colCropStage = -1;
int FarmModel::m_colPlantDate = -1;  // "PLANTDATE"       - output
int FarmModel::m_colDormancy;  // "DORMANCY" - used during runtime

int FarmModel::m_maxFieldArea = int(80 * M2_PER_HA);
RandUniform FarmModel::m_rn;

int FarmField::m_idCounter = 1;


extern F2RProcess* theProcess;

////TCHAR* gCropEventNames[] =
////   {
////   "Planted",
////   "Poor Seed Cond",
////   "Early Frost",
////   "Mid Frost",
////   "Fall Frost",
////   "Winter Frost",
////   "Cool Nights",
////   "Warm Nights",
////   "Pol Heat",
////   "R2 Heat",
////   "Extreme Heat",
////   "Veg Drought",
////   "Pol Drought",
////   "R2 Drought",
////   "R3 Drought",
////   "R5 Drought",
////   "Pod Drought",
////   "Seed Drought",
////   "Early Flood Delay (Corn Planting)",
////   "Early Flood Delay (Soy Planting)",
////   "Early Flood Delay (Soy A)",
////   "Early Flood Delay (Soy B)",
////   "Early Flood Delay (Soy C)",
////   "Cool Spring",
////   "Early Flood Kill",
////   "Mid Flood",
////   "Corn to Soy",
////   "Corn to Fallow",
////   "Soy to Fallow",
////   "Wheat to Corn",
////   "Barley to Corn",
////   "Harvest",
////   "Yield Failure",
////   "Incomplete"
////   };


TCHAR* gFarmEventNames[] =
   {
   "Bought",
   "Sold",
   "Joined",
   "Changed Type",
   "Fields Merged",
   "Eliminated",
   "Recovered"
   };


bool IsCereal(int);
bool IsCereal(int lulc)
   {
   switch (lulc)
      {
      case 133:   // barley
      case 134:   // other cereals
      case 136:   // Oats
      case 140:   // wheat
         return true;
      }

   return false;
   }


float AccumulateYRFs(float yrf, float priorCumYRF);
float AccumulateYRFs(float yrf, float priorCumYRF)
   {
   // accumulate yrf's
   //yrf = (1-priorCumYRF)*yrf;   // accumulate yrfs over the course of the year
   yrf = priorCumYRF + (yrf * (1 - priorCumYRF));

   if (yrf < 0)
      yrf = 0;
   else if (yrf > 1)
      yrf = 1;

   return yrf;
   }

bool FarmRotation::ContainCereals()
   {
   for (int i = 0; i < (int)m_sequenceArray.GetSize(); i++)
      {
      if (IsCereal(m_sequenceArray[i]))
         return true;
      }

   return false;
   }


/////////////////////////////////////////////////////////////////////
// F A R M
/////////////////////////////////////////////////////////////////////

float Farm::Transfer(EnvContext* pContext, Farm* pFarmToTransfer, MapLayer* pLayer)
   {
   float totalAreaStart = m_totalArea;

   // remove the transferred farmHQ from the map
   theProcess->UpdateIDU(pContext, pFarmToTransfer->m_farmHQ, FarmModel::m_colFarmHQ, -1, ADD_DELTA);

   // copy the fields from the farm to transfer to this farm
   for (int i = 0; i < (int)pFarmToTransfer->m_fieldArray.GetSize(); i++)
      {
      FarmField* pFieldToTranfer = pFarmToTransfer->m_fieldArray[i];
      FarmField* pField = new FarmField(*pFieldToTranfer);  // creates a new FieldID
      // note: at this point, the Field's m_pFarm ptr points to the FarmToTransfer.  We will adjust this
      // below, but we save that until the end, so that we can distinguish fields just added from pre-existing fields

      // transfer an individual field to "this" farm.  This involves
      // 1) adjusting the two farm's areas
      // 2) fixing up the rotational scheme of the new Fields to match the recieving farm
      // 3) taking care of any constraints on crops
      for (int i = 0; i < (int)pField->m_iduArray.GetSize(); i++)
         {
         int idu = pField->m_iduArray[i];

         // write the new field ID to the IDUs
         theProcess->UpdateIDU(pContext, idu, FarmModel::m_colFieldID, pField->m_id);
         theProcess->UpdateIDU(pContext, idu, FarmModel::m_colFarmID, this->m_id);
         theProcess->UpdateIDU(pContext, idu, FarmModel::m_colFarmType, this->m_farmType);

         // add in area
         float area = 0;
         pLayer->GetData(idu, FarmModel::m_colArea, area);
         m_totalArea += area;

         // if different farm types, update rotations
         if (this->m_farmType != pFarmToTransfer->m_farmType)
            {
            FarmRotation* pRotation = this->m_pCurrentRotation;

            if (pRotation != NULL)      // rotation defined for this farm?
               {
               // apply the Farms' rotational scheme (at random position in rotation) to this idu
               int rotationCount = (int)pRotation->m_sequenceArray.GetSize();
               int rotIndex = (int)FarmModel::m_rn.RandValue(0, double(rotationCount) - 0.000001);

               // update the rotation and rotationIndex fields in the IDUs - that determines the rotational state of the field
               theProcess->UpdateIDU(pContext, idu, FarmModel::m_colRotation, pRotation->m_rotationID, ADD_DELTA);
               theProcess->UpdateIDU(pContext, idu, FarmModel::m_colRotIndex, rotIndex, ADD_DELTA);
               }
            else
               {
               theProcess->UpdateIDU(pContext, idu, FarmModel::m_colRotation, -1, ADD_DELTA);
               theProcess->UpdateIDU(pContext, idu, FarmModel::m_colRotIndex, -1, ADD_DELTA);
               }
            }  // end of: if ( farm types are different )

         // check additional constraints - convert to Fallow?
         int cli = -1;
         pLayer->GetData(idu, FarmModel::m_colCLI, cli);

         switch (cli)
            {
            case 1:
            case 2:
            case 3:
            case 40:
               // convert to fallow and remove from rotation
               theProcess->UpdateIDU(pContext, idu, FarmModel::m_colLulc, FALLOW, ADD_DELTA);
               theProcess->UpdateIDU(pContext, idu, FarmModel::m_colRotation, -1, ADD_DELTA);
               theProcess->UpdateIDU(pContext, idu, FarmModel::m_colRotIndex, -1, ADD_DELTA);
               break;
            }

         }  // end ofL for each IDU in the field

      m_fieldArray.Add(pField);
      pFieldToTranfer->m_iduArray.RemoveAll();
      }

   // get rid of the fields from the transfer farm
   pFarmToTransfer->m_fieldArray.RemoveAll();
   pFarmToTransfer->m_totalArea = 0;
   pFarmToTransfer->m_isPurchased = true;

   this->m_lastExpansion = pContext->currentYear;

   return m_totalArea - totalAreaStart;      // area absorbed into new farm
   }


// Farm::AdjustFieldBoundaries() consolidates fields within this Farm that 
// meet the following criteria:
// 1) the farm is expandable
// 2) the fields are adjacent
// 3) the fields have the same crop type, meaning they have the same LULC class
// 4) the joined fields results in a new field < max field size parameter (specified in xml)

float Farm::AdjustFieldBoundaries(EnvContext* pContext, int& count)
   {
   MapLayer* pLayer = (MapLayer*)pContext->pMapLayer;

   CArray< int, int > mergedIDUs;

   count = 0;

   // basic ideas: fields that are adjacent to each other and same crop type are joined/assigned same Field ID
   // if the above mentioned constraints are met
   int fieldsConsolidated = 1;
   int startFieldCount = (int)m_fieldArray.GetSize();

   while (fieldsConsolidated > 0)
      {
      fieldsConsolidated = 0;

      for (int i = 0; i < (int)this->m_fieldArray.GetSize(); i++)
         {
         // note on loop counter: we iterate though the list of field
         // For each field in the list, look at all "later" fields
         // if two field merged, loop counter is set back one 
         // to allow continued merging with starting field
         FarmField* pThisField = this->m_fieldArray[i];

         if (pThisField->m_iduArray.GetSize() == 0) // no IDUs?
            continue;

         int thisLulc = -1;
         pLayer->GetData(pThisField->m_iduArray[0], FarmModel::m_colLulc, thisLulc);

         // is this field adjacent to any other fields with the same crop type?
         for (int j = i + 1; j < this->m_fieldArray.GetSize(); j++)
            {
            //if (j == i)    // can't combine with yourself
            //   continue;

            FarmField* pOtherField = this->m_fieldArray[j];

            // no IDUs to transfer?
            if (pOtherField->m_iduArray.GetSize() == 0)
               continue;

            // max field size constraint applies?
            if (pOtherField->m_totalArea + pThisField->m_totalArea > FarmModel::m_maxFieldArea)  // over 80 ha?
               continue;

            // mismatched LULC?
            int otherLulc = -1;
            pLayer->GetData(pOtherField->m_iduArray[0], FarmModel::m_colLulc, otherLulc);
            if (thisLulc != otherLulc)  // crops don't match, so move on to next FarmField
               continue;

            // are the two field adjacent?
            if (pThisField->IsFieldAdjacent(pOtherField, pLayer))
               {
               // always add to the earliest  field in the list, remove the latest (early=smaller index)
               FarmField* pFromField = pOtherField;  // j > i ? pThisField : pOtherField;
               FarmField* pToField = pThisField; //  j <= i ? pThisField : pOtherField;

               // adjacent to and matching crop - combine fields
               for (int k = 0; k < pFromField->m_iduArray.GetSize(); k++)
                  {
                  int idu = pFromField->m_iduArray[k];
                  bool found = false;
                  for (int ii = 0; ii < mergedIDUs.GetSize(); ii++)
                     {
                     if (idu == mergedIDUs[ii])
                        {
                        found = true; break;
                        }
                     }
                  if (!found)
                     mergedIDUs.Add(idu);

                  theProcess->UpdateIDU(pContext, idu, FarmModel::m_colFieldID, pToField->m_id);
                  }

               pToField->m_iduArray.Append(pFromField->m_iduArray);  // copy IDUs
               pFromField->m_iduArray.RemoveAll();                       // eliminate old IDUs

               ASSERT(pToField->m_totalArea > 0);
               ASSERT(pFromField->m_totalArea > 0);
               pToField->m_totalArea += pFromField->m_totalArea;       // combine areas
               pFromField->m_totalArea = 0;
               fieldsConsolidated++;

               bool success = this->m_fieldArray.Remove(pFromField);// get rid of the field that was eliminated (deletes the other field)
               ASSERT(success);
               j--;   // Set loop counter back one to account for removed other field
               }  // end of: if ( adjacent )
            }  // end of: for each "other" field )
         }  // end of: for each "this" field

      count += fieldsConsolidated;
      }  // end of: while( fieldsConsolidated > 0 )

   float mergedArea = 0;
   for (int i = 0; i < mergedIDUs.GetSize(); i++)
      {
      float area = 0;
      pLayer->GetData(mergedIDUs[i], FarmModel::m_colArea, area);
      mergedArea += area;
      }

   count = startFieldCount - (int)m_fieldArray.GetSize();

   return mergedArea;
   }


bool FarmField::IsFieldAdjacent(FarmField* pOtherField, MapLayer* pIDULayer)
   {
   for (int i = 0; i < (int)this->m_iduArray.GetSize(); i++)
      {
      if (pOtherField->m_iduArray.GetSize() == 0)
         continue;

      // are any of this fields' IDUs adjacent to the other fields IDUs?
      for (int j = 0; j < this->m_iduArray.GetSize(); j++)
         {
         int thisIDU = this->m_iduArray[j];

         for (int k = 0; k < pOtherField->m_iduArray.GetSize(); k++)
            {
            int otherIDU = pOtherField->m_iduArray[k];

            if (pIDULayer->IsNextTo(thisIDU, otherIDU))
               {
               return true;
               }  // end of: if ( NextTo() )
            }  // end of: for ( l < pOtherField->m_iduArray.GetSize() )
         }  // end of: for ( k < pThisField->m_iduArray.GetSize() )

      }  // end of: for each "this" field

   return false;
   }


void Farm::ChangeFarmType(EnvContext* pContext, FARMTYPE farmType)
   {
   // change the farm type. This involves
   // 1) fixing up the rotational scheme of the new Fields to match the recieving farm
   // 2) taking care of any constraints on crops

   this->m_farmType = farmType;

   for (int f = 0; f < m_fieldArray.GetSize(); f++)
      {
      FarmField* pField = m_fieldArray[f];

      for (int i = 0; i < (int)pField->m_iduArray.GetSize(); i++)
         {
         int idu = pField->m_iduArray[i];

         // update the FarmType field in the IDU's for this farm
         theProcess->UpdateIDU(pContext, idu, FarmModel::m_colFarmType, farmType, ADD_DELTA);

         // fix up rotations
         FarmRotation* pRotation = this->m_pCurrentRotation;

         if (pRotation != NULL)      // rotation defined for this farm?
            {
            // apply the Farms' rotational scheme (at random position in rotation) to this idu
            int rotationCount = (int)pRotation->m_sequenceArray.GetSize();
            int rotIndex = (int)FarmModel::m_rn.RandValue(0, double(rotationCount) - 0.000001);

            // update the rotation and rotationIndex fields in the IDUs - that determines the rotational state of the field
            theProcess->UpdateIDU(pContext, idu, FarmModel::m_colRotation, pRotation->m_rotationID, ADD_DELTA);
            theProcess->UpdateIDU(pContext, idu, FarmModel::m_colRotIndex, rotIndex, ADD_DELTA);
            }
         else
            {
            theProcess->UpdateIDU(pContext, idu, FarmModel::m_colRotation, -1, ADD_DELTA);
            theProcess->UpdateIDU(pContext, idu, FarmModel::m_colRotIndex, -1, ADD_DELTA);
            }
         }  // end ofL for each IDU in the field
      }

   int count = 0;
   AdjustFieldBoundaries(pContext, count);
   }
/*
Questions:
 is there a minimum time between conversions?  yes, use 5 years
 do we use trend line rather than annual deltas?  use trendlines based in IC's fro initial conditions
*/

/////////////////////////////////////////////////////////////////////
// F A R M    M O D E L 
/////////////////////////////////////////////////////////////////////

FarmModel::FarmModel(void)
   : m_id(FARM_MODEL)
   , m_doInit(0)
   , m_useVSMB(true)
   , m_climateManager()
   , m_climateScenarioID(0)
   //, m_colFarmHQ( -1 )
   //, m_colFieldID( -1 )
   //, m_colFCTRegion( -1 )
   //, m_colFSTRegion(-1)
   , m_colSLC(-1)
   , m_colLulcA(-1)
   , m_colCropStatus(-1)
   , m_colYieldRed(-1)
   , m_colYield(-1)
   , m_colDairyAvg(-1)
   , m_colBeefAvg(-1)
   , m_colPigsAvg(-1)
   , m_colPoultryAvg(-1)
   , m_colCadID(-1)
   , m_avgYieldReduction(0)
   , m_avgFarmSizeHa(0)
   , m_adjFieldArea(0)
   , m_adjFieldCount(0)
   , m_adjFieldAreaHa(0)
   , m_avgFieldSizeHa(0)
   , m_yrfThreshold(0.90f)
   , m_pDailyData(NULL)
   , m_pCropEventData(NULL)
   , m_pCropEventPivotTable(NULL)
   , m_pFarmEventData(NULL)
   , m_pFarmEventPivotTable(NULL)
   , m_farmExpRateTable(2, 0)
   , m_reExpandPeriod(5)
   , m_maxFarmArea(int(1500 * M2_PER_HA))
   , m_pFarmTypeExpandCountData(NULL)
   , m_pFarmSizeTData(NULL)
   , m_pFarmTypeCountData(NULL)
   , m_pFarmExpTransMatrix(NULL)
   , m_pFarmTypeTransMatrix(NULL)
   , m_pFCTRData(NULL)
   , m_pFarmSizeTRData(NULL)
   , m_pFldSizeLData(NULL)
   , m_pFldSizeLRData(NULL)
   , m_pFldSizeTRData(NULL)
   , m_enableFCT(true)
   , m_enableFST(true)
   , m_totalIDUArea(0)
   , m_maxProcessors(-1)
   , m_processorsUsed(0)
   , m_outputPivotTable(true)
   , m_vsmb()
   {
   // zero out event array
   //memset(m_cropEvents, 0, sizeof(float) * (1 + CE_EVENTCOUNT));
   }


FarmModel::~FarmModel(void)
   {
   if (m_pDailyData != NULL)
      delete m_pDailyData;

   if (m_pCropEventData != NULL)
      delete m_pCropEventData;

   if (m_pCropEventPivotTable != NULL)
      delete m_pCropEventPivotTable;

   if (m_pFarmEventData != NULL)
      delete m_pFarmEventData;

   if (m_pFarmEventPivotTable != NULL)
      delete m_pFarmEventPivotTable;

   if (m_pFarmTypeExpandCountData != NULL)
      delete m_pFarmTypeExpandCountData;

   if (m_pFarmSizeTData != NULL)
      delete m_pFarmSizeTData;

   if (m_pFarmTypeCountData != NULL)
      delete m_pFarmTypeCountData;

   if (m_pFarmExpTransMatrix != NULL)
      delete m_pFarmExpTransMatrix;

   if (m_pFarmTypeTransMatrix != NULL)
      delete m_pFarmTypeTransMatrix;

   if (m_pFCTRData != NULL)
      delete m_pFCTRData;

   if (m_pFarmSizeTRData != NULL)
      delete m_pFarmSizeTRData;

   if (m_pFldSizeLData != NULL)
      delete m_pFldSizeLData;

   if (m_pFldSizeLRData != NULL)
      delete m_pFldSizeLRData;

   if (m_pFldSizeTRData != NULL)
      delete m_pFldSizeTRData;
   }

int FarmModel::AddBuiltInCropEventTypes() 
   {
   this->AddCropEventType(CE_UNDEFINED, "Undefined");
   this->AddCropEventType(CE_PLANTED, "Planted");
   this->AddCropEventType(CE_POORSEEDCOND, "Poor Seed Cond");
   this->AddCropEventType(CE_EARLY_FROST, "Early Frost");
   this->AddCropEventType(CE_MID_FROST, "Mid Frost");
   this->AddCropEventType(CE_FALL_FROST, "Fall Frost");
   this->AddCropEventType(CE_WINTER_FROST, "Winter Frost");
   this->AddCropEventType(CE_COOL_NIGHTS,"Cool Nights");
   this->AddCropEventType(CE_WARM_NIGHTS, "Warm Nights");
   this->AddCropEventType(CE_POL_HEAT, "Pol Heat");
   this->AddCropEventType(CE_R2_HEAT, "R2 Heat");
   this->AddCropEventType(CE_EXTREME_HEAT,"Extreme Heat");
   this->AddCropEventType(CE_VEG_DROUGHT, "Veg Drought");
   this->AddCropEventType(CE_POL_DROUGHT, "Pol Drought");
   this->AddCropEventType(CE_R2_DROUGHT, "R2 Drought");
   this->AddCropEventType(CE_R3_DROUGHT, "R3 Drought");
   this->AddCropEventType(CE_R5_DROUGHT, "R5 Drought");
   this->AddCropEventType(CE_POD_DROUGHT, "Pod Drought");
   this->AddCropEventType(CE_SEED_DROUGHT,"Seed Drought");
   this->AddCropEventType(CE_EARLY_FLOOD_DELAY_CORN_PLANTING,"Early Flood Delay (Corn Planting)");
   this->AddCropEventType(CE_EARLY_FLOOD_DELAY_SOY_PLANTING,"Early Flood Delay (Soy Planting)");
   this->AddCropEventType(CE_EARLY_FLOOD_DELAY_SOY_A,"Early Flood Delay (Soy A)");
   this->AddCropEventType(CE_EARLY_FLOOD_DELAY_SOY_B,"Early Flood Delay (Soy B)");
   this->AddCropEventType(CE_EARLY_FLOOD_DELAY_SOY_C,"Early Flood Delay (Soy C)");
   this->AddCropEventType(CE_COOL_SPRING, "Cool Spring");
   this->AddCropEventType(CE_EARLY_FLOOD_KILL, "Early Flood Kill");
   this->AddCropEventType(CE_MID_FLOOD,"Mid Flood");
   this->AddCropEventType(CE_CORN_TO_SOY, "Corn to Soy");
   this->AddCropEventType(CE_CORN_TO_FALLOW, "Corn to Fallow");
   this->AddCropEventType(CE_SOY_TO_FALLOW,"Soy to Fallow");
   this->AddCropEventType(CE_WHEAT_TO_CORN, "Wheat to Corn");
   this->AddCropEventType(CE_BARLEY_TO_CORN,"Barley to Corn");
   this->AddCropEventType(CE_HARVEST, "Harvest");
   this->AddCropEventType(CE_YIELD_FAILURE, "Yield Failure");
   this->AddCropEventType(CE_INCOMPLETE, "Incomplete");

   return (int) this->m_cropEventTypes.GetSize();
   }


bool FarmModel::Init(EnvContext* pContext, LPCTSTR initStr)
   {
   m_processorsUsed = omp_get_num_procs();

   m_processorsUsed = (2 * m_processorsUsed / 3) + 1;   // default uses 2/3 of the cores
   if (m_maxProcessors > 0 && m_processorsUsed >= m_maxProcessors)
      m_processorsUsed = m_maxProcessors;

   omp_set_num_threads(m_processorsUsed);

   MapLayer* pMapLayer = (MapLayer*)pContext->pMapLayer;

   this->AddBuiltInCropEventTypes();

   // load table
   bool ok = LoadXml(pContext, initStr);
   if (!ok)
      return false;

   // initialize submodels
   m_climateManager.Init(pContext, initStr);
   m_csModel.Init(this, pMapLayer, pContext->pQueryEngine, pContext->pExprEngine);

   if (m_doInit > 0)
      InitializeFarms(pMapLayer);   // populates FarmType, FT_Code fields based on FT_Extent strings

   // these are hard coded columns (for now)
   ok = theProcess->CheckCol(pMapLayer, m_colArea, "Area", TYPE_FLOAT, CC_MUST_EXIST);
   if (!ok)
      return false;

   theProcess->CheckCol(pMapLayer, m_colFieldID, "Field_ID", TYPE_INT, CC_AUTOADD);
   theProcess->CheckCol(pMapLayer, m_colSLC, "SLC", TYPE_INT, CC_AUTOADD);
   theProcess->CheckCol(pMapLayer, m_colRotIndex, "ROT_INDEX", TYPE_INT, CC_AUTOADD);
   theProcess->CheckCol(pMapLayer, m_colCropStatus, "CropStatus", TYPE_INT, CC_AUTOADD);
   theProcess->CheckCol(pMapLayer, m_colCropStage, "CropStage", TYPE_INT, CC_AUTOADD);
   theProcess->CheckCol(pMapLayer, m_colCropAge, "CropAge", TYPE_INT, CC_AUTOADD);
   theProcess->CheckCol(pMapLayer, m_colPlantDate, "PlantDate", TYPE_INT, CC_AUTOADD);
   theProcess->CheckCol(pMapLayer, m_colYieldRed, "YieldRed", TYPE_FLOAT, CC_AUTOADD);
   theProcess->CheckCol(pMapLayer, m_colYield, "Yield", TYPE_FLOAT, CC_AUTOADD);
   theProcess->CheckCol(pMapLayer, m_colDormancy, "DORMANCY", TYPE_INT, CC_AUTOADD);

   theProcess->CheckCol(pMapLayer, m_colDairyAvg, "DairyAvg", TYPE_FLOAT, CC_AUTOADD);
   theProcess->CheckCol(pMapLayer, m_colBeefAvg, "BeefAvg", TYPE_FLOAT, CC_AUTOADD);
   theProcess->CheckCol(pMapLayer, m_colPigsAvg, "PigsAvg", TYPE_FLOAT, CC_AUTOADD);
   theProcess->CheckCol(pMapLayer, m_colPoultryAvg, "Poultry_Av", TYPE_FLOAT, CC_AUTOADD);
   theProcess->CheckCol(pMapLayer, m_colCadID, "CAD_ID", TYPE_INT, CC_MUST_EXIST);
   theProcess->CheckCol(pMapLayer, m_colCLI, "CLI_d_code", TYPE_INT, CC_MUST_EXIST);

   pMapLayer->SetColNoData(m_colRotIndex);
   pMapLayer->SetColNoData(m_colCropStatus);
   pMapLayer->SetColNoData(m_colCropStage);
   pMapLayer->SetColNoData(m_colPlantDate);
   pMapLayer->SetColNoData(m_colYieldRed);
   pMapLayer->SetColNoData(m_colYield);

   // initialize dormancys to 0
   pMapLayer->SetColData(m_colDormancy, VData(0), true);

   // build the farm aggregations
   int farmCount = BuildFarms(pMapLayer);

   UpdateAvgFarmSizeByRegion(pMapLayer, true);      // recalculates all values in FST_INFO's

   // set up input variables
   theProcess->m_inVarIndexFarmModel = theProcess->GetInputVarCount();
   theProcess->m_inVarCountFarmModel = 1;
   theProcess->AddInputVar("Climate Scenario ID", this->m_climateScenarioID, "0=CCSM4, IPSL=1, MPI=2");

   // set up any outputs for the FarmModel
   SetupOutputVars(pContext);

   CString msg;
   msg.Format("Farm Model: Success building %i farms, %i farm types, %i rotations", (int)m_farmArray.GetSize(),
      (int)m_farmTypeArray.GetSize(), (int)m_rotationArray.GetSize());
   Report::Log(msg);

   return true;
   }


bool FarmModel::InitRun(EnvContext* pContext)
   {
   m_climateManager.InitRun(m_climateScenarioID);
   m_csModel.InitRun(pContext);

   // initialize scenario-specific farm data
   for (int i = 0; i < (int)m_farmArray.GetSize(); i++)
      {
      Farm* pFarm = m_farmArray[i];
      pFarm->m_pClimateStation = m_climateManager.GetStationFromID(pFarm->m_climateStationID);
      pFarm->m_isExpandable = false;
      }

   // also initializates individual crop area output vars
   AllocateInitialCropRotations((MapLayer*)pContext->pMapLayer);

   // reset output data objects
   for (int i = 0; i < (int)m_annualCIArray.GetSize(); i++)
      m_annualCIArray[i]->ClearRows();

   for (int i = 0; i < (int)m_dailyCIArray.GetSize(); i++)
      m_dailyCIArray[i]->ClearRows();

   m_pDailyData->ClearRows();
   m_pCropEventData->ClearRows();
   m_pFarmEventData->ClearRows();
   m_pFCTRData->ClearRows();
   m_pFarmSizeTRData->ClearRows();
   m_pFldSizeLData->ClearRows();    // field size by LULC_B
   m_pFldSizeLRData->ClearRows();   // field size by LULC_B and region
   m_pFldSizeTRData->ClearRows();   // field size by Farmt Type and Region

   if (m_outputPivotTable)
      {
      m_pCropEventPivotTable->ClearRows();
      m_pFarmEventPivotTable->ClearRows();
      }

   if (m_pFarmExpTransMatrix != NULL)
      {
      int farmTypeCount = (int)m_farmTypeArray.GetSize();
      int regionCount = (int)m_regionsArray.GetSize();
      int count = farmTypeCount * regionCount;
      for (int i = 1; i < count + 1; i++)
         for (int j = 0; j < count; j++)
            m_pFarmExpTransMatrix->Set(i, j, 0);
      }

   if (m_pFarmTypeTransMatrix != NULL)
      {
      int farmTypeCount = (int)m_farmTypeArray.GetSize();
      int regionCount = (int)m_regionsArray.GetSize();
      int count = farmTypeCount * regionCount;
      for (int i = 1; i < count + 1; i++)
         for (int j = 0; j < count; j++)
            m_pFarmTypeTransMatrix->Set(i, j, 0);
      }
   return true;
   }


// run one year
bool FarmModel::Run(EnvContext* pContext, bool useAddDelta)
   {
   MapLayer* pLayer = (MapLayer*)pContext->pMapLayer;
   bool readOnly = pLayer->m_readOnly;
   pLayer->m_readOnly = false;

   // update climate metrics for this year
   m_climateManager.Run(pContext, useAddDelta);
   m_csModel.Run(pContext, useAddDelta);

   // reset any columns and dataobjs that need to be reset at the start of the year
   ResetAnnualData(pContext);

   if (m_enableFCT)
      UpdateFarmCounts(pContext);

   // runs through one year
   RotateCrops(pContext, useAddDelta);     // Farms only
   GrowCrops(pContext, useAddDelta);       // Farms only

   if (m_enableFarmExpansion)
      ExpandFarms(pContext);

   // check for joinable fields
   ConsolidateFields(pContext);

   UpdateAnnualOutputs(pContext, useAddDelta);

   pLayer->m_readOnly = readOnly;


   /////////////////
   //int fvCount = 0;
   //float fvArea = 0;
   //float fvhqArea = 0;
   //for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
   //   {
   //   int farmType = -1;
   //   pLayer->GetData(idu, m_colFarmType, farmType);
   //
   //   if (farmType == FT_OTH)
   //      {
   //      int hq = -1;
   //      float area = 0;
   //      pLayer->GetData(idu, m_colFarmHQ, hq);
   //      pLayer->GetData(idu, m_colArea, area);
   //
   //      if (hq >= 0)
   //         {
   //         fvCount++;
   //         fvhqArea += area;
   //         }
   //
   //      fvArea += area;
   //      }
   //   }
   //
   //CString msg;
   //msg.Format( "FarmModel: Other Crop Area = %.1f ha", fvArea/10000);
   //Report::LogWarning( msg );

   return true;
   }


bool FarmModel::EndRun(EnvContext* pContext)
   {
   LPCTSTR outPath = PathManager::GetPath(PM_OUTPUT_DIR);   // terminated with a backslash
   CString path = outPath;
   path += "FarmExTransMatrix.csv";

   if (m_pFarmExpTransMatrix != NULL)
      m_pFarmExpTransMatrix->WriteAscii(path);

   path = outPath;
   path += "FarmTypeTransMatrix.csv";

   if (m_pFarmTypeTransMatrix != NULL)
      m_pFarmTypeTransMatrix->WriteAscii(path);

   return true;
   }


int FarmModel::ConsolidateFields(EnvContext* pContext)
   {
   int count = 0;
   for (int i = 0; i < m_farmArray.GetSize(); i++)
      {
      Farm* pFarm = m_farmArray[i];

      // consolidate fields if this farm is expandable, or has expanded in the last three years ???
      if (pFarm->m_isExpandable || ((pContext->currentYear - pFarm->m_lastExpansion) < 3))
         {
         int farmID = pFarm->m_id;

         int _count = 0;
         pFarm->AdjustFieldBoundaries(pContext, _count);
         count += _count;
         }
      }

   return count;
   }


bool FarmModel::ResetAnnualData(EnvContext* pContext)
   {
   MapLayer* pLayer = (MapLayer*)pContext->pMapLayer;
   //MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;
   pLayer->SetColNoData(m_colYieldRed);
   pLayer->SetColNoData(m_colYield);
   pLayer->SetColNoData(m_colPlantDate);

   // intialize farms for this year
   m_totalIDUAreaAg = 0;

   for (int i = 0; i < (int)m_farmArray.GetSize(); i++)
      {
      Farm* pFarm = m_farmArray[i];

      pFarm->m_isExpandable = false;

      if (pFarm->IsActive() == false)
         continue;

      for (int j = 0; j < (int)pFarm->m_fieldArray.GetSize(); j++)
         {
         FarmField* pField = pFarm->m_fieldArray[j];

         if (pField == NULL)
            continue;

         int iduCount = (int)pField->m_iduArray.GetCount();
         for (int k = 0; k < iduCount; k++)
            {
            int idu = pField->m_iduArray[k];

            float area = 0;
            pLayer->GetData(idu, m_colArea, area);

            int lulc = 0;
            pLayer->GetData(idu, m_colLulc, lulc);

            if (IsAg(pLayer, idu))
               {
               CSCrop* pCrop;
               if ( m_csModel.m_cropLookup.Lookup(lulc, pCrop) )
                  pLayer->SetData(idu, m_colCropStage, pCrop->m_cropStages[0]->m_id);
               else
                  pLayer->SetData(idu, m_colCropStage, CS_PREPLANT);

               pLayer->SetData(idu, m_colCropStatus, CE_UNDEFINED);
               pLayer->SetData(idu, m_colYieldRed, 0);
               m_totalIDUAreaAg += area;
               }
            else  // non-ag 
               {
               pLayer->SetData(idu, m_colCropStage, CS_NONCROP);
               pLayer->SetData(idu, m_colCropStatus, CE_UNDEFINED); //CST_NONCROP );
               pLayer->SetData(idu, m_colYieldRed, 0);
               }
            }
         }
      }

   // reset events
   m_cropEvents[0] = (float) pContext->currentYear;
   for (int i = 0; i < m_cropEventTypes.GetSize(); i++)
      m_cropEvents[i + 1] = 0;

   m_farmEvents[0] = (float) pContext->currentYear;
   for (int i = 0; i < FE_EVENTCOUNT; i++)
      m_farmEvents[i + 1] = 0;

   return true;
   }


/*
---------------------------------------------------------------
UpdateFarmCounts()
  using the farm count trajectory array, which contains
  information about annual projected deltas for a given farm
  type and region, determine if we are achieving the specified
  counts of active farms.  If not, try to convert farms as needed
  to achieve the targets
---------------------------------------------------------------
*/

bool FarmModel::UpdateFarmCounts(EnvContext* pContext)
   {
   MapLayer* pLayer = (MapLayer*)pContext->pMapLayer;

   for (int i = 0; i < this->m_fctArray.GetSize(); i++)
      {
      m_fctArray[i]->m_deficitCount = 0;
      m_fctArray[i]->count = 0;
      m_fctArray[i]->farmArray.RemoveAll();
      }

   // basic idea. get current counts, and then adjust as needed based on trajectory information in the FTC array
   for (int i = 0; i < this->m_farmArray.GetSize(); i++)
      {
      Farm* pFarm = this->m_farmArray[i];

      if (pFarm->IsActive())
         {
         // get region and farmtype for this farm
         int hqIDU = pFarm->m_farmHQ;

         if (hqIDU < 0)
            continue;

         int regionID = -1;
         pLayer->GetData(hqIDU, m_colRegion, regionID);
         long key = FCT_INFO::Key(regionID, (int)pFarm->m_farmType);

         FCT_INFO* pInfo = NULL;

         BOOL found = m_fctMap.Lookup(key, pInfo);

         if (found && pInfo != NULL)
            {
            pInfo->count++;
            pInfo->farmArray.Add(pFarm);
            }
         }
      }

   // farm type counts updated, make adjustments as needed

   // update each FCT_INFO (farm count trajectories by type and region)
   for (int i = 0; i < this->m_fctArray.GetSize(); i++)
      {
      int countRemoved = 0;
      int countAdded = 0;

      FCT_INFO* pInfo = m_fctArray[i];

      ASSERT(pInfo->count == (int)pInfo->farmArray.GetSize());

      float delta = pInfo->slope;  // this is the number of farms to add/remove this year
      float frac = (float)fmod(fabs(delta), 1);
      int count = 0;   // this is the number of farms to be added or removed

      // deal with float/int conversion for slopes with decimals stochastically
      if (frac > 0.0001f && frac >= (float)m_rn.RandValue(0, 1))
         count++;

      // negative slope?  Then we need to remove farms
      if (delta < -0.00001f)
         {
         count -= (int)delta;  // note: delta is negative, so this is really an addition, meaning number to remove.

         // randomly order array, remove/add as needed
         ::ShuffleArray(pInfo->farmArray.GetData(), pInfo->count, &m_rn);

         int countRemoved = count;
         if (count >= pInfo->count) // don't remove more are there
            countRemoved = pInfo->count;

         // remove (retire) farms as needed
         for (int j = 0; j < countRemoved; j++)
            {
            Farm* pFarm = pInfo->farmArray[j];
            pFarm->m_isRetired = true;
            //theProcess->UpdateIDU( pContext, pFarm->m_farmHQ, m_colFarmHQ, -1, ADD_DELTA );   // set in IDUs, but not in the Farm object

            AddFarmEvent(pContext, pFarm->m_farmHQ, FE_ELIMINATED, pFarm->m_id, pFarm->m_farmType, pFarm->m_totalArea / M2_PER_HA);
            }

         pInfo->m_deficitCount = countRemoved - count;   // note: this is negative value orzero
         }

      else if (delta > 0.00001f)  // positive slope? then look for farms to add back
         {
         count += (int)delta;   // count to add 

         // have count, start looking for farms to add back
         for (int j = 0; j < m_farmArray.GetSize(); j++)
            {
            if (countAdded == count)  //  have we added enough? then quit adding
               break;

            Farm* pFarm = m_farmArray[j];

            // add a farm if:
            //   1) it is currently retired
            //   2) it has an HQ
            //   3) it has fields (Note: "transferred" farms lose their fields during ExpandFarms() )
            //   4) it is in the correct region
            if (pFarm->m_isRetired) //pFarm->IsActive() == false )
               {
               // right region?
               int hq = pFarm->m_farmHQ;   // contains farmid if valid farm

               if (hq < 0)
                  continue;

               if (pFarm->m_fieldArray.GetSize() == 0)
                  continue;

               int regionID = -1;
               pLayer->GetData(hq, m_colRegion, regionID);

               if (regionID != pInfo->regionID)
                  continue;

               // found an inactive farm in the region,  make it active with this farm type
               pFarm->m_isRetired = false;   // unretire this farm
               pFarm->m_isPurchased = false; // tag it as unpurchased, since it is in effect a new farm 
               pFarm->m_farmType = pInfo->farmType;

               // set the IDU FARM_HQ field for this Farm's HQ idu to the Farms' id 
               //theProcess->UpdateIDU( pContext, pFarm->m_farmHQ, m_colFarmHQ, pFarm->m_id, ADD_DELTA );
               AddFarmEvent(pContext, pFarm->m_farmHQ, FE_RECOVERED, pFarm->m_id, pFarm->m_farmType, pFarm->m_totalArea / M2_PER_HA);

               // set up fields by getting a rotation
               FarmType* pFarmType = NULL;
               m_farmTypeMap.Lookup(pInfo->farmType, pFarmType);

               if (pFarmType != NULL)
                  {
                  int rotCount = (int)pFarmType->m_rotationArray.GetSize();

                  if (rotCount > 0)
                     {
                     int rotIndex = (int)m_rn.RandValue(0.0, double(rotCount) - 0.000001);

                     FarmRotation* pRotation = pFarmType->m_rotationArray[rotIndex];
                     int seqCount = (int)pRotation->m_sequenceArray.GetSize();

                     for (int k = 0; k < pFarm->m_fieldArray.GetSize(); k++)
                        {
                        int seqIndex = (int)m_rn.RandValue(0.0, double(seqCount) - 0.000001);

                        int lulc = pRotation->m_sequenceArray[seqIndex];

                        FarmField* pField = pFarm->m_fieldArray[k];
                        for (int l = 0; l < pField->m_iduArray.GetSize(); l++)
                           {
                           int idu = pField->m_iduArray[l];
                           theProcess->UpdateIDU(pContext, idu, m_colLulc, lulc, ADD_DELTA);
                           theProcess->UpdateIDU(pContext, idu, m_colRotation, pRotation->m_rotationID, ADD_DELTA);
                           theProcess->UpdateIDU(pContext, idu, m_colRotIndex, seqIndex, ADD_DELTA);
                           }
                        }
                     }
                  }

               countAdded++;
               }  // if inactive farm found
            }  // for each farm

         // did we get enough?
         pInfo->m_deficitCount = (countAdded < count) ? (count - countAdded) : 0;
         }  // end of: else if ( delta > 0 )

      pInfo->count += countAdded - countRemoved;
      }  // end of: for each FTC_INFO

   // last step is to fill any remaining FCT deficits if any.  This involved converting 
   // active farms within regions
   for (int i = 0; i < this->m_fctArray.GetSize(); i++)
      {
      FCT_INFO* pInfo = m_fctArray[i];

      if (pInfo->m_deficitCount <= 0)  // are there currently deficits that need to be addressed? if no, continue
         continue;

      // we have a deficit, so look for farms we can convert to this type
      // basic idea - look for farms within the region that can be converted.
      // Conversion is allowed when a farm:
      //  1) is in the same region, 
      //  2) is of a type not currently in a deficit for the region

      FarmType* pFarmType = NULL;
      m_farmTypeMap.Lookup(pInfo->farmType, pFarmType);
      ASSERT(pFarmType != NULL);

      for (int j = 0; j < pFarmType->m_expandTypes.GetSize(); j++)
         {
         FarmType* pExpFarmType = pFarmType->m_expandTypes[j];
         FCT_INFO* pExInfo = NULL;
         long key = FCT_INFO::Key(pInfo->regionID, pExpFarmType->m_type);
         if (m_fctMap.Lookup(key, pExInfo) == FALSE)
            continue;    // no FCT infr, so move on?  This is the only place to look for deficits ??????

         ASSERT(pExInfo != NULL);
         if (pExInfo->m_deficitCount < 0)   // was there a surplus in this class?
            {
            // yes, so it is a candidate class for conversion 
            // so start converting the farms in this class until deficit reaches 0
            for (int k = 0; k < pExInfo->farmArray.GetSize(); k++)
               {
               if (pExInfo->m_deficitCount >= 0)
                  break;

               if (pInfo->m_deficitCount <= 0)
                  break;

               Farm* pExFarm = pExInfo->farmArray[k];
               FARMTYPE oldFarmType = pExFarm->m_farmType;
               pExFarm->ChangeFarmType(pContext, pInfo->farmType);

               // add event for this transfer
               AddFarmEvent(pContext, pExFarm->m_farmHQ, FE_CHANGEDTYPE, pExFarm->m_id, pExFarm->m_farmType, pExFarm->m_totalArea / M2_PER_HA);

               // update transition matrix
               int row, col;
               m_farmTypesIndexMap.Lookup(oldFarmType, row);            // rows are "from"
               m_farmTypesIndexMap.Lookup(pInfo->farmType, col);         // cols are"to"

               int count = m_pFarmTypeTransMatrix->GetAsInt(col, row);
               m_pFarmTypeTransMatrix->Set(col, row, count + 1);

               // clean up farm arrays
               pExInfo->farmArray.Remove(pExFarm);
               pInfo->farmArray.Add(pExFarm);

               pInfo->m_deficitCount--;
               pExInfo->m_deficitCount++;
               k--;
               }
            }

         // are we done with this FCT_INFO?
         if (pInfo->m_deficitCount <= 0)
            break;
         }  // end of: for each farm tye on the expandables list

      if (pInfo->m_deficitCount <= 0)
         break;
      }  // end of: for each FCT_INFO

   return true;
   }


// called once only in Init()
void FarmModel::SetupOutputVars(EnvContext* pContext)
   {
   MapLayer* pLayer = (MapLayer*)pContext->pMapLayer;

   // set up any output data objs
   ASSERT(m_pDailyData == NULL);
   m_pDailyData = new FDataObj(3, 0);
   m_pDailyData->SetName("Farm Model Daily Data");
   m_pDailyData->SetLabel(0, "Time");
   m_pDailyData->SetLabel(1, "Avg Yield Reduction");
   m_pDailyData->SetLabel(2, "Avg Planting Date");

   // allocate slots for crop event areas (first col is year)
   m_cropEvents.SetSize(1 + m_cropEventTypes.GetSize());
   for (int i = 0; i < m_cropEvents.GetSize(); i++)
      m_cropEvents[i] = 0;

   ASSERT(m_pCropEventData == NULL);
   m_pCropEventData = new FDataObj(1 + (int) m_cropEventTypes.GetSize(), 0);
   m_pCropEventData->SetName("Farm Model Crop Events");
   m_pCropEventData->SetLabel(0, "Time");
   for (int i = 0; i < m_cropEventTypes.GetSize(); i++)
      m_pCropEventData->SetLabel(i + 1, m_cropEventTypes[i]->label);

   // crop event pivot table
   if (m_outputPivotTable)
      {
      ASSERT(m_pCropEventPivotTable == NULL);
      m_pCropEventPivotTable = new VDataObj(14, 0, U_UNDEFINED);
      m_pCropEventPivotTable->SetName("Crop Events Pivot Table");
      m_pCropEventPivotTable->SetLabel(0, "Year");
      m_pCropEventPivotTable->SetLabel(1, "DayOfYear");
      m_pCropEventPivotTable->SetLabel(2, "EventCode");
      m_pCropEventPivotTable->SetLabel(3, "EventName");
      m_pCropEventPivotTable->SetLabel(4, "Idu_Index");
      m_pCropEventPivotTable->SetLabel(5, "Area(ha)");
      m_pCropEventPivotTable->SetLabel(6, "LulcCode");
      m_pCropEventPivotTable->SetLabel(7, "Lulc");
      m_pCropEventPivotTable->SetLabel(8, "FarmID");
      m_pCropEventPivotTable->SetLabel(9, "FarmType");
      m_pCropEventPivotTable->SetLabel(10, "Ecoregion");
      m_pCropEventPivotTable->SetLabel(11, "SLC");
      m_pCropEventPivotTable->SetLabel(12, "EventYRF");
      m_pCropEventPivotTable->SetLabel(13, "CropYRF");
      }

   ASSERT(m_pFarmEventData == NULL);
   m_pFarmEventData = new FDataObj(1 + FE_EVENTCOUNT, 0);
   m_pFarmEventData->SetName("Farm Model Farm Events");
   m_pFarmEventData->SetLabel(0, "Time");
   for (int i = 0; i < FE_EVENTCOUNT; i++)
      m_pFarmEventData->SetLabel(i + 1, gFarmEventNames[i]);


   // farm event pivot table
   if (m_outputPivotTable)
      {
      ASSERT(m_pFarmEventPivotTable == NULL);
      m_pFarmEventPivotTable = new VDataObj(7, 0, U_UNDEFINED);
      m_pFarmEventPivotTable->SetName("Farm Events Pivot Table");
      m_pFarmEventPivotTable->SetLabel(0, "Year");
      m_pFarmEventPivotTable->SetLabel(1, "EventCode");
      m_pFarmEventPivotTable->SetLabel(2, "EventName");
      m_pFarmEventPivotTable->SetLabel(3, "HQ_Idu_Index");
      m_pFarmEventPivotTable->SetLabel(4, "Area(ha)");
      m_pFarmEventPivotTable->SetLabel(5, "FarmID");
      m_pFarmEventPivotTable->SetLabel(6, "FarmType");
      }

   // Farm expansion stats
   ASSERT(m_pFarmTypeExpandCountData == NULL);
   int farmTypeCount = (int)m_farmTypeArray.GetSize();
   ASSERT(farmTypeCount == FT_COUNT);
   m_pFarmTypeExpandCountData = new FDataObj(1 + farmTypeCount, 0);
   m_pFarmTypeExpandCountData->SetName("Farm Type Expansion Counts");
   m_pFarmTypeExpandCountData->SetLabel(0, "Time");
   for (int i = 0; i < farmTypeCount; i++)
      m_pFarmTypeExpandCountData->SetLabel(i + 1, m_farmTypeArray[i]->m_code);

   // farm type stats
   ASSERT(m_pFarmSizeTData == NULL);
   m_pFarmSizeTData = new FDataObj(1 + farmTypeCount, 0);
   m_pFarmSizeTData->SetName("Farm Type Average Sizes");
   m_pFarmSizeTData->SetLabel(0, "Time");
   for (int i = 0; i < farmTypeCount; i++)
      m_pFarmSizeTData->SetLabel(i + 1, m_farmTypeArray[i]->m_code);

   ASSERT(m_pFarmTypeCountData == NULL);
   m_pFarmTypeCountData = new FDataObj(1 + farmTypeCount, 0);
   m_pFarmTypeCountData->SetName("Farm Type Counts");
   m_pFarmTypeCountData->SetLabel(0, "Time");
   for (int i = 0; i < farmTypeCount; i++)
      m_pFarmTypeCountData->SetLabel(i + 1, m_farmTypeArray[i]->m_code);

   // set up some necessary maps, arrays
   // first, collect regions
   for (int i = 0; i < m_farmTypeArray.GetSize(); i++)
      {
      FarmType* pFarmType = m_farmTypeArray[i];

      for (int j = 0; j < pFarmType->m_expandRegions.GetSize(); j++)
         {
         int region = atoi(pFarmType->m_expandRegions[j]);
         m_regionsArray.InsertUnique(region);
         }
      }

   m_regionsArray.Sort();

   int regionCount = (int)m_regionsArray.GetSize();
   for (int i = 0; i < regionCount; i++)
      m_regionsIndexMap.SetAt(m_regionsArray[i], i);
   for (int i = 0; i < farmTypeCount; i++)
      m_farmTypesIndexMap.SetAt(m_farmTypeArray[i]->m_type, i);

   // get ag classes and store in a map for quick access to their index into the data objs
   int agClassesCount = 0;
   LulcNode* pNode = pContext->pLulcTree->FindNode(1, ANNUAL_CROP);
   ASSERT(pNode != NULL);

   for (int i = 0; i < pNode->m_childNodeArray.GetSize(); i++)
      {
      LulcNode* pLulcBNode = pNode->m_childNodeArray[i];
      int lulcB = pLulcBNode->m_id;
      m_agLulcBCols.SetAt(lulcB, agClassesCount);       // note that this DOES NOT include the time col
      m_agLulcBNames.SetAt(lulcB, (LPCTSTR)pLulcBNode->m_name);
      agClassesCount++;
      }

   pNode = pContext->pLulcTree->FindNode(1, PERENNIAL_CROP);
   ASSERT(pNode != NULL);

   for (int i = 0; i < pNode->m_childNodeArray.GetSize(); i++)
      {
      LulcNode* pLulcBNode = pNode->m_childNodeArray[i];
      int lulcB = pLulcBNode->m_id;
      m_agLulcBCols.SetAt(lulcB, agClassesCount + 1);
      m_agLulcBNames.SetAt(lulcB, (LPCTSTR)pLulcBNode->m_name);
      agClassesCount++;
      }

   ASSERT(m_pFldSizeLData == NULL);    // field size by LULC_B - need column for ag-based LULC_B classes
   m_pFldSizeLData = new FDataObj(1 + agClassesCount, 0);
   m_pFldSizeLData->SetName("Field Size by LulcB");
   m_pFldSizeLData->SetLabel(0, "Time");

   POSITION pos = m_agLulcBCols.GetStartPosition();
   while (pos != NULL)
      {
      int lulcB;
      int col;
      m_agLulcBCols.GetNextAssoc(pos, lulcB, col);

      LulcNode* pNode = pContext->pLulcTree->FindNode(2, lulcB);
      ASSERT(pNode != NULL);

      m_pFldSizeLData->SetLabel(1 + col, pNode->m_name);
      }

   ASSERT(m_pFldSizeLRData == NULL);   // field size by LULC_B and region (regions=outer, LULCB=inner)
   m_pFldSizeLRData = new FDataObj(1 + regionCount * agClassesCount, 0);
   m_pFldSizeLRData->SetName("Field Size by LULC_B and Region");
   m_pFldSizeLRData->SetLabel(0, "Time");

   pos = m_agLulcBCols.GetStartPosition();
   while (pos != NULL)
      {
      int lulcB = -1;
      int lulcCol = -1;
      m_agLulcBCols.GetNextAssoc(pos, lulcB, lulcCol);

      LulcNode* pNode = pContext->pLulcTree->FindNode(2, lulcB);
      ASSERT(pNode != NULL);

      for (int i = 0; i < regionCount; i++)
         {
         int region = m_regionsArray[i];

         int lrCol = lulcCol * regionCount + i;
         CString label;
         label.Format("%s_%i", (LPCTSTR)pNode->m_name, region);
         m_pFldSizeLRData->SetLabel(1 + lrCol, label);
         }
      }

   ASSERT(m_pFldSizeTRData == NULL);   // field size by Farm Type and Region
   m_pFldSizeTRData = new FDataObj(1 + regionCount * farmTypeCount, 0);
   m_pFldSizeTRData->SetName("Field Size by Region and Farm Type");
   m_pFldSizeTRData->SetLabel(0, "Time");

   int col = 1;
   for (int i = 0; i < farmTypeCount; i++)
      {
      FarmType* pFarmType = NULL;
      m_farmTypeMap.Lookup(m_farmTypeArray[i]->m_type, pFarmType);
      ASSERT(pFarmType != NULL);

      for (int j = 0; j < regionCount; j++)
         {
         CString label;
         label.Format("%s_%i", (LPCTSTR)pFarmType->m_code, m_regionsArray[j]);
         m_pFldSizeTRData->SetLabel(col++, label);
         }
      }

   // expansion transition matrix
   ASSERT(m_pFarmExpTransMatrix == NULL);
   m_pFarmExpTransMatrix = new VDataObj(farmTypeCount * regionCount + 1, farmTypeCount * regionCount, U_UNDEFINED);
   m_pFarmExpTransMatrix->SetName("Farm Expansion Transition Matrix");

   ASSERT(m_pFarmTypeTransMatrix == NULL);
   m_pFarmTypeTransMatrix = new VDataObj(farmTypeCount * regionCount + 1, farmTypeCount * regionCount, U_UNDEFINED);
   m_pFarmTypeTransMatrix->SetName("Farm Type Transition Matrix");

   int count = 0;
   for (int i = 0; i < farmTypeCount; i++)
      {
      FarmType* pFarmType = NULL;
      m_farmTypeMap.Lookup(m_farmTypeArray[i]->m_type, pFarmType);
      ASSERT(pFarmType != NULL);

      for (int j = 0; j < regionCount; j++)
         {
         CString label;
         label.Format("%s_%i", (LPCTSTR)pFarmType->m_code, m_regionsArray[j]);

         m_pFarmExpTransMatrix->SetLabel(count + 1, label);
         m_pFarmExpTransMatrix->Set(0, count, (LPCTSTR)label);

         m_pFarmTypeTransMatrix->SetLabel(count + 1, label);
         m_pFarmTypeTransMatrix->Set(0, count, (LPCTSTR)label);
         count++;
         }
      }

   for (int i = 1; i < count + 1; i++)
      for (int j = 0; j < count; j++)
         {
         m_pFarmExpTransMatrix->Set(i, j, 0);
         m_pFarmTypeTransMatrix->Set(i, j, 0);
         }

   // initialize FCT data obj
   ASSERT(m_pFCTRData == NULL);
   int fctCount = (int)m_fctArray.GetSize();
   m_pFCTRData = new FDataObj(1 + fctCount, 0);
   m_pFCTRData->SetName("Farm Type Counts By Region");
   m_pFCTRData->SetLabel(0, "Time");
   for (int i = 0; i < fctCount; i++)
      {
      FarmType* pFarmType = NULL;
      m_farmTypeMap.Lookup(m_fctArray[i]->farmType, pFarmType);
      CString label;

      ASSERT(pFarmType != NULL);
      label.Format("%s_%i", (LPCTSTR)pFarmType->m_code, m_fctArray[i]->regionID);
      //else
      //   label.Format( "UNK_%i", m_fctArray[i]->regionID );

      m_pFCTRData->SetLabel(i + 1, label);
      }

   ASSERT(m_pFarmSizeTRData == NULL);
   m_pFarmSizeTRData = new FDataObj(1 + (int)m_fstArray.GetSize(), 0);
   m_pFarmSizeTRData->SetName("Avg Farm Sizes By FarmType and Region");
   m_pFarmSizeTRData->SetLabel(0, "Time");
   for (int i = 0; i < (int)m_fstArray.GetSize(); i++)
      {
      FarmType* pFarmType = NULL;
      m_farmTypeMap.Lookup(m_fstArray[i]->farmType, pFarmType);
      CString label;

      if (pFarmType != NULL)
         label.Format("%s_%i", (LPCTSTR)pFarmType->m_code, m_fstArray[i]->regionID);
      else
         label.Format("UNK_%i", m_fstArray[i]->regionID);

      m_pFarmSizeTRData->SetLabel(i + 1, label);
      }

   // internal variables
   theProcess->m_outVarIndexFarmModel = theProcess->GetOutputVarCount();

   theProcess->AddOutputVar("Mean Yield Reduction", m_avgYieldReduction, "");
   theProcess->AddOutputVar("Avg Farm Size (ha)", m_avgFarmSizeHa, "");
   theProcess->AddOutputVar("Avg Farm Size by Farm Type", m_pFarmSizeTData, "");
   theProcess->AddOutputVar("Farm Counts by Farm Type", m_pFarmTypeCountData, "");
   theProcess->AddOutputVar("Expansion Counts by Farm Type", m_pFarmTypeExpandCountData, "");
   theProcess->AddOutputVar("Farm Counts by Farm Type and Region", m_pFCTRData, "");
   theProcess->AddOutputVar("Avg Farm Size by Farm Type and Region", m_pFarmSizeTRData, "");
   //theProcess->AddOutputVar( "Farm Expansion Transition Matrix", m_pFarmExpTransMatrix, "" );

   theProcess->AddOutputVar("Annual Adjust Field Count", m_adjFieldCount, "");
   theProcess->AddOutputVar("Annual Adjusted Field Area (ha)", m_adjFieldAreaHa, "");
   theProcess->AddOutputVar("Avg Field Size (ha)", m_avgFieldSizeHa, "");

   theProcess->AddOutputVar("Avg Field Size (ha) by LULC_B", m_pFldSizeLData, "");
   theProcess->AddOutputVar("Avg Field Size (ha) by LULC_B and Region", m_pFldSizeLRData, "");

   theProcess->AddOutputVar("Daily Data", m_pDailyData, "");

   theProcess->AddOutputVar("Crop Event Data", m_pCropEventData, "");
   if (m_outputPivotTable)
      theProcess->AddOutputVar("Crop Event Pivot Table", m_pCropEventPivotTable, "");

   theProcess->AddOutputVar("Farm Event Data", m_pFarmEventData, "");
   if (m_outputPivotTable)
      theProcess->AddOutputVar("Farm Event Pivot Table", m_pFarmEventPivotTable, "");

   // rotation-related variables
   for (int i = 0; i < (int)this->m_rotationArray.GetSize(); i++)
      {
      FarmRotation* pRotation = m_rotationArray.GetAt(i);

      CString name = pRotation->m_name;
      name += " Area (ha)";
      theProcess->AddOutputVar(name, pRotation->m_totalArea, "");  // remeber the index of the first one added

      name = pRotation->m_name;
      name += " Area (%)";
      theProcess->AddOutputVar(name, pRotation->m_totalAreaPct, "");

      name = pRotation->m_name;
      name += " Area (% of Ag)";
      theProcess->AddOutputVar(name, pRotation->m_totalAreaPctAg, "");
      }

   // get a list of all LULC values included in a rotation and do those as well
   for (int i = 0; i < (int)this->m_rotationArray.GetSize(); i++)
      {
      FarmRotation* pRotation = m_rotationArray.GetAt(i);

      for (int j = 0; j < (int)pRotation->m_sequenceArray.GetSize(); j++)
         {
         int lulc = pRotation->m_sequenceArray[j];

         // does this exist in the global array already?
         bool found = false;
         for (int k = 0; k < (int)m_rotLulcArray.GetSize(); k++)
            {
            if (m_rotLulcArray[k] == lulc)
               {
               found = true;
               break;
               }
            }

         if (!found)
            {
            m_rotLulcArray.Add(lulc);
            m_rotLulcAreaArray.Add(0);
            }
         }
      }

   for (int i = 0; i < (int)this->m_rotLulcArray.GetSize(); i++)
      {
      int lulc = m_rotLulcArray.GetAt(i);

      // find the corresponding crop
      int level = pContext->pLulcTree->FindLevelFromFieldname(m_lulcField); // level is one-based
      ASSERT(level > 0);

      LulcNode* pNode = pContext->pLulcTree->FindNode(level, lulc);
      ASSERT(pNode != NULL);

      CString name = pNode->m_name;
      name += " Area (ha)";
      theProcess->AddOutputVar(name, m_rotLulcAreaArray[i], "");
      }

   // climate indicators (output vars added in Build method)
   m_dailyCIArray.Add(BuildOutputClimateDataObj("Daily Precip (mm)", true));   // all these "Add()s" allocate a new FDataObj
   m_dailyCIArray.Add(BuildOutputClimateDataObj("Daily Tmin (C)", true));
   m_dailyCIArray.Add(BuildOutputClimateDataObj("Daily Tmean (C)", true));
   m_dailyCIArray.Add(BuildOutputClimateDataObj("Daily Tmax (C)", true));

   // note - following order must match the "output data object indices" enum in FarmModel.h
   m_annualCIArray.Add(BuildOutputClimateDataObj("Annual Precip (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Annual Tmin (C)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Annual Tmean (C)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Annual Tmax (C)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx1-Jan (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx1-Feb (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx1-Mar (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx1-Apr (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx1-May (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx1-Jun (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx1-Jul (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx1-Aug (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx1-Sep (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx1-Oct (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx1-Nov (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx1-Dec (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx3-Jan (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx3-Feb (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx3-Mar (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx3-Apr (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx3-May (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx3-Jun (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx3-Jul (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx3-Aug (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx3-Sep (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx3-Oct (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx3-Nov (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Rx3-Dec (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("CDD (days)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("R10mm (days)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("10yr Storm (events)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("100yr Storm (events)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Short Dur. Precip (mm)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Extreme Heat Events", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Extreme Cold Events", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("GSL (days)", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("CHU", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Cereal GDD", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("Alfalfa GDD", false));
   m_annualCIArray.Add(BuildOutputClimateDataObj("P-Days", false));

   // all outputs generated, fix up counter
   count = theProcess->GetOutputVarCount();
   theProcess->m_outVarCountFarmModel = count - theProcess->m_outVarIndexFarmModel;

   CString msg;
   int rotations = (int)this->m_rotationArray.GetSize();
   int lulcs = (int)this->m_rotLulcArray.GetSize();
   int outputs = rotations * 3 + lulcs;
   msg.Format("Farm Model: Added %i output variables: %i*3 rotations + %i rotational lulc classes", outputs, rotations, lulcs);
   Report::Log(msg);
   }


FDataObj* FarmModel::BuildOutputClimateDataObj(LPCTSTR label, bool isDaily)
   {
   int stationCount = m_climateManager.GetStationCount();

   FDataObj* pData = new FDataObj(1 + stationCount, 0);   // time + stations 
   if (isDaily)
      pData->SetLabel(0, "Day");
   else
      pData->SetLabel(0, "Year");

   int i;
   for (i = 0; i < stationCount; i++)
      {
      CString _label(m_climateManager.GetStation(i)->m_name);
      _label += ": ";
      _label += label;
      pData->SetLabel(i + 1, _label);
      }

   pData->SetName(label);

   theProcess->AddOutputVar(label, pData, "");

   return pData;
   }


bool FarmModel::RotateCrops(EnvContext* pContext, bool useAddDelta)
   {
   MapLayer* pLayer = (MapLayer*)pContext->pMapLayer;

   float noDataValue = pLayer->GetNoDataValue();

   // create a random number generator for ????
   RandUniform rnRotIndex(0.0, 1.0, 1);

   // initialize outputVars
   for (int i = 0; i < (int)this->m_rotationArray.GetSize(); i++)
      {
      FarmRotation* pRotation = m_rotationArray.GetAt(i);
      pRotation->m_totalArea = 0;
      pRotation->m_totalAreaPct = 0;
      pRotation->m_totalAreaPctAg = 0;
      }

   for (int i = 0; i < (int)m_rotLulcAreaArray.GetSize(); i++)
      m_rotLulcAreaArray[i] = 0;

   float areaHayPasture = 0;
   float areaCashCrop = 0;

   // basic idea - loop through Farms.
   // For each farm move each IDU in the 
   // cadastre through it rotation.
   for (int i = 0; i < (int)m_farmArray.GetSize(); i++)
      {
      Farm* pFarm = m_farmArray[i];

      if (pFarm->IsActive() == false)      // skip farms that are retired or purchased
         continue;

      for (int j = 0; j < (int)pFarm->m_fieldArray.GetSize(); j++)
         {
         FarmField* pField = pFarm->m_fieldArray[j];
         int iduCount = (int)pField->m_iduArray.GetCount();
         for (int k = 0; k < iduCount; k++)
            {
            int idu = pField->m_iduArray[k];

            int lulcA = -1;
            pLayer->GetData(idu, m_colLulcA, lulcA);

            if (lulcA != ANNUAL_CROP && lulcA != PERENNIAL_CROP)
               continue;

            int rotationID = 0;
            pLayer->GetData(idu, m_colRotation, rotationID);

            int rotIndex = 0;
            pLayer->GetData(idu, m_colRotIndex, rotIndex);

            if (rotationID <= 0)  // check for case where rotation < 0, make sure rotIndex = -1
               {
               if (rotIndex >= 0)
                  theProcess->UpdateIDU(pContext, idu, m_colRotIndex, -1, useAddDelta ? ADD_DELTA : SET_DATA);
               continue;
               }

            else
               {
               // this IDU is Ag, and is in a rotation, so rotate through...

               // first, find the farm rotation
               FarmRotation* pRotation = NULL;
               BOOL found = m_rotationMap.Lookup(rotationID, pRotation);     // get the FarmRotation from the rotationID
               if (!found || pRotation == NULL)
                  continue;      // no valid rotation found

               int lulc = -1;
               pLayer->GetData(idu, m_colLulc, lulc);
               if (lulc <= 0)   // no lulc value found
                  continue;

               float area = 0;
               pLayer->GetData(idu, m_colArea, area);

               // track areas of crops in the rotations for output
               // (Note that these are starting values, which are adjusted as needed below)
               for (int i = 0; i < (int)m_rotLulcArray.GetSize(); i++)
                  {
                  if (lulc == m_rotLulcArray[i])
                     {
                     m_rotLulcAreaArray[i] += (area / M2_PER_HA);
                     break;
                     }
                  }

               // see where we are at in the rotation.  
               // Note: if we are in a rotation, but the lulc value is invalid, then move to the next item in the rotation
               // Note: if the rotIndex < 0 , then randomly assign an lulc
               if (rotIndex >= (int)pRotation->m_sequenceArray.GetSize() - 1)// last item in sequence
                  rotIndex = 0;     // then move to start of rotation
               else if (rotIndex < 0)   // not yet assigned a value in sequence
                  {
                  // randomly assign an lulc class in this rotation
                  int rotLength = (int)pRotation->m_sequenceArray.GetSize();
                  rotIndex = (int)rnRotIndex.RandValue(0, (double)rotLength);
                  }
               else
                  rotIndex++;

               // get next LULC value in the rotation
               int nextLulc = pRotation->m_sequenceArray[rotIndex];
               if (nextLulc != lulc)
                  theProcess->UpdateIDU(pContext, idu, m_colLulc, nextLulc, useAddDelta ? ADD_DELTA : SET_DATA);

               theProcess->UpdateIDU(pContext, idu, m_colRotIndex, rotIndex, useAddDelta ? ADD_DELTA : SET_DATA);

               pRotation->m_totalArea += area / M2_PER_HA;

               for (int i = 0; i < (int)m_rotLulcArray.GetSize(); i++)
                  {
                  if (lulc == m_rotLulcArray[i])
                     m_rotLulcAreaArray[i] -= (area / M2_PER_HA);

                  if (nextLulc == m_rotLulcArray[i])
                     m_rotLulcAreaArray[i] += (area / M2_PER_HA);
                  }
               }  // end of: else ( rotationID > 0 )
            }  // end of: for each Field IDU
         }  // end of each field
      }  // for each farm

   // update rotation summaries
   for (int j = 0; j < (int)m_rotationArray.GetSize(); j++)
      {
      FarmRotation* pRotation = m_rotationArray.GetAt(j);
      pRotation->m_totalAreaPct = 100 * pRotation->m_totalArea / (m_totalIDUArea / M2_PER_HA);
      pRotation->m_totalAreaPctAg = 100 * pRotation->m_totalArea / (m_totalIDUAreaAg / M2_PER_HA);

      //CString msg;
      //msg.Format( "Crop Rotation '%s': Target=%f, Realized=%4.1f", pRotation->m_name, pRotation->m_initPctArea*100, pRotation->m_totalAreaPct );
      //Report::Log( msg );
      }

   return true;
   }


bool FarmModel::GrowCrops(EnvContext* pContext, bool useAddDelta)
   {
   // basic idea:  crop selection has been set prior to getting here.
   //  the job here is to march through an annual growing season, estimating
   //  changes in crop management and impacts on yield.  We do this farm by farm.
   //
   // Note that when we get here, the ClimateManager has already updated it's internal 
   // arrays to have a full year of weather stats/metrics.

   MapLayer* pLayer = (MapLayer*)pContext->pMapLayer;

   // start the annual clock running
   int doy = 0;
   int colStationID = m_climateManager.GetStationIDCol();

   // loop through each day of the year
   for (int doy = 1; doy <= 365; doy++)   // note: doy's are one-based
      {
      int year = 2000, month, dom;
      ::GetCalDate(doy, &year, &month, &dom, 0);

      CString msg;
      msg.Format("%s %i   ", ::GetMonthStr(month), dom);
      ::EnvSetLLMapText(msg);

      float tArea = 0;
      float tYRF = 0;


      // update individual farms
//#pragma omp parallel for
      for (int i = 0; i < (int)m_farmArray.GetSize(); i++)
         {
         Farm* pFarm = m_farmArray[i];

         if (pFarm->IsActive() == false)
            continue;

         for (int j = 0; j < (int)pFarm->m_fieldArray.GetSize(); j++)
            {
            FarmField* pField = pFarm->m_fieldArray[j];
            int iduCount = (int)pField->m_iduArray.GetCount();

            if (pField->m_iduArray.GetSize() == 0)   // any idus?
               continue;

            // get weather information for this farm
            int idu = pField->m_iduArray[0];
            int stationID = -1;

            pLayer->GetData(idu, colStationID, stationID);

            ClimateStation* pStation = m_climateManager.GetStationFromID(stationID);
            if (pStation == NULL)
               continue;

            //ASSERT( pStation == pFarm->m_pClimateStation );

            // initial daily farm data
            pFarm->m_dailyPlantingAcresAvailable = MAX_DAILY_CORN_PLANTING_AC;

            // we have a farm and associate climate, iterate through each IDU in the farm.
            // for each IDU associated with this farm, 
            for (int j = 0; j < (int)pField->m_iduArray.GetSize(); j++)
               {
               idu = pField->m_iduArray[j];

               // TODO: Verify VSMB
               // Update the current IDU's soil moisture status
               if (m_useVSMB)
                  m_vsmb.UpdateSoilMoisture(idu, pStation, year, doy);

               if (IsCrop(pFarm, pLayer))   // LULC_A =annual or perennial crop
                  {
                  float priorCumYRF = 0;
                  pLayer->GetData(idu, m_colYieldRed, priorCumYRF);

                  if (priorCumYRF < m_yrfThreshold)
                     {
                     // 0-1 yield reduction factors
                     float yrf = CheckCropConditions(pContext, pFarm, pLayer, idu, doy, pContext->currentYear, priorCumYRF);

                     // accumulate area
                     float area = 0;
                     pLayer->GetData(idu, m_colArea, area);

                     // area-weighted average
                     tYRF += yrf * area;
                     tArea += area;

                     // update priorCumYRF
                     float cumYRF = AccumulateYRFs(yrf, priorCumYRF);
                     theProcess->UpdateIDU(pContext, idu, m_colYieldRed, cumYRF, SET_DATA);

                     if (cumYRF >= m_yrfThreshold)
                        AddCropEvent(pContext, idu, CE_YIELD_FAILURE, "Yield Failure", area * M2_PER_HA, doy, 0, cumYRF);

                     }  // end of: if ( priorCumYRF < 1 )
                  }  // end of: IsAnnualCrop()
               }  // end of: for each field IDU

            }  // end of: for each Field
         }  // end of: for each farm

      m_avgYieldReduction = tYRF / tArea;

      UpdateDailyOutputs(doy, pContext);

      }  // end of: for each day of year

   /*
      else  // check for case where rotation < 0, make sure rotIndex = -1
         {
         if ( rotIndex >= 0 )
            theProcess->UpdateIDU( pContext, idu, m_colRotIndex, -1, useAddDelta ? ADD_DELTA : SET_DATA );
         }
      }

   // update rotation summaries
   for ( int j=0; j < (int) m_rotationArray.GetSize(); j++ )
      {
      FarmRotation *pRotation = m_rotationArray.GetAt( j );
      pRotation->m_totalAreaPct   = 100*pRotation->m_totalArea/( m_totalIDUArea / M2_PER_HA );
      pRotation->m_totalAreaPctAg = 100*pRotation->m_totalArea/( m_totalIDUAreaAg / M2_PER_HA );

      //CString msg;
      //msg.Format( "Crop Rotation '%s': Target=%f, Realized=%4.1f", pRotation->m_name, pRotation->m_initPctArea*100, pRotation->m_totalAreaPct );
      //Report::Log( msg );
      }
      */

   return true;
   }




int FarmModel::InitializeFarms(MapLayer* pLayer)
   {
   // this method is only called if the init flag is set to "1" in the XML input file <farm_model> tag
   //
   // it 1) decodes the Farm id string in FT_EXTENT (e.g. "Ottawas54401113CLCTY_FCO1")
   //       by searching for an '_', and parsining what is to the right of the underscore
   //    2) populates FT_CODE with the string farmtype code (e.g. FCO)
   //    3) populates FARM_TYPE with the corresponding farm type numeric code
   //    4) populates FARMID with with a unique identifier for each unique FT_EXTENT
   //    5) Populates FARMHQ with ???

   int colFTExtent = pLayer->GetFieldCol("FT_Extent");
   if (colFTExtent < 0)
      return -1;

   int colFTHQ = pLayer->GetFieldCol("FT_HQ");
   if (colFTHQ < 0)
      return -1;

   int colFarmType = pLayer->GetFieldCol("FarmType");
   if (colFarmType < 0)
      colFarmType = pLayer->m_pDbTable->AddField("FarmType", TYPE_INT);

   int colFTCode = pLayer->GetFieldCol("FT_Code");
   if (colFTCode < 0)
      colFTCode = pLayer->m_pDbTable->AddField("FT_Code", TYPE_STRING, 8, 0);

   pLayer->SetColNoData(colFarmType);
   pLayer->SetColNull(colFTCode);

   // iterate through IDUs
   CString ft;
   TCHAR buffer[64];
   int count = 0;

   CMap< CString, LPCTSTR, int, int > farmIDMap;
   int lastFarmID = 0;

   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      // get the FT_Extent entry.  This is a string like PR5440032SLCCTY_DY01
      pLayer->GetData(idu, colFTExtent, ft);

      if (ft.IsEmpty())
         continue;

      // have we seen this farm before?
      int farmID = -1;
      if (farmIDMap.Lookup((LPCTSTR)ft, farmID) == false)  // not found
         {
         // not found, so add a new one
         farmID = lastFarmID++;
         farmIDMap[ft] = farmID;
         }

      pLayer->SetData(idu, m_colFarmID, farmID);

      // is this a farm HQ idu?
      CString farmHQ;
      pLayer->GetData(idu, colFTHQ, farmHQ);

      if (farmHQ.GetLength() > 1)
         pLayer->SetData(idu, m_colFarmHQ, farmID);
      else
         pLayer->SetData(idu, m_colFarmHQ, -1);

      // start parsing FT_Code to get farm type
      lstrcpy(buffer, (LPCTSTR)ft);

      TCHAR* start = _tcschr(buffer, '_');
      start++;

      TCHAR* end = start;
      while (isalpha(*end)) end++;  // find first non-alpha character
      *end = NULL;

      pLayer->SetData(idu, colFTCode, start);

      // can we find this one in our legal list?
      bool found = false;
      for (int j = 0; j < (int)m_farmTypeArray.GetSize(); j++)
         {
         if (m_farmTypeArray[j]->m_code.CompareNoCase(start) == 0)
            {
            pLayer->SetData(idu, colFarmType, m_farmTypeArray[j]->m_type);
            count++;
            found = true;
            break;
            }
         }

      if (!found)
         pLayer->SetData(idu, colFarmType, -99);
      }  // end of: for each IDU

   CString msg;
   msg.Format("Farm Model: Initialized [FarmType] for %i of %i IDUs", count, (int)pLayer->GetRecordCount());
   Report::Log(msg);

   return 0;
   }


int FarmModel::BuildFarms(MapLayer* pLayer)
   {
   // iterate through IDU's creating and populating Farms
   if (m_colFarmID < 0)
      return -1;

   m_farmMap.RemoveAll();
   m_farmArray.RemoveAll();

   int farmID = 0;
   CArray< int, int > multiHQList;

   // iterate through IDUs
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      // get the farmID
      pLayer->GetData(idu, m_colFarmID, farmID);

      if (farmID < -2)
         continue;

      // look up the Farm based on this ID (have we seen it already?)
      Farm* pFarm = NULL;
      BOOL found = m_farmMap.Lookup(farmID, pFarm);

      if (!found)  // we haven't seen it, so add it
         {
         pFarm = new Farm;
         pFarm->m_id = farmID;
         this->m_farmArray.Add(pFarm);
         m_farmMap[farmID] = pFarm;

         // set climate station for this farm
         int stationID = -1;
         pLayer->GetData(idu, m_climateManager.GetStationIDCol(), stationID);
         pFarm->m_pClimateStation = m_climateManager.GetStationFromID(stationID);
         pFarm->m_climateStationID = stationID;

         // set cadaster for this farm
         pLayer->GetData(idu, m_colCadID, pFarm->m_cadID);

         // set farm type, assuming that the first IDU encountered contains the farm type
         int farmType = (int)FT_UNK;
         pLayer->GetData(idu, m_colFarmType, farmType);
         pFarm->m_farmType = (FARMTYPE)farmType;
         }

      FarmField* pField = new FarmField(pFarm);
      int count = (int)pFarm->m_fieldArray.Add(pField);
      pField->m_iduArray.Add(idu);
      pLayer->SetData(idu, m_colFieldID, pField->m_id);

      // create and attach soil info to this IDU
      if (m_useVSMB)
         {
         int soilIndex = -1;
         pLayer->GetData(idu, m_colSoilID, soilIndex);
         //m_vsmb.SetSoilInfo(idu, soilIndex, pFarm->m_pClimateStation );
         }
      // TODO:  Need to populate soil properties, layers according to soil type


      float area = 0;
      pLayer->GetData(idu, m_colArea, area);
      //ASSERT( area > 0 );
      pField->m_totalArea = area;
      pFarm->m_totalArea += area;

      int farmHQ = -1;
      pLayer->GetData(idu, m_colFarmHQ, farmHQ);   // farmHQ = farmID if this idu is an HQ, -1 otherwise

      // valid farm?
      if (farmHQ > 0 && pFarm->m_id >= 0)   // ignore farms that don't have a type
         {
         if (pFarm->m_farmHQ > 0)   // does this farm already have an HQ?
            multiHQList.Add(pFarm->m_id);    // then add this one to list of farms with more than one HQ
         else
            pFarm->m_farmHQ = idu;             // first (and hopefully only) HQ, set the farm's HQ to point to this idu.
         }
      }  // end of: for each IDU

   // have farms, now populate additional needed fields (no longer needed - specified in XML
   int colAvgHerd = pLayer->GetFieldCol("AVG_HERD");

   if (colAvgHerd >= 0)
      {
      int colDairyAvg = -1;
      int colBeefAvg = -1;
      int colPigsAvg = -1;
      int colPoultryAvg = -1;

      pLayer->CheckCol(colDairyAvg, "DairyAvg", TYPE_FLOAT, CC_AUTOADD);
      pLayer->CheckCol(colBeefAvg, "BeefAvg", TYPE_FLOAT, CC_AUTOADD);
      pLayer->CheckCol(colPigsAvg, "Pigs_Avg", TYPE_FLOAT, CC_AUTOADD);
      pLayer->CheckCol(colPoultryAvg, "Poultry_Av", TYPE_FLOAT, CC_AUTOADD);

      pLayer->SetColData(colDairyAvg, 0, true);
      pLayer->SetColData(colBeefAvg, 0, true);
      pLayer->SetColData(colPigsAvg, 0, true);
      pLayer->SetColData(colPoultryAvg, 0, true);

      for (int i = 0; i < (int)m_farmArray.GetSize(); i++)
         {
         Farm* pFarm = m_farmArray[i];

         int totalHerd = 0;
         float totalArea = 0;

         for (int k = 0; k < pFarm->m_fieldArray.GetSize(); k++)
            {
            FarmField* pField = pFarm->m_fieldArray[k];

            for (int j = 0; j < pField->m_iduArray.GetSize(); j++)
               {
               int idu = pField->m_iduArray[j];

               int avgHerd = 0;
               pLayer->GetData(idu, colAvgHerd, avgHerd);

               float area = 0;
               pLayer->GetData(idu, m_colArea, area);

               totalHerd += avgHerd;
               totalArea += area;
               }
            }

         // pass two - distribute animals
         // have total herd size, distribute over all IDUs
         for (int k = 0; k < pFarm->m_fieldArray.GetSize(); k++)
            {
            FarmField* pField = pFarm->m_fieldArray[k];

            for (int j = 0; j < pField->m_iduArray.GetSize(); j++)
               {
               int idu = pField->m_iduArray[j];

               float area = 0;
               pLayer->GetData(idu, m_colArea, area);

               int farmTypeID = 0;
               pLayer->GetData(idu, m_colFarmType, farmTypeID);

               FarmType* pFarmType = this->FindFarmTypeFromID(farmTypeID);

               if (pFarmType)
                  {
                  switch (pFarmType->m_type)
                     {
                     case FT_DYO:   // dairy only (unused)
                     case FT_DFC:   // Dairy and Field Crop
                     case FT_DYH:   // Dairy aned hay
                        pLayer->SetData(idu, colDairyAvg, totalHerd * area / totalArea);
                        break;

                     case FT_CCF:   // cow-calf and field crop
                     case FT_CCO:   // cow-calf only
                        pLayer->SetData(idu, colDairyAvg, totalHerd * area / totalArea);
                        break;

                     case FT_HOG:
                     case FT_HFC:
                        pLayer->SetData(idu, colDairyAvg, totalHerd * area / totalArea);
                        break;

                     case FT_PE:
                     case FT_PEF:
                        pLayer->SetData(idu, colDairyAvg, totalHerd * area / totalArea);
                        break;

                     default:
                        break;
                     }
                  }
               }
            }
         }
      }

   // make sure every farm has a HQ
   CArray< int, int > noFarmHQList;
   for (int i = 0; i < (int)m_farmArray.GetSize(); i++)
      {
      Farm* pFarm = m_farmArray[i];

      if (pFarm->m_id >= 0 && pFarm->m_farmHQ <= 0)
         {
         noFarmHQList.Add(pFarm->m_id);
         int hqIDU = pFarm->m_fieldArray[0]->m_iduArray[0];
         pFarm->m_farmHQ = hqIDU;
         pLayer->SetData(hqIDU, m_colFarmHQ, pFarm->m_id);
         }
      }

   CString msg;
   if (noFarmHQList.GetSize() > 0)
      {
      msg.Format("Farm Model: %i Farms were found with no farm headquarters.  The first IDU for each farm will be assigned as headquarters.", (int)noFarmHQList.GetSize());
      Report::LogWarning(msg);
      }

   if (multiHQList.GetSize() > 0)
      {
      msg.Format("Farm Model: %i Farms with multiple farm headquarters found. Only the first will be used.", (int)multiHQList.GetSize());
      Report::LogWarning(msg);
      }

   return (int)m_farmArray.GetSize();
   }


// only called in InitRun();
void FarmModel::AllocateInitialCropRotations(MapLayer* pLayer)
   {
   m_totalIDUArea = pLayer->GetTotalArea();
   m_totalIDUAreaAg = 0;

   // reset Rotation areas
   int rotationCount = (int)m_rotationArray.GetCount();
   float total = 0;
   for (int i = 0; i < rotationCount; i++)
      {
      FarmRotation* pRotation = m_rotationArray[i];

      pRotation->m_totalArea = 0;
      pRotation->m_totalAreaPct = 0;
      pRotation->m_totalAreaPctAg = 0;

      total += pRotation->m_initPctArea;
      }

   // this array tracks the areas for each crop in a rotation 
   for (int i = 0; i < (int)m_rotLulcArray.GetSize(); i++)
      m_rotLulcAreaArray[i] = 0;

   RandUniform rn(0.0, (double)total, 0);
   RandUniform rnRotIndex(0.0, 1.0, 1);

   pLayer->SetColNoData(m_colRotation);
   pLayer->SetColNoData(m_colRotIndex);

   // make an array of IDUs we will randomize
   int iduCount = pLayer->GetPolygonCount();
   int* iduArray = new int[iduCount];
   for (int i = 0; i < iduCount; i++)
      iduArray[i] = i;

   // randomize IDU traversal
   RandUniform rnShuffle(0, iduCount);
   ShuffleArray< int >(iduArray, iduCount, &rnShuffle);

   // okay, preliminaries accomplished.  Next, allocate the rotations.
   // Basic idea is to allocate the rotation that
   // has the largest allocation deficit as we move through IDUs,
   // expressed as a percent of the target allocation
   for (int i = 0; i < iduCount; i++)
      {
      int idu = iduArray[i];

      int lulcA = -1;
      pLayer->GetData(idu, m_colLulcA, lulcA);

      float area = 0;
      pLayer->GetData(idu, m_colArea, area);

      if (lulcA == 2)
         m_totalIDUAreaAg += area;

      int rotationID = 0;
      pLayer->GetData(idu, m_colRotation, rotationID);

      // are we in a rotation already??  If so, skip
      if (rotationID > 0)
         {
         ASSERT(0);
         continue;
         }

      // get lulc
      int lulc = -1;
      pLayer->GetData(idu, m_colLulc, lulc);

      // is this lulc in a rotation?  If so, apply an appropriate rotation - whichever has the highest pct remaining area
      FarmRotation* pBest = NULL;
      int       bestIndex = -1;
      float mostRemainingAreaSoFar = 0;

      for (int j = 0; j < rotationCount; j++)
         {
         FarmRotation* pRotation = m_rotationArray.GetAt(j);
         int lulcIndex = -1;

         // does the LULC in the coverage exist in this rotation?
         //if ( pRotation->m_sequenceMap.Lookup( lulc, lulcIndex ) )
         bool found = false;
         for (int i = 0; i < (int)pRotation->m_sequenceArray.GetSize(); i++)
            {
            if (pRotation->m_sequenceArray[i] == lulc)
               {
               found = true;
               break;
               }
            }

         if (found)
            {
            float pctRemainingArea = (pRotation->m_initPctArea - (pRotation->m_totalAreaPct / 100)) / pRotation->m_initPctArea;

            if (pctRemainingArea > mostRemainingAreaSoFar)
               {
               mostRemainingAreaSoFar = pctRemainingArea;
               pBest = pRotation;
               bestIndex = j;
               }
            }
         }

      if (pBest != NULL) // anything found for this IDU?
         {
         pLayer->SetData(idu, m_colRotation, pBest->m_rotationID);

         // randomly assign an lulc class in this rotation
         int rotLength = (int)pBest->m_sequenceArray.GetSize();
         int rotIndex = (int)rnRotIndex.RandValue(0, (double)rotLength);

         pLayer->SetData(idu, m_colRotIndex, rotIndex);

         int rotLulc = pBest->m_sequenceArray[rotIndex];
         pLayer->SetData(idu, m_colLulc, rotLulc);

         // update area summaries for this rotation         
         float area = 0;
         pLayer->GetData(idu, m_colArea, area);

         pBest->m_totalArea += (area / M2_PER_HA);
         pBest->m_totalAreaPct = 100 * pBest->m_totalArea / (m_totalIDUArea / M2_PER_HA);
         pBest->m_totalAreaPctAg = 100 * pBest->m_totalArea / (m_totalIDUAreaAg / M2_PER_HA);
         }
      }  // end of:  for ( i < iduCount )

   // update output variables   
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int lulc = -1;
      pLayer->GetData(idu, m_colLulc, lulc);

      float area = 0;
      pLayer->GetData(idu, m_colArea, area);

      for (int i = 0; i < (int)m_rotLulcArray.GetSize(); i++)
         {
         if (lulc == m_rotLulcArray[i])
            {
            m_rotLulcAreaArray[i] += (area / M2_PER_HA);
            break;
            }
         }
      }

   // final report
   for (int j = 0; j < rotationCount; j++)
      {
      FarmRotation* pRotation = m_rotationArray.GetAt(j);

      CString msg;
      msg.Format("Farm Model Crop Rotation '%s': Target=%.1f, Realized=%.1f", pRotation->m_name, pRotation->m_initPctArea * 100, pRotation->m_totalAreaPct);
      Report::Log(msg);
      }

   delete[] iduArray;
   }


float FarmModel::CheckCropConditions(EnvContext* pContext, Farm* pFarm, MapLayer* pLayer, int idu, int doy, int year, float priorCumYRF)
   {
   float yrf = 0;  // yield reduction factor ([0-1]- assume no yield reduction unless called for below

   ClimateStation *pStation = pFarm->m_pClimateStation;
   if (pStation == NULL)
      return 0;

   int lulc = -1;
   pLayer->GetData(idu, m_colLulc, lulc);

   float area = 0;
   pLayer->GetData(idu, m_colArea, area);

   float areaHa = area / M2_PER_HA;

   int cropStage = -99;
   pLayer->GetData(idu, m_colCropStage, cropStage);

   // has crop already harvested or failed?  then no need to go futher  Perennial crops????
   if (cropStage == CS_HARVESTED || cropStage == CS_FAILED)
      return yrf;

   // if we've hit the end of the year without a harvest, trigger an incomplete. 
   if (doy == 365 && cropStage == CS_PLANTED)
      {
      AddCropEvent(pContext, idu, CE_INCOMPLETE, "Incomplete", areaHa, doy, 0, priorCumYRF);
      theProcess->UpdateIDU(pContext, idu, m_colCropStage, CS_FAILED, ADD_DELTA);
      return yrf;
      }

   switch (lulc)
      {
      case CORN:
         {
         float chu = pStation->m_chuCornMay1[doy - 1];

         // check crop stage to see if we need to remember the doy we 
         // entered a crop stage we want to pay attention to.
         if (cropStage < CS_HARVESTED)
            {
            if (600 <= chu && chu < 1300 && cropStage != CS_LATEVEGETATION)
               {
               pLayer->SetData(idu, m_colCropStage, CS_LATEVEGETATION);
               cropStage = CS_LATEVEGETATION;
               }
            else if (1300 <= chu && chu < 1600 && cropStage != CS_POLLINATION)
               {
               pLayer->SetData(idu, m_colCropStage, CS_POLLINATION);
               cropStage = CS_POLLINATION;
               }
            else if (1601 <= chu && chu < 2000 && cropStage != CS_REPRODUCTIVE)
               {
               pLayer->SetData(idu, m_colCropStage, CS_REPRODUCTIVE);
               cropStage = CS_REPRODUCTIVE;
               }
            }  // end of: if ( cropStage < CS_HARVEST )

         // planting effects
         // note: Wet, or cold + wet, can delay planting, so check these.
         // If it is just Cold (not Wet), then delay until May 1 and plant,
         // unless you are still getting freezing temperatures at night (Tmin<0). 
         // If Tmin > 0 in May and it's dry, then planting begins.

         float tMin = 0, tMax = 0;
         pStation->GetData(doy, year, TMIN, tMin);
         pStation->GetData(doy, year, TMAX, tMax);
         float tMean = (tMax + tMin) / 2.0f;

         float precip = 0;
         pStation->GetData(doy, year, PRECIP, precip);

         if (doy == MAY24 && cropStage < CS_PLANTED)   // hit may 24, and still no planting?
            {                                            // switch to soybeans
            theProcess->UpdateIDU(pContext, idu, m_colLulc, SOYBEANS, ADD_DELTA | SET_DATA);
            yrf = AddCropEvent(pContext, idu, CE_CORN_TO_SOY, "Corn to Soy", areaHa, doy, 0, priorCumYRF);
            return yrf;
            }

         // check for planting conditions
         if (doy >= MAY1 && cropStage == CS_PREPLANT && pFarm->m_dailyPlantingAcresAvailable > (area * ACRE_PER_M2))
            {
            bool plant = true;         // is it okay to plant?

            // do we need to delay another day?  first, check wetness

            // TODO: VSMB Entry point  > 80% field capacity to wet to seed

            int cropEvent = CE_COOL_SPRING;

            if (m_useVSMB)
               {
               /////float fc = m_vsmb.GetFieldCapacity(idu);
               /////float availWater = m_vsmb.GetAvailableWater(idu);
               /////
               /////float clayContent;
               /////pLayer->GetData(m_colClayContent, idu, clayContent);
               /////
               /////float fcThreshold = 0.9f;
               /////
               /////if (clayContent <= 0.4f)
               /////   fcThreshold = 0.8f;
               /////  
               /////if ((availWater/fc) > fcThreshold)
               /////   {
               /////   plant = false;
               /////   cropEvent = CE_EARLY_FLOOD_DELAY_CORN_PLANTING;
               /////   }
               }
            else
               {
               // get last weeks weather
               float weeklyPrecip = 0;
               float historicPrecip = 0;
               for (int i = 0; i < 7; i++)
                  {
                  float _precip = 0;
                  pStation->GetData(doy - i - 1, year, PRECIP, _precip);
                  weeklyPrecip += _precip;

                  pStation->GetHistoricMean(doy - i - 1, PRECIP, _precip);
                  historicPrecip += _precip;
                  }

               if (weeklyPrecip > (1.3 * historicPrecip))
                  {
                  plant = false;
                  cropEvent = CE_EARLY_FLOOD_DELAY_CORN_PLANTING;
                  }
               }

            // check temperatures - freezing weather at night?
            // if ( May 1 and tmean (prior 14 days < 10 ) then trigger COLD_SPRING event, apply one weeks worth of yield reduction,
            // and then ignore until MAY8
            if (tMean < 10)
               plant = false;

            // can we plant?
            if (plant)
               {
               // yes, so reduce from daily planting acreage and trigger planting events
               pFarm->m_dailyPlantingAcresAvailable -= (area * ACRE_PER_M2);
               theProcess->UpdateIDU(pContext, idu, m_colCropStage, CS_PLANTED, SET_DATA);
               theProcess->UpdateIDU(pContext, idu, m_colPlantDate, doy, SET_DATA);
               yrf = AddCropEvent(pContext, idu, CE_PLANTED, "Planted", areaHa, doy, 0, priorCumYRF);
               }
            else
               {  // not yet, and reduce yield appropriately
               yrf = 0.01f;

               if (doy >= JUN1)
                  yrf = 0.02f;

               AddCropEvent(pContext, idu, cropEvent, NULL, areaHa, doy, yrf, priorCumYRF);
               }
            }  // end of: if ( pre-planting and doy > MAY1 )

         // switch to fallow if crop has failed before Jun 10
         if (doy == JUN10 && cropStage == CS_FAILED)
            {
            theProcess->UpdateIDU(pContext, idu, m_colLulc, FALLOW, ADD_DELTA | SET_DATA);
            theProcess->UpdateIDU(pContext, idu, m_colCropStage, CS_PREPLANT, ADD_DELTA | SET_DATA);
            AddCropEvent(pContext, idu, CE_CORN_TO_FALLOW, "Corn to Fallow", areaHa, doy, 0, priorCumYRF);
            return yrf;
            }

         // crop growing? then check production effects
         if (cropStage >= CS_PLANTED && cropStage < CS_HARVESTED)
            {
            if (chu >= 2800)
               {
               theProcess->UpdateIDU(pContext, idu, m_colCropStage, CS_HARVESTED, SET_DATA);
               AddCropEvent(pContext, idu, CE_HARVEST, "Harvest", areaHa, doy, 0, priorCumYRF);
               return yrf;
               }

            // killing frost A: Tmin <=-2 C with <680 accumulated CHUs
            if (chu < 680 && tMin <= -2.0f)
               yrf = AddCropEvent(pContext, idu, CE_EARLY_FROST, "Early Frost", areaHa, doy, (float)m_rn.RandValue(0, 0.08f), priorCumYRF);

            // Early flooding: Weekly Precipitation 30% greater than weekly Mean Precipitation (between May 16 and June 20)

            // TODO: VSMB???  soil saturated?

            if (MAY15 < doy && doy <= JUN20)
               {
               float weeklyPrecip = 0;
               float historicPrecip = 0;
               for (int i = 0; i < 7; i++)
                  {
                  float _precip = 0;
                  pStation->GetData(doy - i - 1, year, PRECIP, _precip);
                  weeklyPrecip += _precip;

                  pStation->GetHistoricMean(doy - i - 1, PRECIP, _precip);
                  historicPrecip += _precip;
                  }

               // add - if ( tMean for the 7 days ), 1 and 2 day events.
               if (weeklyPrecip > (1.3 * historicPrecip))
                  {
                  yrf = AddCropEvent(pContext, idu, CE_EARLY_FLOOD_KILL, "Early Flood Kill", areaHa, doy, 1.0f, priorCumYRF);
                  theProcess->UpdateIDU(pContext, idu, m_colCropStage, CS_FAILED, ADD_DELTA | SET_DATA);
                  return yrf;
                  }
               }

            // killing frost B (doesn't actually kill, just reduces yield substantially)
            if (chu > 930 && chu < 1310 && tMin <= -2)
               yrf = AddCropEvent(pContext, idu, CE_EARLY_FROST, "Early Frost", areaHa, doy, (float)m_rn.RandValue(0.3f, 0.5f), priorCumYRF);

            // drought A (CDD >10 and CHU between 600 and 1,300)

            // TODO: VSMB????

            if (doy == pStation->m_doyCHU1300)
               {
               int count = 0;
               for (int day = pStation->m_doyCHU600; day <= doy; day++)
                  {
                  float _precip = 0;
                  pStation->GetData(day, year, PRECIP, _precip);

                  if (_precip < 1.0f)
                     count++;

                  if (count >= 10)
                     break;
                  }

               if (count >= 10)
                  yrf = AddCropEvent(pContext, idu, CE_VEG_DROUGHT, "Veg Drought", areaHa, doy, (float)m_rn.RandValue(0.10f, 0.20f), priorCumYRF);
               }

            // drought B (pollination): CDD >10 and CHU between 1,301 and 1,600.           
            if (doy == pStation->m_doyCHU1600)
               {
               int count = 0;
               for (int day = pStation->m_doyCHU1300; day <= doy; day++)
                  {
                  float _precip = 0;
                  pStation->GetData(day, year, PRECIP, _precip);

                  if (_precip < 1.0f)
                     count++;

                  if (count >= 10)
                     break;
                  }

               if (count >= 10)
                  yrf = AddCropEvent(pContext, idu, CE_POL_DROUGHT, "POL Drought", areaHa, doy, 0.50f, priorCumYRF);
               }

            // lethal pollen heat: Tmax >=35 C and CHU between 1,450-1,550
            if (chu > 1450 && chu < 1550 && tMax >= 35.0f)
               yrf = AddCropEvent(pContext, idu, CE_POL_HEAT, "POL Heat", areaHa, doy, 0.15f, priorCumYRF);

            // drought C: P<45mm at CHUs between 1,601 and 1,825
            if (doy == pStation->m_doyCHU1826)
               {
               int startDOY = pStation->m_doyCHU1600;
               float totalPrecip = 0;
               for (int day = startDOY; day <= doy; day++)
                  {
                  float precip = 0;
                  pStation->GetData(day, year, PRECIP, precip);
                  totalPrecip += precip;
                  }

               if (totalPrecip < 45.0f)
                  yrf = AddCropEvent(pContext, idu, CE_R2_DROUGHT, "R2 Drought", areaHa, doy, 0.15f, priorCumYRF);
               }

            // heat stress during early R stages - Tmax >=33 C for 6+ days at CHUs between 1,601 and 2,000
            if (doy == pStation->m_doyCHU2000)
               {
               int startDOY = pStation->m_doyCHU1600 + 1;
               float hotDays = 0;
               for (int day = startDOY; day <= doy; day++)
                  {
                  float tMax = 0;
                  pStation->GetData(day, year, TMAX, tMax);

                  if (tMax > 33)
                     hotDays++;

                  if (hotDays >= 6)
                     break;
                  }

               if (hotDays >= 6)
                  {
                  // in addition to other stresses
                  yrf = AddCropEvent(pContext, idu, CE_R2_HEAT ,"R2 Heat", areaHa, doy, yrf + 0.04f, priorCumYRF);
                  }
               }

            // drought D: P<45mm at CHUs between 1,826 and 2,000

            // TODO: VSMB???

            if (doy == pStation->m_doyCHU2000)
               {
               int start = pStation->m_doyCHU1826;
               float totalPrecip = 0;

               for (int day = start; day <= doy; day++)
                  {
                  float precip = 0;
                  pStation->GetData(day, year, PRECIP, precip);
                  totalPrecip += precip;
                  }

               if (totalPrecip < 45)
                  {
                  float yrf = -(0.2f / 45) * totalPrecip + 0.2f;
                  yrf = AddCropEvent(pContext, idu, CE_R3_DROUGHT, "R3 Drought", areaHa, doy, yrf, priorCumYRF);  // apply linear relationships with 0-45
                  }
               }

            // killing frost C -Tmin <=-2 C with 2,165 to 2,475 accumulated CHUs;
            if (2165 < chu && chu < 2475 && tMin <= -2.0f)
               yrf = AddCropEvent(pContext, idu, CE_EARLY_FROST, "Early Frost", areaHa, doy, 0.50f, priorCumYRF);


            // TODO:  VSMB?

            // Drought E: P<8mm at CHUs between 2,001 and 2,165
            if (doy == pStation->m_doyCHU2165)
               {
               int start = pStation->m_doyCHU2000;
               float totalPrecip = 0;

               for (int day = start; day <= doy; day++)
                  {
                  float precip = 0;
                  pStation->GetData(day, year, PRECIP, precip);
                  totalPrecip += precip;
                  }

               if (totalPrecip < 8.0f)
                  {
                  float yrf = -(0.2f / 8) * totalPrecip + 0.2f;
                  yrf = AddCropEvent(pContext, idu, CE_R3_DROUGHT, "R3 Drought", areaHa, doy, yrf, priorCumYRF);  // linear function
                  }
               }

            // Killing frost D: Tmin <=-2 C with 2,476 to 2,799 accumulated CHUs; 
            if (2476 < chu && chu <= 2600 && tMin <= -2.0f)
               yrf = AddCropEvent(pContext, idu, CE_FALL_FROST, "Fall Frost", areaHa, doy, 0.20f, priorCumYRF);
            else if (2601 < chu && chu <= 2799 && tMin <= -2.0f)
               yrf = AddCropEvent(pContext, idu, CE_FALL_FROST, "Fall Frost", areaHa, doy, 0.05f, priorCumYRF);

            // TODO? VSMB

            // Drought F: P<16mm at CHUs between 2,166 and 2,475 
            if (doy == pStation->m_doyCHU2475)
               {
               int start = pStation->m_doyCHU2000;
               float totalPrecip = 0;

               for (int day = start; day <= doy; day++)
                  {
                  float precip = 0;
                  pStation->GetData(day, year, PRECIP, precip);
                  totalPrecip += precip;
                  }

               if (totalPrecip < 16.0f)
                  {
                  float yrf = -(0.15f / 16) * totalPrecip + 0.15f;
                  yrf = AddCropEvent(pContext, idu, CE_R5_DROUGHT,"R5 Drought", areaHa, doy, yrf, priorCumYRF);  // apply linear function 0-15
                  }
               }
            }  // end of: if ( cropStage >= CS_PLANTED && cropStage < CS_HARVESTED )  

         return yrf;
         }

      case SOYBEANS:
         {
         if (doy == JUN10 && cropStage < CS_PLANTED)   // hit Jun 10, and still not going?
            {                                            // switch to fallow
            theProcess->UpdateIDU(pContext, idu, m_colLulc, FALLOW, ADD_DELTA | SET_DATA);
            theProcess->UpdateIDU(pContext, idu, m_colCropStage, CS_PREPLANT, SET_DATA);
            AddCropEvent(pContext, idu, CE_SOY_TO_FALLOW, "Soy To Fallow", areaHa, doy, 0, priorCumYRF);
            return yrf;
            }

         if (doy >= MAY10 && cropStage < CS_PLANTED && pFarm->m_dailyPlantingAcresAvailable >= (area * ACRE_PER_M2))  // check planting conditions
            {
            // TODO: VSMB?
            // if Weekly Precipitation 30% greater than weekly 
            // Mean Precipitation (weeks between May 1 and May 24), delay planting
            float weeklyPrecip = 0, weeklyHistoricPrecip = 0;
            for (int i = 0; i < 7; i++)      // look back one week
               {
               float precip = 0;
               pStation->GetData(doy - i, year, PRECIP, precip);
               weeklyPrecip += precip;

               float historicPrecip = 0;
               pStation->GetHistoricMean(doy, PRECIP, historicPrecip);
               weeklyHistoricPrecip += historicPrecip;
               }

            if (weeklyPrecip < (weeklyHistoricPrecip * 1.30f))
               {
               // go ahead and plant the crop
               theProcess->UpdateIDU(pContext, idu, m_colCropStage, CS_PLANTED, SET_DATA);
               theProcess->UpdateIDU(pContext, idu, m_colPlantDate, doy, SET_DATA);
               AddCropEvent(pContext, idu, CE_PLANTED, "Planted", areaHa, doy, 0, priorCumYRF);
               cropStage = CS_PLANTED;

               pFarm->m_dailyPlantingAcresAvailable -= (area * ACRE_PER_M2);
               }
            else  // delay - too wet
               {
               if (doy > MAY24)
                  yrf = AddCropEvent(pContext, idu, CE_EARLY_FLOOD_DELAY_SOY_PLANTING, "Early Flood Delay Soy Planting", areaHa, doy, 0.08f, priorCumYRF);

               if (doy > JUN7)
                  yrf = AddCropEvent(pContext, idu, CE_EARLY_FLOOD_DELAY_SOY_PLANTING, "Early Flood Delay Soy Planting", areaHa, doy, 0.15f, priorCumYRF);
               }
            }  // end of: time to plant?

         // time to harvest?
         if (CS_PLANTED <= cropStage && cropStage < CS_HARVESTED)
            {
            int plantDate = 0;
            pLayer->GetData(idu, m_colPlantDate, plantDate);

            if (doy == plantDate + 130)
               {
               theProcess->UpdateIDU(pContext, idu, m_colCropStage, CS_HARVESTED, SET_DATA);
               AddCropEvent(pContext, idu, CE_HARVEST, "Harvest", areaHa, doy, 0, priorCumYRF);
               return yrf;
               }

            // haven't harvested yet, so continue checking conditions
            float tMin = 0, tMax = 0;
            pStation->GetData(doy, year, TMIN, tMin);
            pStation->GetData(doy, year, TMAX, tMax);

            // Spring killing frost: Tmin < 0C 26+ days after seeding
            if (doy >= plantDate + 26 && doy < plantDate + 33 && tMin < 0)
               {
               theProcess->UpdateIDU(pContext, idu, m_colCropStage, CS_FAILED, SET_DATA);
               yrf = AddCropEvent(pContext, idu, CE_EARLY_FROST, "Early Frost", areaHa, doy, 1.0f, priorCumYRF);
               return yrf;
               }

            // TODO: VSMB?

            // Flooding A: Precipitation 30% greater than mean precipitation 25 to 35 days after seeding; 
            if (doy == plantDate + 35)
               {
               float totalPrecip = 0, totalHistoricPrecip = 0;
               for (int i = plantDate + 25; i <= doy; i++)
                  {
                  float precip = 0;
                  pStation->GetData(i, year, PRECIP, precip);
                  totalPrecip += precip;

                  float historicPrecip = 0;
                  pStation->GetHistoricMean(doy, PRECIP, historicPrecip);
                  totalHistoricPrecip += historicPrecip;
                  }

               if (totalPrecip > (totalHistoricPrecip * 1.30f))
                  yrf = AddCropEvent(pContext, idu, CE_EARLY_FLOOD_DELAY_SOY_A, "Early Flood Delay Soy A", areaHa, doy, 0.20f, priorCumYRF);
               }

            // Flooding B: Precipitation 30% greater than weekly precipitation 25 to 45 days after seeding
            if (doy == plantDate + 45)
               {
               float totalPrecip = 0, totalHistoricPrecip = 0;
               for (int i = plantDate + 25; i <= doy; i++)
                  {
                  float precip = 0;
                  pStation->GetData(i, year, PRECIP, precip);
                  totalPrecip += precip;

                  float historicPrecip = 0;
                  pStation->GetHistoricMean(doy, PRECIP, historicPrecip);
                  totalHistoricPrecip += historicPrecip;
                  }

               if (totalPrecip > (totalHistoricPrecip * 1.30f))
                  yrf = AddCropEvent(pContext, idu, CE_EARLY_FLOOD_DELAY_SOY_B, "Early Flood Delay Soy B", areaHa, doy, (float)m_rn.RandValue(0, 0.90f), priorCumYRF);  // linear function?
               }

            // Flooding C: Precipitation 30% greater than mean precipitation 40 to 45 days after seeding
            if (doy == plantDate + 45)
               {
               float totalPrecip = 0, totalHistoricPrecip = 0;
               for (int i = plantDate + 40; i <= doy; i++)
                  {
                  float precip = 0;
                  pStation->GetData(i, year, PRECIP, precip);
                  totalPrecip += precip;

                  float historicPrecip = 0;
                  pStation->GetHistoricMean(doy, PRECIP, historicPrecip);
                  totalHistoricPrecip += historicPrecip;
                  }

               if (totalPrecip > (totalHistoricPrecip * 1.30f))
                  yrf = AddCropEvent(pContext, idu, CE_EARLY_FLOOD_DELAY_SOY_C, "Early Flood Delay Soy C", areaHa, doy, (float)m_rn.RandValue(0, 0.18f), priorCumYRF);
               }

            // Cool nights: Tmin  <10C for 5+ days 45-55 days after seeding
            if (doy == plantDate + 55)
               {
               int days = 0;
               for (int day = plantDate + 45; day <= doy; day++)
                  {
                  float _tMin = 0;
                  pStation->GetData(day, year, TMIN, _tMin);

                  if (_tMin < 10.0f)
                     days++;
                  }

               if (days > 5)
                  {
                  float yrf = 5.0f * (days - 5);
                  if (yrf < 0) yrf = 0;
                  if (yrf > 1) yrf = 1;

                  yrf = AddCropEvent(pContext, idu, CE_COOL_NIGHTS, "Cool Nights", areaHa, doy, yrf, priorCumYRF);  // linear days or less=0,, 5-10 is linear between (0, 25)  etc...
                  }
               }

            // Killing frost B: Tmin<0C 56 to 100 days after seeding; 
            if (tMin < 0 && doy >= plantDate + 56 && doy <= plantDate + 100)
               yrf = AddCropEvent(pContext, idu, CE_MID_FROST, "Mid Frost", areaHa, doy, (float)m_rn.RandValue(0, 0.80f), priorCumYRF);

            // Cold nights: 1C<Tmin<10C 55 to 100 days after seeding; 
            if (tMin < 10 && doy >= plantDate + 56 && doy <= plantDate + 100)
               yrf = AddCropEvent(pContext, idu, CE_COOL_NIGHTS, "Cool Nights", areaHa, doy, 0.10f, priorCumYRF);

            // Warm nights: Tmin>=24C 55 to 100 days after seeding
            if (tMin > 24 && doy >= plantDate + 56 && doy <= plantDate + 100)
               yrf = AddCropEvent(pContext, idu, CE_WARM_NIGHTS, "Warm Nights", areaHa, doy, 0.25f, priorCumYRF);

            // TODO: VSMB?

            // Flooding D: Precipitation >90mm 60 to 80 days after seeding
            if (doy == plantDate + 80)
               {
               float tPrecip = 0;
               for (int day = plantDate + 60; day < doy; day++)
                  {
                  float precip = 0;
                  pStation->GetData(day, year, PRECIP, precip);
                  tPrecip += precip;
                  }

               if (tPrecip > 90)
                  yrf = AddCropEvent(pContext, idu, CE_MID_FLOOD, "Mid Flood", areaHa, doy, (float)m_rn.RandValue(0, 0.26f), priorCumYRF);
               }

            // Drought A: Precipitation <10mm 65 to 80 days after seeding
            if (doy == plantDate + 80)
               {
               float tPrecip = 0;
               for (int day = plantDate + 65; day < doy; day++)
                  {
                  float precip = 0;
                  pStation->GetData(day, year, PRECIP, precip);
                  tPrecip += precip;
                  }

               if (tPrecip < 10)
                  yrf = AddCropEvent(pContext, idu, CE_POD_DROUGHT,"POD Drought", areaHa, doy, 0.19f, priorCumYRF);
               }

            // Drought B: Precipitation <10mm 81 to 95 days after seeding
            if (doy == plantDate + 95)
               {
               float tPrecip = 0;
               for (int day = plantDate + 81; day < doy; day++)
                  {
                  float precip = 0;
                  pStation->GetData(day, year, PRECIP, precip);
                  tPrecip += precip;
                  }

               if (tPrecip < 10)
                  yrf = AddCropEvent(pContext, idu, CE_POD_DROUGHT, "POD Drought", areaHa, doy, 0.36f, priorCumYRF);
               }

            // Fall frost A: Tmin <-1C between 90 and 100 days after seeding; 
            if (tMin < -1 && doy >= plantDate + 90 && doy <= plantDate + 100)
               yrf = AddCropEvent(pContext, idu, CE_FALL_FROST, "Fall Frost", areaHa, doy, (float)m_rn.RandValue(0, 0.65f), priorCumYRF);

            // Extreme heat: mean Tmax > 33C 95-120 days after seeding
            if (tMax < 33 && doy >= plantDate + 95 && doy <= plantDate + 120)
               yrf = AddCropEvent(pContext, idu, CE_EXTREME_HEAT, "Extreme Heat", areaHa, doy, 0.22f, priorCumYRF);

            // Fall frost B: Tmin <-1C 101 to 110 days after seeding; 
            int coldAndDrought = 0;
            if (tMin < -1 && doy >= plantDate + 101 && doy <= plantDate + 110)
               yrf = AddCropEvent(pContext, idu, CE_FALL_FROST, "Fall Frost", areaHa, doy, (float)m_rn.RandValue(0, 0.37f), priorCumYRF);

            // TODO: VSMB?

            // Drought C: P<5mm 96-115 days after seeding
            if (doy == plantDate + 115)
               {
               float tPrecip = 0;
               for (int day = plantDate + 96; day < doy; day++)
                  {
                  float precip = 0;
                  pStation->GetData(day, year, PRECIP, precip);
                  tPrecip += precip;
                  }

               if (tPrecip < 5)
                  {
                  coldAndDrought += 2;
                  yrf = AddCropEvent(pContext, idu, CE_SEED_DROUGHT, "Seed Drought", areaHa, doy, (float)m_rn.RandValue(0, 0.37f), priorCumYRF);
                  }
               }

            if (coldAndDrought)
               yrf = 0.42f;

            // Fall frost C: Tmin <-1C 111 to 120 days after seeding
            if (tMin < -1 && doy >= plantDate + 111 && doy <= plantDate + 120)
               yrf = AddCropEvent(pContext, idu, CE_FALL_FROST, "Fall Frost", areaHa, doy, (float)m_rn.RandValue(0, 0.11f), priorCumYRF);
            }
         }
         break;

      case ALFALFA:
         {
         // General approach: Start tracking cGDD on April 1.  Harvest at 375 GDD.
         // Delay harvest until you have 2 successive dry days.
         // No harvest is allowed after Sept 1.

         // Late spring frost (TMin <=-3C between May 1 and June 30)
         // no action required, since this is reflected in the CDD calculations

         if (doy < APR1)
            return 0;

         // first cut "plant date" is April 1
         if (doy == APR1)
            {
            // go ahead and plant the crop
            theProcess->UpdateIDU(pContext, idu, m_colCropStage, CS_PLANTED, SET_DATA);
            theProcess->UpdateIDU(pContext, idu, m_colPlantDate, doy, SET_DATA);
            AddCropEvent(pContext, idu, CE_PLANTED,"Planted",  areaHa, doy, 0, priorCumYRF);
            return 0;
            }

         if (doy == SEP1)
            {
            theProcess->UpdateIDU(pContext, idu, m_colCropStage, CS_HARVESTED, SET_DATA);
            AddCropEvent(pContext, idu, CE_HARVEST, "Harvest", areaHa, doy, 0, priorCumYRF);
            return 0;
            }

         // crop actively growing...
         int cropStage = -99;
         pLayer->GetData(idu, m_colCropStage, cropStage);

         if (CS_PLANTED <= cropStage && cropStage < CS_HARVESTED)
            {
            int plantDate = -1;
            pLayer->GetData(idu, m_colPlantDate, plantDate);

            float gddNow = 0, gddStart = 0;;
            pStation->GetData(doy, pContext->currentYear, GDD5APR1, gddNow);
            pStation->GetData(plantDate, pContext->currentYear, GDD5APR1, gddStart);

            float periodGDD = gddNow - gddStart;

            if (periodGDD > 375)
               {
               // can we cut?  need more than 2 preceding dry days
               float precip0 = 0, precip1 = 0;
               pStation->GetData(doy, pContext->currentYear, PRECIP, precip0);
               pStation->GetData(doy - 1, pContext->currentYear, PRECIP, precip1);

               if (precip0 < 1.0f && precip1 < 1.0f)   // 1mm threshold
                  {
                  theProcess->UpdateIDU(pContext, idu, m_colPlantDate, doy, SET_DATA);

                  // get period yield reduction 
                  // Temperature stress: If TempMean <21, TempStr = 1-((TempMean -21)/(5-21))^2
                  //                     If 21<TempMean<27, TempStr = 1
                  //                     If TempMean >27, TempStr = 1-((TempMean-27)/(35-27))^2
                  float periodTMean = 0;
                  int   period = 0;
                  for (int day = plantDate; day <= doy; day++)
                     {
                     float tMean = 0;
                     pStation->GetData(day, year, TMEAN, tMean);
                     periodTMean += tMean;
                     period++;
                     }

                  periodTMean /= period;

                  float tStress = 0;
                  if (periodTMean <= 21)
                     {
                     float f = (periodTMean - 21) / (5 - 21);
                     tStress = 1 - (f * f);
                     yrf = AddCropEvent(pContext, idu, CE_COOL_NIGHTS, "Cool Nights", areaHa, doy, 1 - tStress, priorCumYRF);
                     }
                  else if (periodTMean <= 27)
                     tStress = 1;
                  else    // periodTMean > 27
                     {
                     float f = (periodTMean - 27) / (35 - 27);
                     tStress = 1 - (f * f);
                     yrf = AddCropEvent(pContext, idu, CE_WARM_NIGHTS, "Warm Nights", areaHa, doy, 1 - tStress, priorCumYRF);
                     }
                  }  // end of: if ( cut alfalfa )
               }  // end of:  if ( periodGDD > 375
            }  // end of: if ( planted and not harvested )

         return yrf;
         }

      case SPRING_WHEAT:
         {
         // pre-emergence?
         if (cropStage < CS_PLANTED)  // check planting conditions
            {
            // After MAY1, switch to corn
            if (doy > MAY1)
               {
               theProcess->UpdateIDU(pContext, idu, m_colLulc, CORN, ADD_DELTA | SET_DATA);
               AddCropEvent(pContext, idu, CE_WHEAT_TO_CORN, "Wheat to Corn", areaHa, doy, 0, priorCumYRF);
               return 0;
               }
            else if (doy > 8)  // allow for 7 days preceding
               {
               // TODO: VSMB?

               // If greater than 30 Precent above historic precip, then delay.
               float weeklyPrecip = 0;
               float historicPrecip = 0;
               for (int i = 0; i < 7; i++)
                  {
                  float _precip = 0;
                  pStation->GetData(doy - i - 1, year, PRECIP, _precip);
                  weeklyPrecip += _precip;

                  pStation->GetHistoricMean(doy - i - 1, PRECIP, _precip);
                  historicPrecip += _precip;
                  }

               if (weeklyPrecip > (1.3 * historicPrecip))
                  return 0;
               }

            float gdd = pStation->m_cumDegDays0Apr15[doy - 1];
            if (gdd > 145)   // time to plant?
               {
               theProcess->UpdateIDU(pContext, idu, m_colCropStage, CS_PLANTED, SET_DATA);
               theProcess->UpdateIDU(pContext, idu, m_colPlantDate, doy, SET_DATA);
               AddCropEvent(pContext, idu, CE_PLANTED, "Planted", areaHa, doy, 0, priorCumYRF);
               cropStage = CS_PLANTED;
               }
            }

         // post-emergence?
         if (CS_PLANTED <= cropStage && cropStage < CS_HARVESTED)
            {
            float gdd = pStation->m_cumDegDays0Apr15[doy - 1];
            if (gdd > 1600)
               {
               theProcess->UpdateIDU(pContext, idu, m_colCropStage, CS_HARVESTED, SET_DATA);
               AddCropEvent(pContext, idu, CE_HARVEST, "Harvest", areaHa, doy, 0, priorCumYRF);
               return 0;
               }
            }

         return yrf;
         }

      //case BARLEY:
      //   {
      //   // pre-emergence?
      //   if (cropStage < CS_PLANTED)  // check planting conditions
      //      {
      //      // After MAY1, switch to corn
      //      if (doy > MAY1)
      //         {
      //         theProcess->UpdateIDU(pContext, idu, m_colLulc, CORN, ADD_DELTA | SET_DATA);
      //         AddCropEvent(pContext, idu, CE_BARLEY_TO_CORN, "Barley to Corn",  areaHa, doy, 0, priorCumYRF);
      //         return 0;
      //         }
      //      else if (doy > 8)
      //         {
      //         // TODO: VSMB?
      //
      //         // If greater than 30 Precent above historic precip, then delay.
      //         float weeklyPrecip = 0;
      //         float historicPrecip = 0;
      //         for (int i = 0; i < 7; i++)
      //            {
      //            float _precip = 0;
      //            pStation->GetData(doy - i - 1, year, PRECIP, _precip);
      //            weeklyPrecip += _precip;
      //
      //            pStation->GetHistoricMean(doy - i - 1, PRECIP, _precip);
      //            historicPrecip += _precip;
      //            }
      //
      //         if (weeklyPrecip > (1.3 * historicPrecip))
      //            return 0;
      //         }
      //
      //      float gdd = pStation->m_cumDegDays0Apr15[doy - 1];
      //      if (gdd > 130)   // time to plant?
      //         {
      //         theProcess->UpdateIDU(pContext, idu, m_colCropStage, CS_PLANTED, SET_DATA);
      //         theProcess->UpdateIDU(pContext, idu, m_colPlantDate, doy, SET_DATA);
      //         AddCropEvent(pContext, idu, CE_PLANTED, "Planted", areaHa, doy, 0, priorCumYRF);
      //         cropStage = CS_PLANTED;
      //         }
      //      }
      //
      //   // post-emergence?
      //   if (CS_PLANTED <= cropStage && cropStage < CS_HARVESTED)
      //      {
      //      float gdd = pStation->m_cumDegDays0Apr15[doy - 1];
      //      if (gdd > 1400)
      //         {
      //         theProcess->UpdateIDU(pContext, idu, m_colCropStage, CS_HARVESTED, SET_DATA);
      //         AddCropEvent(pContext, idu, CE_HARVEST, "Harvest", areaHa, doy, 0, priorCumYRF);
      //         return 0;
      //         }
      //      }
      //   return yrf;
      //   }

      //case POTATOES:
      //   {
      //   // pre-emergence?
      //   if (cropStage < CS_PLANTED && doy >= APR15)
      //      {
      //      float pDays = 0;
      //      pStation->GetData(doy, year, PDAYS, pDays);
      //
      //      if (pDays > 155)   // time to plant? (assume late maturing variety)
      //         {
      //         theProcess->UpdateIDU(pContext, idu, m_colCropStage, CS_PLANTED, SET_DATA);
      //         theProcess->UpdateIDU(pContext, idu, m_colPlantDate, doy, SET_DATA);
      //         AddCropEvent(pContext, idu, CE_PLANTED, "Planted", areaHa, doy, 0, priorCumYRF);
      //         return 0;
      //         }
      //      }
      //
      //   // post-emergence?
      //   if (CS_PLANTED <= cropStage && cropStage < CS_HARVESTED)
      //      {
      //      float pDays = 0;
      //      pStation->GetData(doy, year, PDAYS, pDays);
      //
      //      if (pDays > 1000)  // assumes late maturing variety
      //         {
      //         theProcess->UpdateIDU(pContext, idu, m_colCropStage, CS_HARVESTED, SET_DATA);
      //         AddCropEvent(pContext, idu, CE_HARVEST, "Harvest", areaHa, doy, 0, priorCumYRF);
      //         return 0;
      //         }
      //      }
      //
      //   return yrf;
      //   }

      default:
         {
         yrf = m_csModel.UpdateCropStatus(pContext, this, pFarm, pStation, pLayer, idu, areaHa, doy, year, lulc, cropStage, priorCumYRF);
         }
      }

   return yrf;
   }

   /*

   float WoodyCrop::CheckCondition(EnvContext* pContext, FarmModel *pFarmModel, Farm* pFarm, ClimateStation* pStation, MapLayer* pLayer, int idu, int cropStage, int doy, int year, float priorCumYRF)
      {
      // questions:
      // 1) when was this planted?
      // 2) crop stages - can these be generically defined?

      // applicable crop stages:
      //  CS_PREPLANT
      //  CS_ACTIVE_GROWTH
      //  CS_DORMANT      // Winter until break dormancy (aft Mar1 + 5 cons days>5)
      //  ---> back to CS_ACTIVE_GROWTH and repeat

      // IDU columns uses:
      //  m_colCropAge

      // crop has been planted but not yet hardened?
      switch (cropStage)
         {
         case CS_PREPLANT:
            // condition which triggers planting
            if (doy > JUN14)
               {
               float areaHa = 0;
               pLayer->GetData(idu, FarmModel::m_colArea, areaHa);  // m2
               areaHa /= M2_PER_HA;  // convert m2->ha
               return pFarmModel->AddCropEvent(pContext, idu, CE_YIELD_FAILURE, areaHa, doy, 0, priorCumYRF);
               }

            if (doy > this->m_minPlantingDate)
               {
               float tMean = 0;
               for (int i = 0; i < 5; i++)
                  {
                  float _tMean = 0;
                  pStation->GetData(doy - i - 1, year, TMEAN, _tMean);
                  tMean += _tMean;
                  }
               tMean /= 5;

               if (tMean >= 5.0f)
                  {
                  // if field accessible? (vol. moisture content < 92% of FC)
                  // if ( vmc < 0.92 )   // 
                  theProcess->UpdateIDU(pContext, idu, FarmModel::m_colCropStage, CS_ACTIVE_GROWTH, ADD_DELTA);
                  }
               }

            return 0;

         case CS_ACTIVE_GROWTH:
            {
            if (doy == 1) 
               {
               pLayer->SetData(idu, FarmModel::m_colDormancy, this->m_dormancyCriticalDate);
               return 0 ;
               }

            float yrf = 0;
            int dormancy = 0;
            pLayer->GetData(idu, FarmModel::m_colDormancy, dormancy);   // = this->m_dormancyCriticalDate + 7;  // seven days after the critical date

            // start checking for dormancy after Sept 
            if (doy > dormancy)  // make date user specified 
               {
               // calculate temperature and precip for the previous 21 days
               //float periodPrecip = 0;
               //float historicPrecip = 0;
               float tMean = 0;
               for (int i = 0; i < 21; i++)
                  {
                  //float _precip = 0;
                  //pStation->GetData(doy - i - 1, year, PRECIP, _precip);
                  //periodPrecip += _precip;
                  //
                  //pStation->GetHistoricMean(doy - i - 1, PRECIP, _precip);
                  //historicPrecip += _precip;

                  float _tMean = 0;
                  pStation->GetData(doy - i - 1, year, TMEAN, _tMean);
                  tMean += _tMean;
                  }

               tMean /= 21;
               //periodPrecip /= 21;

               // do we need to extend the growing season?
               // (if in the prior 21 days, was our average temperature
               // over the 90th percentile for historic mean 
               if (tMean < this->m_highTempThreshold ) // || periodPrecip < this->m_highPrecipThreshold)
                  {
                  pLayer->SetData(idu, FarmModel::m_colDormancy, this->m_dormancyCriticalDate + 14);   // growing season extended by two weeks (dynamic critical date)
                  }
               else
                  {
                  // transition to dormant state; harvest if needed
                  int cropAge = 0;
                  pLayer->GetData(idu, FarmModel::m_colCropAge, cropAge);

                  float area = 0;
                  pLayer->GetData(idu, FarmModel::m_colArea, area);
                  float areaHa = area / M2_PER_HA;

                  // harvest?  Note: cropAge is ZERO-based, m_startHarvestYear is 0-based
                  if ((cropAge > this->m_startHarvestYr) && ((cropAge-this->m_startHarvestYr) % this->m_harvFreqYr == 0))
                     {
                     yrf = pFarmModel->AddCropEvent(pContext, idu, CE_HARVEST, areaHa, doy, 0, priorCumYRF);
                     }

                  // in any case, make dormant
                  theProcess->UpdateIDU(pContext, idu, FarmModel::m_colCropStage, CS_DORMANT, SET_DATA);
                  }
               }
            return yrf;
            }

         case CS_DORMANT:
            // critical winter date reached? (March 1)
            {
            if (doy >= this->m_minPlantingDate)
               {
               // track daily temperature for last 5 days
               bool itsCold = false;
               for (int i = 0; i < 5; i++)
                  {
                  float tMean = 0;
                  pStation->GetData(doy - i - 1, year, TMEAN, tMean);

                  if (tMean < 5)
                     {
                     itsCold = true;
                     break;
                     }
                  }

               if (itsCold)
                  return 0;

               // we passed the 5 consecutive days >= 5 C test
               // Spring Regrowth begins - accumulate GDD
               theProcess->UpdateIDU(pContext, idu, FarmModel::m_colCropStage, CS_ACTIVE_GROWTH, ADD_DELTA);
               }
            }
            return 0;
         }
      }

float NativeHerbaceousCrop::CheckCondition(EnvContext* pContext, FarmModel *pFarmModel, Farm* pFarm, ClimateStation* pStation, MapLayer* pLayer, int idu, int cropStage, int doy, int year, float priorCumYRF)
   {
   // Basic Model
   switch (cropStage)
      {
      case CS_PREPLANT:
         // condition which triggers planting
         if (doy > this->m_minPlantingDate)  // are we past the minimum planting date?
            {
            // has it been warm enough/long enough to plant?
            float tMean = 0;
            for (int i = 0; i < this->m_plantingDateTempPeriod; i++)
               {
               float _tMean = 0;
               pStation->GetData(doy - i - 1, year, TMEAN, _tMean);
               tMean += _tMean;
               }
            tMean /= this->m_plantingDateTempPeriod;

            if (tMean >=  this->m_plantingDateTempThreshold)
               {
               // if field accessible? (vol. moisture content < 92% of FC)
               // if ( vmc < 0.92 )   // 
               theProcess->UpdateIDU(pContext, idu, FarmModel::m_colCropStage, CS_ACTIVE_GROWTH, ADD_DELTA);
               }
            }

         // have we exceeded the max planting day without planting?
         if (doy > this->m_maxPlantingDate)
            {
            float areaHa = 0;
            pLayer->GetData(idu, FarmModel::m_colArea, areaHa);  // m2
            areaHa /= M2_PER_HA;  // convert m2->ha
            return pFarmModel->AddCropEvent(pContext, idu, CE_YIELD_FAILURE, areaHa, doy, 0, priorCumYRF);
            }
         return 0;

      case CS_ACTIVE_GROWTH:
         {
         if (doy == 1)
            {
            pLayer->SetData(idu, FarmModel::m_colDormancy, this->m_dormancyCriticalDate);
            return 0;
            }

         float yrf = 0;
         int dormancy = 0;
         pLayer->GetData(idu, FarmModel::m_colDormancy, dormancy);   // = this->m_dormancyCriticalDate + 7;  // seven days after the critical date

         // start checking for dormancy after Sept 
         if (doy > dormancy)  // make date user specified 
            {
            // calculate temperature and precip for the previous 21 days
            //float periodPrecip = 0;
            //float historicPrecip = 0;
            float tMean = 0;
            for (int i = 0; i < 21; i++)
               {
               //float _precip = 0;
               //pStation->GetData(doy - i - 1, year, PRECIP, _precip);
               //periodPrecip += _precip;
               //
               //pStation->GetHistoricMean(doy - i - 1, PRECIP, _precip);
               //historicPrecip += _precip;

               float _tMean = 0;
               pStation->GetData(doy - i - 1, year, TMEAN, _tMean);
               tMean += _tMean;
               }

            tMean /= 21;
            //periodPrecip /= 21;

            // do we need to extend the growing season?
            // (if in the prior 21 days, was our average temperature
            // over the 90th percentile for historic mean 
            if (tMean < this->m_highTempThreshold) // || periodPrecip < this->m_highPrecipThreshold)
               {
               pLayer->SetData(idu, FarmModel::m_colDormancy, this->m_dormancyCriticalDate + 14);   // growing season extended by two weeks (dynamic critical date)
               }
            else
               {
               // transition to dormant state; harvest if needed
               int cropAge = 0;
               pLayer->GetData(idu, FarmModel::m_colCropAge, cropAge);

               float area = 0;
               pLayer->GetData(idu, FarmModel::m_colArea, area);
               float areaHa = area / M2_PER_HA;

               // harvest?  Note: cropAge is ZERO-based, m_startHarvestYear is 0-based
               if ((cropAge > this->m_startHarvestYr) && ((cropAge - this->m_startHarvestYr) % this->m_harvFreqYr == 0))
                  {
                  yrf = pFarmModel->AddCropEvent(pContext, idu, CE_HARVEST, areaHa, doy, 0, priorCumYRF);
                  }

               // in any case, make dormant
               theProcess->UpdateIDU(pContext, idu, FarmModel::m_colCropStage, CS_DORMANT, SET_DATA);
               }
            }
         return yrf;
         }

      case CS_DORMANT:
         // critical winter date reached? (March 1)
         {
         if (doy >= this->m_minPlantingDate)
            {
            // track daily temperature for last 5 days
            bool itsCold = false;
            for (int i = 0; i < 5; i++)
               {
               float tMean = 0;
               pStation->GetData(doy - i - 1, year, TMEAN, tMean);

               if (tMean < 5)
                  {
                  itsCold = true;
                  break;
                  }
               }

            if (itsCold)
               return 0;

            // we passed the 5 consecutive days >= 5 C test
            // Spring Regrowth begins - accumulate GDD
            theProcess->UpdateIDU(pContext, idu, FarmModel::m_colCropStage, CS_ACTIVE_GROWTH, ADD_DELTA);
            }
         }
         return 0;
      }
      }
      */

   
 void FarmModel::UpdateAnnualOutputs(EnvContext* pContext, bool useAddDelta)
   {
   MapLayer* pLayer = (MapLayer*)pContext->pMapLayer;
   int year = pContext->currentYear;

   float tYRF = 0, tArea = 0;

   m_avgFieldSizeHa = 0;
   int fieldCount = 0;

   // first update yield results (IDU Level)
   for (int i = 0; i < (int)this->m_farmArray.GetSize(); i++)
      {
      Farm* pFarm = m_farmArray[i];

      if (pFarm->IsActive() == false)
         continue;

      for (int j = 0; j < (int)pFarm->m_fieldArray.GetSize(); j++)
         {
         FarmField* pField = pFarm->m_fieldArray[j];

         for (int k = 0; k < (int)pField->m_iduArray.GetSize(); k++)
            {
            int idu = pField->m_iduArray[k];

            float yrf = -1, area = 0;    // yield reduction factor (0-1)
            pLayer->GetData(idu, m_colYieldRed, yrf);
            pLayer->GetData(idu, m_colArea, area);

            if (yrf >= 0)
               {
               tYRF += yrf * area;
               tArea += area;
               theProcess->UpdateIDU(pContext, idu, m_colYieldRed, yrf, ADD_DELTA);   // add final value using delta array
               }

            int lulc = 0;
            pLayer->GetData(idu, m_colLulc, lulc);

            float precipJunToAug = 0;
            pFarm->m_pClimateStation->GetPeriodPrecip(JUN1, AUG31, pContext->currentYear, precipJunToAug);

            float gdd10 = 0;
            pFarm->m_pClimateStation->GetPeriodGDD(JUN1, AUG31, pContext->currentYear, 10.0f, gdd10);

            float yield = 0;
            GetAnnualYield(lulc, precipJunToAug, gdd10, yield);

            if (yield > 0)
               theProcess->UpdateIDU(pContext, idu, m_colYield, yield * (1 - yrf), ADD_DELTA);

            m_avgFieldSizeHa += area;
            fieldCount++;
            }
         }
      }

   m_avgYieldReduction = tYRF / tArea;
   m_avgFieldSizeHa /= (fieldCount * M2_PER_HA);

   // farm count information by farm type

   int farmTypeCount = (int)m_farmTypeArray.GetSize();
   CArray<float, float> farmCountData;

   farmCountData.SetSize(1 + farmTypeCount);   // includes 'year'
   memset(farmCountData.GetData(), 0, (1 + farmTypeCount) * sizeof(float));

   farmCountData[0] = (float)pContext->currentYear;   // length = 1+farmTypeCount

   // iterate though the farm size (which is FarmType x Region 
   for (int i = 0; i < m_farmArray.GetSize(); i++)
      {
      Farm* pFarm = m_farmArray[i];

      if (pFarm->IsActive() && pFarm->m_farmType >= 0)
         {
         int farmType = pFarm->m_farmType;
         farmCountData[1 + farmType] += 1;
         }
      }

   m_pFarmTypeCountData->AppendRow(farmCountData);


   // Next, get farm type counts, sizes by farm type and region, for all farm type/region
   // combos listed in the <farm_count_trajectories> tag in the xml input file
   CArray<float, float> fcTData;
   fcTData.Add((float)pContext->currentYear);

   for (int i = 0; i < m_fctArray.GetSize(); i++)
      fcTData.Add((float)m_fctArray[i]->count);
   m_pFCTRData->AppendRow(fcTData);

   // farm sizes by farm type and region, based on the <farm_size_trajectories> tag in the xml input file
   // Note:  We do two calculations
   // 1) m_pFarmSizeTRData - this contains Farm size by farm type and region
   // 2) m_pFarmSizeTData - contains FarmSize by type only (Not disaggregated to region)
   //   for the latter, we take the regional farm size averages, coupled with the the number of 
   //   farms of that type in the region.  These are the "Agg" variable below

   CArray<float, float> fsTRData;      // by type and region
   CArray<float, float> fsTData;       // only by type (not regions)
   CArray<int, int> fsTCounts;         // counts for normalization

   int fstCount = (int)m_fstArray.GetSize();

   fsTData.SetSize(1 + farmTypeCount);
   fsTRData.SetSize(1 + fstCount);
   fsTCounts.SetSize(farmTypeCount);   // note: no 'year' prepended

   memset(fsTData.GetData(), 0, (1 + farmTypeCount) * sizeof(float));
   memset(fsTRData.GetData(), 0, (1 + fstCount) * sizeof(float));
   memset(fsTCounts.GetData(), 0, farmTypeCount * sizeof(int));

   fsTData[0] = (float)pContext->currentYear;   // length = 1+farmTypeCount
   fsTRData[0] = (float)pContext->currentYear;   // length = 1+fstCount

   // iterate though the farm size (which is FarmType x Region 
   for (int i = 0; i < m_fstArray.GetSize(); i++)
      {
      int farmType = m_fstArray[i]->farmType;
      int farmCount = m_fstArray[i]->count;
      int region = m_fstArray[i]->regionID;

      fsTRData[i + 1] = (float)m_fstArray[i]->avgSizeHa;      // 

      // aggregate across regions
      fsTData[1 + farmType] += (m_fstArray[i]->avgSizeHa * farmCount);      // length = 1+farmTypeCount; FarmType ID's must be zero-based, sequential in xml input file!
      fsTCounts[farmType] += farmCount;
      }

   // normalize agg data
   for (int i = 0; i < farmTypeCount; i++)
      fsTData[1 + i] /= fsTCounts[i];

   m_pFarmSizeTRData->AppendRow(fsTRData);
   m_pFarmSizeTData->AppendRow(fsTData);

   // Next, Field size by LULC_B, region
   // basic idea is to allocate to floating point arrays, one hold LULC_B field sizes, one holding LULC_BxREGION values, + time
   // we also allocate arrays folding the field counts for the above arrays.
   // NOte that in all cases, LULC_B are only those assocaited with Ag

   CArray<float, float> fldSizeLData;      // by LULC_B
   CArray<float, float> fldSizeLRData;     // by LULC_B and region
   CArray<int, int> fldCountLData;
   CArray<int, int> fldCountLRData;

   int regionCount = (int)m_regionsArray.GetSize();
   int lulcBCount = (int)m_agLulcBCols.GetCount();
   fldSizeLData.SetSize(1 + lulcBCount);
   fldSizeLRData.SetSize(1 + lulcBCount * regionCount);

   fldCountLData.SetSize(lulcBCount);
   fldCountLRData.SetSize(lulcBCount * regionCount);

   // zero out arrays
   memset(fldSizeLData.GetData(), 0, (1 + lulcBCount) * sizeof(float));
   memset(fldSizeLRData.GetData(), 0, (1 + lulcBCount * regionCount) * sizeof(float));

   memset(fldCountLData.GetData(), 0, lulcBCount * sizeof(int));
   memset(fldCountLRData.GetData(), 0, (lulcBCount * regionCount) * sizeof(int));

   // iterate though farms, fields to get stats
   for (int i = 0; i < m_farmArray.GetSize(); i++)
      {
      Farm* pFarm = m_farmArray[i];

      if (pFarm->IsActive())
         {
         int fieldCount = (int)pFarm->m_fieldArray.GetSize();
         for (int j = 0; j < fieldCount; j++)
            {
            FarmField* pField = pFarm->m_fieldArray[j];

            if (pField->m_iduArray.GetSize() > 0)
               {
               int idu = pField->m_iduArray[0];

               if (IsAg(pLayer, idu))
                  {
                  int lulcB = -1;
                  pLayer->GetData(idu, m_colLulc, lulcB);

                  int fieldID = -1;
                  pLayer->GetData(idu, m_colFieldID, fieldID);

                  float area = -1;
                  pLayer->GetData(idu, m_colArea, area);

                  int region = -1;
                  pLayer->GetData(idu, m_colRegion, region);

                  int lulcCol = -1;   // this is the col index in the dataobj being populated
                  BOOL ok = m_agLulcBCols.Lookup(lulcB, lulcCol);
                  ASSERT(ok);

                  int regionIndex = -1;
                  ok = m_regionsIndexMap.Lookup(region, regionIndex);
                  if (!ok)
                     continue;

                  int lrCol = lulcCol * (int)m_regionsArray.GetSize() + regionIndex;

                  // populate arrays.  Note that for the LULC_BxREGION arrays, outer loop is LULC_B (ag),
                  // inner loop is REGION
                  fldSizeLData[1 + lulcCol] += area;    // col DOES NOT include time, so we add 1
                  fldSizeLRData[1 + lrCol] += area;

                  fldCountLData[lulcCol] += 1;         // these ones don't have a time col
                  fldCountLRData[lrCol] += 1;
                  }
               }
            }
         }
      }  // end of: for each Farm

   // normalize
   fldSizeLData[0] = (float)pContext->currentYear;
   fldSizeLRData[0] = (float)pContext->currentYear;

   for (int i = 0; i < lulcBCount; i++)
      fldSizeLData[i + 1] /= (fldCountLData[i] * M2_PER_HA);

   for (int i = 0; i < lulcBCount * regionCount; i++)
      fldSizeLRData[i + 1] /= (fldCountLRData[i] * M2_PER_HA);

   m_pFldSizeLData->AppendRow(fldSizeLData);
   m_pFldSizeLRData->AppendRow(fldSizeLRData);


   //CMap<int,int, float, float>  lulcAreaMap;  // key=fieldID, value=current area
   //
   //for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
   //   {
   //   if (IsAg( pLayer, idu ))
   //      {
   //      int lulcB = -1;
   //      pLayer->GetData(idu, m_colLulc, lulcB);
   //
   //      int fieldID = -1;
   //      pLayer->GetData(idu, m_colFieldID, fieldID );
   //
   //      float area = -1;
   //      pLayer->GetData(idu, m_colArea, area );
   //
   //      int region =-1;
   //      pLayer->GetData(idu, m_colRegion, region );
   //
   //      int col = -1;   // this is the col index in the dataobj being populated
   //      BOOL ok = m_agLulcBCols.Lookup(lulcB, col);
   //      ASSERT( ok );
   //
   //
   //      //ok = lulcAreaMap
   //      
   //      }
   //
   //
   //   }



   // climate data - annual totals/means
   int stationCount = m_climateManager.GetStationCount();
   CArray< float, float > dataPrecip;  dataPrecip.SetSize(stationCount + 1);   dataPrecip[0] = (float)year;
   CArray< float, float > dataTmin;    dataTmin.SetSize(stationCount + 1);   dataTmin[0] = (float)year;
   CArray< float, float > dataTmean;   dataTmean.SetSize(stationCount + 1);   dataTmean[0] = (float)year;
   CArray< float, float > dataTmax;    dataTmax.SetSize(stationCount + 1);   dataTmax[0] = (float)year;
   CArray< float, float > dataRx1Jan;  dataRx1Jan.SetSize(stationCount + 1);   dataRx1Jan[0] = (float)year;
   CArray< float, float > dataRx1Feb;  dataRx1Feb.SetSize(stationCount + 1);   dataRx1Feb[0] = (float)year;
   CArray< float, float > dataRx1Mar;  dataRx1Mar.SetSize(stationCount + 1);   dataRx1Mar[0] = (float)year;
   CArray< float, float > dataRx1Apr;  dataRx1Apr.SetSize(stationCount + 1);   dataRx1Apr[0] = (float)year;
   CArray< float, float > dataRx1May;  dataRx1May.SetSize(stationCount + 1);   dataRx1May[0] = (float)year;
   CArray< float, float > dataRx1Jun;  dataRx1Jun.SetSize(stationCount + 1);   dataRx1Jun[0] = (float)year;
   CArray< float, float > dataRx1Jul;  dataRx1Jul.SetSize(stationCount + 1);   dataRx1Jul[0] = (float)year;
   CArray< float, float > dataRx1Aug;  dataRx1Aug.SetSize(stationCount + 1);   dataRx1Aug[0] = (float)year;
   CArray< float, float > dataRx1Sep;  dataRx1Sep.SetSize(stationCount + 1);   dataRx1Sep[0] = (float)year;
   CArray< float, float > dataRx1Oct;  dataRx1Oct.SetSize(stationCount + 1);   dataRx1Oct[0] = (float)year;
   CArray< float, float > dataRx1Nov;  dataRx1Nov.SetSize(stationCount + 1);   dataRx1Nov[0] = (float)year;
   CArray< float, float > dataRx1Dec;  dataRx1Dec.SetSize(stationCount + 1);   dataRx1Dec[0] = (float)year;
   CArray< float, float > dataRx3Jan;  dataRx3Jan.SetSize(stationCount + 1);   dataRx3Jan[0] = (float)year;
   CArray< float, float > dataRx3Feb;  dataRx3Feb.SetSize(stationCount + 1);   dataRx3Feb[0] = (float)year;
   CArray< float, float > dataRx3Mar;  dataRx3Mar.SetSize(stationCount + 1);   dataRx3Mar[0] = (float)year;
   CArray< float, float > dataRx3Apr;  dataRx3Apr.SetSize(stationCount + 1);   dataRx3Apr[0] = (float)year;
   CArray< float, float > dataRx3May;  dataRx3May.SetSize(stationCount + 1);   dataRx3May[0] = (float)year;
   CArray< float, float > dataRx3Jun;  dataRx3Jun.SetSize(stationCount + 1);   dataRx3Jun[0] = (float)year;
   CArray< float, float > dataRx3Jul;  dataRx3Jul.SetSize(stationCount + 1);   dataRx3Jul[0] = (float)year;
   CArray< float, float > dataRx3Aug;  dataRx3Aug.SetSize(stationCount + 1);   dataRx3Aug[0] = (float)year;
   CArray< float, float > dataRx3Sep;  dataRx3Sep.SetSize(stationCount + 1);   dataRx3Sep[0] = (float)year;
   CArray< float, float > dataRx3Oct;  dataRx3Oct.SetSize(stationCount + 1);   dataRx3Oct[0] = (float)year;
   CArray< float, float > dataRx3Nov;  dataRx3Nov.SetSize(stationCount + 1);   dataRx3Nov[0] = (float)year;
   CArray< float, float > dataRx3Dec;  dataRx3Dec.SetSize(stationCount + 1);   dataRx3Dec[0] = (float)year;
   CArray< float, float > dataCDD;     dataCDD.SetSize(stationCount + 1);   dataCDD[0] = (float)year;
   CArray< float, float > dataR10mm;   dataR10mm.SetSize(stationCount + 1);   dataR10mm[0] = (float)year;
   CArray< float, float > dataR10yr;   dataR10yr.SetSize(stationCount + 1);   dataR10yr[0] = (float)year;
   CArray< float, float > dataR100Yr;  dataR100Yr.SetSize(stationCount + 1);   dataR100Yr[0] = (float)year;
   CArray< float, float > dataRShort;  dataRShort.SetSize(stationCount + 1);   dataRShort[0] = (float)year;
   CArray< float, float > dataExtHeat; dataExtHeat.SetSize(stationCount + 1);   dataExtHeat[0] = (float)year;
   CArray< float, float > dataExtCold; dataExtCold.SetSize(stationCount + 1);   dataExtCold[0] = (float)year;
   CArray< float, float > dataGSL;     dataGSL.SetSize(stationCount + 1);   dataGSL[0] = (float)year;
   CArray< float, float > dataCHU;     dataCHU.SetSize(stationCount + 1);   dataCHU[0] = (float)year;
   CArray< float, float > dataCGDD;    dataCGDD.SetSize(stationCount + 1);   dataCGDD[0] = (float)year;
   CArray< float, float > dataAGDD;    dataAGDD.SetSize(stationCount + 1);   dataAGDD[0] = (float)year;
   CArray< float, float > dataPDays;   dataPDays.SetSize(stationCount + 1);   dataPDays[0] = (float)year;

   for (int i = 0; i < stationCount; i++)
      {
      ClimateStation* pStation = m_climateManager.GetStation(i);
      dataPrecip[i + 1] = pStation->m_annualPrecip;
      dataTmin[i + 1] = pStation->m_annualTMin;
      dataTmean[i + 1] = pStation->m_annualTMean;
      dataTmax[i + 1] = pStation->m_annualTMax;

      dataRx1Jan[i + 1] = pStation->m_rx1[0];
      dataRx1Feb[i + 1] = pStation->m_rx1[1];
      dataRx1Mar[i + 1] = pStation->m_rx1[2];
      dataRx1Apr[i + 1] = pStation->m_rx1[3];
      dataRx1May[i + 1] = pStation->m_rx1[4];
      dataRx1Jun[i + 1] = pStation->m_rx1[5];
      dataRx1Jul[i + 1] = pStation->m_rx1[6];
      dataRx1Aug[i + 1] = pStation->m_rx1[7];
      dataRx1Sep[i + 1] = pStation->m_rx1[8];
      dataRx1Oct[i + 1] = pStation->m_rx1[9];
      dataRx1Nov[i + 1] = pStation->m_rx1[10];
      dataRx1Dec[i + 1] = pStation->m_rx1[11];

      dataRx3Jan[i + 1] = pStation->m_rx3[0];
      dataRx3Feb[i + 1] = pStation->m_rx3[1];
      dataRx3Mar[i + 1] = pStation->m_rx3[2];
      dataRx3Apr[i + 1] = pStation->m_rx3[3];
      dataRx3May[i + 1] = pStation->m_rx3[4];
      dataRx3Jun[i + 1] = pStation->m_rx3[5];
      dataRx3Jul[i + 1] = pStation->m_rx3[6];
      dataRx3Aug[i + 1] = pStation->m_rx3[7];
      dataRx3Sep[i + 1] = pStation->m_rx3[8];
      dataRx3Oct[i + 1] = pStation->m_rx3[9];
      dataRx3Nov[i + 1] = pStation->m_rx3[10];
      dataRx3Dec[i + 1] = pStation->m_rx3[11];

      dataCDD[i + 1] = (float)pStation->m_maxConsDryDays;
      dataR10mm[i + 1] = (float)pStation->m_r10mmDays;
      dataR10yr[i + 1] = (float)pStation->m_r10yrDays;
      dataR100Yr[i + 1] = (float)pStation->m_r100yrDays;
      dataRShort[i + 1] = (float)pStation->m_rShortDurationDays;
      dataExtHeat[i + 1] = (float)pStation->m_extHeatDays;
      dataExtCold[i + 1] = (float)pStation->m_extColdDays;
      dataGSL[i + 1] = (float)pStation->m_gslDays;
      dataCHU[i + 1] = pStation->m_chuCornMay1[364];
      dataCGDD[i + 1] = pStation->m_cumDegDays0Apr15[364];
      dataAGDD[i + 1] = pStation->m_cumDegDays5Apr1[364];
      dataPDays[i + 1] = pStation->m_pDays[364];
      }

   // Climate Indice summaries
   m_annualCIArray[DO_PRECIP]->AppendRow(dataPrecip);
   m_annualCIArray[DO_TMIN]->AppendRow(dataTmin);
   m_annualCIArray[DO_TMEAN]->AppendRow(dataTmean);
   m_annualCIArray[DO_TMAX]->AppendRow(dataTmax);

   m_annualCIArray[DO_RX1_JAN]->AppendRow(dataRx1Jan);
   m_annualCIArray[DO_RX1_FEB]->AppendRow(dataRx1Feb);
   m_annualCIArray[DO_RX1_MAR]->AppendRow(dataRx1Mar);
   m_annualCIArray[DO_RX1_APR]->AppendRow(dataRx1Apr);
   m_annualCIArray[DO_RX1_MAY]->AppendRow(dataRx1May);
   m_annualCIArray[DO_RX1_JUN]->AppendRow(dataRx1Jun);
   m_annualCIArray[DO_RX1_JUL]->AppendRow(dataRx1Jul);
   m_annualCIArray[DO_RX1_AUG]->AppendRow(dataRx1Aug);
   m_annualCIArray[DO_RX1_SEP]->AppendRow(dataRx1Sep);
   m_annualCIArray[DO_RX1_OCT]->AppendRow(dataRx1Oct);
   m_annualCIArray[DO_RX1_NOV]->AppendRow(dataRx1Nov);
   m_annualCIArray[DO_RX1_DEC]->AppendRow(dataRx1Dec);

   m_annualCIArray[DO_RX3_JAN]->AppendRow(dataRx3Jan);
   m_annualCIArray[DO_RX3_FEB]->AppendRow(dataRx3Feb);
   m_annualCIArray[DO_RX3_MAR]->AppendRow(dataRx3Mar);
   m_annualCIArray[DO_RX3_APR]->AppendRow(dataRx3Apr);
   m_annualCIArray[DO_RX3_MAY]->AppendRow(dataRx3May);
   m_annualCIArray[DO_RX3_JUN]->AppendRow(dataRx3Jun);
   m_annualCIArray[DO_RX3_JUL]->AppendRow(dataRx3Jul);
   m_annualCIArray[DO_RX3_AUG]->AppendRow(dataRx3Aug);
   m_annualCIArray[DO_RX3_SEP]->AppendRow(dataRx3Sep);
   m_annualCIArray[DO_RX3_OCT]->AppendRow(dataRx3Oct);
   m_annualCIArray[DO_RX3_NOV]->AppendRow(dataRx3Nov);
   m_annualCIArray[DO_RX3_DEC]->AppendRow(dataRx3Dec);

   m_annualCIArray[DO_CDD]->AppendRow(dataCDD);
   m_annualCIArray[DO_R10MM]->AppendRow(dataR10mm);
   m_annualCIArray[DO_STORM10]->AppendRow(dataR10yr);
   m_annualCIArray[DO_STORM100]->AppendRow(dataR100Yr);
   m_annualCIArray[DO_SHORTDURPRECIP]->AppendRow(dataRShort);
   m_annualCIArray[DO_EXTHEAT]->AppendRow(dataExtHeat);
   m_annualCIArray[DO_EXTCOLD]->AppendRow(dataExtCold);
   m_annualCIArray[DO_GSL]->AppendRow(dataGSL);
   m_annualCIArray[DO_CHU]->AppendRow(dataCHU);
   m_annualCIArray[DO_CERGDD]->AppendRow(dataCGDD);
   m_annualCIArray[DO_ALFGDD]->AppendRow(dataAGDD);
   m_annualCIArray[DO_PDAYS]->AppendRow(dataPDays);

   // event data.  Note that it is lread stored in the event array,
   // so all we have to do is add it to the data obj
   m_pCropEventData->AppendRow(m_cropEvents.GetData(), 1 + (int) m_cropEventTypes.GetCount());
   m_pFarmEventData->AppendRow(m_farmEvents, 1 + FE_EVENTCOUNT);
   }


void FarmModel::UpdateDailyOutputs(int doy, EnvContext* pContext)
   {
   MapLayer* pLayer = (MapLayer*)pContext->pMapLayer;
   int year = pContext->currentYear;
   float day = float(365 * (year - pContext->startYear) + doy - 1);

   float tPlantDate = 0, tAreaPlantDate = 0;

   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      float area = 0;
      pLayer->GetData(idu, m_colArea, area);

      // planting date
      int plantDate = 0;
      pLayer->GetData(idu, m_colPlantDate, plantDate);

      if (plantDate > 0)
         {
         tPlantDate += (float)plantDate * area;
         tAreaPlantDate += area;
         }
      }

   tPlantDate /= tAreaPlantDate;

   CArray< float, float > data;
   data.Add((float)day);
   data.Add(m_avgYieldReduction);
   data.Add(tPlantDate);

   m_pDailyData->AppendRow(data);

   // climate data
   int stationCount = m_climateManager.GetStationCount();
   CArray< float, float > dataPrecip;  dataPrecip.SetSize(stationCount + 1); dataPrecip[0] = (float)day;
   CArray< float, float > dataTmin;    dataTmin.SetSize(stationCount + 1);   dataTmin[0] = (float)day;
   CArray< float, float > dataTmean;   dataTmean.SetSize(stationCount + 1);  dataTmean[0] = (float)day;
   CArray< float, float > dataTmax;    dataTmax.SetSize(stationCount + 1);   dataTmax[0] = (float)day;

   for (int i = 0; i < stationCount; i++)
      {
      ClimateStation* pStation = m_climateManager.GetStation(i);

      float precip = 0, tMin = 0, tMax = 0, tMean = 0;
      pStation->GetData(doy, year, PRECIP, dataPrecip[i + 1]);
      pStation->GetData(doy, year, TMIN, dataTmin[i + 1]);
      pStation->GetData(doy, year, TMEAN, dataTmean[i + 1]);
      pStation->GetData(doy, year, TMAX, dataTmax[i + 1]);
      }

   m_dailyCIArray[DO_PRECIP]->AppendRow(dataPrecip);
   m_dailyCIArray[DO_TMIN]->AppendRow(dataTmin);
   m_dailyCIArray[DO_TMEAN]->AppendRow(dataTmean);
   m_dailyCIArray[DO_TMAX]->AppendRow(dataTmax);
   }


float FarmModel::AddCropEvent(EnvContext* pContext, int idu, int eventID, LPCTSTR eventLabel, float areaHa, int doy, float yrf, float priorCumYRF)
   {
   theProcess->UpdateIDU(pContext, idu, m_colCropStatus, eventID, ADD_DELTA);

   int eventIndex = 0;
   bool ok = m_cropEventIndexLookup.Lookup(eventID, eventIndex);

   m_cropEvents[eventIndex + 1] += areaHa;   // +1 because m_cropEvents[0] = year

   if (m_outputPivotTable)
      {
      // are we tracking this IDU?
      int index = -1;
      if (m_trackIDUArray.GetSize() > 0 && m_trackIDUMap.Lookup(idu, index) == FALSE)
         return yrf;

      // are tracking this event?
      if (m_trackCropEventArray.GetSize() > 0 && m_trackCropEventMap.Lookup(eventID, index) == FALSE)
         return yrf;

      // add event to pivot table
      CArray< VData, VData > data;
      data.SetSize(14);

      int lulc = -1;
      pContext->pMapLayer->GetData(idu, m_colLulc, lulc);

      int farmID = -1;
      pContext->pMapLayer->GetData(idu, m_colFarmID, farmID);

      int farmType = -1;
      pContext->pMapLayer->GetData(idu, m_colFarmType, farmType);

      int slc = -1;
      pContext->pMapLayer->GetData(idu, m_colSLC, slc);

      int climateStation = -1;
      pContext->pMapLayer->GetData(idu, this->m_climateManager.m_colStationID, climateStation);

      TCHAR* lulcName = "Unknown";
      switch (lulc)
         {
         case CORN:           lulcName = "Corn";       break;
         case SOYBEANS:       lulcName = "Soybeans";   break;
         case ALFALFA:        lulcName = "Alfalfa";    break;
         case SPRING_WHEAT:   lulcName = "SprWheat";   break;
         case BARLEY:         lulcName = "Barley";     break;
         case POTATOES:       lulcName = "Potatoes";   break;
         default:
            {
            // is it in our dynamic crop list?
            for (int i = 0; i < m_csModel.m_crops.GetSize(); i++)
               {
               CSCrop* pCrop = m_csModel.m_crops[i];
               if (pCrop->m_id == lulc)
                  {
                  lulcName = (TCHAR*)(LPCTSTR)pCrop->m_name;
                  break;
                  }
               }
            }
         }
      LPCTSTR _eventLabel = (eventLabel != NULL) ? eventLabel : this->m_cropEventTypes[eventIndex]->label;

      data[0] = pContext->currentYear;
      data[1] = doy;
      data[2] = eventID;
      data[3] = _eventLabel;
      data[4] = idu;
      data[5] = areaHa;
      data[6] = lulc;
      data[7] = lulcName;
      data[8] = farmID;
      data[9] = farmType;
      data[10] = climateStation;
      data[11] = slc;
      data[12] = yrf;

      float cumYRF = ::AccumulateYRFs(yrf, priorCumYRF);
      data[13] = cumYRF;

      m_pCropEventPivotTable->AppendRow(data);
      }

   return yrf;
   }


void FarmModel::AddFarmEvent(EnvContext* pContext, int idu, int eventID, int farmID, int farmType, float areaHa)
   {
   m_farmEvents[eventID + 1] += areaHa;   // +1 because m_farmEvents[0] = year

   if (m_outputPivotTable)
      {
      // are we tracking this IDU?
      int index = -1;
      if (m_trackIDUArray.GetSize() > 0 && m_trackIDUMap.Lookup(idu, index) == FALSE)
         return;

      // are tracking this event?
      if (m_trackFarmEventArray.GetSize() > 0 && m_trackFarmEventMap.Lookup(eventID, index) == FALSE)
         return;

      // add event to pivot table
      CArray< VData, VData > data;
      data.SetSize(7);

      data[0] = pContext->currentYear;
      data[1] = eventID;
      data[2] = gFarmEventNames[eventID];
      data[3] = idu;
      data[4] = areaHa;
      data[5] = farmID;
      data[6] = farmType;

      m_pFarmEventPivotTable->AppendRow(data);
      }

   return;
   }



void FarmModel::GetAnnualYield(int lulc, float precipJuntoAug, float heatUnitsAbove10, float& yield)
   {
   yield = 0.0f;

   float H = heatUnitsAbove10 * 9.0f / 5.0f;     // convert to farenheit
   float R = precipJuntoAug;     // mm

   switch (lulc)
      {
      case CORN:       //corn bu/acre
         yield = float(3.33f + 0.03f * log(R) + 0.18f * log(H));   //  (0.029795f*heatUnitsAbove10)+(0.015804f*precipJun1toAugust1)+95.23247f;
         break;

      case SOYBEANS:   //soybeans bu/acre
         yield = float(1.62f + 0.03f * log(R) + 0.26f * log(H));      // (0.012931f*heatUnitsAbove10)+(0.004948f*precipJun1toAugust1)+26.59982f;
         break;

      case ALFALFA:    //hay tonnes/acre
         yield = float(1.12f + 0.01f * log(R) - 0.03f * log(H));  //(-0.00019f*heatUnitsAbove10)+(0.001687f*precipJun1toAugust1)+3.813044f; 
         break;
      }

   if (yield > 0)
      yield = (float) exp(yield);
   }


bool FarmModel::ExpandFarms(EnvContext* pContext)
   {
   // for each farm, determine, using a random process, whether the farm should try to expand
   ASSERT(m_farmExpRateTable.GetRowCount() > 0);

   MapLayer* pIDULayer = (MapLayer*)pContext->pMapLayer;

   float rate = m_farmExpRateTable.IGet((float)pContext->currentYear, 0, 1, IM_LINEAR);  // (0-1)

   // set up data collection
   int expandedCount = 0;
   float areaTransfered = 0;

   int passedCount = 0;
   int noNeighborCount = 0;
   int invalidNeighborsCount = 0;

   float* expandCountByFarmTypes = new float[FT_COUNT + 1];
   memset(expandCountByFarmTypes, 0, sizeof(float) * (FT_COUNT + 1));
   expandCountByFarmTypes[0] = (float)pContext->currentYear;

   CArray< int, int > neighbors;
   CArray< float, float > distances;

   int farmCount = 0;
   int consideredCount = 0;
   int randomCount = 0;
   m_adjFieldCount = 0;
   m_adjFieldArea = 0;

   UpdateAvgFarmSizeByRegion(pIDULayer, false);      // recalculates all values

   // start looping through farms, updating 
   for (int i = 0; i < m_farmArray.GetSize(); i++)
      {
      Farm* pFarm = m_farmArray[i];

      if (pFarm->IsActive() == false)
         continue;

      // expandable farm type?
      FarmType* pFarmType = NULL;
      m_farmTypeMap.Lookup(pFarm->m_farmType, pFarmType);

      if (pFarmType == NULL || pFarmType->m_searchRadiusField < 0)
         continue;

      // is the farm in an allowable expansion region?
      bool allowedRegion = false;
      if (pFarmType->m_expandRegions.GetSize() > 0)
         {
         if (pFarmType->m_expandRegions[0][0] == '*')
            allowedRegion = true;
         else
            {
            int farmRegion = -1;
            pIDULayer->GetData(pFarm->m_farmHQ, m_colRegion, farmRegion);
            for (int l = 0; l < pFarmType->m_expandRegions.GetSize(); l++)
               {
               int region = atoi(pFarmType->m_expandRegions[l]);

               if (region == farmRegion)
                  {
                  allowedRegion = true;
                  break;
                  }
               }
            }
         }

      // is this Farm INeligible to consider for expansion?
      bool isEligible =
         (pFarm->IsActive()     // must be active
            && allowedRegion        // and in an allowed region
            && ((pContext->currentYear - pFarm->m_lastExpansion) >= m_reExpandPeriod)  // and is outside the reexpansion period
            && (pFarm->m_fieldArray.GetSize() > 0)         // and has fields
            && (pFarm->m_id >= 0)                           // and has an ID
            && (pFarm->m_totalArea <= m_maxFarmArea));      // and max area constraint hasn't been met 

      if (isEligible == false)
         continue;   // skip this one, go on to the next farm

      // have we reached the target avg farm size yet?
      int hqIDU = pFarm->m_farmHQ;
      if (hqIDU >= 0)
         {
         // get the region of the farm HQ to use in lookup key
         int regionID = -1;
         pIDULayer->GetData(hqIDU, m_colRegion, regionID);
         long key = FST_INFO::Key(regionID, (int)pFarm->m_farmType);

         FST_INFO* pInfo = NULL;
         BOOL found = m_fstMap.Lookup(key, pInfo);

         if (found && pInfo != NULL)
            {
            // get FST target for this farmtype, region
            float target = pInfo->GetTarget(pContext->yearOfRun);   // ha
            if (pInfo->GetAvgSize() >= target)      // ha
               continue;
            }
         }

      // passes all the tests, so it is expandable.  try to expand it.
      pFarm->m_isExpandable = true;

      // roll the dice with the random number generator
      farmCount++;
      consideredCount++;
      int nCount = 0;

      if (m_rn.RandValue(0, 1) <= rate)
         {
         randomCount++;

         // get neighbors with different FarmIDs
         nCount = GetQualifiedNeighbors(pIDULayer, pFarm, pFarmType->m_searchRadiusField, neighbors, distances);

         if (nCount == 0)   // No neighbors?
            noNeighborCount++;
         else
            {
            // neighbors found, look through them
            bool found = false;
            Farm* pNeighborFarm = NULL;
            float distance = -1;

            // look through ordered list of expand types for this FarmType
            for (int k = 0; k < pFarmType->m_expandTypes.GetSize(); k++)
               {
               int expandType = pFarmType->m_expandTypes[k]->m_type;

               for (int n = 0; n < nCount; n++)
                  {
                  // are any of the identified IDU's the same Farm Type?
                  int nFarmType = -1;
                  pIDULayer->GetData(neighbors[n], m_colFarmType, nFarmType);

                  if (nFarmType == expandType)
                     {
                     // yes.  Now check if the neigboring farm is smaller than the expanding farm
                     int nFarmID = -1;
                     pIDULayer->GetData(neighbors[n], m_colFarmID, nFarmID);

                     pNeighborFarm = FindFarmFromID(nFarmID);

                     if (pNeighborFarm->IsActive()
                        && (pNeighborFarm->m_totalArea < pFarm->m_totalArea)
                        && ((pFarm->m_totalArea + pNeighborFarm->m_totalArea) < m_maxFarmArea))
                        {
                        // ALL CONDITIONS SATISFIED.  Transfer the neighbor farm to this farm.
                        // This farm will get all the neighbor's fields
                        // The farm being transferred will lose it's field and become inactive
                        int soldFarmHQ = pNeighborFarm->m_farmHQ;
                        int soldFarmID = pNeighborFarm->m_id;
                        int soldFarmType = pNeighborFarm->m_farmType;
                        float soldFarmArea = pNeighborFarm->m_totalArea;

                        areaTransfered += pFarm->Transfer(pContext, pNeighborFarm, pIDULayer);
                        AddFarmEvent(pContext, pFarm->m_farmHQ, FE_BOUGHT, pFarm->m_id, pFarm->m_farmType, pFarm->m_totalArea / M2_PER_HA);
                        AddFarmEvent(pContext, soldFarmHQ, FE_SOLD, soldFarmID, soldFarmType, soldFarmArea / M2_PER_HA);

                        expandedCount++;
                        expandCountByFarmTypes[pFarmType->m_type + 1] += 1.0f;

                        // adjust field boundaries (combines new fields with old fields when criteria met)
                        int adjFieldCount = 0;
                        float adjFieldArea = pFarm->AdjustFieldBoundaries(pContext, adjFieldCount);
                        m_adjFieldCount += adjFieldCount;
                        m_adjFieldArea += adjFieldArea;

                        // update the average farm size for this region/farmtype
                        this->UpdateAvgFarmSizeByRegion(pIDULayer, false);

                        // update transition matrix
                        if (pFarm->m_farmHQ >= 0 && pNeighborFarm->m_farmHQ >= 0)
                           {
                           int regionID = -1, neighborRegionID = -1;
                           pIDULayer->GetData(pFarm->m_farmHQ, m_colRegion, regionID);
                           pIDULayer->GetData(pNeighborFarm->m_farmHQ, m_colRegion, neighborRegionID);

                           int farmTypeIndex = -1, neighborFarmTypeIndex = -1;
                           m_farmTypesIndexMap.Lookup(pFarm->m_farmType, farmTypeIndex);
                           m_farmTypesIndexMap.Lookup(pNeighborFarm->m_farmType, neighborFarmTypeIndex);

                           int regionIndex = -1, neighborRegionIndex = -1;
                           BOOL ok0 = m_regionsIndexMap.Lookup(regionID, regionIndex);
                           BOOL ok1 = m_regionsIndexMap.Lookup(neighborRegionID, neighborRegionIndex);

                           if (ok0 && ok1)
                              {
                              int fromIndex = neighborFarmTypeIndex * (int)m_regionsArray.GetSize() + neighborRegionIndex;
                              int toIndex = farmTypeIndex * (int)m_regionsArray.GetSize() + regionIndex;
                              int value = m_pFarmExpTransMatrix->GetAsInt(toIndex + 1, fromIndex);
                              m_pFarmExpTransMatrix->Set(toIndex + 1, fromIndex, value + 1);
                              }
                           }

                        // eliminate transferred farm HQ in the IDU coverage (but not the Farm object)
                        //if ( pNeighborFarm->m_farmHQ >= 0 )
                        //   theProcess->UpdateIDU(pContext, pNeighborFarm->m_farmHQ, m_colFarmHQ, -1, ADD_DELTA );

                        // all done transferring farm
                        found = true;
                        break;
                        }
                     }  // end of: if same farm type
                  }  // end of: for each neighbor IDU

               if (found)
                  {
                  passedCount++;
                  break;      // break out of: for each expandType
                  }
               }  // end of: for each expandType

            // we've look through all the expand types and still not found anything.
            if (!found)
               invalidNeighborsCount++;
            }  // end of: else ( neighborCount > 0 )
         }  // end of: if ( randVal < rate )
      }  // end of: for each Farm   

   // add to dataobj
   m_pFarmTypeExpandCountData->AppendRow(expandCountByFarmTypes, FT_COUNT + 1);

   // clean up
   delete[] expandCountByFarmTypes;
   m_adjFieldAreaHa = m_adjFieldArea / M2_PER_HA;

   CString msg;
   msg.Format("Farm Model: Farm Expansion (%i) - %i Active Farms, %i of %i Eligible Farms considered, %i farms expanded (transfering %.0f ha); %i farms had no neighbors with different FarmIDs, %i had no qualified neighbors",
      pContext->currentYear, farmCount, randomCount, consideredCount, expandedCount, areaTransfered / M2_PER_HA, noNeighborCount, invalidNeighborsCount);
   Report::Log(msg);

   msg.Format("Farm Model: %i Fields (%.1f ha) were combined into other Fields", m_adjFieldCount, m_adjFieldAreaHa);
   Report::Log(msg);
   return true;
   }


bool FarmModel::UpdateFSTInfo(MapLayer* pLayer, Farm* pFarm, Farm* pNeighborFarm)
   {
   // first, find and remove the neighbor farm from the FST info
   int hqIDU = pNeighborFarm->m_farmHQ;

   if (hqIDU >= 0)
      {
      int regionID = -1;
      pLayer->GetData(hqIDU, m_colRegion, regionID);
      long key = FST_INFO::Key(regionID, (int)pNeighborFarm->m_farmType);

      FST_INFO* pInfo = NULL;
      BOOL found = m_fstMap.Lookup(key, pInfo);

      if (found && pInfo != NULL)
         {
         pInfo->count--;
         pInfo->area -= pNeighborFarm->m_totalArea;
         ASSERT(pInfo->area >= 0);

         for (INT_PTR l = 0; l < pInfo->farmArray.GetSize(); l++)
            {
            if (pInfo->farmArray[l] == pNeighborFarm)
               {
               pInfo->farmArray.RemoveAt(l);
               pInfo->GetAvgSize();   // updates internal member
               break;
               }
            }
         }
      }
   // next, find and add this farm to the FST info
   hqIDU = pFarm->m_farmHQ;
   if (hqIDU >= 0)
      {
      int regionID = -1;
      pLayer->GetData(hqIDU, m_colRegion, regionID);
      long key = FST_INFO::Key(regionID, (int)pFarm->m_farmType);

      FST_INFO* pInfo = NULL;
      BOOL found = m_fstMap.Lookup(key, pInfo);

      if (found && pInfo != NULL)
         {
         pInfo->count--;
         pInfo->area += pNeighborFarm->m_totalArea;

         pInfo->GetAvgSize();   // updates internal member
         }
      }

   return true;
   }


bool FarmModel::UpdateAvgFarmSizeByRegion(MapLayer* pLayer, bool initialize)
   {
   // resets and updates FST objects
   for (int i = 0; i < m_fstArray.GetSize(); i++)
      {
      m_fstArray[i]->count = 0;
      m_fstArray[i]->area = 0;
      m_fstArray[i]->avgSizeHa = 0;
      m_fstArray[i]->farmArray.RemoveAll();
      }

   // for each farm, 
   for (int i = 0; i < this->m_farmArray.GetSize(); i++)
      {
      Farm* pFarm = this->m_farmArray[i];

      if (pFarm->IsActive())
         {
         // get region and farmtype for this farm
         int hqIDU = pFarm->m_farmHQ;

         if (hqIDU < 0)
            continue;

         int regionID = -1;
         pLayer->GetData(hqIDU, m_colRegion, regionID);
         long key = FST_INFO::Key(regionID, (int)pFarm->m_farmType);

         FST_INFO* pInfo = NULL;

         BOOL found = m_fstMap.Lookup(key, pInfo);

         if (found && pInfo != NULL)
            {
            // add up the areas for all fields for this farm
            pInfo->count++;
            pInfo->farmArray.Add(pFarm);
            pInfo->area += pFarm->m_totalArea;
            }
         }
      }

   for (int i = 0; i < m_fstArray.GetSize(); i++)
      {
      m_fstArray[i]->GetAvgSize();     // sets internal value

      if (initialize)
         m_fstArray[i]->initAvgSizeHa = m_fstArray[i]->avgSizeHa;
      //CString msg;
      //msg.Format( "FarmModel:  FST Type=%i, region=%i, count=%i, area=%.2f (ha), avgSize=%.2f",
      //   (int)m_fstArray[i]->farmType,
      //   m_fstArray[i]->regionID,
      //   m_fstArray[i]->count,
      //   m_fstArray[i]->area * HA_PER_M2,
      //   m_fstArray[i]->avgSizeHa );
      //
      //Report::Log( msg );
      }

   return true;
   }


// finds all neghbor IDUs with the specified radius of a Farm that are not the same FarmID
const int MAX_NEIGHBORS = 4096;

int FarmModel::GetQualifiedNeighbors(MapLayer* pIDULayer, Farm* pFarm, float radius, CArray<int, int>& neighbors, CArray< float, float >& distances)
   {
   static int _neighbors[MAX_NEIGHBORS];
   static float _distances[MAX_NEIGHBORS];

   neighbors.RemoveAll();
   distances.RemoveAll();

   // basic idea: iterate through all of this Farm's Fields (and IDUs), looking for
   // other IDUs that are within the specified radius and that meet the  qualifying conditions
   // 
   for (int j = 0; j < (int)pFarm->m_fieldArray.GetSize(); j++)
      {
      FarmField* pField = pFarm->m_fieldArray[j];

      for (int k = 0; k < (int)pField->m_iduArray.GetSize(); k++)
         {
         // query for all neighbors for this IDU
         int idu = pField->m_iduArray[k];
         Poly* pFarmPoly = pIDULayer->GetPolygon(idu);
         int count = pIDULayer->GetNearbyPolys(pFarmPoly, _neighbors, _distances, MAX_NEIGHBORS, radius);

         if (count > MAX_NEIGHBORS - 1)
            {
            CString msg;
            msg.Format("Farm Model: Number of neighbors found during GetQualifiedNeighbors() (%i) exceeds allocated limit...", count);
            Report::LogWarning(msg);
            }

         // for each neighboring IDU (within the search radius), see if it passes the qualifying conditions
         // if it does, record it into the neighbors array
         for (int i = 0; i < count; i++)
            {
            int nFarmID = -1;
            pIDULayer->GetData(_neighbors[i], m_colFarmID, nFarmID);

            // should we consider this one?  Meaning: it's not part of the current Farm
            if (nFarmID >= 0 && nFarmID != pFarm->m_id)
               {
               // passes the qualifying conditions. Next, check to see if the _neighbor has been seen before
               bool found = false;
               int n;
               for (n = 0; n < neighbors.GetSize(); n++)
                  {
                  if (_neighbors[i] == neighbors[n])
                     {
                     found = true;
                     break;
                     }
                  }

               if (!found)  //it's a new one, so add it to the lists
                  {
                  neighbors.Add(_neighbors[i]);
                  distances.Add(_distances[i]);
                  }
               else // we've see it before, so just check the distance to be sure we are getting the closest distance
                  {
                  if (_distances[i] < distances[n])
                     distances[n] = _distances[i];
                  }
               }
            }
         }
      }

   return (int)neighbors.GetSize();
   }


// TODO:  Add support for reading VSMB soil info

bool FarmModel::LoadXml(EnvContext* pContext, LPCTSTR filename)
   {
   // have xml string, start parsing
   TiXmlDocument doc;
   bool ok = doc.LoadFile(filename);

   if (!ok)
      { 
      Report::ErrorMsg(doc.ErrorDesc());
      return false;
      }

   // start interating through the nodes
   TiXmlElement* pXmlRoot = doc.RootElement();  // f2r

   TiXmlElement* pXmlFarmModel = pXmlRoot->FirstChildElement("farm_model");
   if (pXmlFarmModel == NULL)
      {
      CString msg("Farm Model: missing <farm_model> element in input file ");
      msg += filename;
      Report::ErrorMsg(msg);
      return false;
      }

   MapLayer* pLayer = (MapLayer*)pContext->pMapLayer;

   // lookup fields
   LPCTSTR trackIDs = NULL;
   float yrfThreshold = 0.90f;
   XML_ATTR attrs[] = {
      // attr            type           address              isReq checkCol
      { "farmID_col",    TYPE_CSTRING,  &m_farmIDField,      true,  0 },
      { "farmType_col",  TYPE_CSTRING,  &m_farmTypeField,    true,  0 },
      { "lulc_col",      TYPE_CSTRING,  &m_lulcField,        true,  0 },
      { "rotation_col",  TYPE_CSTRING,  &m_rotationField,    true,  0 },
      { "region_col",    TYPE_CSTRING,  &m_regionField,      true,  0 },
      { "init",          TYPE_INT,      &m_doInit,           false, 0 },
      { "output_pivot_table", TYPE_BOOL,&m_outputPivotTable, false, 0 },
      { "track",         TYPE_STRING,   &trackIDs,           false, 0 },
      { "use_VSMB",      TYPE_BOOL,     &m_useVSMB,          false, 0 },
      { "yrf_threshold", TYPE_FLOAT,    &yrfThreshold,       false, 0 },
      { NULL,            TYPE_NULL,     NULL,                false, 0 } };

   ok = TiXmlGetAttributes(pXmlFarmModel, attrs, filename, pLayer);

   if (!ok)
      return false;

   // verify columns exist
   theProcess->CheckCol(pLayer, m_colFarmID, m_farmIDField, TYPE_INT, m_doInit ? CC_AUTOADD : CC_MUST_EXIST);
   theProcess->CheckCol(pLayer, m_colFarmType, m_farmTypeField, TYPE_INT, m_doInit ? CC_AUTOADD : CC_MUST_EXIST);
   theProcess->CheckCol(pLayer, m_colFarmHQ, "FARMHQ", TYPE_INT, CC_AUTOADD);
   theProcess->CheckCol(pLayer, m_colLulc, m_lulcField, TYPE_INT, CC_MUST_EXIST);
   theProcess->CheckCol(pLayer, m_colRotation, m_rotationField, TYPE_INT, CC_AUTOADD);
   theProcess->CheckCol(pLayer, m_colLulcA, "LULC_A", TYPE_INT, CC_MUST_EXIST);
   theProcess->CheckCol(pLayer, m_colRegion, m_regionField, TYPE_INT, CC_MUST_EXIST);

   m_yrfThreshold = yrfThreshold;

   // next, <crops>
   TiXmlElement* pXmlCrops = pXmlFarmModel->FirstChildElement("crops");
   m_csModel.LoadXml(pXmlCrops, filename, pLayer);

   // next, farm rotations
   TiXmlElement* pXmlRotations = pXmlFarmModel->FirstChildElement("rotations");

   if (pXmlRotations != NULL)
      {
      // get rotations assocated with this farm types
      TiXmlElement* pXmlRotation = pXmlRotations->FirstChildElement("rotation");

      while (pXmlRotation != NULL)
         {
         LPTSTR name = NULL;
         int    id = 0;
         LPTSTR rotations = 0;
         float  initPctArea = 0;

         XML_ATTR attrs[] = { // attr          type        address      isReq checkCol
                            { "name",          TYPE_STRING,   &name,        true,  0 },
                            { "id",            TYPE_INT,      &id,          true,  0 },
                            { "sequence",      TYPE_STRING,   &rotations,   true,  0 },
                            { "init_pct_area", TYPE_FLOAT,    &initPctArea, true,  0 },
                            { NULL,            TYPE_NULL,     NULL,         false, 0 } };

         bool ok = TiXmlGetAttributes(pXmlRotation, attrs, filename);

         if (ok)
            {
            FarmRotation* pRotation = new FarmRotation;

            pRotation->m_name = name;
            pRotation->m_rotationID = id;

            // parse sequence
            TCHAR* buffer = new TCHAR[lstrlen(rotations) + 1];
            lstrcpy(buffer, rotations);
            TCHAR* next = NULL;
            TCHAR* token = _tcstok_s(buffer, _T(","), &next);

            while (token != NULL)
               {
               int attrCode = atoi(token);

               int index = (int)pRotation->m_sequenceArray.Add(attrCode);
               pRotation->m_sequenceMap.SetAt(attrCode, index);    // BUG ALERT!~!!! fails if the same crops shows up twice in thw sequence
               token = _tcstok_s(NULL, _T(","), &next);
               }
            delete[] buffer;

            pRotation->m_initPctArea = initPctArea;

            m_rotationMap.SetAt(id, pRotation);
            m_rotationArray.Add(pRotation);
            }

         pXmlRotation = pXmlRotation->NextSiblingElement("rotation");
         }
      }  // end of:  if ( pXmlRotations != NULL )


   // next, farm_types
   TiXmlElement* pXmlFarmTypes = pXmlFarmModel->FirstChildElement("farm_types");
   if (pXmlFarmTypes == NULL)
      {
      CString msg("Farm Model: missing <farm_types> element in input file ");
      msg += filename;
      Report::ErrorMsg(msg);
      return false;
      }

   TiXmlElement* pXmlFarmType = pXmlFarmTypes->FirstChildElement("farm_type");
   if (pXmlFarmTypes == NULL)
      {
      CString msg("Farm Model: missing <farm_types> element in input file ");
      msg += filename;
      Report::ErrorMsg(msg);
      return false;
      }

   while (pXmlFarmType != NULL)
      {
      FarmType* pFarmType = new FarmType;

      CString rotations;

      // lookup fields
      XML_ATTR attrs[] = {
         // attr                  type           address                            isReq checkCol
         { "name",                TYPE_CSTRING,  &(pFarmType->m_name),              true,  0 },
         { "id",                  TYPE_INT,      &(pFarmType->m_type),              true,  0 },
         { "code",                TYPE_CSTRING,  &(pFarmType->m_code),              true,  0 },
         { "rotations",           TYPE_CSTRING,  &rotations,                        true,  0 },
         { "expand_radius_hq",    TYPE_FLOAT,    &(pFarmType->m_searchRadiusHQ),    false,  0 },
         { "expand_radius_field", TYPE_FLOAT,    &(pFarmType->m_searchRadiusField), false,  0 },
         { "expand_types",        TYPE_CSTRING,  &(pFarmType->m_expandTypesStr),    false,  0 },
         { "expand_regions",      TYPE_CSTRING,  &(pFarmType->m_expandRegionsStr),  false,  0 },
         { NULL,                  TYPE_NULL,     NULL,                              false, 0 } };

      ok = TiXmlGetAttributes(pXmlFarmType, attrs, filename, NULL);

      if (!ok)
         {
         CString msg;
         msg = "Farm Model:  Error reading <farm_type> tag in input file ";
         msg += filename;
         Report::ErrorMsg(msg);
         return false;
         }
      else
         {
         if (pFarmType->m_type >= 0)
            {
            m_farmTypeArray.Add(pFarmType);
            this->m_farmTypeMap.SetAt(pFarmType->m_type, pFarmType);

            // parse rotations
            int nTokenPos = 0;
            CString rotID = rotations.Tokenize(_T(", "), nTokenPos);
            while (!rotID.IsEmpty())
               {
               int _rotID = atoi(rotID);

               FarmRotation* pRotation = NULL;
               BOOL found = m_rotationMap.Lookup(_rotID, pRotation);

               if (found && pRotation != NULL)
                  pFarmType->m_rotationArray.Add(pRotation);
               else
                  {
                  CString msg;
                  msg.Format("Farm Model: Unrecognized rotation code '%s' found when reading farm type '%s'", (LPCTSTR)rotID, (LPCTSTR)pFarmType->m_name);
                  }

               rotID = rotations.Tokenize(_T(", "), nTokenPos);
               }
            }
         else
            {
            delete pFarmType;
            }
         }
      pXmlFarmType = pXmlFarmType->NextSiblingElement("farm_type");
      }  // end of: while  pXmlFarmType != NULL


   // fix up expand types
   for (int i = 0; i < m_farmTypeArray.GetSize(); i++)
      {
      FarmType* pFarmType = m_farmTypeArray[i];

      // parse expand types
      int nTokenPos = 0;
      CString expType = pFarmType->m_expandTypesStr.Tokenize(_T(", "), nTokenPos);
      while (!expType.IsEmpty())
         {
         bool found = false;
         for (int i = 0; i < (int)m_farmTypeArray.GetSize(); i++)
            {
            if (m_farmTypeArray[i]->m_code.CompareNoCase(expType) == 0)
               {
               pFarmType->m_expandTypes.Add(m_farmTypeArray[i]);
               found = true;
               break;
               }
            }

         if (!found)
            {
            CString msg;
            msg.Format("Farm Model: expand_type '%s' not found when parsing <farm_type> '%s' - this type will be ignored.", (LPCTSTR)expType, (LPCTSTR)pFarmType->m_name);
            Report::LogWarning(msg);
            }

         expType = pFarmType->m_expandTypesStr.Tokenize(_T(", "), nTokenPos);
         }

      // parse expand regions
      ::Tokenize((LPCTSTR)pFarmType->m_expandRegionsStr, ",", pFarmType->m_expandRegions);
      }

   TiXmlElement* pXmlFarmExp = pXmlFarmModel->FirstChildElement("farm_expansion");
   if (pXmlFarmExp != NULL)
      {
      CString rate;
      int maxFarmAreaHa = -1;
      int maxFieldAreaHa = -1;

      XML_ATTR attrs[] = {
         // attr               type          address              isReq checkCol
         { "enable",           TYPE_BOOL,     &m_enableFarmExpansion, false, 0 },
         { "rate",             TYPE_CSTRING,  &rate,              true,  0 },
         { "expansion_period", TYPE_INT,      &m_reExpandPeriod,  true,  0 },
         { "max_farm_area_ha", TYPE_INT,      &maxFarmAreaHa,     false, 0 },   // defaults to 1500 ha
         { "max_field_area_ha",TYPE_INT,      &maxFieldAreaHa,    false, 0 },   // defaults to 80 ha
         { NULL,               TYPE_NULL,     NULL,               false, 0 } };

      ok = TiXmlGetAttributes(pXmlFarmExp, attrs, filename, NULL);

      if (!ok)
         {
         CString msg;
         msg = "Farm Model:  Error reading <farm_expansion> tag in input file ";
         msg += filename;
         Report::ErrorMsg(msg);
         return false;
         }

      if (maxFarmAreaHa > 0)
         m_maxFarmArea = int(maxFarmAreaHa * M2_PER_HA);

      if (maxFieldAreaHa > 0)
         m_maxFieldArea = int(maxFieldAreaHa * M2_PER_HA);

      // parse rates
      m_farmExpRateTable.ClearRows();

      TCHAR* values = new TCHAR[lstrlen(rate) + 2];
      lstrcpy(values, rate);

      TCHAR* nextToken = NULL;
      LPTSTR token = _tcstok_s(values, _T(",() ;\r\n"), &nextToken);

      float pair[2];
      while (token != NULL)
         {
         pair[0] = (float)atof(token);
         token = _tcstok_s(NULL, _T(",() ;\r\n"), &nextToken);
         pair[1] = (float)atof(token);
         token = _tcstok_s(NULL, _T(",() ;\r\n"), &nextToken);

         m_farmExpRateTable.AppendRow(pair, 2);
         }

      delete[] values;
      }

   TiXmlElement* pXmlFCTs = pXmlFarmModel->FirstChildElement("farm_count_trajectories");
   if (pXmlFCTs != NULL)
      {
      XML_ATTR attrs[] = {
         // attr               type          address              isReq checkCol
            { "enable",           TYPE_BOOL,     &m_enableFCT,       false, 0 },
            //{ "region_col",       TYPE_CSTRING,  &m_fctRegionField,     true,  0 },
            { NULL,               TYPE_NULL,     NULL,               false, 0 } };

      ok = TiXmlGetAttributes(pXmlFCTs, attrs, filename, NULL);

      if (!ok)
         {
         CString msg;
         msg = "Farm Model:  Error reading <farm_count_trajectories> tag in input file ";
         msg += filename;
         Report::ErrorMsg(msg);
         return false;
         }

      TiXmlElement* pXmlFCT = pXmlFCTs->FirstChildElement("fct");

      while (pXmlFCT != NULL)
         {
         CString ftCode;
         int region = -1;
         float annualDelta = 0;

         // lookup fields
         XML_ATTR attrs[] = {
            // attr               type          address   isReq checkCol
               { "ft_code",      TYPE_CSTRING,  &ftCode,  true,  0 },
               { "region",       TYPE_INT,      &region,  true,  0 },
               { "annual_delta", TYPE_FLOAT,    &annualDelta, true, 0 },
               { NULL,           TYPE_NULL,     NULL,     false, 0 } };

         ok = TiXmlGetAttributes(pXmlFCT, attrs, filename, NULL);

         if (!ok)
            {
            CString msg;
            msg = "Farm Model:  Error reading <fct> tag in input file ";
            msg += filename;
            Report::ErrorMsg(msg);
            }
         else
            {
            FARMTYPE farmType = FT_UNK;
            for (int i = 0; i < m_farmTypeArray.GetSize(); i++)
               {
               if (ftCode.CompareNoCase(m_farmTypeArray[i]->m_code) == 0)
                  {
                  farmType = m_farmTypeArray[i]->m_type;
                  break;
                  }
               }

            if (farmType == FT_UNK)
               {
               CString msg;
               msg.Format("Farm Model:  Unrecognized Farm Type '%s' reading <fct> tag input file %s", (LPCTSTR)ftCode, (LPCTSTR)filename);
               Report::LogError(msg);
               }
            else
               {
               FCT_INFO* pInfo = new FCT_INFO(region, farmType, annualDelta);
               m_fctArray.Add(pInfo);
               m_fctMap.SetAt(pInfo->Key(), pInfo);
               }
            }

         pXmlFCT = pXmlFCT->NextSiblingElement("fct");
         }
      }

   TiXmlElement* pXmlFSTs = pXmlFarmModel->FirstChildElement("farm_size_trajectories");
   if (pXmlFSTs != NULL)
      {
      XML_ATTR attrs[] = {
         // attr               type          address              isReq checkCol
         { "enable",           TYPE_BOOL,     &m_enableFST,       false, 0 },
         { NULL,               TYPE_NULL,     NULL,               false, 0 } };

      ok = TiXmlGetAttributes(pXmlFCTs, attrs, filename, NULL);

      if (!ok)
         {
         CString msg;
         msg = "Farm Model:  Error reading <farm_size_trajectories> tag in input file ";
         msg += filename;
         Report::ErrorMsg(msg);
         return false;
         }

      TiXmlElement* pXmlFST = pXmlFSTs->FirstChildElement("fst");

      while (pXmlFST != NULL)
         {
         CString ftCode;
         int region = -1;
         float annualDelta = 0;

         // lookup fields
         XML_ATTR attrs[] = {
            // attr               type          address   isReq checkCol
               { "ft_code",      TYPE_CSTRING,  &ftCode,  true,  0 },
               { "region",       TYPE_INT,      &region,  true,  0 },
               { "annual_delta", TYPE_FLOAT,    &annualDelta, true, 0 },
               { NULL,           TYPE_NULL,     NULL,     false, 0 } };

         ok = TiXmlGetAttributes(pXmlFST, attrs, filename, NULL);

         if (!ok)
            {
            CString msg;
            msg = "Farm Model:  Error reading <fst> tag in input file ";
            msg += filename;
            Report::ErrorMsg(msg);
            }
         else
            {
            FARMTYPE farmType = FT_UNK;
            for (int i = 0; i < m_farmTypeArray.GetSize(); i++)
               {
               if (ftCode.CompareNoCase(m_farmTypeArray[i]->m_code) == 0)
                  {
                  farmType = m_farmTypeArray[i]->m_type;
                  break;
                  }
               }

            if (farmType == FT_UNK)
               {
               CString msg;
               msg.Format("Farm Model:  Unrecognized Farm Type '%s' reading <fst> tag input file %s", (LPCTSTR)ftCode, (LPCTSTR)filename);
               Report::LogError(msg);
               }
            else
               {
               FST_INFO* pInfo = new FST_INFO(region, farmType, annualDelta);
               m_fstArray.Add(pInfo);
               m_fstMap.SetAt(pInfo->Key(), pInfo);
               }
            }

         pXmlFST = pXmlFST->NextSiblingElement("fst");
         }

      TiXmlElement* pXmlVSMB = pXmlFarmModel->FirstChildElement("vsmb");

      if (pXmlVSMB)
         {
         LPCTSTR paramFile = pXmlVSMB->Attribute("param_file");
         m_vsmb.LoadParamFile(paramFile);
         }

      }


   // process tracking IDs (if any)
   if (trackIDs != NULL && trackIDs[0] != '*')   // 'track' tag defined?
      {
      CStringArray idArray;
      ::Tokenize(trackIDs, ",", idArray);

      for (int i = 0; i < (int)idArray.GetSize(); i++)
         {
         CString token(idArray[i]);
         token.Trim();

         if (token.GetLength() == 0)
            continue;

         if (::isalpha(token[0]))  // is it an Event name?  Then add to the event tracker
            {
            int eventID = -1;
            for (int i = 0; i < this->m_cropEventTypes.GetSize(); i++)
               {
               if (token.CompareNoCase(this->m_cropEventTypes[i]->label) == 0)
                  {
                  eventID = i;
                  break;
                  }
               }

            if (eventID >= 0)
               {
               int index = (int)m_trackCropEventArray.Add(eventID);
               m_trackCropEventMap.SetAt(eventID, index);
               }
            }
         else
            {
            // it's an idu index 
            }

         int idu = atoi((LPCTSTR)idArray[i]);
         int index = (int)m_trackIDUArray.Add(idu);
         m_trackIDUMap.SetAt(idu, index);
         }
      }


   CString msg;
   msg.Format("Farm Model: %i farm types, %i rotations found while reading %s", (int)m_farmTypeArray.GetSize(), (int)m_rotationArray.GetSize(),
      (LPCTSTR)filename);
   Report::Log(msg);
   return true;
   }


bool FarmModel::IsCrop(Farm* pFarm, MapLayer* pLayer)
   {
   for (int i = 0; i < (int)pFarm->m_fieldArray.GetSize(); i++)
      {
      FarmField* pField = pFarm->m_fieldArray[i];

      for (int j = 0; j < pField->m_iduArray.GetSize(); j++)
         {
         int idu = pField->m_iduArray[j];
         int lulcA = 0;
         pLayer->GetData(idu, m_colLulcA, lulcA);

         if (lulcA == ANNUAL_CROP || lulcA == PERENNIAL_CROP)
            return true;
         }
      }

   return false;
   }

bool FarmModel::IsAnnualCrop(Farm* pFarm, MapLayer* pLayer)
   {
   for (int i = 0; i < (int)pFarm->m_fieldArray.GetSize(); i++)
      {
      FarmField* pField = pFarm->m_fieldArray[i];

      for (int j = 0; j < pField->m_iduArray.GetSize(); j++)
         {
         int idu = pField->m_iduArray[j];
         int lulcA = 0;
         pLayer->GetData(idu, m_colLulcA, lulcA);

         if (lulcA == ANNUAL_CROP)
            return true;
         }
      }

   return false;
   }
