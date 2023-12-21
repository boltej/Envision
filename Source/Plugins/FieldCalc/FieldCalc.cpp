// ExamplePlugin.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "FieldCalc.h"
#include <PathManager.h>
#include <randgen/Randunif.hpp>
#include <misc.h>
#include <FDATAOBJ.H>
#include <EnvModel.h>

// the one and only
//FieldCalculator* theMainModel = nullptr;

//MapLayer* FieldCalculator::m_pMapLayer=nullptr;               // memory managed by EnvModel
//MapExprEngine* FieldCalculator::m_pMapExprEngine = nullptr;     // memory managed by EnvModel
//QueryEngine* FieldCalculator::m_pQueryEngine = nullptr;         // memory managed by EnvMode//l

////bool  FieldCalculator::m_initialized = false;
//int  FieldCalculator::m_colArea=-1;

//CUIntArray  FieldCalculator::m_iduArray;    // used for shuffling IDUs
//bool  FieldCalculator::m_shuffleIDUs=true;
//RandUniform* FieldCalculator::m_pRandUnif=nullptr;

//PtrArray<FieldDef>  FieldCalculator::m_fields;
//PtrArray<Constant>  FieldCalculator::m_constants;


//std::vector<FieldCalculator*> fieldCalculators;


//FDataObj* FieldCalculator::m_pOutputData;




extern "C" EnvExtension * Factory(EnvContext*)
   {
   FieldCalculator *pFC = new FieldCalculator;

   //fieldCalculators.push_back(pFC);

   return (EnvExtension*) pFC;
   }




bool FieldDef::Init()
   {
   m_pFieldCalculator->CheckCol(m_pFieldCalculator->m_pMapLayer, this->m_col, this->m_field, this->m_type, CC_AUTOADD | this->m_type);

   //if (this->m_field.IsEmpty() == false)
   //   m_pFieldCalculator->CheckCol(m_pFieldCalculator->m_pMapLayer, this->m_colVariance, this->m_varianceField, TYPE_FLOAT, CC_AUTOADD | this->m_type);

   LPTSTR query = nullptr;
   if (m_queryStr.IsEmpty() == false)
      query = (LPTSTR)(LPCTSTR)m_queryStr;

   m_pMapExpr = m_pFieldCalculator->m_pMapExprEngine->AddExpr(m_name, m_mapExprStr); // , m_queryStr);
   bool ok = m_pFieldCalculator->m_pMapExprEngine->Compile(m_pMapExpr);

   if (!ok)
      {
      CString msg("FieldCalc: Unable to compile map expression ");
      msg += m_mapExprStr;
      msg += " for <field_def> '";
      msg += m_name;
      msg += "'.  The expression will be ignored";
      Report::ErrorMsg(msg);
      }

   if (this->m_queryStr.IsEmpty() == false)
      this->m_pQuery = m_pFieldCalculator->m_pQueryEngine->ParseQuery(this->m_queryStr, 0, this->m_name); //      m_pMapExpr->GetQuery();

   if (IsGroupBy())
      {
      std::vector<int> attrValues;
      int count = m_pFieldCalculator->m_pMapLayer->GetUniqueValues(this->m_colGroupBy, attrValues);

      m_groupByValues.resize(count);  // set up array to store values
      m_groupByAreas.resize(count);  // set up array to store values

      // create maps of (groupAttr,index) pairs
      for (int i = 0; i < count; i++)
         {
         int attrVal = attrValues[i];
         m_groupByIndexMap[attrVal] = i;  // key = attribute vale (e.g. FN_ID, value = index in group arrays
         }
      }

   return true;
   }


// *************************************************

FieldCalculator::FieldCalculator()
   : EnvModelProcess()
   { }

FieldCalculator::~FieldCalculator()
   {
   if (m_pRandUnif != nullptr)
      delete m_pRandUnif;

   if (m_pOutputData != nullptr)
      delete m_pOutputData;

   // remove the fieldcalc ptr from the global collection
   //fieldCalculators.erase(std::remove(fieldCalculators.begin(), fieldCalculators.end(), this), fieldCalculators.end());
   }

bool FieldCalculator::Init(EnvContext* pEnvContext, LPCTSTR initStr)
   {
   // load input file (or just return if no file specified)
   if (initStr == nullptr || *initStr == '\0')
      return false;

   this->m_pMapLayer = (MapLayer*)pEnvContext->pMapLayer;
   this->m_pMapExprEngine = pEnvContext->pExprEngine;
   this->m_pQueryEngine = pEnvContext->pQueryEngine;
   
   if (LoadXml(pEnvContext, initStr) == false)
      return false;

   if (this->m_shuffleIDUs)
      {
      // initialize internal array for sorting IDUs
      int iduCount = this->m_pMapLayer->GetPolygonCount();

      m_iduArray.SetSize(iduCount);
      for (int i = 0; i < iduCount; i++)
         m_iduArray[i] = i;
      }

   // set data obj column labels
   ASSERT(m_pOutputData == nullptr);

   int fieldCount = (int)m_fields.GetSize();
 
   m_pOutputData = new FDataObj(1+4*fieldCount, 0);
   m_pOutputData->SetName((LPCTSTR) this->m_name);
   m_pOutputData->SetLabel(0, "Year");
   int j = 1;
   for (int i = 0; i < fieldCount; i++)
      {
      CString label = m_fields[i]->m_name + ".value";
      m_pOutputData->SetLabel(j++, label);
      label = m_fields[i]->m_name + ".stddev";
      m_pOutputData->SetLabel(j++, label);
      label = m_fields[i]->m_name + ".count";
      m_pOutputData->SetLabel(j++, label);
      label = m_fields[i]->m_name + ".area";
      m_pOutputData->SetLabel(j++, label);
      }

   CString outvar = this->m_name;

   AddOutputVar(outvar, m_pOutputData, "");

   // initialize idu fields if called for
   _Run(pEnvContext, true);

   return true;
   }


bool FieldCalculator::InitRun(EnvContext* pEnvContext, bool useInitialSeed)
   {   
   m_pOutputData->ClearRows();
   return true;
   }


bool FieldCalculator::_Run(EnvContext* pEnvContext, bool init)
   {
   // anything to run?
   bool run = false;
   for (int i = 0; i < (int)this->m_fields.GetSize(); i++)
      {
      FieldDef* pFD = m_fields[i];
      if (pFD->m_modelID == pEnvContext->id)
         {
         // if we are in initialization, only initialize field if called for in the input file
         if (init && pFD->m_initialize != 0)  // (1/-1 inits the field)
            run = true;
         if (!init && pFD->m_initialize >= 0)
            run = true;

         run = true;
         break;
         }
      }

   if (run == false)
      return true;

   int iduCount = this->m_pMapLayer->GetPolygonCount();

   // shuffle IDU array to randomly look through IDUs when allocating sequences
   // if called for
   if (m_shuffleIDUs && m_pRandUnif != nullptr)
      ::ShuffleArray< UINT >(m_iduArray.GetData(), iduCount, m_pRandUnif);

   // reset field def application counters
   int fieldCount = (int)m_fields.GetSize();
   for (int i = 0; i < fieldCount; i++)
      {
      FieldDef* pFD = m_fields[i];

      if (pFD->m_modelID != pEnvContext->id)
         continue;

      pFD->m_count = 0;
      pFD->m_value = 0;  // global value (sum)
      pFD->m_appliedArea = 0;
      pFD->m_variance = 0;
      pFD->m_meanssq = 0;

      if (pFD->IsGroupBy())
         {
         int attrCount = (int)pFD->m_groupByValues.size();
         for (int j = 0; j < attrCount; j++)
            {
            pFD->m_groupByValues[j] = 0;
            pFD->m_groupByAreas[j] = 0;
            }
         }
      }

   // iterate through IDUs, applying field defs as needed
   if (fieldCount > 0)
      {
      for (int m = 0; m < iduCount; m++)
         {
         UINT idu = m;
         if (m_shuffleIDUs)
            idu = m_iduArray[m];         // a random grab (if shuffled)

         this->m_pQueryEngine->SetCurrentRecord(idu);
         this->m_pMapExprEngine->SetCurrentRecord(idu);

         float area;
         this->m_pMapLayer->GetData(idu, m_colArea, area);

         for (int i = 0; i < fieldCount; i++)
            {
            FieldDef* pFD = m_fields[i];

            if (pFD->m_modelID != pEnvContext->id)
               continue;

            // if we are in initialization, only initialize field if called for in the input file
            if (init && pFD->m_initialize == 0)  // (1/-1 inits the field)
               continue;
            if (!init && pFD->m_initialize < 0)
               continue;

            // does this IDU pas the query?
            bool result = false;
            
            if (pFD->m_pQuery == nullptr || (pFD->m_pQuery->Run(idu, result) && result == true))
               {
               // apply the map expression the the map layer
               if (pFD->m_pMapExpr != nullptr)   // is the map expr for the field definition
                  {
                  bool ok = m_pMapExprEngine->EvaluateExpr(pFD->m_pMapExpr, true);
                  if (ok)
                     {
                     float value = (float)pFD->m_pMapExpr->GetValue();
                     ASSERT(std::isnan(value) == false);

                     if (pFD->m_minLimit != LONG_MAX && value < pFD->m_minLimit)
                        value = pFD->m_minLimit;

                     if (pFD->m_maxLimit != LONG_MIN && value > pFD->m_maxLimit)
                        value = pFD->m_maxLimit;

                     if (pFD->IsGroupBy())
                        {
                        int groupAttr;
                        this->m_pMapLayer->GetData(idu, pFD->m_colGroupBy, groupAttr);
                        int index = pFD->m_groupByIndexMap[groupAttr];

                        switch (pFD->m_operator)
                           {
                           case FCOP_SUM:
                           case FCOP_MEAN:
                              pFD->m_groupByValues[index] += value;
                              break;

                           case FCOP_AREAWTMEAN:
                              pFD->m_groupByValues[index] += value * area;
                              break;
                           }
                        pFD->m_groupByAreas[index] += area;
                        }
                     else
                        {
                        switch (pFD->m_operator)
                           {
                           case FCOP_AREAWTMEAN:
                              pFD->m_value += value * area;
                              break;

                           //case FCOP_MIN:
                           //   if ( value < pFD->m_value )
                           //      pFD->m_value = value;
                           //   break;
                           //
                           //case FCOP_MAX:
                           //   if (value > pFD->m_value)
                           //      pFD->m_value = value;
                           //   break;

                           default:
                              //case FCOP_SUM:
                              //case FCOP_MEAN:
                              pFD->m_value += value;
                              break;
                           }
                        this->UpdateIDU(pEnvContext, idu, pFD->m_col, value, pFD->m_useDelta ? ADD_DELTA : SET_DATA);
                        }

                     pFD->m_count++;
                     pFD->m_appliedArea += area;
                     }
                  }
               }
            }
         }  // end of: for (m < iduCount)

       // fix up area wted means
      for (int i = 0; i < fieldCount; i++)
         {
         FieldDef* pFD = m_fields[i];

         if (pFD->m_modelID != pEnvContext->id)
            continue;

         if (pFD->m_operator == FC_OP::FCOP_AREAWTMEAN)
            {
            if (pFD->m_appliedArea != 0)
               pFD->m_value /= pFD->m_appliedArea;
            else
               pFD->m_value = 0;
            }
         else
            pFD->m_value /= pFD->m_count;  // average value

         // write groupbys to each qualifying IDU (non-groupbys handled during _Run())
         if (pFD->IsGroupBy())
            {
            // fix up area wted means
            if (pFD->m_operator == FC_OP::FCOP_AREAWTMEAN)
               {
               int attrCount = (int)pFD->m_groupByValues.size();
               for (int j = 0; j < attrCount; j++)
                  {
                  if (pFD->m_groupByAreas[j] != 0)
                     pFD->m_groupByValues[j] /= pFD->m_groupByAreas[j];
                  else
                     pFD->m_groupByValues[j] = 0;
                  }
               }

            // write results to IDUs in each group
            for (int m = 0; m < iduCount; m++)
               {
               UINT idu = m;
               if (m_shuffleIDUs)
                  idu = m_iduArray[m];         // a random grab (if shuffled)

               bool result = false;
               if (pFD->m_applyEverywhere || pFD->m_pQuery == nullptr || (pFD->m_pQuery->Run(idu, result) && result == true))
                  {
                  int groupAttr;
                  this->m_pMapLayer->GetData(idu, pFD->m_colGroupBy, groupAttr);
                  int index = pFD->m_groupByIndexMap[groupAttr];
                  float value = pFD->m_groupByValues[index];

                  ASSERT(std::isnan(value) == false);
                  if (init)
                     this->UpdateIDU(pEnvContext, idu, pFD->m_col, value, SET_DATA);
                  else
                     this->UpdateIDU(pEnvContext, idu, pFD->m_col, value, pFD->m_useDelta ? ADD_DELTA : SET_DATA);
                  }
               }
            }

         // all done...
         CString msg;
         msg.Format("FieldDef %s: Count=%i, Applied Area=%.1f ha", (LPCTSTR)pFD->m_name, pFD->m_count, pFD->m_appliedArea * HA_PER_M2);
         Report::LogInfo(msg);
         }

      // apply delta array
      pEnvContext->pEnvModel->ApplyDeltaArray((MapLayer*)pEnvContext->pMapLayer);

      // compute variances over IDUs
      for (int idu = 0; idu < iduCount; idu++)
         {
         this->m_pQueryEngine->SetCurrentRecord(idu);
         this->m_pMapExprEngine->SetCurrentRecord(idu);

         for (int i = 0; i < fieldCount; i++)
            {
            FieldDef* pFD = m_fields[i];

            if (pFD->m_modelID != pEnvContext->id)
               continue;

            // if we are in initialization, only initialize field if called for in the input file
            if (init && pFD->m_initialize == 0)  // (1/-1 inits the field)
               continue;
            if (!init && pFD->m_initialize < 0)
               continue;

            bool result = false;
            float sum = 0;
            if (pFD->m_pQuery == nullptr || (pFD->m_pQuery->Run(idu, result) && result == true))
               {
               float value = 0;
               this->m_pMapLayer->GetData(idu, pFD->m_col, value);

               float mean = pFD->m_value;
               pFD->m_meanssq += (value - mean) * (value - mean);
               }
            }
         }  // end of: for each idu

      for (int i = 0; i < fieldCount; i++)
         {
         FieldDef* pFD = m_fields[i];
         if (pFD->m_modelID != pEnvContext->id)
            continue;

         pFD->m_variance = (1.0f / (pFD->m_count - 1.0f)) * pFD->m_meanssq;
         CString msg;
         msg.Format("%s: mean=%f, variance=%f", (LPCTSTR) pFD->m_name, pFD->m_value, pFD->m_variance);
         Report::LogInfo(msg);
         }
      }  // end of: if ( fieldCount < 0 )

   CollectData(pEnvContext->currentYear);

   return true;
   }


bool FieldCalculator::CollectData(int year) 
   {
   int fieldCount = (int) m_fields.GetSize();

   std::vector<float> data;

   data.push_back((float)year);
   for (int i = 0; i < fieldCount; i++)
      {
      data.push_back((float)m_fields[i]->m_value);
      data.push_back((float)m_fields[i]->m_variance);
      data.push_back((float)m_fields[i]->m_count);
      data.push_back((float)m_fields[i]->m_appliedArea);
      }

   m_pOutputData->AppendRow(data.data(), (int) data.size());
   return true;
   }

bool FieldCalculator::LoadXml(EnvContext *pEnvContext, LPCTSTR filename)
   {
   // search for file along path
   CString path;
   int result = PathManager::FindPath(filename, path);  // finds first full path that contains passed in path if relative or filename, return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 

   if (result < 0)
      {
      CString msg("Field Calculator: Unable to find input file ");
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

   CString msg("  Field Calculator: Loading input file ");
   msg += path;
   Report::Log(msg);

   // start interating through the nodes
   TiXmlElement* pXmlRoot = doc.RootElement();  // field_calculator

   LPTSTR areaCol = nullptr, checkCols = nullptr;
   int    shuffleIDUs = 1;
   XML_ATTR rattrs[] = { // attr          type           address       isReq checkCol
                      { "area_col",     TYPE_STRING,   &areaCol,       false, CC_MUST_EXIST | TYPE_FLOAT },
                      { "shuffle_idus", TYPE_INT,      &shuffleIDUs,   false, 0 },
                      { "check_cols",   TYPE_STRING,   &checkCols,     false, 0 },
                      { nullptr,           TYPE_NULL,     nullptr,           false, 0 } };

   if (TiXmlGetAttributes(pXmlRoot, rattrs, path, this->m_pMapLayer) == false)
      return false;

   if (areaCol == nullptr)
      this->m_colArea = this->m_pMapLayer->GetFieldCol("AREA");
   else
      this->m_colArea = this->m_pMapLayer->GetFieldCol(areaCol);

   if (m_colArea < 0)
      {
      CString msg("Field Calculator: unable to find AREA field in input file");
      msg += filename;
      Report::ErrorMsg(msg);
      return false;
      }

   if (shuffleIDUs)
      {
      m_shuffleIDUs = true;
      if ( m_pRandUnif == nullptr)
         m_pRandUnif = new RandUniform(0.0, (double)this->m_pMapLayer->GetRecordCount(), 0);
      }
   else
      m_shuffleIDUs = false;


   if (checkCols != nullptr)
      {
      CStringArray tokens;
      int count = ::Tokenize(checkCols, _T(",;"), tokens);

      int col = 0;
      for (int i = 0; i < count; i++)
         {
         CStringArray token;
         int _count = ::Tokenize(tokens[i], _T(":"), token);

         TYPE type = TYPE_INT;
         if (_count == 2)
            {
            switch (_tolower(token[1][0]))
               {
               case 'f':   type = TYPE_FLOAT;   break;
               case 'd':   type = TYPE_DOUBLE;  break;
               case 's':   type = TYPE_STRING;  break;
               case 'l':   type = TYPE_LONG;    break;
               }
            }
         this->CheckCol(m_pMapLayer, col, token[0], type, CC_AUTOADD);
         }
      }

   // constants
   TiXmlElement* pXmlConst = pXmlRoot->FirstChildElement("const");
   while (pXmlConst != nullptr)
      {
      Constant* pConst = new Constant;

      XML_ATTR attrs[] = { // attr          type           address       isReq checkCol
                         { "name",         TYPE_CSTRING,  &pConst->m_name,   true,  0 },
                         { "value",        TYPE_FLOAT,    &pConst->m_value,  true,  0 },
                         { nullptr,           TYPE_NULL,     nullptr,              false, 0 } };

      if (TiXmlGetAttributes(pXmlConst, attrs, path, nullptr) == false)
         delete pConst;
      else
         {
         m_constants.Add(pConst);
         m_pMapExprEngine->AddVariable(pConst->m_name, &(pConst->m_value));
         }

      pXmlConst = pXmlConst->NextSiblingElement("const");
      }
   

   TiXmlElement* pXmlFieldDef = pXmlRoot->FirstChildElement("field_def");
   while (pXmlFieldDef != nullptr)
      {
      FieldDef* pFD = new FieldDef(this);
      LPTSTR op = nullptr;
      LPTSTR type = nullptr;
      LPTSTR apply = nullptr;


      XML_ATTR attrs[] = { // attr           type           address             isReq checkCol
                         { "name",           TYPE_CSTRING,  &pFD->m_name,           false, 0 },
                         { "field",          TYPE_CSTRING,  &pFD->m_field,          true, 0 },
                         { "query",          TYPE_CSTRING,  &pFD->m_queryStr,       false, 0 },
                         { "value",          TYPE_CSTRING,  &pFD->m_mapExprStr,     true,  0 },
                         { "max_limit",      TYPE_FLOAT,    &pFD->m_maxLimit,       false, 0 },
                         { "min_limit",      TYPE_FLOAT,    &pFD->m_minLimit,       false, 0 },
                         { "type",           TYPE_STRING,   &type,                  false, 0 },
                         { "use_delta",      TYPE_BOOL,     &pFD->m_useDelta,       false, 0 },
                         { "initialize",     TYPE_INT,      &pFD->m_initialize,     false, 0 },
                         { "groupby",        TYPE_CSTRING,  &pFD->m_groupBy,        false, 0 },
                         { "op",             TYPE_STRING,   &op,                    false, 0 },
                         { "apply",          TYPE_STRING,   &apply,                 false, 0},
                         { "model_id",       TYPE_INT,      &pFD->m_modelID,        false, 0 },
                         { nullptr,          TYPE_NULL,     nullptr,             false, 0 } };

      if (TiXmlGetAttributes(pXmlFieldDef, attrs, path, this->m_pMapLayer) == false)
         delete pFD;
      else
         {
         if (type != nullptr)
            {
            switch (type[0])
               {
               case 'i':
               case 'I':
                  pFD->m_type = TYPE_INT;  break;
               }
            }

         if (pFD->m_modelID == this->m_id || pFD->m_modelID == -99)
            {
            if (pFD->m_groupBy.IsEmpty() == false)
               {
               this->CheckCol(m_pMapLayer, pFD->m_colGroupBy, pFD->m_groupBy, TYPE_LONG, CC_AUTOADD);
               if (op == nullptr)
                  {
                  CString msg;
                  msg.Format("Warning - Field Definition using groupby should specify operation - AREAWTMEAN() assumed!");
                  Report::WarningMsg(msg);
                  pFD->m_operator = FCOP_AREAWTMEAN;
                  }
               else
                  {
                  FC_OP _op = FCOP_AREAWTMEAN;
                  if (op[0] == 's' or op[0] == 'S')
                     _op = FCOP_SUM;
                  else if (op[0] == 'm' or op[0] == 'M')
                     _op = FCOP_MEAN;

                  pFD->m_operator = _op;
                  }
               }

            pFD->m_applyEverywhere = ((apply != nullptr ) ? true : false);

            pFD->Init();
            m_fields.Add(pFD);
            }
         }

      pXmlFieldDef = pXmlFieldDef->NextSiblingElement("field_def");
      }

   return true;
   }