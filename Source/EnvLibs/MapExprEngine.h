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
#include "EnvLibs.h"

#include "QueryEngine.h"
#include "Maplayer.h"
#include "mtparser/MTParser.h"
#include "PtrArray.h"


//----------------------------------------------------------------------------
// Usage:
// 1) Create an instance of a MapExprEngine, passing a MapLayer to the constructor
//
// 2) Add expressions as needed using MapExprEngine::AddExpression()
//
// 3) Compile the expression using MapExprEngine::Compile( pExpr );
//    Alternatively, you can compile all the expressions at once by calling
//    MapExprEngine::CompileAll() after adding all the expressions
//
// 4) Evaluation is tied to a specific maplayer record, so to evaluate, do something like:
//
//  for ( MapLayer::Iterator idu=pLayer->Begin(); idu < pLayer->End(); idu++ )
//     {
//     mapExprEngine.SetCurrentRecord( idu );
//     mapExprEngine.EvaluateAll( true );
//
//     for ( int j=0; j < mapExprEngine.GetExprCount(); j++ )
//        {
//        MapExpr *pExpr = mapExprEngine->GetExpr( j );
//        if ( pExpr->IsValid() )
//            {
//            float value = pExpr->GetValue();
//            // do something with the value....
//            }
//        }   // end of: for ( mapExprEngine.GetExprCount() )
//     }  // end of: for ( MapLayer::Iterator )
//
//  Note:  You can also:
//    1) Attach data to an expression using the MapExpr::m_extra member
//    2) Evaluate individual expressions using MapExprEngine::EvaluateExpr()
//-----------------------------------------------------------------------------

class MapExprEngine;

class MapVar
{
public:
   CString   m_name;
   MTDOUBLE *m_pVar;

   MapVar( LPCTSTR name, MTDOUBLE *pVar ) : m_name( name ), m_pVar( pVar ) { }
};


class LIBSAPI MTimeFct : public MTFunctionI
{
public:
   MTimeFct( MapExprEngine *pEngine ) : MTFunctionI(), m_pEngine( pEngine ) { }

   MapExprEngine *m_pEngine;

   virtual const MTCHAR* getSymbol() { return _T("time"); }

   virtual const MTCHAR* getHelpString(){ return _T("time()"); }
   virtual const MTCHAR* getDescription()
       { return _T("Return the current simulation time"); }
   virtual int getNbArgs(){ return 0; }
   virtual MTDOUBLE evaluate(unsigned int nbArgs, const MTDOUBLE *pArg);
        
   virtual MTFunctionI* spawn(){ return new MTimeFct( m_pEngine ); }

   virtual bool isConstant(){ return false; }
};    

class LIBSAPI MExpFct : public MTFunctionI
{
public:
   virtual const MTCHAR* getSymbol() { return _T("exp"); }

   virtual const MTCHAR* getHelpString(){ return _T("exp()"); }
   virtual const MTCHAR* getDescription()
       { return _T("Return exp"); }
   virtual int getNbArgs(){ return 1; }
   virtual MTDOUBLE evaluate(unsigned int nbArgs, const MTDOUBLE *pArg);
        
   virtual MTFunctionI* spawn(){ return new MExpFct(); }

   virtual bool isConstant(){ return false; }
};      


class MEParser : public MTParser
{
public:
   MEParser( MapExprEngine *pEngine ) : MTParser() 
      {
      this->defineFunc(new MTimeFct( pEngine));
      this->defineFunc(new MExpFct());
      } 
};


class LIBSAPI MapExpr
{
friend class MapExprEngine;

public:
   CString m_name;
   INT_PTR m_extra;

   bool IsGlobal( void ) { return m_global; }
   bool IsFieldBased( void ) { return m_global ? false : true; }
   bool SetExpression( LPCTSTR expr ); // { m_expression = expr;  return m_pEngine->Compile( this ); }

   bool   Compile( LPCTSTR source=NULL ); 
   bool   Evaluate( int polyIndex=-1 ); 
   bool   IsValid( void )  { return m_isValid; }
   double GetValue( void ) { return m_isValid ? m_value : 0; }
   void   SetValue( float value ) { m_isValid = true; m_value = (MTDOUBLE) value; }

   int   SetCurrentRecord( int polyIndex ); // { return m_pEngine->SetCurrentRecord( polyIndex ); }
   MapExprEngine *GetMapExprEngine( void ) { return this->m_pEngine; }

   Query* GetQuery() { return m_pQuery; }

protected:
   CString m_queryStr;
   Query  *m_pQuery;

   CString   m_expression;
   MTDOUBLE  m_value;
   bool      m_isValid;
   bool      m_isCompiled;
   bool      m_global;       // does the expression contain column references?

   MapExprEngine *m_pEngine;
   MEParser m_parser;         // each expression has it's own parser
      
   MapExpr( MapExprEngine *pEngine) : m_extra( 0 ), m_value( 0.0f ), m_isValid( false ), m_isCompiled( false ), m_global( true ), 
      m_pEngine( pEngine), m_pQuery( NULL ), m_parser( pEngine ) { }
   
   MapExpr( MapExprEngine *pEngine, LPCTSTR _name, LPCTSTR query, LPCTSTR expr ) 
      : m_extra( 0 ), m_value( 0 ), m_isValid( false ), m_isCompiled( false ), m_global( true ), m_pEngine( pEngine), m_parser( pEngine),
      m_name( _name ), m_queryStr( query ), m_expression( expr ), m_pQuery( NULL ) { }

   };


class LIBSAPI  MapExprEngine
{
public:
   MapExprEngine( void );        // requires SetMapLayer() before use
   MapExprEngine( MapLayer *pLayer, QueryEngine *pQueryEngine );
   //MapExprEngine( MapExprEngine &m) { *this = m; }
   ~MapExprEngine(void);

   //MapExprEngine &operator = (MapExprEngine &m);

   int SetMapLayer( MapLayer* );  // nust be set for void constructor
   MapLayer *GetMapLayer( void ) { return this->m_pLayer; }
   
   MapVar *AddVariable( LPCTSTR name, MTDOUBLE *pVar ) { MapVar *pMapVar = new MapVar( name, pVar ); m_varArray.Add( pMapVar ); return pMapVar; }
   MapVar *FindMapVar( LPCTSTR name );

   MapExpr *AddExpr( LPCTSTR name, LPCTSTR exprStr, LPCTSTR queryStr=NULL );
   bool RemoveExpr( MapExpr *pExpr ); 

   bool  Compile( MapExpr *pExpr, LPCTSTR source=NULL );
   int   CompileAll( LPCTSTR source=NULL );

   int   SetCurrentRecord( int polyIndex );
   float SetCurrentTime( float time ) { return m_currentTime = time; }

   bool  EvaluateExpr( MapExpr *pExpr, bool useQuery=false );
   int   EvaluateAll( bool useQuerys=false );

   int      GetExprCount( void ) { return (int) m_exprArray.GetSize(); }
   MapExpr *GetExpr( int i ) { return m_exprArray[ i ]; }
   MapExpr *FindMapExpr( LPCTSTR name );

   int   GetUsedColCount( void ) { return (int) m_usedParserVars.GetSize(); }
   int   GetUsedCol( int index ) { return (int) m_usedParserVars[ index ]; }

   static bool IsConstant(LPCTSTR expr);
   
   float m_currentTime;      // this must be set using SetCurrentTime(); 
 
protected:
   PtrArray< MapExpr > m_exprArray;     // deletes MapExprs when done, so clients shouldn't.
   PtrArray< MapVar > m_varArray;       // externally added variables

   int        m_currentRecord;      // polygon index currently beign evaluated

   UINT       m_parserVarCount;
   MTDOUBLE  *m_parserVars;  // array of m_parser variable

   CUIntArray m_usedParserVars;  // an array of maplayer columns actually referenced by the expression set
   
   MapLayer    *m_pLayer;
   QueryEngine *m_pQueryEngine;    // memory managed elsewhere
};


inline
bool MapExpr::SetExpression( LPCTSTR expr )
   {
   m_expression = expr;
   return m_pEngine->Compile( this ); 
   }

inline
bool MapExpr::Compile( LPCTSTR source /*=NULL*/ )
   {
   return this->m_pEngine->Compile( this, source ); 
   }

inline
int MapExpr::SetCurrentRecord( int polyIndex )
   {
   return m_pEngine->SetCurrentRecord( polyIndex ); 
   }


inline
bool MapExprEngine::IsConstant(LPCTSTR expr)
   {
   // see the if outcomeExpr is a constant
   bool isConstant = true;
   //bool isInt = true;
   for (int i = 0; i < lstrlen(expr); i++)
      {
      if (!(isdigit(expr[i]) || expr[i] == '-' || expr[i] == '.'))
         {
         isConstant = false;
         break;
         }
      //else if (expr[i] == '.')
      //   isInt = false;
      }

   return isConstant;
   }