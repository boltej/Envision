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
class HRU;

#include <EnvExtension.h>
#include <PtrArray.h>

typedef void (*MODFLOW_GETNAMFILFN)  (char*);
typedef void (*SWAT_RUNFN) ();
typedef void (*SWAT_RUN_ALLFN) ();
typedef void (*SWAT_ENDRUNFN) ();
typedef void (*SWAT_RUN_DAYFN) (int*, void*, void*, void*, int*, void*, void*, void*, void*, void*, void*);
typedef void (*SWAT_INIT_YEARFN) ();
typedef void (*SWAT_INITFN) (int*, int*, void*, char*, int*);


struct SWAT_OFFSET_INFO
{
   int    swat_HRU_Number;
   int    idu_offset;     // offset in array
   int    hru_offset;
   HRU* pHRU;
   SWAT_OFFSET_INFO(void) : swat_HRU_Number(-1), idu_offset(-1), hru_offset(-1), pHRU(NULL) { }
};
static PtrArray< SWAT_OFFSET_INFO > m_hruOffsetArray;

class SWAT_Flow_Adapter
{
public:
   SWAT_Flow_Adapter(void) : m_pClimateData(NULL)
      , m_col_cfmax(-1)
      , m_swatPath("t")
      , m_hInst(0)
      , m_RunSWATFn(NULL)
      , m_EndRunSWATFn(NULL)
      , m_InitSWATFn(NULL)
      , m_RunSWAT_AllFn(NULL)
      , m_RunSWAT_DayFn(NULL)
      , m_InitSWAT_YearFn(NULL)
      , m_iGrid(1)
      , m_nhru(0)
      , m_pathSz(0)
   { }

   ~SWAT_Flow_Adapter(void) { if (m_pClimateData) delete m_pClimateData; }

protected:
   int m_col_cfmax;
   CString m_swatPath;
   FDataObj* m_pClimateData;
   HINSTANCE  m_hInst;
   SWAT_RUNFN                   m_RunSWATFn;
   SWAT_ENDRUNFN                m_EndRunSWATFn;
   SWAT_RUN_ALLFN               m_RunSWAT_AllFn;
   SWAT_INITFN                  m_InitSWATFn;
   SWAT_RUN_DAYFN               m_RunSWAT_DayFn;
   SWAT_INIT_YEARFN             m_InitSWAT_YearFn;

   void* m_cc;
   int* m_iunit;
   int m_iGrid;
   int m_nhru;
   int m_pathSz;

   double* m_precipArray;
   double* m_tmnArray;
   double* m_tmxArray;
   double* m_gwRechargeArray;
   double* m_swcArray;
   double* m_aetArray;
   double* m_petArray;
   double* m_irrArray;
   int* m_lulcArray;

   void* m_irrEfficiency;

   double* m_irrToSWATArray;
   //CArray< CPoint, CPoint > m_hruOffsetArray;
   // initialization
   float InitSWAT_Flow_Adapter(FlowContext* pFlowContext, LPCTSTR);

   virtual bool Init(EnvContext* pEnvContext, LPCTSTR initStr);
   virtual bool InitRun(FlowContext* pFlowContext, bool useInitialSeed);
   virtual bool Run(EnvContext* pContext);
   virtual bool RunDay(FlowContext* pFlowContext);
   virtual bool EndYear(EnvContext* pEnvContext);
   virtual bool InitYear(FlowContext* pFlowContext);

   PtrArray< SWAT_OFFSET_INFO > SWAT_Flow_Adapter::m_hruOffsetArray;

public:
   //-------------------------------------------------------------------
   //------ models -----------------------------------------------------
   //-------------------------------------------------------------------
   float RunSWAT_Flow_Adapter(FlowContext* pFlowContext, LPCTSTR initStr);

   void InitSWAT(int*, int*, void*, char*, int*);
   void RunSWAT();
   void EndRunSWAT();
   void RunSWAT_All();
   void RunSWAT_Day(int*, void*, void*, void*, int*, void*, void*, void*, void*, void*, void*);
   void InitSWAT_Year();

   bool LoadXml(EnvContext*, LPCTSTR filename);

};