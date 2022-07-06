// ExamplePlugin.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "FieldCalc.h"
#include <PathManager.h>
#include <randgen/Randunif.hpp>
#include <misc.h>
#include <FDATAOBJ.H>
FieldCalculator* theModel = NULL;


//extern "C" EnvExtension* Factory(EnvContext*)
//   { 
//   return (EnvExtension*) new ExampleModel; 
//   }


bool FieldDef::Init()
   {
   ASSERT(theModel != NULL);

   // compile query
   //if (m_queryStr.GetLength() > 0)
   //   {
   //   CString name = "FieldCalc." + this->m_name;
   //   m_pQuery = theModel->m_pQueryEngine->ParseQuery(m_queryStr, 0, name);
   //
   //   if (m_pQuery == NULL)
   //      {
   //      CString msg;
   //      msg.Format("FieldCalc: Bad query encountered reading field '%s' - Query is '%s'", (LPCTSTR)this->m_name, (LPCTSTR) m_queryStr);
   //      msg += m_queryStr;
   //      Report::ErrorMsg(msg);
   //      }
   //   }

   theModel->CheckCol(theModel->m_pMapLayer, this->m_col, this->m_name, TYPE_FLOAT, CC_AUTOADD);

   LPTSTR query = NULL;
   if (m_queryStr.IsEmpty() == false)
      query = (LPTSTR)(LPCTSTR)m_queryStr;

   m_pMapExpr = theModel->m_pMapExprEngine->AddExpr(m_name, m_mapExprStr, m_queryStr);
   bool ok = theModel->m_pMapExprEngine->Compile(m_pMapExpr);

   //m_pQuery = m_pMapExpr->GetQuery();

   if (!ok)
      {
      CString msg("FieldCalc: Unable to compile map expression ");
      msg += m_mapExprStr;
      msg += " for <field_def> '";
      msg += m_name;
      msg += "'.  The expression will be ignored";
      Report::ErrorMsg(msg);
      }

   return true;
   }




// *************************************************

FieldCalculator::FieldCalculator()
   : EnvModelProcess()
   , m_colArea(-1)
   , m_shuffleIDUs(false)
   , m_pRandUnif(NULL)
   , m_pOutputData(NULL)
      {
      ASSERT(theModel == NULL);  // singleton
      theModel = this;
      }


bool FieldCalculator::Init(EnvContext* pEnvContext, LPCTSTR initStr)
   {
   m_pMapLayer = (MapLayer*)pEnvContext->pMapLayer;
   m_pMapExprEngine = pEnvContext->pExprEngine;
   m_pQueryEngine = pEnvContext->pQueryEngine;

   // load input file (or just return if no file specified)

   if (initStr == NULL || *initStr == '\0')
      return false;

   if (LoadXml(pEnvContext, initStr) == false)
      return false;

   // initialize internal array for sorting IDUs
   int iduCount = m_pMapLayer->GetPolygonCount();

   m_iduArray.SetSize(iduCount);
   for (int i = 0; i < iduCount; i++)
      m_iduArray[i] = i;

   // set data obj column labels
   ASSERT(m_pOutputData == NULL);
   int fieldCount = (int)m_fields.GetSize();
   m_pOutputData = new FDataObj(fieldCount + 1, 0);
   m_pOutputData->SetLabel(0, "Year");

   for (int i = 0; i < fieldCount; i++)
      {
      CString label = m_fields[i]->m_name + ".count";
      m_pOutputData->SetLabel(i + 1, label);
      }

   AddOutputVar("FieldCalc-Counts", m_pOutputData, "");

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
   int iduCount = theModel->m_pMapLayer->GetPolygonCount();

   // shuffle IDU array to randomly look through IDUs when allocating sequences
   // if called for
   if (m_shuffleIDUs)
      ::ShuffleArray< UINT >(m_iduArray.GetData(), iduCount, m_pRandUnif);
   
   // reset field def application counters
   int fieldCount = (int)m_fields.GetSize();
   for (int i = 0; i < fieldCount; i++)
      m_fields[i]->m_count = 0;

   // iterate through IDUs, applying field defs as needed
   for (int m = 0; m < iduCount; m++)
      {
      UINT idu = m_iduArray[m];         // a random grab (if shuffled)

      theModel->m_pQueryEngine->SetCurrentRecord(idu);
      theModel->m_pMapExprEngine->SetCurrentRecord(idu);

      for (int i = 0; i < fieldCount; i++)
         {
         FieldDef* pFD = m_fields[i];

         // if we are in initialization, only initialize field if called for in the input file
         if ( init && pFD->m_initColData == false)
            continue;

         // apply the map expression the the map layer
         if (pFD->m_pMapExpr != NULL)   // is the map expr for the field definition
            {
            bool ok = m_pMapExprEngine->EvaluateExpr( pFD->m_pMapExpr, true);
            if (ok)
               {
               pFD->m_value = (float)pFD->m_pMapExpr->GetValue();
               theModel->UpdateIDU(pEnvContext, idu, pFD->m_col, pFD->m_value, pFD->m_useDelta ? ADD_DELTA : SET_DATA);
               pFD->m_count++;
               }
            }
            else
               pFD->m_value = 0;
         }
      }

   CollectData(pEnvContext->currentYear);

   return true;
   }


bool FieldCalculator::CollectData(int year) 
   {
   int fieldCount = (int) m_fields.GetSize();
   float* data = new float[fieldCount + 1];

   data[0] = year;
   for (int i = 0; i < fieldCount; i++)
      data[i + 1] = m_fields[i]->m_count;

   m_pOutputData->AppendRow(data, fieldCount + 1);

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

   if (TiXmlGetAttributes(pXmlRoot, rattrs, path, m_pMapLayer) == false)
      return false;

   if (areaCol == NULL)
      this->m_colArea = m_pMapLayer->GetFieldCol("AREA");
   else
      this->m_colArea = m_pMapLayer->GetFieldCol(areaCol);

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
      m_pRandUnif = new RandUniform(0.0, (double)m_pMapLayer->GetRecordCount(), 0);
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

      XML_ATTR attrs[] = { // attr          type           address          isReq checkCol
                         { "field",        TYPE_CSTRING,  &pFD->m_name,       true,  CC_AUTOADD | TYPE_FLOAT },
                         { "query",        TYPE_CSTRING,  &pFD->m_queryStr,   false, 0 },
                         { "value",        TYPE_CSTRING,  &pFD->m_mapExprStr, true,  0 },
                         { "use_delta",    TYPE_BOOL,     &pFD->m_useDelta,   false, 0 },
                         { "initialize",   TYPE_BOOL,     &pFD->m_initColData, false, 0 },
                         { NULL,           TYPE_NULL,     NULL,                false, 0 } };

      if (TiXmlGetAttributes(pXmlFieldDef, attrs, path, m_pMapLayer) == false)
         delete pFD;
      else
         {
         pFD->Init();
         m_fields.Add(pFD);
         }

      pXmlFieldDef = pXmlFieldDef->NextSiblingElement("field_def");
      }


   return true;
   }