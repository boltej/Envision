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
// VersionUpdater.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"



int _tmain(int argc, _TCHAR* argv[])
   {
   if ( argc != 2 )
      {
      printf( "Usage: VersionUpdater 'filename'\n" );
      return -1;
      }


	FILE *fp;
	const char *file = argv[1];
#ifdef WIN32
	errno_t err;
	if ((err = fopen_s(&fp, file, "rt")) != 0) {
#else
	if ((fp_config = fopen(configfile, "rt")) == NULL) {
#endif
		fprintf(stderr, "Unable to open file %s!\n", file);
		return -1;
	}

   /*TCHAR *file = argv[ 1 ];

   FILE *fp = fopen( file, "rt" );

   if ( fp == NULL )
      {
      printf( "Unable to open file %s\n", file );
      return -1;
      }*/
   
   int majVer, minVer;

   fscanf_s( fp, "%i.%i", &majVer, &minVer );

   printf( "Read version: %i %i\n", majVer, minVer );

   fclose( fp );

#ifdef WIN32
	if ((err = fopen_s(&fp, file, "wt")) != 0) {
#else
	if ((fp_config = fopen(configfile, "wt")) == NULL) {
#endif
		fprintf(stderr, "Unable to open file %s!\n", file);
		return -1;
	}

 /*  fp = fopen( file, "wt" );

   if ( fp == NULL )
      {
      printf( "Unable to open file %s\n", file );
      return -1;
      }*/
   
   fprintf( fp, "%i.%i", majVer, minVer+1 );
   printf( "Writing version %i.%i\n", majVer, minVer+1 );

   fclose( fp );
	return 0;
   }

