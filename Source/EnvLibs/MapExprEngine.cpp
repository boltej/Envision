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

#include "MapExprEngine.h"

MTDOUBLE MTimeFct::evaluate(unsigned int nbArgs, const MTDOUBLE *pArg) { return m_pEngine->m_currentTime; }
MTDOUBLE MExpFct::evaluate(unsigned int nbArgs, const MTDOUBLE *pArg) { return exp(*pArg); }

//float MapExprEngine::m_currentTime = 0;

bool MapExpr::Evaluate( int polyIndex /*=-1*/ )
   {
   if ( m_pEngine == NULL )
      return false;

   if ( polyIndex >= 0 )
      m_pEngine->SetCurrentRecord( polyIndex );

   return m_pEngine->EvaluateExpr( this );
   }


MapExprEngine::MapExprEngine( void  ) 
: m_pLayer( NULL )
, m_pQueryEngine( NULL )
, m_currentTime( 0 )
, m_currentRecord( -1 )
, m_parserVars( NULL )
   {
   }

MapExprEngine::MapExprEngine( MapLayer *pLayer, QueryEngine *pQueryEngine ) 
: m_pLayer( pLayer )
, m_pQueryEngine( pQueryEngine )   // memory managed elsewhere
, m_currentTime( 0 )
, m_currentRecord( -1 )
, m_parserVars( NULL )
   {
   m_parserVars = new MTDOUBLE[ pLayer->GetFieldCount() ];
   }


MapExprEngine::~MapExprEngine(void)
   {
   if ( m_parserVars != NULL )
      delete [] m_parserVars;
   }



int MapExprEngine::SetMapLayer( MapLayer *pLayer )
   {
   m_pLayer = pLayer;

   if ( m_parserVars != NULL )
      delete [] m_parserVars;

   int fieldCount = pLayer->GetFieldCount();
   int varCount   = (int) m_varArray.GetSize();

   m_parserVars = new MTDOUBLE[ fieldCount + varCount ];

   return pLayer->GetFieldCount();
   }


MapExpr *MapExprEngine::AddExpr( LPCTSTR m_name, LPCTSTR expr, LPCTSTR query /*=NULL*/ )
   {
   MapExpr *pExpr = new MapExpr( this, m_name, query, expr );
   m_exprArray.Add( pExpr );

   return pExpr;
   }


bool MapExprEngine::Compile( MapExpr *pExpr, LPCTSTR source /*=NULL*/ )
   {
   pExpr->m_global = true;  // assume global unless determined not to be below

   LPCTSTR _source = source;
   if ( source == NULL )
      _source = "Map Expression";

   // process query
   if ( m_pQueryEngine && ! pExpr->m_queryStr.IsEmpty() )
      {
      pExpr->m_pQuery = m_pQueryEngine->ParseQuery( pExpr->m_queryStr, -1, _source );
      if ( pExpr->m_pQuery )    // successful compile?
         pExpr->m_pQuery->m_text = pExpr->m_queryStr;
      else
         {
#ifndef NO_MFC
         CString msg( "Unable to compile map expression query: " );
         msg += pExpr->m_queryStr;
         AfxMessageBox( msg );
#endif
         pExpr->m_pQuery = NULL;
         }
      }
   else    // no query specified, assume everywhere
      {
      pExpr->m_pQuery = NULL;
      }

   // compile this formula
   try 
      {
      pExpr->m_parser.enableAutoVarDefinition( true );     // let the compiler create internal variables
      pExpr->m_parser.compile( pExpr->m_expression );

      pExpr->m_isCompiled = true;

      // iterate through the generated parser variables. for each variable, 
      // verify that it is either a field name, 
      int varCount = pExpr->m_parser.getNbUsedVars();
      if ( varCount > 0 )
         {
         for ( int i=0; i < varCount; i++ )
            {
            std::string str = pExpr->m_parser.getUsedVar( i );
            LPCTSTR cstr = str.c_str();
            CString var( cstr );

            // is it a field column in the attached MapLayer?
            int col = m_pLayer->GetFieldCol( var );

            if ( col < 0 )  // not recognized as a column name
               {            // is it a MapExpr?
               MapExpr *pMapExpr = FindMapExpr( var );

               if ( pMapExpr != NULL )
                  {
                  pExpr->m_parser.redefineVar( var, &(pMapExpr->m_value) );
                  return true;    // No more required
                  }
               else
                  {
                  // not a MapExpr, is it a Map Variable?
                  MapVar *pMapVar = FindMapVar( var );

                  if ( pMapVar != NULL )
                     {
                     pExpr->m_parser.redefineVar( var, pMapVar->m_pVar );
                     return true;
                     }
                  else
                     {
#ifndef NO_MFC
                     CString msg( _T("MapExpr Evaluator: Unrecognized variable found: '") );
                     msg += var;
                     msg += "' - MapExpr Evaluator will ignore this m_expression...";

                     AfxMessageBox( msg );
#endif
                     break;
                     }
                  }
               }

            // have we seen this one before?
            bool found = false;
            for ( UINT j=0; j < (UINT) m_usedParserVars.GetSize(); j++ )
               {
               if ( m_usedParserVars[ j ] == col )
                  {
                  found = true;
                  break;
                  }
               }

            if ( ! found )
               m_usedParserVars.Add( col );
            
            pExpr->m_global = false;
            pExpr->m_parser.redefineVar( var, &(m_parserVars[col]));
            }
         }
      }
   catch (...)
      {
#ifndef NO_MFC
      CString msg( _T("Error encountered parsing m_expression: ") );
      msg += pExpr->m_expression;
      msg += _T("... setting to zero" );
      AfxMessageBox( msg );
#endif
      pExpr->m_parser.compile( _T("0") );
      }
   
   return true;
   }


int MapExprEngine::CompileAll( LPCTSTR source /*=NULL*/ )
   {
   for ( int i=0; i < (int) m_exprArray.GetSize(); i++ )
      {
      MapExpr *pExpr = m_exprArray[ i ];
      Compile( pExpr, source );
      }

   return (int) m_exprArray.GetSize();
   }


int MapExprEngine::SetCurrentRecord( int recordIndex )
   {
   if ( m_currentRecord == recordIndex )
      return (int) m_usedParserVars.GetSize();

   m_currentRecord = recordIndex;

   if ( m_pLayer == NULL )
      return -1;

   int count = 0;
   for ( int i=0; i < (int) m_usedParserVars.GetSize(); i++ )
      {
      int col = m_usedParserVars[ i ];
      ASSERT( col >= 0 );

      if ( col >= 0  )
         {
         if ( m_pLayer->GetData( recordIndex, col, m_parserVars[ col ] ) != false )
            count++;
         }
      }

   return count;
   }


bool MapExprEngine::RemoveExpr( MapExpr *pExpr )
   {
   int count =  GetExprCount();

   for ( int i=0; i < count; i++ )
      {
      MapExpr *_pExpr = GetExpr( i );

      if ( pExpr == _pExpr )  // found it?
         {
         m_exprArray.RemoveAt( i );   // deletes pExpr
         return true;
         }
      }

   return false;
   }



bool MapExprEngine::EvaluateExpr( MapExpr *pExpr, bool useQuery /*=false*/ )
   {
   if ( pExpr->m_isCompiled == false )
      {
      bool ok = Compile( pExpr );
      if ( ! ok )
         return false;
      }

   if ( useQuery && pExpr->m_pQuery != NULL )
      {
      bool result = false;
      bool ok = pExpr->m_pQuery->Run( m_currentRecord, result );

      if ( !ok || !result )
         {
         pExpr->m_value = 0;
         pExpr->m_isValid = false;
         return false;
         }
      }

   // otherwise evaluate...
   double value;
   if ( pExpr->m_parser.evaluate( value ) == false )
      {
      pExpr->m_value = 0;
      pExpr->m_isValid = false;
      return false;
      }

   pExpr->m_value = (float) value;
   pExpr->m_isValid = true;

   return true;
   }


int MapExprEngine::EvaluateAll( bool useQueries /*=false*/ )
   {
   int count = 0;

   // order - whatever order they are added, first in goes first
   for ( int i=0; i < (int) m_exprArray.GetSize(); i++ )
      {
      MapExpr *pExpr = m_exprArray[ i ];
      if ( EvaluateExpr( pExpr, useQueries ) )
         count++;
      }

   return count;
   }


MapExpr *MapExprEngine::FindMapExpr( LPCTSTR name )
   {
   for ( int i=0; i < (int) m_exprArray.GetSize(); i++ )
      {
      if ( m_exprArray[ i ]->m_name.CompareNoCase( name ) == 0 )
         return m_exprArray[ i ];
      }

   return NULL;
   }


MapVar *MapExprEngine::FindMapVar( LPCTSTR name )
   {
   for ( int i=0; i < (int) m_varArray.GetSize(); i++ )
      {
      if ( m_varArray[ i ]->m_name.CompareNoCase( name ) == 0 )
         return m_varArray[ i ];
      }

   return NULL;
   }
