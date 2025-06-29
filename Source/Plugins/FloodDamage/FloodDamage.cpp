// FloodDamage.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"

#include "FloodDamage.h"

#include <Maplayer.h>
#include <tixml.h>
#include <Path.h>
#include <PathManager.h>
#include <Scenario.h>
#include <EnvConstants.h>

#include <iostream>
#include <filesystem>

#include <direct.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


extern "C" _EXPORT EnvExtension * Factory(EnvContext*) { return (EnvExtension*) new FloodDamage; }

void CaptureException();



// Propagate() is called ONCE when the event first runs.
// It uses the Hazus Data returns from the model run and
// and translates it into the IDUs in Envision
bool AHEvent::Propagate(EnvContext* pEnvContext)
   {
   //---------------------
   // Building Damage
   //---------------------
   // grab input file generated from damage model (csv)
   CString msg("Acute Hazards: reading building damage parameter file ");
   msg += this->m_earthquakeInputPath;
   Report::Log(msg);

   CString path;
   path.Format("%s/EQ_%s_building_damage_%s_%i.csv", this->m_earthquakeInputPath,
      this->m_earthquakeScenario, pEnvContext->pScenario->m_name, pEnvContext->runID);
   int rows = this->m_earthquakeData.ReadAscii(path, ',', 0);

   ///////
   ///this->m_earthquakeData.WriteAscii("/Envision/studyAreas/OrCoast/Hazus/output/EQ_M90_building_damage_test.csv");

   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;
   int idus = pIDULayer->GetRowCount();
   ASSERT(rows == idus);

   // pull out needed columns
   PtrArray<HazDataColInfo> eqColInfos;
   eqColInfos.Add(new HazDataColInfo{ "DS", -1 });
   eqColInfos.Add(new HazDataColInfo{ "rep_time", -1 });
   eqColInfos.Add(new HazDataColInfo{ "rep_cost",  -1 });
   eqColInfos.Add(new HazDataColInfo{ "habitable", -1 });
   eqColInfos.Add(new HazDataColInfo{ "ls_ftly",  -1 });
   eqColInfos.Add(new HazDataColInfo{ "ls_injy",  -1 });

   // find the CSV column associated with each statistic type
   for (int i = 0; i < eqColInfos.GetSize(); i++)
      {
      HazDataColInfo* pInfo = eqColInfos[i];
      pInfo->col = this->m_earthquakeData.GetCol(pInfo->field);
      ASSERT(pInfo->col >= 0);
      }

   //---------------------
   // Repeat for TSUNAMI
   //---------------------

   msg = "Acute Hazards: reading tsunami damage parameter file ";
   msg += this->m_tsunamiInputPath;

   //tsunami_input = "/Envision/studyAreas/OrCoast/Hazus/output/TSU_L1_MaxMF_building_damage.csv"

   path.Format("%s/TSU_%s_building_damage_%s_%i.csv", this->m_tsunamiInputPath,
      this->m_tsunamiScenario, pEnvContext->pScenario->m_name, pEnvContext->runID);

   this->m_tsunamiData.ReadAscii(path, ',', 0);

   // pull out needed columns
   PtrArray<HazDataColInfo> tsuColInfos;
   tsuColInfos.Add(new HazDataColInfo{ "DS",        -1 });
   tsuColInfos.Add(new HazDataColInfo{ "rep_time",  -1 });
   tsuColInfos.Add(new HazDataColInfo{ "rep_cost",  -1 });
   tsuColInfos.Add(new HazDataColInfo{ "habitable", -1 });
   tsuColInfos.Add(new HazDataColInfo{ "ls_ftly",  -1 });
   tsuColInfos.Add(new HazDataColInfo{ "ls_injy",  -1 });

   //tsuColInfos.Add(new HazDataColInfo{ "CasSev1",  -1 });  // casuality severitys
   //tsuColInfos.Add(new HazDataColInfo{ "CasSev2",  -1 });
   //tsuColInfos.Add(new HazDataColInfo{ "CasSev3",  -1 });
   //tsuColInfos.Add(new HazDataColInfo{ "CasSev4",  -1 });

   // find the CSV column associated with each statistic type
   for (int i = 0; i < tsuColInfos.GetSize(); i++)
      {
      HazDataColInfo* pInfo = tsuColInfos[i];
      pInfo->col = this->m_tsunamiData.GetCol(pInfo->field);
      ASSERT(pInfo->col >= 0);
      }

   // iterate through IDUs, populating data on hazard states as needed
   ASSERT(idus == m_earthquakeData.GetRowCount());
   ASSERT(idus == m_tsunamiData.GetRowCount());

   enum HAZARD_METRIC_INDEX { HMI_DS, HMI_REP_TIME, HMI_REP_COST, HMI_HABITABLE, HMI_CASFTLY, HMI_CASINJY };

   int currentYear = pEnvContext->currentYear;

   for (int idu = 0; idu < idus; idu++)
      {
      // we'll start with damage state
      int damageIndexEq = m_earthquakeData.GetAsInt(eqColInfos[HMI_DS]->col, idu);
      int damageIndexTsu = m_tsunamiData.GetAsInt(tsuColInfos[HMI_DS]->col, idu);

      int damageIndex = max(damageIndexEq, damageIndexTsu);
      //if (damageIndexEq == damageIndexTsu && damageIndex < 4)
      //   damageIndex++;

      // bldg damage 0(none)-4(max damage)
      m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduBldgDamageEq, damageIndexEq, ADD_DELTA);
      m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduBldgDamageTsu, damageIndexTsu, ADD_DELTA);
      m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduBldgDamage, damageIndex, ADD_DELTA);

      // was the building damaged? Then set repair time in IDUs
      if (damageIndex > 0)
         {
         float timeToRepair = 0;
         if (damageIndexEq > damageIndexTsu)
            timeToRepair = m_earthquakeData.GetAsFloat(eqColInfos[HMI_REP_TIME]->col, idu);
         else
            timeToRepair = m_tsunamiData.GetAsFloat(eqColInfos[HMI_REP_TIME]->col, idu);

         int yearsToRepair = int(timeToRepair / 365);
         //yearsToRepair++;    // round up
         // for time to restore, we populate the following fields:
         // IDU field "REPAIR_YRS" (int) - put the year the building will be restored
         // IDU field "BLDG_STATUS" (int) - put the year the building status (0=normal, 1=being restored, 2=uninhabitable)
         m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduRepairYrs, yearsToRepair, ADD_DELTA);
         m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduBldgStatus, 1, ADD_DELTA);

         // repair costs
         float repCost = m_earthquakeData.GetAsFloat(eqColInfos[HMI_REP_COST]->col, idu);

         //ASSERT(repCost > 0);
         if (repCost > 0)
            {
            // for time to restore, we populate the following fields:
            // IDU field "BD_REPAIR" (float) - cost of any repair ($1000's of $)
            float impValue = 0;
            pIDULayer->GetData(idu, m_pAHModel->m_colIduImprValue, impValue);
            //float repairCost = impValue * repCostFrac / 1000000.0f;  // M$, since that's what the budget is
            float repCostM = repCost / 1000000.0f;  // M$, since that's what the budget is
            m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduRepairCost, repCostM, ADD_DELTA);
            }

         // habitable
         float randVal = (float)m_pAHModel->m_randUniform.RandValue();   // 0-1
         float pHab = m_earthquakeData.GetAsFloat(eqColInfos[HMI_HABITABLE]->col, idu);

         int habitable = (randVal >= pHab) ? 0 : 1;
         m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduHabitable, habitable, ADD_DELTA);
         }  // end of: if ( damageIndex > 0 )

      // life safety next
      //float casSev1 = m_earthquakeData.Get(eqColInfos[HMI_CASSEV1]->col, idu);  // people
      //float casSev2 = m_earthquakeData.Get(eqColInfos[HMI_CASSEV2]->col, idu);
      //float casSev3 = m_earthquakeData.Get(eqColInfos[HMI_CASSEV3]->col, idu);
      //float casSev4 = m_earthquakeData.Get(eqColInfos[HMI_CASSEV4]->col, idu);
      //float casTotal = casSev1 + casSev2 + casSev3 + casSev4;
      //m_pAHModel->m_numCasSev1 += casSev1;
      //m_pAHModel->m_numCasSev2 += casSev2;
      //m_pAHModel->m_numCasSev3 += casSev3;
      //m_pAHModel->m_numCasSev4 += casSev4;
      //
      //m_pAHModel->m_numCasTotal += casTotal;
      float injuriesEQ = m_earthquakeData.GetAsFloat(eqColInfos[HMI_CASFTLY]->col, idu);  // people
      float fatalitiesEQ = m_earthquakeData.GetAsFloat(eqColInfos[HMI_CASINJY]->col, idu);
      float injuriesTSU = m_tsunamiData.GetAsFloat(tsuColInfos[HMI_CASFTLY]->col, idu);  // people
      float fatalitiesTSU = m_tsunamiData.GetAsFloat(tsuColInfos[HMI_CASINJY]->col, idu);
      float injuries = injuriesEQ + injuriesTSU;
      float fatalities = fatalitiesEQ + fatalitiesTSU;

      m_pAHModel->m_numInjuriesEQ += injuriesEQ;
      m_pAHModel->m_numFatalitiesEQ += fatalitiesEQ;
      m_pAHModel->m_numCasualitiesEQ += (injuriesEQ + fatalitiesEQ);

      m_pAHModel->m_numInjuriesTSU += injuriesTSU;
      m_pAHModel->m_numFatalitiesTSU += fatalitiesTSU;
      m_pAHModel->m_numCasualitiesTSU += (injuriesTSU + fatalitiesTSU);


      m_pAHModel->m_numInjuries += injuries;
      m_pAHModel->m_numFatalities += fatalities;
      m_pAHModel->m_numCasualities += (injuries + fatalities);

      m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduInjuries, injuries, ADD_DELTA);
      m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduFatalities, fatalities, ADD_DELTA);
      m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduCasualties, injuries + fatalities, ADD_DELTA);
      }  // end of: for each IDU

   return true;
   }


///////////////////////////////////////////////////////
//                 FloodDamage
///////////////////////////////////////////////////////

FloodDamage::FloodDamage(void)
   : m_randLogNormal(1, 1, 0)
   , m_randNormal(1, 1, 0)
   , m_randUniform(0.0, 1.0, 0)
   , m_colIduRepairYrs(-1)
   , m_colIduBldgStatus(-1)
   , m_colIduRepairCost(-1)
   , m_colIduHabitable(-1)
   , m_colIduBldgDamage(-1)
   , m_colIduBldgDamageEq(-1)
   , m_colIduBldgDamageTsu(-1)
   , m_colIduImprValue(-1)
   //, m_annualRepairCosts(0)
   , m_nUninhabitableStructures(0)
   , m_nIDUsDamaged(0)
   , m_nIDUsDamaged1(0)
   , m_nIDUsDamaged2(0)
   , m_nIDUsDamaged3(0)
   , m_nIDUsDamaged4(0)
   , m_nIDUsRepaired(0)
   , m_nIDUsBeingRepaired(0)
   , m_totalBldgs(0)
   , m_nInhabitableStructures(0)
   , m_pctInhabitableStructures(0)
   , m_nFunctionalBldgs(0)
   , m_pctFunctionalBldgs(0)
   { }

FloodDamage::~FloodDamage(void)
   {
   // Clean up
   //Py_Finalize();   // clean up python instance
   }

// override API Methods
bool FloodDamage::Init(EnvContext* pEnvContext, LPCTSTR initStr)
   {
   bool ok = LoadXml(pEnvContext, initStr);
   if (!ok)
      return FALSE;

   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   // make sure necesary columns exist
   this->CheckCol(pIDULayer, m_colIduNDUs, "NDU", TYPE_INT, CC_MUST_EXIST);
   this->CheckCol(pIDULayer, m_colIduImprValue, "IMPR_VALUE", TYPE_INT, CC_MUST_EXIST);
   this->CheckCol(pIDULayer, m_colIduRepairYrs, "REPAIR_YRS", TYPE_INT, CC_AUTOADD);
   this->CheckCol(pIDULayer, m_colIduBldgStatus, "BLDGSTATUS", TYPE_INT, CC_AUTOADD);
   this->CheckCol(pIDULayer, m_colIduRepairCost, "REPAIRCOST", TYPE_FLOAT, CC_AUTOADD);
   this->CheckCol(pIDULayer, m_colIduHabitable, "HABITABLE", TYPE_INT, CC_AUTOADD);
   this->CheckCol(pIDULayer, m_colIduBldgDamage, "BLDGDAMAGE", TYPE_INT, CC_AUTOADD);
   this->CheckCol(pIDULayer, m_colIduBldgDamageEq, "BLDGDMGEQ", TYPE_INT, CC_AUTOADD);
   this->CheckCol(pIDULayer, m_colIduBldgDamageTsu, "BLDGDMGTS", TYPE_INT, CC_AUTOADD);
   this->CheckCol(pIDULayer, m_colIduRemoved, "REMOVED", TYPE_INT, CC_MUST_EXIST);
   this->CheckCol(pIDULayer, m_colIduCasualties, "CASUALTIES", TYPE_INT, CC_AUTOADD);
   this->CheckCol(pIDULayer, m_colIduInjuries, "INJURIES", TYPE_INT, CC_AUTOADD);
   this->CheckCol(pIDULayer, m_colIduFatalities, "FATALITIES", TYPE_INT, CC_AUTOADD);
   this->CheckCol(pIDULayer, m_colBldgType, "BLDGTYPE", TYPE_INT, CC_AUTOADD);

   // initialize column info
   pIDULayer->SetColData(m_colIduRepairYrs, VData(-1), true);
   pIDULayer->SetColData(m_colIduBldgStatus, VData(0), true);
   pIDULayer->SetColData(m_colIduRepairCost, VData(0.0f), true);
   pIDULayer->SetColData(m_colIduHabitable, VData(1), true);
   pIDULayer->SetColData(m_colIduBldgDamage, VData(0), true);
   pIDULayer->SetColData(m_colIduBldgDamageEq, VData(0), true);
   pIDULayer->SetColData(m_colIduBldgDamageTsu, VData(0), true);
   pIDULayer->SetColData(m_colIduCasualties, VData(0), true);
   pIDULayer->SetColData(m_colIduInjuries, VData(0), true);
   pIDULayer->SetColData(m_colIduFatalities, VData(0), true);


   // add output variables
   //this->AddOutputVar("Annual Repair Expenditures", m_annualRepairCosts, "");
   this->AddOutputVar("Habitable Structures (count)", m_nInhabitableStructures, "");
   this->AddOutputVar("Habitable Structures (pct)", m_pctInhabitableStructures, "");
   this->AddOutputVar("Functional Bldgs (count)", m_nFunctionalBldgs, "");
   this->AddOutputVar("Functional Bldgs (pct)", m_pctFunctionalBldgs, "");
   this->AddOutputVar("Uninhabitable Structures", m_nUninhabitableStructures, "");
   
   this->AddOutputVar("IDUs with Slight Damage", m_nIDUsDamaged1, "");
   this->AddOutputVar("IDUs with Moderate Damage", m_nIDUsDamaged2, "");
   this->AddOutputVar("IDUs with Extensive Damage", m_nIDUsDamaged3, "");
   this->AddOutputVar("IDUs with Complete Damage", m_nIDUsDamaged4, "");

   this->AddOutputVar("Total No. IDUs Damaged", m_nIDUsDamaged, "");
   this->AddOutputVar("Total No. IDUs Damaged/Repaired", m_nIDUsDamagedAndRepaired, "");
   this->AddOutputVar("IDUs Repaired", m_nIDUsRepaired, "");
   this->AddOutputVar("IDUs Being Repaired", m_nIDUsBeingRepaired, "");

   // life safety
   this->AddOutputVar("Casualties (Total)", m_numCasualities, "");
   this->AddOutputVar("Fatalities (Total)", m_numFatalities, "");
   this->AddOutputVar("Injuries (Total)", m_numInjuries, "");

   UpdateBldgType(pEnvContext, false);

   InitPython();

   return TRUE;
   }

bool FloodDamage::InitRun(EnvContext* pEnvContext, bool useInitialSeed)
   {
   // reset annual output variables
   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   pIDULayer->SetColData(m_colIduRepairYrs, VData(-1), true);
   pIDULayer->SetColData(m_colIduBldgStatus, VData(0), true);
   pIDULayer->SetColData(m_colIduRepairCost, VData(0.0f), true);
   pIDULayer->SetColData(m_colIduHabitable, VData(1), true);
   pIDULayer->SetColData(m_colIduBldgDamage, VData(0), true);
   pIDULayer->SetColData(m_colIduCasualties, VData(0), true);
   pIDULayer->SetColData(m_colIduInjuries, VData(0), true);
   pIDULayer->SetColData(m_colIduFatalities, VData(0), true);

   m_nUninhabitableStructures = 0;
   //m_nDUsDamaged1 = 0;
   //m_nDUsDamaged2 = 0;
   //m_nDUsDamaged3 = 0;
   //m_nDUsDamaged4 = 0;
   //m_nDUsDamaged = 0;
   //m_nDUsDamagedAndRepaired = 0;
   //m_nDUsRepaired = 0;
   //m_nDUsBeingRepaired = 0;

   m_nIDUsDamaged1 = 0;
   m_nIDUsDamaged2 = 0;
   m_nIDUsDamaged3 = 0;
   m_nIDUsDamaged4 = 0;
   m_nIDUsDamaged = 0;
   m_nIDUsDamagedAndRepaired = 0;
   m_nIDUsRepaired = 0;
   m_nIDUsBeingRepaired = 0;

   m_nInhabitableStructures = 0;
   m_pctInhabitableStructures = 0;
   m_totalBldgs = 0;
   m_nFunctionalBldgs = 0;
   m_pctFunctionalBldgs = 0;
   
   m_numInjuriesEQ = 0;
   m_numFatalitiesEQ = 0;
   m_numCasualitiesEQ = 0;

   m_numInjuriesTSU = 0;
   m_numFatalitiesTSU = 0;
   m_numCasualitiesTSU = 0;

   m_numInjuries = 0; 
   m_numFatalities = 0;
   m_numCasualities = 0;

   int idus = pIDULayer->GetRowCount();
   for (int idu = 0; idu < idus; idu++)
      {
      float imprValue = 0;
      pIDULayer->GetData(idu, m_colIduImprValue, imprValue);

      if (imprValue > 10000)
         m_totalBldgs++;

      m_nInhabitableStructures = m_totalBldgs;
      m_pctInhabitableStructures = 1.0f;
      m_nFunctionalBldgs = m_totalBldgs;
      m_pctFunctionalBldgs = 1.0f;
      }

   // run any scheduled events
   for (int i = 0; i < m_events.GetSize(); i++)
      m_events[i]->m_status = AHS_PRE_EVENT;

   return TRUE;
   }

bool FloodDamage::Run(EnvContext* pEnvContext)
   {
   int currentYear = pEnvContext->currentYear;

   // reset annual output variables
   m_nUninhabitableStructures = 0;
   //m_nDUsDamaged = 0;
   //m_nDUsDamaged1 = 0;
   //m_nDUsDamaged2 = 0;
   //m_nDUsDamaged3 = 0;
   //m_nDUsDamaged4 = 0;
   //m_nDUsDamagedAndRepaired = 0;
   //m_nDUsRepaired = 0;
   //m_nDUsBeingRepaired = 0;

   m_nIDUsDamaged = 0;
   m_nIDUsDamaged1 = 0;
   m_nIDUsDamaged2 = 0;
   m_nIDUsDamaged3 = 0;
   m_nIDUsDamaged4 = 0;
   m_nIDUsDamagedAndRepaired = 0;
   m_nIDUsRepaired = 0;
   m_nIDUsBeingRepaired = 0;



   m_nInhabitableStructures = 0;
   m_pctInhabitableStructures = 0;
   m_totalBldgs = 0;
   m_nFunctionalBldgs = 0;
   m_pctFunctionalBldgs = 0;

   // update building stats
   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;
   int idus = pIDULayer->GetRowCount();
   for (int idu = 0; idu < idus; idu++)
      {
      float imprValue = 0;
      pIDULayer->GetData(idu, m_colIduImprValue, imprValue);

      if (imprValue > 10000)
         m_totalBldgs++;
      }

   m_nInhabitableStructures = m_totalBldgs;
   m_pctInhabitableStructures = 1.0f;
   m_nFunctionalBldgs = m_totalBldgs;
   m_pctFunctionalBldgs = 1.0f;

   // update results from any prior events if needed
   for (int i = 0; i < m_events.GetSize(); i++)
      {
      if (m_events[i]->m_use && m_events[i]->m_status == AHS_POST_EVENT)
         {
         Report::Log_i("Processing prior year event (%i)", m_events[i]->m_year);
         Update(pEnvContext);
         break;
         }
      }

   // run any events that are scheduled for this year
   for (int i = 0; i < m_events.GetSize(); i++)
      {
      AHEvent* pEvent = m_events[i];

      if (pEvent->m_use && pEvent->m_year == currentYear)
         {
         Report::Log_i("Processing current year event (%i)", m_events[i]->m_year);
         pEvent->Run(pEnvContext);
         }
      }

   return TRUE;
   }



void FloodDamage::UpdateBldgType(EnvContext* pEnvContext, bool useDelta)
   {
   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;
   CString propInd, landUse;
   int bedrooms = 0;
   int imprVal = 0;
   int removed = 0;

   int colPropInd = pIDULayer->GetFieldCol("prop_ind");
   int colLandUse = pIDULayer->GetFieldCol("landuse");
   int colBedrooms = pIDULayer->GetFieldCol("bedrooms");
   int colImprVal = pIDULayer->GetFieldCol("impr_value");
   int colRemoved = pIDULayer->GetFieldCol("REMOVED");
   int colBldgType = pIDULayer->GetFieldCol("BldgType");


   int bldgType = 0;
   int floors = 1;

   // iterate through map, computing report values
   for (MapLayer::Iterator idu = pIDULayer->Begin(); idu < pIDULayer->End(); idu++)
      {
      pIDULayer->GetData(idu, colPropInd, propInd);
      pIDULayer->GetData(idu, colLandUse, landUse);
      pIDULayer->GetData(idu, colBedrooms, bedrooms);
      pIDULayer->GetData(idu, colImprVal, imprVal);
      pIDULayer->GetData(idu, colRemoved, removed);

      int bldgType = 0;
      int floors = 0;

      if (propInd == "AGRICULTURAL")
         {
         if (landUse == "FARMS")
            {
            bldgType = 1; floors = 1;
            }
         else if (landUse == "FOREST")
            {
            bldgType = 1; floors = 1;
            }
         else if (landUse = "FISHERIES")
            {
            bldgType = 1; floors = 1;
            }
         else if (landUse = "FIELD & SEED")
            {
            bldgType = 1; floors = 1;
            }
         }

      else if (propInd == "SINGLE FAMILY RESIDENCE")
         {
         if (bedrooms >= 4)
            {
            bldgType = 2; floors = 2;
            }
         else
            {
            bldgType = 1; floors = 1;
            }
         }

      else if (propInd == "APARTMENT")
         {
         if (bedrooms >= 4)
            {
            bldgType = 2; floors = 2;
            }
         else
            {
            bldgType = 1; floors = 1;
            }
         }

      else if (propInd == "MOBILE HOME PARK")
         {
         bldgType = 1; floors = 1;
         }

      else if (propInd == "PARKING")
         {
         bldgType = 3; floors = 4;
         }   //  assuming that parking structures are concrete buildings

      else if (propInd == "AMUSEMENT-RECREATION")
         {
         bldgType = 6; floors = 4;
         }   //  assuming that AMUSEMENT - RECREATION buildings are steel moment frame buildings

      else if (propInd == "COMMERCIAL BUILDING")
         {
         bldgType = 3; floors = 3;
         }

      else if (propInd == "COMMERCIAL CONDOMINIUM")
         {
         bldgType = 3; floors = 4;
         }

      else if (landUse == "CONDOMINIUM PROJECT")
         {
         bldgType = 3; floors = 4;
         }

      else if (propInd == "APARTMENT")
         {
         if (bedrooms >= 4)
            {
            bldgType = 2; floors = 2;
            }
         else
            {
            bldgType = 1; floors = 1;
            }
         }

      else if (propInd == "DUPLEX")
         {
         bldgType = 2; floors = 2;
         }

      else if (propInd == "FINANCIAL INSTITUTION")
         {
         bldgType = 5; floors = 2;
         }

      else if (propInd == "HOTEL, MOTEL")
         {
         if (landUse == "HOTEL")
            {
            bldgType = 3; floors = 5;
            }
         else if (landUse == "MOTEL")
            {
            bldgType = 2; floors = 2;
            }
         else
            {
            bldgType = 2; floors = 2;
            }
         }

      else if (propInd == "INDUSTRIAL")
         {
         bldgType = 6; floors = 1;
         }

      else if (propInd == "INDUSTRIAL HEAVY")
         {
         bldgType = 6; floors = 1;
         }

      else if (propInd == "OFFICE BUILDING")
         {
         bldgType = 5; floors = 3;
         }

      else if (propInd == "RELIGIOUS")
         {
         bldgType = 4; floors = 3;
         }

      else if (propInd == "RETAIL")
         {
         if (landUse == "FOOD STORES")
            {
            bldgType = 5; floors = 1;
            }
         else if (landUse == "SHOPPING CENTER")
            {
            bldgType = 6; floors = 3;
            }
         else if (landUse == "SUPERMARKET")
            {
            bldgType = 6; floors = 1;
            }
         else if (landUse == "STORE BUILDING")
            {
            bldgType = 5; floors = 1;
            }
         else if (landUse == "AUTO SALES")
            {
            bldgType = 6; floors = 3;
            }
         else
            {
            bldgType = 5; floors = 3;
            }
         }

      else if (propInd == "SERVICE")
         {
         if (landUse == "SCHOOL")
            {
            bldgType = 4; floors = 3;
            }
         else if (landUse == "PUBLIC SCHOOL")
            {
            bldgType = 4; floors = 3;
            }
         else if (landUse == "BAR")
            {
            bldgType = 5; floors = 2;
            }
         else if (landUse == "RESTAURANT BUILDING")
            {
            bldgType = 5; floors = 1;
            }
         else if (landUse == "NIGHTCLUB")
            {
            bldgType = 5; floors = 2;
            }
         else if (landUse == "LAUNDROMAT")
            {
            bldgType = 6; floors = 1;
            }
         else if (landUse == "FAST FOOD FRANCHISE")
            {
            bldgType = 5; floors = 1;
            }
         else if (landUse == "AUTO REPAIR")
            {
            bldgType = 3; floors = 2;
            }
         else if (landUse == "CARWASH")
            {
            bldgType = 6; floors = 1;
            }
         else if (landUse == "FUNERAL HOME")
            {
            bldgType = 2; floors = 2;
            }
         else if (landUse == "SERVICE STATION")
            {
            bldgType = 3; floors = 3;
            }
         else
            {
            bldgType = 4; floors = 3;
            }
         }

      else if (landUse == "STATE PROPERTY")
         {
         bldgType = 3; floors = 1;
         }

      else if (landUse == "MUNICIPAL PROPERTY")
         {
         bldgType = 3; floors = 1;
         }

      else if (landUse == "COUNTY PROPERTY")
         {
         bldgType = 3; floors = 1;
         }

      else if (propInd == "TRANSPORT")
         {
         bldgType = 6; floors = 1;
         }

      else if (landUse == "COMMERCIAL (NEC)")
         {
         bldgType = 6; floors = 1;
         }

      else if (propInd == "UTILITIES")
         {
         bldgType = 6; floors = 1;
         }

      else if (propInd == "WAREHOUSE")
         {
         bldgType = 6; floors = 1;
         }

      else if (landUse == "MISC BUILDING")
         {
         bldgType = 3; floors = 1;
         }

      else if (landUse == "MIXED COMPLEX")
         {
         bldgType = 4; floors = 1;
         }

      else if (landUse == "EASEMENT")
         {
         bldgType = 1; floors = 1;
         }

      else if (landUse == "LEASED LAND/BLDG")
         {
         bldgType = 3; floors = 1;
         }

      if (imprVal <= 10000)
         {
         bldgType = 0; floors = 0;
         }

      if (removed > 0)
         {
         bldgType = 0; floors = 0;
         }


      UpdateIDU(pEnvContext, idu, colBldgType, bldgType, useDelta ? ADD_DELTA : SET_DATA);
      }  // end of: for each IDU
   }


bool FloodDamage::InitPython()
   {
   /*
   Report::Log("Acute Hazards:  Initializing embedded Python interpreter...");

   char cwd[512];
   _getcwd(cwd, 512);

   CString _path(PathManager::GetPath(PM_PROJECT_DIR));
   _path += "Hazus";
   _chdir(_path);

   // launch a python instance and run the model
   Py_SetProgramName(L"Envision");  // argv[0]  deprecated

   wchar_t path[512];
   int cx = swprintf(path, 512, L"%hs/DLLs;%hs/Lib;%hs/Lib/site-packages",
      (LPCTSTR)m_pythonPath, (LPCTSTR)m_pythonPath, (LPCTSTR)m_pythonPath);

   Py_SetPath(path);

   PyImport_AppendInittab("redirection", PyInit_redirection);

   Py_Initialize();

   //CString code;
   //code.Format("sys.path.append('%s')", (LPCTSTR)this->m_pyModulePath);
   int retVal = PyRun_SimpleString("import sys");
   //retVal = PyRun_SimpleString(code);

   // add Python function for redirecting output on the Python side
   retVal = PyRun_SimpleString("\
import redirection\n\
import sys\n\
class StdoutCatcher:\n\
    def write(self, stuff):\n\
        redirection.stdoutredirect(stuff)\n\
sys.stdout = StdoutCatcher()");

   Report::Log("Acute Hazards: Python initialization complete...");
   */
   return true;
   }



/*
bool FloodDamage::InitPython()
   {
   Report::Log("Acute Hazards:  Initializing embedded Python interpreter...");

   char cwd[512];
   _getcwd(cwd, 512);

   CString _path(PathManager::GetPath(PM_PROJECT_DIR));
   _path += "Hazus";
   _chdir(_path);

   // launch a python instance and run the model
   Py_SetProgramName(L"Envision");  // argv[0]  deprecated

   ////////
   //wchar_t path[512];
   //int cx = swprintf(path, 512, L"%hs/DLLs;%hs/Lib;%hs/Lib/site-packages",
   //   (LPCTSTR) m_pythonPath, (LPCTSTR) m_pythonPath, (LPCTSTR) m_pythonPath);
   //
   //Py_SetPath(path);
//
//   PyImport_AppendInittab("redirection", PyInit_redirection);
//
//   Py_Initialize();
//
//   //CString code;
//   //code.Format("sys.path.append('%s')", (LPCTSTR)this->m_pyModulePath);
//   int retVal = PyRun_SimpleString("import sys");
//   //retVal = PyRun_SimpleString(code);
//
//   // add Python function for redirecting output on the Python side
//   retVal = PyRun_SimpleString("\
//import redirection\n\
//import sys\n\
//class StdoutCatcher:\n\
//    def write(self, stuff):\n\
//        redirection.stdoutredirect(stuff)\n\
//sys.stdout = StdoutCatcher()");
   /////////////


   ///// NEW
   PyConfig config;
   PyStatus status;
   PyConfig_InitPythonConfig(&config);
   config.isolated = 1;
   config.buffered_stdio = 0;
   status = PyConfig_SetBytesString(&config, &config.program_name, "Envision");
   if (PyStatus_Exception(status))
      {
      PyConfig_Clear(&config);
      Py_ExitStatusException(status);
      return false;
      }

   PyImport_AppendInittab("redirection", PyInit_redirection);

   status = Py_InitializeFromConfig(&config);
   if (PyStatus_Exception(status))
      {
      PyConfig_Clear(&config);
      Py_ExitStatusException(status);
      return false;
      }

   int retVal = PyRun_SimpleString("\
import sys\n\
import redirection\n\
import sys\n\
class StdoutCatcher:\n\
    def write(self, stuff):\n\
        redirection.stdoutredirect(stuff)\n\
sys.stdout = StdoutCatcher()");

   Report::Log("Acute Hazards: Python initialization complete...");

   return true;
   }
*/



// Update is called during Run() processing.
// For any events that are in AHS_POST_EVENT status,
// do any needed update of IDUs recovering from that event 
// Notes: 
//  1) Any building tagged for repair will have "BLDG_DAMAGE" set to a negative value

bool FloodDamage::Update(EnvContext* pEnvContext)
   {
   // basic idea is to loop through IDUs', applying any post-event
   // changes to the landscape, e.g. habitability, recovery period, etc.
   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   //int idus = this->m_earthquakeData.GetRowCount();
   int idus = pIDULayer->GetRowCount();

   // loop through IDU's, interpreting the probability information
   // from the damage model into the IDUs
   int removed = 0;

   for (int idu = 0; idu < idus; idu++)
      {
      float imprValue = 0;
      pIDULayer->GetData(idu, m_colIduImprValue, imprValue);

      int _removed = 0;
      pIDULayer->GetData(idu, m_colIduRemoved, _removed);
      if (_removed > 0)
         removed += 1; // _removed;

      if (imprValue > 10000 && _removed <= 0)
         {
         int damage = 0;
         pIDULayer->GetData(idu, m_colIduBldgDamage, damage);

         int nDUs = 0;
         pIDULayer->GetData(idu, m_colIduNDUs, nDUs);

         switch (damage)
            {
            case 1:  m_nIDUsDamaged1++; /*m_nDUsDamaged1 += nDUs;*/ break;
            case 2:  m_nIDUsDamaged2++; /*m_nDUsDamaged2 += nDUs;*/ break;
            case 3:  m_nIDUsDamaged3++; /*m_nDUsDamaged3 += nDUs;*/ break;
            case 4:  m_nIDUsDamaged4++; /*m_nDUsDamaged4 += nDUs;*/ break;
            }

         // if the damage is negative, we are actively repairing it,
         // so indicate that it has been repaired

         if (damage < 0)
            {
            //float repairCost = 0;
            //pIDULayer->GetData(idu, m_colIduRepairCost, repairCost);

            int repairYears = 0;
            pIDULayer->GetData(idu, m_colIduRepairYrs, repairYears);

            if (repairYears > 0)
               {
               m_nIDUsBeingRepaired++;
               m_nIDUsBeingRepaired += nDUs;
               UpdateIDU(pEnvContext, idu, m_colIduRepairYrs, repairYears - 1, ADD_DELTA);
               }
            else  // building is repaired, so indicate that in the IDUs
               {
               m_nIDUsRepaired++;
               //m_nDUsRepaired += nDUs;
               UpdateIDU(pEnvContext, idu, m_colIduRepairYrs, -1, ADD_DELTA);
               UpdateIDU(pEnvContext, idu, m_colIduRepairCost, 0.0f, ADD_DELTA);
               UpdateIDU(pEnvContext, idu, m_colIduBldgStatus, 0, ADD_DELTA);
               UpdateIDU(pEnvContext, idu, m_colIduHabitable, 1, ADD_DELTA);
               UpdateIDU(pEnvContext, idu, m_colIduBldgDamage, 0, ADD_DELTA);
               }
            }

         int habitable = 0;
         pIDULayer->GetData(idu, m_colIduHabitable, habitable);
         if (habitable == 0)
            m_nUninhabitableStructures++;

         int _removed = 0;
         pIDULayer->GetData(idu, m_colIduRemoved, _removed);
         if (_removed > 0)
            removed += 1; // _removed;
         }
      }  // end of: for each idu

   // summary stats
   m_nIDUsDamaged = m_nIDUsDamaged1 + m_nIDUsDamaged2 + m_nIDUsDamaged3 + m_nIDUsDamaged4;
   //m_nDUsDamaged = m_nDUsDamaged1 + m_nDUsDamaged2 + m_nDUsDamaged3 + m_nDUsDamaged4;
   m_totalBldgs += removed;
   m_nIDUsDamagedAndRepaired = m_nIDUsDamaged + m_nIDUsRepaired + m_nIDUsBeingRepaired;
   //m_nDUsDamagedAndRepaired = m_nDUsDamaged + m_nDUsRepaired + m_nDUsBeingRepaired;
   m_nInhabitableStructures = m_totalBldgs - m_nUninhabitableStructures;
   m_pctInhabitableStructures = float(m_nInhabitableStructures) / m_totalBldgs;
   m_nFunctionalBldgs = m_totalBldgs - m_nIDUsDamaged;
   m_pctFunctionalBldgs = float(m_nIDUsDamaged) / m_totalBldgs;

   return true;
   }


bool FloodDamage::LoadXml(EnvContext* pEnvContext, LPCTSTR filename)
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
   TiXmlElement* pXmlRoot = doc.RootElement();  // <acute_hazards>
   CString codePath;

   XML_ATTR attrs[] = {
      // attr            type           address            isReq checkCol
      { "python_path",  TYPE_CSTRING,   &m_pythonPath,     true,  0 },
      { NULL,           TYPE_NULL,     NULL,               false, 0 } };
   ok = TiXmlGetAttributes(pXmlRoot, attrs, filename, NULL);

   TiXmlElement* pXmlEvent = pXmlRoot->FirstChildElement("event");
   if (pXmlEvent == NULL)
      {
      CString msg("Acute Hazards: no <event>'s defined");
      msg += filename;
      Report::ErrorMsg(msg);
      return false;
      }

   while (pXmlEvent != NULL)
      {
      AHEvent* pEvent = new AHEvent;

      CString pyModulePath;

      XML_ATTR attrs[] = {
         // attr                  type           address            isReq checkCol
         { "year",                TYPE_INT,      &pEvent->m_year,           true,  0 },
         { "name",                TYPE_CSTRING,  &pEvent->m_name,           true,  0 },
         { "use",                 TYPE_INT,      &pEvent->m_use,            true,  0 },
         { "py_function",         TYPE_CSTRING,  &pEvent->m_pyFunction,     true,  0 },
         { "py_module",           TYPE_CSTRING,  &pyModulePath,             true,  0 },
         { "env_output",          TYPE_CSTRING,  &pEvent->m_envOutputPath,  true,  0 },
         { "earthquake_input",    TYPE_CSTRING,  &pEvent->m_earthquakeInputPath,   true,  0 },
         { "tsunami_input",       TYPE_CSTRING,  &pEvent->m_tsunamiInputPath,   true,  0 },
         { "earthquake_scenario", TYPE_CSTRING,  &pEvent->m_earthquakeScenario, true,  0 },
         { "tsunami_scenario",    TYPE_CSTRING,  &pEvent->m_tsunamiScenario, true,  0 },
         { NULL,                  TYPE_NULL,     NULL,        false, 0 } };

      ok = TiXmlGetAttributes(pXmlEvent, attrs, filename, NULL);

      if (!ok)
         {
         delete pEvent;
         CString msg;
         msg = "Acute Hazards Model:  Error reading <event> tag in input file ";
         msg += filename;
         Report::ErrorMsg(msg);
         return false;
         }
      else
         {
         nsPath::CPath path(pyModulePath);
         pEvent->m_pyModuleName = path.GetTitle();
         pEvent->m_pyModulePath = path.GetPath();
         pEvent->m_pAHModel = this;

         m_events.Add(pEvent);
         }

      pXmlEvent = pXmlEvent->NextSiblingElement("event");
      }

   return true;
   }

/*
void CaptureException()
   {
   PyObject* exc_type = NULL, * exc_value = NULL, * exc_tb = NULL;
   PyErr_Fetch(&exc_type, &exc_value, &exc_tb);
   PyObject* str_exc_type = PyObject_Repr(exc_type); //Now a unicode object
   PyObject* pyStr = PyUnicode_AsEncodedString(str_exc_type, "utf-8", "Error ~");
   const char* msg = PyBytes_AS_STRING(pyStr);

   Report::LogError(msg);

   Py_XDECREF(str_exc_type);
   Py_XDECREF(pyStr);

   Py_XDECREF(exc_type);
   Py_XDECREF(exc_value);
   Py_XDECREF(exc_tb);
   }
   */