#pragma once

#include <EnvExtension.h>

#include <MapLayer.h>
#include <Typedefs.h>
#include <VData.h>
#include <queryengine.h>
#include <tixml.h>
#include <MapExprEngine.h>
#include <PtrArray.h>

#include <map>

#define _EXPORT __declspec( dllexport )

class RandUniform;
class FieldCalculator;


enum FC_OP {            //  Description                                              |      Extent         | useDelta?|
   FCOP_UNDEFINED = 0,  //|----------------------------------------------------------|---------------------|-----------
   //FCOP_PCT_AREA = 1,   // "pctArea" - value defined as a percent of the total IDU   | always global extent|    no    |
   //                     // area that satisfies the specified query, expressed as a
   //                     // non-decimal percent.  No "value" attr specified.
   FCOP_SUM = 2,        // "sum" sums the "value" expression over an area            | always global extent |   yes   |
                        //  defined by the query.  If "use_delta" specified, the 
                        //  query area is further restricted to those IDUs in which
                        //  a change, specified by the useDelta list, occured
                       
   FCOP_AREAWTMEAN = 3,  // "areaWtMean" - area wt mean of the "value" expression,    | always global extent |   yes   |
                         // summed over the query area. If "use_delta" specified, the 
                         //  query area is further restricted to those IDUs in which
                         //  a change, specified by the useDelta list, occured 

   FCOP_LENWTMEAN = 4,   // for line features

   FCOP_MEAN = 5    // "mean" - mean of the "value" expression,                  | always global extent |   yes   |
                         // summed over the query area. If "use_delta" specified, the 
                         // query area is further restricted to those IDUs in which
                         // a change, specified by the useDelta list, occured
                         //VT_DELTA        = 6,   // "delta" - report change in field
                         //VT_FRACTION     = 7    // "fraction" - percetn change in field (0-100)
   };


// class Constant defines a constant value, stored in m_value
class Constant
   {
   public:
      CString m_name;      // name of constant
      double  m_value;     // value of constant
      bool    m_isInput;
      CString m_description;

      Constant() : m_value(0), m_isInput(false) { }

      bool Evaluate() { return true; }  // nothing required
   };

//-- class Variable defines a variable. ----------------------------------------
//
//------------------------------------------------------------------------------

class FieldDef
   {
   friend class FieldCalculator;

   public:
      CString   m_name;
      CString   m_field;

   protected:
      FieldCalculator* m_pFieldCalculator = nullptr;
      CString   m_queryStr;
      CString   m_mapExprStr;
      bool      m_useDelta = false;    //
      int       m_initialize=0;
      Query*    m_pQuery=nullptr;      // memory managed ??
      MapExpr*  m_pMapExpr=nullptr;    // memory managed ??
      int       m_col=-1;              // column associated with this variable (-1 if no col)
      float     m_value=0;
      float     m_maxLimit = (float) LONG_MIN;
      float     m_minLimit = (float) LONG_MAX;
      int       m_modelID = -99;       // should match .envx entry if needed
      TYPE      m_type = TYPE_FLOAT;

      // groupby info
      CString   m_groupBy;   // field to aggregate by, if aggregration desired; otherwise empty
      int       m_colGroupBy = -1;   // corresponding column
      FC_OP     m_operator = FC_OP::FCOP_UNDEFINED;

      std::map<int,int> m_groupByIndexMap; // key=FN_ID, value=index in groupby arrays
      std::vector<float> m_groupByValues;
      std::vector<float> m_groupByAreas;

      // miscellaneous trackers
      int m_count = 0;
      float m_appliedArea = 0;

   public:
      // methods                               
      FieldDef(FieldCalculator* pFC) { m_pFieldCalculator = pFC; }
      ~FieldDef() {}

      bool Init();
      //bool Evaluate();

      bool IsGroupBy() { return m_colGroupBy >= 0; }
   };




class _EXPORT FieldCalculator : public  EnvModelProcess
   {
   public:
      FieldCalculator();
      ~FieldCalculator();

      virtual bool Init(EnvContext* pEnvContext, LPCTSTR initStr);
      virtual bool InitRun(EnvContext* pEnvContext, bool useInitialSeed);
      virtual bool Run(EnvContext* pEnvContext){ return _Run(pEnvContext, false); }

   public:
      MapLayer*      m_pMapLayer;            // memory managed by EnvModel
      MapExprEngine* m_pMapExprEngine;       // memory managed by EnvModel
      QueryEngine*   m_pQueryEngine;         // memory managed by EnvModel

   protected:
      int m_colArea;

      CUIntArray m_iduArray;                 // used for shuffling IDUs
      bool m_shuffleIDUs;
      RandUniform* m_pRandUnif = nullptr;    // memory managed by "this" object

      PtrArray<Constant> m_constants;  // only added to the first instance
      PtrArray<FieldDef> m_fields;

      //std::vector<unique_ptr<FieldDef>> m_fields;

      FDataObj* m_pOutputData = nullptr;      // memory managed by "this" object

      bool _Run(EnvContext* pEnvContext, bool init);

      bool CollectData(int year);

      bool IsFieldInModel(FieldDef* pFD, int modelID) { return (pFD->m_modelID == modelID || pFD->m_modelID == -99); }

      bool LoadXml(EnvContext*, LPCTSTR);

   };


extern "C" _EXPORT EnvExtension * Factory(EnvContext*); // { return (EnvExtension*) new FieldCalculator; }

