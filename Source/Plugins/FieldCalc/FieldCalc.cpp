// ExamplePlugin.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "FieldCalc.h"
#include <PathManager.h>
#include <randgen/Randunif.hpp>
#include <misc.h>
#include <FDATAOBJ.H>

FieldCalculator* theModel = nullptr;




MapLayer* FieldCalculator::m_pMapLayer=nullptr;               // memory managed by EnvModel
MapExprEngine* FieldCalculator::m_pMapExprEngine = nullptr;     // memory managed by EnvModel
QueryEngine* FieldCalculator::m_pQueryEngine = nullptr;         // memory managed by EnvModel


bool  FieldCalculator::m_initialized = false;
int  FieldCalculator::m_colArea=-1;

CUIntArray  FieldCalculator::m_iduArray;    // used for shuffling IDUs
bool  FieldCalculator::m_shuffleIDUs=true;
RandUniform* FieldCalculator::m_pRandUnif=nullptr;

PtrArray<FieldDef>  FieldCalculator::m_fields;
PtrArray<Constant>  FieldCalculator::m_constants;

//FDataObj* FieldCalculator::m_pOutputData;





//extern "C" EnvExtension* Factory(EnvContext*)
//   { 
//   return (EnvExtension*) new ExampleModel; 
//   }


bool FieldDef::Init()
   {
   ASSERT(theModel != NULL);
   theModel->CheckCol(FieldCalculator::m_pMapLayer, this->m_col, this->m_field, TYPE_FLOAT, CC_AUTOADD);

   LPTSTR query = NULL;
   if (m_queryStr.IsEmpty() == false)
      query = (LPTSTR)(LPCTSTR)m_queryStr;

   m_pMapExpr = FieldCalculator::m_pMapExprEngine->AddExpr(m_name, m_mapExprStr, m_queryStr);
   bool ok = FieldCalculator::m_pMapExprEngine->Compile(m_pMapExpr);

   if (!ok)
      {
      CString msg("FieldCalc: Unable to compile map expression ");
      msg += m_mapExprStr;
      msg += " for <field_def> '";
      msg += m_name;
      msg += "'.  The expression will be ignored";
      Report::ErrorMsg(msg);
      }

   m_pQuery = m_pMapExpr->GetQuery();

   if (IsGroupBy())
      {
      CUIntArray attrValues;
      int count = FieldCalculator::m_pMapLayer->GetUniqueValues(this->m_colGroupBy, attrValues);

      m_groupByValues.SetSize(count);  // set up array to store values
      m_groupByAreas.SetSize(count);  // set up array to store values

      // create maps of (groupAttr,index) pairs
      for (int i = 0; i < count; i++)
         m_groupByIndexMap[attrValues[i], i];
      }

   return true;
   }


// *************************************************

FieldCalculator::FieldCalculator()
   : EnvModelProcess()
   //, m_colArea(-1)
   //, m_shuffleIDUs(false)
   //, m_pRandUnif(NULL)
   //, m_pOutputData(NULL)
      {
      //ASSERT(theModel == NULL);  // singleton
      theModel = this;
      }


bool FieldCalculator::Init(EnvContext* pEnvContext, LPCTSTR initStr)
   {
   theModel = (FieldCalculator*) pEnvContext->pEnvExtension;

   if (FieldCalculator::m_initialized == false)
      {
      FieldCalculator::m_pMapLayer = (MapLayer*)pEnvContext->pMapLayer;
      FieldCalculator::m_pMapExprEngine = pEnvContext->pExprEngine;
      FieldCalculator::m_pQueryEngine = pEnvContext->pQueryEngine;

      // load input file (or just return if no file specified)

      if (initStr == NULL || *initStr == '\0')
         return false;

      if (LoadXml(pEnvContext, initStr) == false)
         return false;

      // initialize internal array for sorting IDUs
      int iduCount = FieldCalculator::m_pMapLayer->GetPolygonCount();

      m_iduArray.SetSize(iduCount);
      for (int i = 0; i < iduCount; i++)
         m_iduArray[i] = i;

      FieldCalculator::m_initialized = true;
      }

   // set data obj column labels
   ASSERT(m_pOutputData == NULL);

   int fieldCount = (int)m_fields.GetSize();
   int fc = 0;
   for (int i = 0; i < fieldCount; i++)
      if (m_fields[i]->m_modelID == theModel->m_id || m_fields[i]->m_modelID == -99)
         fc++;


   m_pOutputData = new FDataObj(1+2*fc, 0);
   m_pOutputData->SetLabel(0, "Year");
   int j = 1;
   for (int i = 0; i < fieldCount; i++)
      {
      if (m_fields[i]->m_modelID == theModel->m_id || m_fields[i]->m_modelID == -99)
         {
         CString label = m_fields[i]->m_name + ".count";
         m_pOutputData->SetLabel(j++, label);
         label = m_fields[i]->m_name + ".area";
         m_pOutputData->SetLabel(j++, label);
         }
      }

   CString outvar = theModel->m_name;
   outvar += ".";
   outvar += "FieldCalc";

   AddOutputVar(outvar, m_pOutputData, "");

   // initialize idu fields if called for
   _Run(pEnvContext, true);

   return true;
   }



bool FieldCalculator::InitRun(EnvContext* pEnvContext, bool useInitialSeed)
   {
   theModel = (FieldCalculator*)pEnvContext->pEnvExtension;

   m_pOutputData->ClearRows();

   return true;
   }



bool FieldCalculator::_Run(EnvContext* pEnvContext, bool init)
   {
   theModel = (FieldCalculator*)pEnvContext->pEnvExtension;

   int iduCount = FieldCalculator::m_pMapLayer->GetPolygonCount();

   // shuffle IDU array to randomly look through IDUs when allocating sequences
   // if called for
   if (m_shuffleIDUs)
      ::ShuffleArray< UINT >(m_iduArray.GetData(), iduCount, m_pRandUnif);
   
   // reset field def application counters
   int fieldCount = (int)m_fields.GetSize();
   int fcUsed = 0;
   for (int i = 0; i < fieldCount; i++)
      {
      FieldDef* pFD = m_fields[i];

      if (pFD->m_modelID == theModel->m_id || pFD->m_modelID == -99)
         {
         fcUsed++;
         pFD->m_count = 0;
         pFD->m_totalArea = 0;

         if (pFD->IsGroupBy())
            {
            int attrCount = (int)pFD->m_groupByValues.GetSize();
            for (int j = 0; j < attrCount; j++)
               {
               pFD->m_groupByValues[j] = 0;
               pFD->m_groupByAreas[j] = 0;
               }
            }
         }
      }

   // iterate through IDUs, applying field defs as needed
   if (fcUsed > 0)
      {
      for (int m = 0; m < iduCount; m++)
         {
         UINT idu = m_iduArray[m];         // a random grab (if shuffled)


         theModel->m_pQueryEngine->SetCurrentRecord(idu);
         theModel->m_pMapExprEngine->SetCurrentRecord(idu);

         float area;
         FieldCalculator::m_pMapLayer->GetData(idu, m_colArea, area);

         for (int i = 0; i < fieldCount; i++)
            {
            FieldDef* pFD = m_fields[i];

            if (pFD->m_modelID == theModel->m_id || pFD->m_modelID == -99)
               {
               // if we are in initialization, only initialize field if called for in the input file
               if (init && pFD->m_initColData == false)
                  continue;

               // is the timing right?
               //if ( pFD->m_timing == 1 && pEnvContext->)

               // apply the map expression the the map layer
               if (pFD->m_pMapExpr != NULL)   // is the map expr for the field definition
                  {
                  bool ok = m_pMapExprEngine->EvaluateExpr(pFD->m_pMapExpr, true);
                  if (ok)
                     {
                     if (pFD->IsGroupBy())
                        {
                        float value = (float)pFD->m_pMapExpr->GetValue();

                        int groupAttr;
                        FieldCalculator::m_pMapLayer->GetData(idu, pFD->m_colGroupBy, groupAttr);
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
                        pFD->m_value = (float)pFD->m_pMapExpr->GetValue();
                        theModel->UpdateIDU(pEnvContext, idu, pFD->m_col, pFD->m_value, pFD->m_useDelta ? ADD_DELTA : SET_DATA);
                        }

                     pFD->m_count++;
                     pFD->m_totalArea += area;
                     }
                  }
               }
            }
         }
      }
   // do anynecessary cleanup
   for (int i = 0; i < fieldCount; i++)
      {
      FieldDef* pFD = m_fields[i];

      if (pFD->m_modelID == theModel->m_id || pFD->m_modelID == -99)
         {
         if (pFD->IsGroupBy())
            {
            // fix up area wted means
            if (pFD->m_operator == FC_OP::FCOP_AREAWTMEAN)
               {
               int attrCount = (int)pFD->m_groupByValues.GetSize();
               for (int j = 0; j < attrCount; j++)
                  {
                  pFD->m_groupByValues[j] /= pFD->m_groupByAreas[j];
                  }
               }

            // write results to IDUs in each group
            for (int m = 0; m < iduCount; m++)
               {
               UINT idu = m_iduArray[m];         // a random grab (if shuffled)
               bool result;
               if (pFD->m_pQuery == NULL || (pFD->m_pQuery->Run(idu, result) && result == true))
                  {
                  int groupAttr;
                  FieldCalculator::m_pMapLayer->GetData(idu, pFD->m_colGroupBy, groupAttr);
                  int index = pFD->m_groupByIndexMap[groupAttr];
                  float value = pFD->m_groupByValues[index];

                  theModel->UpdateIDU(pEnvContext, idu, pFD->m_col, value, pFD->m_useDelta ? ADD_DELTA : SET_DATA);
                  }
               }
            }
         }
      }

   CollectData(pEnvContext->currentYear);

   return true;
   }


bool FieldCalculator::CollectData(int year) 
   {
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

   delete [] data;
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

   LPTSTR areaCol = NULL;
   int    shuffleIDUs = 1;
   XML_ATTR rattrs[] = { // attr          type           address       isReq checkCol
                      { "area_col",     TYPE_STRING,   &areaCol,       false, CC_MUST_EXIST | TYPE_FLOAT },
                      { "shuffle_idus", TYPE_INT,      &shuffleIDUs,   false, 0 },
                      { NULL,           TYPE_NULL,     NULL,           false, 0 } };

   if (TiXmlGetAttributes(pXmlRoot, rattrs, path, FieldCalculator::m_pMapLayer) == false)
      return false;

   if (areaCol == NULL)
      this->m_colArea = FieldCalculator::m_pMapLayer->GetFieldCol("AREA");
   else
      this->m_colArea = FieldCalculator::m_pMapLayer->GetFieldCol(areaCol);

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
      m_pRandUnif = new RandUniform(0.0, (double)FieldCalculator::m_pMapLayer->GetRecordCount(), 0);
      }
   else
      m_shuffleIDUs = false;

   // constants
   TiXmlElement* pXmlConst = pXmlRoot->FirstChildElement("const");
   while (pXmlConst != NULL)
      {
      Constant* pConst = new Constant;

      XML_ATTR attrs[] = { // attr          type           address       isReq checkCol
                         { "name",         TYPE_CSTRING,  &pConst->m_name,   true,  0 },
                         { "value",        TYPE_FLOAT,    &pConst->m_value,  true,  0 },
                         { NULL,           TYPE_NULL,     NULL,              false, 0 } };

      if (TiXmlGetAttributes(pXmlConst, attrs, path, NULL ) == false)
         delete pConst;         
      else
         m_constants.Add(pConst);

      pXmlConst = pXmlConst->NextSiblingElement("const");
      }


   TiXmlElement* pXmlFieldDef = pXmlRoot->FirstChildElement("field_def");
   while (pXmlFieldDef != NULL)
      {
      FieldDef* pFD = new FieldDef;
      LPTSTR op = NULL;

      XML_ATTR attrs[] = { // attr          type           address             isReq checkCol
                         { "name",         TYPE_CSTRING,  &pFD->m_name,       false, 0 },
                         { "field",        TYPE_CSTRING,  &pFD->m_field,        true,  CC_AUTOADD | TYPE_FLOAT },
                         { "query",        TYPE_CSTRING,  &pFD->m_queryStr,    false, 0 },
                         { "value",        TYPE_CSTRING,  &pFD->m_mapExprStr,  true,  0 },
                         { "use_delta",    TYPE_BOOL,     &pFD->m_useDelta,    false, 0 },
                         { "initialize",   TYPE_BOOL,     &pFD->m_initColData, false, 0 },
                         { "groupby",      TYPE_CSTRING,  &pFD->m_groupBy,     false, 0 },
                         { "op",           TYPE_STRING,   &op,                 false, 0 },
                         { "model_id",     TYPE_INT,      &pFD->m_modelID,     false, 0 },
                         { NULL,           TYPE_NULL,     NULL,                false, 0 } };

      if (TiXmlGetAttributes(pXmlFieldDef, attrs, path, FieldCalculator::m_pMapLayer) == false)
         delete pFD;
      else
         {
         if (pFD->m_groupBy.IsEmpty() == false)
            {
            this->CheckCol(m_pMapLayer, pFD->m_colGroupBy, pFD->m_groupBy, TYPE_LONG, CC_AUTOADD | TYPE_FLOAT);
            if (op == NULL)
               {
               CString msg;
               msg.Format("  Warning - Field Definition using groupby should specify operation - AREAWTMEAN() assumed!");
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

         pFD->Init();
         m_fields.Add(pFD);
         }

      pXmlFieldDef = pXmlFieldDef->NextSiblingElement("field_def");
      }

   return true;
   }