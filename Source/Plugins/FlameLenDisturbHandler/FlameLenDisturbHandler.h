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
#include <afx.h>
#include "stdafx.h"
#include <EnvExtension.h>
#include <PtrArray.h>
#include <vector>
#include <map>
#include <randgen\Randunif.hpp>
#include <Vdataobj.h>

#include <iostream>
#include <QueryEngine.h>
#include <EnvConstants.h>
#include <afxtempl.h>
#include <initializer_list>
using namespace std;

// Distubance stuff
struct FIRE_STATE
   {
   int   vegClass;
	int	pvt;
	int   region;
   int   variant;
   float none;
   float groundFire;
   float mixedSeverityFire1;
   float mixedSeverityFire2;
   float severeFire;
   int   surfaceToVeg;
   int   mixedSeverityFire1ToVeg;
   int   mixedSeverityFire2ToVeg;
   int   severeToVeg;
   };

struct FIRELOOKUPFILE
   {   
   CString firelookup_filename;
   };

class FlameLenDisturbHandler :  public EnvModelProcess
{
public:
   FlameLenDisturbHandler(void);
   ~FlameLenDisturbHandler(void);

   bool Init   ( EnvContext *pEnvContext, LPCTSTR initStr );
   bool InitRun( EnvContext *pEnvContext, bool useInitSeed);
   bool Run    ( EnvContext *pEnvContext );
   bool LoadXml( LPCTSTR filename );

protected:
   int   m_colFlameLen;
   int   m_colDisturb;
	int   m_colPotentialDisturb;
	int   m_colPotentialFlameLen;
   int   m_colVegClass;
   int   m_colVariant;
   int   m_colPVT;
   int   m_colRegion;
   int   m_colArea;
   int   m_colTSD;
   int   m_pvt;
   int   m_vegClass;
   int   m_variant;
   int   m_region;
   int   m_disturb;
   int   m_TSD;
  
   FIRELOOKUPFILE m_fireLookupFile;
   
   // key is vegclass/variant, value is index into m_fireSeverityArray 
   CMap< __int64, __int64, int, int > m_vegFireMap;

   PtrArray< FIRE_STATE > m_fireSeverityArray;

   // output variables
   float m_surfaceFireAreaHa;
   float m_surfaceFireAreaPct;
   float m_surfaceFireAreaCumHa;
   
   float m_lowSeverityFireAreaHa;
   float m_lowSeverityFireAreaPct;
   float m_lowSeverityFireAreaCumHa;
   
   float m_highSeverityFireAreaHa;
   float m_highSeverityFireAreaPct;   
   float m_highSeverityFireAreaCumHa; 
   
   float m_standReplacingFireAreaHa; 
   float m_standReplacingFireAreaPct; 
   float m_standReplacingFireAreaCumHa;
             
}; // end of class declaration for VDDTadapter

