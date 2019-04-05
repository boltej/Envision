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

#include <stdstring.h>
#include <PtrArray.h>

class QueryEngine;
class MapLayer;
class DeltaArray;
class Query;

enum FeatureType { FT_EVENT, FT_FIELD };
enum EventFlags { EF_DEFAULT=0, EF_CHANGE=1, EF_SPATIAL=2, EF_TEMPORAL=4 };

class FeatureClass
   {
   public:
      CString m_name;

      FeatureType m_type;

   public:
      FeatureClass( FeatureType type ) : m_type( type ) { }

      static FeatureClass* CreateFeatureClass( FeatureType type );

   };



class EventFeatureClass : public FeatureClass
   {
   // an EventFeature defines a spatiotemporal query that is applied to
   // the landscape space-time domain.  Queries may exist in either the spatial domain,
   // temporal domain, or both.
   //  A temporal query is satisified for an IDU in one of the following ways:
   //   1) the associated spatial query is satisfied throughout the window in time;
   //   2) the associated spatial query is satisfied at any point in time
   //   
   // Note that for spatiotemporal queries, the query language provides
   // a new temporal function:  delta( {field}, {start}, {end}, {threshold}).
   // This function examine a window of field values, looking for changes
   // over the period that exceed the specified threshold
   public:
      CString m_query;

      int m_eventFlags;


   protected:
      Query *m_pQuery;

      int m_startPeriod;
      int m_endPeriod;

      int m_windowStartOffset;
      int m_windowEndOffset;
 
   public:
      EventFeatureClass() : FeatureClass( FT_EVENT ), m_pQuery( NULL ), m_startPeriod(-1), m_endPeriod( -1 ), m_windowStartOffset(0), m_windowEndOffset( 1 ) {} 

      bool SetQuery( PCTSTR queryStr, int startPeriod, int endPeriod, int windowStartOffset, int windowEndOffset )
         {
         return false;
         }
   
   };


class EventFeature
   {
   public:
      int m_idu;
      EventFeatureClass *m_pFeatureClass;
   };



class Analytics
   {
   public:
      Analytics();
      ~Analytics();

      bool LoadXml( PCTSTR xmlFile );

      int AddFeatureClass( FeatureClass *pFeature ) { return (int) m_featureClassArray.Add( pFeature ); }


   // feature cube 1- contains a three dimensional representation of
   // an Envision run - dimensions are time X iduOffset X field, represented using 
   // a 3D array of floats, where each value is the value of the field at that point.
   // a subset of fields 

   // feature cube 2 - idu map X time, with filters for 1) spatiotemporal queries,
   // 2) ???.  To simplify, the map is represented by centroid points, and visualized as 
   // an irregular 3d cube.  Color coding and point size is used to add dimensionality to
   // the 
    



   protected:
      



   // correlation analysis
   protected:
      PtrArray< FeatureClass > m_featureClassArray;
      

   public:
      bool SpatioTemporalCorrelation( MapLayer*, DeltaArray* );
   };
