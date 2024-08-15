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
// DeltaMapper.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "DeltaMapper.h"
#include <map.h>
#include <maplayer.h>
#include <MapExprEngine.h>
 

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

int ProcessDeltas( int argc, TCHAR* argv[] );
int SaveResult( MapLayer *pIduLayer, int currentYear, int nextYear, QueryEngine *pQE, Query *pQuery, int op, MapExprEngine *pME, MapExpr *pExpr, FILE *fpOut );

enum { OP_SUM=1, OP_AWM=2, OP_PCTAREA=3 };

using namespace std;

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	HMODULE hModule = ::GetModuleHandle(NULL);

	if (hModule != NULL)
	{
		// initialize MFC and print and error on failure
		if (!AfxWinInit(hModule, NULL, ::GetCommandLine(), 0))
		{
			// TODO: change error code to suit your needs
			_tprintf(_T("Fatal Error: MFC initialization failed\n"));
			nRetCode = 1;
		}
		else
		{
      ProcessDeltas( argc, argv );
		}
	}
	else
	{
		// TODO: change error code to suit your needs
		_tprintf(_T("Fatal Error: GetModuleHandle failed\n"));
		nRetCode = 1;
	}

	return nRetCode;
}


int ProcessDeltas( int argc, TCHAR* argv[] )
   {
   // command line should look like:
   // DeltaMapper <inputfile.csv> <outputfile> <shapefile.shp> <"queryStr"> <operator [sum|awm|pctArea]> <"expression">

   if ( argc != 7 )
      {
      printf( "Usage: DeltaMapper <deltafile.csv> <shapefile.shp> <queryStr> <year>\n" );
      return -1;
      }

   TCHAR *deltaFile   = argv[ 1 ];
   TCHAR *iduFile  = argv[ 2 ];
   TCHAR *year  = argv[ 3 ];
   
   FILE *fpDelta = fopen( deltaFile, "rt" );
   if ( fpDelta == NULL )
      {
      printf( "Input file %s not found - aborting", deltaFile );
      return -2;
      }

   Map map;
   MapLayer *pIduLayer = map.AddShapeLayer( iduFile );

   if ( pIduLayer == NULL )
      {
      printf( "Can't load shape file %s - aborting", iduFile );
      return -4;
      }


   // all inputs verified, start processing

   // header first - just copy from input
   TCHAR buffer[ 128 ];
   fgets( buffer, 127, fpDelta );
   
   // start interating through the delta array
   int currentYear = -1;
   float area = 0;

   int year, idu, col, dType;
   TCHAR oldVal[ 32 ], newVal[ 32 ], field[ 32 ];

   _int64 scannedCount = 0;
   _int64 processedCount = 0;

   printf( "Processing" );
   // iterate though each line in the csv 
   while ( fgets( buffer, 127, fpDelta ) != NULL )
      {
      // next, cycle through records
      //"year,idu,col,field,oldValue,newValue,type
      int count = sscanf( buffer, "%i,%i,%i,%[^,],%[^,],%[^,],%i", &year, &idu, &col, field, oldVal, newVal, &dType );
      scannedCount++;

      // are we crossing a year boundary?  if so, write current values and reset
      //if ( year > currentYear )
      //   {
      //   SaveResult( pIduLayer, currentYear, year, &qe, pQuery, op, &me, pMapExpr, fpOut );
      //   currentYear = year;
      //   printf( "." );
      //   }

      // apply the delta
      VData val;
      val.Parse( newVal );
      pIduLayer->SetData( idu, col, val );
      processedCount++;
      }

   // save final result at end of run
   SaveResult( pIduLayer, year);
   
   fclose( fpDelta );

   printf( "\nProcessed %i of %i records from %s", (ULONG) processedCount, (ULONG) scannedCount, deltaFile );

   return 1;
   }


int SaveResult( MapLayer *pIduLayer, int currentYear, int nextYear, QueryEngine *pQE, Query *pQuery, int op, MapExprEngine *pME, MapExpr *pExpr, FILE *fpOut )
   {
   int colArea = pIduLayer->GetFieldCol( "AREA" );

   float area = 0;
   float totalArea = 0;
   float value = 0;

   for ( MapLayer::Iterator idu=pIduLayer->Begin(); idu < pIduLayer->End(); idu++ )
      {
      if ( pQE )
         pQE->SetCurrentRecord( idu );

      if ( pME )
         pME->SetCurrentRecord( idu );

      float area = 0;
      pIduLayer->GetData( idu, colArea, area );
      totalArea += area;

      bool passConstraints = true;
         
      if ( pQuery )
         {
         bool result = false;
         bool ok = pQuery->Run( idu, result );
            
         if ( result == false )
            passConstraints = false;
         }

      // did we pass the constraints?  If so, evaluate preferences
      if ( passConstraints )
         {
         float _value = 0;

         if ( pExpr != NULL )
            {
            bool ok = pME->EvaluateExpr( pExpr, false );
            if ( ok )
               _value = (float) pExpr->GetValue();
            }

         // we have a value for this IDU, accumulate it based on the model type
         switch( op )
            {
            case OP_SUM:
               value += _value;
               break;

            case OP_AWM:
               value += _value * area;
               break;

            case OP_PCTAREA:
               value += area;
               break;
            }
         }  // end of: if( passed constraints)
      }  // end of: for ( each IDU )

   int index = 1;
  
   switch( op )
      {
      case OP_SUM:
         break;      // no fixup required

      case OP_AWM:
         value /= totalArea;
         break;

      case OP_PCTAREA:
         value /= totalArea;
      }

   // write results to output file
   //fprintf( fpOut, "%d,%f\n", currentYear, value );

   //currentYear++;
   while ( currentYear < nextYear )
      fprintf( fpOut, "%d,%f\n", currentYear++, value );
   
   return 1;
   }
   