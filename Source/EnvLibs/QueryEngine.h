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
#if !defined _QUERYENGINE_H
#define _QUERYENGINE_H

#include "EnvLibs.h"

#include "Vdata.h"
#include "PtrArray.h"
#include "deltaArray.h"

#ifndef NO_MFC
#include <afxtempl.h>
#endif

class MapLayer;
class QueryEngine;
class Poly;
class QValueSet;
class QNode;

//extern   LIBSAPI  QueryEngine *gpQueryEngine;

extern bool QShowCompilerErrors;
extern void QInitGlobals( int lineNo, LPCTSTR queryStr, LPCTSTR source );

const int VALUESET = 10000;

// GetConditionsText() and GetLabel() options
const int INCLUDE_NEWLINES    = 0x01;
const int SHOW_LEADING_TAGS   = 0x02;
const int REMOVE_UNDERSCORES  = 0x04;
const int NO_SHOW_PARENTHESES = 0x08;
const int HUMAN_READABLE      = 0x10;
const int REPLACE_FIELDNAMES  = 0x20;
const int REPLACE_ATTRS       = 0x40;
const int USE_HTML            = 0x80;

const int ALL_OPTIONS         = INCLUDE_NEWLINES | SHOW_LEADING_TAGS | REMOVE_UNDERSCORES | HUMAN_READABLE | REPLACE_FIELDNAMES | REPLACE_ATTRS | USE_HTML;


// temporary structure for compiler use
class QArgList;
struct QFNARG
   {
   //QFNARG() : function( -1 ), pArgList( NULL ) { }
   //QFNARG( int _function, QArgList *_pArgList ) : function( _function ), pArgList( _pArgList ) { }

   int function;
   QArgList *pArgList;
   };


// define an external that is set at runtime outside of the QueryEngine.  
// These are defined by @identifer syntax
class QExternal
   {
   friend class QueryEngine;
   friend class QNode;

   public:
      CString m_identifier;

   protected:
      int     m_col;      // if this referneces a column, column number.  Otherwise, -1
      VData   m_value;     // should be a ptr to a value

   public:
      QExternal( LPCTSTR identifier, int col ) 
         : m_identifier( identifier ), m_col( col ), m_value( NULL ) { }

      void SetValue( VData &value ) { m_value = value; }
      void GetValue( VData &value ) { value = m_value; }
   };


class LIBSAPI QNode
   {
   public:
      VData  m_value;      // value of this node - depends on type (m_nodeType)
      QNode *m_pLeft;      // NULL for leaf (terminal node)
      QNode *m_pRight;     // NULL EXCEPT FOR FIELDS: QNode* pointing to field containing index or integer record index
      int    m_nodeType;   // what operation (if any) is associated with this node
      LONG_PTR m_extra;    // extra data, holds:
                           //   ptr to define for constants (ints, doubles, etc)
                           //   field column for FIELDS
                           //   QArgList* for Functions
                           //   pointer to QExternal structure for externals

      LONG_PTR m_extra1;   // more extra data, holds:
                           //    ptr to MapLayer for FIELDS

      // various constructors
      QNode( void ) : m_value( 0 ), m_pLeft( NULL ), m_pRight( NULL ),
         m_nodeType( -1 ), m_extra( 0 ), m_extra1( 0 ) { }

      // constructing from a logical or conditional expression
      QNode( QNode *pLeft, int op, QNode *pRight )
         : m_value( 0 ), m_pLeft( pLeft ), m_pRight( pRight ), m_nodeType( op ),
           m_extra( 0 ), m_extra1( 0 ) { }

      // contructing from a fieldRef (MapLayer column)
      //QNode( MapLayer *pLayer, int col, int op, QNode *pRight );
      QNode(MapLayer *pLayer, int col );    // record is defined by fieldRef
      QNode( MapLayer *pLayer, int col, QNode *pFieldRef );    // record is defined by another fieldRef
      QNode( MapLayer *pLayer, int col, INT_PTR record );      // record is defined as an explicit index

      // constructing from function
      QNode( QFNARG &fnArg );

      // constructing from a valueSet
      QNode( QValueSet* );

      // atomic expressions
      QNode( int value );        // from integer
      QNode( double value );     // from double
      QNode( char *str );        // from string
      QNode( QExternal *pExt );  // from an external

      // copy constructor
      QNode( QNode &node ) : m_value( node.m_value ), m_pLeft( NULL ), m_pRight( NULL ), m_nodeType( node.m_nodeType ),
            m_extra( node.m_extra ), m_extra1( node.m_extra1 ) { }

      QNode* DeepCopy() 
         {
         // Make a shallow copy
         QNode *pNode = new QNode( *this );

         // Now go deep
         if ( m_pLeft != NULL )
            pNode->m_pLeft = m_pLeft->DeepCopy();
         if ( m_pRight != NULL )
            pNode->m_pRight = m_pRight->DeepCopy();

         return pNode;
         }

      // destructor
      ~QNode( void );

      // methods
      bool Solve( void );
      bool IsTerminal( void ) { return m_pLeft == NULL; } // && m_pRight == NULL;
      //void Free( void );

      bool GetValueAsBool( bool &value ) { return m_value.GetAsBool( value ); }
      bool GetValueAsInt (int &value) { return m_value.GetAsInt(value); }
      bool GetValueAsDouble(double &value) { return m_value.GetAsDouble(value); }
      bool GetLabel( CString &label, int options=ALL_OPTIONS );

   protected:
      bool SolveLogical( void );
      bool SolveArithmetic( void );
      bool SolveRelational( void );

      void GetNeighbors( Poly *pPoly, CMap<int, int, bool, bool> &foundPolys );
   };



class LIBSAPI Query     // a query, consiting of a logical expression and additional flags
   {
   friend class QueryEngine;
   friend class QueryPtrArray;

   public:
      Query( QNode *pRoot, QueryEngine *pQueryEngine ) : m_solved( false ), m_value( false ), m_inUse( true ),
            m_pRoot( pRoot ), m_pQueryEngine( pQueryEngine ) { }

      Query( Query &query );

   public:
      ~Query( void ) { if ( m_pRoot != NULL ) delete m_pRoot; }  // Note: applications should use QueryEngine::RemoveQuery(); this destructor should not be called!!!
   
   protected:
      bool Solve( bool &result );
 
   public:
      bool GetConditionsText( CString &text, int options=ALL_OPTIONS );
      int  GetConditionsText( QNode *pNode, CString &text, int indentLevel, int options=ALL_OPTIONS );

      bool Run( int i, bool &result );   // run this query on the specified polygon
      int  Select( bool clear ); ///, bool redraw );

      QueryEngine *GetQueryEngine( void ) { return m_pQueryEngine; }

   public:
      bool m_solved;
      bool m_value;
      bool m_inUse;

      CString m_text;   // the actual string read from a file
      QNode *m_pRoot;    // pointer to the root node of the expression
   
   protected:
      QNode *CopyTree( QNode *pNodeToCopy );
      QueryEngine *m_pQueryEngine;
   };


class QArgument : public VData
   {    
   public:
      QNode *m_pNode;
      CString m_label;     // descriptive label of what this argument is
      int m_extra;

      QArgument( LPCTSTR i ) : VData( i ), m_pNode( 0 ), m_extra(0) { }
      QArgument( int    i ) : VData( i ), m_pNode( 0 ), m_extra(0) { }
      QArgument( double i ) : VData( i ), m_pNode( 0 ), m_extra(0) { }
      QArgument( bool   i ) : VData( i ), m_pNode( 0 ), m_extra(0) { }   
      QArgument( int function, QNode *pNode ) : VData ( function ), m_pNode( pNode ), m_extra(0) { }
      QArgument(int function, int polyIndex) : VData(function), m_pNode(NULL), m_extra( polyIndex) { }
      QArgument( void )     : VData(), m_pNode( 0 ), m_extra(0) { }

      QArgument( const QArgument &a ){ *this = a; }

      QArgument& operator= ( const QArgument &a ) 
         {
         *((VData*)this) = (VData)a;
         m_label = a.m_label;
         m_extra = a.m_extra;

         if ( a.m_pNode != NULL )
            m_pNode = a.m_pNode->DeepCopy();

         return *this;
         }

      ~QArgument()
         {
         if ( m_pNode != 0 )  // removed 12/7/10 - unsure if needed (jpb)
            delete m_pNode;
         }

      LPCTSTR GetAsString();
   };

class QArgList : public CArray< QArgument, QArgument& >
   {
   public:
      QArgList( QArgList &l ) { for ( int i=0; i < l.GetSize(); i++ ) Add( l.GetAt( i ) ); }
      QArgList() : CArray< QArgument, QArgument&>() { };   
   };


// QValueSet is a set of numbers
class QValueSet  : public CArray< VData, VData& >
{
public:
   QValueSet( void ) { }
   QValueSet( QValueSet &set ) { for ( int i=0; i < (int) set.GetSize(); i++ ) Add( set[ i ] ); }
   
};


struct QUERY_COMPILER_ERROR
   {
   CString   m_errmsg;
   Query    *m_pQuery;
   int      m_lineNo;
   QUERY_COMPILER_ERROR() { }
   QUERY_COMPILER_ERROR(const QUERY_COMPILER_ERROR &q) : m_errmsg(q.m_errmsg), m_pQuery(q.m_pQuery) { m_lineNo = q.m_lineNo; }
   };


class   LIBSAPI QueryEngine
   {
   friend class QNode;

   protected:
      Query *m_pCurrentQuery;    // the rule current being evaluated, -1 if none being evaluated...
      int    m_currentRecord;    // current polygon being evaluated.  Must be set prior to a query evaluation
      bool   m_showRuntimeError;
      FILE  *m_fpLogFile;
      static int m_currentTime;      // set externally with SetCurrentTime()

   public:
      static bool   m_debug;
      static CString m_debugInfo;
      static void AddDebugInfo(LPCTSTR info) { m_debugInfo += info; }

   public:
      MapLayer *m_pMapLayer;
      
   // temporal query support
   protected:
      DeltaArray *m_pDeltaArray;  // only for temporal queries

   public:
      void SetDeltaArray( DeltaArray *pDA ) { m_pDeltaArray = pDA; }


   // various collections 
   protected:
      //CArray< MapLayer*, MapLayer* > m_mapLayerArray;    // database tables

      PtrArray< Query > m_queryArray;

      PtrArray< QArgList > m_argListArray;    // store the per-query argLists (ptrs to ArgLists, one per fn ref in each query)

      PtrArray< QValueSet > m_valueSetArray;    // store the per-query value list for any defined valueSets e.g [1|2|4]
      QValueSet m_tempValueSet;
      
      CStringArray m_stringArray;   // should be replaced with a map...

      CArray< QUERY_COMPILER_ERROR, QUERY_COMPILER_ERROR& > m_errorArray;
      
      // externals management
      PtrArray< QExternal > m_externalArray;
      
   private:
      QueryEngine() /*: m_debug( false )*/ { }

   public:
      // constructors/destructor
      QueryEngine( MapLayer* );
      QueryEngine(const QueryEngine &q) { }
      ~QueryEngine();

      void ShowCompilerErrors( bool show );

      // run a query
      bool Run( Query *pQuery, int record, bool &result );
      //bool Run( Query *pQuery, int record, int startPeriod, int endPeriod, int windowStartOffset, int windowEndOffset, DeltaArray *pDA, bool &result );  // spatiotemporal

      int  SelectQuery( Query *pQuery, bool clear ); //, bool redraw );

      // query management
      bool   Clear( void );    // clears out everything (rules, constraints, fuzzysets, variables, strings...
      int    LoadQueryFile( LPCTSTR filename, bool clear );
      Query *GetQuery( int i ) { return m_queryArray[ i ]; }
      Query *GetLastQuery() { if ( m_queryArray.GetSize() == 0 ) return NULL;  else return m_queryArray[ m_queryArray.GetSize()-1 ]; }
      Query *AddQuery( Query *pQuery ) { m_queryArray.Add( pQuery ); return pQuery; }
      Query *AddQuery( QNode *pNode ) { Query *pQuery = new Query( pNode, this ); return AddQuery( pQuery ); }
      bool   RemoveQuery( Query* );
      int    GetSelectionCount( void ) { return (int) m_queryArray.GetSize(); }

      QFNARG AddFunctionArgs( int functionID);                                               // for TIME, INDEX (no arguments)
      QFNARG AddFunctionArgs( int functionID, QNode *pNode );                                // for NEXTTO, NEXTTOAREA
      QFNARG AddFunctionArgs( int functionID, int index );                                   // for POLYINDEX
      QFNARG AddFunctionArgs( int functionID, QNode *pNode0, QNode *pNode1);                 // for DELTA
      QFNARG AddFunctionArgs( int functionID, QNode *pNode, double value);                   // for WITHIN, MOVAVG
      QFNARG AddFunctionArgs( int functionID, QNode *pNode, double value, double value2 );   // for WITHINAREA

      QValueSet *CreateValueSet( void ) { QValueSet *pSet = new QValueSet( m_tempValueSet ); m_valueSetArray.Add( pSet ); m_tempValueSet.RemoveAll(); return pSet; }
      int    AddValue( int    value );  // { VData vv(value); m_tempValueSet.Add( vv ); return value; } 
      double AddValue( double value );  // { VData vv(value); m_tempValueSet.Add(vv ); return value; }

      int GetExternalCount( void ) { return (int) m_externalArray.GetSize(); }
      QExternal *GetExternal( int i ) { return m_externalArray[ i ]; }
      QExternal *AddExternal( LPCTSTR id, int col=-1 ) { QExternal *pExt = new QExternal( id, col ); m_externalArray.Add( pExt ); return pExt; }
      QExternal *FindExternal( LPCTSTR id );
      
      // error management
      void ClearErrors() { m_errorArray.RemoveAll(); }
      int  GetErrorCount() { return (int) m_errorArray.GetSize(); }
      QUERY_COMPILER_ERROR *GetError( int i ) { return &m_errorArray[ i ]; }
      int AddError( int lineNo, LPCTSTR msg );

      // string management
      LPCTSTR FindString( LPCTSTR label, bool add );

      // MapLayer management
      //int  AddMapLayer( MapLayer *pLayer );
      //MapLayer *GetMapLayer( int i=-1 ) { if ( i < 0 ) return m_pMapLayer; return m_mapLayerArray[ i ]; }
      MapLayer *GetMapLayer() { return m_pMapLayer; }
      void SetCurrentRecord( int record ) { m_currentRecord = record; }

      // miscellaneous
      static void SetCurrentTime(int time) { m_currentTime = time; }
      void RuntimeError( LPCTSTR msg );
      FILE *StartLog() { fopen_s( &m_fpLogFile, "queryengine.log", "wt" ); return m_fpLogFile; }
      void StopLog() { fclose( m_fpLogFile ); m_fpLogFile = NULL; }

   protected:   // helper functions for compiler - not for public use

   public:
      //void AddArgument( LPSTR  arg ) { m_tempArgBuffer.Add( QArgument( arg ) ); }
      //void AddArgument( int    arg ) { m_tempArgBuffer.Add( QArgument( arg ) ); }
      //void AddArgument( double arg ) { m_tempArgBuffer.Add( QArgument( arg ) ); }
      //void AddArgument( bool   arg ) { m_tempArgBuffer.Add( QArgument( arg ) ); }
      //void AddArgument( void )       { m_tempArgBuffer.Add( QArgument() );      }  // a NULL_VALUE argument

   public:
      void  InitGlobals( int lineNo, LPCTSTR queryStr, LPCTSTR source ); //  { QInitGlobals( lineNo, queryStr, source ); }
      //Query *ParseQuery( LPCTSTR buffer );  // parse a single query, and add it to the engine
      Query *ParseQuery( LPCTSTR queryStr, int lineNo, LPCTSTR source );  // parse a single query, and add it to the engine
   };

#endif

