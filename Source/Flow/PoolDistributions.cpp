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

#include "PoolDistributions.h"
#include "AlgLib\AlgLib.h"
#include <stdstring.h>

PoolDistributions::PoolDistributions() :
m_pPoolDistQE(NULL)
   { }



PoolDistributions::~PoolDistributions(void)
   {

   if (m_pPoolDistQE != NULL)
      delete m_pPoolDistQE;
   }

int PoolDistributions::ParsePoolDistributions( PCTSTR ldString)
   {
   m_poolDistStr = ldString;

   // if string is empty
   if (m_poolDistStr.IsEmpty())
      return 0;

   CString str = m_poolDistStr;
   CString token;
   int curPos = 0;

   float fraction = 0.0;

   token = str.Tokenize(_T(",() ;\r\n"), curPos);

   while (token != _T(""))
      {
      PoolDist *pLD = new PoolDist;
      pLD->poolIndex = atoi(token);
      token = str.Tokenize(_T(",() ;\r\n"), curPos);
      pLD->fraction = (float)atof(token);
      token = str.Tokenize(_T(",() ;\r\n"), curPos);

      fraction += pLD->fraction;

      m_poolDistArray.Add(pLD);
      }

   // A pool distribution consists of an index and fraction where the fraction indicators of the pool distribution must sum to 1 
   if (alglib::fp_neq(fraction, 1.0))
      {
      CString msg;
      msg.Format(_T("PoolDistributions: Fractions of all the pool_distributions for a pool must total 1.0"));
      Report::ErrorMsg(msg);
      return -1;
      }

   return (int)m_poolDistArray.GetSize();
   }

int PoolDistributions::ParsePools(TiXmlElement *pXmlElement, MapLayer *pIDULayer, LPCTSTR filename)
   {
   // Handle pools of poolDistribution
   TiXmlElement *pXmlPools = pXmlElement->FirstChildElement(_T("pools"));
   if (pXmlPools != NULL)
      {
      int defaultIndex = 0;
      TiXmlGetAttr(pXmlPools, _T("default"), defaultIndex, 0, false);

      TiXmlElement *pXmlPool = pXmlPools->FirstChildElement(_T("pool"));
      while (pXmlPool != NULL)
         {
         // reset pool values
         m_poolDistArray.Clear();

         LPTSTR poolDistStr = NULL;
         LPTSTR poolQryStr = NULL;
         bool isRatio = false;

         XML_ATTR poolAttrs[] = {
            // attr								type			address					     isReq	checkCol
               { _T("pool_distributions"), TYPE_STRING, &poolDistStr, true, 0 },
               { _T("query"), TYPE_STRING, &poolQryStr, false, 0 },
               { _T("ratio"), TYPE_BOOL, &isRatio, false, 0 },
               { NULL, TYPE_NULL, NULL, false, 0 } };

         bool ok = TiXmlGetAttributes(pXmlPool, poolAttrs, filename);
         if (!ok)
            {
            CString msg;
            msg.Format(_T("Pool Distributions: Misformed element reading <pool> attributes in input file %s - it is missing the 'pool_distributions' attribute"), filename);
            Report::ErrorMsg(msg);
            break;
            }
         else
            {
            Pool *pPool = new Pool;

            ParsePoolDistributions( poolDistStr );
            pPool->poolDistArray = m_poolDistArray;

            pPool->isRatio = isRatio;

            m_poolQueryStr = poolQryStr;

            // a query exists
            if (m_poolQueryStr.GetLength() > 0 && pPool->pPoolQuery == NULL)
               {
               if (!m_pPoolDistQE) m_pPoolDistQE = new QueryEngine(pIDULayer);
               pPool->pPoolQuery = m_pPoolDistQE->ParseQuery(m_poolQueryStr, 0, _T("Pool Query"));
               }
            else
               {
               // no query
               pPool->pPoolQuery = NULL;
               }

            m_poolArray.Add(pPool);
            }


         pXmlPool = pXmlPool->NextSiblingElement(_T("pool"));
         } // end of: while ( pXmlPool != NULL )
      }

   m_poolDistArray.Clear();

   return (int)m_poolArray.GetSize();
   }

int PoolDistributions::GetPoolIndex(int idu)
   {
   // return the pool index that matches the idu
   int poolsArraySize = (int)m_poolArray.GetSize();
   for (int index = 0; index < poolsArraySize; index++)
      {
      // get the pool
      Pool *pSoilPool = m_poolArray[index];

      // query exists 
      bool pass = true;
      bool ok = true;
      if (pSoilPool->pPoolQuery)
         {
         ok = pSoilPool->pPoolQuery->Run(idu, pass);
         }

      // found match
      if (ok && pass)
         {
         return index;
         }
      }
   return -1;
   }