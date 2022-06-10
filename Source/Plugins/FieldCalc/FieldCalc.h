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

   protected:
      CString   m_queryStr;
      CString   m_mapExprStr;
      bool      m_useDelta;      //
      bool      m_initColData;
      //Query* m_pQuery;
      MapExpr* m_pMapExpr;
      int       m_col;           // column associated with this variable (-1 if no col)
      float     m_value;


      int m_count;

   public:
      // methods                               
      FieldDef() : m_col(-1), /*m_pQuery(NULL),*/ m_pMapExpr(NULL), m_useDelta(true),m_initColData(true), m_count(0){}
      ~FieldDef() {}

      bool Init();
      //bool Evaluate();
   };




class _EXPORT FieldCalculator : public  EnvModelProcess
   {
   public:
      FieldCalculator();

      virtual bool Init(EnvContext* pEnvContext, LPCTSTR initStr);

      virtual bool InitRun(EnvContext* pEnvContext, bool useInitialSeed);
      
      virtual bool Run(EnvContext *pEnvContext) { return _Run(pEnvContext, false); }

      //virtual bool EndRun(EnvContext *pContext) { return true; }
      //virtual bool Setup(EnvContext *pContext, HWND hWnd) { return false; }
      //virtual bool GetConfig(std::string &configStr) { return false; }
      //virtual bool SetConfig(EnvContext*, LPCTSTR configStr) { return false; }
      //virtual int  InputVar(int id, MODEL_VAR** modelVar);
      //virtual int  OutputVar(int id, MODEL_VAR** modelVar);

   public:
      MapLayer*      m_pMapLayer;            // memory managed by EnvModel
      MapExprEngine* m_pMapExprEngine;       // memory managed by EnvModel
      QueryEngine*   m_pQueryEngine;         // memory managed by EnvModel

   protected:
      int m_colArea;

      CUIntArray m_iduArray;    // used for shuffling IDUs
      bool m_shuffleIDUs;
      RandUniform* m_pRandUnif;

      PtrArray<FieldDef> m_fields;
      PtrArray<Constant> m_constants;

      FDataObj* m_pOutputData;

      bool _Run(EnvContext* pEnvContext, bool init);

      bool CollectData(int year);


      bool LoadXml(EnvContext*, LPCTSTR);

   };


extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new FieldCalculator; }

