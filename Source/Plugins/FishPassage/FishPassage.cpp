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


bool FishPassage::Init(EnvContext* pEnvContext, LPCTSTR initStr)
   {
   // for each fish passage restriction, locate in IDU.
   int cols = 2;

   m_pOutputData = new FDataObj(cols, 0);
   //m_pOutputData->SetLan

   return true;
   }



bool FishPassage::InitRun(EnvContext* pEnvContext, bool useInitialSeed)
   {
   //theModel = (FishPassage*)pEnvContext->pEnvExtension;

   //m_pOutputData->ClearRows();

   return true;
   }



bool FishPassage::Run(EnvContext* pEnvContext)
   {
   theModel = (FishPassage*)pEnvContext->pEnvExtension;

   
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
                      { "stream_layer",        TYPE_STRING,   &streamLayer,   true, 0 },
                      { NULL,                  TYPE_NULL,     NULL,           false, 0 } };

   if (TiXmlGetAttributes(pXmlRoot, rattrs, path, NULL) == false)
      return false;

   this->m_pFPLayer = pEnvContext->pMapLayer->GetMapPtr()->GetLayer(fpLayer);
   
   if (this->m_pFPLayer == NULL)
      {
      std::string msg = std::format("Unable to find fish passage map layer {}", fpLayer);
      Report::ErrorMsg(msg.c_str());
      return false;
      }
   
   this->m_pStreamLayer = pEnvContext->pMapLayer->GetMapPtr()->GetLayer(streamLayer);
   if (this->m_pFPLayer == NULL)
      {
      std::string msg = std::format("Unable to find stream map layer {}", fpLayer);
      Report::ErrorMsg(msg.c_str());
      return false;
      }

   return true;
   }