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

#include <EnvExtension.h>
#include <Maplayer.h>
#include <FDATAOBJ.H>
#include <Vdataobj.h>

#include <Vdata.h>
#include <QueryEngine.h>
#include <MapExprEngine.h>
#include <tinyxml.h>
#include <PtrArray.h>

#include <sstream>

#define _EXPORT __declspec( dllexport )

using namespace std;

class OutputGroup;


/*
-------------------------------------------------------------------------------
Basic idea - Define individual outputs and output groups that report info

OutputGroups - exposes an FDataObj that is populated by the child outputs
Outputs - if inside a group, shows up as FDataObjs, if outside a group,
   shows up as individual output vars

if a "summarize_by" attribute is defined, then additional data is collected that
   that summarizes the data based on the field info that is provide for the 
   "summarize_by" field in the IDU coverage.  If "summarize_by" is defined at the 
   <output_group> level, all child <output>'s inherit this attribute

for "summarize_by" outputs:
   1) for <output>s that are INSIDE an <output_group>, an FDataObj is created (and exposed)
      for each child <output> - this dataobj contains a column for each summary.  The 
      name of the dataobj is {groupName}.{outputName}_by_{summarizeField}
   2) for <output>s that are OUTSIDE an <output_group>, an FDataObj is created (and exposed)
      for that output that contains a column for each summary.  The name of the dataobj 
      is {outputName}_by_{summarizeField}

if a "pivot_table" attribute is defined, then a "session" dataobj is used to create
   an alternative form output that is amenable to use with pivot tables. If "pivot_table" 
   is defined at the <output_group> level, all child <output>'s inherit this attribute.

for "pivot_table" outputs/outputGroups:
   1) for <output>s that are INSIDE an <output_group>, a single VDataObj (for the OutputGroup) is
      created that contains summaries for all contained <output> vars (one column per child 
      <output>. Individual records (rows) are used for each summary.  The name of the dataobj 
      is {groupName}_by_{summarizeField}_pivot.csv
   2) for <output>s that are OUTSIDE an <output_group>, a VDataObj is created 
      for that output that contains a column for each summary.  The name of the dataobj 
      is {outputName}_by_{summarizeField}_pivot.csv

----------------------------------------------------------------------------------
*/


enum VAR_TYPE {           //  Description                                              |      Extent         | useDelta?|
   VT_UNDEFINED    = 0,   //|----------------------------------------------------------|---------------------|-----------
   VT_PCT_AREA     = 1,   // "pctArea" - value defined as a percent of the total IDU   | always global extent|    no    |
                          // area that satisfies the specified query, expressed as a
                          // non-decimal percent.  No "value" attr specified.
   VT_SUM_AREA     = 2,   // "sum" sums the "value" expression over an area            | always global extent |   yes   |
                          //  defined by the query.  If "use_delta" specified, the 
                          //  query area is further restricted to those IDUs in which
                          //  a change, specified by the useDelta list, occured
                          // 
   VT_AREAWTMEAN   = 3,   // "areaWtMean" - area wt mean of the "value" expression,    | always global extent |   yes   |
                          // summed over the query area. If "use_delta" specified, the 
                          //  query area is further restricted to those IDUs in which
                          //  a change, specified by the useDelta list, occured 

   VT_LENWTMEAN = 4,      //

   VT_MEAN         = 5    // "mean" - mean of the "value" expression,    | always global extent |   yes   |
                          // summed over the query area. If "use_delta" specified, the 
                          //  query area is further restricted to those IDUs in which
                          //  a change, specified by the useDelta list, occured    
   };


class Constant
{
public:
   CString m_name;      // name of constant
   //double  m_value;     // value of constant
   CString m_value;
   CString m_description;

   Constant() { }
};

class Input
{
public:
   CString m_name;
   MTDOUBLE m_value;

   Input() : m_name(), m_value( 0.0 ) { }
};


class Stratifiable
{
public:
   Stratifiable( void );
   ~Stratifiable( void );

   // summarize information
   CString   m_stratifyByStr;
   CString   m_stratifyField; // field name used in strata selection - empty if not using strata selection
   int       m_colStratifyField;   // corresponding column
   MAP_FIELD_INFO *m_pStratifyFieldInfo;  

   FDataObj *m_pStratifyData;   // defined if m_stratifyField specified - each col is a summarize-by attr
                                 // this are exposed as output variables
  
   CArray< float, float > m_stratifiedValues;
   CArray< VData, VData& > m_stratifiedAttrs;   // ints or strings that are values to track
   CStringArray m_stratifiedLabels;
   
   CMap< int, int, int, int > m_attrMap;     // key = FIELD_ATTR index, value = m_stratifiedValues index

   bool IsStratified( void ) { return ( m_pStratifyFieldInfo != NULL && m_colStratifyField >= 0 ); }   

   int ParseFieldSpec( LPCTSTR summarizeBy, MapLayer *pLayer );
   int GetStratifiedAttrCount( void ) { return (int) m_stratifiedAttrs.GetSize(); }
};


class Pivotable
 {
 public:
   // pivot table information
   bool      m_makePivotTable;   // defined on
   VDataObj *m_pPivotData;       // contains pivot table info - ONLY defined for outputs with no parent OutputGroup
 
   Pivotable( void );
   ~Pivotable( void );
};


class ReportObject : public Stratifiable, public Pivotable 
{
public:
   ReportObject( void );

   CString m_name;

   // which map layer is this associated with?
   MapLayer *m_pMapLayer;
   CString   m_mapLayerName;

   bool m_use;
};

//-- class Output defines a Output. ----------------------------------------
//
//  There are currently four types of Outputs:
//     1) expression Outputs, defined by an evaluatable expression
//     2) pctArea Outputs, that return the pctArea of the target 
//           areas that satisfies the Output's query
//     3) sumArea Outputs, that return the values of the expression summed over
//        query/total area.  This are always global in extent
//------------------------------------------------------------------------------

enum EXTENT { EXTENT_IDU, EXTENT_GLOBAL };

class Output : public ReportObject
{
public:
   VAR_TYPE  m_varType;
   TYPE      m_dataType;
   int       m_col;

   CString   m_expression;
   CString   m_query;

   double    m_value;
   float     m_cumValue;
   float     m_queryArea;
   int       m_count;

   MapExprEngine *m_pMapExprEngine; // memory managed by Reporter
   MapExpr  *m_pMapExpr;   // memory managed elsewhere
   OutputGroup *m_pGroup;   // if in a group, NULL if not

public:
   Output();
   ~Output() { }
};


class OutputGroup : public ReportObject
{
public:
   OutputGroup( void );
   ~OutputGroup( void );
   
   FDataObj *m_pData;            // this is the outputVar that is exposed to Envision.  It collects output
                                 // on all outputs defined in this group

   // note:  OutputGroups never hold summarize data, that is always defined 
   // at the Output level

   int AddOutput( Output *pOutput ) { return (int) m_outputArray.Add( pOutput ); }
   int GetOutputCount( ) { return (int) m_outputArray.GetSize(); }
   Output *GetOutput( int i ) { return m_outputArray[ i ]; }

//protected:
   PtrArray< Output> m_outputArray;
};


class _EXPORT Reporter : public EnvModelProcess
{
public:
   Reporter( void );
   ~Reporter( void );

   // Exposed entry points - override these if so desired
   virtual bool Init   ( EnvContext *pEnvContext, LPCTSTR initStr );
   virtual bool InitRun( EnvContext *pEnvContext, bool useInitialSeed );
   virtual bool Run    ( EnvContext *pEnvContext );
   virtual bool EndRun ( EnvContext *pContext );
   //virtual vool Setup( EnvContext *pContext, HWND hWnd )      { return FALSE; }
   virtual bool GetConfig(EnvContext*, std::string &configStr) { configStr = m_configStr; return TRUE; }
   virtual bool SetConfig(EnvContext*, LPCTSTR configStr);

protected:
   bool LoadXml( EnvContext *pEnvContext, LPCTSTR filename );
   bool LoadXml(TiXmlElement *pXmlRoot, EnvContext *pEnvContext);

   int  LoadXmlOutputs( TiXmlElement *pXmlGroup, OutputGroup *pGroup );
   int  LoadXmlOutputs( TiXmlElement *pXmlParent, OutputGroup *pGroup, MapLayer *pLayer, QueryEngine *pQueryEngine, LPCTSTR filename );
   
protected:
   std::string m_configFile;
   std::string m_configStr;

   //MapExprEngine *m_pMapExprEngine;
   // MapExprEngine array - one for each layer referenced in the input file
   PtrArray< MapExprEngine > m_mapExprEngineArray;
   CArray< int, int > m_colAreaArray;

   MapExprEngine *FindMapExprEngine( MapLayer *pLayer );
   Constant    *FindConstant( LPCTSTR name );
   OutputGroup *FindOutputGroup( LPCTSTR name );

   void EvaluateOutputs( PtrArray < Output > &outputArray, MapLayer*, int currentIDU, float area );
   void NormalizeOutputs( PtrArray < Output > &outputArray, MapLayer *pLayer, float totalArea );
   void ResetOutputs( PtrArray< Output > &outputArray );
   void CollectOutput( EnvContext* );
   bool WritePivotData( VDataObj *pData, LPCTSTR filename );

   bool GenerateEvalCode( LPCTSTR className, stringstream &evalCode );
   bool LegalizeName( CString &name );
   bool ExportSourceCode( LPCTSTR className, LPCTSTR fileName );

   PtrArray< Constant    > m_constArray;
   PtrArray< Input       > m_inputArray;
   PtrArray< OutputGroup > m_outputGroupArray;
   PtrArray< Output      > m_outputArray;      // <output>s with no <output_group>

   //int m_colArea;

   CString m_startTimeStr;
   //MapLayer *m_pMapLayer;
};


extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new Reporter; }



inline
MapExprEngine *Reporter::FindMapExprEngine( MapLayer *pLayer )
   {
   for ( int i=0; i < (int) m_mapExprEngineArray.GetSize(); i++ )
      if ( m_mapExprEngineArray[i]->GetMapLayer() == pLayer )
         return m_mapExprEngineArray[i];

   return NULL;
   }
