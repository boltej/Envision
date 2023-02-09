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
// DeltaExtractor.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "DeltaExtractor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;
int ExtractDeltas( int argc, TCHAR* argv[] );

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
      ExtractDeltas( argc, argv );
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


int ExtractDeltas( int argc, TCHAR* argv[] )
   {
   // command line should look like:
   // DeltaExtractor <inputfile.csv> <outputfile.csv> <field[,field2...]>

   if ( argc != 4 )
      {
      printf( "Usage: DeltaExtractor inputfile.csv outputfile.csv FIELD1,FIELD2...\n" );
      return -1;
      }

   TCHAR *infile = argv[ 1 ];
   TCHAR *outfile = argv[ 2 ];
   TCHAR *fieldList = argv[ 3 ];
   //TCHAR* options = argv[4];

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

   // count fields in field list
   int fieldCount = 1;
   TCHAR *p = fieldList;
   while (*p != NULL )
      {
      if ( *p == ',' )
         fieldCount++;
      p++;
      }


   bool removeDups = true;
   bool includeFieldName = false;
   //if (  strchr())

   // allocate field ptrs
   TCHAR **fields = new TCHAR*[ fieldCount ];
   TCHAR *next = nullptr;

   TCHAR *fbuffer = new TCHAR[ lstrlen( fieldList )+1 ];
   lstrcpy( fbuffer, fieldList );

   TCHAR *token = _tcstok_s( fbuffer, _T(","), &next );
   int count = 0;
   while ( token != NULL )
      {
      fields[ count ] = token;
      count++;
   
      token = _tcstok_s( NULL, _T( "," ), &next );
      }

   ASSERT( count == fieldCount );

   // header first - just copy from input
   TCHAR buffer[ 128 ];
   fgets( buffer, 127, fpIn );
   fputs( "year,idu,col,field,oldValue,newValue,type\n", fpOut);   // note: dropped "field" (well, not yet...)
   //fputs( "\n", fpOut );

   int year, idu, col, dType;
   TCHAR oldVal[ 32 ], newVal[ 32 ], field[ 32 ];

   _int64 scannedCount = 0;
   _int64 extractedCount = 0;

  printf( "Processing" );

   while ( fgets( buffer, 127, fpIn ) != NULL )
      {
      // next, cycle through records
      //"year,idu,col,field,oldValue,newValue,type
      int count = sscanf( buffer, "%i,%i,%i,%[^,],%[^,],%[^,],%i", &year, &idu, &col, field, oldVal, newVal, &dType );
      scannedCount++;

      if (removeDups == false || lstrcmp(oldVal, newVal) != 0)
         {
         for (int i = 0; i < fieldCount; i++)
            {
            if (lstrcmpi(fields[i], field) == 0)  // match?
               {
               fprintf(fpOut, "%i,%i,%i,%s,%s,%s,%i\n", year, idu, col, field, oldVal, newVal, dType);
               extractedCount++;
               continue;
               }
            }
         }
      if ( scannedCount % 1000000 == 0 )
         printf( "." );
      }
   
   delete [] fields;

   fclose( fpIn );
   fclose( fpOut );

   printf( "\nExtracted %i of %i records from %s", (ULONG) extractedCount, (ULONG) scannedCount, infile );



   return 1;
   }
