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
#pragma once

#ifndef NO_MFC
#include <afxtempl.h>
#endif
#include "Delta.h"
#include <Vdata.h>
#include <PtrArray.h>

const int SFU_END = -2;


class MapLayer;
class Poly;
class PolyArray;

//========================================================================================
// m_pMapLayer is needed because plug-ins incorporate DeltaArray.cpp and call AddDelta(.)
//

// DELTA_MAP provides a means by which a single delta array can be referenced by multiple map layers.
// The basic idea is that each map layer can be in it's own state relative to it current position in the delta array,
// so this structure keeps track of where in the delta array each maplayer is.

struct DELTA_MAP
   {
   const MapLayer *pMapLayer;
   INT_PTR firstUnapplied;  // marks the index of the first DELTA element that has not been applied to the 
                            // MapLayer (0 means all have been applied, -1 means no deltas in the array)

   DELTA_MAP() : pMapLayer( NULL ), firstUnapplied( -1 ) {}
   DELTA_MAP( const MapLayer *_pMapLayer, INT_PTR _firstUnapplied ) : pMapLayer( _pMapLayer ), firstUnapplied( _firstUnapplied )  { }
   DELTA_MAP( const DELTA_MAP& d ) : pMapLayer( d.pMapLayer ), firstUnapplied( d.firstUnapplied )  { }

   DELTA_MAP &operator = (const DELTA_MAP &d ) { pMapLayer = d.pMapLayer; firstUnapplied = d.firstUnapplied; return *this; }
   };


class DeltaMapArray : public CArray< DELTA_MAP, DELTA_MAP& >
   {
   };


class LIBSAPI DeltaArray
   {
   public:
      DeltaArray(const MapLayer * mapLayer, int baseYear );
      DeltaArray(const DeltaArray &d );
      ~DeltaArray(void);

      bool m_showDeltaMessages;
      static int m_deltaAllocationSize;

   private:
      int m_dynamicPolyCount;
      CArray< PolyArray*, PolyArray*> m_polyArrayList;

   protected:
      // Dynamic Array Stuff
      //static const int INITIALSIZE = 320000;
      //static const int GROWBY      = 320000;
      PtrArray< DELTA > m_arraySections;    // arrays of deltas, allocated as needed

      //DELTA* m_array;
      INT_PTR   m_sizeAllocated;
      INT_PTR   m_size;

      DeltaMapArray m_deltaMapArray;         // stores info for each MapLayer using this DeltaArray 

      int m_baseYear;                             // year defined as starting year (default=0). 
      CArray<INT_PTR,INT_PTR> m_yearIndexMap;     // ex: m_yearIndexMap[42] is the index of the first delta that occured in a year >= 42
      INT_PTR GetIndexFromYear( int year ) const; // returns index of first element after the beginning of the specified year

      INT_PTR GetSectionDeltaCount( INT_PTR section ) { if ( section == m_arraySections.GetSize()-1 ) return m_size % m_deltaAllocationSize; else return m_deltaAllocationSize; }  // returns size of section, between 0 and m_deltaAllocationSize

   public:
      void AddMapLayer(const MapLayer *pLayer, INT_PTR firstUnapplied = -1) { DELTA_MAP dm(pLayer, firstUnapplied); m_deltaMapArray.Add(dm); }

      const MapLayer *GetBaseLayer( ) { ASSERT( m_deltaMapArray.GetCount() > 0 );  return m_deltaMapArray[ 0 ].pMapLayer; }

      // Dynamic Array Stuff
      INT_PTR GetSize()  const { return m_size; }
      INT_PTR GetCount() const { return m_size; }
      void RemoveAt( INT_PTR index );
      //INT_PTR Compact();    // removes redundant deltas

      DELTA& GetAt( INT_PTR index ); 
      const DELTA& GetAt( INT_PTR index ) const; 

      DELTA& operator[]( INT_PTR index ){ return GetAt( index ); }
      const DELTA& operator[]( INT_PTR index ) const { return GetAt( index ); }
      void FreeExtra();

   public:
      // AddDelta(.) returns < 0 on error; and on success the index of the added delta
      // The only correct way to call this AddDelta is via EnvModel::AddDelta(.)
      INT_PTR AddDelta( int cell, short col, int year, VData newValue, short type );

   public:      
      void ClearDeltas( void );
      void SetBaseYear( int year ) { m_baseYear = year; }

      //--DeltaArray::GetFirstUnapplied/SetFirstUnapplied-----------------------------------------------------------
      //    Get or set the first unapplied marker of the delta array.  firstUnapplied is initialized to -1 and then
      //    set to 0 when the first delta is added to the array.  SetFirstUnapplied is called by ApplyDeltaArray and
      //    UnapplyDeltaArray.  
      //
      //    GET FIRST UNAPPLIED:
      //    --RETURN VALUES--
      //    no deltas exist in the delta array                    --> -1
      //    deltas have been added, but have not been applied     -->  0
      //    deltas have been applied up to index n-1              -->  n > 0
      //
      //    SET FIRST UNAPPLIED:
      //    --PARAMETERS--
      //    firstUnapplied = new value for m_firstUnapplied; if -2 (default), sets firstUnapplied to the size of the delta array
      //
      INT_PTR GetFirstUnapplied( const MapLayer *pLayer );
      void SetFirstUnapplied( const MapLayer *pLayer, INT_PTR firstUnapplied ); // use SFU_END to indicate all deltas applied.
      //------------------------------------------------------------------------------------------------------------
      
      void GetIndexRangeFromYear( int year, INT_PTR &fromIndexClosed, INT_PTR &toIndexOpen ) const;
      void GetIndexRangeFromYearRange( int fromYear, int toYear, INT_PTR &fromIndexClosed, INT_PTR &toIndexOpen ) const;
      int  GetMaxYear() const { return (int) m_yearIndexMap.GetCount() - 1; } 

      int  GetFirstYear() const { if ( m_size > 0 ) return GetAt( 0 ).year; return -1; }
      int  GetLastYear() const  { if ( m_size > 0 ) return GetAt( m_size-1 ).year; return -1; }

      int GetYears( CArray<int,int> &years );


      bool Save( FILE *fp );
      bool Load( LPCTSTR filename );
      bool Load( FILE *fp );
      
   // dynamic map stuff
   public:
      void AddSubdivideDelta( int year, int parentCell, PolyArray &polyArray); // polys passed are managed by DeltaArray.
      void AddMergeDelta( int year, CDWordArray parentList, Poly *pNewPoly ); // polys passed are managed by DeltaArray.
      void AddIncrColDelta( int year, short col, VData incrValue );     
   };

inline
DELTA& DeltaArray::GetAt( INT_PTR index )
   { 
   ASSERT( 0 <= index && index < m_size ); 

   INT_PTR section = index / m_deltaAllocationSize;

   DELTA *deltaArray = m_arraySections[ section ];
   ASSERT( deltaArray != NULL );

   index = index % m_deltaAllocationSize;

   return *(deltaArray+index);
   }

inline
const DELTA& DeltaArray::GetAt( INT_PTR index ) const
   {
   ASSERT( 0 <= index && index < m_size ); 
   
   INT_PTR section = index / m_deltaAllocationSize;

   DELTA *deltaArray = m_arraySections[ section ];
   ASSERT( deltaArray != NULL );

   index = index % m_deltaAllocationSize;

   return *(deltaArray+index);
   }

