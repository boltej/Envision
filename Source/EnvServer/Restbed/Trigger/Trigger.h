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


#include <EnvExtension.h>
#include <MapLayer.h>
#include <VData.h>
#include <mtparser\mtparser.h>
#include <queryEngine.h>

#include <afxtempl.h>

#define _EXPORT __declspec( dllexport )


class TriggerOutcome
{
public:
   int       m_colTarget;
   CString   m_targetExpr;
   MTParser  m_parser;
   float     m_probability;   // decimal percent 

   float m_value;      // output variable - results from parsing expression

   TriggerOutcome() : m_parser(), m_value( 0 ), m_probability( 1.0f ) { }
   ~TriggerOutcome() { /*if ( m_pParser != NULL ) delete m_pParser; */ }

   bool Compile( const MapLayer* );
   bool Evaluate();
};


// a TriggerOutcomeArray holds zero or more possible target outcomes for a trigger 
class TriggerOutcomeArray : public CArray< TriggerOutcome*, TriggerOutcome* >
{
public:
   TriggerOutcomeArray() : CArray< TriggerOutcome*, TriggerOutcome* >() { }
   ~TriggerOutcomeArray( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); RemoveAll(); }
};


// Trigger map values from a source column to corresponding value(s) in a target column
// It corresponds to the mappings for one autoProcess in the input file.
class Trigger
{
public:
   TriggerOutcomeArray m_outcomeArray;

   CString m_name;
   CString m_queryStr;
   Query  *m_pQuery;
   bool    m_use;    // exposed as scenario variable
   
   Trigger() :  m_pQuery( NULL ), m_use( true ) { }
   ~Trigger();

   bool CompileQuery();
};


// TriggerProcess - this corresponds to one .envx file entry and one input file
class TriggerProcess
{
friend class TriggerProcessCollection;

public:
   TriggerProcess( int id ) : m_processID( id ) { }
  ~TriggerProcess();

   bool LoadXml( LPCTSTR filename, const MapLayer *pLayer );
   bool Run    ( EnvContext *pContext );
   //bool UpdateVariables( int idu );     // sets parservars


public:
   int m_processID;     // model id specified in envx file
   CString m_filename;  // input file name

protected:
   CArray< Trigger*, Trigger* > m_triggerArray;
};


// this is a wrapper class for interfacing with Envision's AP extension
class _EXPORT TriggerProcessCollection : public EnvModelProcess
{
public:
   TriggerProcessCollection() : EnvModelProcess() { }
   ~TriggerProcessCollection();
      
   // overrides
   bool Init   ( EnvContext *pContext, LPCTSTR initStr /*xml input file*/ );
   bool InitRun( EnvContext *pContext, bool ) { return TRUE; }
   bool Run    ( EnvContext *pContext );   
  
   TriggerProcess *GetTriggerProcessFromID( int id, int *index=NULL );

   // variable maintenance
   static MapLayer *m_pMapLayer;
   static double   *m_parserVars;
   static CMap<int, int, int, int > m_usedParserVars;  // key and value is column index
   static QueryEngine *m_pQueryEngine;     // memory managed by EnvModel
   
   static bool UpdateVariables( int idu );

protected:
   CArray < TriggerProcess*, TriggerProcess* > m_triggerProcessArray;

};


extern "C" _EXPORT EnvExtension* Factory() { return (EnvExtension*) new TriggerProcessCollection; }

