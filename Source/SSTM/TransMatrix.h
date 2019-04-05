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

#include <MapLayer.h>
#include <randgen\randunif.hpp>
#include <VDataObj.h>
#include <DeltaArray.h>
#include <LulcTree.h>
#include <EnvContext.h>

class SimpleSTM;


class MapCell
   {
   public:
      MapCell() : m_cell( -1 ), m_years( -1 ) {}
      MapCell( int cell, int years ) : m_cell( cell ), m_years( years ) {}
      MapCell( const MapCell &m ){ *this = m; }
      
      MapCell& operator=( const MapCell& m )
         {
         m_cell  = m.m_cell;
         m_years = m.m_years;
         return *this;
         }

   public:
      int m_cell;  // same as IDU recno.
      int m_years; // incremented each simulation year
   };

//--------------------------------------------------------------------------------------
// A TransCell contains information about a single transition in the Transition Matrix
//--------------------------------------------------------------------------------------

class TransCell
   {
   public:
      TransCell()
         : /*m_colLulc( -1 ),*/ m_dormantYears(-1), m_percentChance(-1), m_toRow( -1 ), m_allowScaling( false ), m_dormantScalar( 1.0f ), m_pcScalar( 1.0f ) {}
      TransCell( int dormantYears, int percentChance, int toRow, bool allowScaling ) 
         : /*m_colLulc( -1 ),*/ m_dormantYears(dormantYears), m_percentChance(percentChance), m_toRow( toRow ), m_allowScaling( allowScaling ), m_dormantScalar( 1.0f ), m_pcScalar( 1.0f ) {}

      TransCell( const TransCell &tc ){ *this = tc; }

      TransCell& operator=( const TransCell &tc )
         {
         //m_colLulc       = tc.m_colLulc;
         m_dormantYears  = tc.m_dormantYears;
         m_percentChance = tc.m_percentChance;
         m_toRow         = tc.m_toRow;
         m_allowScaling  = tc.m_allowScaling;
         m_dormantScalar = tc.m_dormantScalar;
         m_pcScalar      = tc.m_pcScalar;
         return *this;
         }

   public:
      //int   m_colLulc;       // column in database to use
      int   m_dormantYears;  // expected time in this LULC_C(state) before permitted to transition to next
      int   m_percentChance; // in [0,100]; used to prioritize multiple TransCells in a TransRow.
      int   m_toRow;         // index to the TransRow containg the subsequent LULC_C state
      bool  m_allowScaling;
      float m_dormantScalar;     // scale factor for dormancy period - normally, 1 (no scaling)
      float m_pcScalar;          // scale factor for percentChance - normally 1 (no scaling)
   };


class TransMatrix;


//-----------------------------------------------------------------------------------------------------------------------------------------
// a TransRow is a collection of TransCells, representing a row in the transition matrix describing
//   possible transitions for a LULC class (m_class). Each TransCell contains information for one transition.
//   The TransRow also contains a list of IDU's in this class and the period they've been in this class (contained in the MapCell list)
//-----------------------------------------------------------------------------------------------------------------------------------------

class TransRow
   {
   friend class TransMatrix;

   public:
      TransRow() 
         : m_class( -1 ), m_minYear( INT_MAX ), m_pTransMatrix( NULL ) {}
      TransRow( int lulcClass, TransMatrix *pTransMatrix ) 
         : m_class( lulcClass ), m_minYear( INT_MAX ), m_pTransMatrix( pTransMatrix ) {}
      TransRow( const TransRow& tr ){ *this = tr; }
      ~TransRow() {}

      TransRow& operator=( const TransRow& tr ); // Copy point to the same TransMatrix

   private:
      // m_row is a row of the LULC_C transition Matrix, namely it is ROW:m_class;
      CArray< TransCell, TransCell& > m_row; 

      // m_cells is list of MapCell(IDU) where LULC_C == m_class
      CList< MapCell, MapCell& > m_cells;

      TransMatrix *m_pTransMatrix;
      
      static RandUniform m_rand;

      int m_class;   // Lulc_C class
      int m_minYear; // minimum m_dormantYears over TransCells in m_row.

   private:
      void AddMapCell( int cell, int year );

   public:
      void AddTransCell( int dormantYears, int percentChance, int toRow, bool allowScaling );
      void AddMapCell( int cell );
      void RemoveMapCell( int cell );
      void Process( EnvContext * pEnv); //DeltaArray* pDeltaArray, const MapLayer *pMapLayer, int year, LulcTree *pLulcTree, CUIntArray *pTargetPolyArray = NULL );
      void AgeCells();

      int  GetClass(){ return m_class; }
      void ClearData();
   };

class TransMatrix
   {
   friend class TransRow;

   public:
      TransMatrix();

      ~TransMatrix(void);

      TransMatrix( const TransMatrix& tm ){ *this = tm; }
      TransMatrix& operator=( const TransMatrix& tm );
      
   private:
      CArray< TransRow, TransRow& > m_rows;

      SimpleSTM *m_pSSTModel;
      
      VDataObj *m_pDataObj;

      int m_colLulc;
      
      static const int SEED = 54321; // seed used during Initilize(.)

   public:
      //int  GetTransRowCount(){ return m_rows.GetCount(); }
      //TransRow& GetTransRow( int i ){ ASSERT( 0<=i && i<=m_rows.GetCount() ); return m_rows.GetAt(i); }

      void ClearData();

      bool LoadXml( EnvContext *pEnv, LPCSTR file, const MapLayer *pLayer );
      void Initialize( SimpleSTM *pModel, EnvContext *pEnv, const MapLayer *pMapLayer );
      void Update ( EnvContext *pEnv );
      void Process( EnvContext *pEnv );
   };
