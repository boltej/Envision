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

#include "stdafx.h"

#include "ColumnTrace.h"

#include "DeltaArray.h"
#include "DataManager.h"

#include <MAP.h>
#include "EnvModel.h"

//extern EnvModel *gpModel;

ColumnTrace::ColumnTrace( EnvModel *pModel, int column, int run /*= -1*/, const DeltaArray *pDeltaArray /*= NULL*/ )
:  m_run( run ),
   m_columnIndex( column ),
   m_rows( -1 ),
   m_currentYear( 0 ),
   m_maxYear( -1 ),
   m_pDeltaArray( NULL ),
   m_column(U_UNDEFINED)
   {
   ASSERT(pModel != NULL);
   ASSERT(pModel->m_pIDULayer );

   m_rows = pModel->m_pIDULayer->GetRecordCount( MapLayer::ALL );

   // some calling functions already have pointer to correct delta array, some only know run number.  Setting neither parameter
   // sets m_pDeltaArray to the current model delta array
   if (! pDeltaArray)
      m_pDeltaArray = pModel->m_pDataManager->GetDeltaArray( run );
   else
      m_pDeltaArray = pDeltaArray;

   ASSERT( m_pDeltaArray );

   m_maxYear = m_pDeltaArray->GetMaxYear() + 1;  // + 1 to point to the end of the DeltaArray

   m_column.SetSize( 1, m_rows ); // one column, one record for each row in the map

   // load data from original map to the stored one
   VData data;
   for ( int row=0; row < m_rows; row++ )
      {
      bool ok = pModel->m_pIDULayer->GetData( row, m_columnIndex, data );
      ASSERT( ok );

      ok = m_column.Set( 0, row, data );
      ASSERT( ok );
      }

   // if the EnvModel::m_pIDULayer has not been reset, we need to do this now to our ResultsMapWnd.
   // use current delta array rather than column trace delta array, since they can be different
   if ( pModel->m_currentYear > 0 ) 
      {
      DeltaArray *pDeltaArray = pModel->m_pDeltaArray;
      ASSERT (pDeltaArray);

      for (INT_PTR i=pDeltaArray->GetFirstUnapplied(pModel->m_pIDULayer)-1; i >= 0; i-- )
         {
         DELTA &delta = pDeltaArray->GetAt( i );
         //ASSERT( ! delta.oldValue.Compare( delta.newValue ) );

         if ((! delta.oldValue.Compare(delta.newValue)) && delta.col == m_columnIndex )
            {
            VData value;
            m_column.Get( 0, delta.cell, value );
            ASSERT( value.Compare( delta.newValue ) == true );
            m_column.Set( 0, delta.cell, delta.oldValue );
            }
         }
      }
   }

ColumnTrace::~ColumnTrace()
   {
   }

VData ColumnTrace::Get( int row )
   {
   ASSERT( 0 <= row && row < m_rows );
   VData data; 
   bool ok = m_column.Get( 0, row, data );
   ASSERT(ok);

   return data;
   }

bool ColumnTrace::SetCurrentYear( int year )
   {
   if ( year < 0 || m_maxYear < year )
      {
      ASSERT(0);
      return false;
      }

   if ( m_currentYear == year )
      return true;

   INT_PTR from, to;

   if ( m_currentYear < year )
      {
      m_pDeltaArray->GetIndexRangeFromYearRange( m_currentYear, year, from, to );

      for ( INT_PTR i=from; i<to; i++ )
		   {
         const DELTA &delta = m_pDeltaArray->GetAt(i);
         //ASSERT( ! delta.oldValue.Compare( delta.newValue ) );

         if ( (! delta.oldValue.Compare(delta.newValue)) && delta.col == m_columnIndex )
            {
            VData value;
            m_column.Get( 0, delta.cell, value );
            ASSERT( value.Compare( delta.oldValue ) == true );
            value = delta.newValue;
            m_column.Set( 0, delta.cell, value );
            }
         }
      }
   else // m_currentYear > year 
      {
      m_pDeltaArray->GetIndexRangeFromYearRange( m_currentYear, year, from, to );

      for ( INT_PTR i=from; i>to; i-- )
         {
         const DELTA &delta = m_pDeltaArray->GetAt( i );
         //ASSERT( ! delta.oldValue.Compare( delta.newValue ) );

         if ( (! delta.oldValue.Compare(delta.newValue)) && delta.col == m_columnIndex )
            {
            VData value;
            m_column.Get( 0, delta.cell, value );
            ASSERT( value.Compare( delta.newValue ) == true );
            value = delta.oldValue;
            m_column.Set( 0, delta.cell, value );
            }
         }
      }

   m_currentYear = year;

   return true;
   }
