#pragma once

using namespace std;

#include <vector>
#include <string>
#include <Maplayer.h>
#include <QueryEngine.h>
#include <MapExprEngine.h>
#include <tixml.h>
#include "VSMB.h"
#include <afxtempl.h> 


class Farm;

enum CSKEYWORD { CS_TMIN, CS_TMEAN, CS_TMAX, CS_PRECIP, CS_AVC };



struct CSField
   {
   CString name;
   TYPE type;
   VData defaultValue;

   int col;

   CSField(): type(TYPE_NULL), col(-1) {}
   };


struct CSCropEvent
   {
   CString name;
   int id;
   float yrf;
   CString when;
   Query* pWhen;

   CSCropEvent() : id(-1), yrf(0), pWhen(NULL) {}

   Query* CompileWhen(QueryEngine* pQE);
   };


struct CSTransition
   {
   CString toStage;
   CString when;
   Query* pWhen;
   //CString outcome;
   //CString outcomeTarget;
   //CString outcomeExpr;
   //MapExpr* pOutcomeExpr;
   //VData outcomeValue;

   //CSTransition(TCHAR* _toStage, TCHAR* _when, TCHAR* _outcome);
   //~CSTransition(void) { if (pOutcomeExpr != NULL) delete pOutcomeExpr; }

   Query *CompileWhen(QueryEngine* pQE);
   //MapExpr *CompileOutcome(MapExprEngine* pME, LPCTSTR name);

   };

struct CSEvalExpr
   {
   CString name;
   CString outcome;
   CString when;

   CString outcomeTarget;
   CString outcomeExpr;
   VData outcomeValue;

   Query* pWhen;  // memory managed by engine
   MapExpr* pOutcomeExpr;

   int col;
   
   CSEvalExpr(void) : pWhen(NULL), pOutcomeExpr(NULL), col(-1) {}
   ~CSEvalExpr(void) { /*if (pOutcomeExpr != NULL) delete pOutcomeExpr;*/ }

   Query* CompileWhen(QueryEngine* pQE);
   MapExpr* CompileOutcome(MapExprEngine* pME, LPCTSTR name, MapLayer*);
   };


class CSCropStage
   {
   public:
      int m_id;   // 1-based offset in FarmModel::m_crops array
      CString m_name;

      PtrArray<CSCropEvent> m_events;
      PtrArray<CSTransition> m_transitions;
      PtrArray<CSEvalExpr> m_evalExprs;

      CSCropStage(LPCTSTR name) : m_id(-1), m_name(name) {}
   };


class CSCrop
   {
   public:
      CString m_name;
      int m_id;
      CString m_code;
      bool m_isRotation;
      int m_harvestStartYr;
      int m_harvestFreq;
      float m_yrfThreshold;

      PtrArray<CSCropStage> m_cropStages;
      PtrArray<CSField> m_fields;

      CSCrop() : m_id(-1), m_isRotation(true), m_harvestFreq(0), m_harvestStartYr(0), m_yrfThreshold(-1.0f) {}
   };


class CSModel
   {
   public:
      // QExternals
      static int m_doy;
      static int m_year;
      static int m_idu;

      // available climate variables
      static float m_pDays;
      static float m_precip;
      static float m_hMeanPrecip;
      static float m_tMean;
      static float m_tMin;
      static float m_tMax;
      static float m_gdd0;
      //static float m_gdd5;
      static float m_gdd0Apr15;
      static float m_gdd5Apr1;
      static float m_gdd5May1;
      static float m_chuMay1;
      static float m_pET;

      // available soil variables
      static float m_pctAWC;
      static float m_pctSat;
      static float m_swc;
      static float m_swe;

      // other stuff
      static MapLayer* m_pMapLayer;

      static ClimateStation *m_pClimateStation; // refererence to FarmModel climate station
      static VSMBModel* m_pVSMB;  // memory managed elsewhere

      PtrArray<CSCrop> m_crops;
      CMap<int, int, CSCrop*, CSCrop*> m_cropLookup;

   public:
      int Init(FarmModel* pFarmModel,MapLayer *pIDULayer, QueryEngine* pQE, MapExprEngine* pME);
      int InitRun(EnvContext *);   // called at start of run
      int Run(EnvContext*, bool) { return 1; }   // called at start of year

      //int SolveWhen(TCHAR* expr);

      static float Avg(int kw, int period);
      static float AbovePeriod(int kw, int period);

      float UpdateCropStatus(EnvContext* pContext, FarmModel* pFarmModel, Farm* pFarm, 
         ClimateStation* pStation, MapLayer* pLayer, int idu, float areaHa, int doy, int year,
         int lulc, int cropStage, float priorCumYRF);

      bool LoadXml(TiXmlElement* pRoot, LPCTSTR path, MapLayer *pLayer);

      CSModel() {}  // 
   };

