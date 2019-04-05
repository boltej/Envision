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

#include "DataAggregator.h"



void DataAggregator::SetPolys( int *polys, int count )
   {
   if ( m_manageInternally && m_polyIndexArray != NULL )
      delete [] m_polyIndexArray;

   m_polyIndexArray = polys; 
   m_polyIndexCount = count; 

   m_manageInternally = false; 
   }



int DataAggregator::AddPolys( int *polys, int count )      // assumes local management, return total number of polys managed
   {
   int newCount = m_polyIndexCount + count;
   int *oldPolyIndexArray = m_polyIndexArray;

   m_polyIndexArray = new int[ newCount ];

   if ( oldPolyIndexArray != NULL )
      memcpy( m_polyIndexArray, oldPolyIndexArray, m_polyIndexCount*sizeof( int ) );

   memcpy( m_polyIndexArray+m_polyIndexCount, polys, count*sizeof( int ) );


   if ( m_manageInternally && oldPolyIndexArray != NULL )
      delete [] oldPolyIndexArray;

   m_polyIndexCount   = newCount;
   m_manageInternally = true;
   return newCount;
   }
 


bool DataAggregator::GetData( int col, VData &value, DAMETHOD method )
   {
   if ( m_pLayer == NULL )
      return false;

   if ( col < 0 || col >= m_pLayer->GetFieldCount() )
      return false;

   if ( m_polyIndexCount <= 0 || m_polyIndexArray == NULL )
      return false;

   // preliminaries
   switch( method )
      {
      case DAM_MEAN:
      case DAM_SUM:
         break;

      case DAM_FIRST:   // only need to look at the first value
         return m_pLayer->GetData( m_polyIndexArray[0], col, value );

      case DAM_AREAWTMEAN:
         if ( m_colArea <= 0 )
            return false;
         break;         // nothing required for these ones

      case DAM_MAJORITYAREA:
         if ( m_colArea <= 0 )
            return false;
         // fall through to next case
      case DAM_MAJORITYCOUNT:
         // set up majority stuff
         m_polyValueArray.Clear();
         m_valuePtrMap.RemoveAll();
         break;
      }

   VData _value = 0;
   float totalArea = 0;

   for ( int i=0; i < m_polyIndexCount; i++ )
      {
      int polyIndex = m_polyIndexArray[ i ];

      int polyValue = 0;
      if ( m_pLayer->GetData( polyIndex, col, polyValue ) == false )
         continue;

      switch( method )
         {
         case DAM_MEAN:
         case DAM_SUM:
            _value += polyValue;
            break;

         case DAM_AREAWTMEAN:
            {
            float area;
            m_pLayer->GetData( polyIndex, m_colArea, area );
            _value += ( polyValue * area );
            totalArea += area;
            }
            break;

         case DAM_MAJORITYAREA:
            {
            float area;
            m_pLayer->GetData( polyIndex, m_colArea, area );
            AddPolyValues( polyValue, area );
            }
            break;

         case DAM_MAJORITYCOUNT:
            AddPolyValues( polyValue, 0 );
            break;
         }
      }
  
   // done with polygons, do any final preocessing
   switch ( method )
      {
      case DAM_MEAN:
         _value /= m_polyIndexCount;
         break;

      case DAM_AREAWTMEAN:
         _value /= totalArea;
         break;

      case DAM_MAJORITYAREA:
         GetMajorityArea( _value );
         break;

      case DAM_MAJORITYCOUNT:
         GetMajorityCount( _value );
         break;
      }
      
   value = _value;

   return true;
   }



bool DataAggregator::AddPolyValues( VData value, float area )
   {
   // have we seen this one already?
   POLY_VALUES *pPV = NULL;

   if ( m_valuePtrMap.Lookup( value, pPV ) )   // already added?
      {
      ASSERT( pPV != NULL );

      pPV->area += area;
      pPV->count++;
      }
   else
      {
      pPV = new POLY_VALUES;
      pPV->area = area;
      pPV->count = 1;
      m_polyValueArray.Add( pPV );
      m_valuePtrMap.SetAt( value, pPV );
      }

   return true;
   }


bool DataAggregator::GetMajorityCount( VData &value )
   {
   POSITION pos = m_valuePtrMap.GetStartPosition();
   VData key;
   POLY_VALUES *pPV = NULL;
   int topCount = -1;

   while( pos != NULL )
      {
      m_valuePtrMap.GetNextAssoc( pos, key, pPV );
      
      if ( pPV->count > topCount )
         {
         value = key;
         topCount = pPV->count;
         }
      }

   return ( topCount > 0 );
   }

bool DataAggregator::GetMajorityCount( int &value )
   {
   VData _value;
   bool ok = GetMajorityCount( _value );

   if ( ok )
      ok = _value.GetAsInt( value );
   
   if ( ! ok )
      value = 0;

   return ok;
   }

bool DataAggregator::GetMajorityCount( float &value )
   {
   VData _value;
   bool ok = GetMajorityCount( _value );

   if ( ok )
      ok = _value.GetAsFloat( value );
   
   if ( ! ok )
      value = 0;

   return ok;
   }

bool DataAggregator::GetMajorityCount( bool &value )
   {
   VData _value;
   bool ok = GetMajorityCount( _value );

   if ( ok )
      ok = _value.GetAsBool( value );
   
   if ( ! ok )
      value = 0;

   return ok;
   }


bool DataAggregator::GetMajorityArea( VData &value )
   {
   POSITION pos = m_valuePtrMap.GetStartPosition();
   VData key;
   POLY_VALUES *pPV = NULL;
   float topArea = -1;

   while( pos != NULL )
      {
      m_valuePtrMap.GetNextAssoc( pos, key, pPV );
      
      if ( pPV->area > topArea )
         {
         value = key;
         topArea = pPV->area;
         }
      }

   return ( topArea > 0 );
   }


bool DataAggregator::GetMajorityArea( int &value )
   {
   VData _value;
   bool ok = GetMajorityArea( _value );

   if ( ok )
      ok = _value.GetAsInt( value );
   
   if ( ! ok )
      value = 0;

   return ok;
   }

bool DataAggregator::GetMajorityArea( float &value )
   {
   VData _value;
   bool ok = GetMajorityArea( _value );

   if ( ok )
      ok = _value.GetAsFloat( value );
   
   if ( ! ok )
      value = 0;

   return ok;
   }


bool DataAggregator::GetMajorityArea( bool &value )
   {
   VData _value;
   bool ok = GetMajorityArea( _value );

   if ( ok )
      ok = _value.GetAsBool( value );
   
   if ( ! ok )
      value = 0;

   return ok;
   }

