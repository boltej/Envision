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
// DynamicVeg.cpp : Defines the initialization routines for the DLL.

#include "stdafx.h"
#pragma hdrstop

#include <PathManager.h>
#include <iostream>
#include "DynamicVeg.h"
#include <Maplayer.h>
#include <map.h>
#include <Vdataobj.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
#include <ctime>
#include <cstdlib>
#include <random>
#include <numeric>
#include <Report.h>
#include <tixml.h>
#include <omp.h>
#include "GDALWrapper.h"
#include "GeoSpatialDataObj.h"
#include <afxtempl.h>
#include <initializer_list>
#include <deltaarray.h>
#include <EnvModel.h>

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


extern "C" _EXPORT EnvExtension * Factory(EnvContext * pContext)
   {
   return (EnvExtension*) new  DynamicVeg;
   }


//PtrArray< MC1_OUTPUT > DynamicVeg::m_mc1_outputArray;

PtrArray< OUTPUT > DynamicVeg::m_outputArray;

USEVEGTRANSTYPE DynamicVeg::m_useVegTransFlag;

VEGTRANSFILE DynamicVeg::m_vegtransfile;

INITIALIZER DynamicVeg::m_initializer;

PVT DynamicVeg::m_dynamic_update;

RandUniform   DynamicVeg::m_rn;

bool DynamicVeg::m_staticsInitialized = false;

VDataObj DynamicVeg::m_inputtable(U_UNDEFINED);
VDataObj DynamicVeg::m_deterministic_inputtable(U_UNDEFINED);
VDataObj DynamicVeg::m_probMultiplier_inputtable(U_UNDEFINED);
VDataObj DynamicVeg::m_MC1_siteindex(U_UNDEFINED);
VDataObj DynamicVeg::m_MC1_pvt(U_UNDEFINED);

int DynamicVeg::m_colMC1cell = -1;
int DynamicVeg::m_colMC1row = -1;
int DynamicVeg::m_colMC1col = -1;
int DynamicVeg::m_colVegClass = -1;
int DynamicVeg::m_colPriorVegClass = -1;
int DynamicVeg::m_colDisturb = -1;
int DynamicVeg::m_colAgeClass = -1;
int DynamicVeg::m_ageClass = -1;
int DynamicVeg::m_colPVT = -1;
int DynamicVeg::m_colLAI = -1;
int DynamicVeg::m_colCarbon = -1;
int DynamicVeg::m_colCursi = -1;
int DynamicVeg::m_colFutsi = -1;
int DynamicVeg::m_colRegen = -1;
int DynamicVeg::m_colVariant = -1;
int DynamicVeg::m_colTIV = -1;
int DynamicVeg::m_colTSD = -1;
int DynamicVeg::m_colVegTranType = -1;
int DynamicVeg::m_colManage = -1;
int DynamicVeg::m_colRegion = -1;
int DynamicVeg::m_colCalcStandAge = -1;
int DynamicVeg::m_mc1_output = -1;
int DynamicVeg::m_disturb = 0;
int DynamicVeg::m_cursi = -1;
int DynamicVeg::m_futsi = -1;
int DynamicVeg::m_pvt = -1;
int DynamicVeg::m_regen = 0;
int DynamicVeg::m_variant = -1;
int DynamicVeg::m_mc1row = -1;
int DynamicVeg::m_mc1col = -1;
int DynamicVeg::m_mc1pvt = -1;
int DynamicVeg::m_mc1Cell = -1;
int DynamicVeg::m_manage = -1;
int DynamicVeg::m_coverType = -1;
int DynamicVeg::m_structureStage = -1;
int DynamicVeg::m_region = -1;
int DynamicVeg::m_si = 0;
int DynamicVeg::m_TSD = -1;
int DynamicVeg::m_selected_to_trans = -1;
int DynamicVeg::m_selected_pvtto_trans = -1;
int DynamicVeg::m_vegClass = -1;

float DynamicVeg::m_mc1si = 0;
float DynamicVeg::m_selected_prob = 0;
float DynamicVeg::m_LAI = 0;
float DynamicVeg::m_carbon = 0;
float DynamicVeg::m_carbon_conversion_factor = 8.89563e-5f;
bool DynamicVeg::m_validFlag = false;
bool DynamicVeg::m_useProbMultiplier = false;
bool DynamicVeg::m_climateChange = false;
bool DynamicVeg::m_flagProbabilisticTransition = false;
bool DynamicVeg::m_flagDeterministicFile = false;
bool DynamicVeg::m_flagDeterministicTransition = false;
bool DynamicVeg::m_pvtProbMultiplier = false;
bool DynamicVeg::m_vegClassProbMultipier = false;
bool DynamicVeg::m_useLAI = false;
bool DynamicVeg::m_useCarbon = false;
bool DynamicVeg::m_useVariant = false;

vector<vector<vector<int> > > DynamicVeg::m_vpvt;  //3d vector to hold MC1 pvt data [year,row,column]

vector<vector<vector<float> > > DynamicVeg::m_vsi;  //3d vector to hold MC1 si data [year,row,column]

map<ProbKeyclass, std::vector< std::pair<int, float> >, probclasscomp> DynamicVeg::probmap;

map<ProbKeyclass, std::vector< std::pair<int, float> >, probclasscomp> DynamicVeg::probmap2;

map<ProbMultiplierPVTKeyclass, std::vector<float>, ProbMultiplierPVTClassComp> DynamicVeg::m_probMultiplierPVTMap;

map<ProbMultiplierVegClassKeyclass, std::vector<float>, ProbMultiplierVegClassClassComp> DynamicVeg::m_probMultiplierVegClassMap;

map<ProbIndexKeyclass, std::vector<int>, ProbIndexClassComp> DynamicVeg::m_probIndexMap;

map<TSDIndexKeyclass, std::vector<int>, TSDIndexClassComp> DynamicVeg::m_TSDIndexMap;

map<DeterminIndexKeyClass, std::vector<int>, DeterminIndexClassComp> DynamicVeg::m_determinIndexMap;

map<DeterministicKeyclass, std::vector< std::pair<int, int> >, deterministicclasscomp> DynamicVeg::m_deterministic_trans;

ProbKeyclass DynamicVeg::m_probInsertKey, DynamicVeg::m_probLookupKey;

DeterministicKeyclass DynamicVeg::m_deterministicInsertKey, DynamicVeg::m_deterministicLookupKey;

ProbIndexKeyclass DynamicVeg::m_probIndexInsertKey, DynamicVeg::m_probIndexLookupKey;

TSDIndexKeyclass DynamicVeg::m_TSDIndexInsertKey, DynamicVeg::m_TSDIndexLookupKey;

DeterminIndexKeyClass DynamicVeg::m_determinIndexInsertKey, DynamicVeg::m_determinIndexLookupKey;

ProbMultiplierPVTKeyclass DynamicVeg::m_probMultiplierPVTInsertKey, DynamicVeg::m_probMultiplierPVTLookupKey;

ProbMultiplierVegClassKeyclass  DynamicVeg::m_probMultiplierVegClassInsertKey, DynamicVeg::m_probMultiplierVegClassLookupKey;

CArray< CString, CString > DynamicVeg::m_badVegTransClasses;

CArray< CString, CString > DynamicVeg::m_badDeterminVegTransClasses;

// added - pb
CArray < int > DynamicVeg::m_vegTransitionArray;
CArray < int > DynamicVeg::m_disturbArray;





DynamicVeg::DynamicVeg() : EnvModelProcess()
   {
   //AddInputVar( _T("MC1 MC1_output"), m_mc1_output, _T("Index of MC1 mc1_output: 0-hadleyA2, 1-mirocA2, 2-csiroA2") );
   AddInputVar(_T("FORESTC conversion factor"), m_carbon_conversion_factor, _T("conversion factor for FORESTC"));
   };

DynamicVeg::~DynamicVeg()
   {

   }

bool DynamicVeg::Init(EnvContext* pEnvContext, LPCTSTR initStr)
   {
   const MapLayer* pLayer = pEnvContext->pMapLayer;

   // get input file names and input variables   
   if (m_staticsInitialized)
      return TRUE;

   m_vegTransitionArray.SetSize(pLayer->GetRecordCount());

   m_disturbArray.SetSize(pLayer->GetRecordCount());

   for (INT_PTR i = 0; i < pLayer->GetRecordCount(); i++)
      {
      m_vegTransitionArray[i] = 0;
      m_disturbArray[i] = 0;
      }

   m_staticsInitialized = true;

   clock_t start = clock();

   bool ok = LoadXml(initStr);

   clock_t finish = clock();
   double duration = (float)(finish - start) / CLOCKS_PER_SEC;
   CString msg;
   //msg.Format( "DynamicVeg: Loaded inputs file (%.2f seconds)", (float) duration );
   //Report::Log( msg );

   if (!ok)
      {
      msg = ("Unable to find DynamicVeg's .xml init file");
      Report::ErrorMsg(msg);
      return FALSE;
      }

   // a switch is set in .xml file if user wants type of VEGCLASS transtion sent to VEGTRANTYPE in IDU layer
   if (m_useVegTransFlag.useVegTransFlag == 1)
      CheckCol(pLayer, m_colVegTranType, "VEGTRNTYPE", TYPE_INT, CC_AUTOADD);

   for (int i = 0; i < (int)m_outputArray.GetSize(); i++)
      {
      OUTPUT* pOutput = m_outputArray[i];
      AddOutputVar(pOutput->name, pOutput->value, "");
      }

   CString tempinistring;
   CString inputString(initStr);
   CString* pString = &inputString;


   // choose which climate change files to use if necessary
   //MC1_OUTPUT *pMC1_output = m_mc1_outputArray[ m_mc1_output ]; 

   // check and store relevant columns
   CheckCol(pLayer, m_colVegClass, "VEGCLASS", TYPE_INT, CC_MUST_EXIST);
   CheckCol(pLayer, m_colDisturb, "DISTURB", TYPE_INT, CC_MUST_EXIST);
   CheckCol(pLayer, m_colPVT, "PVT", TYPE_INT, CC_MUST_EXIST);
   CheckCol(pLayer, m_colTSD, "TSD", TYPE_INT, CC_MUST_EXIST);
   CheckCol(pLayer, m_colRegion, "REGION", TYPE_INT, CC_MUST_EXIST);
   CheckCol(pLayer, m_colAgeClass, "AGECLASS", TYPE_INT, CC_MUST_EXIST);

   m_colRegen = pLayer->GetFieldCol("REGEN");
   m_colMC1cell = pLayer->GetFieldCol("MC1CELL");
   m_colMC1row = pLayer->GetFieldCol("MC1ROW");
   m_colMC1col = pLayer->GetFieldCol("MC1COL"); //hack, will go away with GDAL NetCDF file reader
   m_colCursi = pLayer->GetFieldCol("CURSI");
   m_colFutsi = pLayer->GetFieldCol("FUTSI");
   m_colLAI = pLayer->GetFieldCol("LAI");
   m_colCarbon = pLayer->GetFieldCol("FORESTC");
   m_colVariant = pLayer->GetFieldCol("VARIANT");
   m_colTIV = pLayer->GetFieldCol("TIV");
   m_colPriorVegClass = pLayer->GetFieldCol("PRIOR_VEG");
   if (m_colLAI >= 0) m_useLAI = true;
   if (m_colCarbon >= 0) m_useCarbon = true;

   //if probability multiplier file exist
   if (m_vegtransfile.probMultiplier_filename != "")
      {
      start = clock();

      LoadProbMultiplierCSV(m_vegtransfile.probMultiplier_filename, pEnvContext);

      finish = clock();
      duration = (float)(finish - start) / CLOCKS_PER_SEC;
      msg.Format("Loaded Probability Multiplier CSV file '%s'(%.2f seconds)", (LPCTSTR)m_vegtransfile.probMultiplier_filename, (float)duration);
      Report::Log(msg);

      m_useProbMultiplier = true;
      }


   //if these files exist, climate change handle through changing site index and pvt   
   // if (pMC1_output->siteIndexFile != "" | pMC1_output->pvtFile != "") 
   //{

   //if ( ! LoadMC1PvtSiCSV( pMC1_output->siteIndexFile, pMC1_output->pvtFile, pEnvContext) )
   //  return FALSE;

   //m_climateChange = true;
   // 
   ////set initial conditions in IDU layer
   //for ( MapLayer::Iterator idu = pEnvContext->pMapLayer->Begin( ); idu != pEnvContext->pMapLayer->End(); idu++ )
   //   {
   //      
   //   ((MapLayer *)(pEnvContext->pMapLayer))->SetData(idu, m_colRegen, 1);

   //   pLayer->GetData( idu, m_colMC1row,   m_mc1row );//hack until gdal netcdf file reader
   //   pLayer->GetData( idu, m_colMC1col,   m_mc1col );
   //    
   //   //set year arg to 0 for first year initialization
   //   PvtSiteindex2DeltaArr(pEnvContext, idu, m_mc1row, m_mc1col, 0); //also sets m_mc1si and m_mc1pvt

   //   }
   //}

   // go to LoadProbCSV funtion for loading file into data object, filename set in .xml file
   start = clock();

   ok = LoadProbCSV(m_vegtransfile.probability_filename, pEnvContext);

   finish = clock();
   duration = (float)(finish - start) / CLOCKS_PER_SEC;
   msg.Format("Loaded Probability CSV file '%s' (%.2f seconds)", (LPCTSTR)m_vegtransfile.probability_filename, (float)duration);
   Report::Log(msg);

   if (!ok)
      return FALSE;

   // check to see if columns exist in both lookup table and IDU layer
   if (m_colVariant >= 0 && m_useVariant)
      {
      m_useVariant = true;
      }
   else
      {
      m_useVariant = false;
      }

   // go to LoadDeterministicTransCSV funtion for loading file into data object, filename set in .xml file
   start = clock();

   m_flagDeterministicFile = LoadDeterministicTransCSV(m_vegtransfile.deterministic_filename, pEnvContext);

   finish = clock();
   duration = (float)(finish - start) / CLOCKS_PER_SEC;
   msg.Format("Loaded Deterministic Transition CSV file '%s' (%.2f seconds)", (LPCTSTR)m_vegtransfile.deterministic_filename, (float)duration);
   Report::Log(msg);

   return TRUE;
   }

bool DynamicVeg::InitRun(EnvContext* pEnvContext, bool useInitSeed)
   {
   // MC1_OUTPUT *pMC1_output = m_mc1_outputArray[ m_mc1_output ];

   // Hack for now until better NetCDF option is developed
   // go to LoadMC1PvtSiCSV funtion for loading file into data object, index 2 MC1 Site index, index 3 MC1 PVT is choosen as first in input string in .envx file
   /*if (pMC1_output->siteIndexFile != "" | pMC1_output->pvtFile != "")
      {
      if ( ! LoadMC1PvtSiCSV( pMC1_output->siteIndexFile, pMC1_output->pvtFile, pEnvContext) )
      return FALSE;
      }*/

   return(TRUE);

   }

bool DynamicVeg::Run(EnvContext* pEnvContext)
   {
   MapLayer* pLayer = (MapLayer*)pEnvContext->pMapLayer;

   int current_year = pEnvContext->currentYear;
   m_probMultiplierPVTLookupKey.timestep = current_year;
   m_probMultiplierVegClassLookupKey.timestep = current_year;

   switch (pEnvContext->id)
      {
      // update AGECLASS, VEGCLASS, PVT, STANDAGE, LAI, CARBON, VEGTRANSTYPE, VARIANT, TSD
      case MODEL_ID::UPDATE_VEG_INFO:   // happens after successional and disturbance transitions in this time step
         RunUpdateVegInfo(pEnvContext);
         break;

      case MODEL_ID::SUCCESSION_TRANSITIONS:
      case MODEL_ID::DISTURBANCE_TRANSITIONS:   // Successional model (ID=0) or Disturbance model (ID=1)
         RunTransitions(pEnvContext);
         break;
      }// end of: switch( modelID)

   ::EnvApplyDeltaArray(pEnvContext->pEnvModel);
   return TRUE;

   }

void DynamicVeg::RunUpdateVegInfo(EnvContext* pEnvContext)
   {
   MapLayer* pLayer = (MapLayer*)pEnvContext->pMapLayer;

   // increment time in age class
   for (INT_PTR i = 0; i < pLayer->GetRecordCount(); i++)
      {
      int idu = (int)i;

      pLayer->GetData(idu, m_colAgeClass, m_ageClass);

      if (m_vegTransitionArray[i] == 0) // no successional or disturbance transitions
         {
         m_flagDeterministicTransition = false;
         m_flagProbabilisticTransition = false;

         //these flags will increment AGECLASS by 1
         UpdateIDUVegInfo(pEnvContext, idu, m_flagProbabilisticTransition, m_flagDeterministicTransition);
         }
      else //reset 1 to 0 for next time step
         {
         m_vegTransitionArray[i] = 0;
         }

      } // end maplayer loop

    // if running succession transitions, reset IDU layer transtion type to zero before each annual step. ID=0 is successional transitions 
   if (pEnvContext->id == MODEL_ID::SUCCESSION_TRANSITIONS && m_useVegTransFlag.useVegTransFlag == 1)
      {
      for (MapLayer::Iterator idu = pLayer->Begin(); idu != pLayer->End(); idu++)
         UpdateIDU(pEnvContext, idu, m_colVegTranType, 0, ADD_DELTA);
      }

   //reset disturb array. this is used to prevent same disturbance from happing in same timestep more than once
   for (INT_PTR d = 0; d < pLayer->GetRecordCount(); d++)
      m_disturbArray[d] = 0;
   }


void DynamicVeg::RunTransitions(EnvContext* pEnvContext)
   {
   MapLayer* pLayer = (MapLayer*)pEnvContext->pMapLayer;

   // if disturbance transitions, count and report current and prior year disturbances
   if (pEnvContext->id == MODEL_ID::DISTURBANCE_TRANSITIONS)
      {
      int npos = 0;
      int nneg = 0;
      for (MapLayer::Iterator idu = pLayer->Begin(); idu != pLayer->End(); idu++)
         {
         pLayer->GetData(idu, m_colDisturb, m_disturb);

         if (m_disturb > 0)
            npos++;
         else if (m_disturb < 0)
            nneg++;
         }

      CString msg;
      msg.Format("Disturbance Transitions: %i disturbances found; (%i prior disturbances) in year %i", npos, nneg, pEnvContext->yearOfRun);
      Report::Log(msg);
      }

   // loop through the IDU
   for (MapLayer::Iterator idu = pLayer->Begin(); idu != pLayer->End(); idu++)
      {
      m_flagProbabilisticTransition = false; //initialize these flags for each IDU
      m_flagDeterministicTransition = false;

      CString msg;

      //climate change attributes      
      if (m_colMC1row != -1)   pLayer->GetData(idu, m_colMC1row, m_mc1row);
      if (m_colMC1col != -1)   pLayer->GetData(idu, m_colMC1col, m_mc1col);
      if (m_colFutsi != -1)    pLayer->GetData(idu, m_colFutsi, m_futsi);
      if (m_colCursi != -1)    pLayer->GetData(idu, m_colCursi, m_cursi);
      if (m_colMC1cell != -1)  pLayer->GetData(idu, m_colMC1cell, m_mc1Cell);

      // optional columns
      if (m_colRegen != -1)   pLayer->GetData(idu, m_colRegen, m_regen);

      pLayer->GetData(idu, m_colDisturb, m_disturb);
      pLayer->GetData(idu, m_colVegClass, m_vegClass);
      pLayer->GetData(idu, m_colPVT, m_pvt);
      pLayer->GetData(idu, m_colTSD, m_TSD);
      pLayer->GetData(idu, m_colAgeClass, m_ageClass);
      pLayer->GetData(idu, m_colRegion, m_region);

      // in the IDU layer previous disturbances can be set to a negative value.
      // So, set to zero only for this plugin to access successional transitions
      // in the probability lookup table. only for process id = 0
      if (m_disturb < 0 && pEnvContext->id == MODEL_ID::SUCCESSION_TRANSITIONS)
         m_disturb = 0;

      //if (m_vegClass >= 1000000)   // not developed
      {
      if (m_useVariant) pLayer->GetData(idu, m_colVariant, m_variant);
      if (m_useVariant) pLayer->GetData(idu, m_colVariant, m_variant);

      if (m_useLAI) pLayer->GetData(idu, m_colLAI, m_LAI);

      if (m_useCarbon) pLayer->GetData(idu, m_colCarbon, m_carbon);

      m_coverType = (int)floor(m_vegClass / 10000.0); //first three digits of VEGCLASS starting from left
      m_structureStage = (int)floor(fabs(((10000.0 * m_coverType) - m_vegClass) / 10.0)); //second three digits of VEGCLASS starting from left
      m_variant = (int)fabs(floor((10 * (floor(fabs((10000.0 * m_coverType) - m_vegClass) / 10.0))) - fabs((10000.0 * m_coverType) - m_vegClass))); //last digit of VEGCLASS from left

      int current_year = pEnvContext->yearOfRun;
      int year_index = 0;

      // dynamic_update is set in the studysite_PVT.xml file (ie Eugene_PVT.xml) see file for explanation 
      if (m_dynamic_update.dynamic_update)
         {
         year_index = current_year;
         }
      else
         {
         year_index = 0;
         }

      if (m_climateChange) //currently climate change is expressed through site index and pvt.
         {
         PvtSiteindex2DeltaArr(pEnvContext, idu, m_mc1row, m_mc1col, year_index); //sets m_mc1si and m_mc1pvt
         }
      else
         {
         m_si = 0;
         pLayer->GetData(idu, m_colPVT, m_pvt);
         }

      // Disturbance Transitions
      if ((m_disturb > 0 && m_disturbArray[idu] != m_disturb && pEnvContext->id == MODEL_ID::DISTURBANCE_TRANSITIONS))   // climate change disturbances     
         {
         DisturbanceTransition(pEnvContext, idu);

         if (m_flagProbabilisticTransition)
            {
            UpdateIDUVegInfo(pEnvContext, idu, m_flagProbabilisticTransition, m_flagDeterministicTransition);
            m_vegTransitionArray[idu] = 1;
            m_disturbArray[idu] = m_disturb;
            }
         }
      else if ((m_disturb == 0 || (m_disturb > CC_DISTURB_LB && m_disturb < CC_DISTURB_UB))
         && pEnvContext->id == MODEL_ID::SUCCESSION_TRANSITIONS)
         // Successional Transitions
         {
         if ((int)m_mc1si == (int)-99.1f) continue; // there may be -99 in input data file
         if ((int)m_mc1pvt == (int)-99.1f) continue;

         // run probabalistic transitions
         ProbabilityTransition(pEnvContext, idu); //  This sets m_select_to_trans to the intended TO transition, also sets m_flagProbablisticTransition flag to true (TO found) or false

         if (m_flagProbabilisticTransition)
            {
            m_vegTransitionArray[idu] = 1;
            }
         else
            {
            DeterministicTransition(pEnvContext, idu);//also sets flag true or false

            if (m_flagDeterministicTransition)
               m_vegTransitionArray[idu] = 1;
            }

         // update TimeInAgeclass, AGECLASS, VEGCLASS, PVT, STANDAGE, LAI, CARBON, VEGTRANSTYPE, VARIANT, TSD
         UpdateIDUVegInfo(pEnvContext, idu, m_flagProbabilisticTransition, m_flagDeterministicTransition);
         }

      if (m_selected_to_trans != m_vegClass && (m_flagProbabilisticTransition || m_flagDeterministicTransition) && m_colPriorVegClass >= 0)
         UpdateIDU(pEnvContext, idu, m_colPriorVegClass, m_vegClass, ADD_DELTA);
      }
      } // end IDU loop
   }





bool DynamicVeg::EndRun(EnvContext*)
   {
   if (m_badVegTransClasses.GetSize() == 0)
      return TRUE;

   int count = (int)m_badVegTransClasses.GetSize();
   CString msg;
   msg.Format("No probabilistic transitions were found for the following %d veg classes: \n"
      "m_vegClass,m_disturb,m_regen,m_si,m_pvt,m_region\n", count);

   int top = min((int)m_badVegTransClasses.GetSize(), 20);

   for (int i = 0; i < top; i++)
      {
      msg += "Missing probabilistic transition: ";
      msg += m_badVegTransClasses[i];
      }

   Report::Log(msg);

   m_badVegTransClasses.RemoveAll();

   if (m_badDeterminVegTransClasses.GetSize() == 0)
      return TRUE;

   count = (int)m_badDeterminVegTransClasses.GetSize();

   msg.Format("No deterministic transitions were found for the following %d veg classes: \n"
      "m_vegClass,m_disturb,m_regen,m_si,m_pvt,m_region\n", count);


   top = min((int)m_badDeterminVegTransClasses.GetSize(), 20);

   for (int i = 0; i < top; i++)
      {
      msg += "Missing deterministic transition: ";
      msg += m_badDeterminVegTransClasses[i];
      }

   Report::Log(msg);

   m_badDeterminVegTransClasses.RemoveAll();

   return TRUE;

   }

bool DynamicVeg::DisturbanceTransition(EnvContext* pEnvContext, int idu)
   {
   bool CCdisturbFlag = CC_DISTURB_LB < m_disturb&& m_disturb < CC_DISTURB_UB;

   switch (m_disturb)
      {
      case HARVEST:
         ProbabilityTransition(pEnvContext, idu);
         break;

      case THINNING:
         ProbabilityTransition(pEnvContext, idu);
         break;

      case PARTIAL_HARVEST:
         ProbabilityTransition(pEnvContext, idu);
         break;

      case RESTORATION_ACTIVITY:
         ProbabilityTransition(pEnvContext, idu);
         break;

      case PLANTATION:
         ProbabilityTransition(pEnvContext, idu);
         break;

      case SELECTION_HARVEST:
         ProbabilityTransition(pEnvContext, idu);
         break;

      case SURFACE_FIRE:
         ProbabilityTransition(pEnvContext, idu);
         break;

      case LOW_SEVERITY_FIRE:
         ProbabilityTransition(pEnvContext, idu);
         break;

      case HIGH_SEVERITY_FIRE:
         ProbabilityTransition(pEnvContext, idu);
         break;

      case STAND_REPLACING_FIRE:
         ProbabilityTransition(pEnvContext, idu);
         break;

      case PRESCRIBED_FIRE:
      case PRESCRIBED_FIRE_2:
      case PRESCRIBED_FIRE_UNDERBURNING:
      case PRESCRIBED_FIRE_UNDERBURNING_2:
      case PRESCRIBED_SURFACE_FIRE:
      case PRESCRIBED_SURFACE_FIRE_2:
      case PRESCRIBED_LOW_SEVERITY_FIRE:
      case PRESCRIBED_LOW_SEVERITY_FIRE_2:
      case PRESCRIBED_HIGH_SEVERITY_FIRE:
      case PRESCRIBED_HIGH_SEVERITY_FIRE_2:
      case PRESCRIBED_STAND_REPLACING_FIRE:
      case PRESCRIBED_STAND_REPLACING_FIRE_2:
         ProbabilityTransition(pEnvContext, idu);
         break;

      case MECHANICAL_THINNING:
      case MECHANICAL_THINNING_2:
         ProbabilityTransition(pEnvContext, idu);
         break;

      case MOWING_GRINDING:
      case MOWING_GRINDING_2:
         ProbabilityTransition(pEnvContext, idu);
         break;

      case SALVAGE_HARVEST:
      case SALVAGE_HARVEST_2:
         ProbabilityTransition(pEnvContext, idu);
         break;

      case SUPRESSION:
      case SUPRESSION_2:
         ProbabilityTransition(pEnvContext, idu);
         break;

      case EVEN_AGED_TREATMENT:
      case EVEN_AGED_TREATMENT_2:
         ProbabilityTransition(pEnvContext, idu);
         break;

      case THIN_FROM_BELOW:
      case THIN_FROM_BELOW_2:
         ProbabilityTransition(pEnvContext, idu);
         break;

      case PARTIAL_HARVEST_LIGHT:
      case PARTIAL_HARVEST_LIGHT_2:
         ProbabilityTransition(pEnvContext, idu);
         break;

      case PARTIAL_HARVEST_HIGH:
      case PARTIAL_HARVEST_HIGH_2:
         ProbabilityTransition(pEnvContext, idu);
         break;

         // Climate change disturbances 
      case FDD2FSI:
      case FDD2FUC:
      case FDD2FVG:
      case FDD2FWI:
      case FDW2FTM:
      case FDW2FVG:

      case FMH2FDD:
      case FMH2FSI:

      case FSI2FDD:
      case FSI2FMH:
      case FSI2FTO:
      case FSI2FWI:
      case FSI2FUC:

      case FTM2FDW:

      case FTO2FUC:

      case FUC2FDD:
      case FUC2FVG:
      case FUC2FWI:
      case FUC2FSI:

      case FVG2FDD:
      case FVG2FDW:
      case FVG2FWI:

      case FWI2FDD:
      case FWI2FSI:
      case FWI2FTO:
      case FWI2FVG:

      case FMH2FWI:
      case FDD2FMH:
      case FDW2FWI:
      case FVG2FSI:
      case FWI2FMH:
      case FTO2FDD:
      case FTO2FWI:
      case FMH2FVG:
      case FSI2FTM:
      case FWI2FTM:
      case FVG2FTM:
      case FSI2FDW:
      case FDD2FTM:
      case FDD2FDW:
      case FWI2FDW:
      case FSI2FVG:
      case FDD2FTO:
      case FMH2FTO:
      case FWI2FUC:
      case FMH2FUC:
      case FVG2FUC:
      case FTO2FVG:
      case FVG2FTO:
         ProbabilityTransition(pEnvContext, idu);
         break;

      default:
         if (CCdisturbFlag && m_unseenCCtrans[CC_DISTURB_UB - m_disturb])
            { // Unrecognized climate change disturbance transition
            CString msg;
            msg.Format("Unrecognized climate change transition, m_disturb = %d", m_disturb);
            Report::Log(msg);
            }
         break;

      }
   if (CCdisturbFlag) m_unseenCCtrans[CC_DISTURB_UB - m_disturb] = false;

   return TRUE;

   }

bool DynamicVeg::ProbabilityTransition(EnvContext* pEnvContext, int idu)
   {
   const MapLayer* pLayer = pEnvContext->pMapLayer;

   CString msg;
   int mc1sito10;
   float probability_sum;

   if (pEnvContext->id == MODEL_ID::SUCCESSION_TRANSITIONS)
      m_flagProbabilisticTransition = false;

   std::vector< std::pair<int, float> >* m_final_probs = 0;
   std::vector< float >* m_probMultiplier = 0;
   std::vector< std::pair<int, float> >* m_original_final_probs;

   m_selected_to_trans = -1;

   // m_cursi in IDU layer is an integer, ie 114. Lookup table for site index is in multiples of ten
   // so, round to nearest 10. multiples of 5 round up.
   mc1sito10 = RoundToNearest10(m_mc1si);

   if (m_climateChange)
      {
      m_probLookupKey.si = mc1sito10;
      m_probLookupKey.pvt = m_mc1pvt;
      }
   else
      {
      m_probLookupKey.si = m_si;
      m_probLookupKey.pvt = m_pvt;
      }

   //generate a lookup key from current variables in the IDU layer for the prob lookup table

   // handle persrcribed fire the same as low-severty fire
   int disturb = GetAdjustedDisturb(m_disturb);

   m_probLookupKey.from = m_vegClass;
   m_probLookupKey.regen = m_regen;
   m_probLookupKey.region = m_region;
   m_probLookupKey.disturb = disturb;

   m_final_probs = &probmap[m_probLookupKey]; //returns the proper probabilities from the lookup table

   // Only call this catch for DISTURB greater than zero.  Meaning, not all IDUs will have a "successional"
   //  probabilistic transition, so don't send to log file 
   if (m_final_probs->size() == 0 && m_disturb > 0)
      {
      if (m_vegClass >= 2000000 && (m_disturb<CC_DISTURB_LB || m_disturb>CC_DISTURB_UB))
         {
         bool alreadySeen = false;
         msg.Format("%i %i %i %i %i %i\n", m_vegClass, m_disturb, m_regen, m_si, m_pvt, m_region);

         int list_length = (int)m_badVegTransClasses.GetSize();
         int max_list_length = 1000;
         if (0 < list_length && list_length < max_list_length)
            for (int i = 0; i < list_length; i++)
               {
               if (m_badVegTransClasses[i] == msg)
                  {
                  alreadySeen = true;
                  break;
                  }
               }

         if (!alreadySeen && list_length < max_list_length)
            m_badVegTransClasses.Add(msg);
         }

      return m_flagProbabilisticTransition;
      } // end of if ( m_final_probs->size() == 0 )

   probability_sum = 0.0f;

   // sum probability list. Second is the second of the vector pair or the probability
   for (vector<pair<int, float>>::iterator it = m_final_probs->begin(); it != m_final_probs->end(); ++it)
      {
      // if a probability multiplier file is listed in .xml file
      if (m_useProbMultiplier)
         {
         if (m_pvtProbMultiplier)
            {
            m_probMultiplierPVTLookupKey.pvt = m_pvt;
            m_probMultiplierPVTLookupKey.region = m_region;
            m_probMultiplierPVTLookupKey.disturb = disturb;

            m_probMultiplier = &m_probMultiplierPVTMap[m_probMultiplierPVTLookupKey];
            }

         if (m_vegClassProbMultipier)
            {
            m_probMultiplierVegClassLookupKey.pvt = m_pvt;
            m_probMultiplierVegClassLookupKey.region = m_region;
            m_probMultiplierVegClassLookupKey.disturb = disturb;
            m_probMultiplierVegClassLookupKey.vegclass = m_vegClass;

            m_probMultiplier = &m_probMultiplierVegClassMap[m_probMultiplierVegClassLookupKey];
            }

         //if there is multiplier then multiply
         if (m_probMultiplier->size() != 0)
            it->second = it->second * m_probMultiplier->at(0);
         }

      probability_sum += it->second;
      }

   double rand_num = (double)m_rn.RandValue(0., 1.);  //randum number between 0 and 1, uniform distribution

   //normalize probability list if disturbance. this means a disturbance transition will be selected. 
   if (m_disturb != 0)
      {
      for (vector<pair<int, float>>::iterator it = m_final_probs->begin(); it != m_final_probs->end(); ++it)
         {
         it->second = probability_sum > 0.0 ? it->second / probability_sum : 1.0f / m_final_probs->size();
         }
      }

   //generate a lookup key from current variables in the IDU layer for the prob lookup table
   m_probLookupKey.from = m_vegClass;
   m_probLookupKey.regen = m_regen;
   m_probLookupKey.region = m_region;
   m_probLookupKey.disturb = disturb;

   //get the probability list again, original probs are necessary in case probs are normalizing.
   m_original_final_probs = &probmap2[m_probLookupKey];

   probability_sum = 0.0f;

   // resum necessary if probability list normalized. Second is the second of the vector pair or the probability
   for (vector<pair<int, float>>::iterator it = m_final_probs->begin(); it != m_final_probs->end(); ++it)
      {
      probability_sum += it->second;
      }

   //Begin Selecting probability transition based on VDDT algorithym  
   if (rand_num < probability_sum)
      {
      //randomly shuffle 
      //random_shuffle(m_final_probs->begin(), m_final_probs->end());

      // Use the VDDT algorithym to choose which probablity and stateclass is selected for transition
      m_selected_to_trans = ChooseProbTrans(rand_num, probability_sum, m_final_probs, m_original_final_probs);

      //if (disturb == 29)
      //   {
      //   CString msg;
      //   msg.Format("Prescribed Fire:  VegClass: %i, Region: %i, PVT: %i, Regen: %i", m_vegClass, m_region,  m_pvt, m_regen);
      //   Report::Log(msg);
      //   }
      }

   //  -99 in transition lookup table in the transfer "to" column. Also for each IDU m_trans initialized to -1.  only apply to delta array if valued is not -99 or -1
   // set flag.  send transition to delta array if TSD threshold is met. done in UpdateIDUVegInfo
   if (m_selected_to_trans >= 0)
      {
      m_flagProbabilisticTransition = true;
      }

   return m_flagProbabilisticTransition;
   }

int DynamicVeg::ChooseProbTrans(double rand_num, float probability_sum, vector<pair<int, float> >* m_final_probs, std::vector< std::pair<int, float> >* m_original_final_probs)
// Returns integer vegclass tranisitioned to.
   {
   double sum_of_trans_prob;
   int i = 0;
   int PTindex = 0;

   sum_of_trans_prob = 0.0;

   vector<pair<int, float>>::iterator ir = m_final_probs->begin();

   vector<pair<int, float>>::iterator orig_ir = m_original_final_probs->begin();

   //get first value of the second item in vector pair which is the probability 
   sum_of_trans_prob += ir->second;

   //Begin the selection of class transition based on VDDT algorithym
   while (rand_num > sum_of_trans_prob && PTindex < m_final_probs->size())
      {
      if (m_final_probs->size() > 1)
         {
         ++ir;
         ++orig_ir;
         PTindex++;

         if (PTindex >= 0)
            sum_of_trans_prob += ir->second;
         }
      else
         {
         sum_of_trans_prob = sum_of_trans_prob + rand_num;// there is only one probablility, so end this while loop
         }

      } //end while

   //m_selected_prob = ir->second;
   m_selected_prob = orig_ir->second;

   //returns the integer "to" class of the transition which is the first item in the vector pair. ir iterator at position determined in while loop 
   return (ir->first);

   }

static int errAgeClassCount = 0;

bool DynamicVeg::DeterministicTransition(EnvContext* pEnvContext, int idu)
   {
   MapLayer* pLayer = (MapLayer*)pEnvContext->pMapLayer;
   vector<int>* determmapindex = 0;

   CString msg;

   m_selected_to_trans = -1;

   int current_year = pEnvContext->currentYear;

   std::vector< std::pair<int, int> >* dtrans_value = 0;

   m_flagDeterministicTransition = false;

   //build the key for the lookup table
   m_determinIndexLookupKey.region = m_region;
   m_determinIndexLookupKey.from = m_vegClass;

   if (m_climateChange)
      {
      m_determinIndexLookupKey.pvt = m_mc1pvt;
      }
   else
      {
      m_determinIndexLookupKey.pvt = m_pvt;
      }

   determmapindex = &m_determinIndexMap[m_determinIndexLookupKey];
   int endage_col = m_deterministic_inputtable.GetCol("ENDAGE");

   if (determmapindex->size() > 1)
      {
      CString msg;
      msg.Format("Multiple entries for the same state in the deterministic transition table: vegclass_from=%i pvt=%i region=%i",
         m_vegClass, m_climateChange ? m_mc1pvt : m_pvt, m_region);
      Report::LogError(msg);
      }

   if (determmapindex->size() >= 1)
      {
      int endage;
      m_deterministic_inputtable.Get(endage_col, determmapindex->at(0), endage);
      if (m_ageClass >= endage)
         {
         if (m_ageClass > endage && errAgeClassCount < 10)
            {
            CString msg;
            msg.Format("m_ageClass > endage.  year=%i idu=%i, m_ageClass=%i, endage=%i, m_vegClass=%i, pvt=%i, m_region=%i m_disturb=%i",
               pEnvContext->currentYear, idu, m_ageClass, endage, m_vegClass, m_climateChange ? m_mc1pvt : m_pvt, m_region, m_disturb);
            Report::LogWarning(msg);

            errAgeClassCount++;
            if (errAgeClassCount == 10)
               {
               msg.Format("Further 'm_ageClass > endage' messages will be suppressed.");
               Report::LogWarning(msg);
               }
            }

         int vegclass_to, pvt_to;
         int vto_col = m_deterministic_inputtable.GetCol("VEGCLASSto");
         int pvtto_col = m_deterministic_inputtable.GetCol("PVTto");
         m_deterministic_inputtable.Get(vto_col, determmapindex->at(0), vegclass_to);
         m_deterministic_inputtable.Get(pvtto_col, determmapindex->at(0), pvt_to);
         m_selected_to_trans = vegclass_to;
         m_selected_pvtto_trans = pvt_to;
         }
      }

   if (m_selected_to_trans >= 0)
      {
      m_flagDeterministicTransition = true;
      }

   return m_flagDeterministicTransition;

   }

bool DynamicVeg::UpdateIDUVegInfo(EnvContext* pEnvContext, int idu, bool probabilistic_flag, bool deterministic_flag)
   {

   //NOTE:  This Class is called after a State Class Transition has been determinded.  It will use the new State Class, and the previsous one,
   // to go back into the probability and deterministic lookup up tables to retrieve ageclass, and Time since Disturbance (TSD)
   // data.  It will then up date the IDU layer's AGECLASS VEGCLASS, PVT, STANDAGE, LAI, CARBON, VEGTRANSTYPE, VARIANT, TSD


   int
      mc1sito10,
      minage,
      maxage,
      startage = 0,
      endage = 0,
      mintsd,
      relativeAge,
      keepRelativeAge,
      proportion,
      tsdMax,
      relativeTsd,
      new_tsd;
   CString msg;
   float lai;
   float carbon_table_value, carbon;

   int new_timeInAgeClass = 0;


   MapLayer* pLayer = (MapLayer*)pEnvContext->pMapLayer;

   vector<int>* probmapindex = 0;

   vector<int>* determmapindex = 0;

   if (!probabilistic_flag && !deterministic_flag && m_vegClass >= 2000000 && pEnvContext->id == 2) //no probabilistic or deterministic transition
      {
      new_timeInAgeClass = m_ageClass + 1;

      UpdateIDU(pEnvContext, idu, m_colAgeClass, new_timeInAgeClass, ADD_DELTA);

      //new_tsd = m_TSD + 1;

      //UpdateIDU( pEnvContext, idu, m_colTSD, new_tsd, ADD_DELTA );
      return true;
      }

   //ASSERT(m_selected_to_trans >= 0);

   //m_cursi in IDU layer is an integer, ie 114. Lookup table for site index is in multiples of ten
   //so, round to nearest 10. multiples of 5 round up.
   mc1sito10 = RoundToNearest10(m_mc1si);

   if (m_climateChange)
      {
      m_probIndexLookupKey.si = mc1sito10;
      m_probIndexLookupKey.pvt = m_mc1pvt;
      m_determinIndexInsertKey.pvt = m_mc1pvt; //the new transition is now the from. build lookup key to deterministic map
      }
   else
      {
      m_probIndexLookupKey.si = m_si;
      m_probIndexLookupKey.pvt = m_pvt;
      m_determinIndexLookupKey.pvt = m_pvt; //the new transition is now the from. build lookup key to determinsitic map
      }

   m_determinIndexLookupKey.region = m_region; //the new transition is now the from. build lookup key to determinsitic map
   m_determinIndexLookupKey.from = m_selected_to_trans; //spot in table where the to is now the from for new age class

   //generate a lookup key from current variables in the IDU layer for the prob lookup table
   int disturb = GetAdjustedDisturb(m_disturb);

   m_probIndexLookupKey.from = m_vegClass; //transition "from" veg class
   m_probIndexLookupKey.to = m_selected_to_trans;
   m_probIndexLookupKey.regen = m_regen;
   m_probIndexLookupKey.prob = m_selected_prob;
   m_probIndexLookupKey.region = m_region;
   m_probIndexLookupKey.disturb = disturb;

   if (/*m_selected_to_trans >= 2000000 &&*/ deterministic_flag)//deterministic transition, even if prob transition happened before.
      {
      if (m_vegClass != m_selected_to_trans && m_selected_to_trans != -99)
         UpdateIDU(pEnvContext, idu, m_colVegClass, m_selected_to_trans, ADD_DELTA);

      if (m_pvt != m_selected_pvtto_trans && m_selected_pvtto_trans != -99)
         UpdateIDU(pEnvContext, idu, m_colPVT, m_selected_pvtto_trans, ADD_DELTA);

      m_determinIndexLookupKey.pvt = m_selected_pvtto_trans;

      //getting index into deterministic file for the line that was transitioned to (now the from). for new age value
      determmapindex = &m_determinIndexMap[m_determinIndexLookupKey];

      //ASSERT(determmapindex != NULL);

      if (determmapindex == NULL)
         {
         bool alreadySeen = false;

         msg.Format("%i %i %i %i %i %i\n", m_selected_to_trans, m_disturb, m_regen, m_si, m_pvt, m_region);

         int list_length = (int)m_badDeterminVegTransClasses.GetSize();
         int max_list_length = 1000;
         if (0 < list_length && list_length < max_list_length)
            for (int i = 0; i < list_length; i++)
               {

               if (m_badDeterminVegTransClasses[i] == msg)
                  {
                  alreadySeen = true;
                  break;
                  }
               }

         if (!alreadySeen && list_length < max_list_length)
            m_badDeterminVegTransClasses.Add(msg);
         }

      int startage_col = m_deterministic_inputtable.GetCol("STARTAGE");

      if (determmapindex->size() == 0)
         {
         CString msg;
         msg.Format("No transitions (1) defined for deterministic lookup .csv file: vegclass=%i disturb=%i (%i) regen=%i si=%i pvt=%i region=%i",
            m_selected_to_trans, disturb, m_disturb, m_regen, m_si, m_pvt, m_region);
         Report::LogWarning(msg);
         }
      else
         {
         m_deterministic_inputtable.Get(startage_col, determmapindex->at(0), startage);
         }

      UpdateIDU(pEnvContext, idu, m_colAgeClass, startage, ADD_DELTA);// determinsitic transitions can transition to itself, resetting age class to beginning

      if (m_useLAI)
         {
         int lai_col = m_deterministic_inputtable.GetCol("LAI");

         m_deterministic_inputtable.Get(lai_col, determmapindex->at(0), lai);

         if (m_LAI != lai)
            UpdateIDU(pEnvContext, idu, m_colLAI, lai, ADD_DELTA);
         }

      if (m_useCarbon)
         {
         int carbon_col = m_deterministic_inputtable.GetCol("CARBON");

         m_deterministic_inputtable.Get(carbon_col, determmapindex->at(0), carbon_table_value);

         carbon = carbon_table_value * m_carbon_conversion_factor;

         if (m_carbon != carbon)
            UpdateIDU(pEnvContext, idu, m_colCarbon, carbon, ADD_DELTA);
         }

      // if flag set in .xml to send type of transition to IDU layer. 1 for Deterministic
      if (m_useVegTransFlag.useVegTransFlag == 1)
         UpdateIDU(pEnvContext, idu, m_colVegTranType, 1, ADD_DELTA);
      }

   if (m_selected_to_trans >= 1000000 && probabilistic_flag) //Probabilistic transition.  1000000 is the minimun VEGCLASS id number
      {

      probmapindex = &m_probIndexMap[m_probIndexLookupKey];//getting index into prob file for the line that transition was select from, for age info etc..   

      if (probmapindex->size() > 0) //probability transitions exist for this destination state class

         {//ASSERT(probmapindex->at(0) <= 5791);

         int minage_col = m_inputtable.GetCol("MINAGE");
         int maxage_col = m_inputtable.GetCol("MAXAGE");
         int tsd_col = m_inputtable.GetCol("TSD");
         int relage_col = m_inputtable.GetCol("RELATIVEAGE");
         int keeprel_col = m_inputtable.GetCol("KEEPRELAGE");
         int proport_col = m_inputtable.GetCol("PROPORTION");
         int tsdmax_col = m_inputtable.GetCol("TSDMAX");
         int reltsd_col = m_inputtable.GetCol("RELTSD");
         int pvtto_col = m_inputtable.GetCol("PVTto");

         m_inputtable.Get(minage_col, probmapindex->at(0), minage);
         m_inputtable.Get(maxage_col, probmapindex->at(0), maxage);
         m_inputtable.Get(tsd_col, probmapindex->at(0), mintsd);
         m_inputtable.Get(relage_col, probmapindex->at(0), relativeAge);
         m_inputtable.Get(keeprel_col, probmapindex->at(0), keepRelativeAge); // 1=true
         m_inputtable.Get(proport_col, probmapindex->at(0), proportion);
         m_inputtable.Get(tsdmax_col, probmapindex->at(0), tsdMax);
         m_inputtable.Get(reltsd_col, probmapindex->at(0), relativeTsd);
         m_inputtable.Get(pvtto_col, probmapindex->at(0), m_selected_pvtto_trans);

         m_determinIndexLookupKey.pvt = m_selected_pvtto_trans;

         determmapindex = &m_determinIndexMap[m_determinIndexLookupKey];

         ASSERT(determmapindex != NULL);

         if (determmapindex == NULL)
            {
            bool alreadySeen = false;

            msg.Format("%i %i %i %i %i %i\n", m_selected_to_trans, m_disturb, m_regen, m_si, m_pvt, m_region);

            int list_length = (int)m_badDeterminVegTransClasses.GetSize();
            int max_list_length = 1000;
            if (0 < list_length && list_length < max_list_length)
               for (int i = 0; i < list_length; i++)
                  {
                  if (m_badDeterminVegTransClasses[i] == msg)
                     {
                     alreadySeen = true;
                     break;
                     }
                  }

            if (!alreadySeen && list_length < max_list_length)
               m_badDeterminVegTransClasses.Add(msg);
            }

         // Gets the line in the look up tables for the proper transition so age and tsd can be set 
         int startage_col = m_deterministic_inputtable.GetCol("STARTAGE");
         int endage_col = m_deterministic_inputtable.GetCol("ENDAGE");

         if (determmapindex->size() == 0)
            {
            CString msg;
            msg.Format("No transitions (2) defined for deterministic lookup .csv file: vegclass=%i disturb=%i (%i) regen=%i si=%i pvt=%i region=%i",
               m_selected_to_trans, disturb, m_disturb, m_regen, m_si, m_pvt, m_region);
            Report::LogError(msg);
            }
         else
            {
            m_deterministic_inputtable.Get(startage_col, determmapindex->at(0), startage);
            m_deterministic_inputtable.Get(endage_col, determmapindex->at(0), endage);
            }
         }
      else //there are no probability transitions for this state class. set properties with some values from deteministic lookup table, others are defaults
         {
         determmapindex = &m_determinIndexMap[m_determinIndexLookupKey];

         ASSERT(determmapindex != NULL);

         if (determmapindex == NULL)
            {
            bool alreadySeen = false;

            msg.Format("%i %i %i %i %i %i\n", m_selected_to_trans, m_disturb, m_regen, m_si, m_pvt, m_region);

            int list_length = (int)m_badDeterminVegTransClasses.GetSize();
            int max_list_length = 1000;
            if (0 < list_length && list_length < max_list_length)
               for (int i = 0; i < list_length; i++)
                  {

                  if (m_badDeterminVegTransClasses[i] == msg)
                     {
                     alreadySeen = true;
                     break;
                     }
                  }

            if (!alreadySeen && list_length < max_list_length)
               m_badDeterminVegTransClasses.Add(msg);
            }

         int startage_col = m_deterministic_inputtable.GetCol("STARTAGE");
         int endage_col = m_deterministic_inputtable.GetCol("ENDAGE");

         if (determmapindex->size() == 0)
            {
            CString msg;
            msg.Format("No transitions (3) defined for deterministic lookup .csv file: vegclass=%i disturb=%i (%i) regen=%i si=%i pvt=%i region=%i",
               m_selected_to_trans, disturb, m_disturb, m_regen, m_si, m_pvt, m_region);
            Report::LogError(msg);
            }
         else
            {
            m_deterministic_inputtable.Get(startage_col, determmapindex->at(0), startage);
            m_deterministic_inputtable.Get(endage_col, determmapindex->at(0), endage);
            }

         mintsd = 0;
         relativeAge = 0;
         keepRelativeAge = 0; //false
         proportion = 1;
         tsdMax = 9999;
         relativeTsd = 0;
         }

      if (m_disturb > 0) //since disturbances > zero happen outside of ILAP models, they will happen regardless of Time Since Disturbance (TSD)
         {
         if (probmapindex->size() > 0)
            {
            if (m_useVariant)
               {
               int variant = 1;

               int variant_col = m_inputtable.GetCol("VARIANT");

               m_inputtable.Get(variant_col, probmapindex->at(0), variant);  // blows up here when probmaxindex length=0

               UpdateIDU(pEnvContext, idu, m_colVariant, variant, ADD_DELTA);
               }
            }

         if ((m_selected_to_trans != m_vegClass) || (m_selected_pvtto_trans != m_pvt))
            {
            if (determmapindex->size() > 0)
               {
               if (m_useLAI)
                  {
                  int lai_col = m_deterministic_inputtable.GetCol("LAI");

                  m_deterministic_inputtable.Get(lai_col, determmapindex->at(0), lai);

                  if (m_LAI != lai)
                     UpdateIDU(pEnvContext, idu, m_colLAI, lai, ADD_DELTA);
                  }

               if (m_useCarbon)
                  {
                  int carbon_col = m_deterministic_inputtable.GetCol("CARBON");

                  m_deterministic_inputtable.Get(carbon_col, determmapindex->at(0), carbon_table_value);

                  carbon = carbon_table_value * m_carbon_conversion_factor;

                  if (m_carbon != carbon)
                     UpdateIDU(pEnvContext, idu, m_colCarbon, carbon, ADD_DELTA);
                  }
               }

            // if flag set in .xml to send type of transition to IDU layer. 3 for disturbance transition
            if (m_useVegTransFlag.useVegTransFlag == 1)
               UpdateIDU(pEnvContext, idu, m_colVegTranType, 3, ADD_DELTA);

            if (keepRelativeAge != 1) //do not keep relative age
               {

               UpdateIDU(pEnvContext, idu, m_colVegClass, m_selected_to_trans, ADD_DELTA);

               if (m_pvt != m_selected_pvtto_trans && m_selected_pvtto_trans != -99)
                  UpdateIDU(pEnvContext, idu, m_colPVT, m_selected_pvtto_trans, ADD_DELTA);

               new_timeInAgeClass = startage + relativeAge; //relative age can be negative, holding a VEGCLASS longer in said class with respect to deterministic transitions.   

               if (new_timeInAgeClass > endage) new_timeInAgeClass = endage;

               if (new_timeInAgeClass < startage) new_timeInAgeClass = startage;

               UpdateIDU(pEnvContext, idu, m_colAgeClass, new_timeInAgeClass, ADD_DELTA);
               }
            else // keep relative age
               {
               new_timeInAgeClass = m_ageClass + relativeAge;

               if (new_timeInAgeClass > endage) new_timeInAgeClass = endage;

               if (new_timeInAgeClass < startage) new_timeInAgeClass = startage;

               UpdateIDU(pEnvContext, idu, m_colVegClass, m_selected_to_trans, ADD_DELTA);

               if (m_pvt != m_selected_pvtto_trans && m_selected_pvtto_trans != -99)
                  UpdateIDU(pEnvContext, idu, m_colPVT, m_selected_pvtto_trans, ADD_DELTA);

               UpdateIDU(pEnvContext, idu, m_colAgeClass, new_timeInAgeClass, ADD_DELTA); //only UpdateIDU if change occurs
               }
            }
         else //no change in veg class, keep relative age has no affect
            {
            new_timeInAgeClass = m_ageClass + relativeAge;

            if (new_timeInAgeClass > endage) new_timeInAgeClass = endage;

            if (new_timeInAgeClass < startage) new_timeInAgeClass = startage;

            UpdateIDU(pEnvContext, idu, m_colAgeClass, new_timeInAgeClass, ADD_DELTA);

            // if flag set in .xml to send type of transition to IDU layer. 3 for disturbance transition
            if (m_useVegTransFlag.useVegTransFlag == 1)
               UpdateIDU(pEnvContext, idu, m_colVegTranType, 3, ADD_DELTA);
            }
         }
      else //for all other probabilities with disturb 0 (successional), then depend on a minimum tsd, minAgeclass and maxAgeclass
         {
         if (m_TSD >= mintsd && m_ageClass >= minage && m_ageClass <= maxage) //can only happen if TSD is greater than min tsd for ILAP model,same with minag and maxage
            {
            if (m_selected_to_trans != m_vegClass)
               {
               if (m_useLAI && determmapindex->size() > 0)
                  {
                  int lai_col = m_deterministic_inputtable.GetCol("LAI");

                  m_deterministic_inputtable.Get(lai_col, determmapindex->at(0), lai);

                  if (m_LAI != lai)
                     UpdateIDU(pEnvContext, idu, m_colLAI, lai, ADD_DELTA);
                  }

               // if flag set in .xml to send type of transition to IDU layer. 2 for successional transition
               if (m_useVegTransFlag.useVegTransFlag == 1)
                  UpdateIDU(pEnvContext, idu, m_colVegTranType, 2, ADD_DELTA);

               if (keepRelativeAge != 1) //do not keep relative age
                  {

                  new_timeInAgeClass = startage + relativeAge; //relative age can be negative, holding a VEGCLASS longer in said class with respect to deterministic transitions.   

                  if (new_timeInAgeClass > endage) new_timeInAgeClass = endage;

                  if (new_timeInAgeClass < startage) new_timeInAgeClass = startage;

                  UpdateIDU(pEnvContext, idu, m_colVegClass, m_selected_to_trans, ADD_DELTA);

                  if (m_pvt != m_selected_pvtto_trans && m_selected_pvtto_trans != -99)
                     UpdateIDU(pEnvContext, idu, m_colPVT, m_selected_pvtto_trans, ADD_DELTA);

                  UpdateIDU(pEnvContext, idu, m_colAgeClass, new_timeInAgeClass, ADD_DELTA);
                  }
               else // keep relative age
                  {
                  new_timeInAgeClass = m_ageClass + relativeAge;

                  if (new_timeInAgeClass > endage) new_timeInAgeClass = endage;

                  if (new_timeInAgeClass < startage) new_timeInAgeClass = startage;

                  UpdateIDU(pEnvContext, idu, m_colVegClass, m_selected_to_trans, ADD_DELTA);

                  if (m_pvt != m_selected_pvtto_trans && m_selected_pvtto_trans != -99)
                     UpdateIDU(pEnvContext, idu, m_colPVT, m_selected_pvtto_trans, ADD_DELTA);

                  UpdateIDU(pEnvContext, idu, m_colAgeClass, new_timeInAgeClass, ADD_DELTA);
                  }
               }
            else //no change in veg class, keep relative age has no affect
               {
               new_timeInAgeClass = m_ageClass + relativeAge;

               if (new_timeInAgeClass > endage) new_timeInAgeClass = endage;

               if (new_timeInAgeClass < startage) new_timeInAgeClass = startage;

               UpdateIDU(pEnvContext, idu, m_colAgeClass, new_timeInAgeClass, ADD_DELTA); //only UpdateIDU if change occurs   

               // if flag set in .xml to send type of transition to IDU layer. 2 for successional transition
               if (m_useVegTransFlag.useVegTransFlag == 1)
                  UpdateIDU(pEnvContext, idu, m_colVegTranType, 2, ADD_DELTA);
               }

            }
         else // m_TSD < mintsd
            {
            new_timeInAgeClass = m_ageClass + 1;

            UpdateIDU(pEnvContext, idu, m_colAgeClass, new_timeInAgeClass, ADD_DELTA);
            }
         }

      if (relativeTsd == -9999) //if relative tsd is -9999 then set new tsd to 0
         {
         //new_tsd = 0;
         //if(new_tsd != m_TSD) UpdateIDU( pEnvContext, idu, m_colTSD, new_tsd, ADD_DELTA );
         }
      else
         {
         new_tsd = m_TSD + relativeTsd;

         if (new_tsd != m_TSD) UpdateIDU(pEnvContext, idu, m_colTSD, new_tsd, ADD_DELTA);
         }

      }   //end if probabilistic      

   return (true);

   }

bool DynamicVeg::LoadProbMultiplierCSV(CString probMultiplierFilename, EnvContext* pEnvContext)
   {
   int records = m_probMultiplier_inputtable.ReadAscii(probMultiplierFilename, ',', TRUE);

   float probMultTemp;

   if (records <= 0 || probMultiplierFilename == "")
      {
      CString msg;
      msg.Format("LoadProbMultiplierCSV could not load probability multiplier .csv file specified in PVT.xml file");
      Report::WarningMsg(msg);
      return false;
      }

   //get column numbers in prob input table
   int region_col = m_probMultiplier_inputtable.GetCol("REGION");
   int pvt_col = m_probMultiplier_inputtable.GetCol("PVT");
   int disturb_col = m_probMultiplier_inputtable.GetCol("DISTURB");
   int timestep_col = m_probMultiplier_inputtable.GetCol("Timestep");
   int probmult_col = m_probMultiplier_inputtable.GetCol("ProbMultiplier");
   int colFutsi = m_probMultiplier_inputtable.GetCol("FUTSI");
   int vegclass_col = m_probMultiplier_inputtable.GetCol("VEGCLASS");

   if (region_col < 0 || pvt_col < 0 || disturb_col < 0 ||
      timestep_col < 0 || probmult_col < 0 || colFutsi < 0)
      {
      CString msg;
      msg.Format("LoadProbMultiplierCSV One or more column headings are incorrect in Probability lookup file");
      Report::ErrorMsg(msg);
      return false;
      }

   int tabrows = m_probMultiplier_inputtable.GetRowCount();

   // loop through look up table  
   for (int j = 0; j < tabrows; j++)
      {
      //Building key for probabilities
      if (vegclass_col == -1)
         {
         m_probMultiplier_inputtable.Get(pvt_col, j, m_probMultiplierPVTInsertKey.pvt);
         m_probMultiplier_inputtable.Get(timestep_col, j, m_probMultiplierPVTInsertKey.timestep);
         m_probMultiplier_inputtable.Get(disturb_col, j, m_probMultiplierPVTInsertKey.disturb);
         m_probMultiplier_inputtable.Get(region_col, j, m_probMultiplierPVTInsertKey.region);
         m_probMultiplier_inputtable.Get(colFutsi, j, m_probMultiplierPVTInsertKey.si);
         m_probMultiplier_inputtable.Get(probmult_col, j, probMultTemp);

         //Building map 
         m_probMultiplierPVTMap[m_probMultiplierPVTInsertKey].push_back(probMultTemp);

         m_pvtProbMultiplier = true;

         }
      else
         {
         m_probMultiplier_inputtable.Get(pvt_col, j, m_probMultiplierVegClassInsertKey.vegclass);
         m_probMultiplier_inputtable.Get(pvt_col, j, m_probMultiplierVegClassInsertKey.pvt);
         m_probMultiplier_inputtable.Get(timestep_col, j, m_probMultiplierVegClassInsertKey.timestep);
         m_probMultiplier_inputtable.Get(disturb_col, j, m_probMultiplierVegClassInsertKey.disturb);
         m_probMultiplier_inputtable.Get(region_col, j, m_probMultiplierVegClassInsertKey.region);
         m_probMultiplier_inputtable.Get(colFutsi, j, m_probMultiplierPVTInsertKey.si);
         m_probMultiplier_inputtable.Get(probmult_col, j, probMultTemp);

         //Building map 
         m_probMultiplierVegClassMap[m_probMultiplierVegClassInsertKey].push_back(probMultTemp);

         m_vegClassProbMultipier = true;

         }
      }

   return true;

   }

bool DynamicVeg::LoadDeterministicTransCSV(CString deterministicfilename, EnvContext* pEnvContext)
   {

   const MapLayer* pLayer = pEnvContext->pMapLayer;

   CString msg;
   int rndStandAge;
   int startStandAge;
   int endStandAge;
   int tabvegclass;
   int tabpvt;
   int tabregion;
   float tablai;
   float tabcarbon;
   int totemp;
   int pvttotemp;

   int records = m_deterministic_inputtable.ReadAscii(deterministicfilename, ',', TRUE);

   if (records <= 0 || deterministicfilename == "")
      {
      msg.Format("LoadDeterministicTransCSV could not load deterministic transition .csv file specified in PVT.xml file");
      Report::ErrorMsg(msg);
      return false;
      }

   //get column numbers in prob input table
   int region_col = m_deterministic_inputtable.GetCol("REGION");
   int vfrom_col = m_deterministic_inputtable.GetCol("VEGCLASSfrom");
   int vto_col = m_deterministic_inputtable.GetCol("VEGCLASSto");
   int abbvfrom_col = m_deterministic_inputtable.GetCol("ABBREVfrom");
   int abbvto_col = m_deterministic_inputtable.GetCol("ABBREVto");
   int pvt_col = m_deterministic_inputtable.GetCol("PVT");
   int pvtto_col = m_deterministic_inputtable.GetCol("PVTto");
   int startage_col = m_deterministic_inputtable.GetCol("STARTAGE");
   int endage_col = m_deterministic_inputtable.GetCol("ENDAGE");
   int rndage_col = m_deterministic_inputtable.GetCol("RNDAGE");

   if (vfrom_col < 0 || vto_col < 0 || abbvfrom_col < 0 || abbvto_col < 0 || pvtto_col < 0 ||
      pvt_col < 0 || startage_col < 0 || endage_col < 0 || rndage_col < 0 || region_col < 0)
      {
      msg.Format("DynamicVeg::LoadDeterministicTransCSV One or more column headings are incorrect in deterministic lookup file");
      Report::ErrorMsg(msg);
      return false;
      }

   int tabrows = m_deterministic_inputtable.GetRowCount();

   // loop through look up table  
   for (int j = 0; j < tabrows; j++)
      {

      //Build key
      m_deterministic_inputtable.Get(vfrom_col, j, m_determinIndexInsertKey.from);
      m_deterministic_inputtable.Get(pvt_col, j, m_determinIndexInsertKey.pvt);
      m_deterministic_inputtable.Get(region_col, j, m_determinIndexInsertKey.region);

      //build map, this map is used once a stateclass transition has happened. j will be the index into this map
      //so values used in Age Class determination can be used.
      m_determinIndexMap[m_determinIndexInsertKey].push_back(j);

      //Building key
      m_deterministic_inputtable.Get(vfrom_col, j, m_deterministicInsertKey.from);
      m_deterministic_inputtable.Get(vto_col, j, totemp);
      m_deterministic_inputtable.Get(pvtto_col, j, pvttotemp);
      m_deterministic_inputtable.Get(region_col, j, m_deterministicInsertKey.region);
      m_deterministic_inputtable.Get(pvt_col, j, m_deterministicInsertKey.pvt);
      m_deterministic_inputtable.Get(endage_col, j, m_deterministicInsertKey.endage);

      // Building map   
      m_deterministic_trans[m_deterministicInsertKey].push_back(std::make_pair(totemp, pvttotemp));

      }

   int lai_col = -1;

   if (m_useLAI)
      {
      lai_col = m_deterministic_inputtable.GetCol("LAI");

      if (lai_col < 0)
         {
         CString msg;
         msg.Format("Init LAI column heading is incorrect in deterministic lookup file");
         Report::ErrorMsg(msg);

         m_useLAI = false;
         return false;
         }
      }

   int carbon_col = -1;

   if (m_useCarbon)
      {
      carbon_col = m_deterministic_inputtable.GetCol("CARBON");

      if (carbon_col < 0)
         {
         CString msg;
         msg.Format("Init CARBON column heading is incorrect in deterministic lookup file");
         Report::ErrorMsg(msg);

         m_useCarbon = false;
         return false;
         }
      }

   int tsd_col = m_inputtable.GetCol("TSD");
   int minAge_col = m_inputtable.GetCol("MINAGE");
   int maxAge_col = m_inputtable.GetCol("MAXAGE");

   //if necessary intialize attributes in IDU layer
   for (MapLayer::Iterator idu = pEnvContext->pMapLayer->Begin(); idu != pEnvContext->pMapLayer->End(); idu++)
      {
      int rndTSD = 0;
      int meanTSD = 0;
      int meanMinAge = 0;
      int meanMaxAge = 0;
      int n = 0;
      int sumTSD = 0;
      int sumMinAge = 0;
      int sumMaxAge = 0;

      vector<int>* tsdIndex = 0;

      pLayer->GetData(idu, m_colRegion, m_region);
      pLayer->GetData(idu, m_colPVT, m_pvt);
      pLayer->GetData(idu, m_colVegClass, m_vegClass);
      pLayer->GetData(idu, m_colTSD, m_TSD);
      pLayer->GetData(idu, m_colDisturb, m_disturb);

      // the value for DISTURB in the IDU layer may be a negative value (historic)
      // this value is used as part of key into the probability lookup table. For
      // successional tranitions the value should be zero, so set equal to zero for the key
      if (m_disturb < 0) m_disturb = 0;

      // loop through look up table  
      for (int j = 0; j < tabrows; j++)
         {

         double rand_num = (double)m_rn.RandValue(0., 1.);

         m_deterministic_inputtable.Get(rndage_col, j, rndStandAge);
         m_deterministic_inputtable.Get(startage_col, j, startStandAge);
         m_deterministic_inputtable.Get(endage_col, j, endStandAge);
         m_deterministic_inputtable.Get(vfrom_col, j, tabvegclass);
         m_deterministic_inputtable.Get(pvt_col, j, tabpvt);
         m_deterministic_inputtable.Get(region_col, j, tabregion);
         if (m_useLAI) m_deterministic_inputtable.Get(lai_col, j, tablai);
         if (m_useCarbon) m_deterministic_inputtable.Get(carbon_col, j, tabcarbon);

         if (m_vegClass == tabvegclass && m_pvt == tabpvt && m_region == tabregion)
            {

            rndStandAge = (int)floor(startStandAge + ((endStandAge - startStandAge) * (rand_num)));

            //if specified by switch in the input .xml file, use one of these methods to intialize TSD in the IDU layer
            switch (m_initializer.initTSD)
               {

               case 1:

                  rndTSD = (int)floor(startStandAge + ((endStandAge - startStandAge) * (rand_num))) - startStandAge;

                  break;

               case 2:

                  if (m_TSD >= startStandAge && m_TSD <= endStandAge)
                     continue;

                  rndTSD = (int)floor(startStandAge + ((endStandAge - startStandAge) * (rand_num))) - startStandAge;

                  break;

               case 3:

                  //just for proper lookup key, these keys are for future use
                  m_TSDIndexLookupKey.si = 0;

                  //generate a lookup key from current variables in the IDU layer for the prob lookup table
                  m_TSDIndexLookupKey.from = m_vegClass;
                  m_TSDIndexLookupKey.regen = m_regen;
                  m_TSDIndexLookupKey.region = m_region;
                  m_TSDIndexLookupKey.pvt = m_pvt;
                  m_TSDIndexLookupKey.disturb = m_disturb; // should always be zero in Init.

                  tsdIndex = &m_TSDIndexMap[m_TSDIndexLookupKey];

                  n = (int)tsdIndex->size();

                  if (n > 0)
                     {
                     int initVal = tsdIndex->at(0);
                     int uniqVals = 0;
                     for (int i = 0; i < n; i++)
                        {
                        sumTSD += m_inputtable.GetAsInt(tsd_col, tsdIndex->at(i));
                        sumMinAge += m_inputtable.GetAsInt(minAge_col, tsdIndex->at(i));
                        sumMaxAge += m_inputtable.GetAsInt(maxAge_col, tsdIndex->at(i));

                        // just need to see if at least one is not the same
                        if (initVal != tsdIndex->at(i))
                           uniqVals++;
                        }

                     (sumTSD > 0) ? meanTSD = (int)floor(sumTSD / n) : meanTSD = 0;
                     (sumMinAge > 0) ? meanMinAge = (int)floor(sumMinAge / n) : meanMinAge = 0;
                     (sumMaxAge > 0) ? meanMaxAge = (int)floor(sumMaxAge / n) : meanMaxAge = 0;

                     rndTSD = (int)floor((double)m_rn.RandValue(0., (double)meanTSD));

                     }
                  else // no  prob successional transition found, use initial IDU layer value
                     {
                     rndTSD = m_TSD;
                     }

                  break;

               default:

                  break;
               }

            if (rndTSD < 0)
               rndTSD = 0;

            // In the .xml file, if user wants DynamicVeg to initialize TSD
            if (m_initializer.initTSD > 0)
               ((MapLayer*)(pEnvContext->pMapLayer))->SetData(idu, m_colTSD, rndTSD);

            // In the .xml file, if user wants DynamicVeg to initialize AGECLASS
            if (m_initializer.initAgeClass > 0)
               ((MapLayer*)(pEnvContext->pMapLayer))->SetData(idu, m_colAgeClass, rndStandAge);

            if (m_useLAI)
               ((MapLayer*)(pEnvContext->pMapLayer))->SetData(idu, m_colLAI, tablai);

            if (m_useCarbon)
               ((MapLayer*)(pEnvContext->pMapLayer))->SetData(idu, m_colCarbon, tabcarbon * m_carbon_conversion_factor);

            break;
            }
         }
      }

   return true;

   }

bool DynamicVeg::LoadProbCSV(CString probfilename, EnvContext* pEnvContext)
   {
   const MapLayer* pLayer = pEnvContext->pMapLayer;

   CString msg;
   int totemp;
   float probtemp;
   //   int disturb;
   //   int tabvegclass;
   //   int tabpvt;
   //   int tabregion;
   //   int minTSD;
   //   int maxTSD;
   //   int minAgeClass;

      //read in the the data in the .csv lookup table. index zero is MC1 probability lookup table
   int records = m_inputtable.ReadAscii(probfilename, ',', TRUE);

   if (records <= 0)
      {
      msg.Format("Could not load probability .csv file specified in PVT.xml file");
      Report::ErrorMsg(msg);
      return false;
      }

   msg.Format("Loaded %i records from '%s'", records, (LPCTSTR)probfilename);
   Report::Log(msg);

   //get column numbers in prob input table
   int vfrom_col = m_inputtable.GetCol("VEGCLASSfrom");
   int vto_col = m_inputtable.GetCol("VEGCLASSto");
   int abbvfrom_col = m_inputtable.GetCol("ABBREVfrom");
   int abbvto_col = m_inputtable.GetCol("ABBREVto");
   int futsi_col = m_inputtable.GetCol("FUTSI");
   int pvt_col = m_inputtable.GetCol("PVT");
   int regen_col = m_inputtable.GetCol("REGEN");
   int p_col = m_inputtable.GetCol("P");
   int propor_col = m_inputtable.GetCol("pxPropor");
   int region_col = m_inputtable.GetCol("REGION");
   int disturb_col = m_inputtable.GetCol("DISTURB");
   //int probtype_col = m_inputtable.GetCol("VDDTProbType");
   int minage_col = m_inputtable.GetCol("MINAGE");
   int maxage_col = m_inputtable.GetCol("MAXAGE");
   int tsd_col = m_inputtable.GetCol("TSD");
   int relage_col = m_inputtable.GetCol("RELATIVEAGE");
   int keeprel_col = m_inputtable.GetCol("KEEPRELAGE");
   int proport_col = m_inputtable.GetCol("PROPORTION");
   int tsdmax_col = m_inputtable.GetCol("TSDMAX");
   int reltsd_col = m_inputtable.GetCol("RELTSD");
   int rndage_col = m_inputtable.GetCol("RNDAGE");

   if (vfrom_col < 0 || vto_col < 0 ||
      pvt_col < 0 || regen_col < 0 || p_col < 0 ||
      propor_col < 0 || region_col < 0 || disturb_col < 0 ||
      minage_col < 0 || maxage_col < 0 || tsd_col < 0 || relage_col < 0 ||
      keeprel_col < 0 || proport_col < 0 || tsdmax_col < 0 || reltsd_col < 0)
      {
      msg.Format("LoadProbCSV One or more column headings are incorrect in Probability lookup file");
      Report::ErrorMsg(msg);
      return false;
      }

   int variant_col = m_inputtable.GetCol("VARIANT");

   if (variant_col >= 0)
      m_useVariant = true;

   int tabrows = m_inputtable.GetRowCount();

   Report::Log("Iterating through input table");

   // NOTE: HARD CODED VALUES - REPLACE
   std::vector<int> pvts = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,22,99 };
   std::vector<int> rgns = { 7,8,9,11,17,18,19,99 };
   //std::vector<int> vars = { 1,2,3,4,5,6 };
   std::vector<int> regens = { 0, 1, 2 };
   int countAdded = 0;

   // loop through look up table  , building the following maps:
   // probMap(2)[] - key=probInsertKey:(vegFrom,futsi,pvt,regen,region,disturb), value=pair(vegTo,prob)
   // probIndexMap[] - key=probInsertKey:(vegFrom,vegTo,futsi,pvt,regen,region,prob,disturb), value=row in m_inputTable
   for (int j = 0; j < tabrows; j++)
      {
      //Building key for probabilities

      int pvt = -1, regen = -1, region = -1;
      m_inputtable.Get(vfrom_col, j, m_probInsertKey.from);
      m_inputtable.Get(futsi_col, j, m_probInsertKey.si);
      m_inputtable.Get(pvt_col, j, pvt);  // these are wildcardable, handled below
      m_inputtable.Get(regen_col, j, regen);
      m_inputtable.Get(region_col, j, region);
      m_inputtable.Get(disturb_col, j, m_probInsertKey.disturb);

      m_inputtable.Get(vto_col, j, totemp);
      m_inputtable.Get(p_col, j, probtemp);

      // handle wildcards
      for (int _pvt : pvts)
         {
         for (int _regen : regens)
            {
            for (int _region : rgns)
               {
               // insert new entry in map if we match a valid value OR a wildcard (-99) value
               if ((pvt == _pvt || pvt == -99)   // valid pvt OR wildcard
                  && (region == _region || region == -99)  // valid region OR wildcard
                  && (regen == _regen || regen == -99))    // valid regen ORr wildcard
                  {
                  // Build map
                  m_probInsertKey.pvt = _pvt;
                  m_probInsertKey.region = _region;
                  m_probInsertKey.regen = _regen;

                  probmap[m_probInsertKey].push_back(std::make_pair(totemp, probtemp));
                  probmap2[m_probInsertKey].push_back(std::make_pair(totemp, probtemp));
                  countAdded++;
                  }
               }
            }
         }


      //Building map     
      //probmap[m_probInsertKey].push_back(std::make_pair(totemp, probtemp));
      //probmap2[m_probInsertKey].push_back(std::make_pair(totemp, probtemp));

      //Building key for index into map
      m_inputtable.Get(vfrom_col, j, m_probIndexInsertKey.from);
      m_inputtable.Get(vto_col, j, m_probIndexInsertKey.to);
      m_inputtable.Get(futsi_col, j, m_probIndexInsertKey.si);
      m_inputtable.Get(pvt_col, j, pvt);
      m_inputtable.Get(regen_col, j, regen);
      m_inputtable.Get(region_col, j, region);
      m_inputtable.Get(p_col, j, m_probIndexInsertKey.prob);
      m_inputtable.Get(disturb_col, j, m_probIndexInsertKey.disturb);

      // handle wildcards
      for (int _pvt : pvts)
         {
         for (int _regen : regens)
            {
            for (int _region : rgns)
               {
               // insert new entry in map if we match a valid value OR a wildcard (-99) value
               if ((pvt == _pvt || pvt == -99)   // valid pvt OR wildcard
                  && (region == _region || region == -99)  // valid region OR wildcard
                  && (regen == _regen || regen == -99))    // valid regen ORr wildcard
                  {
                  // Build map
                  m_probIndexInsertKey.pvt = _pvt;
                  m_probIndexInsertKey.region = _region;
                  m_probIndexInsertKey.regen = _regen;

                  //Building map, this map is used once a stateclass transition has happened. j will be the index into this map
                  //so values used in Age Class determination can be used.
                  m_probIndexMap[m_probIndexInsertKey].push_back(j);
                  }
               }
            }
         }

      //Building map, this map is used once a stateclass transition has happened. j will be the index into this map
      //so values used in Age Class determination can be used.
      //m_probIndexMap[m_probIndexInsertKey].push_back(j);

      //Building key for TSDindex into map
      m_inputtable.Get(vfrom_col, j, m_TSDIndexInsertKey.from);
      m_inputtable.Get(region_col, j, region);
      m_inputtable.Get(pvt_col, j, pvt);
      m_inputtable.Get(disturb_col, j, m_TSDIndexInsertKey.disturb);
      m_inputtable.Get(futsi_col, j, m_TSDIndexInsertKey.si);
      m_inputtable.Get(regen_col, j, regen);

      // handle wildcards
      for (int _pvt : pvts)
         {
         for (int _regen : regens)
            {
            for (int _region : rgns)
               {
               // insert new entry in map if we match a valid value OR a wildcard (-99) value
               if ((pvt == _pvt || pvt == -99)   // valid pvt OR wildcard
                  && (region == _region || region == -99)  // valid region OR wildcard
                  && (regen == _regen || regen == -99))    // valid regen ORr wildcard
                  {
                  // Build map
                  m_TSDIndexInsertKey.pvt = _pvt;
                  m_TSDIndexInsertKey.region = _region;
                  m_TSDIndexInsertKey.regen = _regen;

                  // used to initialize TSD in IDU layer with TSD threshold in Probability lookup table
                  m_TSDIndexMap[m_TSDIndexInsertKey].push_back(j);
                  }
               }
            }
         }


      // used to initialize TSD in IDU layer with TSD threshold in Probability lookup table
      //m_TSDIndexMap[m_TSDIndexInsertKey].push_back(j);

      //FUTSI, PVT, REGEN, REGION xxx
      }

   // if necessary intialize attributes in IDU layer
   //for ( MapLayer::Iterator idu = pEnvContext->pMapLayer->Begin( ); idu != pEnvContext->pMapLayer->End(); idu++ )
   //   {      
   //   pLayer->GetData( idu, m_colRegion, m_region );
   //   pLayer->GetData( idu, m_colPVT, m_pvt );
   //   pLayer->GetData( idu, m_colVegClass, m_vegClass );
   //   pLayer->GetData( idu, m_colDisturb, m_disturb );
   //      
   //   // loop through look up table  
   //   for (int j = 0; j < tabrows; j++)
   //      {
   //      double rand_num = (double) m_rn.RandValue(0. ,1. );  
   //        
   //      m_inputtable.Get(disturb_col, j,disturb);
   //      m_inputtable.Get(vfrom_col,   j,tabvegclass);
   //      m_inputtable.Get(pvt_col,     j,tabpvt);
   //      m_inputtable.Get(tsd_col,     j,minTSD);
   //      m_inputtable.Get(minage_col,  j,minAgeClass);
   //      m_inputtable.Get(tsdmax_col,  j,maxTSD);
   //      m_inputtable.Get(region_col,  j,tabregion);
   //      }
   //   }

   return true;

   }

bool DynamicVeg::LoadMC1PvtSiCSV(CString MC1Sifilename, CString MC1Pvtfilename, EnvContext* pEnvContext)
   {

   CString msg;
   int pvt_temp;
   float si_temp;

   m_vpvt.clear(); //clear these vectors and start all over, size 0
   m_vsi.clear();

#define rownum 40  //MC1 rows
#define colnum 76  //MC1 columns
#define yrsnum 93  //number of years 

   //read in the the data in the .csv MC1 site index and pvt files. 2d ascii file 76 columns, 3720 rows (40 rows x 93 years)
   int SIrecords = m_MC1_siteindex.ReadAscii(MC1Sifilename, ',', TRUE);

   if (SIrecords <= 0)
      {
      CString msg(_T("Error reading site index file "));
      msg += MC1Sifilename;
      Report::ErrorMsg(msg);
      return false;
      }

   int PVTrecords = m_MC1_pvt.ReadAscii(MC1Pvtfilename, ',', TRUE);

   if (PVTrecords <= 0)
      {
      CString msg(_T("Error reading pvt file "));
      msg += MC1Pvtfilename;
      Report::ErrorMsg(msg);
      return false;
      }

   int datrow = 0;

   // read 2d ascii file into 3d vector
   for (int y = 0; y < yrsnum; y++)
      {
      m_vpvt.push_back(vector<vector<int> >());

      m_vsi.push_back(vector<vector<float> >());

      for (int r = 0; r < rownum; r++)
         {

         m_vpvt.at(y).push_back(vector<int>());

         m_vsi.at(y).push_back(vector<float>());

         for (int c = 0; c < colnum; c++)
            {

            m_MC1_pvt.Get(c, datrow, pvt_temp);

            m_MC1_siteindex.Get(c, datrow, si_temp);

            m_vpvt.at(y).at(r).push_back(pvt_temp);

            m_vsi.at(y).at(r).push_back(si_temp);

            } // end column loop

         datrow++;

         } //end row loop
      } // end year loop

   return TRUE;

   }

int DynamicVeg::RoundToNearest10(float fNumberToRound)
   {
   int iToNearest = 10;
   int iNearest = 0;
   bool bIsUpper = false;

   int iNumberToRound = int(fNumberToRound + 0.5); //round to nearest integer

   int iRest = iNumberToRound % iToNearest;
   if (iNumberToRound == 550) bIsUpper = true;

   if (bIsUpper == true)
      {
      iNearest = (iNumberToRound - iRest) + iToNearest;
      return iNearest;
      }
   else if (iRest >= (iToNearest / 2))
      {
      iNearest = (iNumberToRound - iRest) + iToNearest;
      return iNearest;
      }
   else if (iRest < (iToNearest / 2))
      {
      iNearest = (iNumberToRound - iRest);
      return iNearest;
      }

   return 0;
   }

bool DynamicVeg::LoadXml(LPCTSTR _filename)
   {
   CString filename;
   if (PathManager::FindPath(_filename, filename) < 0) //  return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 
      {
      CString msg;
      msg.Format("Input file '%s' not found - this process will be disabled", _filename);
      Report::ErrorMsg(msg);
      return false;
      }

   // start parsing input file
   TiXmlDocument doc;
   bool ok = doc.LoadFile(filename);

   bool loadSuccess = true;

   if (!ok)
      {
      CString msg;
      msg.Format("Error reading input file %s:  %s", filename, doc.ErrorDesc());
      Report::ErrorMsg(msg);
      return false;
      }

   // start interating through the nodes
   TiXmlElement* pXmlRoot = doc.RootElement();  // <integrator

   XML_ATTR setAttrpvt[] = {
         { "dynamic_update", TYPE_INT, &(m_dynamic_update.dynamic_update), true, 0 },
         { NULL, TYPE_NULL, NULL, false, 0 } };

   ok = TiXmlGetAttributes(pXmlRoot, setAttrpvt, filename, NULL);
   //TiXmlElement *pXmlDynamic_updates = pXmlRoot->Attribute("dynamic_update");

   // get mc1_outputs next  ************************************************************************************
   //TiXmlElement *pXmlMC1_outputs = pXmlRoot->FirstChildElement( "mc1_outputs" );

   //if ( pXmlMC1_outputs == NULL )
   //   {
   //   CString msg( "Unable to find <mc1_outputs> tag reading " );
   //   msg += filename;
   //   Report::ErrorMsg( msg );
   //   return false;
   //   }

   //TiXmlElement *pXmlMC1_output = pXmlMC1_outputs->FirstChildElement( "mc1_output" );

   //while ( pXmlMC1_output != NULL )
   //   {
   //   // you need some stucture to store these in
   //   MC1_OUTPUT *pMC1_output = new MC1_OUTPUT;
   //   XML_ATTR setAttrs[] = {
   //      // attr                    type          address                           isReq  checkCol
   //      { "id",                   TYPE_INT,      &(pMC1_output->id),                 true,   0 },
   //      { "name",                 TYPE_CSTRING,  &(pMC1_output->name),               true,   0 },
   //      { "site_index_file",      TYPE_CSTRING,  &(pMC1_output->siteIndexFile),      true,   0 },
   //      { "pvt_file",             TYPE_CSTRING,  &(pMC1_output->pvtFile),            true,   0 },
   //      { NULL,                   TYPE_NULL,     NULL,                             false,  0 } };

   //   ok = TiXmlGetAttributes( pXmlMC1_output, setAttrs, "mc1_output", filename, NULL );
   //   if ( ! ok )
   //      {
   //      CString msg; 
   //      msg.Format( _T("Misformed root element reading <mc1_output> attributes in input file %s"), filename );
   //      Report::ErrorMsg( msg );
   //      delete pMC1_output;
   //      }
   //   else
   //      {
   //      m_mc1_outputArray.Add( pMC1_output );
   //      }

   //pXmlMC1_output = pXmlMC1_output->NextSiblingElement( "mc1_output" );
   //   }

   // get the VEGCLASS Transition Type log binary switch value **************************************************
   TiXmlElement* pXmlTransDeltas = pXmlRoot->FirstChildElement("transition_deltas");

   if (pXmlTransDeltas == NULL)
      {
      CString msg("Unable to find <transition_deltas> tag reading ");
      msg += filename;
      Report::ErrorMsg(msg);
      return false;
      }

   XML_ATTR setAttrsvegtransdelta[] = {
      // attr                       type           address                                      isReq  checkCol        
         { "id", TYPE_INT, &(m_useVegTransFlag.useVegTransFlag), true, 0 },
         { NULL, TYPE_NULL, NULL, false, 0 } };

   ok = TiXmlGetAttributes(pXmlTransDeltas, setAttrsvegtransdelta, filename, NULL);


   // get vegtrans file info next  *****************************************************************************
   TiXmlElement* pXmlVegtransfiles = pXmlRoot->FirstChildElement("vegtransfiles");

   if (pXmlVegtransfiles == NULL)
      {
      CString msg("Unable to find <vegtransfiles> tag reading ");
      msg += filename;
      Report::ErrorMsg(msg);
      return false;
      }

   TiXmlElement* pXmlVegtransfile = pXmlVegtransfiles->FirstChildElement("vegtransfile");

   LPCTSTR probFilename = NULL, detFilename = NULL, probMultFilename = NULL;

   XML_ATTR setAttrsveg[] = {
      // attr                      type           address              isReq  checkCol        
         { "probability_filename",    TYPE_STRING, &probFilename,     true, 0 },
         { "deterministic_filename",  TYPE_STRING, &detFilename,      true, 0 },
         { "probMultiplier_filename", TYPE_STRING, &probMultFilename, false, 0 },
         { NULL, TYPE_NULL, NULL, false, 0 } };

   ok = TiXmlGetAttributes(pXmlVegtransfile, setAttrsveg, filename, NULL);

   int retVal = PathManager::FindPath(probFilename, m_vegtransfile.probability_filename);  //  return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 
   if (retVal < 0)
      {
      CString msg;
      msg.Format("Probability Transition file '%s' not found - Dynamic Veg will be disabled...", probFilename);
      Report::ErrorMsg(msg);
      return false;
      }

   retVal = PathManager::FindPath(detFilename, m_vegtransfile.deterministic_filename);  //  return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 
   if (retVal < 0)
      {
      CString msg;
      msg.Format("Deterministic Transition file '%s' not found - Dynamic Veg will be disabled...", detFilename);
      Report::ErrorMsg(msg);
      return false;
      }

   if (probMultFilename != NULL && probMultFilename[0] != NULL)
      {
      retVal = PathManager::FindPath(probMultFilename, m_vegtransfile.probMultiplier_filename);  //  return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 
      if (retVal < 0)
         {
         CString msg;
         msg.Format("Probablistic Multiplier file '%s' not found - Dynamic Veg will be disabled...", probMultFilename);
         Report::ErrorMsg(msg);
         return false;
         }
      }

   // get initializer info next  *****************************************************************************
   TiXmlElement* pXmlInitializers = pXmlRoot->FirstChildElement("initializers");

   if (pXmlInitializers == NULL)
      {
      CString msg("Unable to find <initializers> tag reading ");
      msg += filename;
      Report::ErrorMsg(msg);
      return false;
      }

   TiXmlElement* pXmlInitializer = pXmlInitializers->FirstChildElement("initializer");

   XML_ATTR setAttrsini[] = {
      // attr                     type           address                            isReq  checkCol        
         { "age_class_initializer", TYPE_INT, &(m_initializer.initAgeClass), true, 0 },
         { "tsd_initializer", TYPE_INT, &(m_initializer.initTSD), true, 0 },
         { NULL, TYPE_NULL, NULL, false, 0 } };

   ok = TiXmlGetAttributes(pXmlInitializer, setAttrsini, filename, NULL);

   // get Outputs next ******************************************************************************************
   TiXmlElement* pXmlOutputs = pXmlRoot->FirstChildElement("outputs");

   if (pXmlOutputs != NULL)
      {
      TiXmlElement* pXmlOutput = pXmlOutputs->FirstChildElement("output");

      while (pXmlOutput != NULL)
         {
         // you need some stucture to store these in
         OUTPUT* pOutput = new OUTPUT;
         XML_ATTR setAttrs[] = {
            // attr                    type          address                 isReq  checkCol
               { "name", TYPE_CSTRING, &(pOutput->name), true, 0 },
               { "query", TYPE_CSTRING, &(pOutput->query), true, 0 },
               { NULL, TYPE_NULL, NULL, false, 0 } };

         ok = TiXmlGetAttributes(pXmlOutput, setAttrs, filename, NULL);

         if (!ok)
            {
            CString msg;
            msg.Format(_T("Misformed element reading <output> attributes in input file %s"), filename);
            Report::ErrorMsg(msg);
            delete pOutput;
            }
         else
            {
            m_outputArray.Add(pOutput);
            }

         pXmlOutput = pXmlOutput->NextSiblingElement("output");
         }
      }

   return true;
   }



bool DynamicVeg::PvtSiteindex2DeltaArr(EnvContext* pEnvContext, int idu, int mc1row, int mc1col, int year)
   {
   /*From Gabe email exchange 05172011 1635  Topic MC1 netCDF index to Eugene IDU index

      The data dimensions are 76x40x93. That's 76 columns (longitude), 40 rows (latitude), and 93 time-slices (years).
      The northwest corner of the study area is row 1, column 1 in the netCDF file. To translate the row and column
      numbers in the netCDF file into the row and column numbers in the MC1-IDU lookup table that I sent yesterday,
      you will need to add 687 to the row number, and add 196 to the column number.

      Again, row_number(netCDF)+687=MC1ROW (in the csv lookup table I sent yesterday)

      and

      column_number(netCDF+196=MC1COL (in the csv lookup table I sent yesterday)
      */

   const
      MapLayer* pLayer = pEnvContext->pMapLayer;

   ASSERT(0);   // NOTE: THE SITE-SPECIFIC DEPENDENCIES NEED TO BE REMOVED!!!
   ASSERT(m_climateChange == true);

   int row_index = mc1row - 688;  //vectors are zero indexed, so -688
   int col_index = mc1col - 197;  //vectors are zero indexed, so -197
   int mc1sito10;

   m_mc1pvt = m_vpvt.at(year).at(row_index).at(col_index);  //[year,row,column] 3d vector from netCDF file 93x40x76
   m_mc1si = m_vsi.at(year).at(row_index).at(col_index);  //[year,row,column] 3d vector from netCDF file 93x40x76  

   mc1sito10 = RoundToNearest10(m_mc1si);

   if (year == 0)
      {
      ((MapLayer*)(pEnvContext->pMapLayer))->SetData(idu, m_colFutsi, mc1sito10);
      ((MapLayer*)(pEnvContext->pMapLayer))->SetData(idu, m_colPVT, m_mc1pvt);

      }
   else
      {
      int oldsi;

      pLayer->GetData(idu, m_colFutsi, oldsi);
      if (oldsi != mc1sito10)
         UpdateIDU(pEnvContext, idu, m_colFutsi, mc1sito10, ADD_DELTA);

      int oldpvt;

      pLayer->GetData(idu, m_colPVT, oldpvt);
      if (oldpvt != m_mc1pvt)
         UpdateIDU(pEnvContext, idu, m_colPVT, m_mc1pvt, ADD_DELTA);
      }

   return true;
   }


int DynamicVeg::GetAdjustedDisturb(int disturb)
   {
   switch (disturb)
      {
      case PRESCRIBED_FIRE:
      case PRESCRIBED_FIRE_UNDERBURNING:
      case PRESCRIBED_SURFACE_FIRE:
      case PRESCRIBED_LOW_SEVERITY_FIRE:
      case PRESCRIBED_HIGH_SEVERITY_FIRE:
      case PRESCRIBED_STAND_REPLACING_FIRE:
      case PRESCRIBED_FIRE_2:
      case PRESCRIBED_FIRE_UNDERBURNING_2:
      case PRESCRIBED_SURFACE_FIRE_2:
      case PRESCRIBED_LOW_SEVERITY_FIRE_2:
      case PRESCRIBED_HIGH_SEVERITY_FIRE_2:
      case PRESCRIBED_STAND_REPLACING_FIRE_2:
         return 29;   // 29=Prescribed Fire in the co_prob_trans.csv file // SURFACE_FIRE;
         
      case MECHANICAL_THINNING:
      case MECHANICAL_THINNING_2:
         return MECHANICAL_THINNING;

      case MOWING_GRINDING:
      case MOWING_GRINDING_2:
         return MOWING_GRINDING;
    
      case SALVAGE_HARVEST:
      case SALVAGE_HARVEST_2:
         return SALVAGE_HARVEST;
         
      case SUPRESSION:
      case SUPRESSION_2:
         return SUPRESSION;
         
      case THIN_FROM_BELOW:
      case THIN_FROM_BELOW_2:
         return THIN_FROM_BELOW;
         
      case PARTIAL_HARVEST_LIGHT:
      case PARTIAL_HARVEST_LIGHT_2:
         return PARTIAL_HARVEST;
       
      case PARTIAL_HARVEST_HIGH:
      case PARTIAL_HARVEST_HIGH_2:
         return PARTIAL_HARVEST_HIGH;

      case EVEN_AGED_TREATMENT:
      case EVEN_AGED_TREATMENT_2:
         return EVEN_AGED_TREATMENT;
      }
   return disturb;
   }