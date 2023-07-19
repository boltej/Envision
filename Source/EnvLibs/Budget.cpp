#include "EnvLibs.h"
#pragma hdrstop

#include "Budget.h"

#include <Path.h>



// statics
PtrArray< VDataObj > CostItem::m_lookupTableArray(true);

MapExprEngine* Budget::m_pMapExprEngine = NULL;    // memory managed elswhere

//-----------------------------------------------------------------
//    G L O B A L    C O N S T R A I N T S
//-----------------------------------------------------------------

BLTIndex& BLTIndex::operator = (BLTIndex& index)
   {
   m_keyColArray = index.m_keyColArray;
   this->m_isBuilt = index.m_isBuilt;
   //this->m_map       = index.m_map;
   this->m_pDataObj = index.m_pDataObj;
   this->m_pMapLayer = index.m_pMapLayer;
   this->m_size = index.m_size;

   POSITION pos = index.m_map.GetStartPosition();
   m_map.RemoveAll();
   m_map.InitHashTable(index.m_map.GetHashTableSize());
   UINT  key = 0;
   int   value = 0;

   while (pos != NULL)
      {
      index.m_map.GetNextAssoc(pos, key, value);
      m_map.SetAt(key, value);
      }

   return *this;
   }

UINT BLTIndex::GetKeyFromMap(int idu)
   {
   int count = (int)m_keyColArray.GetSize();

   switch (count)
      {
      case 1:
      {
      UINT key = GetColValue(0, idu);
      return key;
      }

      case 2:
      {
      WORD low = GetColValue(0, idu);
      WORD high = GetColValue(1, idu);
      DWORD key = MAKELONG(low, high);
      return key;
      }

      case 3:
      {
      UINT val0 = GetColValue(0, idu);
      UINT val1 = GetColValue(1, idu);
      UINT val2 = GetColValue(2, idu);
      val0 = val0 << 22;
      val1 = val1 << 11;
      UINT key = val0 | val1 | val2;
      return key;
      }

      case 4:
      {
      UINT val0 = GetColValue(0, idu);
      UINT val1 = GetColValue(1, idu);
      UINT val2 = GetColValue(2, idu);
      UINT val3 = GetColValue(3, idu);
      val0 = val0 << 24;
      val1 = val1 << 16;
      val2 = val2 << 8;
      UINT key = val0 | val1 | val2 | val3;
      return key;
      }
      }

   return -1;
   }

UINT BLTIndex::GetKey(int row)
   {
   int count = (int)m_keyColArray.GetSize();

   if (count == 1)
      return (UINT)m_pDataObj->GetAsUInt(m_keyColArray[0]->doCol, row);

   if (count == 2)
      {
      WORD low = (WORD)m_pDataObj->GetAsUInt(m_keyColArray[0]->doCol, row);
      WORD high = (WORD)m_pDataObj->GetAsUInt(m_keyColArray[1]->doCol, row);
      DWORD key = MAKELONG(low, high);
      return key;
      }

   if (count == 3)
      {
      UINT val0 = m_pDataObj->GetAsUInt(m_keyColArray[0]->doCol, row);
      UINT val1 = m_pDataObj->GetAsUInt(m_keyColArray[1]->doCol, row);
      UINT val2 = m_pDataObj->GetAsUInt(m_keyColArray[2]->doCol, row);

      val0 = val0 << 22;
      val1 = val1 << 11;
      UINT key = val0 | val1 | val2;
      return key;
      }

   if (count == 4)
      {
      UINT val0 = m_pDataObj->GetAsUInt(m_keyColArray[0]->doCol, row);
      UINT val1 = m_pDataObj->GetAsUInt(m_keyColArray[1]->doCol, row);
      UINT val2 = m_pDataObj->GetAsUInt(m_keyColArray[2]->doCol, row);
      UINT val3 = m_pDataObj->GetAsUInt(m_keyColArray[3]->doCol, row);

      val0 = val0 << 24;
      val1 = val1 << 16;
      val2 = val2 << 8;
      UINT key = val0 | val1 | val2 | val3;
      return key;
      }

   return -1;
   }


BudgetItem::BudgetItem(LPCTSTR name, BUDGET_TYPE type, LPCTSTR expr)
   : m_name(name)
   , m_type(type)
   , m_expression(expr)
   , m_pMapExpr(NULL)
   , m_addReturnsToBudget(false) //??
   , m_limit(0)
   , m_costs(0)
   , m_cumulativeCosts(0)
   , m_maintenanceCosts(0)
   , m_initialCosts(0)
   {
   // what kind of expression?  can be a constant of an evaluable expression.
   // ASSUME ALWAYS A MAPEXPR FOR NOW, but must be global
   m_pMapExpr = Budget::m_pMapExprEngine->AddExpr(name, expr);
   ASSERT(m_pMapExpr->IsGlobal());
   if (Budget::m_pMapExprEngine->EvaluateExpr(m_pMapExpr, false) == false)
   if( m_pMapExpr->Evaluate() ==false)
      {
      CString msg;
      msg.Format("Global Constraint '%s': Unable to evaluate expression '%s'", name, expr);
      Report::LogWarning(msg);
      m_limit = 0;
      }

   else {
      m_limit = (float) m_pMapExpr->GetValue();
      }
   }


void BudgetItem::Reset(bool includeCumulative)
   {
   if (includeCumulative)
      m_cumulativeCosts = 0;
   
   m_costs = 0;
   m_maintenanceCosts = 0;
   m_initialCosts = 0;

   // get the value by evaluating the asociated expression
   if (m_pMapExpr != NULL && m_pMapExpr->IsGlobal())
      {
      bool ok = Budget::m_pMapExprEngine->EvaluateExpr(m_pMapExpr, false);

      if (ok)
         m_limit = (float) m_pMapExpr->GetValue();
      else
         m_limit = 0;
      }
   else
      m_limit = 0;

   for (int i = 0; i < (int)m_costItems.GetSize(); i++)
      {
      if (includeCumulative)
         m_costItems[i]->m_totalCumulativeCosts = 0;

      m_costItems[i]->m_totalCosts = 0;
      m_costItems[i]->m_totalInitialCosts = 0;
      m_costItems[i]->m_totalMaintenanceCosts = 0;
      }
   }

// a cost applies if it has been not exceeded
// gets called by EnvModel::DoesPolicyApply()
bool BudgetItem::DoesCostExceedBudget(float costIncrement)
   {
   if (m_type == BT_MAX_COUNT)
      costIncrement = 1;

   switch (m_type)
      {
      case BT_UNKNOWN:
         return false;      // never applies (never constrains)  

      case BT_RESOURCE_LIMIT:
      case BT_MAX_AREA:
      case BT_MAX_COUNT:
         return (m_costs + costIncrement) >= m_limit ? true : false;
      }

   return false;
   }


// ApplyCostItem() applies a given CostItem to this BudgetItem,
// subtracting the specified cost from the global pool 
bool BudgetItem::ApplyCostItem(CostItem* pCostItem, int idu, float area, bool useInitCost)
   {
   bool ok = true;

   switch (m_type)
      {
      case BT_UNKNOWN:
         return false;

      case BT_RESOURCE_LIMIT:
         {
         // note: positive costs indicate the policy consumes $s
         // negative costs (returns) do not get counted
         float cost = 0;
         if (useInitCost)
            ok = pCostItem->GetInitialCost(idu, area, cost);   // these are basis-adjusted (total) costs 
         else
            ok = pCostItem->GetMaintenanceCost(idu, area, cost);

         if (!ok)
            return false;

         // generating a return?  Then ignore
         if (cost <= 0 && this->m_addReturnsToBudget == false)
            return true;

         if (useInitCost)
            {
            m_initialCosts += cost;
            pCostItem->m_totalInitialCosts += cost;
            }
         else
            {
            m_maintenanceCosts += cost;
            pCostItem->m_totalMaintenanceCosts += cost;
            }

         m_costs += cost;
         m_cumulativeCosts += cost;
         pCostItem->m_totalCosts += cost;
         pCostItem->m_totalCumulativeCosts += cost;
         }
      return true;

      case BT_MAX_AREA:
         m_costs += area;
         m_cumulativeCosts += area;
         return true;

      case BT_MAX_COUNT:
         m_costs += 1;
         m_cumulativeCosts++;
         return true;

      default:
         ASSERT(0);
         return false;
      }

   return false;
   }


CostItem::CostItem(MapLayer* pMapLayer)
   : m_pMapLayer(pMapLayer)
   //: m_initialCost(0)
   //, m_maintenanceCost(0)
   //, m_duration(0)
   //, m_pBudgetItem(NULL)
   //, m_pLookupTable(NULL)
   //, m_pLookupTableIndex(NULL)
   //, m_colMaintenanceCost(-1)
   //, m_colInitialCost(-1)
   {
   ASSERT(m_pMapLayer != nullptr);
   }


bool CostItem::Init(Budget* pBudget, LPCTSTR biName, LPCTSTR ciName, CI_BASIS basis, LPCTSTR initCostExpr, LPCTSTR maintenanceCostExpr, LPCTSTR durationExpr, LPCTSTR lookup /*= NULL*/)
   {
   m_name = ciName;
   m_basis = basis;
   m_initialCostExpr = initCostExpr;
   m_maintenanceCostExpr = maintenanceCostExpr;
   m_durationExpr = durationExpr;
   //m_lookupStr = lookup;

   m_pBudgetItem = pBudget->FindBudgetItem(biName);
   if ( m_pBudgetItem == NULL )
      {
      CString msg;
      msg.Format("Cost Item error: A Budget Item '%s' referenced by the cost item '%s' was not found.  This cost item will be ignored",
         biName, ciName);
      Report::ErrorMsg(msg);
      return false;
      }

   m_pBudgetItem->AddCostItem(this);

   // associated lookup table?
   if (this->m_lookupStr.GetLength() > 0 )
      {
      // note:up to two lookup expresions are supported, of the form:
      // lookup='Eugene_Manage_Costs.csv|manage=14;vegclass=@vegclass'

      // first, get filename 
      TCHAR buffer[512];
      lstrcpy(buffer, this->m_lookupStr);
      LPTSTR start = buffer;
      LPTSTR filename = start;

      // parse expression: tablename:col=value
      LPTSTR end = _tcschr(filename, '|');
      if (end == NULL)
         {
         CString msg("Cost Item:  Error parsing lookup entry '");
         msg += this->m_lookupStr;
         msg += "'.  This should be of the form 'path|lookupCol=lookupValue'.  This constraint will be ignored...";
         Report::ErrorMsg(msg);
         return false;
         }
      *end = NULL;

      // seen this table before?  If not, we need to load it and add it to the table list
      bool found = false;
      for (int i = 0; i < (int)m_lookupTableArray.GetSize(); i++)
         {
         if (lstrcmpi(m_lookupTableArray[i]->GetName(), filename) == 0)
            {
            m_pLookupTable = m_lookupTableArray[i];
            found = true;
            break;
            }
         }

      if (!found)
         {
         // was a full path specified?  if not, add IDU path
         LPTSTR bslash = _tcschr(filename, '\\');
         LPTSTR fslash = _tcschr(filename, '/');

         nsPath::CPath path;
         if (bslash || fslash)
            path = filename;  // no modification necessary if full path specified
         else
            {
            //path = this->m_pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer->m_path;
            //path.MakeFullPath();
            //path.RemoveFileSpec();
            //path.Append(filename);
            }

         VDataObj* pData = new VDataObj(U_UNDEFINED);
         int count = pData->ReadAscii(path);

         if (count <= 0)
            {
            CString msg("Cost Item: Error loading lookup table '");
            msg += filename;
            msg += "'.  This constraint will be ignored...";
            Report::ErrorMsg(msg);
            delete pData;
            return false;
            }
         else
            {
            m_pLookupTable = pData;
            m_pLookupTable->SetName(filename);
            m_lookupTableArray.Add(pData);
            }
         }

      // at this point, we should have a valid m_pLookupTable VDataObj loaded.
      // add an index
      //////ASSERT(m_pLookupTableIndex == NULL);
      //////m_pLookupTableIndex = new BLTIndex(this->m_pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer, m_pLookupTable);
      // note - index has no columns, and is not built at this point

      // start getting field/value pairs
      start = end + 1;
      while (*start == ' ') start++;

      // iterate through field list, looking for field/value pairs
      do {
         bool isValueFieldRef = false;
         int  mapCol = -1;
         int  doCol = -1;
         VData keyValue;
         LPTSTR fieldName = start;

         LPTSTR value = (LPTSTR)_tcschr(fieldName, '=');
         if (value == NULL)
            {
            CString msg("Cost Item:  Error parsing lookup entry '");
            msg += this->m_lookupStr;
            msg += "'.  This should be of the form 'path|lookupCol=lookupValue'.  This constraint will be ignored...";
            Report::ErrorMsg(msg);
            return false;
            }

         *value = NULL;  // terminate fieldname
         doCol = m_pLookupTable->GetCol(fieldName);
         //////mapCol = this->m_pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer->GetFieldCol(fieldName);

         if (doCol < 0)
            {
            CString msg("Cost Item:  Error parsing 'lookup' expression '");
            msg += this->m_lookupStr;
            msg += "': Column not found in lookup table ";
            msg += m_pLookupTable->GetName();
            Report::ErrorMsg(msg);
            return false;
            }

         if (mapCol < 0)
            {
            CString msg("Cost Item:  Error parsing 'lookup' expression '");
            msg += this->m_lookupStr;
            msg += "': Column not found in IDU coverage";
            Report::ErrorMsg(msg);
            return false;
            }

         value++;        // value starts after '='
         while (*value == ' ') value++; // skip leading blanks

         // value can be a literal value (e.g. 7) or a field self-reference e.g. @SomeField)
         if (*value == '@')
            {
            isValueFieldRef = true;

            LPTSTR fieldRef = value + 1;
            LPTSTR end = fieldRef;
            while (isalnum(*end) || *end == '_' || *end == '-')
               end++;
            TCHAR temp = *end;
            *end = NULL;

            ////////int col = this->m_pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer->GetFieldCol(fieldRef);
            ////////if (col < 0)
            ////////   {
            ////////   CString msg;
            ////////   msg.Format("Error parsing Policy Constraint: lookup expression field reference @%s was not found in IDU dataset", fieldRef);
            ////////   Report::LogWarning(msg);
            ////////   continue;
            ////////   }

            ////////keyValue = col;
            *end = temp;
            }
         else
            {
            keyValue.Parse(value);  // this is the right hand side value
            isValueFieldRef = false;
            }

         B_COL_INFO* pInfo = new B_COL_INFO;
         pInfo->doCol = doCol;
         pInfo->mapCol = mapCol;
         pInfo->isValueFieldRef = isValueFieldRef;
         pInfo->keyValue = keyValue;
         pInfo->fieldName = fieldName;

         m_pLookupTableIndex->AddCol(pInfo);

         start = _tcschr(value + 1, ';');   // look for a trailing ';'
         if (start)
            start++;
         } while (start != NULL);

         m_pLookupTableIndex->Build();  // creates lookups
      }  // end of: if lookupExpr != NULL 

   // lookup table taken care of (if needed), evaluate cost expressions
   ParseCost(m_pLookupTable, m_initialCostExpr, m_initialCost, m_colInitialCost, m_pInitCostMapExpr);
   ParseCost(m_pLookupTable, m_maintenanceCostExpr, m_maintenanceCost, m_colMaintenanceCost, m_pMainCostMapExpr);

   float duration = 0; int col = 0;   // these are ignored
   ParseCost(NULL, m_durationExpr, duration, col, m_pDurationMapExpr);
   m_duration = (int)duration;
   return true;
   }


bool CostItem::ParseCost(VDataObj* pLookupTable, LPCTSTR costExpr, float& cost, int& col, MapExpr*& pMapExpr)
   {
   if (costExpr == NULL)
      return false;

   // expression can be:
   // 1) a simple number  - starts with a number or -, no alphas 
   // 2) a Map Expression  
   // 3) a lookup table column  - m_pLookupTable != NULL .

   // is there a lookup table?  Then this should be a column name
   if (pLookupTable != NULL)
      {
      col = pLookupTable->GetCol(costExpr);
      if (col < 0)
         {
         CString msg("Cost Item:  Error parsing cost expression '");
         msg += costExpr;
         msg += "': Column not found in lookup table ";
         msg += pLookupTable->GetName();
         Report::ErrorMsg(msg);
         return false;
         }
      }
   else  // not a lookup table, is it a number or expression
      {
      // 
      bool isNumber = true;
      int len = (int) ::strlen(costExpr);
      for (int i = 0; i < len; i++)
         {
         if (isalpha(costExpr[i]) || costExpr[i] == '_')
            {
            isNumber = false;
            break;
            }
         }

      if (isNumber)
         cost = (float)atof(costExpr);

      else   // its a MapExpr
         {
         CString varName(m_name);
         varName.Replace(' ', '_');
         //pMapExpr = Budget::m_pMapExprEngine->AddExpr((PCTSTR)varName, costExpr);
         pMapExpr = m_pMapLayer->GetMapExprEngine()->AddExpr((PCTSTR)varName, costExpr);
         bool ok = pMapExpr->Compile();

         if (!ok)
            {
            CString msg;
            msg.Format("Unable to compile Cost Item expression: %s", m_name);
            Report::ErrorMsg(msg);
            return false;
            }
         if (pMapExpr->IsGlobal())
            {
            bool ok = pMapExpr->Evaluate();

            if (!ok)
               {
               CString msg;
               msg.Format("Unable to evaluate Cost Item expression %s as global expression", m_name);
               Report::LogWarning(msg);
               return false;
               }
            else
               cost = (float)pMapExpr->GetValue();
            }
         }
      }
   return true;
   }


bool CostItem::GetInitialCost(int idu, float area, float& cost)
   {
   bool found = GetCost(idu, m_pInitCostMapExpr, m_initialCost, m_colInitialCost, cost);

   if (!found)
      {
      cost = 0;
      return false;
      }

   cost = AdjustCostToBasis(cost, area);
   return true;
   }


bool CostItem::GetMaintenanceCost(int idu, float area, float& cost)
   {
   bool found = GetCost(idu, m_pMainCostMapExpr, m_maintenanceCost, m_colMaintenanceCost, cost);

   if (!found)
      {
      cost = 0;
      return false;
      }

   cost = AdjustCostToBasis(cost, area);
   return true;
   }


float CostItem::AdjustCostToBasis(float cost, float area)
   {
   if (cost == 0)
      return 0;

   switch (m_basis)
      {
      case CIB_UNKNOWN:
         return cost;

      case CIB_UNIT_AREA:
         return cost * area;

      case CIB_ABSOLUTE:
         return cost;

      default:
         ASSERT(0);
         return 0;
      }

   return 0;
   }


// gets the non-basis-adjusted (raw) cost from the lookup table, const value, or map expr.  This should be 
// adjusted to the basis to estimate the total actual cost of the policy for an IDU
bool CostItem::GetCost(int idu, MapExpr* pMapExpr, float costToUse, int colCost, float& cost)
   {
   if (m_pLookupTable == NULL)
      {
      // is it a non-global map expression?
      if (pMapExpr)
         {
         if (pMapExpr->IsGlobal() == false)
            {
            pMapExpr->Evaluate(idu);
            cost = (float)pMapExpr->GetValue();
            }
         }
      else
         cost = costToUse;

      return true;
      }

   // else its a lookup table
   if (colCost < 0)
      {
      cost = 0;
      return false;
      }

   ASSERT(m_pLookupTableIndex != NULL);
   ASSERT(m_pLookupTableIndex->IsBuilt());

   /////////////////////////////
   //  ///// test
   //   int colManage   = m_pLookupTable->GetCol( "manage" );
   //   int colVegclass = m_pLookupTable->GetCol( "vegclass" );
   //   WORD low  = (WORD) m_pLookupTable->GetAsUInt( colManage, 0 );
   //   WORD high = (WORD) m_pLookupTable->GetAsUInt( colVegclass, 0 );
   //   DWORD key = MAKELONG( low, high );
   //
   //   int row = -1;
   //   bool ok = m_pLookupTableIndex->m_map.Lookup( key, row );
   //   if ( ok )
   //      ok = false;
   //   else
   //      ok = true;
   //////////////////////////////////////

      // basic idea:  we want to:
      //  1) get any lookup columns associated with this policy constraint,
      //  2) evaluate the lookup value(s)
      //  3) generate a lookup key
      //  4) look up the associated value in the attached dataobj

   cost = 0;
   bool found = m_pLookupTableIndex->Lookup(idu, colCost, cost);

   return found ? true : false;
   }



// gets the non-basis-adjusted (raw) cost from the lookup table, const value, or map expr.  This should be 
// adjusted to the basis to estimate the total actual cost of the policy for an IDU
bool CostItem::GetDuration(int idu, int& duration)
   {
   // is it a non-global map expression?
   if (m_pDurationMapExpr)
      {
      if (m_pDurationMapExpr->IsGlobal() == false)
         {
         m_pDurationMapExpr->Evaluate(idu);
         duration = (int)m_pDurationMapExpr->GetValue();
         }
      else  // global, updated already, so just return value
         duration = m_duration;
      }
   else
      duration = m_duration;

   return true;
   }

BudgetItem* Budget::FindBudgetItem(LPCTSTR name)
   {
   int count = (int) m_budgetItems.GetSize();
   for (int i = 0; i < count; i++)
      {
      BudgetItem* pConstraint = m_budgetItems[i];

      if (pConstraint->m_name.CompareNoCase(name) == 0)
         return pConstraint;
      }

   return NULL;
   }
