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
// Actor.h: interface for the Actor class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTOR_H__9DAE65F7_4F78_4557_AE09_6D0AB2DF171E__INCLUDED_)
#define AFX_ACTOR_H__9DAE65F7_4F78_4557_AE09_6D0AB2DF171E__INCLUDED_

#ifndef NO_MFC
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxtempl.h>
#endif
#include <tixml.h>
#include <randgen/Randnorm.hpp>
#include <randgen/Randunif.hpp>
#include <PtrArray.h>
#include <PropertyList.h>
#include <misc.h>

#include "EnvAPI.h"

class EnvModel;
class MapLayer;
class Actor;
class ActorManager;
class EnvPolicy;

typedef void (*ACTOR_NOTIFYPROC)( Actor*, int, long );


// ActorGroup - indicate the general class of the actor (rather than specific actor instances
class ENVAPI ActorGroup
{
friend class Actor;
public:
   ActorGroup();
   ActorGroup( ActorGroup&);

   CString  m_name;
   CString  m_originator;
   CString  m_studyArea;
   int      m_id;                 // id of this actorGroup in database
   CString  m_query;              // empty for anything but AIM_QUERY
   int      m_decisionFrequency;  // how often this actor makes a decision on a particular parcel (years)
   int      m_count;              // how many instances of objects, class Actor, in this instance 
   float    m_valueStdDev;
   CArray<float,float> m_values;

   int   m_decisionElements;      // actor-specific decision element flags.  See EnvModel.h

   float m_altruismWt;        // Altruism dial: number in (0,1) that determines 
                              // how much actors consider scarcity when choosing policies.
   float m_selfInterestWt;    // Self-interest dial: number in (0,1) that determines 
                              // how much actors consider own values when choosing policies.
   float m_utilityWt;         // utility dial (0,1) that determines
                              // how much actors weight an externally defined utility fn that defines policy scores
   float m_socialNetworkWt;   // social network dial (0,1) that determines
                              // how much actors weigh influences from a social network during policy selection

protected:
   CArray<bool,bool> m_policies; // array of policies flags for determining which are policies this actor uses
                                 // NOTE: This is currently ignored...
public:
   void  SetPolicyCount( int count );
   void  SetApplyPolicy( int policy, bool doesPolicyApply ) { m_policies[ policy ] = doesPolicyApply; }
   bool  DoesPolicyApply( int policy ) { return m_policies[ policy ]; }
   void  SetValueCount( int size );
   int   GetValueCount( void ) { return (int) m_values.GetSize(); }
   void  SetValue( int index, float value ) { m_values[ index ] = value; }
   float GetValue( int index ) { return m_values[ index ]; }
};


// Actor - specific instances of idividuals in an ActorGroup
class ENVAPI Actor
{
public:
	Actor();
   Actor( Actor &actor ) { *this = actor; }  // copy constructor
	virtual ~Actor();

   Actor & operator = ( Actor& );

   ActorGroup *m_pGroup;         // what group does this belong to
   int         m_index;          // index of this actor in the ActorManager array
   
protected:
   // array of polygons associate with this Actor
   CUIntArray m_polyIndexArray;

   // objectives information
   CArray<float,float> m_valueArray;

   RandUniform m_randShuffle;      // used for shuffling arrays

   PropertyList m_propertyList;
   
public:
   //float m_altruismWt;        // Altruism dial: number in (0,1) that determines 
                              // how much actors consider scarcity when choosing policies.
   //float m_selfInterestWt;    // Self-interest dial: number in (0,1) that determines 
                              // how much actors consider own values when choosing policies.   
   //float m_utilityWt;         // Utility dial (0,1) that determines
                              // how much actors weight an externally defined utility fn that defines policy scores

   float GetAltruismWt( void ) { return m_pGroup->m_altruismWt; }        // Altruism dial: number in (0,1) that determines 
                              // how much actors consider scarcity when choosing policies.
   float GetSelfInterestWt( void ) { return m_pGroup->m_selfInterestWt; }    // Self-interest dial: number in (0,1) that determines 
                              // how much actors consider own values when choosing policies.   
   float GetUtilityWt( void ) { return m_pGroup->m_utilityWt; }         // Utility dial (0,1) that determines
                              // how much actors weight an externally defined utility fn that defines policy scores
   float GetSocialNetworkWt( void ) { return m_pGroup->m_socialNetworkWt; }


   static ACTOR_NOTIFYPROC m_notifyProc;
 
   long m_extra;

public:
   // history of policies applied by this actor
   CArray< EnvPolicy*, EnvPolicy* > m_policyHistoryArray;

   int GetDecisionFrequency() { return m_pGroup->m_decisionFrequency; } // how often this actor makes a decision on a particular parcel (years)
   int GetID() { return m_pGroup->m_id; }
   bool DoesPolicyApply( int policy ) { return m_pGroup->m_policies[ policy ]; }

   // internal data management
   void AddPoly( int index )      { m_polyIndexArray.Add( index ); }
   void SetPoly( int index, int polyIndex ) { m_polyIndexArray[ index ] = polyIndex; }
   void RemovePoly( int index );
   int  GetPolyCount()            { return (int) m_polyIndexArray.GetSize(); }
   void SetPolyCount( int count ) { return m_polyIndexArray.SetSize( count ); }
   int  GetPoly( int i )          { return m_polyIndexArray[ i ]; }
   void ShufflePolyIDs( void )    { ShuffleArray< UINT >( m_polyIndexArray.GetData(), m_polyIndexArray.GetSize(), &m_randShuffle ); }
   
// initialization                 
   void InitRun() { m_policyHistoryArray.RemoveAll(); }
   
   // simulation management
   /*Not Coded*/int Update( void );                          
   void  AddValue( float weight=0.0f )      { m_valueArray.Add( weight ); }
   int   GetValueCount( void )              { return (int) m_valueArray.GetSize(); }
   float GetValue( int index )              { return m_valueArray[ index ]; }
   bool  SetValue( int index, float value ) {  m_valueArray[ index ] = value; return true; }
   void  SetValueCount( int count )         { m_valueArray.SetSize( count ); }

   // property management
   bool GetPropValue( LPCTSTR propName, VData   &value ) { return m_propertyList.GetPropValue( propName, value ); }
   bool GetPropValue( LPCTSTR propName, float   &value ) { return m_propertyList.GetPropValue( propName, value ); }
   bool GetPropValue( LPCTSTR propName, int     &value ) { return m_propertyList.GetPropValue( propName, value ); }
   bool GetPropValue( LPCTSTR propName, bool    &value ) { return m_propertyList.GetPropValue( propName, value ); }
   bool GetPropValue( LPCTSTR propName, CString &value ) { return m_propertyList.GetPropValue( propName, value ); }

   void SetPropValue( LPCTSTR propName, VData   &value ) { m_propertyList.SetPropValue( propName, value ); }
   void SetPropValue( LPCTSTR propName, float    value ) { m_propertyList.SetPropValue( propName, value ); }
   void SetPropValue( LPCTSTR propName, int      value ) { m_propertyList.SetPropValue( propName, value ); }
   void SetPropValue( LPCTSTR propName, bool     value ) { m_propertyList.SetPropValue( propName, value ); }
   void SetPropValue( LPCTSTR propName, CString &value ) { m_propertyList.SetPropValue( propName, value ); }

   void SetNotifyProc( ACTOR_NOTIFYPROC proc, long extra ) { m_notifyProc = proc; m_extra = extra; }
};


class ActorArray : public PtrArray< Actor >
   { };


class ActorGroupArray : public PtrArray< ActorGroup >
   { };


// Actor Association types
enum { AAT_NONE=0, AAT_COMMONATTRIBUTE=1, AAT_VALUECLUSTER=2,  AAT_NEIGHBORS=4  };


class ActorAssociation
{
public:
   ActorAssociation( int type ) : m_associationType( type ) {}
   ActorAssociation( void ) : m_associationType( AAT_NONE ) {}

   int m_associationType;

   CArray< Actor*, Actor* > m_actorArray;
};

// Methods for inputting actor values
enum ACTOR_INIT_METHOD { AIM_NONE=0, AIM_IDU_WTS=1, AIM_IDU_GROUPS=2, AIM_QUERY=3, AIM_UNIFORM=4, AIM_RANDOM=5, AIM_END=6 };

class ENVAPI ActorManager
{
protected:
   EnvModel       *m_pModel;       // associated EnvModel
   ActorArray      m_actorArray;
   ActorGroupArray m_actorGroupArray;

   int m_defaultActorGroupIndex;

   CArray< ActorAssociation*, ActorAssociation*> m_associationArray;

   void ResetActorIndices();
   
public:
   // actor management
   ActorManager( EnvModel *pModel) : m_pModel(pModel), m_actorAssociations( 0 ), m_actorInitMethod( AIM_NONE ), m_dynamicUpdateInterval( -1 ),
            m_rand( 1, 0, 1 ), m_defaultActorGroupIndex( -1 ), m_loadStatus( -1 ) { }
   ~ActorManager() { RemoveAll(); }
   
   void RemoveAll() { RemoveAllAssociations(); RemoveAllActorGroups(); RemoveAllActors(); }

   ACTOR_INIT_METHOD m_actorInitMethod;   // 1=databaseGroups, 2=IDU coverage, 3=uniform, 4=random
   CString m_actorInitArgs;
   int     m_actorAssociations;
   int     m_associationTypes;
   int     m_dynamicUpdateInterval;    // -1 means no dynamic update

   // random number generators
   RandNormal m_rand;
   RandUniform m_randShuffle;      // used for shuffling arrays

   CString m_path;
   CString m_importPath;          // if imported, path of  import file
   int     m_loadStatus;          // -1=undefined, 0=loaded from envx, 1=loaded from xml file
 
   int LoadXml( LPCTSTR filename, bool isImporting, bool appendToExisting );       // returns number of nodes in the first level
   int LoadXml( TiXmlNode *pActorsNode, bool isImporting );
   int SaveXml( LPCTSTR filename );
   int SaveXml( FILE *fp, bool includeHdr, bool useFileRef );

   int  CreateActors( void );

protected:
   int CreateActorsFromIduWts();
   int CreateActorsFromIduGroups();
   int CreateActorsFromQuery();
   int CreateActorUniform();

public:
   Actor *CreateActorFromGroup( ActorGroup *pGroup, bool random );

protected:
   int  AddActorSelfInterestColumns( bool bForce /*=false*/);
   void CopyActorValuesToCells();
   bool IsDynamic( void ) { return m_dynamicUpdateInterval > 0 ? true : false; }

public:
   int GetActorCount() { return (int) m_actorArray.GetSize(); }
   int GetActorGroupCount() { return (int) m_actorGroupArray.GetSize(); }

   Actor *GetActor( int i ) { return m_actorArray[ i ]; }
   Actor *GetActorFromIDU( int idu, int *index=NULL );

   ActorGroup *GetActorGroup( int i ) { return m_actorGroupArray[ i ]; }
   ActorGroup *GetActorGroupFromID( int id );
   int         GetActorGroupIndexFromID( int id );
   void        ShuffleActors( void ) { ShuffleArray< Actor* >( m_actorArray.GetData(), m_actorArray.GetSize(), &m_randShuffle ); }

   int  GetManagedCellCount( void );

   void RemoveActor( int i ) { m_actorArray.RemoveAt( i ); ResetActorIndices(); }
   void RemoveAllActors() { m_actorArray.RemoveAll(); }
   void RemoveAllActorGroups() { m_actorGroupArray.RemoveAll(); }
   int  AddActor( Actor *pActor ) { return pActor->m_index = (int) m_actorArray.Add( pActor ); }
   int  AddActorGroup( ActorGroup *pGroup ) { return (int) m_actorGroupArray.Add( pGroup ); }

   // actor association management
   // note: for BuildAssocations(), extra=col for types=ATT_COMMONATTRIBUTE 
   int  BuildAssociations( int types, MapLayer *pLayer, int colActor, int extra, bool clearPrevious=true );
   void RemoveAllAssociations();

};



#endif // !defined(AFX_ACTOR_H__9DAE65F7_4F78_4557_AE09_6D0AB2DF171E__INCLUDED_)
