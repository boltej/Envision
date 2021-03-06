#include "StdAfx.h"
#pragma hdrstop


#include "CSModel.h"
#include <misc.h>
#include <DATE.HPP>
#include "FarmModel.h"

// statics
int CSModel::m_doy = -1;
int CSModel::m_year = -1;
int CSModel::m_idu = -1;

float CSModel::m_pDays = 0;
float CSModel::m_precip = 0;
float CSModel::m_hMeanPrecip = 0;
float CSModel::m_tMean = 0;
float CSModel::m_tMin = 0;
float CSModel::m_tMax = 0;
float CSModel::m_gdd0 = 0;
//float CSModel::m_gdd5 = 0;
float CSModel::m_gdd0Apr15 = 0;
float CSModel::m_gdd5Apr1 = 0;
float CSModel::m_gdd5May1 = 0;
float CSModel::m_pctAWC = 0;
float CSModel::m_pctSat = 0;
float CSModel::m_swc = 0;
float CSModel::m_swe = 0;
float CSModel::m_chuMay1 = 0;
float CSModel::m_pET = 0;


MapLayer* CSModel::m_pMapLayer = NULL;
ClimateStation* CSModel::m_pClimateStation = NULL;
VSMBModel* CSModel::m_pVSMB = NULL;

extern F2RProcess* theProcess;

Query* ParseQuery(QueryEngine* pQE, CString& query, LPCTSTR source);
int ReplaceDates(CString& query);
bool ReplaceDate(CString& query, int index, int month);
int ReplaceKeywordArgs(CString& query, LPCTSTR fnName);


Query* CSWhen::CompileWhen(QueryEngine* pQE, LPCTSTR className)
   {
   // compile when query
   if (when.IsEmpty() == FALSE)
      {
      pWhen = ParseQuery(pQE, when, "CSModel 'When' Compiler");

      if (pWhen == NULL)
         {
         CString msg;
         msg.Format("CSModel 'When' Compiler: Bad 'when' query encountered reading <%s>: 'When' Query is '%s'", (LPCTSTR) className, (LPCTSTR)when);
         Report::ErrorMsg(msg);
         }
      }

   return pWhen;
   }


MapExpr *CSAction::CompileOutcome(MapExprEngine *pME, LPCTSTR name, MapLayer *pLayer)
   {   
   // if outcome defined, parse it
   if (outcome.IsEmpty())
      return NULL;

   int index = outcome.Find('=');
   if (index < 0)
      {
      Report::LogWarning("CSTransition: outcome statement missing '='");
      return NULL;
      }

   outcomeTarget = outcome.Left(index).Trim();
   outcomeExpr = outcome.Mid(index + 1).Trim();

   // outcome target should be an IDU field
   theProcess->CheckCol(pLayer, this->col, this->outcomeTarget, TYPE_INT, CC_MUST_EXIST);

   // see the if outcomeExpr is a constant
   bool isConstant = true;
   bool isInt = true;
   for (int i = 0; i < lstrlen(outcomeExpr); i++)
      {
      if (!(isdigit(outcomeExpr[i]) || outcomeExpr[i] == '-' || outcomeExpr[i] == '.'))
         {
         isConstant = false;
         break;
         }
      else if (outcomeExpr[i] == '.')
         isInt = false;
      }

   if (isConstant)
      {
      if (isInt)
         this->outcomeValue = (int) atoi(outcomeExpr);
      else
         this->outcomeValue = (float)atof(outcomeExpr);
      }
   else
      {
      this->pOutcomeExpr = pME->AddExpr(name, outcomeExpr, NULL);
      bool ok = pME->Compile(pOutcomeExpr);
   
      if (!ok)
         {
         CString msg("CSEvalExpr: Unable to compile map expression '");
         msg += outcomeExpr;
         msg += "'.  The expression will be ignored";
         Report::ErrorMsg(msg);
         }
      }

   return this->pOutcomeExpr;
   }


bool CSAction::TakeAction(EnvContext* pContext, int idu) 
   {
   // if there is an associated action, take it
   if (pOutcomeExpr != NULL) // query satisfied?
      {
      // run outcome expression
      pOutcomeExpr->Evaluate(idu);
      outcomeValue = pOutcomeExpr->GetValue();

      // is the target a field?
      if (col >= 0)
         {
         float value = 0;
         outcomeValue.GetAsFloat(value);
         theProcess->UpdateIDU(pContext, idu, col, value, SET_DATA);
         return true;
         }
      }
   return false;
   }



int CSModel::Init(FarmModel* pFarmModel,MapLayer *pIDULayer, QueryEngine *pQE, MapExprEngine *pME)
   {
   m_pMapLayer = pIDULayer;
   m_pVSMB = pFarmModel->m_useVSMB ? &(pFarmModel->m_vsmb) : NULL;

   // install query engine functions
   pQE->AddUserFn("Avg", CSModel::Avg);
   pQE->AddUserFn("AbovePeriod", CSModel::AbovePeriod);
   pQE->AddUserFn("BelowPeriod", CSModel::BelowPeriod);
   pQE->AddUserFn("DOYFromCHU", CSModel::DOYFromCHU);

   // ad query engine externals
   QExternal* pExt = pQE->AddExternal("DOY"); pExt->SetValue(VData(&m_doy, TYPE_PINT, true));

   pExt = pQE->AddExternal("PDAYS");      pExt->SetValue(VData(&m_pDays, TYPE_PFLOAT, true));

   pExt = pQE->AddExternal("PRECIP");     pExt->SetValue(VData(&m_precip, TYPE_PFLOAT, true));
   pExt = pQE->AddExternal("HPRECIP");    pExt->SetValue(VData(&m_hMeanPrecip, TYPE_PFLOAT, true));
   
   pExt = pQE->AddExternal("GDD0");       pExt->SetValue(VData(&m_gdd0, TYPE_PFLOAT, true));
   pExt = pQE->AddExternal("GDD0APR15");  pExt->SetValue(VData(&m_gdd0Apr15, TYPE_PFLOAT, true));
   pExt = pQE->AddExternal("GDD5APR1");   pExt->SetValue(VData(&m_gdd5Apr1, TYPE_PFLOAT, true));
   pExt = pQE->AddExternal("GDD5MAY1");   pExt->SetValue(VData(&m_gdd5May1, TYPE_PFLOAT, true));
   pExt = pQE->AddExternal("CHUMAY1");    pExt->SetValue(VData(&m_chuMay1, TYPE_PFLOAT, true));

   pExt = pQE->AddExternal("TMIN");       pExt->SetValue(VData(&m_tMin, TYPE_PFLOAT, true));
   pExt = pQE->AddExternal("TMAX");       pExt->SetValue(VData(&m_tMax, TYPE_PFLOAT, true));
   pExt = pQE->AddExternal("TMEAN");      pExt->SetValue(VData(&m_tMean, TYPE_PFLOAT, true));
   
   pExt = pQE->AddExternal("PET");        pExt->SetValue(VData(&m_pET, TYPE_PFLOAT, true));

   pExt = pQE->AddExternal("pAWC");       pExt->SetValue(VData(&m_pctAWC, TYPE_PFLOAT, true));
   pExt = pQE->AddExternal("pSAT");       pExt->SetValue(VData(&m_pctSat, TYPE_PFLOAT, true));
   pExt = pQE->AddExternal("SWC");        pExt->SetValue(VData(&m_swc, TYPE_PFLOAT, true));
   pExt = pQE->AddExternal("SWE");        pExt->SetValue(VData(&m_swe, TYPE_PFLOAT, true));

   // initialize crops by compiling expressions as needed
   for (int i = 0; i < m_crops.GetSize(); i++)
      {
      CSCrop* pCrop = m_crops[i];

      // check/populate and <field> cols for this crop
      for (int j = 0; j < pCrop->m_fields.GetSize(); j++)
         {
         CSField* pField = pCrop->m_fields[j];
         theProcess->CheckCol(m_pMapLayer, pField->col, pField->name, pField->type, CC_AUTOADD);
         }

      // compile transitions, crop events, and do actions
      for (int j = 0; j < pCrop->m_cropStages.GetSize(); j++)
         {
         CSCropStage* pStage = pCrop->m_cropStages[j];

         for (int k = 0; k < pStage->m_transitions.GetSize(); k++)
            {
            CSTransition* pTrans = pStage->m_transitions[k];
            pTrans->CompileWhen(pQE, "transition");
            pTrans->CompileOutcome(pME, "Transition", pIDULayer);
            }

         for (int k = 0; k < pStage->m_events.GetSize(); k++)
            {
            CSCropEvent* pEvent= pStage->m_events[k];
            pEvent->CompileWhen(pQE, "crop_event");
            pEvent->CompileOutcome(pME, "CropEvent", pIDULayer);

            // compile YRF expression if needed
            if (pEvent->yrf < 0)
               {
               pEvent->pYrfExpr = pME->AddExpr("yrf", pEvent->yrfExpr, NULL);
               bool ok = pME->Compile(pEvent->pYrfExpr);
            
               if (!ok)
                  {
                  CString msg("  CS Model: Unable to compile YRF expression '");
                  msg +=pEvent->yrfExpr;
                  msg += "'.  The expression will be ignored";
                  Report::ErrorMsg(msg);
                  }
               }
            }

         for (int k = 0; k < pStage->m_evalExprs.GetSize(); k++)
            {
            CSEvalExpr* pExpr = pStage->m_evalExprs[k];
            pExpr->CompileWhen(pQE, "do");
            pExpr->CompileOutcome(pME, "EvalExpr", pIDULayer);
            }
         }
      }

   return 1;
   }

int CSModel::InitRun( EnvContext *pContext)
   {
   // Initialize field cols where appropriate
   CSCrop* pCrop = NULL;
   
   for (int idu = 0; idu < m_pMapLayer->GetRecordCount(); idu++)
      {
      int lulc = -1;
      m_pMapLayer->GetData(idu, FarmModel::m_colLulc, lulc);

      if (this->m_cropLookup.Lookup(lulc, pCrop))
         {
         for (int i = 0; i < pCrop->m_fields.GetSize(); i++)
            {
            CSField* pField = pCrop->m_fields[i];
            theProcess->UpdateIDU(pContext, idu, pField->col, pField->defaultValue, SET_DATA);
            }

         // initialize crop stage as well
         theProcess->UpdateIDU(pContext, idu, FarmModel::m_colCropStage, pCrop->m_cropStages[0]->m_id, SET_DATA);
         theProcess->UpdateIDU(pContext, idu, FarmModel::m_colCropStage, pCrop->m_cropStages[0]->m_vsmbStage, SET_DATA);
         }
      }  

   return true;
   }

float CSModel::UpdateCropStatus(EnvContext* pContext, FarmModel* pFarmModel, Farm* pFarm, 
      ClimateStation* pStation, MapLayer* pIDULayer, int idu, float areaHa, 
      int doy, int year, int lulc, int cropStage, float priorCumYRF)
   {
   float yrf = 0;
   m_doy = doy;  // 1-based
   m_year = year;
   m_idu = idu;
   m_pClimateStation = pStation;

   m_pDays = m_pClimateStation->m_pDays[doy - 1];  // doy is one-based

   // set current precip for this idu
   m_pClimateStation->GetData(doy, year, PRECIP, m_precip);
   m_pClimateStation->GetHistoricMean(doy, PRECIP, m_hMeanPrecip);

   m_pClimateStation->GetData(doy, year, TMIN, m_tMin);
   m_pClimateStation->GetData(doy, year, TMEAN, m_tMean);
   m_pClimateStation->GetData(doy, year, TMAX, m_tMax);

   m_gdd0 = m_pClimateStation->m_cumDegDays0[doy-1];
   m_gdd0Apr15 = m_pClimateStation->m_cumDegDays0Apr15[doy-1];
   m_gdd5Apr1 = m_pClimateStation->m_cumDegDays5Apr1[doy-1];
   m_gdd5May1 = m_pClimateStation->m_cumDegDays5May1[doy-1];

   m_chuMay1 = m_pClimateStation->m_chuCornMay1[doy - 1];
   m_pET     = m_pClimateStation->GetPET(VPM_PRIESTLEY_TAYLOR, doy, year);

   // soil properties
   if (m_pVSMB)
      {
      m_swc    = m_pVSMB->GetAverageSWC(idu);
      m_pctSat = m_pVSMB->GetPercentSaturated(idu);
      m_pctAWC = m_pVSMB->GetPercentAWC(idu);
      m_swe    = m_pVSMB->GetSWE(idu);
      }
   CSCrop* pCrop = NULL;
   bool ok = m_cropLookup.Lookup(lulc, pCrop);

   if (!ok)
      return 0;

   // valid CSModel crop, run the model
   if (cropStage < 1 || cropStage > pCrop->m_cropStages.GetSize())  // crop stage is one-based
      {
      CString msg;
      msg.Format("CSModel::CheckCondition() - invalid crop stage index (%i) encountered processing crop %s", cropStage, (LPCTSTR)pCrop->m_name);
      Report::LogError(msg);
      return 0;
      }

   CSCropStage* pStage = pCrop->m_cropStages[cropStage-1];
   ASSERT(pStage != NULL);

   QueryEngine* pQE = pContext->pQueryEngine;

   // are we tracking this IDU?
   bool track = pFarmModel->IsIDUTracked(idu);

   // we have a crop stage, check <eval>s
   for (int i = 0; i < pStage->m_evalExprs.GetSize(); i++)
      {
      CSEvalExpr* pExpr = pStage->m_evalExprs[i];
      ASSERT(pExpr->pWhen != NULL);

      bool result;
      ok = pExpr->pWhen->Run(idu, result);  // run the query

      if (ok && result) // query satisfied?
         {
         // if there is an associated action, take it
         pExpr->TakeAction(pContext, idu);
         }
      }

   // next, check <crop_event>s
   for (int i = 0; i < pStage->m_events.GetSize(); i++)
      {
      CSCropEvent* pEvent = pStage->m_events[i];
      ASSERT(pEvent->pWhen != NULL);

      bool result;
      ok = pEvent->pWhen->Run(idu, result);  // run the query

      if (ok && result) // query satisfied?
         {
         float eventYRF = pEvent->GetYRF(idu);
         yrf = pFarmModel->AddCropEvent(pContext, idu, pEvent->id, pEvent->name, areaHa, doy, eventYRF, priorCumYRF);

         // if there is an associated action, take it
         pEvent->TakeAction(pContext,idu);

         if (track)
            {
            CString msg;
            msg.Format("CSModel: Crop Event %s fired on day %i on IDU %i", (LPCTSTR)pEvent->name, doy, idu);
            Report::LogInfo(msg);
            }
         }
      }

   // events finished, check transitions
   for (int i = 0; i < pStage->m_transitions.GetSize(); i++)
      {
      CSTransition* pTrans = pStage->m_transitions[i];
      ASSERT(pTrans->pWhen != NULL);

      bool result;
      ok = pTrans->pWhen->Run(idu, result);  // run the query

      // query satisfied? then fire transition
      if (ok && result)
         {
         // get the transition
         int toStage = -1;
         for (int j = 0; j < pCrop->m_cropStages.GetSize(); j++)
            {
            // is the the stage we are transitioning TO?
            if (pCrop->m_cropStages[j]->m_name.CompareNoCase(pTrans->toStage) == 0)
               {
               // tell the IDU it is in a the new state
               theProcess->UpdateIDU(pContext, idu, FarmModel::m_colCropStage, pCrop->m_cropStages[j]->m_id, SET_DATA);
               theProcess->UpdateIDU(pContext, idu, FarmModel::m_colVSMBStage, pCrop->m_cropStages[j]->m_vsmbStage, SET_DATA);

               // fire a crop event indicating the transition
               pFarmModel->AddCropEvent(pContext, idu, -99, pTrans->toStage, areaHa, doy, 0, priorCumYRF);

               // special cases - signal planting date
               if (pTrans->toStage.CompareNoCase("Planted") == 0 )
                  theProcess->UpdateIDU(pContext, idu, FarmModel::m_colPlantDate, doy, SET_DATA);
               
               else if (pTrans->toStage.CompareNoCase("Harvested") == 0)
                  theProcess->UpdateIDU(pContext, idu, FarmModel::m_colHarvDate, doy, SET_DATA);

               // if there is an associated action, take it
               pTrans->TakeAction(pContext, idu);

               if (track)
                  {
                  CString msg;
                  msg.Format("CSModel: Crop %s transitioning to stage %s on day %i on IDU %i", (LPCTSTR)pCrop->m_name, (LPCTSTR)pTrans->toStage, doy, idu);
                  Report::LogInfo(msg);
                  }

               break;
               }
            }
         }
      }
   return yrf;
   }

// QUserFn
float CSModel::Avg(int kw, int period)
   {
   switch (kw)
      {
      case CS_TMIN:
      case CS_TMEAN:
      case CS_TMAX:
         {
         int TVAR = TMEAN;
         if (kw == CS_TMIN)
            TVAR = TMIN;
         else if (kw == CS_TMAX)
            TVAR = TMAX;

         float temp = 0;
         int _period = 0;
         for (int i = 0; i < period; i++)
            {
            float _temp = 0;
            if (m_doy - i >= JAN1)
               {
               m_pClimateStation->GetData(m_doy - i, m_year, TVAR, _temp);
               _period++;
               }
            temp += _temp;
            }
         return temp / _period;
         }

      case CS_PRECIP:
         {
         float precip = 0;
         int _period = 0;
         for (int i = 0; i < period; i++)
            {
            float _precip = 0;
            if (m_doy - i >= JAN1)
               {
               m_pClimateStation->GetData(m_doy - i, m_year, PRECIP, _precip);
               _period++;
               }
            precip += _precip;
            }
         return precip / _period;
         }

      case CS_HPRECIP:
         float precip = 0;
         int _period = 0;
         for (int i = 0; i < period; i++)
            {
            float _precip = 0;
            if (m_doy - i >= JAN1)
               {
               m_pClimateStation->GetHistoricMean(m_doy - i, PRECIP, _precip);
               _period++;
               }
            precip += _precip;
            }
         return precip / _period;
   
   //case CS_PHOTO:
      //   break;
      }

   return 0;
   }

float CSModel::AbovePeriod(int kw, int threshold)
   {
   switch (kw)
      {
      case CS_TMIN:
      case CS_TMEAN:
      case CS_TMAX:
      case PRECIP:
         {
         int period = 0;

         int VAR = PRECIP;
         if (kw == CS_TMIN)
            VAR = TMIN;
         else if (kw == CS_TMAX)
            VAR = TMAX;
         else if (kw == CS_TMEAN)
            VAR = TMEAN;

         float value = 0;
         while (true)
            {
            if (m_doy - period < 1)
               return (float) period;

            m_pClimateStation->GetData(m_doy - period, m_year, VAR, value);
            if (value < threshold)
               return (float) period;

            period++;
            }
         return (float) period;
         }

      }

   return 0.0;
   }


float CSModel::DOYFromCHU(int chu, int)
   {
   int doy = 0;
   m_pClimateStation->GetDOYFromCHU(chu, doy);
   
   return (float)doy;
   }


float CSModel::BelowPeriod(int kw, int threshold)
   {
   switch (kw)
      {
      case CS_TMIN:
      case CS_TMEAN:
      case CS_TMAX:
      case PRECIP:
         {
         int period = 0;

         int VAR = PRECIP;
         if (kw == CS_TMIN)
            VAR = TMIN;
         else if (kw == CS_TMAX)
            VAR = TMAX;
         else if (kw == CS_TMEAN)
            VAR = TMEAN;

         float value = 0;
         while (true)
            {
            if (m_doy - period < 1)
               return (float)period;

            m_pClimateStation->GetData(m_doy - period, m_year, VAR, value);
            if (value > threshold)
               return (float)period;

            period++;
            }
         return (float)period;
         }

      }

   return 0.0;
   }


bool CSModel::LoadXml(TiXmlElement* pXmlRoot, LPCTSTR path, MapLayer *pIDULayer)
   {
   // Root is <crops>

   // process <crop> tags
   TiXmlElement* pXmlCrop = pXmlRoot->FirstChildElement("crop");

   while (pXmlCrop != NULL)
      {
      CSCrop* pCrop = new CSCrop;
      CString root = NULL;

      XML_ATTR attrs[] = { // attr          type           address                  isReq checkCol
                         { "name",          TYPE_CSTRING,  &pCrop->m_name,          true,  0 },
                         { "id",            TYPE_INT,      &pCrop->m_id,            true, 0 },
                         { "code",          TYPE_CSTRING,  &pCrop->m_code,          true, 0 },
                         //{ "rotation",      TYPE_BOOL,     &pCrop->m_isRotation,    true,  0 },
                         { "harvestStartYr",TYPE_INT,      &pCrop->m_harvestStartYr,false, 0 },
                         { "harvestFreq",   TYPE_INT,      &pCrop->m_harvestFreq,   false, 0 },
                         { "yrfThreshold",  TYPE_FLOAT,    &pCrop->m_yrfThreshold,  false, 0 },
                         { "root",          TYPE_CSTRING,  &root,                   false, 0 },
                         { NULL,           TYPE_NULL,     NULL,          false, 0 } };

      if (TiXmlGetAttributes(pXmlCrop, attrs, path, NULL) == false)
         {
         delete pCrop;
         return false;
         }

      m_crops.Add(pCrop);
      m_cropLookup.SetAt(pCrop->m_id, pCrop);

      if (root.GetLength() > 0)
         { 
         pCrop->m_rootCoefficentTable = new FDataObj;
         pCrop->m_rootCoefficentTable->ReadAscii(root);
         }
      // process <fields>
      TiXmlElement* pXmlField = pXmlCrop->FirstChildElement("field");
      while (pXmlField != NULL)
         {
         CSField* pField = new CSField;
         CString type;
         CString value;

         XML_ATTR attrs[] = { // attr  type           address        isReq checkCol
                            { "name",  TYPE_CSTRING,  &pField->name, true, 0 },
                            { "type",  TYPE_CSTRING,  &type,         true, 0 },
                            { "value", TYPE_CSTRING,  &value,        true, 0 },
                            { NULL,    TYPE_NULL,     NULL,          false,0 } };

         if (TiXmlGetAttributes(pXmlField, attrs, path, NULL) == false)
            {
            delete pField;
            return false;
            }

         // type
         if (type.CompareNoCase("int")==0)
            pField->type = TYPE_INT;
         else if (type.CompareNoCase("float")==0)
            pField->type = TYPE_FLOAT;
         else
            pField->type = TYPE_STRING;

         // value
         ReplaceDates(value);
         pField->defaultValue.Parse(value);     // ???????? NEED TO CHECK FOR DATES

         pCrop->m_fields.Add(pField);
         pXmlField = pXmlField->NextSiblingElement("field");
         }

      // process <stages>
      TiXmlElement* pXmlStage= pXmlCrop->FirstChildElement("stage");
      CString name;
      int vsmbRoot=0;
      while (pXmlStage != NULL)
         {
         XML_ATTR attrs[] = { // attr type           address        isReq checkCol
                   { "name",      TYPE_CSTRING,  &name,          true,  0 },
                   { "rootStage", TYPE_INT,      &vsmbRoot,      false, 0 },
                   { NULL,        TYPE_NULL,     NULL,           false, 0 } };

         if (TiXmlGetAttributes(pXmlStage, attrs, path, NULL) == false)
            {
            CString msg("CSModel::LoadXml() - Error reading <eval> tag while loading stage '");
            msg += name;
            msg += "'";
            Report::LogError(msg);
            return false;
            }
           
         else
            {
            CSCropStage* pStage = new CSCropStage(name);
            pStage->m_id = (int) pCrop->m_cropStages.Add(pStage);
            pStage->m_id += 1;   // make 1-based
            pStage->m_vsmbStage = vsmbRoot;

            // process <do>s
            TiXmlElement* pXmlEval = pXmlStage->FirstChildElement("do");
            while (pXmlEval != NULL)
               {
               CSEvalExpr* pEval = new CSEvalExpr;

               XML_ATTR attrs[] = { // attr   type           address          isReq checkCol
                                  { "name",   TYPE_CSTRING,  &pEval->name,    true, 0 },
                                  { "when",   TYPE_CSTRING,  &pEval->when,    true, 0 },
                                  { "action", TYPE_CSTRING,  &pEval->outcome, true, 0 },
                                  { NULL,   TYPE_NULL,     NULL,           false,0 } };

               if (TiXmlGetAttributes(pXmlEval, attrs, path, NULL) == false)
                  {
                  CString msg("CSModel::LoadXml() - Error reading <eval> tag while loading stage '");
                  msg += pStage->m_name;
                  msg += "'";
                  Report::LogError(msg);
                  delete pEval;
                  return false;
                  }

               // pEvent->CompileWhen(pQueryE);  NEED TO DO THIS SOMEWHERE!
               pStage->m_evalExprs.Add(pEval);              

               pXmlEval = pXmlEval->NextSiblingElement("do");
               }

            // process <crop_event>
            TiXmlElement* pXmlEvent = pXmlStage->FirstChildElement("crop_event");
            while (pXmlEvent != NULL)
               {
               CSCropEvent* pEvent = new CSCropEvent;
               LPCTSTR yrf = NULL;

               XML_ATTR attrs[] = { // attr   type           address        isReq checkCol
                                  { "name",   TYPE_CSTRING,  &pEvent->name,  true, 0 },
                                  { "id",     TYPE_INT,      &pEvent->id,    false, 0 },
                                  { "yrf",    TYPE_STRING,   &yrf,   false, 0 },
                                  //{ "yrf",    TYPE_FLOAT,    &pEvent->yrf,   true, 0 },
                                  { "when",   TYPE_CSTRING,  &pEvent->when,  true, 0 },
                                  { "action", TYPE_CSTRING,  &pEvent->outcome,  false, 0},
                                  { NULL,     TYPE_NULL,     NULL,           false,0 } };

               if (TiXmlGetAttributes(pXmlEvent, attrs, path, NULL) == false)
                  {
                  CString msg("CSModel::LoadXml() - Error reading <crop_event> tag while loading stage '");
                  msg += pStage->m_name;
                  msg += "'";
                  Report::LogError(msg);
                  delete pEvent;
                  return false;
                  }

               // pEvent->CompileWhen(pQueryE);  NEED TO DO THIS SOMEWHERE!
               int index = pStage->m_events.Add(pEvent);

               if (pEvent->id == -1)
                  pEvent->id = index+1;

               if (yrf == NULL)
                  pEvent->yrf = 1;
               else if ( MapExprEngine::IsConstant(yrf) )
                  pEvent->yrf= (float) atof(yrf);
               else
                  {
                  pEvent->yrfExpr = yrf;
                  pEvent->yrf = -1;  // this signals that expr should be compiled
                  }

               pXmlEvent = pXmlEvent->NextSiblingElement("crop_event");
               }

            // process <transition>
            TiXmlElement* pXmlTrans = pXmlStage->FirstChildElement("transition");
            while (pXmlTrans != NULL)
               {
               CSTransition* pTrans = new CSTransition;

               XML_ATTR attrs[] = { // attr    type          address         isReq checkCol
                                  { "to",      TYPE_CSTRING, &pTrans->toStage,  true, 0 },
                                  { "when",    TYPE_CSTRING, &pTrans->when,     true, 0 },
                                  { "action",  TYPE_CSTRING, &pTrans->outcome,  false, 0 },
                                  { NULL,   TYPE_NULL,     NULL,             false,0 } };

               if (TiXmlGetAttributes(pXmlTrans, attrs, path, NULL) == false)
                  {
                  CString msg("CSModel::LoadXml() - Error reading <transition> tag while loading stage '");
                  msg += pStage->m_name;
                  msg += "'";
                  Report::LogError(msg);
                  delete pTrans;
                  return false;;
                  }

               // pEvent->CompileWhen(pQueryE);  NEED TO DO THIS SOMEWHERE!
               pStage->m_transitions.Add(pTrans);

               pXmlTrans = pXmlTrans->NextSiblingElement("transition");
               }

            pXmlStage = pXmlStage->NextSiblingElement("stage");
            }
         }      

         CString msg;
         msg.Format("  CSModel: Added crop %s with %i stages", (LPCTSTR) pCrop->m_name, (int) pCrop->m_cropStages.GetSize());
         Report::LogInfo(msg);

         for (int i = 0; i < pCrop->m_cropStages.GetSize(); i++)
            {
            CSCropStage* pStage = pCrop->m_cropStages[i];
            msg.Format("    Stage [%s]: %i <evals>, %i <crop_events>, %i <transitions>", (LPCTSTR) pStage->m_name, 
               (int)pStage->m_evalExprs.GetSize(), (int)pStage->m_events.GetSize(), (int)pStage->m_transitions.GetSize());
            Report::LogInfo(msg);
            }

         pXmlCrop = pXmlCrop->NextSiblingElement("crop");
      }

   return true;
   }

Query* ParseQuery(QueryEngine* pQE, CString& query, LPCTSTR source) //"CSModel.CSCropEvent");
   {
   ReplaceDates(query);
   ReplaceKeywordArgs(query, "Avg(");
   ReplaceKeywordArgs(query, "AbovePeriod(");
   ReplaceKeywordArgs(query, "BelowPeriod(");

   Query* pQuery = pQE->ParseQuery(query, 0, source);
   return pQuery;
   }

int ReplaceKeywordArgs(CString& query, LPCTSTR fnName)
   {
   // only look inside "Avg(xxx)" statements
   TCHAR buffer[8];

   int index = query.Find(fnName);  // e.g. Avg(, Count(
   while (index >= 0)
      {
      int start = index + lstrlen(fnName);
      int end = start + 1;

      int match = 1;
      while (match > 0)
         {
         if (query[end] == ')')
            match--;
         else if (query[end] == '(')
            match++;

         end++;
         }

      CString args = query.Mid(start, end - start - 1);

      _itoa(CS_PRECIP, buffer, 10);
      args.Replace("PRECIP", buffer);

      _itoa(CS_TMEAN, buffer, 10);
      args.Replace("TMEAN", buffer);

      _itoa(CS_TMIN, buffer, 10);
      args.Replace("TMIN", buffer);

      _itoa(CS_TMAX, buffer, 10);
      args.Replace("TMAX", buffer);
      
      query.Delete(start,end-start-1);
      query.Insert(start,args);
      
      index = query.Find(fnName, index+6);
      }
   return 1;
   }

int ReplaceDates(CString &query)
   {
   // before parsing query, replace any doy references with the actual DOY
   int index = 0;
   while (index >= 0)
      {
      index = query.Find("JAN", index);
      if (index >= 0 && query.GetLength() >= index + 4 && isdigit(query[index + 3]))
         {
         if ( index==0 || ! isalnum(query[index-1]))
            ReplaceDate(query, index, 0);
         index += 3;
         }
      }

   index = 0;
   while (index >= 0)
      {
      index = query.Find("FEB", index);
      if (index >= 0 && query.GetLength() >= index + 4 && isdigit(query[index + 3]))
         {
         if (index == 0 || !isalnum(query[index - 1]))
            ReplaceDate(query, index, 1);
         index += 3;
         }
      }

   index = 0;
   while (index >= 0)
      {
      index = query.Find("MAR", index);
      if (index >= 0 && query.GetLength() >= index + 4 && isdigit(query[index + 3]))
         {
         if (index == 0 || !isalnum(query[index - 1]))
            ReplaceDate(query, index, 2);
         index += 3;
         }
      }

   index = 0;
   while (index >= 0)
      {
      index = query.Find("APR",index);
      if (index >= 0 && query.GetLength() >= index + 4 && isdigit(query[index + 3]))
         {
         if (index == 0 || !isalnum(query[index - 1]))
            ReplaceDate(query, index, 3);
         index += 3;
         }
      }

   index = 0;
   while (index >= 0)
      {
      index = query.Find("MAY", index);
      if (index >= 0 && query.GetLength() >= index + 4 && isdigit(query[index + 3]))
         {
         if (index == 0 || !isalnum(query[index - 1]))
            ReplaceDate(query, index, 4);
         index += 3;
         }
      }
   index = 0;
   while (index >= 0)
      {
      index = query.Find("JUN", index);
      if (index >= 0 && query.GetLength() >= index + 4 && isdigit(query[index + 3]))
         {
         if (index == 0 || !isalnum(query[index - 1]))
            ReplaceDate(query, index, 5);
         index += 3;
         }
      }
   index = 0;
   while (index >= 0)
      {
      index = query.Find("JUL", index);
      if (index >= 0 && query.GetLength() >= index + 4 && isdigit(query[index + 3]))
         {
         if (index == 0 || !isalnum(query[index - 1]))
            ReplaceDate(query, index, 6);
         index += 3;
         }
      }
   index = 0;
   while (index >= 0)
      {
      index = query.Find("AUG", index);
      if (index >= 0 && query.GetLength() >= index + 4 && isdigit(query[index + 3]))
         {
         if (index == 0 || !isalnum(query[index - 1]))
            ReplaceDate(query, index, 7);
         index += 3;
         }
      }
   index = 0;
   while (index >= 0)
      {
      index = query.Find("SEP", index);
      if (index >= 0 && query.GetLength() >= index + 4 && isdigit(query[index + 3]))
         {
         if (index == 0 || !isalnum(query[index - 1]))
            ReplaceDate(query, index, 8);
         index += 3;
         }
      }
   index = 0;
   while (index >= 0)
      {
      index = query.Find("OCT", index);
      if (index >= 0 && query.GetLength() >= index + 4 && isdigit(query[index + 3]))
         {
         if (index == 0 || !isalnum(query[index - 1]))
            ReplaceDate(query, index, 9);
         index += 3;
         }
      }
   index = 0;
   while (index >= 0)
      {
      index = query.Find("NOV", index);
      if (index >= 0 && query.GetLength() >= index + 4 && isdigit(query[index + 3]))
         {
         if (index == 0 || !isalnum(query[index - 1]))
            ReplaceDate(query, index, 10);
         index += 3;
         }
      }
   index = 0;
   while (index >= 0)
      {
      index = query.Find("DEC", index);
      if (index >= 0 && query.GetLength() >= index + 4 && isdigit(query[index + 3]))
         {
         if (index == 0 || !isalnum(query[index - 1]))
            ReplaceDate(query, index, 11);
         index += 3;
         }
      }
   return 0;
   }

bool ReplaceDate(CString& query, int index, int month)
   {
   // get day string
   int dayIndex = index + 3;
   int day = atoi(query.GetBuffer() + dayIndex);

   int doy = GetDayOfYear(month + 1, day, 2001);  // assume non-leap year 
   TCHAR doyStr[8];
   _itoa(doy, doyStr, 10);

   query.Delete(index, day < 9 ? 4 : 5);
   query.Insert(index, doyStr);

   return true;
   }
