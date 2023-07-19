#pragma once

#include <EnvExtension.h>

#include <vdataobj.h>
#include <fdataobj.h>
#include <AttrIndex.h>
#include <PtrArray.h>

#define _EXPORT __declspec( dllexport )


class TransCol {
public:
   CString m_field;
   int m_col;
   CArray<VData, VData&> m_attrValues;
   FDataObj* m_pTransData;

   CString m_queryStr;
   Query* m_pQuery;

   CMap<VData, VData&, int, int> transMap; //     key = attrValue, value = index

   TransCol(LPCTSTR field, int col) : m_field(field), m_col(col), m_pTransData(NULL), m_pQuery(NULL) {}

   ~TransCol() { if ( m_pTransData != NULL ) delete m_pTransData; }
   };



class Transition {
public:
   std::string m_name;

   Query* m_pFromQuery = nullptr;
   Query* m_pToQuery = nullptr;
   std::vector<int> m_fromIDUs; // vector of IDU indices for IDU tha initial satify "from" query

   float m_area = 0;
   };


class TransGroup {

public:
   PtrArray<Transition> m_transArray;

   FDataObj* m_pTransData = nullptr;
   //CMap<VData, VData&, int, int> transMap; //     key = attrValue, value = index

   TransGroup(LPCTSTR field, int col) {} //: m_field(field), m_col(col), m_pTransData(NULL), m_pQuery(NULL) {}

   ~TransGroup() { if (m_pTransData != nullptr) delete m_pTransData; }
   };


class _EXPORT Transitions : public  EnvModelProcess
   {
   public:
      Transitions();
      Transitions(Transitions& t) { *this = t; }

      Transitions& operator = (Transitions&);

      ~Transitions();

      virtual bool Init(EnvContext* pEnvContext, LPCTSTR initStr);
      virtual bool InitRun(EnvContext* pEnvContext, bool useInitialSeed);
      virtual bool Run(EnvContext* pContext);
      virtual bool EndRun(EnvContext* pContext);

      bool LoadXml(EnvContext* pEnvContext, LPCTSTR filename);

   protected:
      CString m_name;
      int m_colArea;
      PtrArray<TransCol> m_transColArray;
      QueryEngine* m_pQueryEngine;
   };


extern "C" _EXPORT EnvExtension * Factory(EnvContext*);

