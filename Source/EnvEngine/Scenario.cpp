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
#include "stdafx.h"
#pragma hdrstop

#include "Scenario.h"

#include "EnvModel.h"
#include "Policy.h"

#include <tinyxml.h>
#include <PathManager.h>


//extern PolicyManager *gpPolicyManager;

bool NormalizeWeights( float &w0, float &w1, float &w2 )
   {
   float sum = w0 + w1 + w2;

   if ( sum > 0 )
      {
      w0 = w0 / sum;
      w1 = w1 / sum;
      w2 = 1.0f - w0 - w1;
      return true;
      }

   return false;
   }


bool NormalizeWeights( float &w0, float &w1, float &w2, float &w3 )
   {
   float sum = w0 + w1 + w2 + w3;
   
   if ( sum > 0 )
      {
      w0 = w0 / sum;
      w1 = w1 / sum;
      w2 = w2 / sum;
      w3 = 1.0f - w0 - w1 - w2;
      return true;
      }

   return false;
   }


bool NormalizeWeights( float &w0, float &w1, float &w2, float &w3, float &w4 )
   {
   float sum = w0 + w1 + w2 + w3 + w4;
   
   if ( sum > 0 )
      {
      w0 = w0 / sum;
      w1 = w1 / sum;
      w2 = w2 / sum;
      w3 = w3 / sum;
      w4 = 1.0f - w0 - w1 - w2 - w3;
      return true;
      }

   return false;
   }



SCENARIO_VAR::SCENARIO_VAR( AppVar *pAppVar )
: name( pAppVar->m_name )
, vtype( V_APPVAR )
, pVar( pAppVar )
, type( TYPE_NULL )   // always a PTR type
, defaultValue( pAppVar->GetValue() )
, paramLocation( 0 )
, paramScale( 0 )
, paramShape( 0 )
, distType( MD_CONSTANT )
, pRand( NULL )
, inUse( true )
, pModelVar(NULL)
   {
   VData &v = pAppVar->GetValue();
   type = v.type;
   }



Scenario::Scenario( ScenarioManager *pSM, LPCTSTR name ) 
: m_pScenarioManager(pSM),
  m_name( name ),
  m_isShared( true ),
  m_isEditable( true ),
  m_isDefault( false ),
  //m_actorAltruismWt( 0.33f ),
  //m_actorSelfInterestWt( 0.33f ),
  m_policyPrefWt( 0.5f ),
  m_decisionElements( pSM->m_pEnvModel->m_decisionElements ),
  m_evalModelFreq( 1 ),
  m_runCount( 0 )
   {
   // construct a default scenario

   //===================== M O D E L  M E T A G O A L   S E T T I N G S  =====================
   int metagoalCount = this->m_pScenarioManager->m_pEnvModel->GetMetagoalCount();
	for( int i=0; i < metagoalCount; i++ )
	   {
      METAGOAL *pGoal = this->m_pScenarioManager->m_pEnvModel->GetMetagoalInfo( i );
      //EnvEvaluator *pModel = pGoal->pEvaluator;

      //if ( pModel != NULL )
      //   {
      SCENARIO_VAR sv(pGoal->name, V_META, (void*)&(pGoal->weight), TYPE_FLOAT, NULL, VData(0.0f), VData(0.0f), VData(0.0f), true,
         "Metagoal weighting [-3, +3] for this model - determines relative priorities for altruistic actors based on scarcity");
      m_scenarioVarArray.Add(sv);
      //   }
      }

   //=========================== M O D E L / P R O C E S S  U S A G E ========================
   int modelCount = this->m_pScenarioManager->m_pEnvModel->GetEvaluatorCount();
   //
   //if ( modelCount > 0 )
   //   {
   //   for ( int i=0; i < modelCount; i++ )
   //      {
   //      EnvEvaluator *pInfo = EnvModel::GetEvaluatorInfo( i );
   //      m_scenarioVarArray.Add( SCENARIO_VAR( CString( pInfo->m_name+".InUse"), V_MODEL, (void*) &(pInfo->m_use), TYPE_BOOL, NULL, VData( 0.0f ), VData(0.0f), VData(0.0f), false,
   //      "1=enable model for this scenario, 0=disable model for this scenario" ) );
   //      }
   //   }
   //
   int apCount = this->m_pScenarioManager->m_pEnvModel->GetModelProcessCount();
   //
   //if ( apCount > 0 )
   //   {
   //   for ( int i=0; i < apCount; i++ )
   //      {
   //      EnvModelProcess *pInfo = this->m_pScenarioManager->m_pEnvModel->GetModelProcessInfo( i );
   //      m_scenarioVarArray.Add( SCENARIO_VAR( CString( pInfo->m_name+".InUse"), V_AP, (void*) &(pInfo->m_use), TYPE_BOOL, NULL, VData( 0.0f ), VData(0.0f), VData(0.0f), false,
   //      "1=enable process for this scenario, 0=disable process for this scenario" ) );
   //      }
   //   }

   //================================ M O D E L   V A R I A B L E S ==========================
   //int modelCount = this->m_pScenarioManager->m_pEnvModel->GetEvaluatorCount();
   for( int i=0; i < modelCount; i++ )
	   {
      EnvEvaluator *pModel = this->m_pScenarioManager->m_pEnvModel->GetEvaluatorInfo( i );
      if ( pModel->m_use )
         {
         MODEL_VAR *modelVarArray = NULL;
         int varCount = pModel->InputVar( pModel->m_id, &modelVarArray );

         // for each variable exposed by the model, add it to the possible scenario variables
         for ( int j=0; j < varCount; j++ )
            {
            MODEL_VAR &mv = modelVarArray[ j ];
            if ( mv.pVar != NULL )
               {
               SCENARIO_VAR vi( V_MODEL, mv, pModel->m_name );    // set vi.pModelVar = &mv
               m_scenarioVarArray.Add( vi );
               }
            }
         }
      }

   //================= A U T O N O U S  P R O C E S S   V A R I A B L E S ==================
   //int apCount = this->m_pScenarioManager->m_pEnvModel->GetModelProcessCount();
   for( int i=0; i < apCount; i++ )
	   {
      EnvModelProcess *pAP = this->m_pScenarioManager->m_pEnvModel->GetModelProcessInfo( i );
      if ( pAP->m_use )
         {
         MODEL_VAR *modelVarArray = NULL;
         int varCount = pAP->InputVar( pAP->m_id, &modelVarArray );

         // for each variable exposed by the model, add it to the possible scenario variables
         for ( int j=0; j < varCount; j++ )
            {
            MODEL_VAR &mv = modelVarArray[ j ];
            if (mv.pVar != NULL)
               {
               SCENARIO_VAR sv(V_AP, mv, pAP->m_name);
               m_scenarioVarArray.Add(sv);
               }
            }
         }
      }

   //================= A P P L I C A T I O N   V A R I A B L E S ==================
   EnvModel *pModel = this->m_pScenarioManager->m_pEnvModel;

   int appVarCount = pModel->GetAppVarCount( AVT_APP );
   for( int i=0; i < appVarCount; i++ )
	   {
      AppVar *pAppVar = pModel->GetAppVar( i );   // note: type AVT_APP will always be at the head of the list
      SCENARIO_VAR sv(pAppVar);
      m_scenarioVarArray.Add(sv);
      }

   //================= D E C I S I O N   E L E M E N T S ==========================

   SCENARIO_VAR sv("Decision Elements", V_SYSTEM, (void*)&m_decisionElements,
      TYPE_INT, NULL, VData(pModel->m_decisionElements), VData(), VData(), false,
      "Decision Elements to use. This should be the sum of the desired elements: 0=none, 1=Actor Value Alignment, 2=Landscape Feedback, 4=Global Policy Preferences, 8=Utility, 16=Social Network");
   m_scenarioVarArray.Add(sv);

   //=================================== P O L I C I E S =====================================
   // NOTE!!! Policies should always come last
   int policyCount = pSM->m_pEnvModel->m_pPolicyManager->GetPolicyCount();
   for( int i=0; i < policyCount; i++ )
	   {
      Policy *pPolicy = pSM->m_pEnvModel->m_pPolicyManager->GetPolicy( i );
      m_policyInfoArray.AddPolicyInfo( pPolicy, pPolicy->m_use );
      }   

   //Initialize();     // copy any needed info into the scenario
   }

Scenario::Scenario(Scenario &s )
   : m_pScenarioManager( s.m_pScenarioManager )
   , m_name( s.m_name )
   , m_description( s.m_description )
   , m_originator( s.m_originator )
   , m_isEditable( s.m_isEditable )
   , m_isShared( s.m_isShared )
   , m_isDefault( s.m_isDefault )
   , m_policyPrefWt( s.m_policyPrefWt )
   , m_decisionElements( s.m_decisionElements )
   , m_evalModelFreq( s.m_evalModelFreq )
   , m_runCount( s.m_runCount )
   , m_scenarioVarArray( s.m_scenarioVarArray )
   , m_policyInfoArray( s.m_policyInfoArray )

   { }


Scenario::~Scenario(void)
   { }


int Scenario::GetScenarioVarCount( int type /*=V_ALL*/, bool inUseOnly /*=false*/ )
   {
   int count = 0;

   for ( int i=0; i < m_scenarioVarArray.GetSize(); i++ )
      {
      SCENARIO_VAR &vi = this->GetScenarioVar( i );

      if ( int( vi.vtype ) & type )
         {
         if ( !inUseOnly || vi.inUse )
            ++count;
         }
      }

   return count;
   }


SCENARIO_VAR *Scenario::GetScenarioVar( LPCTSTR name )
   {
   int count = (int) m_scenarioVarArray.GetSize();

   for ( int i=0; i < count; i++ )
      {
      SCENARIO_VAR *pInfo = &m_scenarioVarArray[ i ];

      if ( pInfo->name.Compare( name ) == 0 )
         return pInfo;
      }
   
   return NULL;
   }

 
SCENARIO_VAR *Scenario::FindScenarioVar( void* ptr )
   {
   int count = (int) m_scenarioVarArray.GetSize();

   for ( int i=0; i < count; i++ )
      {
      SCENARIO_VAR *pInfo = &m_scenarioVarArray[ i ];

      if ( pInfo->pVar = ptr )
         return pInfo;
      }
   
   return NULL;
   }


int Scenario::GetPolicyCount( bool inUseOnly /*=false*/ )
   {
   int count = 0;

   for ( int i=0; i < m_policyInfoArray.GetSize(); i++ )
      {
      POLICY_INFO &pi = this->GetPolicyInfo( i );
 
      if ( !inUseOnly || pi.inUse )
         ++count;
      }

   return count;
   }


POLICY_INFO *Scenario::GetPolicyInfo( LPCTSTR name )
   {
   int count = (int) m_policyInfoArray.GetSize();

   for ( int i=0; i < count; i++ )
      {
      POLICY_INFO &pi = m_policyInfoArray[ i ];

      if ( pi.policyName.Compare( name ) == 0 )
         return &m_policyInfoArray[ i ];
      }
   
   return NULL;
   }


POLICY_INFO *Scenario::GetPolicyInfoFromID( int id )
   {
   int count = (int) m_policyInfoArray.GetSize();

   for ( int i=0; i < count; i++ )
      {
      POLICY_INFO &pi = m_policyInfoArray[ i ];

      if ( pi.policyID == id )
         return &m_policyInfoArray[ i ];
      }
   
   return NULL;
   }


void Scenario::RemovePolicyInfo( Policy *pPolicy )
   {
   for ( int i=0; i < GetPolicyCount(); i++ )
      {
      if ( this->m_policyInfoArray[ i ].pPolicy == pPolicy )
         {
         m_policyInfoArray.RemoveAt( i );
         return;
         }
      }
   }


// Initialize() initializes all scenario variables.
void Scenario::Initialize()
   {
   for ( int i=0; i < m_scenarioVarArray.GetSize(); i++ )
      {
      SCENARIO_VAR &vi = m_scenarioVarArray[ i ];

      switch( vi.vtype )
         {
         case V_META:
         case V_MODEL:
         case V_AP:
            {
            if ( vi.pRand )
               {
               delete vi.pRand;
               vi.pRand = NULL;
               }

            switch( vi.distType )
               {
               case MD_CONSTANT:
                  break;                
                  
               case MD_UNIFORM:
                  {
                  double location, scale;
                  vi.paramLocation.GetAsDouble( location );
                  vi.paramScale.GetAsDouble( scale );
                  vi.pRand = new RandUniform( location, location+scale, 0 );
                  }
                  break;                  
                  
               case MD_NORMAL:
                  {
                  double location, scale;
                  vi.paramLocation.GetAsDouble( location );
                  vi.paramScale.GetAsDouble( scale );
                  vi.pRand = new RandNormal( location, scale, 0 );
                  }
                  break;

               case MD_WEIBULL:
                  {
                  double location, scale, shape;
                  vi.paramLocation.GetAsDouble( location );
                  vi.paramScale.GetAsDouble( scale );
                  vi.paramShape.GetAsDouble( shape );
                  vi.pRand = new RandWeibull( location, scale, 0 );
                  }
                  break;
                  
               case MD_LOGNORMAL:
                  {
                  double location, scale, shape;
                  vi.paramLocation.GetAsDouble( location );
                  vi.paramScale.GetAsDouble( scale );
                  vi.paramShape.GetAsDouble( shape );
                  vi.pRand = new RandLogNormal( location, scale, 0 );
                  }
                  break;
               }
            }
            break;

         case V_APPVAR:
            {
            AppVar *pAppVar = (AppVar*) vi.pVar;
            if ( pAppVar->IsGlobal() )
               pAppVar->Evaluate();   // evaluate the AppVar expr and store result
            }
            break;

         case V_SYSTEM:
            break; // nothing required

         default: ASSERT( 0 );
         }
      }
   }


int Scenario::SetScenarioVars( int runFlag )
   {
   // runFlag:  0 = set scenario variables, no randomization (uses paramLocation)
   //           1 = set scenario variable, using randomization if variable is defined as random
   //          -1 = set scenario variable EXCEPT any whose "useInSensitivity" flags are 
   //               set to -1, no randomization


   // Envision variables:
   //gpDoc->m_model.m_altruismWt = m_actorAltruismWt;
   //gpDoc->m_model.m_selfInterestWt = m_actorSelfInterestWt;
   EnvModel *pModel = m_pScenarioManager->m_pEnvModel;
   pModel->m_policyPrefWt = m_policyPrefWt;
   pModel->m_evalFreq     = m_evalModelFreq;

   // policies
   for ( int i=0; i < this->m_policyInfoArray.GetSize(); i++ )
      {
      POLICY_INFO &pInfo = m_policyInfoArray[ i ];

      Policy *pPolicy = pModel->m_pPolicyManager->GetPolicyFromID( pInfo.policyID );
      if ( pPolicy )
         {
         // if doing sensitivity with this policy, reverse it's normal use.
         // Otherwise, set it based on the scenario
         if ( runFlag == -1 && pPolicy->m_useInSensitivity == -1 )
            pPolicy->m_use = ! pInfo.inUse;
         else
            pPolicy->m_use = pInfo.inUse;
         }
      }

   // model variables
   for ( int i=0; i < m_scenarioVarArray.GetSize(); i++ )
      {
      SCENARIO_VAR &vi = m_scenarioVarArray[ i ];    // scenario-specific model variable info

      switch( vi.vtype )
         {
         case V_META:
         case V_MODEL:
         case V_AP:
            {
            // its a ptr to a metagoal, model or autoproc variable.  Two cases are possible:
            // - it is a random value (pRand != NULL ) or it is a constant
            // first get the value, then interpret it
            bool useSensitivity = false;
            VData saValue;

            switch( vi.vtype )
               {
               case V_META:
                  {
                  METAGOAL *pGoal = (METAGOAL*) vi.pVar;
                  useSensitivity = runFlag == -1 && pGoal->useInSensitivity == -1;
                  saValue = pGoal->saValue;
                  }
                  break;

               case V_MODEL:
               case V_AP:
                  {
                  MODEL_VAR *pModelVar = vi.pModelVar;
                  ASSERT( pModelVar != NULL && pModelVar->pVar != NULL );
                  useSensitivity = runFlag == -1 && pModelVar->useInSensitivity == -1;
                  saValue = pModelVar->saValue;
                  }
                  break;
               }

            VData val;

            if ( vi.inUse == false )   // nothing specified, use default if exists 
               {
               if ( vi.defaultValue.IsNull() == false )
                  val = vi.defaultValue;
               }
            else
               {
               if ( vi.pRand != NULL && runFlag == 1 )
                  val = vi.pRand->RandValue();
               else
                  {
                  ASSERT( runFlag != 1 || vi.distType == MD_CONSTANT );
                  val = vi.paramLocation; // constant
                  }
               }
            
            switch( vi.type )
               {
               case TYPE_BOOL:
                  {
                  double r;
                  val.GetAsDouble( r );
                  if ( r > 0.5 )
                     *((bool*)vi.pVar) = true;
                  else
                     *((bool*)vi.pVar) = false;

                  // are we doing sensitivity analysis?
                  if ( useSensitivity )
                     *((bool*)vi.pVar) = !(*((bool*)vi.pVar) );   // flip it
                  }
                  break;

               case TYPE_UINT:
                  {
                  int v;
                  // are we doing sensitivity analysis?
                  if ( useSensitivity )
                     {
                     if ( v == 0 )
                        v = 1;
                     else if ( v == 1 )
                        v = 0;
                     else
                        saValue.GetAsInt( v );
                     }
                  else
                     val.GetAsInt( v );

                  *((UINT*)vi.pVar) = (UINT) v;
                  }
                  break;
                  
               case TYPE_INT:
                  {
                  int v;
                  // are we doing sensitivity analysis?
                  if ( useSensitivity )
                     {
                     if ( v == 0 )
                        v = 1;
                     else if ( v == 1 )
                        v = 0;
                     else
                        saValue.GetAsInt( v );
                     }
                  else
                     val.GetAsInt( v );

                  *((int*)vi.pVar) = (int) v;
                  }
                  break;

               case TYPE_SHORT:
                  {
                  int v;
                  // are we doing sensitivity analysis?
                  if ( useSensitivity )
                     {
                     if ( v == 0 )
                        v = 1;
                     else if ( v == 1 )
                        v = 0;
                     else
                        saValue.GetAsInt( v );
                     }
                  else
                     val.GetAsInt( v );

                  *((short*)vi.pVar) = (short) v;
                  }
                  break;

               case TYPE_LONG:
                  {
                  int v;
                  // are we doing sensitivity analysis?
                  if ( useSensitivity )
                     {
                     if ( v == 0 )
                        v = 1;
                     else if ( v == 1 )
                        v = 0;
                     else
                        saValue.GetAsInt( v );
                     }
                  else
                     val.GetAsInt( v );

                  *((long*)vi.pVar) = (long) v;
                  }
                  break;

               case TYPE_FLOAT:
                  {
                  float v;
                  // are we doing sensitivity analysis?
                  if ( useSensitivity )
                     {
                     if ( v == 0 )
                        v = 1;
                     else if ( v == 1 )
                        v = 0;
                     else
                        saValue.GetAsFloat( v );
                     }
                  else
                     val.GetAsFloat( v );

                  *((float*)vi.pVar) = v;
                  }
                  break;

               case TYPE_DOUBLE:
                  {
                  double v;
                  // are we doing sensitivity analysis?
                  if ( useSensitivity )
                     {
                     if ( v == 0 )
                        v = 1;
                     else if ( v == 1 )
                        v = 0;
                     else
                        saValue.GetAsDouble( v );
                     }
                  else
                     val.GetAsDouble( v );

                  *((double*)vi.pVar) = v;
                  }
                  break;
                  
               default:
                  ASSERT( 0 );
               }
            }
            break;

         case V_APPVAR:
            {
            AppVar *pAppVar = (AppVar*) vi.pVar;
            
            ASSERT( pAppVar != NULL );
            ASSERT( pAppVar->m_pMapExpr != NULL );

            if ( pAppVar->m_useInSensitivity == -1 )  // fixed value
               {
               pAppVar->SetValue( pAppVar->m_saValue );
               }
            else if ( pAppVar->m_timing > 0 )
               {
               if ( vi.inUse )
                  {
                  // use whatever expression string is stored the the paramLocation VData
                  // This will be the MapExpr used in this scenario.  It gets evaluated
                  // as needed through the run.
                  CString expr;
                  bool ok = vi.paramLocation.GetAsString( expr );
                  ASSERT( ok );

                  pAppVar->m_expr = expr;
                  // map expression already exist?  If not, add one.
                  if (pAppVar->m_pMapExpr == NULL)
                     this->m_pScenarioManager->m_pEnvModel->AddExpr(pAppVar->m_name, expr); // and in expression evaluator
                  else
                     pAppVar->m_pMapExpr->SetExpression(expr);
                  }
               else
                  {
                  if ( vi.defaultValue.IsNull() == false )
                     pAppVar->m_expr = vi.defaultValue.GetAsString();
                  }

               if ( pAppVar->IsGlobal() )
                  pAppVar->Evaluate();   // evaluate the AppVar expr and store result
               }
            }
            break;

         case V_SYSTEM:
            {
            // decision Elements?
            if ( vi.pVar == (void*) &m_decisionElements )
               {
               // update EnvModel variable
               EnvModel *pModel = m_pScenarioManager->m_pEnvModel;

               if ( vi.inUse )
                  vi.paramLocation.GetAsInt( pModel->m_decisionElements );
               else
                  vi.defaultValue.GetAsInt( pModel->m_decisionElements );
               }
            }
            break;

         default: ASSERT( 0 );
         }
      }

   return 1;
   }


int ScenarioManager::CopyScenarioArray( CArray< Scenario*, Scenario* > &toArray, CArray< Scenario*, Scenario* > &fromArray )
   {
   for ( int i=0; i < toArray.GetSize(); i++ )
      delete toArray[ i ];

   toArray.RemoveAll();

   for ( int i=0; i < fromArray.GetSize(); i++ )
      toArray.Add( new Scenario( *(fromArray[ i ]) ) );

   return (int) fromArray.GetSize();
   }


Scenario *ScenarioManager::GetScenario( LPCTSTR name )
   {
   for ( int i=0; i < GetCount(); i++ )
      {
      Scenario *pScenario = GetScenario( i );
      if ( pScenario->m_name.CompareNoCase( name ) == 0 )
         return pScenario;
      }

   return NULL;
   }


int ScenarioManager::GetScenarioIndex( Scenario *pScenario )
   {
   for ( int i=0; i < GetCount(); i++ )
      {
      if ( m_scenarioArray.GetAt( i ) == pScenario )
         return i;
      }

   return -1;
   }


void ScenarioManager::DeleteScenario( int index )
   {
   if ( index >= GetCount() )
      return;

   Scenario *pScenario = GetScenario( index );

   m_scenarioArray.RemoveAt( index );
   delete pScenario;
   }


void ScenarioManager::SetDefaultScenario( int index )
   {
   for ( int i=0; i < this->GetCount(); i++ )
      GetScenario( i )->m_isDefault = ( i == index ? true : false );
   }


int ScenarioManager::GetDefaultScenario()
   {
   for ( int i=0; i < this->GetCount(); i++ )
      if ( GetScenario( i )->m_isDefault )
         return i;

   return -1;
   }



int ScenarioManager::LoadXml( LPCTSTR _filename, bool isImporting, bool appendToExisting )
   {
   if ( appendToExisting == false )
      RemoveAll();

   CString filename;
   if ( PathManager::FindPath( _filename, filename ) < 0 ) //  return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 
      {
      CString msg;
      msg.Format( "ScenarioManager: Input file '%s' not found - no scenarios will be loaded", _filename );
      Report::ErrorMsg( msg );
      return false;
      }

   // have xml string, start parsing
   TiXmlDocument doc;
   bool ok = doc.LoadFile( filename );

   if ( ! ok )
      {
      CString msg;
      msg.Format("Error reading scenario input file, %s", (LPCTSTR) m_path );
      Report::ErrorMsg(msg);
      return -1;
      }

   // start interating through the scenarios
   TiXmlElement *pXmlRoot = doc.RootElement();

   int defaultIndex = 0;
   pXmlRoot->Attribute( "default", &defaultIndex );

   int count = LoadXml( pXmlRoot, appendToExisting );
   
   SetDefaultScenario( defaultIndex );

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


int ScenarioManager::LoadXml( TiXmlNode *pScenarios, bool appendToExisting )
   {
   m_loadStatus = 0;

   if ( pScenarios == NULL )
      return -1;
   
   // file specified?.
   LPCTSTR file = pScenarios->ToElement()->Attribute( _T("file") );
   
   if ( file != NULL && file[0] != NULL )
      return LoadXml( file, true, appendToExisting );

   // Note: Files are of the form
   // <scenarios default=[index]>
   //    <scenario name='name' originator='orig' isEditable='0|1' isShared='0|1' actorAltruismWt='value' evalModelFreq='value'>
   //       <policies>
   //           <policy id='value' name='policyname' inUse='0|1' />
   //       </policies>
   //       <vars>
   //         <var vtype='[1|2]' name='description' distType='0-4' paramLocation='value' paramScale='value' paramShape='value' inUse='[0|1]' />
   //       </vars>
   //    </scenario>
   // </scenarios>
   //
   // see EnvExtraData.h for the distType values.
   ASSERT( pScenarios != NULL );

   bool loadSuccess = true;

   // iterate through scenarios
   TiXmlNode *pXmlScenarioNode = NULL;
   while( pXmlScenarioNode = pScenarios->IterateChildren( pXmlScenarioNode ) )
      {
      // get the scenario
      TiXmlElement *pXmlScenario = pXmlScenarioNode->ToElement();

      if ( pXmlScenario == NULL )
         continue;
    
      const char *name = pXmlScenario->Attribute( "name" );

      if ( name == NULL )
         {
         CString msg( "Misformed Scenario element reading" );
         msg += m_path;

         Report::InfoMsg( msg );
         loadSuccess = false;
         continue;
         }
   
      Scenario *pScenario = new Scenario( this, name );

      for ( int i=0; i < pScenario->GetPolicyCount(); i++ )
         pScenario->GetPolicyInfo( i ).inUse = false;

      // get the scenario attributes
      //TiXmlGetAttr( pXmlScenario, _T("originator"), _T("originator"),  LPCTSTR itemName, LPCTSTR attrName, int &value, int defaultValue, bool isRequired )
      const char *originator = pXmlScenario->Attribute( "originator" );
      if ( originator != NULL )
         pScenario->m_originator = originator;

      int isEditable = 1;
      int isShared   = 1;
      pXmlScenario->Attribute( "isEditable", &isEditable );
      pXmlScenario->Attribute( "isShared",   &isShared );

      //double actorAltruismWt = 0.33;
      //double actorSelfInterestWt = 0.33;
      double policyPrefWt = 0.34;
      int evalModelFreq = 1;
      int decisionElements = m_pEnvModel->m_decisionElements;

      //pXmlScenario->Attribute( "landscapeFeedbackWt", &actorAltruismWt );
      //pXmlScenario->Attribute( "actorValueWt", &actorSelfInterestWt );
      pXmlScenario->Attribute( "policyPreferenceWt", &policyPrefWt     );
      pXmlScenario->Attribute( "evalModelFreq",      &evalModelFreq    );
      pXmlScenario->Attribute( "decisionElements",   &decisionElements );

      //pScenario->m_actorAltruismWt = (float) actorAltruismWt;
      //pScenario->m_actorSelfInterestWt = (float) actorSelfInterestWt;
      pScenario->m_policyPrefWt = (float) policyPrefWt;
      pScenario->m_evalModelFreq    = evalModelFreq;
      pScenario->m_decisionElements = decisionElements;

      //NormalizeWeights( pScenario->m_actorAltruismWt, pScenario->m_actorSelfInterestWt, pScenario->m_policyPrefWt );
      TiXmlNode *pXmlDescNode = pXmlScenarioNode->FirstChild( "description" );
      if ( pXmlDescNode )
         {
         TiXmlElement *pDesc = pXmlDescNode->ToElement();
         pScenario->m_description = pDesc->GetText();
         pScenario->m_description.Trim();
         }
            
      // policies are next
      TiXmlNode *pXmlPoliciesNode = pXmlScenarioNode->FirstChild("policies");

      if ( pXmlPoliciesNode != NULL )
         {
         TiXmlElement *pXmlPolicy = pXmlPoliciesNode->FirstChildElement( "policy" );
         while( pXmlPolicy != NULL  )
            {
            if ( pXmlPolicy->Type() == TiXmlElement::ELEMENT)
               {
               // get the policy
               const char *name = pXmlPolicy->Attribute( "name" );
               int id;
               int inUse;
               pXmlPolicy->Attribute( "id", &id );
               pXmlPolicy->Attribute( "inUse", &inUse );

               // make sure the policy name, id match an existing policy
               Policy *pPolicy = m_pEnvModel->m_pPolicyManager->GetPolicyFromID( id );

               if ( pPolicy == NULL )
                  {
                  CString msg;
                  msg.Format( "A Policy (%s, ID: %i) was referenced by a scenario that does not exist in the current policy set.  It will be removed next time you save the scenario.",
                     name, id );
                  Report::LogWarning( msg );
                  }
               else
                  {
                  // policy found, store the recorded info
                  POLICY_INFO *pInfo = pScenario->GetPolicyInfoFromID( id );
                  ASSERT (pInfo != NULL);
                  pInfo->inUse = inUse ? true : false;
                  }
               }

            pXmlPolicy = pXmlPolicy->NextSiblingElement( "policy" );
            }
         }  // end of: if ( pXmlPoliciesNode != NULL )

      // set all scenario variable off
      for ( int i=0; i < pScenario->GetScenarioVarCount(); i++ )
         pScenario->GetScenarioVar( i ).inUse = false;

      // basic scenario parameters set, add variables
      //   <var vtype='[1|2]' name='description' distType='0-4' paramLocation='value' paramScale='value' paramShape='value' inUse='[0|1]' />
      //   <var vtype='[1|2]' name='description' value='[value]' inUse='[0|1]' />
      // get vars node to iterate though
      TiXmlNode *pXmlVarsNode = pXmlScenarioNode->FirstChild("vars");
      if ( pXmlVarsNode != NULL )
         {
         TiXmlElement *pXmlVarNode = pXmlVarsNode->FirstChildElement("var");

         while( pXmlVarNode != NULL )
            {
            const char *name      = NULL;
            const char *distType = NULL;
            //int vtype             = -1;
            const char *vtype = NULL;
            int    inUse          = 1;
            float  value          = 0.0f;
            float  paramScale     = 0.0f;
            float  paramLocation  = 0.0f;
            float  paramShape     = 0.0f;
            XML_ATTR attrs[] = { 
               // attr            type           address         isReq checkCol
               { "name",          TYPE_STRING,   &name,          true,    0 },
               //{ "vtype",         TYPE_INT,      &vtype,         false,   0 },
               { "vtype",         TYPE_STRING,   &vtype,         true,    0 },
               { "distType",      TYPE_STRING,   &distType,      false,   0 },
               { "inUse",         TYPE_INT,      &inUse,         true,    0 },
               { "value",         TYPE_FLOAT,    &value,         false,   0 },
               { "paramScale",    TYPE_FLOAT,    &paramScale,    false,   0 },
               { "paramLocation", TYPE_FLOAT,    &paramLocation, false,   0 },
               { "paramShape",    TYPE_FLOAT,    &paramShape,    false,   0 },
               { NULL,           TYPE_NULL,     NULL,            false,   0 } };

            if ( TiXmlGetAttributes( pXmlVarNode, attrs, this->m_path ) == false )  ///filename, m_pMapLayer ) == false )
               return false;
   
            SCENARIO_VAR *pInfo = pScenario->GetScenarioVar( name );
  
            if ( pInfo == NULL ) // still not found?
               {
               CString msg;
               msg.Format( "Scenario variable [%s] in %s is not a valid scenario variable... ignoring", name, (LPCTSTR) m_path );

               Report::LogWarning( msg );
               loadSuccess = false;
               }
            else
               {
               pInfo->inUse = inUse ? true : false;

               if (::isdigit(vtype[0]))
                  {
                  pInfo->vtype = (VTYPE)atoi(vtype);
                  }
               else
                  {
                  if (_tcsicmp(vtype, _T("system")) == 0)
                     pInfo->vtype = V_SYSTEM;
                  else if (_tcsicmp(vtype, _T("scenario")) == 0)
                     pInfo->vtype = V_SYSTEM;
                  else if (_tcsicmp(vtype, _T("metagoal")) == 0)
                     pInfo->vtype = V_META;
                  else if (_tcsicmp(vtype, _T("evaluator")) == 0)
                     pInfo->vtype = V_MODEL;
                  else if (_tcsicmp(vtype, _T("model")) == 0)
                     pInfo->vtype = V_AP;
                  else if (_tcsicmp(vtype, _T("appvar")) == 0)
                     pInfo->vtype = V_APPVAR;
                  else
                     pInfo->vtype = V_UNKNOWN;
                  }

               if (distType != NULL)
                  {
                  if (isdigit(distType[0]))
                     pInfo->distType = (MODEL_DISTR)atoi(distType);
                  else
                     switch (tolower(distType[0]))
                        {
                        case 'n':   pInfo->distType = MD_NORMAL;  break;
                        case 'u':   pInfo->distType = MD_UNIFORM; break;
                        case 'l':   pInfo->distType = MD_LOGNORMAL; break;
                        case 'w':   pInfo->distType = MD_WEIBULL; break;
                        case 'c':   pInfo->distType = MD_CONSTANT; break;
                        default:
                           {
                           CString msg;
                           msg.Format("Invalid scenario type encountered reading scenario variable '%s'.  This variable will treated as a constant.", name);
                           pInfo->inUse = false;
                           }
                        }
                  }

               if (pInfo->distType == MD_CONSTANT && pXmlVarNode->Attribute("value") != NULL) // constant????
                  pInfo->paramLocation = value;
               else
                  {
                  pInfo->paramScale = (float)paramScale;
                  pInfo->paramLocation = (float)paramLocation;
                  pInfo->paramShape = (float)paramShape;
                  }
               }

            pXmlVarNode = pXmlVarNode->NextSiblingElement("var");
            }  // end of : while( pXmlVarNode != NULL )
         }  // end of: if ( pXmlVarsNode != NULL )

      AddScenario( pScenario );
      pScenario->Initialize();
      }

   if ( loadSuccess == false )
      Report::LogWarning( "There was a problem loading one or more scenarios.  See the Message Tab for details." );

   return GetCount();
   }



int ScenarioManager::SaveXml( LPCTSTR filename )
   {
   if ( m_includeInSave == false )
      return 0;

   // open the file and write contents
   FILE *fp;
   fopen_s( &fp, filename, "wt" );
   if ( fp == NULL )
      {
      LONG s_err = GetLastError();
      CString msg = "Failure to open " + CString(filename) + " for writing.  ";
      Report::SystemErrorMsg( s_err, msg );
      return false;
      }

   bool useFileRef = ( m_loadStatus == 1  ? true : false );

   int count = SaveXml( fp, true, useFileRef );
   fclose( fp );

   return count;
   }


int ScenarioManager::SaveXml( FILE *fp, bool includeHdr, bool useFileRef )
   {
   ASSERT( fp != NULL );

   if ( includeHdr )
      fputs( "<?xml version='1.0' encoding='utf-8' ?>\n\n", fp );

      if ( useFileRef ) 
      {
      if ( m_importPath.IsEmpty() )
         {
#ifndef NO_MFC
         CFileDialog dlg( FALSE, _T("xml"), "", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "XML Files|*.xml|All files|*.*||" );
         if ( dlg.DoModal() == IDCANCEL )
            return 0;
      
         m_importPath = dlg.GetPathName();
#else
	 return 0;
#endif
         }

      int loadStatus = m_loadStatus;
      m_loadStatus = 0;
      fprintf( fp, "<scenarios default='%i' file='%s' />\n", GetDefaultScenario(), (LPCTSTR) m_importPath );
      SaveXml( m_importPath );
      m_loadStatus = loadStatus;
      return GetCount();
      }

    fprintf( fp, "<scenarios default='%i'>\n", GetDefaultScenario() );

    for ( int i=0; i < GetCount(); i++ )
      {
      Scenario *pScenario = GetScenario( i );
      
      int isEditable = pScenario->m_isEditable ? 1 : 0;
      int isShared   = pScenario->m_isShared   ? 1 : 0;
      
//      fprintf( fp, "\t<scenario name='%s' \n\t\toriginator='%s' \n\t\tisEditable='%i' \n\t\tisShared='%i' \n\t\tlandscapeFeedbackWt='%g' \n\t\tactorValueWt='%g' \n\t\tpolicyPreferenceWt='%g' \n\t\tevalModelFreq='%i'>\n",
//         (LPCTSTR) pScenario->m_name, (LPCTSTR) pScenario->m_originator, isEditable, isShared, gpDoc->m_model.m_altruismWt, gpDoc->m_model.m_selfInterestWt, gpDoc->m_model.m_policyPrefWt,
//         gpDoc->m_model.m_evalFreq );
      fprintf( fp, "\t<scenario name='%s' \n\t\toriginator='%s' \n\t\tisEditable='%i' \n\t\tisShared='%i' \n\t\tpolicyPreferenceWt='%g' \n\t\tevalModelFreq='%i'>\n",
         (LPCTSTR) pScenario->m_name, (LPCTSTR) pScenario->m_originator, isEditable, isShared, 
         m_pEnvModel->m_policyPrefWt, m_pEnvModel->m_evalFreq );

      fputs( "\t\t<description>\n\t\t\t", fp );
      if ( pScenario->m_description.GetLength() > 0 )
         fputs( pScenario->m_description, fp );
      fputs( "\n\t\t</description>\n", fp );


      fputs( "\t\t<policies>\n", fp );
      
      // policies next
      for ( int i=0; i < pScenario->GetPolicyCount(); i++ )
         {
         POLICY_INFO &pi = pScenario->GetPolicyInfo( i );
         fprintf( fp, "\t\t\t<policy id='%i' inUse='%i' name='%s' />\n", pi.policyID, pi.inUse, (LPCTSTR)pi.policyName );
         }

      fputs( "\t\t</policies>\n", fp );
      
      // vars next
      fputs( "\t\t<vars>\n", fp );
      
      for ( int j=0; j < pScenario->GetScenarioVarCount(); j++ )
         {
         SCENARIO_VAR &vi = pScenario->GetScenarioVar( j );

         int inUse = vi.inUse ? 1 : 0;
         float paramLocation = 0;
         float paramScale = 0;
         float paramShape = 0;
         vi.paramLocation.GetAsFloat( paramLocation );
         vi.paramScale.GetAsFloat( paramScale );
         vi.paramShape.GetAsFloat( paramShape );

         LPCTSTR vTypeStr = vi.GetTypeLabel();

         // V_UNKNOWN = 0, V_SYSTEM = 1, V_META = 2, V_MODEL = 4, V_AP = 8, V_APPVAR = 16
         switch( vi.vtype )
            {
            case V_META:
            case V_MODEL:
            case V_AP:
            case V_SYSTEM:
            case V_APPVAR:
               {
               if ( vi.distType == MD_CONSTANT )
                  fprintf( fp, "\t\t\t<var vtype='%s' inUse='%i' name='%s' value='%f' />\n", vTypeStr, inUse, (LPCTSTR) vi.name, paramLocation );
               else
                  fprintf( fp, "\t\t\t<var vtype='%s' inUse='%i' distType='%i' name='%s' paramLocation='%f' paramScale='%f' paramShape='%f' />\n",
                     vTypeStr, inUse, (int) vi.distType, (LPCTSTR) vi.name, paramLocation, paramScale, paramShape );
               }
               break;

            default:
               ASSERT( 0 );
            }
         }
      fputs( "\t\t</vars>\n", fp );

      fputs( "\t</scenario>\n\n", fp );
      }

   fputs( "\t</scenarios>\n", fp );
   return GetCount();

   }


void ScenarioManager::AddPolicyToScenarios( Policy *pPolicy, bool inUse )
   {
   for ( int i=0; i < GetCount(); i++ )
      {
      Scenario *pScenario = GetScenario( i );
      pScenario->AddPolicyInfo( pPolicy, inUse );
      }
   }



void ScenarioManager::RemovePolicyFromScenarios( Policy *pPolicy )
   {
   for ( int i=0; i < GetCount(); i++ )
      {
      Scenario *pScenario = GetScenario( i );
      pScenario->RemovePolicyInfo( pPolicy );
      }
   }


