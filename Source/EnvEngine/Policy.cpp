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
// Policy.cpp: implementation of the Policy class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Policy.h"
#include "EnvModel.h"
#include "DeltaArray.h"
#include "Actor.h"
#include "Scenario.h"
//#include "SaveXmlOptionsDlg.h"
#include <PathManager.h>
#include <MAP.h>
#include <Maplayer.h>
#include <PolyBuffer.h>
#include <PolyClipper.h>
#include <mtparser/MTParser.h>
#include <Path.h>


// Globals
extern CString pogBuffer;
extern int pogIndex;
extern CString poPolicyName;
extern Policy *poPolicy;

extern MultiOutcomeArray *pogpMultiOutcomeArray;
//extern QueryEngine     *PolicyManager::m_pQueryEngine;
extern int poparse( void );
//extern EnvModel        *gpModel;
//extern CEnvDoc         *gpDoc;
//extern ActorManager    *gpActorManager;
//extern ScenarioManager *gpScenarioManager;

//bool ENVAPI BufferOutcomeFunction::m_enabled = true;
//QueryEngine *PolicyManager::m_pQueryEngine = NULL;


PtrArray< VDataObj > PolicyConstraint::m_lookupTableArray( true );


GOAL_SCORE::~GOAL_SCORE()
   {
   if ( pQuery )
      pQuery->GetQueryEngine()->RemoveQuery( pQuery );  // deletes the query
   }

SCORE_MODIFIER::~SCORE_MODIFIER()
   {
   if ( pQuery )
      pQuery->GetQueryEngine()->RemoveQuery( pQuery );
   }


ObjectiveScores *ObjectiveArray::AddObjective( LPCTSTR name, int metagoalIndex )
   { 
   ObjectiveScores *pArray = new ObjectiveScores( name, metagoalIndex ); 
   this->Add( pArray );
   return pArray;
   }

ObjectiveScores &ObjectiveScores::operator = ( ObjectiveScores &osa ) 
   {
   m_metagoalIndex = osa.m_metagoalIndex;
   //m_modelID= osa.m_modelID;
   m_objectiveName = osa.m_objectiveName;

   for ( int i=0; i < osa.m_goalScoreArray.GetSize(); i++ )
      m_goalScoreArray.Add( osa.m_goalScoreArray.GetAt( i ) );

   for ( int i=0; i < osa.m_modifierArray.GetSize(); i++ )
      m_modifierArray.Add( osa.m_modifierArray.GetAt( i ) );

   return *this;
   }


void ObjectiveScores::Clear()
   {
   m_goalScoreArray.RemoveAll();
   m_modifierArray.RemoveAll();
   }


ObjectiveArray &ObjectiveArray::operator = (ObjectiveArray &oa )
   {
   for ( int i=0; i < oa.GetSize(); i++ )
      {
      this->Add( new ObjectiveScores( *oa.GetAt( i ) ) );
      }

   return *this;
   }


OutcomeInfo::OutcomeInfo( Policy *_pPolicy, LPCTSTR _field, VData _value, int _delay, OutcomeDirective *pDirective ) 
   : pPolicy( _pPolicy )
   , value( _value )
   , col( -1 )
   , m_pMapExpr( NULL )
   , m_pFunction( NULL )
   , m_pDirective( pDirective )
   {
   lstrcpy( field, _field );

   // if value is string, need to determine if it is an equation
   if ( value.GetType() == TYPE_STRING || value.GetType() == TYPE_DSTRING )
      {
      CString name;
      name.Format("Policy_%i", pPolicy->m_pPolicyManager->m_nextPolicyID );

      m_pMapExpr = pPolicy->m_pPolicyManager->m_pEnvModel->m_pExprEngine->AddExpr( (PCTSTR) name, value.val.vString, NULL );
      bool ok = m_pMapExpr->Compile();

      if ( ! ok )
         {
         CString msg;
         msg.Format( "Unable to compile Policy Outcome expression: %s", value.val.vString );
         Report::ErrorMsg( msg );
         return;            
         }         
      }
   }


void OutcomeInfo::ToString( CString &outcomeStr, bool useHtml )
   {
   if ( m_pFunction )
      {
      m_pFunction->ToString( outcomeStr );
      return;
      }

   // is there a descriptor for the attribute?
   MAP_FIELD_INFO *pInfo = this->pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer->FindFieldInfo( field );

   CString label;

   if ( pInfo != NULL )  // found field info.  get attributes to populate box
      {
      FIELD_ATTR *pAttr = NULL;
      if ( ::IsInteger( pInfo->type ) )
         {
         int _value;
         value.GetAsInt( _value );
         pAttr = pInfo->FindAttribute( _value );
         }
      else if ( ::IsString( pInfo->type ) )
         {
         CString _value;
         value.GetAsString( _value );
         pAttr = pInfo->FindAttribute( _value );
         }

      if ( pAttr != NULL )
         label = pAttr->label;
      }

   CString _value;

   if( useHtml )
      {
      if ( label.IsEmpty() )
         value.GetAsString( _value );
      else
         _value = "<i>" + label + "</i>";

      outcomeStr = field;
      outcomeStr += "=";
      if ( this->m_pMapExpr != NULL ) outcomeStr += "eval( ";
      outcomeStr += _value;
      if ( this->m_pMapExpr != NULL ) outcomeStr += " )";
      }
   else
      {   
      if ( value.GetType() == TYPE_DSTRING || value.GetType() == TYPE_STRING )
         {
         if ( this->m_pMapExpr == NULL )
            outcomeStr.Format( "%s=\"%s\"", field, value.GetAsString() );
         else
            outcomeStr.Format( "%s=eval( %s )", field, value.GetAsString() );
         }

      else
         outcomeStr.Format( "%s=%s", field, value.GetAsString() );

      if ( ! label.IsEmpty() )
         {
         outcomeStr += "{";
         outcomeStr += label;
         outcomeStr += "}";
         }
      }

   if ( m_pDirective )
      {
      CString directive;
      m_pDirective->ToString( directive );
      outcomeStr += directive;
      }
   }


void OutcomeDirective::ToString( CString& str )
   {
   switch( type )
      {
      case ODT_EXPIRE:
         str.Format( "[e:%i,%s]", period, outcome.GetAsString() );
         return;
         
      case ODT_PERSIST:
         str.Format( "[p:%i]", period );
         return;
         
      case ODT_DELAY:
         str.Format( "[d:%i]", period );
         return;
      }

   str.Empty();
   return;
   }


void MultiOutcomeInfo::ToString( CString &moStr, bool includeOutcomeProb /*=true*/ )
   {
   if ( outcomeInfoArray.GetSize() == 0 )
      {
      moStr.Empty();
      return;
      }

   for ( int i=0; i < outcomeInfoArray.GetSize(); i++ )
      {
      OutcomeInfo *pInfo = outcomeInfoArray.GetAt( i );
      CString oStr;
      pInfo->ToString( oStr, false );
      moStr += oStr;

      if( i != outcomeInfoArray.GetSize()-1 )
         moStr += " and ";
      }

   if ( includeOutcomeProb && probability >= 0 )
      {
      moStr += ":";
      char pStr[ 16 ];

      sprintf_s( pStr, 16, "%6.2f", probability );

      //_itof_s( probability, pStr, 8, 10 );
      moStr += pStr;
      }
   }

//-----------------------------------------------------------------
//    G L O B A L    C O N S T R A I N T S
//-----------------------------------------------------------------

LTIndex &LTIndex::operator = ( LTIndex &index )
   {
   m_keyColArray     = index.m_keyColArray;
   this->m_isBuilt   = index.m_isBuilt;
   //this->m_map       = index.m_map;
   this->m_pDataObj  = index.m_pDataObj;
   this->m_pMapLayer = index.m_pMapLayer;
   this->m_size      = index.m_size;
   
   POSITION pos = index.m_map.GetStartPosition();
   m_map.RemoveAll();
   m_map.InitHashTable( index.m_map.GetHashTableSize() );
   UINT  key   = 0;
   int   value = 0;
   
   while (pos != NULL)
      {
      index.m_map.GetNextAssoc(pos, key, value );
      m_map.SetAt( key, value );
      }

   return *this;
   }


UINT LTIndex::GetKeyFromMap( int idu )
   {
   int count = (int) m_keyColArray.GetSize();

   switch( count )
      {
      case 1:
         {
         UINT key = GetColValue( 0, idu );
         return key;
         }
         
      case 2:
         {
         WORD low  = GetColValue( 0, idu );
         WORD high = GetColValue( 1, idu );
         DWORD key = MAKELONG( low, high );
         return key;
         }

      case 3:
         {
         UINT val0 = GetColValue( 0, idu );
         UINT val1 = GetColValue( 1, idu );
         UINT val2 = GetColValue( 2, idu );
         val0 = val0 << 22;
         val1 = val1 << 11;
         UINT key = val0 | val1 | val2;
         return key;
         }

      case 4:
         {
         UINT val0 = GetColValue( 0, idu );
         UINT val1 = GetColValue( 1, idu );
         UINT val2 = GetColValue( 2, idu );
         UINT val3 = GetColValue( 3, idu );
         val0 = val0 << 24;
         val1 = val1 << 16;
         val2 = val2 << 8;
         UINT key = val0 | val1 | val2 | val3;
         return key;
         }
      }

   return -1;   
   }


UINT LTIndex::GetKey( int row )  
   {
   int count = (int) m_keyColArray.GetSize();

   if ( count == 1 )
      return (UINT) m_pDataObj->GetAsUInt( m_keyColArray[ 0 ]->doCol, row );

   if ( count == 2 )
      {
      WORD low  = (WORD) m_pDataObj->GetAsUInt( m_keyColArray[ 0 ]->doCol, row );
      WORD high = (WORD) m_pDataObj->GetAsUInt( m_keyColArray[ 1 ]->doCol, row );
      DWORD key = MAKELONG( low, high );
      return key;
      }

   if ( count == 3 )
      {
      UINT val0 = m_pDataObj->GetAsUInt( m_keyColArray[ 0 ]->doCol, row ); 
      UINT val1 = m_pDataObj->GetAsUInt( m_keyColArray[ 1 ]->doCol, row ); 
      UINT val2 = m_pDataObj->GetAsUInt( m_keyColArray[ 2 ]->doCol, row ); 
      
      val0 = val0 << 22;
      val1 = val1 << 11;
      UINT key = val0 | val1 | val2;
      return key;
      }

   if ( count == 4 )
      {
      UINT val0 = m_pDataObj->GetAsUInt( m_keyColArray[ 0 ]->doCol, row ); 
      UINT val1 = m_pDataObj->GetAsUInt( m_keyColArray[ 1 ]->doCol, row ); 
      UINT val2 = m_pDataObj->GetAsUInt( m_keyColArray[ 2 ]->doCol, row ); 
      UINT val3 = m_pDataObj->GetAsUInt( m_keyColArray[ 3 ]->doCol, row ); 
      
      val0 = val0 << 24;
      val1 = val1 << 16;
      val2 = val2 << 8;
      UINT key = val0 | val1 | val2 | val3;
      return key;
      }

   return -1;   
   }


GlobalConstraint::GlobalConstraint( PolicyManager *pPM, LPCTSTR name, CTYPE type, LPCTSTR expr )
: m_pPolicyManager(pPM)
, m_name( name )
, m_type( type )
, m_expression( expr )
, m_pMapExpr( NULL )
, m_value( 0 )
, m_currentValue( 0 )
, m_cumulativeValue( 0 )
, m_currentMaintenanceValue( 0 )
, m_currentInitValue( 0 )
   {
   // what kind of expression?  can be a constant of an evaluable expression.
   // ASSUME ALWAYS A MAPEXPR FOR NOW, but must be global
   m_pMapExpr = m_pPolicyManager->m_pEnvModel->AddExpr( name, expr );
   ASSERT ( m_pMapExpr->IsGlobal() );
   if ( m_pPolicyManager->m_pEnvModel->EvaluateExpr( m_pMapExpr, false, m_value ) == false )
      {
      CString msg;
      msg.Format( "Global Constraint '%s': Unable to evaluate expression '%s'", name, expr );
      Report::LogWarning( msg );
      m_value = 0;
      }
   }


void GlobalConstraint::Reset( void  )
   {
   m_cumulativeValue += m_currentValue; 
   m_currentValue = 0;
   m_currentMaintenanceValue = 0;
   m_currentInitValue = 0;

   // get the value by evaluating the asociated expression
   if ( m_pMapExpr != NULL && m_pMapExpr->IsGlobal() )
      {
      bool ok = m_pPolicyManager->m_pEnvModel->EvaluateExpr( m_pMapExpr, false, m_value );

      if ( !ok )
         m_value = 0;
      }
   else
      m_value = 0;
   }

// a constraint applies if it has been exceeded
// gets called by EnvModel::DoesPolicyApply()
bool GlobalConstraint::DoesConstraintApply( float costIncrement )
   {
   if ( m_type == CT_MAX_COUNT )
      costIncrement = 1;

   switch( m_type )
      {
      case CT_UNKNOWN:
         return false;      // never applies (never constrains)  

      case CT_RESOURCE_LIMIT:
      case CT_MAX_AREA:
      case CT_MAX_COUNT:
         return ( m_currentValue + costIncrement ) >= m_value ? true : false;
      }

   return false;
   }


// ApplyPolicyConstraint() applies a given PolicyConstraint to this global constraint,
// subtracting the specified cost from the global pool 
bool GlobalConstraint::ApplyPolicyConstraint( PolicyConstraint *pConstraint, int idu, float area, bool useInitCost )
   {
   bool ok = true;

   switch( m_type )
      {
      case CT_UNKNOWN:
         return false;
         
      case CT_RESOURCE_LIMIT:
         {
         // note: positive costs indicate the policy consumes available resources,
         // negative costs (returns) do not get counted
         float cost = 0;
         if ( useInitCost )
            ok = pConstraint->GetInitialCost( idu, area, cost );   // these are basis-adjusted (total) costs 
         else
            ok = pConstraint->GetMaintenanceCost( idu, area, cost );

         if ( ! ok )
            return false;

         // generating a return?  Then ignore
         if ( cost <= 0 && m_pPolicyManager->m_pEnvModel->m_addReturnsToBudget == false )
            return true;
         
         if ( useInitCost )
            m_currentInitValue += cost;
         else
            m_currentMaintenanceValue += cost;

         m_currentValue += cost;
         pConstraint->m_pPolicy->m_cumCost += cost;
         }
         return true;

      case CT_MAX_AREA:
         m_currentValue += area;
         pConstraint->m_pPolicy->m_cumCost += area;
         return true;

      case CT_MAX_COUNT:
         m_currentValue += 1;
         pConstraint->m_pPolicy->m_cumCost++;
         return true;
         
      default:
         ASSERT( 0 );
         return false;
      }

   return false;
   }


PolicyConstraint::PolicyConstraint( Policy *pPolicy, LPCTSTR name, CBASIS basis, LPCTSTR initCostExpr, LPCTSTR maintenanceCostExpr, LPCTSTR durationExpr, LPCTSTR lookupExpr ) 
   : m_pPolicy( pPolicy )
   , m_name( name )
   , m_gcName( "" )
   , m_basis( basis )
   , m_initialCost( 0 )
   , m_maintenanceCost( 0 )
   , m_duration( 0 )
   , m_pGlobalConstraint( NULL )
   , m_pLookupTable( NULL )
   , m_pLookupTableIndex( NULL )
   , m_colMaintenanceCost( -1 )
   , m_colInitialCost( -1 )
   , m_initialCostExpr( initCostExpr )
   , m_maintenanceCostExpr( maintenanceCostExpr )
   , m_durationExpr(durationExpr)
   {
   if ( lookupExpr != NULL )
      {
      // note:up to two lookup expresions are supported, of the form:
      // lookup='Eugene_Manage_Costs.csv|manage=14;vegclass=@vegclass'

      // first, get filename 
      TCHAR buffer[ 512 ];
      lstrcpy( buffer, lookupExpr );
      LPTSTR start = buffer;
      LPTSTR filename = start;

      // parse expression: tablename:col=value
      LPTSTR end = _tcschr( filename, '|' );
      if ( end == NULL )
         {
         CString msg( "Policy Constraint:  Error parsing lookup entry '" );
         msg += lookupExpr;
         msg += "'.  This should be of the form 'path|lookupCol=lookupValue'.  This constraint will be ignored...";
         Report::ErrorMsg( msg );
         return;
         }
      *end = NULL;

      // seen this table before?  If not, we need to load it and add it to the table list
      bool found = false;
      for ( int i=0; i < (int) m_lookupTableArray.GetSize(); i++ )
         {
         if ( lstrcmpi( m_lookupTableArray[ i ]->GetName(), filename ) == 0 )
            {
            m_pLookupTable = m_lookupTableArray[ i ];
            found = true;
            break;
            }
         }

      if ( ! found )
         {
         // was a full path specified?  if not, add IDU path
         LPTSTR bslash = _tcschr( filename, '\\' );
         LPTSTR fslash = _tcschr( filename, '/' );

         nsPath::CPath path;
         if ( bslash || fslash )
            path = filename;  // no modification necessary if full path specified
         else
            {
            path = this->m_pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer->m_path;
            path.MakeFullPath();
            path.RemoveFileSpec();
            path.Append( filename );
            }            

         VDataObj *pData = new VDataObj(U_UNDEFINED);
         int count = pData->ReadAscii( path );

         if ( count <= 0 )
            {
            CString msg( "Policy Constraint: Error loading lookup table '" );
            msg += filename;
            msg += "'.  This constraint will be ignored...";
            Report::ErrorMsg( msg );
            delete pData;
            return;
            }
         else
            {
            m_pLookupTable = pData;
            m_pLookupTable->SetName( filename );
            m_lookupTableArray.Add( pData );
            }
         }

      // at this point, we should have a valid m_pLookupTable VDataObj loaded.
      // add an index
      ASSERT( m_pLookupTableIndex == NULL );
      m_pLookupTableIndex = new LTIndex(this->m_pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer, m_pLookupTable );
      // note - index has no columns, and is not built at this point

      // start getting field/value pairs
      start = end+1; 
      while( *start == ' ' ) start++;
     
      // iterate through field list, looking for field/value pairs
      do {
         bool isValueFieldRef = false;
         int  mapCol = -1;
         int  doCol  = -1;
         VData keyValue;
         LPTSTR fieldName = start;
         
         LPTSTR value = (LPTSTR) _tcschr( fieldName, '=' );
         if ( value == NULL )
            {
            CString msg( "Policy Constraint:  Error parsing lookup entry '" );
            msg += lookupExpr;
            msg += "'.  This should be of the form 'path|lookupCol=lookupValue'.  This constraint will be ignored...";
            Report::ErrorMsg( msg );
            return;
            }

         *value = NULL;  // terminate fieldname
         doCol  = m_pLookupTable->GetCol( fieldName );
         mapCol = this->m_pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer->GetFieldCol( fieldName );

         if ( doCol < 0 )
            {
            CString msg( "Policy Constraint:  Error parsing 'lookup' expression '" );
            msg += lookupExpr;
            msg += "': Column not found in lookup table ";
            msg += m_pLookupTable->GetName();
            Report::ErrorMsg( msg );
            return;
            }

         if ( mapCol < 0 )
            {
            CString msg( "Policy Constraint:  Error parsing 'lookup' expression '" );
            msg += lookupExpr;
            msg += "': Column not found in IDU coverage";
            Report::ErrorMsg( msg );
            return;
            }

         value++;        // value starts after '='
         while (*value == ' ') value++; // skip leading blanks

         // value can be a literal value (e.g. 7) or a field self-reference e.g. @SomeField)
         if ( *value == '@' )
            {
            isValueFieldRef = true;

            LPTSTR fieldRef = value + 1;
            LPTSTR end = fieldRef;
            while( isalnum( *end ) || *end == '_' || *end == '-' )
               end++;
            TCHAR temp = *end;
            *end = NULL;

            int col = this->m_pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer->GetFieldCol( fieldRef );
            if ( col < 0 )
               {
               CString msg;
               msg.Format( "Error parsing Policy Constraint: lookup expression field reference @%s was not found in IDU dataset", fieldRef );
               Report::LogWarning( msg );
               continue;
               }
            
            keyValue = col;
            *end = temp;
            }
         else
            {
            keyValue.Parse( value );  // this is the right hand side value
            isValueFieldRef = false;
            }

         COL_INFO *pInfo = new COL_INFO;
         pInfo->doCol = doCol;
         pInfo->mapCol = mapCol;
         pInfo->isValueFieldRef = isValueFieldRef;
         pInfo->keyValue = keyValue;
         pInfo->fieldName = fieldName;

         m_pLookupTableIndex->AddCol( pInfo );

         start = _tcschr( value+1, ';' );   // look for a trailing ';'
         if ( start )
            start++;
      } while ( start != NULL );

      m_pLookupTableIndex->Build();  // creates lookups
      }  // end of: if lookupExpr != NULL 

   // lookup table taken care of (if needed), evaluate cost expressions
   ParseCost( m_pLookupTable, initCostExpr, m_initialCost, m_colInitialCost, m_pInitCostMapExpr );
   ParseCost(m_pLookupTable, maintenanceCostExpr, m_maintenanceCost, m_colMaintenanceCost, m_pMainCostMapExpr );

   float duration = 0; int col = 0;   // these are ignored
   ParseCost(NULL, durationExpr, duration, col, m_pDurationMapExpr);
   m_duration = (int)duration;
   }


bool PolicyConstraint::ParseCost( VDataObj *pLookupTable, LPCTSTR costExpr, float &cost, int &col, MapExpr*& pMapExpr )
   {
   if ( costExpr == NULL )
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
         CString msg("Policy Constraint:  Error parsing cost expression '");
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
         pMapExpr = m_pPolicy->m_pPolicyManager->m_pEnvModel->AddExpr((PCTSTR)varName, costExpr);
         bool ok = pMapExpr->Compile();

         if (!ok)
            {
            CString msg;
            msg.Format("Unable to compile Policy Constraint expression: %s", m_name);
            Report::ErrorMsg(msg);
            return false;
            }
         if (pMapExpr->IsGlobal())
            {
            bool ok = pMapExpr->Evaluate();

            if (!ok)
               {
               CString msg;
               msg.Format("Unable to evaluate Policy Constraint expression %s as global expression", m_name);
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


bool PolicyConstraint::GetInitialCost( int idu, float area, float &cost )
   {
   bool found = GetCost( idu, m_pInitCostMapExpr, m_initialCost, m_colInitialCost, cost );
   
   if ( ! found )
      {
      cost = 0;
      return false;
      }

   cost = AdjustCostToBasis( cost, area );
   return true;
   }


bool PolicyConstraint::GetMaintenanceCost( int idu, float area, float &cost )
   {
   bool found = GetCost( idu, m_pMainCostMapExpr, m_maintenanceCost, m_colMaintenanceCost, cost );

   if ( ! found )
      {
      cost = 0;
      return false;
      }

   cost = AdjustCostToBasis( cost, area );
   return true;
   }


float PolicyConstraint::AdjustCostToBasis( float cost, float area )
   {
   if ( cost == 0 )
      return 0;

   switch ( m_basis )
      {
      case CB_UNKNOWN:
      case CB_UNIT_AREA:
          return cost * area;
             
      case CB_ABSOLUTE:
         return cost;

      default:
         ASSERT( 0 );
         return 0;
      }

   return 0;
   }


// gets the non-basis-adjusted (raw) cost from the lookup table, const value, or map expr.  This should be 
// adjusted to the basis to estimate the total actual cost of the policy for an IDU
bool PolicyConstraint::GetCost( int idu, MapExpr *pMapExpr, float costToUse, int colCost, float &cost )
   {
   if ( m_pLookupTable == NULL )
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
   if ( colCost < 0 )
      {
      cost = 0;
      return false;
      }

   ASSERT( m_pLookupTableIndex != NULL );
   ASSERT( m_pLookupTableIndex->IsBuilt() );

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
   bool found = m_pLookupTableIndex->Lookup( idu, colCost, cost );

   return found ? true : false;
   }



// gets the non-basis-adjusted (raw) cost from the lookup table, const value, or map expr.  This should be 
// adjusted to the basis to estimate the total actual cost of the policy for an IDU
bool PolicyConstraint::GetDuration(int idu, int &duration)
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

//////////////////////////////////////////////////////////////////////
// Policy Construction/Destruction
//////////////////////////////////////////////////////////////////////

Policy::Policy( PolicyManager *pPM )
   : m_pPolicyManager( pPM ),
     m_id( -1 ),
     m_index( -1 ),
     m_name(),
     m_color( RGB( 255, 255, 255 ) ),
     m_exclusive( false ),
     m_persistenceMin( -1 ),
     m_persistenceMax( -1 ),
     m_pMapLayer( NULL ),
     //m_preference( 0 ),
     m_age( 0 ),
     m_use( true ),
     m_useInSensitivity( 0 ),
     m_isScheduled( false ),
     m_mandatory( false ),
     m_compliance( 0.90f ),
     m_startDate( -1 ),
     m_endDate( -1 ),
     m_isShared( true ),
     m_isEditable( true ),
     m_isAdded( false ),
     m_pSiteAttrQuery( NULL ),
     m_appliedCount( 0 ),
     m_cumAppliedCount( 0 ),
     m_appliedArea( 0 ),
     m_expansionArea( 0 ),
     m_cumAppliedArea( 0 ),
     m_cumExpansionArea( 0 ),
     m_potentialArea( 0 ),
     m_passedCount( 0 ),
     m_rejectedSiteAttr( 0 ),
     m_rejectedPolicyConstraint( 0 ),
     m_rejectedNoCostInfo( 0 ),
     m_noOutcomeCount( 0 )
   {
   this->m_objectiveArray.AddObjective( _T("Global"), -1 );  // always one of these, always first on the list
   }  



Policy& Policy::operator = ( Policy &policy )
   {
   m_pPolicyManager  = policy.m_pPolicyManager;
   m_id              = policy.m_id;
   m_index           = policy.m_index;
   m_age             = policy.m_age;
   m_use             = policy.m_use;
   m_useInSensitivity= policy.m_useInSensitivity;
   m_isShared        = policy.m_isShared;
   m_isEditable      = policy.m_isEditable;
   m_isAdded         = policy.m_isAdded;
   m_isScheduled     = policy.m_isScheduled;
   m_mandatory       = policy.m_mandatory;
   m_compliance      = policy.m_compliance;
   m_startDate       = policy.m_startDate;
   m_endDate         = policy.m_endDate;
   m_name            = policy.m_name;
   m_source          = policy.m_source;
   m_originator      = policy.m_originator;
   m_narrative       = policy.m_narrative;
   m_studyArea       = policy.m_studyArea;
   m_color           = policy.m_color;
   m_exclusive       = policy.m_exclusive;
   m_persistenceMin  = policy.m_persistenceMin;
   m_persistenceMax  = policy.m_persistenceMax;
   //m_preference    = policy.m_preference;

   m_siteAttrStr     = policy.m_siteAttrStr;
   m_outcomeStr      = policy.m_outcomeStr;
   //m_prefSelStr      = policy.m_prefSelStr;

   m_appliedCount     = policy.m_appliedCount;
   m_cumAppliedCount  = policy.m_cumAppliedCount;
   m_appliedArea      = policy.m_appliedArea;
   m_expansionArea    = policy.m_expansionArea;
   m_cumAppliedArea   = policy.m_cumAppliedArea;
   m_cumExpansionArea = policy.m_cumExpansionArea;
   m_potentialArea    = policy.m_potentialArea;
   m_passedCount      = policy.m_passedCount;
   m_rejectedSiteAttr = policy.m_rejectedSiteAttr;
   m_rejectedPolicyConstraint = policy.m_rejectedPolicyConstraint;
   m_rejectedNoCostInfo = policy.m_rejectedNoCostInfo;
   m_noOutcomeCount   = policy.m_noOutcomeCount;

   m_pSiteAttrQuery = NULL; // need to compile later... PolicyManager::m_pQueryEngine->ParseQuery(policy.m_siteAttrStr, 0, "Policy Copy Operator");
   m_multiOutcomeArray = policy.m_multiOutcomeArray;

   //m_goalScores.Copy( policy.m_goalScores );
   m_objectiveArray  = policy.m_objectiveArray;

   for ( int i=0; i < (int) policy.m_constraintArray.GetSize(); i++ )
      {
      PolicyConstraint *pConstraint = new PolicyConstraint( *(policy.m_constraintArray[ i ] ) );
      pConstraint->m_pPolicy = this;
      m_constraintArray.Add( pConstraint );
      }

   return *this;
   }


Policy::~Policy()
   {
   m_pPolicyManager = NULL;

   m_objectiveArray.Clear();
   m_constraintArray.Clear();

  
   if ( m_pSiteAttrQuery != NULL && this->m_pPolicyManager != NULL && this->m_pPolicyManager->m_pQueryEngine != NULL )
      this->m_pPolicyManager->m_pQueryEngine->RemoveQuery( m_pSiteAttrQuery );
   }


bool Policy::ParseOutcomes( Policy *pPolicy, CString &outcomeStr, MultiOutcomeArray &moa )
   {
   // note:  Outcomes are in the form of "Attribute=value" pairs, 
   //   nested in 'and'-delineated multiaction items with probabilities, nested in semicolon-delimited alternative outcomes
   //
   // ex:  LULC_C=20 and RIPARIAN="yes":30; LULC_C=10 and RIPARIAN="no":70
   // ex:  LUlC_C=20 and Buffer( arglist ): 100;

   // this parses MultiOutcomeArrays.

   moa.RemoveAll();

   if ( outcomeStr.IsEmpty() )
      return true; 

   pogBuffer  = outcomeStr;
   pogIndex   = 0;
   pogpMultiOutcomeArray = &moa;
   poPolicy = pPolicy; 
   
   int ret = poparse();

   return (ret == 0);
   }


//void Policy::FixUpOutcomePolicyPtrs()
//   {
//   for (int i = 0; i < this->GetMultiOutcomeCount(); i++)
//      {
//      MultiOutcomeInfo &moi = this->m_multiOutcomeArray.GetMultiOutcome(i);
//
//      for (int j = 0; j < moi.GetOutcomeCount(); j++)
//         moi.GetOutcome(j).pPolicy = this;
//      }
//   return;
//   }


bool Policy::SetOutcomes( LPCTSTR outcomes )
   {
   this->ClearMultiOutcomeArray();
   m_outcomeStr = outcomes;
   poPolicyName = this->m_name;

   bool retVal = ParseOutcomes( this, m_outcomeStr, m_multiOutcomeArray );
   return retVal;
   }


int Policy::GetPersistence()
   {
   // get persistence information   
   int actualPersistence;

   if ( m_persistenceMin < m_persistenceMax )
      actualPersistence = (int) this->m_pPolicyManager->m_pEnvModel->m_randUnif.RandValue( m_persistenceMin, m_persistenceMax );
   else
      actualPersistence = m_persistenceMin;

   if ( actualPersistence < 0 )
      actualPersistence = 0;

   return actualPersistence;
   }


float Policy::GetMaxApplicationArea( int idu )
   {
   // iterate through outcomes, looking for Expand() function.  If found, return associated max area.
   // otherwise, return idu area
   int moCount = this->GetMultiOutcomeCount();

   float maxArea = -1;

   for ( int i=0; i < moCount; i++ )
      {
      const MultiOutcomeInfo &mo = GetMultiOutcome( i );

      for ( int j=0; j < mo.GetOutcomeCount(); j++ )
         {
         const OutcomeInfo &oi = mo.GetOutcome( j );

         if ( oi.col < 0 )  // this means it is a function
            {
            ASSERT( oi.m_pFunction != NULL );
            if ( oi.m_pFunction->GetType() == OFT_EXPAND )
               {
               CString strArea;
               if ( oi.m_pFunction->GetArg( 1, strArea ) )   // arg 1 = max expansion area
                  {
                  float area = (float) atof( strArea );
#ifdef NO_MFC
		  using namespace std;
#endif
                  maxArea = max( maxArea, area );
                  }
               }
            }
         }
      }  // end of: outer for()

   // if no area identified, use IDU area
   if ( maxArea < 0 )
      this->m_pPolicyManager->m_pEnvModel->m_pIDULayer->GetData( idu, this->m_pPolicyManager->m_pEnvModel->m_colArea, maxArea );

   return maxArea;
   }



/*
void Policy::SetParserVars( MapLayer *pLayer, int record )
   {
   POSITION pos = m_usedParserVars.GetStartPosition();
   int col;
   int index;
   while (pos != NULL)
      {
      m_usedParserVars.GetNextAssoc( pos, col, index  );
      ASSERT( col == index );

      double value;
      if ( pLayer->GetData( record, col, value ) )
         m_parserVars[ index ] = value;
      }
   }
*/

/*
float Policy::EvaluatePreference()
   {
   // return -3 to +3
   if ( m_pParser == NULL || m_prefSelStr.IsEmpty() )
      return 0;

   m_preference = (float) m_pParser->evaluate();

   if ( m_preference > 3 )
      m_preference = 3;
   else if ( m_preference < -3 )
      m_preference = -3;

   return m_preference;
   }
*/

ObjectiveScores *Policy::FindObjective( LPCTSTR objName )
   {
   for ( int i=0; i < m_objectiveArray.GetSize(); i++ )
      {
      ObjectiveScores *pArray = m_objectiveArray.GetAt( i );
      
      if ( pArray->m_objectiveName.CompareNoCase( objName ) == 0 )
         return pArray;
      }

   return NULL;
   }


float Policy::GetGoalScore( int objectiveIndex )  // NOTE:  -1=global objective
   {
   ASSERT( objectiveIndex <= m_objectiveArray.GetSize() );

   ObjectiveScores *pArray = m_objectiveArray.GetAt( objectiveIndex+1 );
   ASSERT( pArray != NULL );

   float score = 0;
   for ( int j=0; j < pArray->m_goalScoreArray.GetSize(); j++ )
      score += pArray->m_goalScoreArray.GetAt( j ).score;

   score /= pArray->m_goalScoreArray.GetSize();

   return score;
   }


void Policy::GetIduObjectiveScores( int idu, float* scores )
   {
   int objectiveCount = (int) m_objectiveArray.GetSize();
   ASSERT( objectiveCount == this->m_pPolicyManager->m_pEnvModel->GetMetagoalCount()+1 );   // includes global score

   // for each objective (except for global, the first one), 
   // get the objective score for each objective
   for ( int i=1; i < objectiveCount; i++ )
      scores[ i-1 ] = GetIduObjectiveScore( idu, i-1 );
   }



float Policy::GetIduObjectiveScore( int idu, int objective )  // -1 for global, zero-based for rest
   {
   ASSERT( (m_objectiveArray.GetSize()-1) == this->m_pPolicyManager->m_pEnvModel->GetMetagoalCount() );

   int objIndex = objective+1; // includes global objective

   ObjectiveScores *pArray = m_objectiveArray.GetAt( objIndex );

   // for each goal score in the array, evalue the score for the current idu.
   // NOTE:  Assumes Policy::SetParserVars() has already been called for this idu!!!

   GoalScoreArray &scoreArray = pArray->m_goalScoreArray;
   float totalObjScore = 0;
   int   passedScores = 0;
   for ( int j=0; j < scoreArray.GetSize(); j++ )
      {
      GOAL_SCORE &gs = scoreArray.GetAt( j );

      if( gs.pQuery == NULL )   // applies globally?
         totalObjScore += gs.score;    // note: gs.scores are [-3, +3]
      else
         {
         bool passed;
         gs.pQuery->Run( idu, passed );

         if( passed )
            {
            passedScores++;
            totalObjScore += gs.score;
            }
         }
      }  // end of: if ( j < scoreArray.GetSize() )

   if ( passedScores > 0 )  // rescale to average ([-3 to +3 ]
      totalObjScore /= passedScores;      // NOTE - this should be reviewed!!!! jpb 9/8/09

   // now apply modifiers
   ModifierArray &modArray = pArray->m_modifierArray;
   float totalMods  = 0;   
   int   passedMods = 0;
   for ( int j=0; j < modArray.GetSize(); j++ )
      {
      SCORE_MODIFIER &mod = modArray.GetAt( j );

      switch( mod.type )
         {
         case SM_SCENARIO:    // scenario-specific modification
            {
            if ( (int) mod.value == m_pPolicyManager->m_pEnvModel->GetScenarioIndex()  )
               {
               totalMods += mod.value;
               passedMods++;
               }
            }
            break;

         case SM_INCR:     // increment the value if condition is met
         case SM_DECR:
            {
            if ( mod.pQuery == NULL  )
               {
               totalMods += ( mod.type == SM_INCR ? mod.value : -mod.value );
               passedMods++;
               }
            else
               {
               bool passed;
               mod.pQuery->Run( idu, passed );
               if ( passed )
                  {
                  totalMods += ( mod.type == SM_INCR ? mod.value : -mod.value );
                  passedMods++;
                  }
               }
            }
            break;

         case SM_ABS:
            ASSERT( 0 );
         }  // end of: switch( mod.type )
      }  // end of: for( j < modArray.GetSize();

   if ( passedMods > 0 ) 
      totalMods /= passedMods;      // NOTE - this should be reviewed!!!! jpb 9/8/09

   // now have avg obj score, avg mod, compute final objective score
   totalObjScore += totalMods;

   if( totalObjScore < -3 )
      totalObjScore = -3;

   else if ( totalObjScore > 3 )
      totalObjScore = 3;

   return totalObjScore;
   }

//void Policy::NormalizeOutcomeProbabilities()
//   {
//   int count = m_multiOutcomeArray.GetSize();
//   ASSERT( count > 0 );
//
//   int probSum = 0;
//
//   for ( int i=0; i<count; i++ )
//      probSum += m_multiOutcomeArray.GetMultiOutcome(i).probability;
//
//   MultiOutcomeInfo &lastOutcome = m_multiOutcomeArray.GetMultiOutcome(count-1);
//   lastOutcome.probability = 100;
//
//   for ( int i=0; i < count-1; i++ )
//      {
//      MultiOutcomeInfo &outcome = m_multiOutcomeArray.GetMultiOutcome(i);
//      int prob = int( 100.0f*(float)outcome.probability/(float)probSum );
//      outcome.probability = prob;
//      lastOutcome.probability -= prob;
//      }
//   }


PolicyArray& PolicyArray::operator = ( PolicyArray &policyArray )
   {
   for ( int i=0; i < policyArray.GetSize(); i++ )
      Add( new Policy( *policyArray.GetAt( i ) ) );

   m_deleteOnDelete = policyArray.m_deleteOnDelete;
   return  *this;
   }

void PolicyArray::RemoveAll()
   {
   if ( m_deleteOnDelete )
      for ( int i=0; i < GetSize(); i++ ) 
         delete GetAt( i );
   
   CArray< Policy*, Policy*>::RemoveAll(); 
   }     


///////////////////////////////////////////////////////////////////////////////////////////////
//                              PolicyManager methods
///////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

PolicyManager::PolicyManager( EnvModel *pModel )
: m_pEnvModel(pModel)
, m_nextPolicyID( 0 )
, m_loadStatus( -1 )
, m_includeInSave( true )
   { }


PolicyManager::~PolicyManager()
   { 
   RemoveAllPolicies();
   RemoveAllDeletedPolicies();
   /*
   if ( Policy::m_parserVars != NULL )
      {
      delete Policy::m_parserVars;
      Policy::m_parserVars = NULL;
      } */
   }


Policy *PolicyManager::FindPolicy( LPCTSTR name, int *index )
   {
   int len = lstrlen( name );

   for ( int i=0; i < GetPolicyCount(); i++ )
      {
      Policy *pPolicy = GetPolicy( i );

      if ( ( lstrcmp( name, pPolicy->m_name ) == 0 ) && ( pPolicy->m_name.GetLength() == len ) )
         {
         if ( index != NULL )
            *index = i;

         return pPolicy;
         }
      }

   for ( int i=0; i < GetDeletedPolicyCount(); i++ )
      {
      Policy *pPolicy = GetDeletedPolicy( i );
      if ( ( lstrcmp( name, pPolicy->m_name ) == 0 ) && ( pPolicy->m_name.GetLength() == len )  )
         {
         if ( index != NULL )
            *index = i;

         return pPolicy;
         }
      }

   if ( index != NULL )
      *index = -1;

   return NULL;
   }


Policy *PolicyManager::GetPolicyFromID( int id )
   {
   //int count = (int) m_policyArray.GetSize();

   //for ( int i=0; i < count; i++ )
   //   {
   //   if ( m_policyArray[ i ]->m_id == id )
   //      {
   //      //ASSERT( i == id );      // ID should always be offset!!!
   //      return m_policyArray[ i ];
   //      }
   //   }
   int index = this->GetPolicyIndexFromID( id );

   if ( index >= 0 )
      return m_policyArray[ index ];

   return NULL;
   }


Policy *PolicyManager::GetDeletedPolicyFromID( int id )
   {
   int count = (int) m_deletedPolicyArray.GetSize();

   for ( int i=0; i < count; i++ )
      {
      if ( m_deletedPolicyArray[ i ]->m_id == id )
         {
         //ASSERT( i == id );      // ID should always be offset!!!
         return m_deletedPolicyArray[ i ];
         }
      }

   return NULL;
   }


void PolicyManager::DeletePolicy( int index )
   {
   Policy *pPolicy = m_policyArray[ index ];
   m_deletedPolicyArray.Add( pPolicy );

   m_policyArray.RemoveAt( index );
   ResetPolicyIndices();
   }

/*
void PolicyManager::ResetPolicyAppliedCount()
   {
   for ( int i=0; i < m_policyArray.GetSize(); i++ )
      {
      Policy *pPolicy = m_policyArray[ i ];
      pPolicy->m_appliedCount = 0;
      pPolicy->m_cumAppliedCount = 0;
      pPolicy->m_appliedArea = 0;
      pPolicy->m_cumAppliedArea = 0;
      }
   }
*/

void PolicyManager::ResetPolicyIndices()
   {
   m_policyIDToIndexMap.RemoveAll();

   for ( int i=0; i < GetPolicyCount(); i++ )
      {
      Policy *pPolicy = GetPolicy( i );
      pPolicy->m_index = i;
      m_policyIDToIndexMap.SetAt( pPolicy->m_id, i );
      }
   }


int PolicyManager::GetUsedPolicyCount()
   {
   int totalCount = (int) m_policyArray.GetSize();
   int usedCount = 0;

   for ( int i=0; i < totalCount; i++ )
      {
      if ( GetPolicy( i )->m_use )
         usedCount++;
      }

   return usedCount;
   }


int PolicyManager::CompilePolicies( MapLayer *pLayer )
   {
   int total = 0;
   for ( int i=0; i < GetPolicyCount(); i++ )
      {
      // take care of setting up mapping colors
      Policy *pPolicy = (Policy*) GetPolicy( i );
      CompilePolicy( pPolicy, pLayer );
      total++;
      }

   return total;
   }


bool PolicyManager::CompilePolicy( Policy *pPolicy, MapLayer *pLayer )
   {
   // compile site query
   CString& siteAttr = pPolicy->m_siteAttrStr;
   poPolicyName = pPolicy->m_name;

   if ( siteAttr.IsEmpty() || siteAttr.GetLength() == 0 )
      pPolicy->m_pSiteAttrQuery = NULL;

   else
      {
      CString source( "Policy: " );
      source += pPolicy->m_name;

      ASSERT( PolicyManager::m_pQueryEngine != NULL );
      Query *pQuery= PolicyManager::m_pQueryEngine->ParseQuery( pPolicy->m_siteAttrStr, 0, source );

      if ( pQuery )
         {
         pQuery->m_text = pPolicy->m_name;
         pPolicy->m_pSiteAttrQuery = pQuery;
         }
      else
         {
         CString msg( "Unable to compile site attribute query for policy: " );
         msg += pPolicy->m_name;
         msg +=" [";
         msg += pPolicy->m_siteAttrStr;
         msg += "].  This policy will be disabled";
         Report::LogWarning( msg );
         pPolicy->m_pSiteAttrQuery = NULL;
         pPolicy->m_use = false;
         return false;
         }
      }
   // site attributes handled, now fix up outcomes
   ValidateOutcomeFields( pPolicy, pLayer );

   // compile objectives
   for ( int i=0; i < pPolicy->m_objectiveArray.GetSize(); i++ )
      {
      ObjectiveScores *pArray = pPolicy->m_objectiveArray.GetAt( i );

      // first, compile scores
      GoalScoreArray &gsa = pArray->m_goalScoreArray;
      for ( int j=0; j < gsa.GetSize(); j++ )
         {
         GOAL_SCORE &gs = gsa.GetAt( j );

         if ( gs.queryStr.IsEmpty() )
            gs.pQuery = NULL;
         else
            {
            CString source( "Policy: " );
            source += pPolicy->m_name;
            source += "; Objective: ";
            source += pPolicy->m_objectiveArray[ i ]->m_objectiveName;

            gs.pQuery = PolicyManager::m_pQueryEngine->ParseQuery( gs.queryStr, 0, source );

            if ( gs.pQuery == NULL )
               {
               CString msg;
               msg.Format( _T("Error parsing Goal Score query string: " ) );
               msg += gs.queryStr;
               msg += _T(", ignoring...");
               Report::ErrorMsg( msg );
               }
            }
         }

      // next, compile modifers
      ModifierArray &ma = pArray->m_modifierArray;
      for ( int j=0; j < ma.GetSize(); j++ )
         {
         SCORE_MODIFIER &sm = ma.GetAt( j );

         switch( sm.type )
            {
            case SM_SCENARIO:
               {
               ASSERT( sm.scenario >= 0 );
               // just make sure name matches scenario
               Scenario *pScenario = m_pEnvModel->m_pScenarioManager->GetScenario( sm.scenario );
               if ( sm.condition.Compare( pScenario->m_name ) != 0 )
                  {
                  CString msg;
                  msg.Format( _T("Modifier condition [%s] doesn't match specified scenario %i [%s] compiling policy %s... This should be resolved before proceeding"),
                     (LPCTSTR) sm.condition, sm.scenario, (LPCTSTR) pScenario->m_name, (LPCTSTR) pPolicy->m_name );
                  Report::LogWarning( msg );
                  }
               }
               break;

            case SM_INCR:
            case SM_DECR:
               {
               if ( sm.condition.IsEmpty() )
                  sm.pQuery = NULL;
               else
                  {
                  CString source( "Policy: " );
                  source += pPolicy->m_name;
                  source += "; Modifier: ";
                  source += sm.condition;

                  sm.pQuery = PolicyManager::m_pQueryEngine->ParseQuery( sm.condition, 0, source );

                  if ( sm.pQuery == NULL )
                     {
                     CString msg;
                     msg.Format( _T("Error parsing Score Modifier condition query string: " ) );
                     msg += sm.condition;
                     msg += _T(", ignoring...");
                     Report::ErrorMsg( msg );
                     }
                  }
               }
               break;

            case SM_ABS:
               ASSERT( 0 );
            }
         }
      }  // end of:  compile objectives


   // if there is a selection preference, set up a parser to handle it
   /*
   if ( pPolicy->m_prefSelStr.IsEmpty() )
      {
      if ( pPolicy->m_pParser != NULL )
         delete pPolicy->m_pParser;
      }
   else // not empty
      {
      if ( pPolicy->m_pParser == NULL )
         pPolicy->m_pParser = new MTParser;

      if ( Policy::m_parserVars == NULL )
         Policy::m_parserVars = new double[ m_pEnvModel->m_pIDULayer->GetFieldCount() ];

      ASSERT( pPolicy->m_pParser != NULL );
      pPolicy->m_pParser->enableAutoVarDefinition( true );
      pPolicy->m_pParser->compile( pPolicy->m_prefSelStr );

      int varCount = pPolicy->m_pParser->getNbUsedVars();

      for ( int i=0; i < varCount; i++ )
         {
         CString varName = pPolicy->m_pParser->getUsedVar( i ).c_str();

         // is it a legal field in the map?
         int col = m_pEnvModel->m_pIDULayer->GetFieldCol( varName );

         if ( col < 0 )
            {
            CString msg( "Unable to compile preferential selection expression for policy: " );
            msg += pPolicy->m_name;
            msg +=" [";
            msg += pPolicy->m_prefSelStr;
            msg += "].  This preference will be ignored";
            Report::LogWarning( msg );
            delete pPolicy->m_pParser;
            pPolicy->m_prefSelStr.Empty();
            pPolicy->m_pParser = NULL;
            return true;
            }

         // have we already seen this column?
         int index = col;
         if ( Policy::m_usedParserVars.Lookup( col, index ) )
            {
            ASSERT( col == index );
            }
         else  // add it
            Policy::m_usedParserVars.SetAt( col, col );

         pPolicy->m_pParser->redefineVar( varName, Policy::m_parserVars+index); 
         }  // end of:  for ( i < varCount )
      }  // end of: else not empty
*/
   return true;
   }


bool PolicyManager::ValidateOutcomeFields( Policy *pPolicy, MapLayer *pLayer )
   {
   poPolicyName = pPolicy->m_name;

   ASSERT( pLayer );

   for ( int j=0; j < pPolicy->GetMultiOutcomeCount(); j++ )
      {
      MultiOutcomeInfo &m = pPolicy->m_multiOutcomeArray.GetMultiOutcome( j );
      for ( int k=0; k < m.GetOutcomeCount(); k++ )
         {
         OutcomeInfo &outcome = m.GetOutcome( k );

         outcome.pPolicy = pPolicy;

         if ( outcome.m_pFunction == NULL )
            {
            int col = pLayer->GetFieldCol( outcome.field );
            outcome.col = col;

            if ( col < 0 )
               {
               CString msg;
               msg.Format( "Unable to find column %s when setting up policy %s", 
                              outcome.field, (LPCTSTR) pPolicy->m_name );
               Report::ErrorMsg( msg );
               }
            }
         else
            {
            outcome.m_pFunction->m_pPolicy = pPolicy;
            bool ret = outcome.m_pFunction->Build();
      
            if ( ! ret )
               {
               CString msg;
               msg.Format( "Error compiling policy function for Policy %s.", (LPCTSTR) pPolicy->m_name );
               Report::ErrorMsg( msg );
               }
            }
         }
      }

   return true;
   }


void PolicyManager::BuildPolicySchedule()
   {
   m_policySchedule.RemoveAll();

   for ( int i=0; i < GetPolicyCount(); i++ )
      {
      Policy *pPolicy = GetPolicy( i );

      if (pPolicy->m_use && pPolicy->m_isScheduled)
         {
         POLICY_SCHEDULE ps(pPolicy->m_startDate, pPolicy);
         m_policySchedule.Add(ps);
         }
      }

   return;
   }


///////////////////////////////////////////////////////////////////////////////
// Functions
OutcomeFunction* OutcomeFunction::MakeFunction( LPCTSTR functionName, Policy *pPolicy )
   {
   CString name( functionName );
   
   if ( name.CompareNoCase( "Buffer" ) == 0 )
      return (OutcomeFunction*) new BufferOutcomeFunction( pPolicy );

   if ( name.CompareNoCase( "Expand" ) == 0 )
      return (OutcomeFunction*) new ExpandOutcomeFunction( pPolicy );

   if ( name.CompareNoCase( "AddPoint" ) == 0 )
      return (OutcomeFunction*) new AddPointOutcomeFunction( pPolicy );

   if ( name.CompareNoCase( "Increment" ) == 0 )
      return (OutcomeFunction*) new IncrementOutcomeFunction( pPolicy );

   return NULL;
   }

///////// BufferOutcomeFunction ////////////////////////////////
OutcomeFunction* BufferOutcomeFunction::Clone()
   {
   BufferOutcomeFunction *p = new BufferOutcomeFunction( m_pPolicy );

   p->m_distance = m_distance;
   p->m_pQuery   = m_pQuery;  // shallow copy, memory managed by the QueryEngine
   p->m_insideBufferMultiOutcomeArray = m_insideBufferMultiOutcomeArray;
   p->m_outsideBufferMultiOutcomeArray = m_outsideBufferMultiOutcomeArray;
   p->m_pArgs    = new CStringArray();
   p->m_pArgs->Copy( *m_pArgs );

   return p;
   }

bool BufferOutcomeFunction::Build()
   {
   // Entry syntax:
   //    Buffer( distance, query string, Buffer Outcome string, Other Outcome string )

   if ( m_pArgs == NULL || m_pArgs->GetCount() != 4 )
      {
      ASSERT(0);
      return false;
      }

   CString distString     = m_pArgs->GetAt(0);
   CString queryString    = m_pArgs->GetAt(1);
   CString bOutcomeString  = m_pArgs->GetAt(2);
   CString oOutcomeString  = m_pArgs->GetAt(3);

   m_distance = (float)atof( distString );

   if ( m_distance <=0 )
      {
      ASSERT(0);
      return false;
      }


   // Parse Query
   CString source( "Policy '" );
   source += m_pPolicy->m_name;
   source += "' Buffer Outcome Function";

   QueryEngine *pQE = this->m_pPolicy->m_pPolicyManager->m_pQueryEngine;

   m_pQuery = pQE->ParseQuery( queryString, 0, source);
   
   if ( m_pQuery == NULL )
      {
      ASSERT(0);
      return false;
      }
   
   // Parse Outcomes
   m_insideBufferMultiOutcomeArray.RemoveAll();
   m_outsideBufferMultiOutcomeArray.RemoveAll();

   pogpMultiOutcomeArray = &m_insideBufferMultiOutcomeArray;

   bOutcomeString.Trim();
   if ( ! bOutcomeString.IsEmpty() )
      {
      pogBuffer  = bOutcomeString;
      pogIndex   = 0;
      poPolicy   = m_pPolicy;

      int ret = poparse();
      ASSERT( ret == 0 );
      }
   else
      m_insideBufferMultiOutcomeArray.AddMultiOutcome( new MultiOutcomeInfo() );

   pogpMultiOutcomeArray = &m_outsideBufferMultiOutcomeArray;
   oOutcomeString.Trim();

   if ( ! oOutcomeString.IsEmpty() )
      {
      pogBuffer  = oOutcomeString;
      pogIndex   = 0;
      poPolicy   = m_pPolicy;

      int ret = poparse();
      ASSERT( ret == 0 );
      }
   else
      m_outsideBufferMultiOutcomeArray.AddMultiOutcome( new MultiOutcomeInfo() );

   ///////////////////////////////
   for ( int j=0; j < m_insideBufferMultiOutcomeArray.GetSize(); j++ )
      {
      MultiOutcomeInfo &m = m_insideBufferMultiOutcomeArray.GetMultiOutcome( j );
      for ( int k=0; k < m.GetOutcomeCount(); k++ )
         {
         OutcomeInfo &outcome = m.GetOutcome( k );

         if ( outcome.m_pFunction == NULL )
            {
            int col = this->m_pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer->GetFieldCol( outcome.field );
            outcome.col = col;
            
            if ( col < 0 )
               {
               CString msg;
               msg.Format( "Unable to find column %s when setting up buffer outcome", outcome.field );
               Report::ErrorMsg( msg );
               }
            }
         }
      }

   for ( int j=0; j < m_outsideBufferMultiOutcomeArray.GetSize(); j++ )
      {
      MultiOutcomeInfo &m = m_outsideBufferMultiOutcomeArray.GetMultiOutcome( j );
      for ( int k=0; k < m.GetOutcomeCount(); k++ )
         {
         OutcomeInfo &outcome = m.GetOutcome( k );

         if ( outcome.m_pFunction == NULL )
            {
            int col = this->m_pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer->GetFieldCol( outcome.field );
            outcome.col = col;
            
            if ( col < 0 )
               {
               CString msg;
               msg.Format( "Unable to find column %s when setting up buffer outcome", outcome.field );
               Report::ErrorMsg( msg );
               }
            }
         }
      }
   ///////////////////////////////

   return true;
   }

bool BufferOutcomeFunction::Run( DeltaArray *pDeltaArray, int cell, int currentYear, int persistence )
   {
   ASSERT(this->m_pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer != NULL );

   // if buffering has been disabled, treat like a normal policy application, applying the buffer outcome to
   // the entire original cell
   //if ( m_enabled == false )
   //   {
   //   EnvModel *pModel = m_pPolicy->m_pPolicyManager->m_pEnvModel;
   //   int outcome = pModel->SelectPolicyOutcome( m_insideBufferMultiOutcomeArray );
   //   if (outcome >= 0)
   //      {
   //      const MultiOutcomeInfo& bMultiOutcome = m_insideBufferMultiOutcomeArray.GetMultiOutcome( outcome );
   //      pModel->ApplyMultiOutcome( bMultiOutcome, cell, persistence );
   //      }
   //   return true;
   //   }

   //
   // buffering was enabled, so do the whole subdivision thing
   //
   Poly *pPoly = this->m_pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer->GetPolygon( cell );
   Vertex center = pPoly->GetCentroid();

   int *neighbors = NULL;
   int count = 0;
   int n = 0;

   do
      {
      if ( n > 0 )
         delete [] neighbors;
      n += 100;
      neighbors = new int[ n ];
      count = this->m_pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer->GetNearbyPolysFromIndex( pPoly, neighbors, NULL, n, 0 );
      } 
   while ( n <= count );

   if ( count < 0 )
      {
      ASSERT(0);
      delete [] neighbors;
      return false;
      }

   Poly bufferPoly;
   Poly bufferUnion;

   for ( int i=0; i < count; i++ )
      {
      bool result;
      if ( m_pQuery->Run( neighbors[i], result ) == false ) 
         {
         ASSERT(0);
         delete [] neighbors;
         return false;
         }

      if ( result )
         {
         Poly *pPoly = this->m_pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer->GetPolygon( neighbors[i] );
         ASSERT( pPoly );

         PolyBuffer::MakeBuffer( *pPoly, bufferPoly, m_distance );

         if ( bufferUnion.GetVertexCount() > 0 )
            {
            PolyClipper::PolygonClip( GPC_UNION, bufferUnion, bufferPoly, bufferUnion );
            }
         else
            {
            bufferUnion = bufferPoly;
            }
         }
      }

   delete [] neighbors;

   if ( bufferUnion.GetVertexCount() > 0 )
      {
      EnvModel *pModel = m_pPolicy->m_pPolicyManager->m_pEnvModel;

      Poly *pPoly = this->m_pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer->GetPolygon( cell );

      Poly *pInside  = new Poly;
      Poly *pOutside = new Poly;

      PolyClipper::PolygonClip( GPC_INT,  *pPoly, bufferUnion, *pInside );
      PolyClipper::PolygonClip( GPC_DIFF, *pPoly, bufferUnion, *pOutside );

      // TODO: Seem to get negative areas too much
      //       There may be a bug!!!!!!
      if ( pInside->GetVertexCount() == 0 || pInside->GetArea() <= 0 ) 
         {
         delete pInside;
         delete pOutside;

         return false;
         }
      if ( pOutside->GetVertexCount() == 0 || pOutside->GetArea() <= 0 )
         {
         delete pInside;
         delete pOutside;

         int outcome = pModel->SelectPolicyOutcome( m_insideBufferMultiOutcomeArray );
         if (outcome >= 0)
            {
            const MultiOutcomeInfo& bMultiOutcome = m_insideBufferMultiOutcomeArray.GetMultiOutcome( outcome );
            pModel->ApplyMultiOutcome( bMultiOutcome, cell, persistence );
            }
         return true;
         }

      PolyArray replacementList;
      replacementList.Add( pInside );
      replacementList.Add( pOutside );

      pDeltaArray->AddSubdivideDelta( currentYear, cell, replacementList );   //// TODO FIX THIS!!!!

      // Apply outcome to the Buffer Poly
      int outcome = pModel->SelectPolicyOutcome( m_insideBufferMultiOutcomeArray );
      if (outcome >= 0)
         {
         const MultiOutcomeInfo& bMultiOutcome = m_insideBufferMultiOutcomeArray.GetMultiOutcome( outcome );
         pModel->ApplyMultiOutcome( bMultiOutcome, cell, persistence );
         }

      // Apply outcome to the Other Poly
      outcome = pModel->SelectPolicyOutcome( m_outsideBufferMultiOutcomeArray );
      if (outcome >= 0)
         {
         const MultiOutcomeInfo& oMultiOutcome = m_outsideBufferMultiOutcomeArray.GetMultiOutcome( outcome );
         pModel->ApplyMultiOutcome( oMultiOutcome, cell, persistence );      
         }
      }
   else
      return false;

   return true;
   }


///////// ExpandOutcomeFunction ////////////////////////////////
OutcomeFunction* ExpandOutcomeFunction::Clone()
   {
   ExpandOutcomeFunction *p = new ExpandOutcomeFunction( m_pPolicy );

   p->m_area = m_area;
   p->m_pQuery   = m_pQuery;  // shallow copy, memory managed by the QueryEngine
   p->m_multiOutcomeArray = m_multiOutcomeArray;
   p->m_pArgs    = new CStringArray();
   p->m_pArgs->Copy( *m_pArgs );

   return p;
   }

bool ExpandOutcomeFunction::Build()
   {
   // Entry syntax:
   //    Expand( constraint query string, area, Outcome string )
   if ( m_pArgs == NULL || m_pArgs->GetCount() != 3 )
      {
      int count = 0;
      if ( m_pArgs != NULL )
         count = (int) m_pArgs->GetCount();

      CString msg;
      msg.Format( "Expand() policy outcome for policy %s:  Invalid argument count (3 are required, %i found)",
         (LPCTSTR) poPolicyName, count );

      Report::ErrorMsg( msg );

      for ( int i=0; i < count; i++ )
         {
         msg.Format( "   Expand argument %i: %s", i, (LPCTSTR) m_pArgs->GetAt( i ) );
         Report::ErrorMsg( msg );
         }

      return false;
      }

   CString queryString    = m_pArgs->GetAt(0);
   CString _areaString    = m_pArgs->GetAt(1);
   CString outcomeString  = m_pArgs->GetAt(2);
   CString areaString;
   
   // strip comments as needed
   bool inComment = false;
   for ( int i=0; i < _areaString.GetLength(); i++ )
      {
      if ( _areaString[ i ] == '{' )
         inComment = true;
      else if ( _areaString[ i ] == '}' )
         inComment = false;

      if ( ! inComment )
         areaString += _areaString[ i ];
      }

   // check for self reference
   if (queryString.Compare("@") == 0)
      queryString = this->m_pPolicy->m_siteAttrStr;

   m_area = (float) atof( areaString );

   if ( m_area <= 0 )
      {
      CString msg;
      msg.Format( "'Expand' Outcome function <query=%s> - Invalid 'Area' argument: %s",
         (LPCTSTR) queryString, (LPCTSTR) areaString );
      Report::ErrorMsg( msg );
      return false;
      }

   // Parse Query
   CString source( "Policy '" );
   source += m_pPolicy->m_name;
   source += "' Expand Outcome Function";

   QueryEngine *pQE = this->m_pPolicy->m_pPolicyManager->m_pQueryEngine;
   m_pQuery = pQE->ParseQuery( queryString, 0, source );
   
   if ( m_pQuery == NULL )
      {
      CString msg;
      msg.Format( "'Expand' Outcome function - Invalid 'Query' argument: %s",
         (LPCTSTR) queryString );
      Report::ErrorMsg( msg );
      return false;
      }
   
   // Parse Outcomes
   m_multiOutcomeArray.RemoveAll();

   pogpMultiOutcomeArray = &m_multiOutcomeArray;

   outcomeString.Trim();
   if ( ! outcomeString.IsEmpty() )
      {
      pogBuffer  = outcomeString;
      pogIndex   = 0;
      poPolicy   = m_pPolicy;

      int ret = poparse();
      ASSERT( ret == 0 );
      }
   else
      m_multiOutcomeArray.AddMultiOutcome( new MultiOutcomeInfo() );

   for ( int j=0; j < m_multiOutcomeArray.GetSize(); j++ )
      {
      MultiOutcomeInfo &m = m_multiOutcomeArray.GetMultiOutcome( j );
      for ( int k=0; k < m.GetOutcomeCount(); k++ )
         {
         OutcomeInfo &outcome = m.GetOutcome( k );

         if ( outcome.m_pFunction == NULL )
            {
            int col = this->m_pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer->GetFieldCol( outcome.field );
            outcome.col = col;
                        
            if ( col < 0 )
               {
               CString msg;
               msg.Format( "Unable to find column %s when setting up Expand() outcome", outcome.field );
               Report::ErrorMsg( msg );
               }
            }
         }
      }

   return true;
   }


bool ExpandOutcomeFunction::Run( DeltaArray *pDeltaArray, int idu, int currentYear, int persistence )
   {
   ASSERT(this->m_pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer != NULL );
   MapLayer *pLayer = this->m_pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer;

   if ( m_colArea < 0 )
      m_colArea = pLayer->GetFieldCol( _T( "AREA" ) );

   // basic idea - expand in concentric circles aroud the idu (subject to constraints)
   // until necessary area is found.  We do this keeping track of candidate expansion IDUs
   // in m_expansionArray, starting with the "nucleus" IDU and iteratively expanding outward.
   // for the expansion array, we track the index in the expansion array of location of
   // the lastExpandedIndex and the number of unprocessed IDUs that are expandable (poolSize).
   int neighbors[ 64 ];
   bool addToPool = false;
   INT_PTR lastExpandedIndex = 0;
   int poolSize = 1;       // number of unprocessed IDUs that are expandable

   // note: values in m_expansionArray are -(index+1)
   m_expansionIDUs.RemoveAll();
   m_expansionIDUs.Add( idu );  // the first one (nucleus) is added whether it passes the query or not
                                // note: this means the neclues IDU gets treated the same as any
                                // added IDU's passing the test, meaning the expand outcome gets applied
                                // to the nucleus as well as the neighbors.  However, the nucleus
                                // idu is NOT included in the "areaSoFar" calculation, and so does not
                                // contribute to the area count to the max expansion area
   float areaSoFar = 0;
    
   // We collect all the idus that are next to the idus already processed that we haven't seen already
   while( poolSize > 0 && areaSoFar < m_area )
      {
      INT_PTR countSoFar = m_expansionIDUs.GetSize();

      // iterate from the last expanded IDU on the list to the end of the current list,
      // adding any neighbor IDU's that satisfy the expansion criteria.
      for ( INT_PTR i=lastExpandedIndex; i < countSoFar; i++ )
         {
         int nextIDU = m_expansionIDUs[ i ];

         if ( nextIDU >= 0 )   // is this an expandable IDU?
            {
            Poly *pPoly = pLayer->GetPolygon( nextIDU );  // -1 );    // why -1?
            int count = pLayer->GetNearbyPolys( pPoly, neighbors, NULL, 64, 0 );

            for ( int i=0; i < count; i++ )
               {
               float area = AddIDU( neighbors[ i ], addToPool, areaSoFar, m_area );

               if ( addToPool )  // AddIDU() added on to the expansion array AND it is an expandable IDU?
                  {
                  poolSize++;
                  areaSoFar += area;
                  ASSERT( areaSoFar <= m_area );
                  // goto applyOutcome;
                  }
               }

            poolSize--;    // remove the "i"th idu just processed from the pool of unexpanded IDUs 
                           // (if it was in the pool to start with)
            }  // end of: if ( nextIDU > 0 ) 
         }  // end of: for ( i < countSoFar ) - iterated through last round of unexpanded IDUs

      lastExpandedIndex = countSoFar;
      }

//applyOutcome:
   // at this point, the expansion area has been determined (it's stored in the m_expansionIDUs array) 
   // - apply outcomes
   EnvModel *pModel = this->m_pPolicy->m_pPolicyManager->m_pEnvModel;

   for ( INT_PTR i=0; i < m_expansionIDUs.GetSize(); i++ ) 
      {
      int expIDU = m_expansionIDUs[ i ];
      if ( expIDU >= 0 )
         {
         int outcome = pModel->SelectPolicyOutcome( m_multiOutcomeArray );
         if (outcome >= 0)
            {
            const MultiOutcomeInfo& bMultiOutcome = m_multiOutcomeArray.GetMultiOutcome( outcome );
            pModel->ApplyMultiOutcome( bMultiOutcome, expIDU, persistence );
            }
         }
      }

   this->m_pPolicy->m_expansionArea += areaSoFar;  // note: areaSoFar only incloudes expansion area

   return true;
   }


float ExpandOutcomeFunction::AddIDU( int idu, bool &addToPool, float areaSoFar, float maxArea )
   {
   // have we seen this on already?
   for ( int i=0; i < m_expansionIDUs.GetSize(); i++ )
      {
      int expansionIndex = m_expansionIDUs[ i ];

      if ( ( expansionIndex < 0 && -(expansionIndex+1) == idu )    // seen but not an expandable IDU
        || ( expansionIndex >= 0 && expansionIndex == idu ) )      // seen and is an expandable IDU
         {
         addToPool = false;
         return 0;
         }
      }

   // so, we haven't seen this one before. Is the constraint satisified?
   addToPool = true;

   if ( m_pQuery )
      {
      bool result = false;
      bool ok = m_pQuery->Run( idu, result );
      if ( ! ok || ! result )
         addToPool = false;
      }

   // would this IDU cause the area limit to be exceeded?
   float area = 0;
   this->m_pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer->GetData( idu, m_colArea, area );
   if ( addToPool && ( ( areaSoFar + area ) > maxArea ) )
      addToPool = false;

   if ( addToPool )
      m_expansionIDUs.Add( idu/*+1*/ );
   else
      m_expansionIDUs.Add( -(idu+1) ); // Note: what gets added to the expansion array is -(index+1)
   
   return area;
   }


///////////////////////////////////////////////////////////////////////////////
// AddPoint Functions

OutcomeFunction* AddPointOutcomeFunction::Clone()
   {
   AddPointOutcomeFunction *p = new AddPointOutcomeFunction( m_pPolicy );

   p->m_x = m_x;
   p->m_y = m_y;
   p->m_layerName = m_layerName;

   p->m_pArgs = new CStringArray();
   p->m_pArgs->Copy( *m_pArgs );

   return p;
   }

bool AddPointOutcomeFunction::Build()
   {
   // Entry syntax:
   //    AddPoint( layername, x, y )
   
   if ( m_pArgs == NULL || m_pArgs->GetCount() != 3 )
      {
      ASSERT(0);
      return false;
      }

   m_layerName = m_pArgs->GetAt(0);

   Map *pMap = this->m_pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer->m_pMap;

   m_pLayer = pMap->GetLayer( m_layerName );

   if ( m_pLayer == NULL )
      {
      CString msg( "Unable to find layer " );
      msg += m_layerName;
      msg += " while building AddPoint() outcome function";
      Report::ErrorMsg( msg );
      return false;
      }

   if ( m_pLayer->GetType() != LT_POINT )
      {
      CString msg( "Map layer specified (" );
      msg += m_layerName;
      msg += ") is not a Point layer";
      Report::ErrorMsg( msg );
      return false;
      }

   CString x = m_pArgs->GetAt(1);
   CString y = m_pArgs->GetAt(2);

   m_x = (float) atof( x );
   m_y = (float) atof( y );
   
   return true;
   }

bool AddPointOutcomeFunction::Run( DeltaArray *pDeltaArray, int cell, int currentYear, int persistence )
   {
   if ( m_pLayer == NULL )
      return false;

   if ( m_pLayer->GetType() != LT_POINT )
      return false;

   m_pLayer->AddPoint( m_x, m_y );
   return true;
   }



// Increment Functions

OutcomeFunction* IncrementOutcomeFunction::Clone()
   {
   IncrementOutcomeFunction *p = new IncrementOutcomeFunction( m_pPolicy );
   p->m_pArgs = new CStringArray();
   p->m_pArgs->Copy( *m_pArgs );

   p->m_col = m_col;
   p->m_fieldName = m_fieldName;
   p->m_increment = m_increment;
   p->m_layerName = m_layerName;
   p->m_pLayer    = m_pLayer;

   return p;
   }

bool IncrementOutcomeFunction::Build()
   {
   // Entry syntax:
   //    Increment( fieldname, increment )
   
   if ( m_pArgs == NULL || m_pArgs->GetCount() != 2 )
      {
      ASSERT(0);
      return false;
      }

   m_fieldName = m_pArgs->GetAt(0);

   Map *pMap = this->m_pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer->m_pMap;

   m_pLayer = this->m_pPolicy->m_pPolicyManager->m_pEnvModel->m_pIDULayer; //// pMap->GetLayer( m_layerName );

   if ( m_pLayer == NULL )
      {
      CString msg( "Unable to find layer " );
      msg += m_layerName;
      msg += " while building Increment() outcome function";
      Report::ErrorMsg( msg );
      return false;
      }

   m_fieldName.Trim();

   m_col = m_pLayer->GetFieldCol( m_fieldName );

   if ( m_col < 0 )
      {
      CString msg( "Map layer missing field specified in Outcome Increment() function: [" );
      msg += m_fieldName;
      msg += "]";
      Report::ErrorMsg( msg );
      return false;
      }

   CString increment = m_pArgs->GetAt(1);

   m_increment = (float) atof( increment );
   
   return true;
   }


bool IncrementOutcomeFunction::Run( DeltaArray *pDeltaArray, int cell, int currentYear, int persistence )
   {
   if ( m_pLayer == NULL )
      return false;

   if ( m_col < 0 )
      return false;

   VData value;
   m_pLayer->GetData( cell, m_col, value );
   bool ok = true;

   if ( ::IsInteger( value.type ) )
      {
      int iValue = 0;
      ok = value.GetAsInt( iValue ); 
      if ( ok )
         value = iValue + (int) m_increment;
      }
   else
      {
      float fValue = 0;
      ok = value.GetAsFloat( fValue ); 
      if ( ok )
         value = fValue + m_increment;
      }

   if ( ! ok )
      return false;

   EnvModel *pModel = this->m_pPolicy->m_pPolicyManager->m_pEnvModel;
   if ( pModel->AddDelta( cell, m_col, currentYear, value, DT_POLICY ) < 0 )
      return false;  

   return true;
   }





















void Policy::GetSiteAttrFriendly( CString &str, int options )
   {
   Query *pQuery = this->m_pSiteAttrQuery;
   pQuery->GetConditionsText( str, options );
   }


void Policy::GetOutcomesFriendly( CString &str )
   {
   str.Empty();

   int multiCount = this->GetMultiOutcomeCount();

   str.Format( "<table width='400' border='1'cellpadding='3' cellspacing='0' rules='ROWS' FRAME='HSIDES'><tr><td colspan='3'><b>%i Possible Outcomes</b><td></tr>", multiCount );
   str += "<tr><td colspan='2'>Outcomes</td><td align='center'>Probability</td></tr>";
   for ( int i=0; i < multiCount; i++ )
      {
      const MultiOutcomeInfo &moi = GetMultiOutcome( i );
      int outcomeCount = moi.GetOutcomeCount();

      if ( outcomeCount == 0 )
         continue;

      CString outcomesStr;
      for ( int j=0; j < outcomeCount; j++ )
         {
         OutcomeInfo &oi = (OutcomeInfo&) moi.GetOutcome( j );
         CString outcomeStr;
         oi.ToString( outcomeStr, true );
         MAP_FIELD_INFO *pInfo = this->m_pPolicyManager->m_pEnvModel->m_pIDULayer->FindFieldInfo( oi.field );
         if ( pInfo != NULL )
            outcomeStr.Replace( oi.field, pInfo->label );

         if ( j != outcomeCount-1 )
            outcomeStr += "<br>and ";

         outcomesStr += outcomeStr;
         }

      CString multiHdr;
      multiHdr.Format( "<tr><td colspan='2' width='350'>%s</td><td align='center'>%f</td></tr>", (LPCTSTR) outcomesStr, moi.GetProbability() );
      str += multiHdr;
      }
   str += "</table>";
   }



void Policy::GetScoresFriendly( CString &str )
   {
   str.Empty();

   if ( m_objectiveArray.GetSize() == 0 )
      return;

   str += "<table width='400' border='1'cellpadding='3' cellspacing='0' rules='ROWS' FRAME='HSIDES'>";

   for ( int i=0; i < m_objectiveArray.GetSize(); i++ )
      {
      ObjectiveScores *pScores = m_objectiveArray[ i ];

      int gsCount  = (int) pScores->m_goalScoreArray.GetSize();
      int modCount = (int) pScores->m_modifierArray.GetSize();

      if ( gsCount == 0 && modCount == 0 )
         continue;

      str += "<tr><td colspan='2'><b>Objective: ";
      str += pScores->m_objectiveName;
      str += "</b></td><tr>";

      if ( gsCount > 0 )
         {
         CString scoreStr( "<tr><td align='center'><i>Score</i></td><td><i>Condition</i></td></tr>" );
         str += scoreStr;

         scoreStr.Empty();

         for ( int j=0; j < gsCount; j++ )
            {
            GOAL_SCORE &gs = pScores->m_goalScoreArray[ j ];

            CString query;
            if ( gs.pQuery )
               gs.pQuery->GetConditionsText( query, (ALL_OPTIONS & ~SHOW_LEADING_TAGS) );
            else
               query = "Applies Globally";

            scoreStr.Format( "<tr><td align='center'>%g</td><td>%s</td></tr>", gs.score, (LPCTSTR) query );
            str += scoreStr;
            }
         }


      if ( modCount > 0 )
         {
         CString modStr( "<tr><td align='center'><i>Modifiers</i></td><td><i><Conditions></i></td></tr>" );
         str += modStr;
         for ( int j=0; j < modCount; j++ )
            {
            SCORE_MODIFIER &sm = pScores->m_modifierArray[ j ];

            switch( sm.type )
               {
               case SM_SCENARIO:
                  {
                  Scenario *pScenario = m_pPolicyManager->m_pEnvModel->m_pScenarioManager->GetScenario( sm.scenario );
                  modStr.Format( "<tr><td align='center'>%g</td><td>Scenario: <i>%s</i></td>", sm.value, (LPCTSTR) pScenario->m_name );
                  }
                  break;

               case SM_INCR:
               case SM_DECR:
                  {
                  CString condition( "Applies Globally" );
                  if ( sm.pQuery != NULL )
                     sm.pQuery->GetConditionsText( condition, (ALL_OPTIONS & ~SHOW_LEADING_TAGS) );

                  modStr.Format( "<tr><td align='center'>%g</td><td>%s</td></tr>", (sm.type == SM_INCR ? sm.value : -sm.value ), (LPCTSTR) condition );
                  }
                  break;
               }
            str += modStr;
            }
         }
      }
   str += "</table>";
   }


int PolicyManager::AddPolicy( Policy *pPolicy )
   {
   pPolicy->m_index = (int) m_policyArray.Add( pPolicy );
   m_policyIDToIndexMap[ pPolicy->m_id ] = pPolicy->m_index; 
   return pPolicy->m_index; 
   }  // returns index of added item


// isImporting indicates that "filename" reference a policy xml file.

int PolicyManager::LoadXml( LPCTSTR _filename, bool isImporting, bool appendToExisting )
   {
   int count = 0;

   CString filename;
   if ( PathManager::FindPath( _filename, filename ) < 0 ) //  return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 
      {
      CString msg;
      msg.Format( "PolicyManager: Input file '%s' not found - no policies will be loaded", _filename );
      Report::ErrorMsg( msg );
      return false;
      }

   // have xml string, start parsing
   TiXmlDocument doc;
   bool ok = doc.LoadFile( filename );

   if ( ! ok )
      {
#ifndef NO_MFC
      CString msg;
      msg.Format("Error reading policy input file, %s", filename);
      AfxGetMainWnd()->MessageBox( doc.ErrorDesc(), msg );
#endif
      return 0;
      }

   // start interating through the nodes
   TiXmlElement *pXmlRoot = doc.RootElement();     // <policies>

   count = LoadXml( pXmlRoot, appendToExisting );

   if ( appendToExisting == false )
      {
      if ( isImporting )
         {
         m_loadStatus = 1;   // loaded from XML file
         m_importPath = filename;
         }
      else     // loading from ENVX file
         {
         m_loadStatus = 0;
         m_path = filename;
         }
      }

   return count;
   }


//-- PolicyManager::LoadXml() -----------------------------------------------
// 
// call from EnvDoc::OpenDocXml() is:
//   int policyCount = gpPolicyManager->LoadXml( pPolicies, false /* not appending*/ );
//
// if the envx <policy> tag has a 'file=' attribute, call the file-version of 
// LoadXml( path, isImporting=true, appendToExisting=false )
//---------------------------------------------------------------------------

int PolicyManager::LoadXml( TiXmlNode *pXmlPolicies, bool appendToExisting )
   {
   m_loadStatus = 0;
   bool reportError = true;

   if ( pXmlPolicies == NULL )
      return -1;
 
   // file specified?
   LPCTSTR file = pXmlPolicies->ToElement()->Attribute( _T("file") );
   
   if ( file != NULL && file[0] != NULL )
      return LoadXml( file, true, appendToExisting );
   
   // check for constraints
   // e.g.  <global_constraint name='Fuels Treatment Cost' type='resource_limit' value='1000000' />

   TiXmlElement *pXmlConstraint = pXmlPolicies->FirstChildElement( "global_constraint" );
   while ( pXmlConstraint != NULL )
      {
      LPCTSTR name  = NULL;
      LPCTSTR type  = NULL;
      LPCTSTR value = NULL;
      XML_ATTR attrs[] = { // attr          type        address    isReq checkCol
                         { "name",      TYPE_STRING,   &name,     true,  0 },
                         { "type",      TYPE_STRING,   &type,     true,  0 },
                         { "value",     TYPE_STRING,   &value,    true,  0 },
                         { NULL,        TYPE_NULL,     NULL,      false, 0 } };

      bool ok = TiXmlGetAttributes( pXmlConstraint, attrs, this->m_path );

      if ( ok )
         {
         // have constraint, add it to the policy manager
         CTYPE _type = CT_UNKNOWN;

         if ( lstrcmpi( type, "resource_limit" ) == 0 )
            _type = CT_RESOURCE_LIMIT;
         else if ( lstrcmpi( type, "max_area" ) == 0 )
            _type = CT_MAX_AREA;
         else if ( lstrcmpi( type, "max_count" ) == 0 )
            _type = CT_MAX_COUNT;

         if ( _type == CT_UNKNOWN )
            {
            CString msg;
            msg.Format( "Unrecognized 'type' attribute '%s' in <global_constraint> element - setting to 'resource_limit'", type );
            Report::LogWarning( msg );
            }

         //float _value = (float) atof( value );
         
         GlobalConstraint *pConstraint = new GlobalConstraint( this, name, _type, value );
         this->AddGlobalConstraint( pConstraint );
         }  // end of: if ( layer is ok )

      pXmlConstraint = pXmlConstraint->NextSiblingElement( "global_constraint" );
      }  // end of: while ( pXmlConstraint != NULL )
   
   TiXmlNode *pXmlPolicyNode = NULL;

   Policy policy( this );
   int nextPolicyID = -1;
   for ( int i=0; i < GetPolicyCount(); i++ )
      {
      Policy *pPolicy = GetPolicy( i );
      if ( pPolicy->m_id >= nextPolicyID )
         nextPolicyID = pPolicy->m_id;
      }

   // remember model use states and set flags to determine it policy/goal records found in DB
   //???Check
   //gpDoc->m_runEval = false;

   int metagoalCount = m_pEnvModel->GetMetagoalCount();
   //ASSERT( metagoalCount > 0 );
  
   TiXmlElement *pXmlPolicy = pXmlPolicies->FirstChildElement( "policy" );

   while( pXmlPolicy != NULL )
      {
      bool deleted = false;

      // get the classification
      CString _color;
      XML_ATTR attrs[] =
            { // attr        type          address                 isReq checkCol
            { "id",         TYPE_INT    , &policy.m_id,              true, 0 },
            { "name",       TYPE_CSTRING, &policy.m_name,            true, 0 },
            { "originator", TYPE_CSTRING, &policy.m_originator,      true, 0 },
            { "persistMin", TYPE_INT    , &policy.m_persistenceMin,  true, 0 },
            { "persistMax", TYPE_INT    , &policy.m_persistenceMax,  true, 0 },
            { "scheduled",  TYPE_BOOL   , &policy.m_isScheduled,     true, 0 },
            { "mandatory",  TYPE_BOOL   , &policy.m_mandatory,       true, 0 },
            { "compliance", TYPE_FLOAT  , &policy.m_compliance,      true, 0 },
            { "exclusive",  TYPE_BOOL   , &policy.m_exclusive,       true, 0 },
            { "startDate",  TYPE_INT    , &policy.m_startDate,       true, 0 },
            { "endDate",    TYPE_INT    , &policy.m_endDate,         true, 0 },
            { "use",        TYPE_BOOL   , &policy.m_use,             true, 0 },
            { "color",      TYPE_CSTRING, &_color,                   true, 0 },
            { "shared",     TYPE_BOOL   , &policy.m_isShared,        true, 0 },
            { "editable",   TYPE_BOOL   , &policy.m_isEditable,      true, 0 },
            { "deleted",    TYPE_BOOL   , &deleted,                  false, 0 },
            { "layer",      TYPE_CSTRING, &policy.m_mapLayer,        false, 0 },
            { NULL,         TYPE_NULL,    NULL,                      false, 0} };
          
      bool ok = TiXmlGetAttributes( pXmlPolicy, attrs, this->m_path );
      if ( ok )
         {
         CString msg( "Reading policy '" );
         msg += policy.m_name;
         msg += "' from file ";
         msg += this->m_path;
         Report::StatusMsg( msg );

         // process color
         TCHAR buffer[ 256 ];
         lstrcpy( buffer, _color );
         LPTSTR _red = buffer;
         LPTSTR _grn = _tcschr( buffer, ',' );
         LPTSTR _blu = NULL;
         int red=255, grn=255, blu=255;
         int color = 0;

         if ( _grn )
            {
            *_grn = NULL;
            _grn++;

            _blu = _tcschr( _grn, ',' );
            if ( _blu )
               {
               *_blu = NULL;
               _blu++;
               }
            }

         if ( _grn && _blu )    // RGB form defined?
            {
            int red = atoi( _red );
            int grn = atoi( _grn );
            int blu = atoi( _blu );
            color = RGB( red, grn, blu );
            }
         else
            color = atoi( buffer );

         policy.m_color = color;

         if ( ! policy.m_mapLayer.IsEmpty() )
            {
            policy.m_pMapLayer = m_pEnvModel->m_pIDULayer->m_pMap->GetLayer( (LPCTSTR) policy.m_mapLayer );

            if ( policy.m_pMapLayer == NULL )
               {
               CString msg;
               msg.Format( "Error reading policy '%s': layer ;%s' not found.  This policy will be disabled.", (LPCTSTR) policy.m_name, (LPCTSTR) policy.m_mapLayer );
               Report::ErrorMsg( msg );
               policy.m_use = false;
               }
            }

         // process siteAttr element
         TiXmlElement *pXmlSiteAttr = pXmlPolicy->FirstChildElement( "siteAttr" );
         if ( pXmlSiteAttr == NULL )
            {
            CString msg;
            msg.Format( "Error reading policy '%s': missing site attribute ( <siteAttr> query </siteAttr> )", (LPCTSTR) policy.m_name );
            Report::ErrorMsg( msg );
            policy.m_siteAttrStr = "";
            }
         else
            policy.m_siteAttrStr = pXmlSiteAttr->GetText();

         // outcomes next.  See if <outcome> tag (deprecated) exists - if so, use it.  Otherwise, look for <outcomes>
         // process outcome element
         bool usingNewOutcomeSpec = false;
         TiXmlElement *pXmlOutcome = pXmlPolicy->FirstChildElement( "outcome" );
         if ( pXmlOutcome != NULL )
            {
            policy.m_outcomeStr = pXmlOutcome->GetText();   // note: this is a full MultiOutcomeInfo spec
            policy.m_outcomeStr.Replace( _T("\n"), NULL );
            policy.m_outcomeStr.Replace( _T("\r"), NULL );
            policy.m_outcomeStr.Replace( _T("\t"), NULL );
            policy.m_outcomeStr.Trim();
            }
         else
            {
            usingNewOutcomeSpec = true;
            policy.ClearMultiOutcomeArray();

            TiXmlElement *pXmlOutcomes = pXmlPolicy->FirstChildElement( "outcomes" );
            if ( pXmlOutcomes != NULL )
               {
               TiXmlElement *pXmlOutcome = pXmlOutcomes->FirstChildElement( "outcome" );

               CString multiOutcomeArrayStr;

               while( pXmlOutcome != NULL )
                  {
                  float probability = -1.0f;
                  pXmlOutcome->SetValue( policy.m_name );
                  TiXmlGetAttr( pXmlOutcome, _T("probability"), probability, -1, false );
                  
                  // get outcome string
                  CString outcomeStr = pXmlOutcome->GetText();   // note: this is a full MultiOutcomeInfo spec
                  outcomeStr.Replace( _T("\n"), NULL );
                  outcomeStr.Replace( _T("\r"), NULL );
                  outcomeStr.Replace( _T("\t"), NULL );
                  outcomeStr.Trim();
                  
                  multiOutcomeArrayStr += outcomeStr;
                  multiOutcomeArrayStr += ";";

                  // parse outcome using temporary MOA
                  MultiOutcomeArray moa;
                  Policy::ParseOutcomes( &policy, outcomeStr, moa );
               
                  // allocate a multioutcome
                  int count = (int) moa.GetSize();
                  if ( count != 1 )
                     {
                     CString msg;
                     msg.Format( "Error reading policy '%s' <outcome> tag '%s' - only one multioutcome is allowed",
                        (LPCTSTR) policy.m_name, (LPCTSTR) outcomeStr );
                     Report::ErrorMsg( msg ); 
                     }

                  if ( count > 0 )
                     {  // Note: only one MultiOutcomeInfo in the moa array
                     MultiOutcomeInfo *pMOI = new MultiOutcomeInfo( moa.GetMultiOutcome( 0 ) );
                     policy.AddMultiOutcome( pMOI );

                     if ( probability >= 0 )
                        pMOI->probability = float(probability*100);

                     // process any constraints associated with this outcome
                     TiXmlElement *pXmlConstraint = pXmlOutcome->FirstChildElement( "constraint" );
                     while( pXmlConstraint != NULL )
                        {
                        OutcomeConstraint *pConstraint = LoadXmlConstraint( pXmlConstraint, &policy );

                        if ( pConstraint != NULL )
                           pMOI->AddConstraint( pConstraint );

                        pXmlConstraint = pXmlConstraint->NextSiblingElement( "constraint" );
                        }
                     }

                  pXmlOutcome = pXmlOutcome->NextSiblingElement( "outcome" );
                  }

               // strip terminal semi-colon
               multiOutcomeArrayStr.TrimRight( ';' );
               policy.m_outcomeStr = multiOutcomeArrayStr;
               }
            else
               {
               CString msg;
               msg.Format( "Error reading policy '%s': missing outcomes ( <outcomes> outcomes spec </outcomes> )", (LPCTSTR) policy.m_name );
               Report::ErrorMsg( msg );
               policy.m_outcomeStr = "";
               }
            }

         // process preferential selection element
         //pElement = pXmlPolicyElement->FirstChildElement( "preference" );
         //if( pElement != NULL )
         //   policy.m_prefSelStr = pElement->GetText();

         // process any constraints
         policy.RemoveAllConstraints();

         // e.g.   <constraint name='Fuels Treatment Cost' basis='unit_area' value='1000000'  />
         pXmlConstraint = pXmlPolicy->FirstChildElement( "constraint" );
         while ( pXmlConstraint != NULL )
            {
            PolicyConstraint *pConstraint = LoadXmlConstraint( pXmlConstraint, &policy );

            if ( pConstraint != NULL )
               policy.AddConstraint( pConstraint );

            pXmlConstraint = pXmlConstraint->NextSiblingElement( "constraint" );
            }  // end of: while ( pXmlConstraint != NULL )

         // process narrative element
         TiXmlElement *pXmlNarrative = pXmlPolicy->FirstChildElement( "narrative" );
         ASSERT( pXmlNarrative != NULL );
         policy.m_narrative = pXmlNarrative->GetText();

         if ( appendToExisting )
            policy.m_id = ++nextPolicyID;

         // make note of policy sequence
         if ( policy.m_id >= nextPolicyID )
            nextPolicyID = policy.m_id+1;

         // should we use the policy in this session?
         //if ( ! deleted && ( (policy.m_isShared && gpDoc->m_loadSharedPolicies) || ( policy.m_originator.CompareNoCase( gpDoc->m_userName ) == 0 ) ) )
         if ( ! deleted  ) // && ( (policy.m_isShared && gpDoc->m_loadSharedPolicies) || ( policy.m_originator.CompareNoCase( gpDoc->m_userName ) == 0 ) ) )
            {      
            // policy populated, add it
            policy.SetSiteAttrQuery( policy.m_siteAttrStr );

            if ( usingNewOutcomeSpec == false )    // new spec builds as things are being loaded
               policy.SetOutcomes( policy.m_outcomeStr );

            // have all variable, proceed to create a policy
            Policy *pPolicy = new Policy( policy );
            int index = AddPolicy( pPolicy );

            // add a goal (intention) score set for each model
            bool *metagoalFound = new bool[ metagoalCount ];
            //pPolicy->AddObjective( "Global", -1, -1 );  // always a global objective (added in constructor)
            for ( int j=0; j < metagoalCount; j++ )  
               {
               METAGOAL *pInfo = m_pEnvModel->GetMetagoalInfo( j );
               pPolicy->AddObjective( pInfo->name, j );
               metagoalFound[ j ] = false;
               }

            // process <scores> element     
            bool usesScores = true;
            TiXmlElement *pXmlGoalScores = pXmlPolicy->FirstChildElement( "goal_scores" );
            if ( pXmlGoalScores != NULL )
               {
               Report::LogWarning( _T("<goal_score> syntax no longer supported - ignoring...") );
               usesScores = false;
               }

            if ( usesScores )
               {
               TiXmlElement *pXmlScores = pXmlPolicy->FirstChildElement( "scores" );
               ASSERT( pXmlScores != NULL );

               // process scores
               TiXmlNode *pObjectiveNode = NULL;
               int j = 0;
               while ( pObjectiveNode = pXmlScores->IterateChildren( "objective", pObjectiveNode ) )
                  {
                  if ( pObjectiveNode->Type() != TiXmlNode::ELEMENT )
                     continue;

                  TiXmlElement *pObjective = pObjectiveNode->ToElement();
                  ASSERT( pObjective != NULL );
               
                  CString objName;
                  pObjective->SetValue( policy.m_name );
                  TiXmlGetAttr( pObjective, "name", objName, NULL, true );

                  // have objective name and ID, make sure they can be found in model list
                  if ( objName.CompareNoCase( _T("Global") ) != 0 )
                     {
                     METAGOAL *pInfo = m_pEnvModel->FindMetagoalInfo( objName );
                     //EnvEvaluator *pInfo = m_pEnvModel->FindEvaluatorInfo( objName );
                     if ( pInfo == NULL )
                        {
                        if ( reportError )
                           {
                           CString msg( "Objective [" );
                           msg += objName;
                           msg += "] was not found parsing policy ";
                           msg += pPolicy->m_name;
                           msg += ". This should be fixed before continuing...";
                           Report::ErrorMsg( msg );
                           reportError = false;
                           }
                        continue;
                        }

                     // register that we found this metagoal
                     int metagoalIndex = m_pEnvModel->FindMetagoalIndex( pInfo->name );
                     if ( metagoalIndex >= 0 )
                        metagoalFound[ metagoalIndex ] = true;
                     }
                  else     // objective is "global"
                     {

                     }

                  // successfully loaded <objective>, find corresponding ObjectiveScores
                  // and start filling it up.
                  ObjectiveScores *pObjScoreArray = pPolicy->FindObjective( objName );
                  ASSERT( pObjScoreArray != NULL );

                  // first look for <score> elements
                  TiXmlNode *pScoreNode = NULL;
                  while( pScoreNode = pObjectiveNode->IterateChildren( "score", pScoreNode ) )
                     {
                     TiXmlElement *pScore = pScoreNode->ToElement();
                  
                     float  value;
                     pScore->SetValue( policy.m_name );
                     TiXmlGetAttr( pScore, "value", value, 0, true );

                     CString query;
                     TiXmlGetAttr( pScore, "query", query, "", true );

                     pObjScoreArray->AddScore( value, query );
                     }

                  // second, look for <modifier> elements
                  TiXmlNode *pModifierNode = NULL;
                  while( pModifierNode = pObjectiveNode->IterateChildren( "modifier", pModifierNode ) )
                     {
                     TiXmlElement *pModifier = pModifierNode->ToElement();
                     pModifier->SetValue( policy.m_name );

                     int type;
                     TiXmlGetAttr( pModifier, "type", type, 0, true );

                     float value;
                     TiXmlGetAttr( pModifier, "value", value, 0, true );

                     CString condition;
                     TiXmlGetAttr( pModifier, "condition", condition, "", true );

                     int scenario;
                     TiXmlGetAttr( pModifier, "scenario", scenario, -1, true );

                     pObjScoreArray->AddModifier( (SM_TYPE) type, value, condition, scenario );
                     }
                  }  // end of: while ( pObjectiveNode )
               }  // end of: if ( useScores )

            // if we are missing a metagoal and have a corresponding model, then...???
            for ( int j=0; j < metagoalCount; j++ )
               {
               if ( metagoalFound[ j ] == false )
                  {
                  //???Check
                  //gpDoc->m_runEval = true;
                  }
               }

            delete [] metagoalFound;
            }  // end of: if ( useThisPolicy)
         }  // end of: if ( TiXmlGetAttrs() )

      pXmlPolicy = pXmlPolicy->NextSiblingElement( "policy" );
      }  // end of: while ( IterateChilren() )
   
   m_nextPolicyID = nextPolicyID;
   
   //gpDoc->UnSetChanged( CHANGED_POLICIES  );
   return GetPolicyCount();
   }


PolicyConstraint *PolicyManager::LoadXmlConstraint( TiXmlElement *pXmlConstraint, Policy *pPolicy )
   {
   PolicyConstraint *pConstraint = NULL;
   LPCTSTR name     = NULL;
   LPCTSTR gcName   = NULL;
   LPCTSTR basis    = NULL;
   LPCTSTR initCost = NULL;
   LPCTSTR query    = NULL;
   LPCTSTR lookup   = NULL;
   LPCTSTR valueStr = NULL;
   LPCTSTR maintenanceCost = NULL;
   LPCTSTR duration = NULL;

   XML_ATTR attrs[] = { // attr              type           address         isReq checkCol
                      { "name",             TYPE_STRING,   &name,            true,  0 },
                      { "gcName",           TYPE_STRING,   &gcName,          true,  0 },
                      { "basis",            TYPE_STRING,   &basis,           false, 0 },
                      { "lookup",           TYPE_STRING,   &lookup,          false, 0 },
                      { "initCost",         TYPE_STRING,   &initCost,        false, 0 },
                      { "maintenanceCost",  TYPE_STRING,   &maintenanceCost, false, 0 },
                      { "duration",         TYPE_STRING,   &duration,        false, 0 },
                      //{ "query",            TYPE_STRING,   &query,           false, 0 },
                      { NULL,               TYPE_NULL,     NULL,             false, 0 } };

   bool ok = TiXmlGetAttributes( pXmlConstraint, attrs, this->m_path );

   if ( ok )
      {
      // have constraint, add it to the policy manager
      CBASIS _basis = CB_UNIT_AREA;
      if ( basis != NULL && lstrcmpi( basis, "unit_area" ) == 0 )
         _basis = CB_UNIT_AREA;
      else if ( basis != NULL && lstrcmpi( basis, "absolute" ) == 0 )
         _basis = CB_ABSOLUTE;

      if ( _basis == CB_UNKNOWN && initCost != NULL )
         {
         CString msg;
         msg.Format( "Unrecognized 'basis' attribute '%s' reading policy - defaulting to 'unit_area'", basis );
         Report::InfoMsg( msg );
         }
 
      pConstraint = new PolicyConstraint( pPolicy, name, _basis, initCost, maintenanceCost, duration, lookup );
      pConstraint->m_pGlobalConstraint = this->FindGlobalConstraint( gcName );
      if ( pConstraint->m_pGlobalConstraint == NULL )
         {
         CString msg;
         msg.Format( "Policy Constraint error: A Global Constraint '%s' referenced by Policy '%s'was not found for Policy Constraint '%s'.  This constraint will be ignored",
            gcName, (LPCTSTR) pPolicy->m_name, name );
         Report::ErrorMsg( msg );
         delete pConstraint;
         pConstraint = NULL;
         }
      else
         {
         pConstraint->m_gcName = gcName;
         }
      }  // end of: if ( layer is ok )

   return pConstraint;
   }


//-- SaveXml( filename ) -------------------------------------------------------------
//
// this should only get called with an XML extension.  It save's just
// the policies, nothing else


int PolicyManager::SaveXml( LPCTSTR filename )
   {
   if ( m_includeInSave == false )
      return 0;

   //check to see if is xml
   LPCTSTR extension = _tcsrchr( filename, '.' );
   if ( extension && lstrcmpi( extension+1, "xml" ) != 0 )
      {
      CString msg( "Error saving policies - Unsupported File Extension (" );
      msg += filename;
      msg += ")";
      Report::ErrorMsg( msg );
      return 0;
      }

   // open the file and write contents
   FILE *fp;
   fopen_s( &fp, filename, "wt" );
   if ( fp == NULL )
      {
      LONG s_err = GetLastError();
      CString msg = "Failure to open " + CString(filename) + " for writing.  ";
      Report::SystemErrorMsg  ( s_err, msg );
      return -1;
      }

   // ready to save. Don't use file refs, since we are writing an XML file.
   //  File refs are only used when saving to an envx file.
   bool useFileRef = false; // ( m_loadStatus == 1 ? true : false );
   int count = SaveXml( fp, /*includeHeader=*/ true, useFileRef );
   fclose( fp );

   return count;
   }

//-----------------------------------------------------------------------
// PolicyManager::SaveXml() = FILE* version
//
//-----------------------------------------------------------------------

int PolicyManager::SaveXml( FILE *fp, bool includeHdr, bool useFileRef )
   {
   bool useXmlStrings = false;
   
   ASSERT( fp != NULL );

   if ( includeHdr )
      fputs( "<?xml version='1.0' encoding='utf-8' ?>\n\n", fp );

   if ( useFileRef ) 
      {
      if ( m_importPath.IsEmpty() )
         {
         //???Check next section
         //SaveXmlOptionsDlg dlg;
         //dlg.m_file = m_importPath;
         //dlg.DoModal();
         //
         //switch( dlg.m_saveType )
         //   {
         //   case 0:  // no save
         //      fprintf( fp, _T("<policies file='%s' />\n"), (LPCTSTR) m_importPath );
         //      break;
         //
         //   case 1:  // save to project
         //      return SaveXml( fp, /*includeHeader=*/ false, /*useFileRef=*/ false );
         //
         //   case 2:  // save externally
         //      {
         //      int loadStatus = m_loadStatus;
         //      m_loadStatus = 0;
         //      fprintf( fp, _T("<policies file='%s' />\n"), (LPCTSTR) dlg.m_file );
         //      SaveXml( dlg.m_file );
         //      m_loadStatus = loadStatus;
         //      }
         //   }
         }
      else
         {
         int loadStatus = m_loadStatus;
         m_loadStatus = 0;
         fprintf( fp, _T("<policies file='%s' />\n"), (LPCTSTR) m_importPath );
         SaveXml( m_importPath );      // saves as xml
         m_loadStatus = loadStatus;
         }

      return GetPolicyCount();
      }  // end of: useFileRef = true;

   // below is useFileRef = false case
   fputs( "<policies>\n", fp );

   // first, global_constraints
   int constraintCount = this->GetGlobalConstraintCount();

   for ( int i=0; i < constraintCount; i++ )
      {
      GlobalConstraint *pConstraint = this->GetGlobalConstraint( i );
      LPCTSTR type = pConstraint->GetTypeString( pConstraint->m_type );
      fprintf( fp, "\n<global_constraint name='%s' type='%s' value='%s' />",
            (LPCTSTR) pConstraint->m_name, type, pConstraint->GetExpression() );
      }

   fputs( "\n", fp );
   
   for ( int i=0; i < GetPolicyCount(); i++ )
      {
      Policy *pPolicy = GetPolicy( i );
      
      fprintf( fp, "\n\t<policy \n\t\tid='%i' \n\t\tname='%s' \n\t\toriginator='%s' \n\t\tpersistMin='%i' \n\t\tpersistMax='%i' \n\t\tscheduled='%i' \n\t\tmandatory='%i' \n\t\tcompliance='%g' \n\t\texclusive='%i' \n\t\tstartDate='%i' \n\t\tendDate='%i'",
         pPolicy->m_id, (LPCTSTR) pPolicy->m_name, (LPCTSTR) pPolicy->m_originator, pPolicy->m_persistenceMin, pPolicy->m_persistenceMax, pPolicy->m_isScheduled ? 1 : 0, pPolicy->m_mandatory ? 1 : 0,
         pPolicy->m_compliance, pPolicy->m_exclusive ? 1 : 0, pPolicy->m_startDate, pPolicy->m_endDate );

      int red = GetRValue(pPolicy->m_color);
      int grn = GetGValue(pPolicy->m_color);
      int blu = GetBValue(pPolicy->m_color);

      fprintf( fp, "\n\t\tuse='%i' \n\t\tcolor='%i,%i,%i' \n\t\tshared='%i' \n\t\teditable='%i' >", 
         pPolicy->m_use ? 1 : 0, red, grn, blu, pPolicy->m_isShared ? 1 : 0, pPolicy->m_isEditable ? 1 : 0 );
      
      // next, constraints
      int constraintCount = pPolicy->GetConstraintCount();
      for ( int j=0; j < constraintCount; j++ )
         {
         PolicyConstraint *pConstraint = pPolicy->GetConstraint( j );
         LPCTSTR basis = pConstraint->GetBasisString( pConstraint->m_basis );

         if ( pConstraint->GetLookupTable() == NULL )
            {
            fprintf( fp, "\n\t\t<constraint name='%s' gcName='%s' basis='%s' initCost='%s' maintenanceCost='%s' duration='%s' />",
               (LPCTSTR) pConstraint->m_name, (LPCTSTR) pConstraint->m_gcName, basis, (LPCTSTR) pConstraint->m_initialCostExpr, (LPCTSTR) pConstraint->m_maintenanceCostExpr, (LPCTSTR) pConstraint->m_durationExpr );
            }
         else
            {
            fprintf( fp, "\n\t\t<constraint name='%s' gcName='%s' basis='%s' lookup='%s' initCost='%s' maintenanceCost='%s' duration='%s' />",
               (LPCTSTR) pConstraint->m_name, (LPCTSTR) pConstraint->m_gcName, basis, (LPCTSTR) pConstraint->m_lookupStr, (LPCTSTR) pConstraint->m_initialCostExpr, (LPCTSTR) pConstraint->m_maintenanceCostExpr, (LPCTSTR) pConstraint->m_durationExpr );
            }
         }

      // next, site attributes
      CString siteAttr;
      if ( useXmlStrings )
         GetXmlStr( pPolicy->m_siteAttrStr, siteAttr );
      else
         siteAttr = pPolicy->m_siteAttrStr;
      siteAttr.Replace( _T( "'"), _T("\"" ) );
      fprintf( fp, "\n\t\t<siteAttr>%s</siteAttr>", (LPCTSTR) siteAttr );

      // Next, multioutcome info
      fputs( "\n\t\t<outcomes>\n", fp );

      for ( int j=0; j < pPolicy->GetMultiOutcomeCount(); j++ )
         {
         MultiOutcomeInfo &moi = (MultiOutcomeInfo&) pPolicy->GetMultiOutcome( j );
         
         // write each outcome
         float probability = moi.GetProbability()/100.0f;
         fprintf( fp, "\t\t\t<outcome probability='%0.3f' >\n", probability );

         CString moiStr, _moiStr;
         moi.ToString( _moiStr, false );  // don't include outcome prob here, it is alread covered above
         
         if ( useXmlStrings )
            GetXmlStr( _moiStr, moiStr );
         else
            moiStr = _moiStr;

         moiStr.Replace( _T("\n"), NULL );   // terminate string properly
         moiStr.Replace( _T("\r"), NULL );
         moiStr.Replace( _T("\t"), NULL );
         moiStr.Replace( _T( "'"), _T("\"" ) );   // ' --> "   (single to double quote)
         moiStr.Replace( _T(";"), NULL );         // semicolon --> nothing
         moiStr += "\n";
         fputs( moiStr, fp );

         // any constraints?
         for ( int k=0; k < moi.GetConstraintCount(); k++ )
            {
            OutcomeConstraint *pConstraint = moi.GetConstraint( k );

            LPCTSTR basis = pConstraint->GetBasisString( pConstraint->m_basis );

            if ( pConstraint->GetLookupTable() == NULL )
               {
               fprintf( fp, "\n\t\t<constraint name='%s' gcName='%s' basis='%s' initCost='%s' maintenanceCost='%s' duration='%s' />",
                  (LPCTSTR) pConstraint->m_name, (LPCTSTR) pConstraint->m_gcName, basis, (LPCTSTR) pConstraint->m_initialCostExpr, (LPCTSTR) pConstraint->m_maintenanceCostExpr, (LPCTSTR) pConstraint->m_durationExpr );
               }
            else
               {
               fprintf( fp, "\n\t\t<constraint name='%s' gcName='%s' basis='%s' lookup='%s' initCost='%s' maintenanceCost='%s' duration='%s' />",
                  (LPCTSTR) pConstraint->m_name, (LPCTSTR) pConstraint->m_gcName, basis, (LPCTSTR) pConstraint->m_lookupStr, (LPCTSTR) pConstraint->m_initialCostExpr, (LPCTSTR) pConstraint->m_maintenanceCostExpr, (LPCTSTR) pConstraint->m_durationExpr );
               }
            }

         fputs( "\t\t\t</outcome>\n", fp );         
         }

      fputs( "\t\t</outcomes>\n", fp );

      CString narrative;
      GetXmlStr( pPolicy->m_narrative, narrative );
      fprintf( fp, "\n\t\t<narrative>%s</narrative>", (LPCTSTR) narrative );

      // objective scores
      fputs( "\n\t\t<scores>", fp );

      for ( int i=0; i < pPolicy->m_objectiveArray.GetSize(); i++ )
         {
         ObjectiveScores *pArray = pPolicy->m_objectiveArray.GetAt( i );

         if ( pArray->m_goalScoreArray.GetSize() > 0 || pArray->m_modifierArray.GetSize() > 0 )
            {
            fprintf( fp, "\n\t\t\t<objective name='%s' >", (LPCTSTR) pArray->m_objectiveName );

            // first, scores
            GoalScoreArray &gsa = pArray->m_goalScoreArray;
            for ( int j=0; j < gsa.GetSize(); j++ )
               {
               GOAL_SCORE &gs = gsa.GetAt( j );

               CString query;
               GetXmlStr( gs.queryStr, query );

               fprintf( fp, "\n\t\t\t\t<score value='%g' query='%s' />", gs.score, (LPCTSTR) gs.queryStr );
               }

            // next, modifers
            ModifierArray &ma = pArray->m_modifierArray;
            for ( int j=0; j < ma.GetSize(); j++ )
               {
               SCORE_MODIFIER &sm = ma.GetAt( j );

               CString condition;
               GetXmlStr( sm.condition, condition );

               fprintf( fp, "\n\t\t\t\t<modifier type='%i' value='%g' condition='%s' scenario='%i' />", 
                     (int) sm.type, sm.value, (LPCTSTR) condition, sm.scenario );
               }

            fputs( "\n\t\t\t</objective>", fp );
            }
         }

      fputs( "\n\t\t</scores>", fp );
      fputs( "\n\t</policy>\n", fp );
      }

   fputs( "\n</policies>\n", fp );

   return 1;
   }


GlobalConstraint *PolicyManager::FindGlobalConstraint( LPCTSTR name )
   {
   int count = (int) m_constraintArray.GetSize();
   for ( int i=0; i < count; i++ )
      {
      GlobalConstraint *pConstraint = m_constraintArray[ i ];

      if ( pConstraint->m_name.CompareNoCase( name ) == 0 )
         return pConstraint;
      }

   return NULL;
   }


bool PolicyManager::ReplaceGlobalConstraints( PtrArray< GlobalConstraint > &newConstraintArray )
   {
   bool allFound = true;

   RemoveAllGlobalConstraints();

   for ( int i=0; i < (int) newConstraintArray.GetSize(); i++ )
      AddGlobalConstraint( new GlobalConstraint( *(newConstraintArray[i]) ) );

   for ( int i=0; i < this->GetPolicyCount(); i++ )
      {
      Policy *pPolicy = GetPolicy( i );

      // any policy constraints?
      for ( int j=0; j < pPolicy->GetConstraintCount(); j++ )
         {
         PolicyConstraint *pConstraint = pPolicy->GetConstraint( j );
         
         pConstraint->m_pGlobalConstraint = NULL;
         
         for ( int k=0; k < (int) newConstraintArray.GetSize(); k++ )
            {
            if ( m_constraintArray[ k ]->m_name.CompareNoCase( pConstraint->m_gcName ) == 0 )  // match found?
               {
               pConstraint->m_pGlobalConstraint = m_constraintArray[ k ];
               break;
               }
            }

         // no global constraint found for this PolicyConstraint, so remove it from the policy
         if ( pConstraint->m_pGlobalConstraint == NULL )
            {
            pPolicy->DeleteConstraint( j );
            j--;
            CString msg;
            msg.Format( "No corresponding global constraint found when fixing up Policy Constraint '%s' in Policy '%s'.  It will be deleted.",
               (LPCTSTR) pConstraint->m_name, (LPCTSTR) pPolicy->m_name );
            Report::LogWarning( msg );
            allFound = false;
            }
         }

      // any outcome constraints?
      for ( int j=0; j < pPolicy->GetMultiOutcomeCount(); j++ )
         {
         MultiOutcomeInfo &outcome = (MultiOutcomeInfo&) pPolicy->GetMultiOutcome( j );

         for ( int m=0; m < outcome.GetConstraintCount(); m++ )
            {
            OutcomeConstraint *pConstraint = outcome.GetConstraint( m );

            pConstraint->m_pGlobalConstraint = NULL;
         
            for ( int k=0; k < (int) m_constraintArray.GetSize(); k++ )
               {
               if ( m_constraintArray[ k ]->m_name.CompareNoCase( pConstraint->m_gcName ) == 0 )  // match found?
                  {
                  pConstraint->m_pGlobalConstraint = m_constraintArray[ k ];
                  break;
                  }
               }

            // no global constraint found for this PolicyConstraint, so remove it from the policy
            if ( pConstraint->m_pGlobalConstraint == NULL )
               {
               outcome.DeleteConstraint( m );
               m--;
               CString msg;
               msg.Format( "No corresponding global constraint found when fixing up Outcome Constraint '%s' in Policy '%s'.  It will be deleted.",
                  (LPCTSTR) pConstraint->m_name, (LPCTSTR) pPolicy->m_name );
               Report::LogWarning( msg );
               allFound = false;
               }
            }  // end of: for ( m < outcome.GetcConstraintCount() )
         }  // end of: for ( j < pPolicy->GetMultiOutcomeCount() )
      }  // end of: for ( i < policyCount )

   // need to store pointers to new constraints
   return allFound;
   }
