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

#ifndef NO_MFC
#include <afxmt.h>
#endif

#include "DeltaArray.h"
#include <Report.h>
#include <Maplayer.h>


int DeltaArray::m_deltaAllocationSize = 32000;   // default


DeltaArray::DeltaArray(const MapLayer * ml, int baseYear )
:  m_showDeltaMessages( true )
,  m_dynamicPolyCount( -1 )
,  m_baseYear( baseYear )
,  m_arraySections( false )
   {
   //m_array = new DELTA[ m_deltaAllocationSize ];
   m_arraySections.Add( new DELTA[ m_deltaAllocationSize ] );
   m_sizeAllocated =  m_deltaAllocationSize;
   m_size = 0;

   AddMapLayer( ml, -1 );
   }

DeltaArray::DeltaArray(const  DeltaArray &d )
:  m_showDeltaMessages( d.m_showDeltaMessages )
,  m_dynamicPolyCount( d.m_dynamicPolyCount )
,  m_baseYear( d.m_baseYear )
   {
   m_size  = m_sizeAllocated = d.GetSize();

   for ( int i=0; i < d.m_arraySections.GetSize(); i++ )
      {
      DELTA *deltaArray = new DELTA[ m_deltaAllocationSize ];
      memcpy( (void*) deltaArray, (const void*) d.m_arraySections[ i ], m_deltaAllocationSize*sizeof( DELTA ) );
      m_arraySections.Add( deltaArray );
      }

   for ( int i=0; i < d.m_deltaMapArray.GetCount(); i++ )
      AddMapLayer( d.m_deltaMapArray[ i ].pMapLayer, d.m_deltaMapArray[ i ].firstUnapplied );
   }


DeltaArray::~DeltaArray(void)
   {
   ClearDeltas();
   }



INT_PTR DeltaArray::GetFirstUnapplied( const MapLayer *pLayer )
   {
   for ( int i=0; i < m_deltaMapArray.GetCount(); i++ )
      {
      if ( pLayer == m_deltaMapArray[ i ].pMapLayer )
         return m_deltaMapArray[ i ].firstUnapplied;
      }

   ASSERT( 0 );
   return -1;
   }


void DeltaArray::SetFirstUnapplied( const MapLayer *pLayer, INT_PTR firstUnapplied  )
   {
   INT_PTR  _firstUnapplied = 0;
   if ( firstUnapplied != SFU_END ) 
      _firstUnapplied = firstUnapplied;
   else
      _firstUnapplied = GetCount();

   for ( int i=0; i < m_deltaMapArray.GetCount(); i++ )
      {
      if ( pLayer == m_deltaMapArray[ i ].pMapLayer )
         {
         m_deltaMapArray[ i ].firstUnapplied = _firstUnapplied;
         return;
         }
      }

   ASSERT( 0 );
   }


void DeltaArray::ClearDeltas()
   {
   for ( INT_PTR i=0; i < m_arraySections.GetSize(); i++ )
      {
      DELTA *deltaArray = m_arraySections.GetAt( i );
      delete [] deltaArray;
      }

   m_arraySections.RemoveAll();
   
   m_size = m_sizeAllocated = 0;

   for ( int i=0; i<m_polyArrayList.GetCount(); i++ )
      {
      PolyArray *pArray = m_polyArrayList.GetAt(i);

      if ( pArray != NULL )
         {
         for ( int j=0; j<pArray->GetCount(); j++ )
            delete pArray->GetAt(j);

         delete pArray;
         }
      }

   m_yearIndexMap.RemoveAll();

   m_deltaMapArray.RemoveAll();
   // m_firstUnapplied = -1; jpb...
   }


void DeltaArray::RemoveAt( INT_PTR index )
   {/*
   ASSERT( 0 <= index && index < m_size );

   INT_PTR section = index / m_deltaAllocationSize;

   DELTA *deltaArray = m_arraySections[ section ];

   for ( INT_PTR i = index; i < m_size-1; i++ )  note m_size-1 no good
      memcpy( deltaArray + i, deltaArray + i + 1, sizeof( DELTA ) );

   m_size--; */
   }



void DeltaArray::FreeExtra()
   {
   if ( m_size < m_sizeAllocated )
      {
      /*
      m_sizeAllocated = m_size;
      DELTA* temp = m_array;
      m_array = new DELTA[ m_sizeAllocated ];
      memcpy( (void*)m_array,(const void*)temp, m_size*sizeof( DELTA ) );
      delete [] temp;*/
      }
   }


/*

INT_PTR DeltaArray::Compact()
   {
   // NOTE:  This function doesn't take care redundant polygons from subdividing
   //ASSERT( m_firstUnapplied <= 0 );

   DELTA *temp = new DELTA[ m_size ];
   int index = 0;

   for ( INT_PTR i=0; i < GetSize(); i++ )
      {
      DELTA &delta = GetAt( i );

      if ( delta.newValue.Compare( delta.oldValue ) == false )
         {
         temp[ index ] = delta;
         index++;
         }
      else
         {
         //LPCTSTR field = m_pMapLayer->GetFieldLabel( delta.col );
         //CString oldValStr, newValStr;
         //delta.oldValue.GetAsString( oldValStr );
         //delta.newValue.GetAsString( newValStr );

         //CString msg;
         //msg.Format( "Redundant delta removed: Col: %s, OldValue: %s, NewValue: %s\n", field, (LPCTSTR) oldValStr, (LPCTSTR) newValStr );
         //TRACE( (LPCTSTR) msg ); 
         }
      }

   memcpy( (void*) m_array, (const void*) temp, index*sizeof( DELTA ) );
   m_size = index;

   delete [] temp;

   return m_size;
   }
   */

//--DeltaArray::AddDelta-------------------------------------------------------------------------
//    Adds deltas to delta array and deals with dynamic sizing - this function is private and should
//    only be called via EnvModel::AddDelta
//    NOTE: DELTA.oldValue is set in EnvModel::ApplyDeltaArray rather than accessing the map layer 
//       while adding deltas
//
//    --PARAMETERS--
//    cell     = map cell (IDU)
//    col      = column in map database to be changed
//    year     = year in which change occurred
//    newValue = VData object containing value to be applied
//    type     = type of delta indicates where delta originated
//
//    --RETURN VALUES--
//    error, delta was not added    -->  -1
//    redundant delta, not added    -->  -2
//    no issues, delta was added    -->  index of added delta in the delta array
//
//  NOTE: THIS SHOULD ONLY BE CALLED IN REFERENCE TO THE BASE LAYER!!!!!
#ifndef NO_MFC
CCriticalSection gCriticalSection4DeltaArray;
#endif
//-----------------------------------------------------------------------------------------------
INT_PTR DeltaArray::AddDelta( int cell, short col, int year, VData newValue, short type )
   {
   const MapLayer *pBaseLayer = GetBaseLayer();

   if ( pBaseLayer->IsDefunct(cell) )
      return -1;

   //VData oldValue;
   //pBaseLayer->GetData( cell, col, oldValue );
   //if ( newValue == oldValue )  // note: this fails when there are unprocessed deltas for this idu/column
   //   return -2;
#ifndef NO_MFC
   CSingleLock lock( & gCriticalSection4DeltaArray );
   lock.Lock();
#endif

   if ( m_size == m_sizeAllocated )
      {
      m_arraySections.Add( new DELTA[ m_deltaAllocationSize ] );
      m_sizeAllocated += m_deltaAllocationSize;
      }

   m_size++;
   DELTA &newDelta = GetAt( m_size-1 );

   newDelta.cell     = cell;
   newDelta.col      = col;
   newDelta.year     = year;
   newDelta.oldValue = VData();  // NULL until this delta is applied
   newDelta.newValue = newValue;
   newDelta.type     = type;
   
   // do we need to add one or more new years to the map?  Note: this assumes index map is zero-based
   // so we need to substract the base year
   int lastYearInMap = int( m_yearIndexMap.GetCount()) - 1 + m_baseYear;

   if ( lastYearInMap < year )
      {
      int y = -1;
      while ( y < ( year-m_baseYear ) )
         y = (int) m_yearIndexMap.Add( m_size-1 );  // Note: Add() returns index of added element
      ASSERT( y == ( year-m_baseYear ) );
      }

   if( m_deltaMapArray[ 0 ].firstUnapplied < 0 )
      m_deltaMapArray[ 0 ].firstUnapplied = 0;

#ifndef NO_MFC
   lock.Unlock();
#endif
   return m_size-1;
   }


void DeltaArray::AddSubdivideDelta( int year, int parentCell, PolyArray &polyArray )
   {
   const MapLayer *pBaseLayer = GetBaseLayer();

   if ( m_dynamicPolyCount < 0 )
      m_dynamicPolyCount = pBaseLayer->GetRecordCount( MapLayer::ALL );

   if ( m_size == m_sizeAllocated )
      {
      m_arraySections.Add( new DELTA[ m_deltaAllocationSize ] );
      m_sizeAllocated += m_deltaAllocationSize;
      }

   PolyArray *pArray = new PolyArray( polyArray ); // Shallow Copy
   m_polyArrayList.Add( pArray );
   
   for ( int i=0; i < pArray->GetCount(); i++ )
      {
      pArray->GetAt(i)->m_id = m_dynamicPolyCount;
      m_dynamicPolyCount++;
      }

   DELTA &newDelta = GetAt( m_size );

   newDelta.cell     = parentCell;
   newDelta.col      = -1;  //= DELTA::SUBDIVIDE;
   newDelta.year     = year;
   newDelta.oldValue = VData();
   newDelta.newValue = (void*) pArray;
   newDelta.type     = DT_SUBDIVIDE;
   
   // do we need to add one or more new years to the map?  Note: this assumes years are
   // zero-based
   int lastYearInMap = int( m_yearIndexMap.GetCount()) - 1;

   if ( lastYearInMap < ( year-m_baseYear ) )
      {
      int y = -1;
      while ( y < ( year-m_baseYear ) )
         y = (int) m_yearIndexMap.Add( m_size );  // Note: Add() returns index of added element
      ASSERT( y == ( year-m_baseYear ) );
      }

   if ( m_deltaMapArray[ 0 ].firstUnapplied < 0 )
      m_deltaMapArray[ 0 ].firstUnapplied = 0 ;

   m_size++;
   }


void DeltaArray::AddMergeDelta( int year, CDWordArray parentList, Poly *pNewPoly )
   {
   // TODO: implement

   //if ( m_firstUnapplied < 0 )
   //   m_firstUnapplied = 0;
   }


void DeltaArray::AddIncrColDelta( int year, short col, VData incrValue )
   {
   INT_PTR index = AddDelta( -1, col, year, incrValue, DT_INCREMENT_COL );
   // note: old value, new value not yet set.  Delta.cell contains the increment value
   }


INT_PTR DeltaArray::GetIndexFromYear( int year ) const
   {
   ASSERT( ( m_baseYear <= year ) && ((year-m_baseYear) < m_yearIndexMap.GetCount()) );
   return m_yearIndexMap[ year-m_baseYear ];
   }

// -- void DeltaArray::GetIndexRangeFromYear() ----------------------------------------------
//
// Use this to get all deltas for given year
//
// deltas will be in the range [fromIndexClosed, toIndexOpen)
// i.e. fromIndexClosed is the index of the first delta that occured in the year
//      toIndexOpen is the index of the first index that occured in the next year
//
// ------------------------------------------------------------------------------------------
void DeltaArray::GetIndexRangeFromYear( int year, INT_PTR &fromIndexClosed, INT_PTR &toIndexOpen ) const
   {
   GetIndexRangeFromYearRange( year, year+1, fromIndexClosed, toIndexOpen );
   }

// -- DeltaArray::GetIndexRangeFromYearRange ------------------------------------------------
//
//  Use this to get all deltas that occured inbetween the beginning of years 
//
//  ranges are to be interpreted as "from closed to open"
//  year i <==> beginning of year i
//  
//  if fromYear <= toYear
//     [ fromIndexClosed, toIndexOpen )
//  if fromYear > toYear
//     ( toIndexOpen, fromIndexClosed ]
// 
// ------------------------------------------------------------------------------------------
void DeltaArray::GetIndexRangeFromYearRange( int fromYear, int toYear, INT_PTR &fromIndexClosed, INT_PTR &toIndexOpen ) const
   {
   if ( GetSize() == 0 )
      {
      fromIndexClosed = toIndexOpen = 0;
      return;
      }

   int count = (int) GetSize();
   int endYear = (int) GetAt( count-1 ).year + 1;   // actual year (not necessarily 0-based, depending on m_baseYear)
   //int endYear = m_yearIndexMap.GetSize();

   ASSERT( m_baseYear <= fromYear );
   ASSERT( m_baseYear <= toYear   );

   if ( fromYear >= endYear )
      fromIndexClosed = GetCount();
   else
      fromIndexClosed = GetIndexFromYear( fromYear );

   if ( toYear >= endYear )
      toIndexOpen = GetCount();
   else
      toIndexOpen = GetIndexFromYear( toYear );

   if ( toYear < fromYear )
      {
      fromIndexClosed--;
      toIndexOpen--;
      }
   }

bool DeltaArray::Save( FILE *fp )
   {
   ASSERT( fp );

   // Save Version Number
   int version = 1;
   if ( fwrite( &version, sizeof( version ), 1, fp ) != 1 )
      return false;

   // Save Deltas
   if ( fwrite( &m_size, sizeof( m_size ), 1, fp ) != 1 )
      return false;

   for ( INT_PTR i=0; i < m_arraySections.GetSize(); i++ )
      {
      DELTA *deltaArray = m_arraySections[ i ];

      INT_PTR sectionCount = GetSectionDeltaCount( i );
      
      if ( fwrite( deltaArray, sizeof( DELTA ), sectionCount, fp ) != sectionCount )
         return false;
      }

   // Save Year-Index Map
   if ( fwrite( &m_baseYear, sizeof( m_baseYear ), 1, fp ) != 1 )
      return false;
   int yiSize = (int) m_yearIndexMap.GetSize();
   if ( fwrite( &yiSize, sizeof( yiSize ), 1, fp ) != 1 )
      return false;
   if ( fwrite( m_yearIndexMap.GetData(), sizeof( int ), yiSize, fp ) != yiSize )
      return false;

   //------------------------------------------------------
   // SAVE SUBDIVIDED POLYGONS
   // save count of poly arrays
   int numArrays = (int) m_polyArrayList.GetCount();
   if (fwrite(&numArrays, sizeof(int), 1, fp) != 1)
      return false;

   // loop through poly arrays
   for (int pa=0; pa < numArrays; pa++)
      {   
      PolyArray * pPolyArray = m_polyArrayList.GetAt(pa);
      
      // save pointer address to poly array for reconnecting to deltas when loading
      if (fwrite(&pPolyArray, sizeof(Poly *), 1, fp) != 1)
         return false;

      // save count of polys in array
      int numPolys = (int) pPolyArray->GetCount();
      if (fwrite(&numPolys, sizeof(int), 1, fp) != 1)
         return false;

      // loop through polygons in array
      for (int p=0; p < numPolys; p++)
         {
         Poly * pPoly = pPolyArray->GetAt(p);
         
         // save individual parts of polygon in order of definition
         //----------------------------------------------------------
         // m_vertexArray and m_vertexPartArray
         int partCount = (int) pPoly->m_vertexArray.GetSize();
         if (fwrite(&partCount, sizeof(int), 1, fp) != 1)
            return false;
         if (fwrite(pPoly->m_vertexArray.GetData(), sizeof(Vertex), partCount, fp) != partCount)
            return false;
         
         partCount = (int) pPoly->m_vertexPartArray.GetSize();      
         if (fwrite(&partCount, sizeof(int), 1, fp) != 1)
            return false;
         if (fwrite(pPoly->m_vertexPartArray.GetData(), sizeof(DWORD), partCount, fp) != partCount)
            return false;
         
         // bounding box
         if (fwrite(&pPoly->m_xMax, sizeof(float), 1, fp) != 1)
            return false;
         if (fwrite(&pPoly->m_xMin, sizeof(float), 1, fp) != 1)
            return false;
         if (fwrite(&pPoly->m_yMax, sizeof(float), 1, fp) != 1)
            return false;
         if (fwrite(&pPoly->m_yMin, sizeof(float), 1, fp) != 1)
            return false;
         
         // cell ID
         if (fwrite(&pPoly->m_id, sizeof(int), 1, fp) != 1)
            return false;
         
         /*  NOT PERSISTED - THIS CODE DOES NOT WORK AS OF YET
         // pointer to Bin
         if (fwrite((void *) pPoly->m_pBin, sizeof(Bin), 1, fp) != 1)
            return false;
         
         // point array - number of points
         partCount = pPoly->GetVertexCount();
         if (fwrite(&partCount, sizeof(int), 1, fp) != 1)
            return false;
         if (fwrite((void *) pPoly->m_ptArray, sizeof(POINT), partCount, fp) != partCount)
            return false;*/
         
         // m_extra
         if (fwrite(&pPoly->m_extra, sizeof(long), 1, fp) != 1)
            return false;
   
         // parent and child arrays
         partCount = (int) pPoly->GetParents().GetSize();      
         int test = (int) pPoly->GetParents().GetCount();
         
         if (fwrite(&partCount, sizeof(int), 1, fp) != 1)
            return false;
         if (fwrite(pPoly->GetParents().GetData(), sizeof(UINT), partCount, fp) != partCount)
            return false;
         
         partCount = (int) pPoly->GetChildren().GetSize();      
         if (fwrite(&partCount, sizeof(int), 1, fp) != 1)
            return false;
         if (fwrite(pPoly->GetChildren().GetData(), sizeof(UINT), partCount, fp) != partCount)
            return false;
         }
      }
   //------------------------------------------------------

   return true;
   }


bool DeltaArray::Load( LPCTSTR filename )
   {
   FILE *fp;
   fopen_s( &fp, filename, "rb" );

   if ( fp == NULL )
      return false;

   bool ok = Load( fp );

   fclose( fp );

   return ok;
   }


bool DeltaArray::Load( FILE *fp )
   {
   ASSERT( fp );

   ClearDeltas();

   // Load Version Number
   int version;
   if ( fread( (void*) &version, sizeof( version ), 1, fp ) != 1 )
      return false;
   if ( version != 1 )
      return false;

   // Load Deltas
   if ( fread( &m_size, sizeof( m_size ), 1, fp ) != 1 )
      return false;

   INT_PTR sections = m_size / m_deltaAllocationSize;

   m_sizeAllocated = m_size; 

   for ( INT_PTR i=0; i < sections; i++ ) 
      {
      DELTA *deltaArray = new DELTA[ m_deltaAllocationSize ]; 

      INT_PTR sectionCount = GetSectionDeltaCount( i );

      if ( fread( deltaArray, sizeof( DELTA ), sectionCount, fp ) != sectionCount )
         {
         ClearDeltas();
         return false;
         }
      else
         m_arraySections.Add( deltaArray );
      }

   // Load Year-Index Map
   int baseYear;
   if ( fread( &baseYear, sizeof( baseYear ), 1, fp ) != 1 )
      return false;
   int yiSize;
   if ( fread( &yiSize, sizeof( yiSize ), 1, fp ) != 1 )
      return false;
   m_baseYear = baseYear;
   m_yearIndexMap.SetSize( yiSize );

   if ( fread( m_yearIndexMap.GetData(), sizeof( int ), yiSize, fp ) != yiSize )
      return false;

   //------------------------------------------------------
   // LOAD SUBDIVIDED POLYGONS
   // load count of poly arrays and set m_polyArrayList size
   int numArrays;
   if (fread((void *) &numArrays, sizeof(int), 1, fp) != 1)
      return false;
   m_polyArrayList.SetSize(numArrays);

   // initialize counter for looping through delta array
   int deltaArrayIndex = -1;

   // loop through poly arrays
   for (int pa=0; pa < numArrays; pa++)
      {   
      // create new poly array
      PolyArray * pPolyArray = new PolyArray;
      m_polyArrayList.SetAt(pa, pPolyArray);
      
      // read old pointer address to poly array to match up with pointer address in delta array
      Poly * oldPointer;
      if (fread((void *) &oldPointer, sizeof(Poly *), 1, fp) != 1)
         return false;

      // search for next subdivide delta, match old pointer addresses and recreate newValue pointer link
      deltaArrayIndex++;
      DELTA &d = GetAt( deltaArrayIndex );

      while (d.type != DT_SUBDIVIDE ) // col != DELTA::SUBDIVIDE)
         deltaArrayIndex++;

      d = GetAt( deltaArrayIndex );
      ASSERT(d.newValue.val.vPtr == oldPointer);

      d.newValue = pPolyArray;

      // load count of polys in array and set size of poly array
      int numPolys;
      if (fread((void *) &numPolys, sizeof(int), 1, fp) != 1)
         return false;
      pPolyArray->SetSize (numPolys);

      // loop through polygons in array
      for (int p=0; p < numPolys; p++)
         {
         Poly * pPoly = new Poly;
         pPolyArray->SetAt(p, pPoly);
         
         // load individual parts of polygon in order of definition
         //----------------------------------------------------------
         // m_vertexArray and m_vertexPartArray
         int partCount;
         if (fread((void *) &partCount, sizeof(int), 1, fp) != 1)
            return false;
         pPoly->m_vertexArray.SetSize(partCount);
         if (fread(pPoly->m_vertexArray.GetData(), sizeof(Vertex), partCount, fp) != partCount)
            return false;
         
         if (fread(&partCount, sizeof(int), 1, fp) != 1)
            return false;
         pPoly->m_vertexPartArray.SetSize(partCount);
         if (fread(pPoly->m_vertexPartArray.GetData(), sizeof(DWORD), partCount, fp) != partCount)
            return false;
         
         // bounding box
         if (fread(&pPoly->m_xMax, sizeof(float), 1, fp) != 1)
            return false;
         if (fread(&pPoly->m_xMin, sizeof(float), 1, fp) != 1)
            return false;
         if (fread(&pPoly->m_yMax, sizeof(float), 1, fp) != 1)
            return false;
         if (fread(&pPoly->m_yMin, sizeof(float), 1, fp) != 1)
            return false;
         
         // cell ID
         if (fread(&pPoly->m_id, sizeof(int), 1, fp) != 1)
            return false;
         
         /*  NOT PERSISTING - THIS CODE DOES NOT WORK AS OF YET
         // pointer to Bin
         pPoly->m_pBin = new Bin;
         if (fread(pPoly->m_pBin, sizeof(Bin), 1, fp) != 1)
            return false; 
         
         // point array - number of points
         if (fread(&partCount, sizeof(int), 1, fp) != 1)
            return false;
         POINT * temp [partCount];
         if (fwrite(temp, sizeof(POINT), partCount, fp) != partCount)
            return false;
         pPoly->m_ptArray = temp;*/
         
         // m_extra
         if (fread(&pPoly->m_extra, sizeof(long), 1, fp) != 1)
            return false;
   
         // parent and child arrays - these arrays are not actually stored, troubleshoot MapLayer::Subdivide
         // work around = set parent from delta array
         if (fread(&partCount, sizeof(int), 1, fp) != 1)
            return false;
         pPoly->GetParents().SetSize(partCount);
         if (fread(pPoly->GetParents().GetData(), sizeof(UINT), partCount, fp) != partCount)
            return false;

         if (fread(&partCount, sizeof(int), 1, fp) != 1)
            return false;
         pPoly->GetChildren().SetSize(partCount);
         if (fread(pPoly->GetChildren().GetData(), sizeof(UINT), partCount, fp) != partCount)
            return false;

         DELTA &d = GetAt( deltaArrayIndex );
         pPoly->GetParents().Add(d.cell);
         }
      }
   //------------------------------------------------------
   
   return true;
   }


int DeltaArray::GetYears( CArray<int,int> &years )
   {
   years.RemoveAll();

   int yearCount = (int) m_yearIndexMap.GetSize();

   for ( int i=0; i < yearCount; i++ )
      {
      INT_PTR index = m_yearIndexMap[ i ];
      int year = GetAt( index ).year;
      years.Add( year );
      }
   
   return (int) years.GetSize();
   }
