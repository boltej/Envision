#pragma once

#include <EnvExtension.h>

#include <MapLayer.h>
#include <Typedefs.h>
#include <VData.h>
#include <queryengine.h>
#include <tixml.h>
#include <MapExprEngine.h>
#include <PtrArray.h>


#define _EXPORT __declspec( dllexport )

class RandUniform;



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
      CString   m_queryStr;
      CString   m_mapExprStr;
      bool      m_useDelta = false;      //
      bool      m_initColData=false;
      Query*    m_pQuery=NULL;
      MapExpr*  m_pMapExpr=NULL;
      int       m_col=-1;           // column associated with this variable (-1 if no col)
      float     m_value=0;
      int       m_modelID = -99;    // should match .envx entry if needed

      // groupby info
      CString   m_groupBy;   // field to aggregate by, if aggregration desired; otherwise empty
      int       m_colGroupBy = -1;   // corresponding column
      FC_OP     m_operator = FC_OP::FCOP_UNDEFINED;

      CMap<int, int, int, int> m_groupByIndexMap; // key=FN_ID, value=values
      CArray<float, float> m_groupByValues;
      CArray<float, float> m_groupByAreas;

      // miscellaneous trackers
      int m_count = 0;
      float m_totalArea = 0;

   public:
      // methods                               
      FieldDef() {}
      ~FieldDef() {}

      bool Init();
      //bool Evaluate();

      bool IsGroupBy() { return m_colGroupBy >= 0; }
   };




class _EXPORT FieldCalculator : public  EnvModelProcess
   {
   public:
      FieldCalculator();

      virtual bool Init(EnvContext* pEnvContext, LPCTSTR initStr);

      virtual bool InitRun(EnvContext* pEnvContext, bool useInitialSeed);
      
      virtual bool Run(EnvContext* pEnvContext){ return _Run(pEnvContext, false); }

      //virtual bool EndRun(EnvContext *pContext) { return true; }
      //virtual bool Setup(EnvContext *pContext, HWND hWnd) { return false; }
      //virtual bool GetConfig(std::string &configStr) { return false; }
      //virtual bool SetConfig(EnvContext*, LPCTSTR configStr) { return false; }
      //virtual int  InputVar(int id, MODEL_VAR** modelVar);
      //virtual int  OutputVar(int id, MODEL_VAR** modelVar);

   public:
      static MapLayer*      m_pMapLayer;            // memory managed by EnvModel
      static MapExprEngine* m_pMapExprEngine;       // memory managed by EnvModel
      static QueryEngine*   m_pQueryEngine;         // memory managed by EnvModel

   protected:
      static bool m_initialized;
      static int m_colArea;

      static CUIntArray m_iduArray;    // used for shuffling IDUs
      static bool m_shuffleIDUs;
      static RandUniform* m_pRandUnif;

      static PtrArray<FieldDef> m_fields;
      static PtrArray<Constant> m_constants;

      FDataObj* m_pOutputData;

      bool _Run(EnvContext* pEnvContext, bool init);

      bool CollectData(int year);

      int GetActiveFieldCount(int modelID) {
         int fieldCount = (int)m_fields.GetSize(); 
         int fc = 0;
         for (int i = 0; i < fieldCount; i++)
            if (m_fields[i]->m_modelID == modelID || m_fields[i]->m_modelID == -99)
               fc++;
         return fc;
         }
      bool IsFieldInModel(FieldDef* pFD, int modelID) { return (pFD->m_modelID == modelID || pFD->m_modelID == -99); }

      bool LoadXml(EnvContext*, LPCTSTR);

   };


extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new FieldCalculator; }

