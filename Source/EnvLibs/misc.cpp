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

#include "EnvLibs.h"
#pragma hdrstop

#include "misc.h"
#ifndef NO_MFC
#include <afxtempl.h>
#endif

void GetTileParams( int count, int &cols, CArray<int,int> &rowArray )
   {
   cols = 0;

   if ( count == 0 )
      return;
   
   while( count > cols * (cols+1) )
      cols++;

   rowArray.RemoveAll();
   rowArray.SetSize( cols );

   // have rows, now get cols
   for ( int i=0; i < cols; i++ )
      rowArray[ i ] = cols;  // start with this

   int changeCols = count - ( cols*cols );  // 0 for regular grid, positive if cols needs to increase, negative if cols needs to decrease

   for ( int i=0; i < abs( changeCols ); i++ )
      {
      if ( changeCols < 0 )
         rowArray[ i ] -= 1;
      else
         rowArray[ i ] += 1;
      }
   }



int CleanFileName( LPCTSTR filename)
   {
   LPTSTR ptr = (LPTSTR) filename;

   while (*ptr != NULL)
      {
      switch (*ptr)
         {
         case ' ':   *ptr = '_';    break;  // blanks
         case '\\':  *ptr = '_';    break;  // illegal filename characters 
         case '/':   *ptr = '_';    break;
         case ':':   *ptr = '_';    break;
         case '*':   *ptr = '_';    break;
         case '"':   *ptr = '_';    break;
         case '?':   *ptr = '_';    break;
         case '<':   *ptr = '_';    break;
         case '>':   *ptr = '_';    break;
         case '|':   *ptr = '_';    break;
         }
      ptr++;
      }

   //filename.ReleaseBuffer();
   return lstrlen(filename);
   }

 
int Tokenize( const TCHAR* str, const TCHAR* delimiters, CStringArray &tokens)
   {
   ASSERT( str && delimiters );

   int length = (int) _tcslen( str )+1;
 
   TCHAR *buffer = new TCHAR[ length ];
   _tcscpy_s( buffer, length, str );

   tokens.RemoveAll();
   
   TCHAR* pos = 0;
   TCHAR* token = _tcstok_s( buffer, delimiters, &pos ); // Get first token
   
   while( token != NULL )
      {
      tokens.Add( token );
 
      // Get next token, note that first parameter is NULL
      token = _tcstok_s( NULL, delimiters, &pos );
      }

   delete [] buffer;

   return (int) tokens.GetSize();
   }



int GetTileRects( int count, CRect *parentRect, CArray< CRect, CRect& > &rects )
   {
   rects.RemoveAll();

   int cols = 0;
   CArray<int,int> rowArray;

   GetTileParams( count, cols, rowArray );

   /*
   xxxxxxxxxxxxxxxxxxxxxxxxxxxxx
   x             x             x
   x             x     2       x
   x             x             x
   x     1       xxxxxxxxxxxxxxx
   x             x             x
   x             x      3      x
   x             x             x
   xxxxxxxxxxxxxxxxxxxxxxxxxxxxx
 
   col=2;
   rows=[1,2] 

   xxxxxxxxxxxxxxxxxxxxxxxxxxxxx
   x             x             x
   x      1      x     2       x
   x             x             x
   xxxxxxxxxxxxxxxxxxxxxxxxxxxxx
   x             x             x
   x      3      x      4      x
   x             x             x
   xxxxxxxxxxxxxxxxxxxxxxxxxxxxx

   cols=2
   rows=[2,2]
 */

   int width = parentRect->Width() / cols;
   int left  = parentRect->left;
   int right = left + width;
 
   for ( int i=0; i < cols; i++ )
      {
      int rows = rowArray[ i ];   // # of rows associated with this col

      int height = parentRect->Height() / rows; 
      int top    = parentRect->top;
      int bottom = top + height;
     
      for ( int j=0; j < rows; j++ )
         {
         CRect rect(left, top, right, bottom);
         rects.Add( rect );

         // update to next position
         top = bottom;
         bottom = top + height;
         }

      // update to next position
      left += width;
      right = left + width;
      }

   return (int) rects.GetSize();
   }
