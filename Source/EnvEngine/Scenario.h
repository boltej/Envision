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

#include "Policy.h"
#include <randgen/Rand.hpp>
#include <Typedefs.h>
#ifndef NO_MFC
#include <afxtempl.h>
#endif
#include "EnvContext.h"
#include "tixml.h"

#include "EnvAPI.h"

class ScenarioManager;
class EnvModel;
class AppVar;

bool ENVAPI NormalizeWeights( float &w0, float &w1, float &w2 );
bool ENVAPI NormalizeWeights( float &w0, float &w1, float &w2, float &w3 );
bool ENVAPI NormalizeWeights( float &w0, float &w1, float &w2, float &w3, float &w4 );

enum VTYPE { V_UNKNOWN=0, V_SYSTEM=1, V_META=2, V_MODEL=4, V_AP=8, V_APPVAR=16 }; // must be powers of 2

const int  V_ALL  (int( V_META | V_MODEL | V_AP | V_APPVAR | V_SYSTEM));


// SCENARIO_VAR's describe scenario variables.  They include
// a pointer to an underlying variable tht depends on its type.
// It can be optionally be defined as a probability distribution/
// it is used to set values of "inUse" variables at the 
// start of a new run.

struct SCENARIO_VAR
{
public:
   CString name;
   VTYPE vtype;      // address?  Policy ptr?
   void *pVar;       // pointer to the variable to be used      
   TYPE  type;       // type of that variable
   MODEL_DISTR distType;
   Rand *pRand;      // random number generator
   VData defaultValue;
   VData paramLocation; // Note: also stores ID for policies, values for consts
   VData paramScale;
   VData paramShape;
   bool  inUse;      // use this variable?
   CString description;

   MODEL_VAR *pModelVar;  // set of model var-associated SCENARIO_VARs, NULL otherwise (e.g. AppVars)

   // various constructors
   SCENARIO_VAR() : vtype( V_UNKNOWN ), pVar( NULL ), type( TYPE_NULL ), distType( MD_CONSTANT ), pRand( NULL ), defaultValue(), 
      inUse( true ), pModelVar( NULL ) { }

   SCENARIO_VAR( LPCTSTR _name, VTYPE _vtype, void* _pVar, TYPE _type, Rand *_pRand, VData _paramLocation, VData _paramScale, VData _paramShape, bool _inUse, LPCTSTR _description )
      : name( _name ), vtype( _vtype ), pVar( _pVar ), type( _type ), distType( MD_CONSTANT ), pRand( _pRand ), 
        defaultValue( _paramLocation ), paramLocation( _paramLocation ), paramScale( _paramScale ), paramShape( _paramShape ),
        inUse( _inUse ), description( _description ), pModelVar( NULL ) { } 

   SCENARIO_VAR( const SCENARIO_VAR &vi ) { *this = vi; }

   // from a exposed Model Output
   SCENARIO_VAR( VTYPE vt, MODEL_VAR &mv, LPCTSTR modelName )
      : vtype( vt ), pVar( mv.pVar ), type( mv.type ), defaultValue(), paramLocation( mv.paramLocation ), paramScale( mv.paramScale ), 
      paramShape( mv.paramShape ), distType( mv.distType ), pRand( NULL ), inUse( true ),
      pModelVar( &mv )
      {
      name = modelName;
      name += ".";
      name += mv.name;

      description = mv.description;

      // get the default value from the model variable
      defaultValue = mv.paramLocation;
      }

   // from an AppVar
   SCENARIO_VAR( AppVar *pAppVar );

   void operator = ( const SCENARIO_VAR &e ) 
      {
      name = e.name; vtype = e.vtype; pVar = e.pVar; type = e.type; distType = e.distType;
      paramLocation = e.paramLocation; paramScale = e.paramScale; paramShape = e.paramShape;
      inUse = e.inUse; description = e.description; 
      defaultValue = e.defaultValue; pModelVar = e.pModelVar;
      pRand = NULL;
      }

   LPCTSTR GetTypeLabel()
      {
      switch (vtype)
         {
         case V_UNKNOWN: return _T("unknown");
         case V_SYSTEM: return _T("system");
         case V_META: return _T("metagoal");
         case V_MODEL: return _T("evaluator");
         case V_AP: return _T("model");
         case V_APPVAR: return _T("appvar");
         default: return NULL;
         }
      }

   ~SCENARIO_VAR() { if ( pRand != NULL ) delete pRand; }
};


class ScenarioVarArray : public CArray< SCENARIO_VAR, SCENARIO_VAR& >
{
public:
   ScenarioVarArray() : CArray< SCENARIO_VAR, SCENARIO_VAR& >() { }

   ScenarioVarArray( ScenarioVarArray &viArray ) : CArray< SCENARIO_VAR, SCENARIO_VAR& >()
      {
      for ( int i=0; i < viArray.GetSize(); i++ )
         {
         SCENARIO_VAR &vi = viArray.GetAt( i );
         Add( vi );
         }
      }
};


//=======================================================================================
// Scenario class manages the definition of scenarios to be run.  A Scenario is a 
//   collection of variables (SCENARIO_VAR's) and policies that constitute the settings for
//   a given run or multirun.
//
// To add a new SCENARIO_VAR to a scenario, do the following:
//   1) Add the SCENARIO_VAR to the Scenario's collection
//=======================================================================================

struct POLICY_INFO
   {
   CString policyName;
   int     policyID;
   bool    inUse;
   Policy *pPolicy;

   POLICY_INFO() : policyName( "" ), policyID( -1 ), inUse( false ) {}

   POLICY_INFO( Policy *_pPolicy, bool use ) : policyName( _pPolicy->m_name ),
      policyID( _pPolicy->m_id ), inUse( use ), pPolicy( _pPolicy ) { }

   POLICY_INFO( const POLICY_INFO &pi ) : policyName( pi.policyName ), policyID( pi.policyID ), inUse( pi.inUse ), pPolicy( pi.pPolicy ) { }
   };


class PolicyInfoArray : public CArray< POLICY_INFO, POLICY_INFO& >
{
public:
   PolicyInfoArray() : CArray< POLICY_INFO, POLICY_INFO& >() { }

   PolicyInfoArray( PolicyInfoArray &pi ) : CArray< POLICY_INFO, POLICY_INFO& >()
      {
      for ( int i=0; i < pi.GetSize(); i++ )
         Add( pi.GetAt( i ) );      
      }

   int AddPolicyInfo( Policy *pPolicy, bool use )
      {
      POLICY_INFO pi(pPolicy, use);  return (int)CArray<POLICY_INFO, POLICY_INFO&>::Add(pi);
      }
};


class ENVAPI Scenario
{
friend class ScenarioManager;

public:
   Scenario( ScenarioManager*, LPCTSTR name );   // construct a default scenario
   Scenario( Scenario& );     // copy constructor
   ~Scenario( void );

   ScenarioManager *m_pScenarioManager;
   CString m_name;
   CString m_description;
   CString m_originator;
   bool    m_isEditable;
   bool    m_isShared;

private:
   bool    m_isDefault;        // maintained by the scenario manager

public:

   //float   m_actorAltruismWt;       // Value between 0-1.
   //float   m_actorSelfInterestWt;   // Value between 0-1.
   float   m_policyPrefWt;          // Value between 0-1.
   int     m_decisionElements;      // scenario-specific version

   int     m_evalModelFreq;    // Frequency (years) at which landscape evaluative models are run

   int    m_runCount;          // how many times has this scenario been run?

   void Initialize();
   int  SetScenarioVars( int runFlag );   // see envModel.cpp for runFlag definition

   int GetScenarioVarCount( int type=V_ALL, bool inUseOnly=false );
   SCENARIO_VAR &GetScenarioVar( int i ) { return m_scenarioVarArray[ i ]; }
   SCENARIO_VAR *GetScenarioVar( LPCTSTR name );
   SCENARIO_VAR *FindScenarioVar( void* ptr );

   bool IsDefault() { return m_isDefault; }

   int GetPolicyCount( bool inUseOnly=false );
   int AddPolicyInfo( Policy *pPolicy, bool inUse ) { return m_policyInfoArray.AddPolicyInfo( pPolicy, inUse ); }
   POLICY_INFO &GetPolicyInfo( int i ) { return m_policyInfoArray[ i ]; }
   POLICY_INFO *GetPolicyInfo( LPCTSTR name );
   POLICY_INFO *GetPolicyInfoFromID( int id );
   void RemovePolicyInfo( Policy *pPolicy );

protected:
   ScenarioVarArray m_scenarioVarArray;    // contains the variables use on this scenarion
   PolicyInfoArray  m_policyInfoArray;     // contains info about the policies used in this scenario
};


class ENVAPI ScenarioManager
{
public:
   ScenarioManager( EnvModel *pModel ) : m_pEnvModel(pModel), m_loadStatus( -1 ), m_includeInSave( true ) { }
   ~ScenarioManager() { RemoveAll(); }
   
   EnvModel *m_pEnvModel;
   CString m_path;
   CString m_importPath;          // if imported, path of  import file
   int     m_loadStatus;          // -1=undefined, 0=loaded from envx, 1=loaded from xml file
   bool    m_includeInSave;       // when saving project with external ref, update this file?

   CArray< Scenario*, Scenario* > m_scenarioArray;

   void RemoveAll() { for (int i=0; i < GetCount(); i++ ) delete GetScenario( i ); m_scenarioArray.RemoveAll(); }
   int  GetCount() { return(int) m_scenarioArray.GetSize(); }
   int  AddScenario( Scenario *pScenario ) { return (int) m_scenarioArray.Add( pScenario ); }
   void DeleteScenario( int index );
   Scenario *GetScenario( int i ) { if ( i >= 0 )return m_scenarioArray[ i ]; else return NULL; }
   Scenario *GetScenario( LPCTSTR );
   int  GetScenarioIndex( Scenario *pScenario );
   void SetDefaultScenario( int index );
   int  GetDefaultScenario();

   //int LoadScenarios( LPCTSTR filename=NULL );
   //int SaveScenarios( LPCTSTR filename );

   int LoadXml( LPCTSTR filename, bool isImporting, bool appendToExisting );       // returns number of nodes in the first level
   int LoadXml( TiXmlNode *pLulcTree, bool appendToExisting );
   int SaveXml( LPCTSTR filename );
   int SaveXml( FILE *fp, bool includeHdr, bool useFileRef );

   void AddPolicyToScenarios( Policy *pPolicy, bool inUse );
   void RemovePolicyFromScenarios( Policy *pPolicy );

   static int CopyScenarioArray( CArray< Scenario*, Scenario* > &toScenarioArray, CArray< Scenario*, Scenario* > &fromScenarioArray );

};

