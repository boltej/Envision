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
    
#include "QueryEngine.h"
#include "Report.h"
#include "Maplayer.h"
#include "MAP.h"

#ifndef NO_MFC
#include <io.h>
#include <share.h>
#endif
#include <sys/stat.h>
#include <fcntl.h> 
#include "PathManager.h"

int yyparse( void );

#include "Qgrammar.h"

// globals for use by the yacc parser
QueryEngine *_gpQueryEngine = NULL;
char        *gQueryBuffer;
int          gUserFnIndex;
MapLayer    *gFieldLayer;
CString      fQueryTextBuffer;
extern int   QCurrentLineNo;
extern void  QInitGlobals(  int lineNo, LPCTSTR queryStr, LPCTSTR source );
int QueryEngine::m_currentTime = 0;

static bool fIgnoreUndefinedVariables = false;
static bool showWarnings = true;
bool QueryEngine::m_debug = false;
CString QueryEngine::m_debugInfo;


//void QCompilerError( LPCTSTR type, LPCTSTR varName, LPCTSTR setName );


//------------------------ QNode class -----------------//

QNode::QNode( int value ) // from integer
   : m_value( value ),
     m_pLeft( NULL ), 
     m_pRight( NULL ), 
     m_nodeType( INTEGER ) ,
     m_extra( 0 ),
     m_extra1( -1 )
   {
   if (QueryEngine::m_debug)
      {
      CString msg;
      msg.Format("New QNode(int value=%i)", value);
      QueryEngine::m_debugInfo += msg;
      }
   }
    
QNode::QNode( double value ) // from double
    : m_value( value ), 
      m_pLeft( NULL ),
      m_pRight( NULL ), 
      m_nodeType( DOUBLE ),
      m_extra( 0 ),
      m_extra1( -1 )
   {
   if (QueryEngine::m_debug)
      {
      CString msg;
      msg.Format("New QNode(double value=%lf)", value);
      QueryEngine::m_debugInfo += msg;
      }
   }
    
QNode::QNode( char *str )    // from string
    : m_value( str ), 
      m_pLeft( NULL ), 
      m_pRight( NULL ), 
      m_nodeType( STRING ),
      m_extra( 0 ),
      m_extra1( 0 )
   {
   if (QueryEngine::m_debug)
      {
      CString msg;
      msg.Format("New QNode(char *value=%s)", str);
      QueryEngine::m_debugInfo += msg;
      }
   }  /// CHECK VDATA, SHOULD BE PTR
    

QNode::QNode( QExternal *pExternal )    // from external
    : m_value( 0 ), 
      m_pLeft( NULL ), 
      m_pRight( NULL ), 
      m_nodeType( EXTERNAL ),
      m_extra( (LONG_PTR) pExternal ),
      m_extra1( 0 )
   {
   if (QueryEngine::m_debug)
      {
      CString msg;
      msg.Format("New QNode(External=%s)", (LPCTSTR) pExternal->m_identifier);
      QueryEngine::m_debugInfo += msg;
      }
   }

QNode::QNode(MapLayer *pLayer, int fieldCol)  // from a field column in a map layer
   : m_value(-1),
   m_pLeft(NULL),
   m_pRight(NULL),
   m_nodeType(FIELD),
   m_extra(fieldCol),
   m_extra1((LONG_PTR)pLayer)
   { 
   if (QueryEngine::m_debug)
      {
      LPCTSTR field = pLayer->GetFieldLabel(fieldCol);
      CString msg;
      msg.Format("New QNode(field=%s)", field);
      QueryEngine::m_debugInfo += msg;
      }
   }


QNode::QNode( MapLayer *pLayer, int fieldCol, QNode *pRefField )  // from a field column in a map layer - record is defined by another fieldRef
   : m_value( -1 ),
     m_pLeft( NULL ),
     m_pRight( pRefField ),
     m_nodeType( FIELD ),
     m_extra( fieldCol ),
     m_extra1( (LONG_PTR) pLayer )
   {
   if (QueryEngine::m_debug)
      {
      LPCTSTR field = pLayer->GetFieldLabel(fieldCol);
      CString msg;
      msg.Format("New QNode(field=%s)+refField",field);
      QueryEngine::m_debugInfo += msg;
      }
   }

QNode::QNode( MapLayer *pLayer, int fieldCol, INT_PTR record )          // // from a field column in a map layer - record is defined as an explicit index (-1 for current index)
   : m_value( -1 ),
     m_pLeft( NULL ),
     m_pRight( (QNode*) record ),  // NOTE:  this is questionsable, assumes less than 256 columns  /// NOTE2 - I don't understand this comment
     m_nodeType( FIELD ),
     m_extra( fieldCol ),
     m_extra1( (LONG_PTR) pLayer )
   {
   if (QueryEngine::m_debug)
      {
      LPCTSTR field = pLayer->GetFieldLabel(fieldCol);
      CString msg;
      msg.Format("New QNode(field=%s, record=%i)", field, (int) record);
      QueryEngine::m_debugInfo += msg;
      }
   }
    

QNode::QNode( QFNARG &fnArg)
   : m_value( -1 ),
     m_pLeft( NULL ),
     m_pRight( NULL ),
     m_nodeType( fnArg.function ),  // #defined value
     m_extra( (LONG_PTR) fnArg.pArgList ),
     m_extra1( 0 )
   {
   if (QueryEngine::m_debug)
      {
      CString msg;
      msg.Format("New QNode(Function=%i)", fnArg.function);
      QueryEngine::m_debugInfo += msg;
      }
   }


// constructing from a valueSet
QNode::QNode( QValueSet *pSet )
   : m_value( -1 ),
     m_pLeft( NULL ),
     m_pRight( NULL ),
     m_nodeType( VALUESET ),  // #defined value
     m_extra( (LONG_PTR) pSet ),
     m_extra1( 0 )
   { }


QNode::~QNode( void )
   {

   if ( m_pLeft != NULL )
      {
      delete m_pLeft;
      m_pLeft = NULL;
      }
    
   if ( m_pRight != NULL )
      {
      delete m_pRight;
      m_pRight = NULL;
      }
   }
    

// QNode::Solve() - if able to solve this node, return true; otherwise return false.
//    If successful, the value of the result of solving the node is placed in the node m_value member

bool QNode::Solve( void )
   {
   // first, solve each child, if they exist.
   if ( m_pLeft != NULL )
      if ( m_pLeft->Solve() == false )
         return false;
            
   if ( m_pRight != NULL )
      if ( m_pRight->Solve() == false )
         return false;
    
   // now, start figuring out how to solve this one.
    
   // is a terminal (leaf) node?  Then it is a variable value or a constant   
   if ( IsTerminal() )
      {
      switch( m_nodeType )
         {
         case FIELD:
            {
            // Note:  This can be a field in the default layer or or fully qualified fieldRef
            MapLayer *pLayer = (MapLayer*) m_extra1;
            int col = (int) m_extra;
            ASSERT( pLayer != NULL );

            if ( m_pRight == NULL ) // simple reference to a column in the default layer
               return pLayer->GetData( _gpQueryEngine->m_currentRecord, col, m_value );
            
            else  // a fully qualified name of the form LayerName.FieldName( recordIndex )
               {
               INT_PTR _recordIndex = (INT_PTR) m_pRight;
               int recordIndex = 0;
               bool ok = true;

               // is it NOT an explicit record index (in other words, is it a "real" QNode?)
               if ( _recordIndex < 0 || _recordIndex >= 256 )
                  ok = m_pRight->m_value.GetAsInt( recordIndex );
               else
                  recordIndex = (int) _recordIndex;

               if ( ok  && recordIndex >= 0 && recordIndex < pLayer->GetRecordCount() )
                  return pLayer->GetData( recordIndex, col, m_value );

               else
                  {
                  m_value.SetNull();      // = 0;?
                  return false;
                  }
               }
            }
            break;

         // functions
         case NEXTTO:
            {
            // Arg List should be a QNode ( the query )
            QArgList *pArgList = (QArgList*) m_extra;
            ASSERT( pArgList->GetCount() == 1 );

            QArgument &arg = pArgList->GetAt(0);
            ASSERT( arg.m_pNode );
            MapLayer *pMapLayer = _gpQueryEngine->GetMapLayer();
            ASSERT( pMapLayer );

            bool value = true;
            bool ok;
            int currentRecord = _gpQueryEngine->m_currentRecord;
            Poly *pPoly = pMapLayer->GetPolygon( currentRecord );
            
            int *neighbors = NULL;
            int count = 0;
            int n = 0;

            do
               {
               if ( n > 0 )
                  delete [] neighbors;
               n += 100;
               neighbors = new int[ n ];
               count = pMapLayer->GetNearbyPolysFromIndex( pPoly, neighbors, NULL, n, 0 );
               } 
            while ( n <= count );

            for ( int i=0; i<count; i++ )
               {
               _gpQueryEngine->SetCurrentRecord( neighbors[i] );

               if ( arg.m_pNode->Solve() == false )
                  {
                  delete [] neighbors;
                  return false;
                  }
               
               ok = arg.m_pNode->GetValueAsBool( value );
               ASSERT( ok );

               if ( value )
                  break;
               }

            m_value = value;
            _gpQueryEngine->SetCurrentRecord( currentRecord );

            if( neighbors != NULL )
               delete [] neighbors;

            return true;
            }

         case NEXTTOAREA:     // returns the area of the polys satisfying the query that that are extended contiguous with the target IDU
            {
            // Arg List should be a QNode ( the query )
            QArgList *pArgList = (QArgList*) m_extra;
            ASSERT( pArgList->GetCount() == 1 );

            QArgument &arg = pArgList->GetAt(0);
            ASSERT( arg.m_pNode );
            MapLayer *pMapLayer = _gpQueryEngine->GetMapLayer();
            ASSERT( pMapLayer );

            int currentRecord = _gpQueryEngine->m_currentRecord;
            Poly *pPoly = pMapLayer->GetPolygon( currentRecord );

            // store neighbor information in a hash table to allow quick lookup to see if it's been visited or not.
            CMap< int, int, bool, bool > foundPolyMap;   // key is polyID, value is 0/1 found or not

            QNode *pNode = arg.m_pNode;
            pNode->GetNeighbors( pPoly, foundPolyMap );

            // foundPolyMap contains info on every visited polygon.  compute area satisfying the query by iterating through the map
            POSITION p = foundPolyMap.GetStartPosition();
            int polyIndex;
            float totalArea = 0.0f;

            while( p != NULL )
               {
               bool value;
               foundPolyMap.GetNextAssoc( p, polyIndex, value );

               if ( value )
                  {
                  pMapLayer->AddSelection( polyIndex );
                  float area = pMapLayer->GetPolygon( polyIndex )->GetArea();
                  totalArea += area;
                  }
               }

            m_value = totalArea;
            _gpQueryEngine->SetCurrentRecord( currentRecord );

            return true;
            }

         case WITHIN:
            {
            // Arg List should be a QNode ( the query ) and a VData of type double ( the distance )
            QArgList *pArgList = (QArgList*) m_extra;
            ASSERT( pArgList->GetCount() == 2 );

            QArgument &arg1 = pArgList->GetAt(0);
            QNode *pNode = arg1.m_pNode;
            ASSERT( pNode );

            QArgument &arg2 = pArgList->GetAt(1);
            float distance;
            bool ok = arg2.GetAsFloat( distance );
            ASSERT( ok );
            ASSERT( distance > 0 );

            MapLayer *pMapLayer = _gpQueryEngine->GetMapLayer();
            ASSERT( pMapLayer );

            // make sure a spatial index exists
            if ( pMapLayer->GetSpatialIndex() == NULL )
               pMapLayer->CreateSpatialIndex( NULL, 10000, 500, SIM_NEAREST );

            if ( distance > pMapLayer->GetSpatialIndexMaxDistance() && showWarnings )
               {
               CString msg( "QueryEngine: WithIn(.) distance is larger than the Spatial Index was built for; Query may be inaccurate" );
               Report::WarningMsg( msg );
               showWarnings = false;
               }

            bool value = false;
            int currentRecord = _gpQueryEngine->m_currentRecord;
            Poly *pPoly = pMapLayer->GetPolygon( currentRecord );

            int *neighbors = NULL;
            int count = 0;
            int n = 0;

            do
               {
               if ( n > 0 )
                  delete [] neighbors;
               n += 100;
               neighbors = new int[ n ];
               count = pMapLayer->GetNearbyPolysFromIndex( pPoly, neighbors, NULL, n, distance );
               } 
            while ( n <= count );

            for ( int i=0; i<count; i++ )
               {
               _gpQueryEngine->SetCurrentRecord( neighbors[i] );

               if ( pNode->Solve() == false )
                  {
                  delete [] neighbors;
                  return false;
                  }
               
               ok = pNode->GetValueAsBool( value );
               ASSERT( ok );

               if ( value )
                  break;
               }

            m_value = value;
            _gpQueryEngine->SetCurrentRecord( currentRecord );

            if ( neighbors != NULL )
               delete [] neighbors;

            return true;
            }

         case WITHINAREA:
            {
            // Arg List should be a QNode ( the query ) and a VData of type double ( the distance )
            QArgList *pArgList = (QArgList*) m_extra;
            ASSERT( pArgList->GetCount() == 3 );

            QArgument &arg1 = pArgList->GetAt(0);
            QNode *pNode = (QNode*)arg1.m_pNode;
            ASSERT( pNode );

            QArgument &arg2 = pArgList->GetAt(1);
            float distance;
            bool ok = arg2.GetAsFloat( distance );
            ASSERT( ok );
            ASSERT( distance > 0 );

            QArgument &arg3 = pArgList->GetAt(2);
            double areaConstraint;
            ok = arg3.GetAsDouble( areaConstraint );
            ASSERT( ok );
            ASSERT( areaConstraint > 0 );

            MapLayer *pMapLayer = _gpQueryEngine->GetMapLayer();
            ASSERT( pMapLayer );

            if ( distance > pMapLayer->GetSpatialIndexMaxDistance() )
               {
               CString msg( "QueryEngine: WithIn(.) distance is larger than the Spatial Index was built for; Query may be inaccurate" );
               Report::WarningMsg( msg );
               }

            int currentRecord = _gpQueryEngine->m_currentRecord;
            Poly *pPoly = pMapLayer->GetPolygon( currentRecord );
            Vertex center = pPoly->GetCentroid();
            
            int *neighbors = NULL;
            int count = 0;
            int n = 0;

            do
               {
               if ( n > 0 )
                  delete [] neighbors;
               n += 100;
               neighbors = new int[ n ];
               count = pMapLayer->GetNearbyPolysFromIndex( pPoly, neighbors, NULL, n, distance );
               } 
            while ( n <= count );

            bool value;
            double  wantedArea = 0.0;
            double  totalArea  = 0.0;
            float   area;
            Poly   *pNeighbor;

            for ( int i=0; i<count; i++ )
               {
               _gpQueryEngine->SetCurrentRecord( neighbors[i] );

               if ( pNode->Solve() == false )
                  {
                  delete [] neighbors;
                  return false;
                  }
               
               ok = pNode->GetValueAsBool( value );
               ASSERT( ok );

               pNeighbor = pMapLayer->GetPolygon( neighbors[i] );
               area = pNeighbor->GetAreaInCircle( center, distance );
               totalArea += (double)area;

               if ( value )   // was query satisfied?
                  wantedArea += (double)area;   // then add to wanted area
               }

            /*
            if ( totalArea <= 0.0f )
               m_value = 0.0;
            else
               m_value = wantedArea/totalArea;*/
            m_value = false;
            if ( totalArea > 0.0f && wantedArea/totalArea >= areaConstraint )
               m_value = true;
            
            _gpQueryEngine->SetCurrentRecord( currentRecord );

            if ( neighbors != NULL )
               delete [] neighbors;

            return true;
            }

         //case CHANGED:
         //   return true;
         
         //case MOVAVG:
         //   {
         //   // Arg List should be a QNode ( the query ) and a VData of type double ( the distance )
         //   QArgList *pArgList = (QArgList*) m_extra;
         //   ASSERT( pArgList->GetCount() == 2 );
         //
         //   QArgument &arg1 = pArgList->GetAt(0);
         //   QNode *pNode = arg1.m_pNode;
         //   ASSERT( pNode );
         //
         //   QArgument &arg2 = pArgList->GetAt(1);
         //   float distance;
         //   bool ok = arg2.GetAsFloat( distance );
         //   ASSERT( ok );
         //   ASSERT( distance > 0 );
         //
         //   MapLayer *pMapLayer = _gpQueryEngine->GetMapLayer();
         //   ASSERT( pMapLayer );
         //
         //   // make sure a spatial index exists
         //   if ( pMapLayer->GetSpatialIndex() == NULL )
         //      pMapLayer->CreateSpatialIndex( NULL, 10000, 500, SIM_NEAREST );
         //
         //   if ( distance > pMapLayer->GetSpatialIndexMaxDistance() && showWarnings )
         //      {
         //      CString msg;
         //      msg  = "WithIn(.) distance is larger than the Spatial Index was built for";
         //      msg += "Query may be inaccurate";
         //      Report::WarningMsg( msg );
         //      showWarnings = false;
         //      }
         //
         //   bool value = false;
         //   int currentRecord = _gpQueryEngine->m_currentRecord;
         //   Poly *pPoly = pMapLayer->GetPolygon( currentRecord );
         //
         //   int *neighbors = NULL;
         //   int count = 0;
         //   int n = 0;
         //
         //   do
         //      {
         //      if ( n > 0 )
         //         delete [] neighbors;
         //      n += 100;
         //      neighbors = new int[ n ];
         //      count = pMapLayer->GetNearbyPolysFromIndex( pPoly, neighbors, NULL, n, distance );
         //      } 
         //   while ( n <= count );
         //
         //   for ( int i=0; i<count; i++ )
         //      {
         //      _gpQueryEngine->SetCurrentRecord( neighbors[i] );
         //
         //      if ( pNode->Solve() == false )
         //         {
         //         delete [] neighbors;
         //         return false;
         //         }
         //      
         //      ok = pNode->GetValueAsBool( value );
         //      ASSERT( ok );
         //
         //      if ( value )
         //         break;
         //      }
         //
         //   m_value = value;
         //   _gpQueryEngine->SetCurrentRecord( currentRecord );
         //
         //   if ( neighbors != NULL )
         //      delete [] neighbors;
         //
         //   return true;
         //   }

         case TIME:
            {
            // Arg List should be a Integer
            //QArgList *pArgList = (QArgList*)m_extra;
            //ASSERT(pArgList->GetCount() == 0);

            m_value = QueryEngine::m_currentTime;

            return true;
            }

         case INDEX:
            {
            m_value = _gpQueryEngine->m_currentRecord;
            return true;
            }


         case DELTA:
            {
            // Arg List should be two expressions
            QArgList *pArgList = (QArgList*)m_extra;
            ASSERT(pArgList->GetCount() == 2);

            QArgument &arg0 = pArgList->GetAt(0);
            ASSERT(arg0.m_pNode);

            QArgument &arg1 = pArgList->GetAt(1);
            ASSERT(arg1.m_pNode);

            if (arg0.m_pNode->Solve() == false)
               return false;

            if (arg1.m_pNode->Solve() == false)
               return false;
 
            if (::IsInteger(arg0.m_pNode->m_value.GetType()) || ::IsInteger(arg1.m_pNode->m_value.GetType()))
               {
               double value0, value1;
               bool ok = arg0.m_pNode->GetValueAsDouble(value0);
               ASSERT(ok);
               ok = arg1.m_pNode->GetValueAsDouble(value1);
               ASSERT(ok);
               m_value = value0 - value1;
               }
            else
               {
               int value0, value1;
               bool ok = arg0.m_pNode->GetValueAsInt(value0);
               ASSERT(ok);
               ok = arg1.m_pNode->GetValueAsInt(value1);
               ASSERT(ok);
               m_value = int( value0 - value1 );
               }
            
            return true;
            }

         case USERFN2:
            {
            // Arg List should be: 0) function index, 1) int (keyword), 2) fn ptr for user defined function
            QArgList* pArgList = (QArgList*)m_extra;
            ASSERT(pArgList->GetCount() == 3);

            QArgument& fnIndex = pArgList->GetAt(0);
            int _fnIndex;
            bool ok = fnIndex.GetAsInt(_fnIndex);

            QArgument& arg0 = pArgList->GetAt(1);
            int _arg0;
            ok = arg0.GetAsInt(_arg0);

            QArgument& arg1 = pArgList->GetAt(2);
            int _arg1;
            ok = arg1.GetAsInt(_arg1);
            try {
               USERFN2PROC pFn = (USERFN2PROC)_gpQueryEngine->GetUserFn(_fnIndex)->m_pFn;
               m_value = pFn(_arg0, _arg1);
               }
            catch (...) {};

            //for (int i = 0; i < _gpQueryEngine->GetUserFnCount(); i++)
            //   {
            //   if (_gpQueryEngine->GetUserFn(i)->m_nArgs == 2) 
            //      {
            //      USERFN2PROC pFn = (USERFN2PROC)_gpQueryEngine->GetUserFn(i)->m_pFn;
            //      m_value = pFn(_arg0, _arg1);
            //      break;
            //      }
            //   }
            return true;
            }

            /*
         case POLYINDEX:
            {
            // Arg List should be a Integer
            QArgList *pArgList = (QArgList*) m_extra;
            ASSERT( pArgList->GetCount() == 0 );

            MapLayer *pMapLayer = _gpQueryEngine->GetMapLayer();
            ASSERT( pMapLayer );



            bool value;
            bool ok;
            int currentRecord = _gpQueryEngine->m_currentRecord;
            Poly *pPoly = pMapLayer->GetPolygon( currentRecord );
            
            int *neighbors = NULL;
            int count = 0;
            int n = 0;

            do
               {
               if ( n > 0 )
                  delete [] neighbors;
               n += 100;
               neighbors = new int[ n ];
               count = pMapLayer->GetNearbyPolysFromIndex( pPoly, neighbors, NULL, n, 0 );
               } 
            while ( n <= count );

            for ( int i=0; i<count; i++ )
               {
               _gpQueryEngine->SetCurrentRecord( neighbors[i] );

               if ( arg.m_pNode->Solve() == false )
                  return false;
               
               ok = arg.m_pNode->GetValueAsBool( value );
               ASSERT( ok );

               if ( value )
                  break;
               }

            m_value = value;
            _gpQueryEngine->SetCurrentRecord( currentRecord );
            delete [] neighbors;

            return true;
            }
*/

         case INTEGER:        // constants?  nothing to do (m_value already set)...
         case DOUBLE:
         case STRING:
            return true;

         case EXTERNAL:
            {
            QExternal *pExt = (QExternal*)m_extra;
            ASSERT(pExt != NULL);

            if (pExt->m_col >= 0)   // it's a field in the database
               {
               MapLayer *pMapLayer = _gpQueryEngine->GetMapLayer();
               ASSERT(pMapLayer != NULL);
               int currentRecord = _gpQueryEngine->m_currentRecord;
               pMapLayer->GetData(currentRecord, pExt->m_col, m_value);
               }
            else
               {
               VData value;
               pExt->GetValue(value);     // value should be a ptr type

               switch (value.GetType())
                  {
                  case TYPE_PUINT:
                  case TYPE_PINT:
                  case TYPE_PULONG:
                  case TYPE_PLONG:
                     {
                     int ivalue = 0;
                     value.GetAsInt(ivalue);
                     m_value = ivalue;
                     }
                                       
                  case TYPE_PFLOAT:
                  case TYPE_PDOUBLE:
                     {
                     float fvalue = 0;
                     value.GetAsFloat(fvalue);
                     m_value = fvalue;
                     }
                  }  // end of switch (type)
               }  // end of: else col < 0
            }  // end of:  case EXTERNAL 
            return true;
    
         default:
            {
            CString msg;
            msg.Format( "Internal Error - invalid terminal node type (%i)", m_nodeType );
            _gpQueryEngine->RuntimeError( msg );
            ASSERT( 0 );
            return false;
            }
         }  // end of : switch( m_nodeType )
      }  // end of: IsTerminal();
    
    // non terminal, so handle remaining cases by examining children
    //ASSERT( m_pLeft != NULL );
    ASSERT( m_pRight != NULL );
    
    switch( m_nodeType )
       {
       case AND:   // a logical "AND" node...
       case OR:    // logical "OR" node
       case NOT:
          return SolveLogical();
             
       case '+':
       case '-':
       case '*':
       case '/':
       case '%':
          return SolveArithmetic();
    
       case GE:
       case LE:
       case EQ:
       case NE:
       case LT:
       case GT:
       case COR:
       case CAND:
       case CONTAINS:
          return SolveRelational();
    
       default:
          _gpQueryEngine->RuntimeError( "Unknown QNode operator or type" );
          return false;
       }
    }


void QNode::GetNeighbors( Poly *pPoly, CMap< int, int, bool, bool > &foundPolyMap )
   {
   // "this" is a queryExpr node.  Find (recursively) all the neighbors of the poly that satisfy this 

   // first, get all the neighbors of this poly
   int neighbors[ 128 ];
   int count = _gpQueryEngine->m_pMapLayer->GetNearbyPolys( pPoly, neighbors, NULL, 128, 0.0001f );

   // see if they satisfy the query
   bool value;
   for ( int i=0; i < count; i++ )
      {
      int polyIndex = neighbors[ i ];

      // have we visted this poly yet?
      if ( foundPolyMap.Lookup( polyIndex, value ) != 0 )
         continue;

      _gpQueryEngine->SetCurrentRecord( polyIndex );

      if ( this->Solve() == false )
         {
         ASSERT( 0 );
         continue;
         }

      bool ok = this->GetValueAsBool( value );
      ASSERT( ok );

      foundPolyMap.SetAt( polyIndex, value );

      if ( value )   // was the query satisfied?
         {
         pPoly = _gpQueryEngine->m_pMapLayer->GetPolygon( polyIndex );
         this->GetNeighbors( pPoly, foundPolyMap );
         }
      }

   return;
   }


//-- QNode::SolveLogical() ---------------------------------------------
//
// Solve expressions of the form  a AND|OR b.
//
// for fuzzy expressions, the membership functions are combined according
//   to the current settings in _gpQueryEngine....
//---------------------------------------------------------------------


bool QNode::SolveLogical( void )
    {
    bool lvalue = false;
    bool rvalue = false;
    bool ok = false;
    if (m_nodeType != NOT)
       {
       ok = m_pLeft->GetValueAsBool(lvalue);

       if (!ok)
          {
          _gpQueryEngine->RuntimeError("Error getting LH boolean value while evaluating 'AND/OR' ");
          return false;
          }
       }
    
    ok = m_pRight->GetValueAsBool( rvalue );
               
    if ( !ok )
       {
       _gpQueryEngine->RuntimeError( "Error getting RH boolean value while evaluating 'AND/OR' " );
       return false;
       }
    
    switch (m_nodeType)
       {
       case AND:
          m_value = (bool)(lvalue && rvalue);      // make sure vdata can handle this
          break;

       case OR:
          m_value = (bool)(lvalue || rvalue);
          break;

       case NOT:
          m_value = ! rvalue;
             break;
       }

    return true;
    }
    
    
bool QNode::SolveArithmetic( void )
    {
    // get types of arguments
    TYPE ltype = m_pLeft->m_value.type; 
    TYPE rtype = m_pRight->m_value.type; 
    
    // are they both ints?
    if ( IsInteger( ltype ) && IsInteger( rtype ) )
       {
       int lvalue;
       bool ok = m_pLeft->m_value.GetAsInt( lvalue );
       ASSERT( ok );
    
       int rvalue;
       ok = m_pRight->m_value.GetAsInt( rvalue );
       ASSERT( ok );
    
       switch( m_nodeType )
          {
          case '+':   m_value = lvalue + rvalue;  return true;
          case '-':   m_value = lvalue - rvalue;  return true;
          case '*':   m_value = lvalue * rvalue;  return true;
          case '/':   m_value = lvalue / rvalue;  return true;
          case '%':   m_value = lvalue % rvalue;  return true;
          default: ASSERT( 0 ); return false;
          }
       }  // end of:  if ( IsInteger() )
    
    // are one or both doubles (and the other an int)?
    if ( ( IsReal( ltype ) && IsReal( rtype ) )
       ||( IsReal( ltype ) && IsInteger( rtype ) )
       ||( IsInteger( ltype ) && IsReal( rtype ) ) )
       {
       double lvalue;
       bool ok = m_pLeft->m_value.GetAsDouble( lvalue );
       ASSERT( ok );
 
       double rvalue;
       ok = m_pRight->m_value.GetAsDouble( rvalue );
       ASSERT( ok );
 
       switch( m_nodeType )
          {
          case '+':   m_value = lvalue + rvalue;  return true;
          case '-':   m_value = lvalue - rvalue;  return true;
          case '*':   m_value = lvalue * rvalue;  return true;
          case '/':   m_value = lvalue / rvalue;  return true;
          case '%':   m_value = fmod(lvalue,rvalue); return true;
          default: ASSERT( 0 ); return false;
          }
       }  // end of: if ( IsDouble() )
    
    // are they both strings?
    if ( IsString( ltype ) && IsString( rtype ) )
       {
       // it a string to add?
       _gpQueryEngine->RuntimeError( "String addition currently not supported" );
       return false;
       }
    
    CString msg( "Arithmetic operator " );
    msg += (char) m_nodeType;
    msg += " invoked with illegal types...";
    
    _gpQueryEngine->RuntimeError( msg );
    return false;
    }
    
            
bool QNode::SolveRelational( void )
   {
   // handles GE:LE:EQ:NE:LT:GT:COR:CAND:CONTAINS

   // get types of arguments
   TYPE ltype = m_pLeft->m_value.type; 
   TYPE rtype = m_pRight->m_value.type; 

   if ( ltype == TYPE_NULL || rtype == TYPE_NULL )  // left hand, right hand must be a value, otherwise fail
      {
      m_value = false;
      return true;
      }

   // are they both numbers?  then treat them as doubles
   if ( IsNumeric( ltype ) && IsNumeric( rtype ) )
      {
      if ( m_nodeType == COR || m_nodeType == CAND )
         {
         int lvalue;
         bool ok = m_pLeft->m_value.GetAsInt( lvalue );
         ASSERT( ok );
 
         int rvalue;
         ok = m_pRight->m_value.GetAsInt( rvalue );
         ASSERT( ok );
 
         switch ( m_nodeType )
            {
            case COR:  m_value = ( (lvalue | rvalue) != 0 ) ? true : false; return true;
            case CAND: m_value = ( (lvalue & rvalue) != 0 ) ? true : false; return true;
            }      
         ASSERT( 0 );
         }

      double lvalue;
      bool ok = m_pLeft->m_value.GetAsDouble( lvalue );
      ASSERT( ok );
 
      double rvalue;
      ok = m_pRight->m_value.GetAsDouble( rvalue );
      ASSERT( ok );
 
      switch( m_nodeType )
         {
         case GT:   m_value = (bool) ( lvalue >  rvalue ); return true;
         case GE:   m_value = (bool) ( lvalue >= rvalue ); return true;
         case LT:   m_value = (bool) ( lvalue <  rvalue ); return true;
         case LE:   m_value = (bool) ( lvalue <= rvalue ); return true;
         case EQ:   m_value = (bool) ( lvalue == rvalue ); return true;
         case NE:   m_value = (bool) ( lvalue != rvalue ); return true;
         default: ASSERT( 0 ); return false;
         }
      }  // end of:  if ( IsInteger() )
 
   // are they both strings?
   if ( IsString( ltype ) && IsString( rtype ) )
      {
      CString left  = m_pLeft->m_value.GetAsString();
      CString right = m_pRight->m_value.GetAsString();
 
      switch( m_nodeType )
         {
         case GT:   m_value = (bool) ( left.Compare( right ) >  0 ); return true;
         case GE:   m_value = (bool) ( left.Compare( right ) >= 0 ); return true;
         case LT:   m_value = (bool) ( left.Compare( right ) <  0 ); return true;
         case LE:   m_value = (bool) ( left.Compare( right ) <= 0 ); return true;
         case EQ:   m_value = (bool) ( left.Compare( right ) == 0 ); return true;
         case NE:   m_value = (bool) ( left.Compare( right ) != 0 ); return true;
         case CONTAINS:
            {
            int pos = left.Find( right );
            m_value = (bool) ( pos >= 0 ? true : false ); 
            return true;
            }

         default: ASSERT( 0 ); return false;
         }
      }
 
   // comparing strings and numbers?  try to convert the string...
   if ( (IsString( ltype ) && IsNumeric( rtype ) )
      ||(IsNumeric( ltype ) && IsString( rtype ) ) )
      {
      double lvalue;
      bool ok = m_pLeft->m_value.GetAsDouble( lvalue );
      if ( ! ok )
         {
         CString msg;
         CString left =  m_pLeft->m_value.GetAsString();
         CString right = m_pRight->m_value.GetAsString();

         msg = "Unable to compare left-hand string and number: ";
         msg += left;
         msg += ", ";
         msg += right;
         _gpQueryEngine->RuntimeError( msg );
         return false;
         }
 
      double rvalue;
      ok = m_pRight->m_value.GetAsDouble( rvalue );
      if ( ! ok )
         {
         CString msg;
         CString left =  m_pLeft->m_value.GetAsString();
         CString right = m_pRight->m_value.GetAsString();

         msg = "Unable to compare left-hand number and string: ";
         msg += left;
         msg += ", ";
         msg += right;
         _gpQueryEngine->RuntimeError( msg );
         return false;
         }
       
      // have both values as numbers, try again...      
      switch( m_nodeType )
         {
         case GT:   m_value = (bool) ( lvalue >  rvalue ); return true;
         case GE:   m_value = (bool) ( lvalue >= rvalue ); return true;
         case LT:   m_value = (bool) ( lvalue <  rvalue ); return true;
         case LE:   m_value = (bool) ( lvalue <= rvalue ); return true;
         case EQ:   m_value = (bool) ( lvalue == rvalue ); return true;
         case NE:   m_value = (bool) ( lvalue != rvalue ); return true;
         default: ASSERT( 0 ); return false;
         }
      }  // end of:  if ( IsInteger() )
  
   if ( m_nodeType == EQ && ltype == FIELD && rtype == VALUESET )    // is it a field = [valueset]
      {
      m_value = false;

      QValueSet *pSet = (QValueSet*) m_pRight->m_extra;
      int count = (int) pSet->GetSize();

      for ( int i=0; i < count; i++ )
         {
         if ( m_pLeft->m_value != pSet->GetAt( i ) )
            {
            m_value = true;
            return true;
            }
         }

      return true;
      }
   
   // all else fails...
   CString msg( "Relational operator '" );
   switch( m_nodeType )
      {
      case GT: msg += ">'";  break;
      case GE: msg += ">='"; break;
      case LT: msg += "<'";  break;
      case LE: msg += "<='"; break;
      case EQ: msg += "='";  break;
      case NE: msg += "!='"; break;
      }

   msg += " invoked with illegal types...";

   _gpQueryEngine->RuntimeError( msg );
   return false;
   }

   
bool QNode::GetLabel( CString &label, int options )
   {
   label.Empty();

   switch( m_nodeType )
      {
      case FIELD:
         {
         ASSERT( m_extra != 0 );
         ASSERT( m_extra1 != 0 );
         MapLayer *pLayer = (MapLayer*) m_extra1;
         int col = (int) m_extra;
         LPCTSTR fieldName = pLayer->GetFieldLabel( col );

         if( options & SHOW_LEADING_TAGS )
            label = pLayer->m_name + ".";
         
         label += fieldName;

         if ( options & REPLACE_FIELDNAMES )
            {
            MAP_FIELD_INFO *pInfo = pLayer->FindFieldInfo( pLayer->GetFieldLabel( col ) );

            if ( pInfo != NULL )
               {
               CString rep = pInfo->label; // + "(" + fieldName + ")";
               label.Replace( fieldName, rep );
               }
            }

         if ( options & REMOVE_UNDERSCORES )
            label.Replace( '_', ' ' );

         return true;
         }

      case INTEGER:
         {
         int value;
         bool ok = m_value.GetAsInt( value );
         ASSERT( ok );

         label.Format( "%d", value );
         return true;
         }
         
      case DOUBLE:
         {
         double value;
         bool ok = m_value.GetAsDouble( value );
         ASSERT( ok );

         label.Format( "%lg", value );
         return true;
         }

      case STRING:
         {
         LPCTSTR value = m_value.GetAsString();
         ASSERT( value != NULL );
         label.Format( "%s", value );
            
         if ( options & REMOVE_UNDERSCORES )
            label.Replace( '_', ' ' );

         break;
         }

      case AND:  label = "and";  return true;
      case OR:   label = "or";   return true;
      case '+':  label = "+";    return true;
      case '-':  label = "-";    return true;
      case '*':  label = "*";    return true;
      case '/':  label = "/";    return true;
      case GE:
         if ( options & HUMAN_READABLE )
            {
            if ( options & USE_HTML )
               label = "is <i>greater than/equal to</i>";
            else
               label = "is greater than/equal to";
            }
         else
            label = ">="; 
         return true; 

      case LE:
         if ( options & HUMAN_READABLE )
            {
            if ( options & USE_HTML )
               label = "is <i>less than/equal to</i>";
            else
               label = "is less than/equal to";
            }
         else
            label = "<="; 
         return true;

      case EQ:   (options & HUMAN_READABLE ) ? label = "is"              : label = "=";  return true;
      case NE:   (options & HUMAN_READABLE ) ? label = "is not"          : label = "!="; return true;
      case LT:
         if ( options & HUMAN_READABLE )
            {
            if ( options & USE_HTML )
               label = "is <i>less than</i>";
            else
               label = "is less than";
            }
         else
            label = "<"; 
         return true;

      case GT: 
         if ( options & HUMAN_READABLE )
            {
            if ( options & USE_HTML )
               label = "is <i>greater than</i>";
            else
               label = "is greater than";
            }
         else
            label = ">"; 
         return true;

      case COR:  label = "includes";  return true;
      case CAND: label = "+";    return true;
      case CONTAINS: label = "contains";     return true;

      default:
         label = "??";
         return true;
      }

   if ( options & USE_HTML )
      {
      label.Replace( "<", "&lt;" );
      label.Replace( ">", "&gt;" );
      }

   return true;
   }
    

//--------------------- Query methods ---------------------//
   
Query::Query( Query &query )
: m_value( query.m_value ),
  m_solved( false ),
  m_inUse( query.m_inUse ),
  m_text( query.m_text ),
  m_pQueryEngine(query.m_pQueryEngine )
   {
   // make a copy of all the nodes in this expression and hook them up
   m_pRoot = CopyTree( query.m_pRoot );    // pointer to the root node of the expression
   }

QNode *Query::CopyTree( QNode *pNodeToCopy )
   {
   if ( pNodeToCopy == NULL )
      return NULL;

   QNode *pNode = new QNode( *pNodeToCopy );      // make a copy of this node

   pNode->m_pLeft  = CopyTree( pNodeToCopy->m_pLeft );
   pNode->m_pRight = CopyTree( pNodeToCopy->m_pRight );

   return pNode;
   }

//-- Query::Solve() ----------------------------------------
//
// tries to solve the root node of the expression.  If the root
//    node can be solved, set 'value' to the value of the expression
//    and returns true.  If the node can't be solved, (usually because
//    of a runtime error), sets 'value' to false and return false;
//----------------------------------------------------------------
 
bool Query::Solve( bool &value )
   {
   _gpQueryEngine = m_pQueryEngine;

   //ASSERT ( m_pRoot != NULL );
   if ( m_pRoot == NULL )  // a NULL expression (fact) is "true"
      {
      value = true;
      return true;
      }

   bool ok = m_pRoot->Solve();

   if ( ok )      // successfully solved the root node?
      {
      ok = m_pRoot->GetValueAsBool( value );
      ASSERT( ok );
      return true;
      }
   else     // unable to solve the root node, so indicate failure.
      {
      value = false;
      return false;
      }
   }

/*

//-- LogicalStatement::Solve() --------------------------------------
//
// Solves Rules and Constraints, by seeing if there conditions are 
//     satisifed.  
//
// For crisp rules, if the rule conditions are satisifed, the rule's
//     m_value is set to true;  otherwise, m_value is set to false.
//     The function returns the boolean result stored in m_value.//      
//-------------------------------------------------------------------

bool LogicalStatement::Solve( void )
   {
   // has it already been solved?  
   if ( m_solved )   // no need to do it again, just return the solved value
      return m_value;

   else
      {
      ASSERT( m_pConditions != NULL );
      m_solved = true;

      bool ok = m_pConditions->Solve( m_value ); // solve the rule. m_value is the
            // a truth of the rule;  ok indicates whether the conditions were solveable
      if ( ok )
         return m_value;
      else  // error solving rule
         {
         m_inUse = false;
         return false;
         }
      }
   }
*/

bool Query::Run( int i, bool &result )
   {
   if ( m_pQueryEngine == NULL ) 
      return false;  
   
   return m_pQueryEngine->Run( this, i, result ); 
   }

int Query::Select( bool clear ) //, bool redraw )
   {
   return m_pQueryEngine->SelectQuery( this, clear ); //, redraw );
   }



bool Query::GetConditionsText( CString &text, int options )
   {
   text.Empty();

   // have root, recursively build up expression
   GetConditionsText( m_pRoot, text, 0, options );

   return true;
   }


int Query::GetConditionsText( QNode *pNode, CString &text, int indentLevel, int options )
   {
#ifdef NO_MFC
     using namespace std; //for max
#endif
   if ( pNode == NULL ) // no changes needed for null node
      return 0;

   CString leftText, currentText, rightText;
   int leftBranches  = 0;
   int rightBranches = 0;
   int leftType  = -1;
   int rightType = -1;

   if ( pNode->m_nodeType == OR )
      indentLevel++;

   if ( pNode->m_pLeft != NULL )
      {
      leftBranches = GetConditionsText( pNode->m_pLeft, leftText, indentLevel, options )+1;
      leftType     = pNode->m_pLeft->m_nodeType;
      }

   if ( pNode->m_pRight != NULL )
      {
      rightBranches = GetConditionsText( pNode->m_pRight, rightText, indentLevel, options )+1;
      rightType     = pNode->m_pRight->m_nodeType;
      }

   pNode->GetLabel( currentText, options );

   if ( pNode->m_nodeType != leftType && leftBranches > 2 && (options & NO_SHOW_PARENTHESES) == 0 )
     text.AppendFormat( "(%s)", leftText.GetBuffer() );
   else
      text += leftText;

   if ( pNode->m_nodeType == AND  || pNode->m_nodeType == OR )
      {
      if ( options & INCLUDE_NEWLINES )
         {
         if ( options & USE_HTML )
            text += "</br>";
         else
            text += " \r\n";
         }
      else
         text += ' ';

      // indent?
      if ( pNode->m_nodeType == OR && options & INCLUDE_NEWLINES )
         {
         for ( int i=0; i < indentLevel; i++ )
            {
            if ( options & USE_HTML )
               text += "&nbsp;&nbsp;&nbsp;";
            else
               text += "\t";
            }
         }
      }
   else
      text += ' ';
  
   text += currentText;
   text += ' ';

   // handle special cases:
   // first - EQ node with field on left and value on right
   if ( ( pNode->m_nodeType == EQ || pNode->m_nodeType == NE ) 
        && leftType == FIELD 
        && ( rightType == INTEGER || rightType == STRING ) 
        && (options & REPLACE_ATTRS ) )
      {
      // get field and replace attribute if appropriate fieldInfo exists
      QNode *pLeftNode = pNode->m_pLeft;
      MapLayer *pLayer = (MapLayer*) pLeftNode->m_extra1;
      int col = (int) pLeftNode->m_extra;
      LPCTSTR fieldname = pLayer->GetFieldLabel( col );
      MAP_FIELD_INFO *pInfo = pLayer->FindFieldInfo( fieldname );
      if ( pInfo )
         {
         FIELD_ATTR *pAttr = NULL;
         CString value;
         if ( rightType == INTEGER )
            {
            int _value;
            pNode->m_pRight->m_value.GetAsInt( _value );
            pAttr = pInfo->FindAttribute( _value );
            if( pAttr )
               value.Format( "%i", _value );
            }
         else  // STRING
            {
            pNode->m_pRight->m_value.GetAsString( value );
            pAttr = pInfo->FindAttribute( value );
            }

         if( pAttr != NULL )
            {
            if ( options & USE_HTML )
               {
               CString rep( "<i>" );
               rep += pAttr->label + "</i>";
               rightText.Replace( value, rep );
               }
            else
               rightText.Replace( value, pAttr->label );
            }
         }      
      }

   // second - parentheses needed?
   if ( pNode->m_nodeType != rightType && rightBranches > 2 && (options & NO_SHOW_PARENTHESES) == 0 )
     text.AppendFormat( "(%s)", rightText.GetBuffer() );
   else
      text += rightText;

   return max( leftBranches, rightBranches );
   }


    
//------------------------ QueryEngine methods ----------------------//
    
QueryEngine::QueryEngine( MapLayer *pLayer )
 : m_pCurrentQuery( NULL ),
   m_fpLogFile( NULL ),
   //m_debug( false ),
   m_showRuntimeError( true ),
   m_pMapLayer( pLayer ),
   m_pDeltaArray( NULL )
   {
   _gpQueryEngine = this;
   }
    
    
QueryEngine::~QueryEngine( void )
   {
   Clear();
   }


int QueryEngine::LoadQueryFile( LPCTSTR filename, bool clear )
   {
   _gpQueryEngine = this;

   if ( clear )
      Clear();
    
#ifndef NO_MFC
   // Engine reset and cleared if necessary, start loading rules
   int handle;
   _sopen_s( &handle, filename, _O_RDONLY, _SH_DENYNO, _S_IREAD | _S_IWRITE );
   if ( handle < 0 )
      {
      CString msg( "Unable to open file: " );
      msg += filename;
      Report::ErrorMsg( msg );
      return false;
      }

   // file is open, start reading/parsing, load entire file in memory
   int sizeOfFile = _filelength( handle );
   char *buffer = new char[ sizeOfFile+1 ];
   int end = _read( handle, (void*) buffer, sizeOfFile );
   buffer[ end ] = NULL;

   // if ( clear )
   //    reset = true;
   // else
   //    reset = false;
#else
   FILE* fp=fopen(filename,"r");
   if(!fp)
      {
      CString msg( "Unable to open file: " );
      msg += filename;
      Report::ErrorMsg( msg );
      return false;
      }

   struct stat st;
   stat(filename,&st);
   int sizeOfFile(st.st_size);
   char* buffer= new char[sizeOfFile+1];
   fread(buffer,sizeof(char),sizeOfFile,fp);
   buffer[sizeOfFile]='\0';

#endif

   int startCount = GetSelectionCount();
   ParseQuery( buffer, 0, filename );     // parse the string using yyparse();

   int count = GetSelectionCount();
   count -= startCount;

   if ( count >= 0 )
      {
      //CString msg;
      //msg.Format( "Successfully loaded %d rules from %s", count, filename );
      //AfxGetMainWnd()->MessageBox( msg, "Success", MB_OK );
      }
   else
      {
      CString msg( "Errors encountered loading - no queries were loaded" );
      msg += filename;
      Report::WarningMsg( msg );
      }

   delete buffer;

#ifndef NO_MFC
   _close( handle );
#else
   fclose(fp);
#endif
   return count;    
   }


Query *QueryEngine::ParseQuery( LPCTSTR buffer, int lineNo, LPCTSTR source )
   {
   _gpQueryEngine = this;

   CString queryStr( buffer );

   QInitGlobals( lineNo, buffer, source );
   
   // handle special case of NULL or empty query
   if ( buffer == NULL || lstrlen( buffer ) == 0 )
      {
      QNode *pNode = new QNode( 1 );   // "True"
      Query *pQuery = AddQuery( pNode );
      return pQuery;
      }

   gQueryBuffer = (char*) buffer;

   FILE *debugStream = NULL;

   if ( m_debug )
      {
      CString path( PathManager::GetPath( 0 ) );
      path += "qdebug.txt";      
      freopen_s( &debugStream, path, "w", stdout ); // Reassign "stdout" to "freopen.out": 

      m_debugInfo.Empty();
      }
      
   int retVal = yyparse();     // run the parser on the file  (0=success, 1=parse failure)

   if ( m_debug && debugStream != NULL )
      fclose( debugStream );

   if ( retVal == 0 )
      {
      Query *pQuery = GetLastQuery();

      if ( pQuery == NULL )
         return NULL;

      if ( pQuery->m_text.IsEmpty() )
         pQuery->m_text = queryStr; 
      
      return pQuery;
      }
   else 
      return NULL;
   }


//-- QueryEngine::Clear() -----------------------------------------------
//
// Clears out all variable, rules, constraints, fuzzysets, and dbmaps
//----------------------------------------------------------------------

bool QueryEngine::Clear( void )
   {
   m_queryArray.RemoveAll();
   return true;
   }


bool QueryEngine::RemoveQuery( Query *pQuery )
   {
   for ( int i=0; i < m_queryArray.GetSize(); i++ )
      {
      if ( m_queryArray[ i ] == pQuery )
         {
         m_queryArray.RemoveAt( i );
         pQuery = NULL;
         return true;
         }
      }
   
   return false;
   }


QFNARG QueryEngine::AddFunctionArgs(int functionID )  // e.g. Next()
   {
   _gpQueryEngine = this;

   QArgList *pArgList = new QArgList;
   //pArgList->Add(QArgument(functionID, NULL));

   m_argListArray.Add(pArgList);

   QFNARG fnArg;
   fnArg.function = functionID;
   fnArg.pArgList = pArgList;

   if (m_debug)
      {
      CString msg;
      msg.Format("AddFunctionArgs(functionID=%i) + %i arguments\n", functionID, (int)pArgList->GetSize());
      m_debugInfo += msg;
      }

   return fnArg;
   }


QFNARG QueryEngine::AddFunctionArgs(int functionID, int polyIndex)  // eg. ?
   {
   _gpQueryEngine = this;

   QArgList *pArgList = new QArgList;
   QArgument arg(functionID, polyIndex);
   pArgList->Add(arg);

   m_argListArray.Add(pArgList);

   QFNARG fnArg;
   fnArg.function = functionID;
   fnArg.pArgList = pArgList;

   if (m_debug)
      {
      CString msg;
      msg.Format("AddFunctionArgs(functionID=%i,polyIndex=%i) + %i arguments\n", functionID, polyIndex, (int)pArgList->GetSize());
      m_debugInfo += msg;
      }

   return fnArg;
   }


QFNARG QueryEngine::AddFunctionArgs( int functionID, QNode *pNode )
   {
   _gpQueryEngine = this;

   QArgList *pArgList = new QArgList;
   QArgument arg(functionID, pNode);
   pArgList->Add(arg);

   m_argListArray.Add( pArgList );

   QFNARG fnArg;
   fnArg.function = functionID;
   fnArg.pArgList = pArgList;

   if (m_debug)
      {
      CString label;
      pNode->GetLabel(label);
      CString msg;
      msg.Format("AddFunctionArgs(functionID=%i,Qnode=%s) + %i arguments\n", functionID, (LPCTSTR) label, (int)pArgList->GetSize());
      m_debugInfo += msg;
      }

   return fnArg;
   }

QFNARG QueryEngine::AddFunctionArgs(int functionID, QNode *pNode0, QNode *pNode1)
   {
   _gpQueryEngine = this;

   QArgList *pArgList = new QArgList;
   QArgument arg0(functionID, pNode0);
   QArgument arg1(functionID, pNode1);
   pArgList->Add(arg0);
   pArgList->Add(arg1);

   m_argListArray.Add(pArgList);

   QFNARG fnArg;
   fnArg.function = functionID;
   fnArg.pArgList = pArgList;

   if (m_debug)
      {
      CString label0;
      pNode1->GetLabel(label0);
      CString label1;
      pNode1->GetLabel(label1);
      CString msg;
      msg.Format("AddFunctionArgs(functionID=%i,Qnode0=%s,QNode1=%s) + %i arguments\n", functionID, (LPCTSTR)label0, (LPCTSTR)label1, (int)pArgList->GetSize());
      m_debugInfo += msg;
      }

   return fnArg;
   }


QFNARG QueryEngine::AddFunctionArgs( int functionID, QNode *pNode, double value )
   {
   _gpQueryEngine = this;

   QArgList *pArgList = new QArgList;
   QArgument arg0(functionID, pNode);
   QArgument arg1(value);
   pArgList->Add(arg0);
   pArgList->Add(arg1);

   m_argListArray.Add( pArgList );

   QFNARG fnArg;
   fnArg.function = functionID;
   fnArg.pArgList = pArgList;

   if (m_debug)
      {
      CString label;
      pNode->GetLabel(label);
      CString msg;
      msg.Format("AddFunctionArgs(functionID=%i,Qnode=%s,value=%lf) + %i arguments\n", functionID, (LPCTSTR)label, value, (int)pArgList->GetSize());
      m_debugInfo += msg;
      }
   
   return fnArg;
   }


// USERFN2
QFNARG QueryEngine::AddFunctionArgs(int functionID, int arg0, int arg1)
   {
   _gpQueryEngine = this;

   QArgList* pArgList = new QArgList;
   QArgument _arg0(arg0);
   QArgument _arg1(arg1);

   if (functionID == USERFN2)
      {
      QArgument _fnIndex(gUserFnIndex);
      pArgList->Add(_fnIndex);
      }

   pArgList->Add(_arg0);
   pArgList->Add(_arg1);

   m_argListArray.Add(pArgList);

   QFNARG fnArg;
   fnArg.function = functionID;
   fnArg.pArgList = pArgList;

   if (functionID == USERFN2)
      {

      }

   //fnArg
   if (m_debug)
      {
      CString msg;
      msg.Format("AddFunctionArgs(functionID=%i,arg0=%i,arg1=%i)\n", functionID, arg0, arg1);
      m_debugInfo += msg;
      }

   return fnArg;
   }




QFNARG QueryEngine::AddFunctionArgs( int functionID, QNode *pNode, double value, double value2 )
   {
   _gpQueryEngine = this;

   QArgList *pArgList = new QArgList;
   QArgument arg0(functionID, pNode);
   QArgument arg1(value);
   QArgument arg2(value2);

   pArgList->Add(arg0);
   pArgList->Add(arg1);
   pArgList->Add(arg2);

   m_argListArray.Add( pArgList );

   QFNARG fnArg;
   fnArg.function = functionID;
   fnArg.pArgList = pArgList;

   if (m_debug)
      {
      CString label;
      pNode->GetLabel(label);
      CString msg;
      msg.Format("AddFunctionArgs(functionID=%i,Qnode=%s,value1=%lf, value2=%lf) + %i arguments\n", functionID, (LPCTSTR)label, value, value2, (int)pArgList->GetSize());
      m_debugInfo += msg;
      }

   return fnArg;
   }

int QueryEngine::AddValue(int value)
   { 
   VData vv(value); 
   m_tempValueSet.Add( vv ); 

   if (m_debug)
      {
      CString msg;
      msg.Format("AddValue(int value=%i)\n", value);
      m_debugInfo += msg;
      }

   return value; 
   }


double QueryEngine::AddValue(double value)
   {
   VData vv(value); 
   m_tempValueSet.Add(vv ); 
   
   if (m_debug)
      {
      CString msg;
      msg.Format("AddValue(double value=%lf)\n", value);
      m_debugInfo += msg;
      }

   return value; 
   }

LPCTSTR QueryEngine::FindString( LPCTSTR label, bool add )
   {
   int count = (int) m_stringArray.GetSize();

   for ( int i=0; i < count; i++ )
      {
      if ( m_stringArray[ i ].CompareNoCase( label ) == 0 )  // found?
         return m_stringArray[ i ];
      }

   // if we get here, it wasn't found.  Should we add it?
   if ( add )
      {
      int offset = (int) m_stringArray.Add( label );
      return m_stringArray[ offset ];
      }
   else
      return NULL;
   }


bool QueryEngine::Run( Query *pQuery, int record, bool &result )
   {
   _gpQueryEngine = this;
   ASSERT( this == pQuery->m_pQueryEngine );

   m_currentRecord = record;
   m_pCurrentQuery = pQuery;

   return pQuery->Solve( result );
   }

/*
bool QueryEngine::Run( Query *pQuery, int record, int startPeriod, int endPeriod, int windowStartOffset, int windowEndOffset, DeltaArray *pDA, bool &result )  // spatiotemporal
   {
   // spatiotemporal queries check the spatial query at various points in time
   // subject to temporal constraints and including temporal operators
   //
   // temperal operators:
   //   delta( field, delta,  )  check for changes in 'field' - continuous variable
   //   delta( field, start, end )  checks for cases where 
   //
   //   query: delta( TEMP, 1 ) > 10    ' continuous variable, returns value
   //          delta( LULC_A, *, 1, 2 ) ' LULC_A changed from anything (*) to 1 - discreet variable, returns boolean
   //
   //

   return true;
   }
   */

void QueryEngine::ShowCompilerErrors( bool show )
   { 
   QShowCompilerErrors = show;
   }


int QueryEngine::SelectQuery( Query *pQuery, bool clear ) //, bool redraw )
   {
   showWarnings = true;

   _gpQueryEngine = this;

   ASSERT( m_pMapLayer != NULL );

   m_pCurrentQuery = pQuery;

   if ( clear )
      m_pMapLayer->ClearSelection();

   if ( pQuery == NULL )   // NULL query selects all
      {
      m_pMapLayer->SetSelectionSize( m_pMapLayer->GetRecordCount() );
      for ( MapLayer::Iterator i = m_pMapLayer->Begin(); i != m_pMapLayer->End(); i++ )
         m_pMapLayer->SetSelection( i, i );
      }
   else
      {
      for ( MapLayer::Iterator i = m_pMapLayer->Begin(); i != m_pMapLayer->End(); i++ )
         {
         m_currentRecord = i;
         bool result = false;
         bool ok = pQuery->Solve( result );

         if ( ok && result )
            m_pMapLayer->AddSelection( i );
         }
      }

//   if ( redraw )
//      m_pMapLayer->m_pMap->RedrawWindow();

   showWarnings = true;

   return m_pMapLayer->GetSelectionCount();
   }



void QueryEngine::RuntimeError( LPCTSTR _msg )
   {
   if ( m_showRuntimeError )
      {
#ifndef NO_MFC
      CString msg;
      msg.Format( "%s while processing query %s.  Select <Cancel> to disable this message...", _msg, (LPCTSTR) m_pCurrentQuery->m_text );
      if ( AfxGetMainWnd()->MessageBox( msg, "Runtime Error", MB_OKCANCEL ) == IDCANCEL )
         m_showRuntimeError = false;
#else
      //some linux equivalent
#endif
      }
   }


int QueryEngine::AddError( int lineNo, LPCTSTR msg ) 
   { 
   QUERY_COMPILER_ERROR e;
   int index = (int) m_errorArray.Add( e );
   m_errorArray[ index ].m_errmsg = msg;
   m_errorArray[ index ].m_lineNo = lineNo;
   m_errorArray[ index ].m_pQuery = m_pCurrentQuery;

   return index+1;
   }
/*
int QueryEngine::AddMapLayer( MapLayer *pLayer )
   {
   m_mapLayerArray.Add( pLayer );
   m_pMapLayer = pLayer;
   return m_mapLayerArray.GetSize();
   }
*/

QExternal *QueryEngine::FindExternal( LPCTSTR id )
   {
   for ( int i=0; i < (int) m_externalArray.GetSize(); i++ )
      {
      if ( lstrcmpi( m_externalArray[ i ]->m_identifier, id ) == 0 )
         return m_externalArray[ i ];
      }

   return NULL;
   }


void QueryEngine::InitGlobals( int lineNo, LPCTSTR queryStr, LPCTSTR source )
   {
   QInitGlobals( lineNo, queryStr, source );
   }
