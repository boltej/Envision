// MLAdapter.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "MLAdapter.h"

#include <Maplayer.h>
#include <QueryEngine.h>
#include <tixml.h>


bool MLAdapter::Init(EnvContext* pEnvContext, LPCTSTR initStr)
   {
   bool ok = LoadXml(pEnvContext, initStr);
   return ok;
   }


bool MLAdapter::InitRun(EnvContext *pEnvContext, bool useInitialSeed) 
   { 
   return true; 
   }


bool MLAdapter::Run(EnvContext* pEnvContext)
   {
   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;
 
   return true;
   }



bool MLAdapter::LoadXml(EnvContext* pEnvContext, LPCTSTR filename)
   {/*
   TiXmlDocument doc;
   bool ok = doc.LoadFile(filename);

   if (!ok)
      {
      Report::ErrorMsg(doc.ErrorDesc());
      return false;
      }

   MapLayer* pMapLayer = (MapLayer*)pEnvContext->pMapLayer;

   // start interating through the nodes
   TiXmlElement* pXmlRoot = doc.RootElement();  // <acute_hazards>
   CString codePath;

   TiXmlElement* pXmlHazard = pXmlRoot->FirstChildElement("hazard");
   while (pXmlHazard != NULL)
      {
      CString name, hazField, acmPctileField, hazPctileField, viField, query;
      XML_ATTR attrs[] = {
         // attr                type            address  isReq checkCol
         { "name",              TYPE_CSTRING,   &name,           true,  0 },
         { "hazard_col",        TYPE_CSTRING,   &hazField,       true,  CC_MUST_EXIST },
         { "acm_pctile_col",    TYPE_CSTRING,   &acmPctileField, false, CC_AUTOADD },
         { "hazard_pctile_col", TYPE_CSTRING,   &hazPctileField, false, CC_AUTOADD },
         { "vi_col",            TYPE_CSTRING,   &viField,        true,  CC_MUST_EXIST },
         { "query",             TYPE_CSTRING,   &query,          true,  0 },
         { NULL,                TYPE_NULL,      NULL,            false, 0 }
         };
      
      ok = TiXmlGetAttributes(pXmlHazard, attrs, filename, pMapLayer);
      if (ok)
         {
         Hazard* pHazard = new Hazard;
         this->m_hazardArray.Add(pHazard);

         pHazard->m_name = name;
         pHazard->m_hazardField = hazField;  
         pHazard->m_acmPctileField = acmPctileField;
         pHazard->m_hazPctileField = hazPctileField;
         pHazard->m_vulnIndexField = viField;
         pHazard->m_query = query;

         pHazard->m_pQuery = pEnvContext->pQueryEngine->ParseQuery(query, 0, pHazard->m_name);

         CheckCol(pEnvContext->pMapLayer, pHazard->m_colHazard, hazField, TYPE_FLOAT, CC_MUST_EXIST);

         if ( ! acmPctileField.IsEmpty() )
            CheckCol(pEnvContext->pMapLayer, pHazard->m_colAcmPctile, acmPctileField, TYPE_FLOAT, CC_MUST_EXIST);

         if (! hazPctileField.IsEmpty())
            CheckCol(pMapLayer, pHazard->m_colHazPctile, hazPctileField, TYPE_FLOAT, CC_MUST_EXIST);

         CheckCol(pMapLayer, pHazard->m_colVulnIndex, viField, TYPE_FLOAT, CC_MUST_EXIST);

         TiXmlElement* pXmlAC = pXmlHazard->FirstChildElement("adapt_cap_metric");
         while (pXmlAC != NULL)
            {
            CString name, field;
            XML_ATTR attrs[] = {
               // attr     type            address     isReq checkCol
               { "name",   TYPE_CSTRING,   &name,      false,  0 },
               { "col",    TYPE_CSTRING,   &field,     true,  CC_MUST_EXIST },
               { NULL,     TYPE_NULL,      NULL,       false, 0 }
               };

            ok = TiXmlGetAttributes(pXmlAC, attrs, filename, pMapLayer);
            if (ok)
               {
               pHazard->m_adaptCapFields.Add(field);
               int col = -1;
               CheckCol(pMapLayer, col, field, TYPE_FLOAT, CC_MUST_EXIST);
               }

            pXmlAC = pXmlAC->NextSiblingElement("adapt_cap_metric");
            }
         }

      pXmlHazard = pXmlHazard->NextSiblingElement("hazard");
      }*/
   
   return true;
   }





