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
// DeltaProcessor.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "DeltaProcessor.h"
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
   // DeltaProcessor <inputfile.csv> <outputfile> <shapefile.shp> <"queryStr"> <operator [sum|awm|pctArea]> <"expression">

   if ( argc != 7 )
      {
      printf( "Usage: DeltaProcessor <inputfile.csv> <outputfile> <shapefile.shp> <queryStr> <operator [sum|awm|pctArea]> <expression>\n" );
      printf( "  Note that if the query string or expression string have spaces, they must be enclosed in quotes, e.g. \"AREA > 100\"\n" );
      return -1;
      }

   TCHAR *infile   = argv[ 1 ];
   TCHAR *outfile  = argv[ 2 ];
   TCHAR *shpfile  = argv[ 3 ];
   TCHAR *queryStr = argv[ 4 ];
   TCHAR *opStr    = argv[ 5 ];
   TCHAR *exprStr  = argv[ 6 ];
   
   FILE *fpIn = fopen( infile, "rt" );
   if ( fpIn == NULL )
      {
      printf( "Input file %s not found - aborting", infile );
      return -2;
      }

   FILE *fpOut = fopen( outfile, "wt" );
   if ( fpOut == NULL )
      {
      printf( "Can't open output file %s - aborting", outfile );
      return -3;
      }

   // operator
   // enum { OP_SUM=1, OP_AWT=2, OP_PCTAREA=3 };
   int op = 0;
   switch( opStr[ 0 ] )
      {
      case 's':
      case 'S':
         op = OP_SUM;
         break;

      case 'a':
      case 'A':
      op = OP_AWM;
      break;

      case 'p':
      case 'P':
         op = OP_PCTAREA;
         break;
      }

   if ( op == 0 )
      {
      printf( "illegal operator invoked (%s) - aborting", opStr );
      return -3;
      }

   Map map;
   MapLayer *pIduLayer = map.AddShapeLayer( shpfile );

   if ( pIduLayer == NULL )
      {
      printf( "Can't load shape file %s - aborting", shpfile );
      return -4;
      }

   // get field
   //int col = pIduLayer->GetFieldCol( field );
   //if ( col < 0 )
   //   {
   //   printf( "Can't find field [%s] in shape file %s - aborting", field, shpfile );
   //   return -5;
   //   }
   
   // Query next
   Query *pQuery = NULL;
   QueryEngine qe( pIduLayer );
   
   if ( lstrlen( queryStr ) > 0 )
      {
      pQuery = qe.ParseQuery( queryStr, 0, "" );

      if ( pQuery == NULL )
         {
         printf( "Unable to parse query string '%s' - unrecognized syntax - aborting...", queryStr );
         return -5;
         }
      }

   // expression next
   MapExpr *pMapExpr = NULL;
   MapExprEngine me( pIduLayer ,&qe );
   if ( lstrlen( exprStr ) > 0 )
      {
      pMapExpr = me.AddExpr( "MapExpr", exprStr, NULL );
      bool ok = me.Compile( pMapExpr );
      if ( ! ok )
         {
         printf( "Error compiling expression '%s' - aborting...", exprStr );
         return -6;
         }
      }

   if ( pMapExpr == NULL && ( op == OP_SUM || op == OP_AWM ) )
      {
      printf( "Error - no map expression specified - aborting..." );
      return -7;
      }

   // all inputs verified, start processing

   // header first - just copy from input
   TCHAR buffer[ 128 ];
   fgets( buffer, 127, fpIn );

   fprintf( fpOut, "Year,Value\n" );
   
   // start interating through the delta array
   int currentYear = -1;
   float area = 0;

   int year, idu, col, dType;
   TCHAR oldVal[ 32 ], newVal[ 32 ], field[ 32 ];

   _int64 scannedCount = 0;
   _int64 processedCount = 0;

   printf( "Processing" );
   // iterate though each line in the csv 
   while ( fgets( buffer, 127, fpIn ) != NULL )
      {
      // next, cycle through records
      //"year,idu,col,field,oldValue,newValue,type
      int count = sscanf( buffer, "%i,%i,%i,%[^,],%[^,],%[^,],%i", &year, &idu, &col, field, oldVal, newVal, &dType );
      scannedCount++;

      // are we crossing a year boundary?  if so, write current values and reset
      if ( year > currentYear )
         {
         SaveResult( pIduLayer, currentYear, year, &qe, pQuery, op, &me, pMapExpr, fpOut );
         currentYear = year;
         printf( "." );
         }

      // apply the delta
      VData val;
      val.Parse( newVal );
      pIduLayer->SetData( idu, col, val );
      processedCount++;
      }

   // save final result at end of run
   SaveResult( pIduLayer, currentYear, currentYear+1, &qe, pQuery, op, &me, pMapExpr, fpOut );
   
   fclose( fpIn );
   fclose( fpOut );

   printf( "\nProcessed %i of %i records from %s", (ULONG) processedCount, (ULONG) scannedCount, infile );

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
   