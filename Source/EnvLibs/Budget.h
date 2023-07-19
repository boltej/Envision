#pragma once


#include <Vdataobj.h>
#include <PtrArray.h>
#include <DOIndex.h>
#include <Maplayer.h>
#include <MapExprEngine.h>
#include <tixml.h>


//------------------------------------------------------
// About Budgets:
//
// Budgets are defined globally, and contributors to those
// budgets are CostItems
//
// The basic idea is to track contributions from CostItems and
// compare these to the budget constraint;  Once the budget is 
// exceeded, no additional cost should be incurred by the client.
// It is necessary to track year by year contributions. Contributions come in
// two forms:  1) Initial costs when the policy is first applied, and 
// 2) maintenance costs, appiled over a period after the policy is applied.
// Maintenance costs are tracked in a list (gpModel->m_activeConstraintList)
// Maintenance costs are applied at the start of the year to the global constraints.
// After that, each new policy application's initial cost is added into the
// global constraint.
//---------------------------------------------------------

class Budget;
class BudgetItem;


// indexing for policy constraint lookups

struct B_COL_INFO  // stores info about field/values pairs specified in a policy constraint lookup table spec
   {
   int  doCol;       // column in the attached VDataObj for this entry (used when Build()ing index
   int  mapCol;      // column in the IDU coverage for creating IDU-based keys
   bool isValueFieldRef;   // specifies whether the value is a numveric value or a self field reference
   VData keyValue;   //  key value (from field/value pair). if field ref, holds the col for the self-reference field 
                     //  or the value if not a field ref    
   CString fieldName;

   B_COL_INFO(void) : doCol(-1), mapCol(-1), isValueFieldRef(false), keyValue(), fieldName() { }
   };

// Lookup table index.  This class allows for up to four columns and/or values to be specified 
// that are used to look up a cost from the attached dataobj table
class BLTIndex : public DOIndex
   {
   protected:
      PtrArray< B_COL_INFO > m_keyColArray;
      MapLayer* m_pMapLayer;

   public:
      BLTIndex(MapLayer* pLayer, DataObj* pDataObj)
         : DOIndex(pDataObj, -1)  // no size
         , m_pMapLayer(pLayer)
         { }

      BLTIndex(BLTIndex& index) { *this = index; }

      BLTIndex& operator = (BLTIndex&);

      int AddCol(B_COL_INFO* pInfo) { return (int)m_keyColArray.Add(pInfo); }

      UINT GetColValue(int colIndex, int idu)
         {
         if (colIndex >= (int)m_keyColArray.GetSize()) return 0;

         B_COL_INFO* pInfo = m_keyColArray[colIndex];
         UINT key = 0;

         if (pInfo->isValueFieldRef)
            key = m_pMapLayer->m_pData->GetAsUInt(m_keyColArray[colIndex]->mapCol, idu);
         else
            pInfo->keyValue.GetAsUInt(key);

         return key;
         }

      UINT GetKeyFromMap(int idu);

      virtual UINT GetKey(int row);

      bool Lookup(int idu, int lookupCol, float& value)
         {
         UINT key = GetKeyFromMap(idu);

         int  doRow = 0;
         if (m_map.Lookup(key, doRow) == FALSE)
            return FALSE;

         value = m_pDataObj->GetAsFloat(lookupCol, doRow);
         return true;
         }
   };

enum BUDGET_TYPE { BT_UNKNOWN = 0, BT_RESOURCE_LIMIT, BT_MAX_AREA, BT_MAX_COUNT };
enum CI_BASIS { CIB_UNKNOWN = 0, CIB_UNIT_AREA, CIB_ABSOLUTE };




class LIBSAPI CostItem
   {
   public:
      CostItem(MapLayer *);  // default constructor
      
      CostItem(CostItem& pc) { *this = pc; }

      ~CostItem(void) { if (m_pLookupTableIndex != NULL) delete m_pLookupTableIndex; }

      CostItem& operator = (CostItem& pc)
         {
         //m_pPolicy = pc.m_pPolicy;
         m_name = pc.m_name;
         m_budgetItemName = pc.m_budgetItemName;
         m_basis = pc.m_basis;
         m_initialCostExpr = pc.m_initialCostExpr;
         m_maintenanceCostExpr = pc.m_maintenanceCostExpr;
         m_durationExpr = pc.m_durationExpr;
         m_initialCost = pc.m_initialCost;
         m_maintenanceCost = pc.m_maintenanceCost;
         m_duration = pc.m_duration;
         m_pBudgetItem = pc.m_pBudgetItem;
         m_pLookupTable = pc.m_pLookupTable;
         m_pLookupTableIndex = pc.m_pLookupTableIndex ? new BLTIndex(*(pc.m_pLookupTableIndex)) : NULL;
         m_colMaintenanceCost = pc.m_colMaintenanceCost;
         m_colInitialCost = pc.m_colInitialCost;

         m_pMapLayer = pc.m_pMapLayer;
         m_pMainCostMapExpr = pc.m_pMainCostMapExpr;    // NULL if not defined
         m_pInitCostMapExpr = pc.m_pInitCostMapExpr;    // NULL if not defined
         m_pDurationMapExpr = pc.m_pDurationMapExpr;    // NULL if not defined

         return *this;
         }

      bool Init(Budget*, LPCTSTR biName, LPCTSTR ciName, CI_BASIS basis, LPCTSTR initCostExpr, LPCTSTR maintenanceCostExpr, LPCTSTR durationExpr, LPCTSTR lookup=NULL);

      bool ParseCost(VDataObj* pLookupTable, LPCTSTR costExpr, float& cost, int& col, MapExpr*& pMapExpr);
      VDataObj* GetLookupTable(void) { return m_pLookupTable; }

      bool GetInitialCost(int idu, float area, float& cost);
      bool GetMaintenanceCost(int idu, float area, float& cost);
      bool GetDuration(int idu, int& duration); // { return m_duration; }

      CString m_name;
      CString m_budgetItemName;    // name of the associated global constraint
      CI_BASIS  m_basis = CIB_UNKNOWN;       // see enum

      // totals for this CostItem, reset each year
      float m_totalCosts =0;
      float m_totalInitialCosts = 0;
      float m_totalMaintenanceCosts = 0;
      float m_totalCumulativeCosts = 0;

   protected:
      float m_initialCost = 0;       // init cost (NOTE: assumed constant for now)
      float m_maintenanceCost = 0;   // maintenance cost
      int  m_duration = 0;

      bool  GetCost(int idu, MapExpr* pMapExpr, float costToUse, int colCost, float& cost);
      float AdjustCostToBasis(float cost, float area);

   public:
      CString m_initialCostExpr;       // expression used for computing value
      CString m_maintenanceCostExpr;   // expression used for computing value
      CString m_durationExpr;

      MapLayer* m_pMapLayer = nullptr;           // associated map layer
      MapExpr* m_pMainCostMapExpr = nullptr;     // NULL if not defined
      MapExpr* m_pInitCostMapExpr = nullptr;     // NULL if not defined
      MapExpr* m_pDurationMapExpr = nullptr;     // NULL if not defined

      BudgetItem* m_pBudgetItem = nullptr;    // associated Budget Item

   public:
      // cost identification through table lookups
      CString   m_lookupStr;
      VDataObj* m_pLookupTable = nullptr;    // associated data object for cost information (NULL if not defined)
      BLTIndex* m_pLookupTableIndex = nullptr;

   protected:
      int   m_colMaintenanceCost = -1;
      int   m_colInitialCost = -1;

   public:
      LPCTSTR GetBasisString(CI_BASIS type);

   protected:
      static PtrArray< VDataObj > m_lookupTableArray;
   };



class LIBSAPI BudgetItem
   {
   //friend class DataManager;
   friend class Budget;

   public:
      BudgetItem(LPCTSTR name, BUDGET_TYPE type, LPCTSTR expr);
      BudgetItem(BudgetItem& gc) : m_name(gc.m_name), m_type(gc.m_type), m_limit(gc.m_limit), m_costs(gc.m_costs), m_expression(gc.m_expression), m_pMapExpr(gc.m_pMapExpr) { }

      BudgetItem& operator = (BudgetItem& gc)
         {
         m_name = gc.m_name;
         m_type = gc.m_type;
         m_limit = gc.m_limit; 
         m_costs = gc.m_costs;
         m_expression = gc.m_expression;
         m_pMapExpr = gc.m_pMapExpr;
         m_costItems.DeepCopy(gc.m_costItems);
         return *this;
         }

      void  AddCostItem(CostItem* pCostItem) { m_costItems.Add(pCostItem); }
      CostItem* GetCostItem(int i) { return m_costItems[i]; }
      int GetCostItemCount() { return (int)m_costItems.GetSize(); }

      void  Reset(bool);  // called at the start of each time step - updates value, reset currentValue=0, increments cumulative
      void  ResetCumulative(void) { m_cumulativeCosts = 0; }
      bool  DoesCostExceedBudget(float increment);
      bool  ApplyCostItem(CostItem* pConstraint, int idu, float area, bool useInitCost);

      //SpatialAllocator* m_pSA;

      CString m_name;
      BUDGET_TYPE   m_type;     // type of constraint
      CString m_expression;      // expression string for computing value
      bool    m_addReturnsToBudget; //????

      LPCTSTR GetTypeString(BUDGET_TYPE type);
      LPCTSTR GetExpression() { return (LPCTSTR)m_expression; }

      float   GetLimit() { return m_limit; }
      float   GetTotalCosts() { return m_costs; }  // init + maintanence
      float   GetCumulativeCosts() { return m_cumulativeCosts; }
      float   GetInitialCosts() { return m_initialCosts; }
      float   GetMaintenanceCosts() { return m_maintenanceCosts; }
      float   GetAvailableBudget(void) { return (m_limit - m_costs); }
 
   protected:
      float   m_limit;           // max value before constraint hits. (NOTE: assumed constant for now)
      float   m_costs;    // current value of budget item - sum of costs
      float   m_cumulativeCosts; // sum of costs since last reset
      float   m_maintenanceCosts;
      float   m_initialCosts;

      MapExpr* m_pMapExpr;       // associated MapExpr to calculate limit (NULL if not an evaluated expression)

      PtrArray<CostItem> m_costItems;     // associated cost items
   };


class LIBSAPI Budget
   {
   friend class CostItem;
   friend class BudgetItem;

   protected:
      CString m_name;
      PtrArray<BudgetItem> m_budgetItems;     // associate cost items

      static MapExprEngine* m_pMapExprEngine;    // memory managed elswhere

   public:
      Budget(LPCTSTR name, MapExprEngine* pMapExprEngine) : m_name(name)
         { m_pMapExprEngine = pMapExprEngine; }

      // manage budget items
      int     AddBudgetItem(BudgetItem* pBudgetItem) { return (int)m_budgetItems.Add(pBudgetItem); }
      int     GetBudgetItemCount(void) { return (int)m_budgetItems.GetSize(); }

      BudgetItem* GetBudgetItem(int i) { return m_budgetItems[i]; }
      BudgetItem* FindBudgetItem(LPCTSTR name);

      PtrArray<BudgetItem>& GetBudgetItemArray() { return m_budgetItems; }

      void    RemoveAllBudgetItems(void) { m_budgetItems.RemoveAll(); }
      //bool    ReplaceBudgetItems(PtrArray< BudgetItem >& newBudgetItems);

      void Reset( bool includeCumulative) {
         for (int i = 0; i < m_budgetItems.GetSize(); i++)
            GetBudgetItem(i)->Reset(includeCumulative);
         }
   };




inline
LPCTSTR BudgetItem::GetTypeString(BUDGET_TYPE type)
   {
   switch (type)
      {
      case BT_RESOURCE_LIMIT:    return _T("resource_limit");
      case BT_MAX_AREA:          return _T("max_area");
      case BT_MAX_COUNT:         return _T("max_count");

      case BT_UNKNOWN:
         return _T("unknown");

      default:
         ASSERT(0);
         return NULL;
      }
   }


inline
LPCTSTR CostItem::GetBasisString(CI_BASIS basis)
   {
   switch (basis)
      {
      case CIB_UNIT_AREA:    return _T("unit_area");
      case CIB_ABSOLUTE:     return _T("absolute");

      case CIB_UNKNOWN:
         return _T("unknown");

      default:
         ASSERT(0);
         return NULL;
      }
   }