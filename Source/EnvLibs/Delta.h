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
#include <Vdata.h>


// There are several species of Deltas: 
//
//    Normal 
//       col >= 0
//       [semantics of other fields as declared]
//
//    Subdivide 
//       col      == -1
//       cell     == parent cell id 
//       oldValue == (TYPE_NULL)
//       newValue == ptr to a PolyArray of child Poly's (TYPE_PTR)
//                   for each poly, only m_vertexArray and m_id are 
//                   guaranteed to be valid
//       type     == DT_SUBDIVIDE
//
//    Merge
//       col      == -1
//       cell     == child cell id
//       oldValue == (TYPE_NULL)
//       newValue == ptr to CDWordArray of parent index's (TYPE_PTR)
//       type     == DT_MERGE
//
//    IncrementCol
//       col      == col to set
//       cell     == -1
//       oldValue == (TYPE_NULL)
//       newValue == value to increment column by (can be positive or negative
//       type     == DT_INCREMENT_COL


/* 
   The call to DeltaArray::AddDelta() is not guaranteed to be successful; it will fail if 
   the cell (aka IDU or polygon) at issue has been made defunct by subdivision. 

   To ensure no errroneous return value from AddDelta(),  
   immediately after the exit of each function (process) that calls AddDelta(), 
   EnvModel::ApplyDeltaArray() must be called.  The reason is the call to MapLayer::Subdivide(),
   which makes a polygon defunct, does not occur until inside  EnvModel::ApplyDeltaArray(.).
   Also a function (process) that calls AddDelta() with a SUBDIVIDE DELTA cannot rely
   on querying the MapLayer about the defunct status of the polygon affected; 
   within a time-step the function must itself ensure that it does not subsequently 
   call AddDelta(.) on that IDU it just subdivided by AddDelta.  
   */

enum     // delta types (note: Models start at 1000, processes at 2000)
   { 
   DT_NULL          = -1,
   DT_POLICY        = -2,
   DT_SUBDIVIDE     = -4,
   DT_MERGE         = -5,
   DT_INCREMENT_COL = -6,
   DT_POP           = -7,
   DT_APPVAR        = -8,
   DT_MODEL         = 1000,
   DT_MODEL_END     = 1999,
   DT_PROCESS       = 2000,
   DT_PROCESS_END   = 2999
   };


class DELTA
   {
   public:
      DELTA()
         : cell( -1 ), col( -1 ), year( -1 ), oldValue(), newValue(), type( DT_NULL ) {}

      DELTA( int _cell, short _col, int _year, VData _oldValue, VData _newValue, short _type )
         : cell( _cell ), col( _col ), year( _year ), oldValue( _oldValue ), newValue( _newValue ), type( _type ) {}

      DELTA( DELTA &d ){ *this = d; }

      ~DELTA(){}

      DELTA& operator=( DELTA &d )
         {
         cell     = d.cell;
         col      = d.col;
         year     = d.year;
         oldValue = d.oldValue;
         newValue = d.newValue;
         type     = d.type;

         return *this;
         }

      bool operator==( DELTA &d )
         {
         if ( cell != d.cell ) return false;
         if ( col  != d.col ) return false;
         if ( year != d.year ) return false;
         if ( oldValue != d.oldValue ) return false;
         if ( newValue != d.newValue ) return false;
         if ( type     != d.type ) return false;

         return true;
         }


      //enum { 
      //   SUBDIVIDE     = -42, 
      //   MERGE         = -43,
      //   INCREMENT_COL = -44
      //   };

   public:
      int   cell;           // cell this applies to, except DT_INCREMENT_COL - contains increment value
      short type;           // see types above, or handle if provided by model
      short col;
      int   year;
      VData oldValue;
      VData newValue;
     
   public:  
      //LPCTSTR GetTypeStr() { return GetTypeStr( type ); }
      //static LPCTSTR GetTypeStr( short type );
   };
