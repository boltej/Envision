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
#include <FDataObj.h>
class FlowContext;
class Reach;


class SWAT_Flow_Adapter
{
public:
    SWAT_Flow_Adapter( void ) : m_pClimateData( NULL )
      , m_col_cfmax (-1)      
    { }

   ~SWAT_Flow_Adapter( void ) { if ( m_pClimateData ) delete m_pClimateData; }

  protected:
   int m_col_cfmax ;      
   FDataObj *m_pClimateData;

   // initialization

   float InitSWAT_Flow_Adapter(FlowContext *pFlowContext, LPCTSTR);
   float RunSWAT_Flow_Adapter(FlowContext* pFlowContext, LPCTSTR initStr);
   bool  Init(EnvContext* pEnvContext, LPCTSTR initStr);
   bool  InitRun(FlowContext* pFlowContext, bool useInitialSeed);
   bool  Run(EnvContext* pContext);
   bool  InitYear(EnvContext* pContext);
   bool  RunDay(FlowContext* pFlowContext);

   bool EndYear(EnvContext* pContext);
   void RunSWAT_All();
   void RunSWAT();
   void RunSWAT_Day(int* jiunit, void* prec, int* nhru);
   void InitSWAT_Year();
   void InitSWAT(int* jiunit, int* enviGrid, void* envCC, char* name, int* sz);
   void EndRunSWAT();
   bool LoadXml(EnvContext* pEnvContext, LPCTSTR filename);

   m_RunSWAT_AllFn();
    m_RunSWATFn();
    m_RunSWAT_DayFn(jiunit, prec, nhru);
    m_InitSWAT_YearFn();
    m_InitSWATFn(jiunit, enviGrid, envCC, name, sz);
    m_EndRunSWATFn();


protected:
   CArray< float > m_tMaxArray;     // one for each reach calculated
   int   m_colTMax;

// public (exported) methods



   

};