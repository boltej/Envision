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
//-------------------------------------------------------------------
//  DATAOBJ.CPP
//
//-- code for the global data object
//-------------------------------------------------------------------

#include "EnvLibs.h"
#pragma hdrstop

#ifndef NO_MFC
#include <io.h>
#include <afxadv.h>
#endif
#include <limits.h>
#include <math.h>
#include <string.h>

#include "DATAOBJ.H"
#include "STRARRAY.HPP"
#include "Report.h"


const int DO_ALLOCINCREMENT = 20;


//-- DataObj::DataObj() ---------------------------------------------
//
//-- constructors
//-------------------------------------------------------------------

DataObj::DataObj( void )
   : m_dataCols( 0 ),
     m_labelArray()
   {
   }


//-------------------------------------------------------------------
DataObj::DataObj( DataObj &dataObj )
   : m_dataCols( dataObj.m_dataCols )
   {
   this->m_name = dataObj.m_name;     // m_name associated with data object

   for ( int i=0; i < dataObj.m_labelArray.GetSize(); i++ )
      this->m_labelArray.Add( dataObj.m_labelArray[ i ] );
   }


//-------------------------------------------------------------------
DataObj::DataObj( int _cols, int _rows )
   : m_dataCols( _cols ),
     m_labelArray()     
   {
   for ( int i=0; i < _cols; i++ )
      m_labelArray.Add( "None");

   return;
   }


//-- DataObj::~DataObj() --------------------------------------------
//
//-- destructor
//-------------------------------------------------------------------

DataObj::~DataObj( void )
   { }


void DataObj::Clear( void )
   {
   m_labelArray.RemoveAll();
   m_dataCols = 0;
   }


bool DataObj::CheckCol( LPCTSTR field, int &col, int flag ) 
   {
   col = GetCol( field );
   
   if ( col < 0 )
      {
      if ( flag & CC_AUTOADD )
         {
         col = this->AppendCol(field );
         }
      else if ( flag & CC_MUST_EXIST)
         {
         CString msg;
         msg.Format( "Data Object: Column '%s' not found while reading '%s'", field, (LPCTSTR) this->m_name);
         Report::LogError( msg );
         }
      }

   return col >= 0;
   }

//-- DataObj::GetCol() ----------------------------------------------
//
//--  return the col # associated with "colLabel", 0xFFFF if not found
//-------------------------------------------------------------------

int DataObj::GetCol( LPCTSTR colLabel )
   {
   int cols = GetColCount();

   for ( int col=0; col < cols; col++ )
      {
      LPCTSTR label = m_labelArray[ col ];
      int retVal = lstrcmpi( colLabel, label ); //Array[ col ] );
      if ( retVal == 0 )
         return col;
      }

   return -1;
   }


//-- DataObj::SetLabel()---------------------------------------------
//
//--  sets up the label associated with a column
//-------------------------------------------------------------------

int DataObj::SetLabel( int col, LPCTSTR label )
   {
   int cols = GetColCount();

   if ( col >= cols )
      return -1;

   //-- set label for the column / trimming any newlines from the end --//
	const LPCTSTR delims="\r\n";
   CString _label( label );
   _label.TrimRight( delims );
	m_labelArray.SetAtGrow( col, _label );
   
   return (int) m_labelArray.GetSize();
	}


#ifndef NO_MFC
int DataObj::CopyToClipboard( char delimiter /* =\t */)
   {
   USES_CONVERSION;

   // Write to shared file (REMEBER: CF_TEXT is ANSI, not UNICODE, so we need to convert)
   CSharedFile sf(GMEM_MOVEABLE | GMEM_DDESHARE | GMEM_ZEROINIT);

   // Get a tab delimited string to copy to cache
   int rows = GetRowCount();
   int cols = GetColCount();

   CString str;
   for (int col=0; col < cols; col++ )
      str += this->GetLabel( col ) + delimiter;
   str += '\n';
  
   sf.Write(T2A(str.GetBuffer(1)), str.GetLength());
   str.ReleaseBuffer();
   
   for ( int row=0; row < rows; row++ )
      {
      str.Empty();
      for ( int col=0; col < cols; col++)
         {
         str += this->GetAsString( col, row );

         if ( col != cols-1 )
            str += delimiter;
         }

      if ( row != rows-1) 
         str += _T("\n");
        
      sf.Write(T2A(str.GetBuffer(1)), str.GetLength());
      str.ReleaseBuffer();
      }
    
   char c = '\0';
   sf.Write(&c, 1);

   DWORD dwLen = (DWORD) sf.GetLength();
   HGLOBAL hMem = sf.Detach();
   if ( !hMem )
       return NULL;

   hMem = ::GlobalReAlloc( hMem, dwLen, GMEM_MOVEABLE | GMEM_DDESHARE | GMEM_ZEROINIT);
   if ( !hMem )
      return NULL;

   // Cache data
   COleDataSource* pSource = new COleDataSource();
   pSource->CacheGlobalData(CF_TEXT, hMem);
   pSource->SetClipboard();

   return 1;
   }
#endif
