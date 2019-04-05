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
#include "StdAfx.h"
#pragma hdrstop

#include ".\transmatrix.h"
#include <EnvAPI.h>

#include <Report.h>
#include <DbTable.h>
#include <tinyxml.h>

#include "SSTM.h"


////////////////////////////////////////////////////////////////
// TransRow

RandUniform TransRow::m_rand( 0.0, 100.0, 42 );

TransRow& TransRow::operator=( const TransRow& tr )
   {
   m_minYear = tr.m_minYear;
   m_class = tr.m_class;
   m_pTransMatrix = tr.m_pTransMatrix;

   INT_PTR size = tr.m_row.GetSize();
   m_row.SetSize( size );
   TransCell *pTransCell = m_row.GetData();
   for ( INT_PTR i=0; i<size; i++, pTransCell++ )
      *pTransCell = tr.m_row.GetAt(i);

   m_cells.RemoveAll();
   POSITION pos = tr.m_cells.GetHeadPosition();
   while ( pos != NULL )
      {
      // note: the following code is redundant.
      //       I have to call the copy constructor twice
      //       because AddTail does not take a const reference.
      MapCell mapCell( tr.m_cells.GetNext( pos ) );
      m_cells.AddTail( mapCell );
      }

   return *this;
   }


void TransRow::AddTransCell( int dormantYears, int percentChance, int toRow, bool allowScaling )
   {
   if ( dormantYears < m_minYear )
      m_minYear = dormantYears;

   m_row.Add( TransCell( dormantYears, percentChance, toRow, allowScaling ) );
   }


void TransRow::AddMapCell( int cell )
   {
   AddMapCell( cell, -1 );
   }


void TransRow::AddMapCell( int cell, int year )
   {
   #ifdef _DEBUG
      POSITION pos = m_cells.GetHeadPosition();  // change to O(log(n))
      while ( pos != NULL )
         {
         MapCell &mapCell = m_cells.GetNext( pos );
         if (  mapCell.m_cell == cell )
            return;
         }
   #endif

   m_cells.AddTail( MapCell( cell, year ) );
   }


void TransRow::RemoveMapCell( int cell )
   {
   POSITION pos = m_cells.GetHeadPosition();  // O(n) ; change to O(log(n))
   while ( pos != NULL )
      {
      MapCell &mapCell = m_cells.GetAt( pos );
      if ( mapCell.m_cell == cell )
         {
         m_cells.RemoveAt( pos );
         return;
         }
      m_cells.GetNext( pos );
      }

   //ASSERT( 0 );  // could be removed by other means
   }


void TransRow::ClearData()
   {
   m_cells.RemoveAll();
   }


void TransRow::Process( EnvContext * pEnv )
   {
   DeltaArray* pDeltaArray = pEnv->pDeltaArray;
   const MapLayer *pMapLayer = pEnv->pMapLayer;
   LulcTree *pLulcTree = pEnv->pLulcTree;
   bool *targetPolyArray = pEnv->targetPolyArray; // /*= NULL*/ )

   int transCellCount = (int) m_row.GetCount();    // number of transitions in this row
   POSITION pos = m_cells.GetHeadPosition();
   POSITION currentPos = NULL;

   //int lulcLevels = pLulcTree->GetLevels();

   // iterate through each IDU attached to this TransRow
   while ( pos != NULL )
      {
      currentPos = pos;
      MapCell &mapCell = m_cells.GetNext( pos );

      // only Process cells that are in pTargetPolyArray, if it exists.
      if ( targetPolyArray && targetPolyArray[ mapCell.m_cell ] == false )
         continue;
         
      if ( mapCell.m_years > m_minYear )
         {
         int randVal = (int) m_rand.RandValue();
         int valueSoFar = 0;

         for ( int i=0; i < transCellCount; i++ )
            {
            TransCell &transCell = m_row.GetAt(i);

            int dormantYears = transCell.m_dormantYears;
            if ( transCell.m_allowScaling )
               dormantYears = int( dormantYears * transCell.m_dormantScalar );

            int percentChance = transCell.m_percentChance;
            if ( transCell.m_allowScaling )
               percentChance = int( percentChance * transCell.m_pcScalar );

            if ( dormantYears <= mapCell.m_years )
               valueSoFar += percentChance;

            if ( randVal < valueSoFar )
               {
               // make this transition
               TransRow &toTransRow = m_pTransMatrix->m_rows.GetAt( transCell.m_toRow );
               
               int lulcOld = GetClass();
               int lulcNew = toTransRow.GetClass();
               //ASSERT( lulcOld != lulcNew );

               int lulcCol = m_pTransMatrix->m_colLulc;
               //switch( lulcLevels )
               //   {
               //   case 3:  lulcCol = m_pTransMatrix->m_colLulcC;  break;
               //   case 2:  lulcCol = m_pTransMatrix->m_colLulcB;  break;
               //   case 1:  lulcCol = m_pTransMatrix->m_colLulcB;  break;
               //   };

               ASSERT( lulcCol >= 0 );
               this->m_pTransMatrix->m_pSSTModel->UpdateIDU( pEnv, mapCell.m_cell, lulcCol, lulcNew, ADD_DELTA  );

               toTransRow.AddMapCell( mapCell.m_cell );
               m_cells.RemoveAt( currentPos );
               break;
               } // end if ( randVal < valueSoFar )
            } // end for ( i<transCellCount )
         } // end if ( mapCell.m_years > m_minYear )
      } // end while ( pos != NULL )
   }

void TransRow::AgeCells()
   {
   POSITION pos = m_cells.GetHeadPosition();

   while ( pos != NULL )
      {
      MapCell &mapCell = m_cells.GetAt( pos );
      mapCell.m_years++;
      m_cells.GetNext( pos );
      }
   }

////////////////////////////////////////////////////////////////
// TransMatrix
TransMatrix::TransMatrix()
:  m_pDataObj( nullptr),
   m_pSSTModel( nullptr ),
   //m_pTableViewer( NULL ),
   m_colLulc( -1 )
   //m_colLulcA( -1 ),
   //m_colLulcB( -1 ),
   //m_colLulcC( -1 )
   {
   }

TransMatrix::~TransMatrix(void)
   {
   if ( m_pDataObj )
      delete m_pDataObj;

   //if ( m_pTableViewer )
   //   delete m_pTableViewer;
   }

TransMatrix& TransMatrix::operator=( const TransMatrix& tm )
   {
   m_pDataObj = NULL;

   m_pSSTModel = tm.m_pSSTModel;

   //m_pTableViewer = NULL;

   m_colLulc = tm.m_colLulc;
   //m_colLulcB = tm.m_colLulcB;
   //m_colLulcA = tm.m_colLulcA;

   INT_PTR size = tm.m_rows.GetSize();
   m_rows.SetSize( size );
   TransRow *pTransRow = m_rows.GetData();
   for ( INT_PTR i=0; i < size; i++, pTransRow++ )
      {
      *pTransRow = tm.m_rows.GetAt(i);  // the copied row points to tm not *this
      pTransRow->m_pTransMatrix = this;
      }

   return *this;
   }



void TransMatrix::Initialize( SimpleSTM *pModel, EnvContext *pEnvContext, const MapLayer *pMapLayer )
   {
   m_pSSTModel = pModel;

   ClearData();

   int rowCount = (int) m_rows.GetCount();
   int lulc;
   RandUniform randUnif;
   randUnif.SetSeed( SEED );

   for ( MapLayer::Iterator cell = pMapLayer->Begin(); cell != pMapLayer->End(); cell++ )
      {
      pMapLayer->GetData( cell, m_colLulc, lulc );

      for ( int row=0; row < rowCount; row++ )
         {
         TransRow &transRow = m_rows.GetAt(row);
         if ( transRow.GetClass() == lulc )
            {
            int year = int( randUnif.GetUnif01()*transRow.m_minYear );
            transRow.AddMapCell( cell, year );
            }
         }
      }
   }


bool TransMatrix::LoadXml( EnvContext *pContext, LPCTSTR file, const MapLayer *pLayer )
   {
   // have xml string, start parsing
   TiXmlDocument doc;
   bool ok = doc.LoadFile( file );

   bool loadSuccess = true;

   if ( ! ok )
      {
      CString msg;
      msg.Format("SSTM: Error reading TransMatrix input file, %s", file);
      AfxGetMainWnd()->MessageBox( doc.ErrorDesc(), msg );
      return false;
      }

   // Note: Files are of the form
   // <transitions>
   //    <trans start="lulc_c" end="lulc_c" period="period" prob="probability" (optionally) allowScaling="[0|1]"/>
   // </transitions>
 
   // start interating through the nodes
   TiXmlElement *pXmlRoot = doc.RootElement();
   
   // column specified?
   m_colLulc = -1;
   LPCTSTR strCol = pXmlRoot->Attribute( "col" );
   if ( strCol == NULL )   // col not defined, get default
      {
      // assume bottom of heirarchy
      int level = pContext->pLulcTree->GetLevels();

      CString fieldName( pContext->pLulcTree->GetFieldName( level ) );
      m_colLulc = pContext->pMapLayer->GetFieldCol( fieldName );
      }
   else
      m_colLulc = pLayer->GetFieldCol( strCol );
      
   if ( m_colLulc < 0 )
      {
      CString msg;
      msg.Format( "SSTM: Column '%s' not found in IDU coverage", strCol );
      Report::ErrorMsg( msg );
      return false;
      }

   // iterate through 
   TiXmlNode *pXmlTransNode = NULL;
   while( ( pXmlTransNode = pXmlRoot->IterateChildren( pXmlTransNode ) ) != NULL )
      {
      if ( pXmlTransNode->Type() != TiXmlNode::ELEMENT )
            continue;

      // get the classification
      TiXmlElement *pXmlTransElement = pXmlTransNode->ToElement();

      const char *start  = pXmlTransElement->Attribute( "start" );
      const char *end    = pXmlTransElement->Attribute( "end" );
      const char *period = pXmlTransElement->Attribute( "period" );
      const char *prob   = pXmlTransElement->Attribute( "prob" );
      const char *allowScaling = pXmlTransElement->Attribute( "allowScaling" );

      if ( start == NULL || end == NULL || period == NULL || prob == NULL )
         {
         CString msg( "Misformed <trans> element reading" );
         msg += file;
         Report::ErrorMsg( msg );
         loadSuccess = false;
         continue;
         }

      int startLulc = atoi( start );
      int endLulc   = atoi( end );
      int iPeriod   = atoi( period );
      int iProb     = atoi( prob );

      int iAllowScaling = 0;
      if ( allowScaling )
         iAllowScaling = atoi( allowScaling );

      // do we already have a startLULC in the TransMatrix?
      int index = -1;
      for ( int i=0; i < m_rows.GetSize(); i++ )
         {
         if ( m_rows[ i ].GetClass() == startLulc )
            {
            index = i;
            break;
            }
         }
      if ( index == -1 )   // not found, have to add row
         index = (int) m_rows.Add( TransRow( startLulc, this ) );

      m_rows[ index ].AddTransCell( iPeriod, iProb, endLulc, iAllowScaling == 0 ? false : true );   // note: endLulcC arg is lulcCode, should be transrow index,
                                                                  // but we don't have that yet.  We'll fix these up next

      // do we already have a row for the endLulcC?  This is required, even if it doesn't start any transitions
      index = -1;
      for ( int i=0; i < m_rows.GetSize(); i++ )
         {
         if ( m_rows[ i ].GetClass() == endLulc )
            {
            index = i;
            break;
            }
         }
      if ( index == -1 )   // not found, have to add row
         index = (int) m_rows.Add( TransRow( endLulc, this ) );
      }

   // fix up TransCell toRows
   for ( int i=0; i < m_rows.GetSize(); i++ )
      {
      TransRow &row = m_rows[ i ];

      for ( int j=0; j < row.m_row.GetSize(); j++ )
         {
         TransCell &cell = row.m_row[ j ];

         int toLulc = cell.m_toRow;
         int toIndex = -1;

         for ( int k=0; k < m_rows.GetSize(); k++ )
            {
            if ( m_rows[ k ].GetClass() == toLulc )
               {
               toIndex = k;
               break;
               }
            }

         if ( toIndex == - 1 )
            {
            CString msg;
            msg.Format( "SSTM: Unable to find to lulcC class %i when fixing up TransMatrix in file %s; Please fix this file.", toLulc, file );
            Report::LogWarning( msg );
            return false;
            }

         cell.m_toRow = toIndex;
         }
      }

   return loadSuccess;
   }



void TransMatrix::Update( EnvContext *pEnv )
   {
   DeltaArray* pDeltaArray = pEnv->pDeltaArray;
   const MapLayer *pMapLayer = pEnv->pMapLayer;
   int firstUnseenDelta = (int) pEnv->firstUnseenDelta;

   // want the first fromClass
   CMap<int, int, int, int > iduToPreviousLulcCRemoved;      /* corrects wrong */
   iduToPreviousLulcCRemoved.InitHashTable( 257 ); /* corrects wrong */

   int end = (int) pDeltaArray->GetCount();
   int rowCount = (int) m_rows.GetCount();

   int fromClass, toClass;
   bool ok;
   int parentID, parentClass, dmyClass;

   //LulcTree *pLulcTree = pEnv->pLulcTree;
  
   //int lulcLevels = pLulcTree->GetLevels();

   //int col = -1;
   //switch ( lulcLevels )
   //   {
   //   case 3:  col = m_colLulcC; break;
   //   case 2:  col = m_colLulcB; break;
   //   case 1:  col = m_colLulcA; break;
   //   }

   ASSERT( m_colLulc >= 0 );   
   
   for ( int i=firstUnseenDelta; i<end; i++ )
      {
      DELTA &delta = ::EnvGetDelta( pDeltaArray, i );
      
      if ( delta.col == m_colLulc )
         {
         ok = delta.oldValue.GetAsInt( fromClass );
         ASSERT( ok );
         ok = delta.newValue.GetAsInt( toClass );
         ASSERT( ok );
         //ASSERT( fromClass != toClass );
         
         if ( fromClass != toClass )
            {
            // get first fromClass
            if ( 0 == iduToPreviousLulcCRemoved.Lookup(delta.cell, parentClass) )
               iduToPreviousLulcCRemoved.SetAt( delta.cell, fromClass ); /* corrects wrong */

            for ( int row=0; row < rowCount; row++ )
               {
               TransRow &transRow = m_rows.GetAt(row);
               if ( transRow.GetClass() == fromClass )
                  {
                  ASSERT(0 != iduToPreviousLulcCRemoved.Lookup(delta.cell, dmyClass));
                  transRow.RemoveMapCell( delta.cell );
                  }
               else if ( transRow.GetClass() == toClass )
                  {
                  transRow.AddMapCell( delta.cell );
                  }
               }
            }
         }
      else if ( delta.type == DT_SUBDIVIDE )
         {
         parentID = delta.cell;
         parentClass = dmyClass = -1;
         
         // At this delta in the DeltaArray sequence, it's NOT possible for a 
         // subsequent Delta to have caused the LULC_C observed on the map now because
         // we changed AddDelta() ApplyDeltaArray() so that after *applying* a subdivide 
         // (which defuncts the idu) either the Delta is not added to or is removed from,
         // the DeltaArray, respectively.
         //
         // We need to know what the LULC_C on the MAP (not on DeltaArray) was just prior
         // to this subdivide.  We If we try to remove the TransCell, in the TransRow's list, that will fail
         // because subsequent Delta is the future trigger for putting the TransCell in the list.
         //
         // Another approach to problem is revise bad design of Delta so that new,old{Value} do have 
         // usual semantics upon a SUBDIVIDE, rather than the crazy it is now.
         //
         //
         /*wrong*///pEnv->pMapLayer->m_pData->Get( m_colLulcC, parentID, parentClass ); 
         ////if (0 == iduToPreviousLulcC.Lookup(parentID, parentClass))
            pEnv->pMapLayer->m_pData->Get( m_colLulc, parentID, parentClass ); 

         for ( int row=0; row<rowCount; row++ )
            {
            TransRow &transRow = m_rows.GetAt(row);
            if ( transRow.GetClass() == parentClass )
               {

               if ( 0 == iduToPreviousLulcCRemoved.Lookup(delta.cell, dmyClass) )
                  /*was wrong*/transRow.RemoveMapCell( parentID );

               PolyArray *pChildArray = (PolyArray*) delta.newValue.val.vPtr;
               int childCount = (int) pChildArray->GetCount();

               for ( int child=0; child<childCount; child++ )
                  transRow.AddMapCell( pChildArray->GetAt(child)->m_id );
               }
            }
         }
      else if ( delta.type == DT_MERGE )
         {
         ASSERT(0);
         }
      } 
   }

void TransMatrix::Process( EnvContext * pEnv )
   {
   int rowCount = (int) m_rows.GetCount();

   for ( int row=0; row < rowCount; row++ )
      {
      TransRow &transRow = m_rows.GetAt(row);
      transRow.Process( pEnv ); //pDeltaArray, pMapLayer, year, pLulcTree, pTargetPolyArray );
      }

   for ( int row=0; row < rowCount; row++ )
      {
      TransRow &transRow = m_rows.GetAt(row);
      transRow.AgeCells();
      }
   }

void TransMatrix::ClearData()
   {
   int count = (int) m_rows.GetCount();

   for ( int i=0; i < count; i++ )
      {
      TransRow &transRow = m_rows.GetAt(i);
      transRow.ClearData();
      }
   }


