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
#include "FiresList.h"
#include <MapLayer.h>
#include <PathManager.h>
#include <Vdataobj.h>
#include <randgen\randunif.hpp>

#include "FlamMapAP.h"

extern  FlamMapAP *gpFlamMapAP;
 

FiresList::FiresList(void)
{
   m_maxYear = 0;
   m_headerString = "";
}


FiresList::~FiresList(void)
{
   firesList.clear();
}

void FiresList::Init(const char *_initFName) 
   {
   CString initFName;
   if ( PathManager::FindPath( _initFName, initFName ) < 0 )    // search envision paths
      {
      CString msg;
      msg.Format( "FlammapAP: Unable to locate firelist file '%s' on path", _initFName );
      Report::ErrorMsg( msg );
      return;
      }
   
   VDataObj data(U_UNDEFINED);
   int records = data.ReadAscii( initFName );  // autodetect delimiter
   CString tRecord;
   if ( records < 0 )
      {
      CString msg( _T("FiresList::Init(): Failed to open file ") );
      msg += _initFName;
      Report::ErrorMsg(msg);
      gpFlamMapAP->m_runStatus = 0;
      return;
      }

   //TCHAR fmsBase[MAX_PATH];      // NOTE: this assumes fms files are in the same director as the firelist!!!
   //lstrcpy( fmsBase, initFName );
   //char *p = _tcsrchr( fmsBase, '\\');
   //p++;
   //*p = 0;
   TCHAR fmsBase[ MAX_PATH ];
   lstrcpy( fmsBase, gpFlamMapAP->m_fmsPath );   // this will be termainted with a '\'

   int colYr            = -1;
   int colProb          = -1;
   int colJulian        = -1;
   int colBurnPer       = -1;
   int colWindSpeed     = -1;
   int colAzimuth       = -1;
   int colFMfile        = -1;
   int colERC           = -1;
   int colOriginal_Size = -1;
   int colCause         = -1;
   int colIgnitionX     = -1;
   int colIgnitionY = -1;
   int colFireID = -1;
   int fireNum = 1;
   if ( DoesColExist( initFName, data, colYr            , "Yr"           ) == false )   return;
   if ( DoesColExist( initFName, data, colProb          , "Prob"         ) == false )   return;
   if ( DoesColExist( initFName, data, colJulian        , "Julian"       ) == false )   return;
   if ( DoesColExist( initFName, data, colBurnPer       , "BurnPer"      ) == false )   return;
   if ( DoesColExist( initFName, data, colWindSpeed     , "WindSpeed"    ) == false )   return;
   if ( DoesColExist( initFName, data, colAzimuth       , "Azimuth"      ) == false )   return;
   if (DoesColExist(initFName, data, colFMfile, "FMfile") == false)   return;
   if (DoesColExist(initFName, data, colIgnitionX, "IGNITIONX") == false)   return;
   if (DoesColExist(initFName, data, colIgnitionY, "IGNITIONY") == false)   return;
   DoesColExist(initFName, data, colERC, "ERC"); // these two are optional
   if ( DoesColExist( initFName, data, colOriginal_Size , "Original_Size") == false )
      {
      DoesColExist( initFName, data, colOriginal_Size , "Size");
      }
   //DoesColExist(initFName, data, colFireID, "fireID");
   colFireID = data.GetCol("fireID");
   //if ( DoesColExist( initFName, data, colCause         , "Cause"        ) == false )   return;
   //if ( DoesColExist( initFName, data, colIgnitionX     , "IgnitionX"    ) == false )   return;
   //if ( DoesColExist( initFName, data, colIgnitionY     , "IgnitionY"    ) == false )   return;
   m_headerString = "";
   for (int tc = 0; tc < data.GetColCount(); tc++)
   {
	   if (tc > 0)
		   m_headerString += ",";
	   m_headerString += data.GetLabel(tc);
   }
   FILE * echoFiresList = fopen(gpFlamMapAP->m_outputEchoFirelistName, "a+t");
   fprintf(echoFiresList, "%s, EnvFire_ID\n", (LPCTSTR) m_headerString);
   fclose(echoFiresList);

   for ( int row=0; row < data.GetRowCount(); row++ )
      {
      float monteCarloProb = (float) gpFlamMapAP->m_pFiresRand->RandValue(0, 1);
 
      int     yr;
      int     julDate;
      int     burnPeriod;
      int     windSpd;
      int     windAzmth;
      CString fmsName;     // filename only
      float   burnProb;
	   int     erc = 0;
	   float   origSizeHA =0;
	   float igX, igY;
	   CString fireID = "";
      data.Get( colYr           , row,  yr );
      data.Get( colProb         , row,  burnProb );
      data.Get( colJulian       , row,  julDate );
      data.Get( colBurnPer      , row,  burnPeriod );
      data.Get( colWindSpeed    , row,  windSpd );
      data.Get( colAzimuth      , row,  windAzmth );
      data.Get( colFMfile       , row,  fmsName ); // just the filename, no path
	  data.Get(colERC, row, erc);
	  data.Get(colIgnitionX, row, igX);
	  data.Get(colIgnitionY, row, igY);

      if ( colOriginal_Size > 0 )
         data.Get( colOriginal_Size, row,  origSizeHA );
      //data.Get( colCause        , row,  cause );  //?  These are currently ignored
      //data.Get( colIgnitionX    , row,  ignitX );
      //data.Get( colIgnitionY    , row,  ignitY );
	  if (colFireID >= 0)
		  data.Get(colFireID, row, fireID);
	  else
		  fireID.Format("%d", fireNum);
      Fire tmpFire;
      CString fmsFull( fmsBase );
      fmsFull += fmsName;

      CString fmsFullPath;
      int retVal = PathManager::FindPath( fmsFull, fmsFullPath );

      if ( retVal < 0 )
         {
         CString msg;
         msg.Format( "FlammapAP: Unable to find fuel moisture file '%s' when searching paths", (LPCTSTR) fmsFull );
         Report::ErrorMsg( msg );
         }

	   if (yr <= 0 || burnProb <= 0.0 || julDate <= 0 || burnPeriod <= 0 || fmsName.GetLength() <= 0)
		  continue;
	   tRecord = "";
	   for (int tc = 0; tc < data.GetColCount(); tc++)
	   {
		   if (tc > 0)
			   tRecord += ",";
		   tRecord += data.GetAsString(tc, row);
	   }
      // init Fire info.  Note: years are 1-based in input file, make them zero-based here.
	   tmpFire.Init(yr - 1, burnProb, julDate, burnPeriod, windSpd, windAzmth, monteCarloProb, fmsFullPath, fmsName, erc, origSizeHA, igX, igY, fireID, tRecord, gpFlamMapAP->m_envisionFireID++);

      if(tmpFire.GetBurnPeriod() > 0) 
         firesList.push_back(tmpFire);    // add the fire

      if(tmpFire.GetYr() > m_maxYear)
         m_maxYear = tmpFire.GetYr();
	  fireNum++;
      } // for(int i=0;i<recordsTtl;i++)


   CString msg;
   msg.Format( "FlammapAP: Loaded %i fires from firelist file '%s'", data.GetRowCount(), initFName );
   Report::Log( msg );
   /// OLD CODE BELOW
   //int tmp = (int) Fires.size();


   //CString msg;
   //
   //FILE *inFile = NULL;
   //char  inBuf[2048];
   //char fmsBase[MAX_PATH];
   //
   //CString InitFName;
   //PathManager::FindPath( _InitFName, InitFName );
   //
   //if ( ( inFile = fopen(InitFName,"r") ) == NULL) 
   //   {
   //   msg = (_T("FiresList::Init(): Failed to open file "));
   //   msg += InitFName;
   //   Report::ErrorMsg(msg);
   //   gpFlamMapAP->m_runStatus = 0;
   //   return;
   //   }
   //strcpy(fmsBase, InitFName);
   //char *p = strrchr(fmsBase, '\\');
   //p++;
   //*p = 0;
   //// burn the first line of headers
   //fgets(inBuf, 2047,inFile);
   //
   //// read and process data lines
   //while(fgets(inBuf, sizeof(inBuf)-1,inFile) != NULL)
   //   {
   //      float monteCarloProb = (float) gpFlamMapAP->m_pFiresRand->RandValue(0, 1);
   //   
   //   Fire tmpFire( inBuf,monteCarloProb, fmsBase );
   //
   //   if(tmpFire.GetBurnPeriod() > 0) 
   //      m_firesList.push_back(tmpFire);    // add the fire
   //
   //   if(tmpFire.GetYr() > m_maxYear)
   //      m_maxYear = tmpFire.GetYr();
   //} // for(int i=0;i<recordsTtl;i++)
   //
   //fclose(inFile);

} //


int FiresList::AdjustYear(int year, double pRatio)
 {
   bool runFire;
   int numRunFires = 0;
   FireList::iterator i;

   for(i = firesList.begin(); i != firesList.end(); ++i)
      {
      Fire fire = *i;   // jpb = added this operator method

      if( fire.GetYr() == year && fire.GetBurnPeriod() > 0 )
         {
         double monteCarloProb = gpFlamMapAP->m_pFiresRand->RandValue(0, 1);
         runFire = fire.GetBurnProb() * pRatio >= monteCarloProb ? true : false;

         fire.SetDoRun(runFire);
         
         if(runFire)
            numRunFires++;
         }
      }

   return numRunFires;
   }


bool FiresList::DoesColExist( LPCTSTR filename, VDataObj &data, int &col, LPCTSTR field )
   {
   col = data.GetCol( field );

   if ( col < 0 )
      {
      CString msg;
      msg.Format( "FiresList: Input file '%s' is missing a required field '%s'", filename, field );
      return false;
      }

   return true;
   }

