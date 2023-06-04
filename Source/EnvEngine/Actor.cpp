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
// Actor.cpp: implementation of the Actor class.
//
//////////////////////////////////////////////////////////////////////


#include "stdafx.h"

#include "Actor.h"
#include "EnvModel.h"
#include "EnvPolicy.h"
#include "EnvException.h"

#include <Maplayer.h>
#include <PathManager.h>

bool showMessage = true;

//------------------------------------------------------------------------------------------------------------------
// Comments about actors:
//
// 1) Actors are specific instances of ActorGroups.  In effect, ActorGroups are templates from which Actors are made.
//
// 2) There are several possible ways of specifying actors, indicated by the actorInitMethod flags
//    a) AIM_NONE - no actors created or used
//    b) AIM_IDU_WTS - actors are created from weights specified in the IDU coverage.  No groups are defined in the project file
//    c) AIM_IDU_GROUPS - actors are created from Groups defined in the project file.  The IDU cover contains the group ID in a field called ACTOR
//    d) AIM_QUERY - actors are created based on a query string specified for the actorGroup in the project file
//    e) AIM_UNIFORM - a single actor is created from a single Group specified in the project file
//    f) AIM_RANDOM - not currently supported.
//
// ---------------------------------------------------------------------------------------------------------------------
// init method     |     Groups in          | number of Actors |  Actor creation method     | Reqd Col? | Notes
//                 |    Project File?       |    created       |                            |           |
//----------------------------------------------------------------------------------------------------------------------
// AIM_NONE        | none (ignored)         |      none        | none                       | none      |
// AIM_IDU_WTS     | one only               |  one per IDU     | CreateActorsFromIduWts()   | one/wt    | Group wts ignored, col names are ACTOR_XXXX
// AIM_IDU_GROUPS  | yes, GroupID specified |  one per IDU     | CreateActorsFromIduGroups()| ACTOR     |
// AIM_QUERY       | yes, query specified   |  one per IDU     | CreateActorsFromQuery()    | ACTOR     |
// AIM_UNIFORM     | one only               |      one         | CreateActorUniform()       | none      |
// AIM_RANDOM      | none (ignored)         |  one per IDU     | not yet supported          | none      |  not yet supported
//---------------------------------------------------------------------------------------------------------------------

ACTOR_NOTIFYPROC Actor::m_notifyProc = NULL;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Actor::Actor()
   : m_polyIndexArray()
   , m_valueArray()
   , m_extra( 0 )
     //m_altruismWt( 0.5f ),
     //m_selfInterestWt( 0.5f ),
     //m_utilityWt( 0.0f )
   {
   // note:  objectives are added when envx file is loaded
   }


Actor& Actor::operator = ( Actor &actor )
   {
   m_pGroup = actor.m_pGroup;
   m_extra  = actor.m_extra;

   m_valueArray.Copy( actor.m_valueArray );
   m_polyIndexArray.Copy( actor.m_polyIndexArray );

   return *this;
   }

Actor::~Actor()
   { }



void Actor::RemovePoly( int index )  
   { 
   int count = (int) m_polyIndexArray.GetCount();
   for ( int i=0; i<count; i++ )
      {
      if ( m_polyIndexArray[ i ] == index )
         {
         m_polyIndexArray.RemoveAt( i ); 
         return;
         }
      }

   ASSERT(0);  // index was not in m_polyIndexArray
   }

/*
void Actor::ShufflePolyIDs( void )
   {
   int iduCount = (int) m_polyIndexArray.GetSize();

   //--- Shuffle elements by randomly exchanging each with one other.
   for ( int i=0; i < iduCount-1; i++ )
      {
      int randVal = (int) m_randShuffle.RandValue( 0, iduCount-i-0.0001f );

      int r = i + randVal; // Random remaining position.
      int temp = m_polyIndexArray[ i ];
      m_polyIndexArray[i] = m_polyIndexArray[r];
      m_polyIndexArray[r] = temp;
      }
   }
*/   


//////////////////////////////////////////////////////////////////////////
//              A C T O R G R O U P 
//////////////////////////////////////////////////////////////////////////


ActorGroup::ActorGroup()
   : m_id( -1 )
   , m_decisionFrequency( -1 )
   , m_count( 0 )
   , m_valueStdDev( 0.0f )
   , m_altruismWt( 0.33f )
   , m_selfInterestWt( 0.33f )
   , m_utilityWt( 0 )
   , m_socialNetworkWt( 0 )
   , m_decisionElements( 0 )      // actor-specific decision use flags
   { }


ActorGroup::ActorGroup( ActorGroup &group )
:  m_name( group.m_name ),
   m_originator( group.m_originator ),
   m_studyArea( group.m_studyArea ),
   m_id( group.m_id ),
   m_query( group.m_query ),
   m_decisionFrequency( group.m_decisionFrequency ),
   m_count( group.m_count ),
   m_valueStdDev( group.m_valueStdDev ),
   m_altruismWt( group.m_altruismWt ),
   m_selfInterestWt( group.m_selfInterestWt ),
   m_utilityWt( group.m_utilityWt ),
   m_socialNetworkWt( group.m_socialNetworkWt ),
   m_decisionElements( group.m_decisionElements )      // actor-specific decision use flags
   {
   m_values.Copy( group.m_values );
   m_policies.Copy( group.m_policies );
   }


void ActorGroup::SetPolicyCount( int count )
   {
   m_policies.SetSize( count );
   bool *data = m_policies.GetData();

   memset( (void*) data, 0, count*sizeof( bool ) );   // all false to start
   }

void ActorGroup::SetValueCount( int count )
   {
   m_values.SetSize( count );
   float *data = m_values.GetData();

   memset( (void*) data, 0, count*sizeof( float ) );   // all 0 to start
   }


//--------------------------------------------------------
// Basic initialization method.
//--------------------------------------------------------

int ActorManager::CreateActors( void )
   {
   AddActorSelfInterestColumns( true );

   int groupCount = GetActorGroupCount();
 
   for ( int i=0; i < groupCount; i++ )
      {
      ActorGroup *pGroup = GetActorGroup( i );
      ASSERT( pGroup != NULL );
      pGroup->m_count = 0;
      }

   switch ( m_actorInitMethod ) 
      {
      case AIM_NONE:
         RemoveAll();
         break;
         
      case AIM_IDU_WTS:
         CreateActorsFromIduWts();
         break;

      case AIM_IDU_GROUPS:
         CreateActorsFromIduGroups();
         CopyActorValuesToCells();
         break;   

      case AIM_QUERY:
         CreateActorsFromQuery();
         CopyActorValuesToCells();
         break;

      case AIM_UNIFORM:
         CreateActorUniform();
         CopyActorValuesToCells();
         break;

      default:
         ASSERT( 0 );
         break;
      }

   return GetActorCount();
   }


int ActorManager::CreateActorsFromIduWts( void )
   {
   ASSERT( m_actorInitMethod == AIM_IDU_WTS );

   RemoveAllActors();
   RemoveAllAssociations();
 
   int groupCount = GetActorGroupCount();
   ASSERT( groupCount == 1 );

   ActorGroup *pGroup = GetActorGroup( 0 );
   
   // Pick the actor values from the DB
   float wt;   
   Actor *pActor = NULL;
   int badActorCount = 0;
   int valueCount = this->m_pModel->GetActorValueCount();

   for ( MapLayer::Iterator i = this->m_pModel->m_pIDULayer->Begin(MapLayer::ALL); i != this->m_pModel->m_pIDULayer->End(MapLayer::ALL); ++i)
      {
      pActor = CreateActorFromGroup( pGroup, false );   // init values based on Group mean

      // reset values based on IDU info
      for ( int val=0; val < valueCount; ++val )
         { 
         this->m_pModel->m_pIDULayer->GetData((int)i, this->m_pModel->m_colActorValues[ val ], wt );
         
         if ( wt == this->m_pModel->m_pIDULayer->GetNoDataValue() )
            {
            wt = 0.0f;
            badActorCount++;               
            }
         else if ( wt > 3.0f || wt < -3.0f ) 
            {
            CString msg;
            msg.Format("IDU coverage error specifying actor weights: Invalid value in record %d of col %s" ,
               (int)i, this->m_pModel->m_pIDULayer->GetFieldLabel(this->m_pModel->m_colActorValues[val]));
            Report::ErrorMsg(msg);

            wt = (wt < 0 ) ? -3.0f : 3.0f; 
            }

         pActor->SetValue( val, wt );   // was AddValue()  - jpb 7/24/12
         }

      if (this->m_pModel->m_colActor >= 0 )
         this->m_pModel->m_pIDULayer->SetData((int) i, this->m_pModel->m_colActor, pActor->m_index ); // SUBDIVIDE fails without this column

      pActor->AddPoly( (int)i ); // add this cell to the actor
      pGroup->m_count++;

      }

   // actors created and loaded, create associations
   //if ( m_actorAssociations )
   //   gpActorManager->BuildAssociations( this->m_actorAssociations, 0, true );
   //TODO:  Need to modify "Extras" argument

   return GetActorCount();
   }


int ActorManager::CreateActorsFromIduGroups( void )
   {
   ASSERT( m_actorInitMethod == AIM_IDU_GROUPS );

   RemoveAllActors();
   RemoveAllAssociations();
 
   int groupCount = GetActorGroupCount();
   ASSERT( groupCount > 0 );
   
   // Pick the actor groups from the DB
   Actor *pActor = NULL;
   int badActorCount = 0;
   int groupID = -1;

   int colGroupID = this->m_pModel->m_pIDULayer->GetFieldCol( _T( "ACTOR" ) );
   if ( colGroupID < 0 )
      {
      CString msg( _T("IDU coverage is missing the [ACTOR] field - this is required when initializing actors from groups.  No actors will be created" ) );
      Report::ErrorMsg(msg);
      return 0;
      }

   int valueCount = this->m_pModel->GetActorValueCount();

   for ( MapLayer::Iterator i = this->m_pModel->m_pIDULayer->Begin(MapLayer::ALL); i != this->m_pModel->m_pIDULayer->End(MapLayer::ALL); ++i)
      {
      bool ok = this->m_pModel->m_pIDULayer->GetData( i, colGroupID, groupID );
      ASSERT( ok );

      if ( groupID == this->m_pModel->m_pIDULayer->GetNoDataValue() )
         continue;

      ActorGroup *pGroup = this->GetActorGroupFromID( groupID );

      if ( pGroup == NULL )
         {
         CString msg;
         msg.Format( _T("Invalid group ID found in IDU when creating actors: ACTOR=%i at record %i.  This group ID is not defined in the project file"), groupID, (int) i );
         Report::ErrorMsg(msg);
         continue;
         }

      pActor = CreateActorFromGroup( pGroup, true );   // set and randomize 

      ASSERT( this->m_pModel->m_colActor >= 0 );
      //this->m_pModel->m_pIDULayer->SetData((int) i, this->m_pModel->m_colActor, pActor->m_index ); 

      pActor->AddPoly( (int) i ); // add this cell to the actor
      pGroup->m_count++;
      }

   // actors created and loaded, create associations
   //if ( m_actorAssociations )
   //   gpActorManager->BuildAssociations( this->m_actorAssociations, 0, true );
   //TODO:  Need to modify "Extras" argument

   return GetActorCount();
   }

int ActorManager::CreateActorsFromQuery( void )
   {
   ASSERT( m_actorInitMethod == AIM_QUERY );

   RemoveAllActors();
   RemoveAllAssociations();
 
   int groupCount = GetActorGroupCount();
   ASSERT( groupCount > 0 );

   int colGroupID = this->m_pModel->m_pIDULayer->GetFieldCol( _T( "ACTOR" ) );
   if ( colGroupID < 0 )
      {
      CString msg( _T("IDU coverage is missing the [ACTOR] field - this is required when initializing actors from queries. This column will be added to the IDU coverage" ) );
      Report::LogWarning(msg);
      colGroupID = this->m_pModel->m_pIDULayer->m_pDbTable->AddField( _T("ACTOR"), TYPE_INT );
      }

   QueryEngine qe( this->m_pModel->m_pIDULayer );
 
   for ( int i=0; i < groupCount; i++ )
      {
      ActorGroup *pGroup = GetActorGroup( i );
      ASSERT ( pGroup != NULL );

      if ( pGroup->m_query.IsEmpty() )
         {
         CString msg( _T( "Empty Query specified for actor group " ) );
         msg += pGroup->m_name;
         msg += _T ("; No actors will be created for this group..." );
         Report::ErrorMsg( msg );
         continue;
         }

      Query *pQuery = qe.ParseQuery( pGroup->m_query, 0, "Actor Query" );

      if (  pQuery == NULL  )  // error parsing query???
         {
         CString msg( _T( "Unable to parse query while creating actors: " ) );
         msg += pGroup->m_query;
         msg += _T( " for group " );
         msg += pGroup->m_name;
         Report::ErrorMsg( msg );
         continue;
         }

      WAIT_CURSOR;

      qe.SelectQuery( pQuery, true ); //, false );

      // get area of selection
      int selCount = this->m_pModel->m_pIDULayer->GetSelectionCount();

      for ( int j=0; j < selCount; j++ )
         {
         int idu = this->m_pModel->m_pIDULayer->GetSelection( j );
         this->m_pModel->m_pIDULayer->SetData( idu, colGroupID, pGroup->m_id );
         }

      this->m_pModel->m_pIDULayer->ClearSelection();
      }

   m_actorInitMethod = AIM_IDU_GROUPS;
   CreateActorsFromIduGroups();
   m_actorInitMethod = AIM_QUERY;

   return GetActorCount();
   }


int ActorManager::CreateActorUniform()
   {
   RemoveAllActors();
   RemoveAllAssociations();

   if ( GetActorGroupCount() == 0 )
      {
#ifndef NO_MFC
      int retVal = AfxMessageBox( _T( "No actor groups have been defined - would you like to create a default Actor group (recommended)?"), MB_YESNO );

      if ( retVal == IDNO )
         return 0;
#endif
      ActorGroup *pGroup = new ActorGroup;
      pGroup->m_name = _T("Uniform Actor");
      pGroup->m_id = 0;                 // id of this actorGroup in database
      pGroup->m_decisionFrequency = 2;  // how often this actor makes a decision on a particular parcel (years)
      pGroup->m_valueStdDev = 0;
      pGroup->SetValueCount( this->m_pModel->GetActorValueCount() );

      for ( int i=0; i < this->m_pModel->GetActorValueCount(); i++ )
         pGroup->SetValue( i, 0 );

      AddActorGroup( pGroup );
      }

   ASSERT( GetActorGroupCount() >= 1 );

   ActorGroup *pGroup = GetActorGroup( 0 );

   Actor *pActor = CreateActorFromGroup( pGroup, false );
   pGroup->m_count = 1;  // how many instances of objects, class Actor, in this instance 

   // iterate over ACTIVE records and add all polys to this actor
   pActor->SetPolyCount( this->m_pModel->m_pIDULayer->GetPolygonCount( MapLayer::ALL ) );

   for ( MapLayer::Iterator i = this->m_pModel->m_pIDULayer->Begin( MapLayer::ALL ); i != this->m_pModel->m_pIDULayer->End( MapLayer::ALL ); i++ )
      pActor->SetPoly(i, i);
      
   return 1;
   }


int ActorManager::AddActorSelfInterestColumns( bool bForce /*=false*/)
   {
   int err = 0;
   int actorValueCount = this->m_pModel->GetActorValueCount();

   // Test that all columns corresponding to actor value fields are present in the DB.     
   for ( int i=0; i < actorValueCount; i++ )
      {
      METAGOAL *pInfo = this->m_pModel->GetActorValueMetagoalInfo( i );
      //EnvEvaluator* pInfo = this->m_pModel->GetActorValueModelInfo( i );
      CString field;
      field.Format( "ACTOR%s", (PCTSTR) pInfo->name );

      if ( field.GetLength() > 10 )
         field.Truncate( 10 ); // 10 is .dbf file col attrib name limit (ugly)
      field.Replace( ' ', '_' );

      int col = this->m_pModel->m_pIDULayer->GetFieldCol( field );
      if ( col < 0 )
         {
         if ( ! bForce )
            {
            CString msg;
            msg.Format( "Actor value field, %s, is missing - YOU MUST either 1) ADD this field "
               " to the IDU coverage, or 2) specify another actor initialization method "
               " in the project file, or 3) in the project file change the Decision Use flag "
               " of the model, %s, so that it is not used in actor self-interest.  ",
               (PCTSTR) field, (PCTSTR) pInfo->name); 

            Report::ErrorMsg( msg );
            throw new EnvException( "Fix problem before continuing" ); 
            }
         else 
            {
            col = this->m_pModel->m_pIDULayer->m_pDbTable->AddField( field, TYPE_FLOAT );
            //???? gpDoc->SetChanged( CHANGED_COVERAGE );
            }
         }

      this->m_pModel->m_colActorValues.Add( col );
      }
   
   return err;
   }



void ActorManager::CopyActorValuesToCells()
   {
   int actorCount = GetActorCount();
   int valueCount = this->m_pModel->GetActorValueCount();

   for ( int i=0; i < valueCount; i++ )
      this->m_pModel->m_pIDULayer->SetColNoData( this->m_pModel->m_colActorValues[ i ] );

   // iterate through the actors
   for ( int actor=0; actor < actorCount; actor++ )
      {
      Actor *pActor = GetActor( actor );

      int cellCount = pActor->GetPolyCount();
      for ( int i=0; i < cellCount; i++ )
         {
         int cell = pActor->GetPoly( i );
         
         for ( int j=0; j < valueCount; j++ )
            this->m_pModel->m_pIDULayer->SetData( cell, this->m_pModel->m_colActorValues[ j ], pActor->GetValue( j ) );         
         }  // end of:  for ( actor < actorCount )
      }
   }



void ActorManager::ResetActorIndices()
   {
   for ( int i=0; i < GetActorCount(); i++ )
      GetActor( i )->m_index = i;
   }

ActorGroup *ActorManager::GetActorGroupFromID( int id )
   {
   for ( int i=0; i < GetActorGroupCount(); i++ )
      if ( GetActorGroup( i )->m_id == id )
         return GetActorGroup( i );

   return NULL;
   }
   
int ActorManager::GetActorGroupIndexFromID( int id )
   {
   for ( int i=0; i < GetActorGroupCount(); i++ )
      if ( GetActorGroup( i )->m_id == id )
         return i;

   return -1;
   }


int ActorManager::GetManagedCellCount( void )
   {
   int actorCount = GetActorCount();
   int count = 0;

   for ( int i=0; i < actorCount; i++ )
      count += GetActor( i )->GetPolyCount();

   return count;
   }


int ActorManager::BuildAssociations( int types, MapLayer *pLayer, int colActor, int extra, bool clearPrevious /*=true*/ )
   {
   if ( types == AAT_NONE )
      return 0;

   if ( types & AAT_COMMONATTRIBUTE )
      {
      ASSERT( extra >= 0 );

      // note:  Assumes column attribute is an INT
      CUIntArray values;
      int count = pLayer->GetUniqueValues( extra, values );
      
      // create a map that has the attribute values mapped into an array offset
      CMap< int, int, int, int > attrMap;

      for ( int i=0; i < count; i++ )
         {
         attrMap.SetAt( values[ i ], i );
         
         // create an actor association for each of the unique attribute values
         m_associationArray.Add( new ActorAssociation( AAT_COMMONATTRIBUTE ) );
         }

      // iterate throught the attribute table, populating the actor associations
      for ( MapLayer::Iterator i=pLayer->Begin(); i != pLayer->End(); i++ )
         {
         int value;
         if ( pLayer->GetData( i, extra, value ) )
            {
            int index;
            if ( attrMap.Lookup( value, index ) )
               {
               ASSERT( index >= 0 );
               ASSERT( index < m_associationArray.GetSize() );

               // get the actor associated with this cell
               ASSERT( colActor >= 0 );
               int actor=-1;
               pLayer->GetData( i, colActor, actor );

               if ( actor >= 0 )
                  {
                  ActorAssociation *pAssoc = m_associationArray[ index ];
                  Actor *pActor = GetActor( actor );
                  pAssoc->m_actorArray.Add( pActor );
                  }
               }
            }
         }  // end of: for ( i < pLayerIterator->End();
      }  // end of: if ( type & AT_COMMONATTRIBUTE )

   if ( types & AAT_VALUECLUSTER )
      {
      // TODO:  Need code to identify clusters
      }

   return (int) m_associationArray.GetSize();
   }


void ActorManager::RemoveAllAssociations()
   {
   for ( int i=0; i < m_associationArray.GetSize(); i++ )
      delete m_associationArray[ i ];

   m_associationArray.RemoveAll();
   }



int ActorManager::LoadXml( LPCTSTR filename, bool isImporting, bool appendToExisting )
   {
   int count = 0;

   // have xml string, start parsing
   TiXmlDocument doc;
   bool ok = doc.LoadFile( filename );

   if ( ! ok )
      {
#ifndef NO_MFC
      CString msg;
      msg.Format("Error reading actor input file, %s", filename);
      AfxGetMainWnd()->MessageBox( doc.ErrorDesc(), msg );
#endif
      return 0;
      }

   // get first root <actors>
   TiXmlNode *pXmlRoot = doc.RootElement();

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


int ActorManager::LoadXml( TiXmlNode *pXmlActors, bool appendToExisting )
   {
   m_loadStatus = 0;
   bool reportError = true;

   if( pXmlActors == NULL )
      return -1;

   // get default actor group, file from parent node.
   LPCTSTR file = pXmlActors->ToElement()->Attribute( _T("file") );
   pXmlActors->ToElement()->Attribute( _T("default"), &m_defaultActorGroupIndex );

   if ( file != NULL && file[0] != NULL )
      return LoadXml( file, true, appendToExisting );

   TiXmlNode *pXmlActorNode = NULL;
   ActorGroup group;
   int i=0;

   while ( ( pXmlActorNode = pXmlActors->IterateChildren( pXmlActorNode ) ) != NULL )
      {
      i++;
      TiXmlElement *pXmlActor = pXmlActorNode->ToElement();
      pXmlActor->SetValue( "Unknown Actor" );
      TiXmlGetAttr( pXmlActor, _T("id"), group.m_id, -1, true );
      TiXmlGetAttr( pXmlActor, _T("name"), group.m_name, "", true );

      pXmlActor->SetValue( group.m_name );
      TiXmlGetAttr( pXmlActor, _T("originator"), group.m_originator, "", false );
      TiXmlGetAttr( pXmlActor, _T("decisionFreq"), group.m_decisionFrequency, 5, true );
      TiXmlGetAttr( pXmlActor, _T("query"), group.m_query, "", false );
      TiXmlGetAttr( pXmlActor, _T("valueStdDev"), group.m_valueStdDev, 0.5f, false );
      TiXmlGetAttr( pXmlActor, _T("landscapeFeedbackWt"), group.m_altruismWt, 0.5f, false );
      TiXmlGetAttr( pXmlActor, _T("actorValueWt"), group.m_selfInterestWt, 0.5f, false );
      TiXmlGetAttr( pXmlActor, _T("socialNetworkWt"), group.m_socialNetworkWt, 0.0f, false );
      TiXmlGetAttr( pXmlActor, _T("decisionElements"), group.m_decisionElements, -1, false );

      if ( group.m_decisionElements < 0 )
         group.m_decisionElements = m_pModel->m_decisionElements;

      ActorGroup *pGroup = new ActorGroup( group );

      int index = AddActorGroup( pGroup );
      // set up the actor values.  Note that actor values are only created for the those values
      // that have a coresponding model mapped to them.
      int actorValueCount = this->m_pModel->GetActorValueCount();
      pGroup->SetValueCount( actorValueCount );    // only models that are mapped to values are counted

      TiXmlNode *pGoalScoresNode = pXmlActorNode->FirstChild( "goal_scores" );
      ASSERT( pGoalScoresNode != NULL );

      TiXmlNode *pGoalScoreNode = NULL;
      int j=0;
      while ( pGoalScoreNode = pGoalScoresNode->IterateChildren( pGoalScoreNode ) )
         {
         if ( pGoalScoreNode->Type() != TiXmlNode::ELEMENT )
            continue;

         TiXmlElement *pGoalScore = pGoalScoreNode->ToElement();
         ASSERT( pGoalScore != NULL );

         // what is element name?
         CString goal;
         pGoalScore->SetValue( pGroup->m_name );
         TiXmlGetAttr( pGoalScore, "goal", goal, NULL, false );

         if ( ! goal.IsEmpty() )
            {
            float score;
            TiXmlGetAttr( pGoalScore, "score", score, 0.0f, false );

            int metagoalIndex = this->m_pModel->FindMetagoalIndex( goal );
            if ( metagoalIndex < 0 )
               {
               CString msg( _T("Unable to find goal when loading actor definition: ") );
               msg += goal;
               Report::LogWarning( msg );
               }
            else
               {
               int goalIndex = this->m_pModel->GetActorValueIndexFromMetagoalIndex( metagoalIndex );
               pGroup->SetValue( goalIndex, score );
               j++;
               }
            }
         }

      if ( j != actorValueCount && showMessage )
         {
         CString msg;
         msg.Format( "Mismatched value counts for actor %s (%i): XML input has %i, while %i models specify value maps", 
             (LPCTSTR) pGroup->m_name, pGroup->m_id, j, actorValueCount );
         Report::LogWarning( msg );
         showMessage = false;
         }

      //CreateActorsFromGroup( pGroup );  Note: Actors aren't made here!
      }

   int count = GetActorGroupCount();

   if ( m_defaultActorGroupIndex >= count )
      m_defaultActorGroupIndex = 0;

   if ( m_defaultActorGroupIndex < 0 && count > 0 )
      m_defaultActorGroupIndex = 0;

   // actors created and loaded, create associations
   //if ( m_actorAssociations )
   //   gpActorManager->BuildAssociations( this->m_actorAssociations, 0, true );
   //TODO:  Need to modify "Extras" argument
   return count;
   }


int ActorManager::SaveXml( LPCTSTR filename )
   {
   // open the file and write contents
   FILE *fp = NULL;
   fopen_s( &fp, filename, "wt" );
   if ( fp == NULL )
      {
#ifndef NO_MFC
      LONG s_err = GetLastError();
#else
      LONG s_err= errno;
#endif
      CString msg = "Failure to open " + CString(filename) + " for writing.  ";
      Report::SystemErrorMsg( s_err, msg );
      return -1;
      }

   //m_path = filename;
   bool useFileRef  = ( m_loadStatus == 1 ? true : false );
   int count = SaveXml( fp, true, useFileRef );
   fclose( fp );

   return count;
   }


int ActorManager::SaveXml( FILE *fp, bool includeHdr, bool useFileRef  )
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
      fprintf( fp, _T("<actors default='%i' file='%s' />\n"), m_defaultActorGroupIndex, (LPCTSTR) m_importPath );
      SaveXml( m_importPath );
      m_loadStatus = loadStatus;

      return GetActorCount();
      }

   fputs( "\n<!--\n", fp );
   fputs( "===================================================================================\n", fp );
   fputs( "                      A C T O R S \n", fp );
   fputs( "===================================================================================\n", fp );
   fputs( "id:           unique identifier for this actor group (required) \n", fp );
   fputs( "name:         name of this actor group (required)\n", fp );
   fputs( "originator:   name of person defining this group (optional) \n", fp );
   fputs( "decisionFreq: frequency (years) that the actor group makes decisions (required) \n", fp );
   fputs( "decisionElements: see definition under <settings> above \n", fp );
   fputs( "query:        query string for query-defined actors (required if actor initialization method=3, ignored otherwise) \n", fp );
   fputs( "valueStdDev:  standard deviation of actor values for this actor group (default=0 is not specified) \n", fp );
   fputs( "landscapeFeedbackWt: value between 0 and 1 indicating relative weighting of altruistic decision-making (default = 0.5 if not specified) \n", fp );
   fputs( "actorValueWt: value between 0 and 1 indicating relative weighting of self-interested decision-making (default = 0.5 if not specified) \n", fp );
   fputs( "utilityWt: value between 0 and 1 indicating relative weighting of utility (must define a utility function plugin!) default = 0.0 if not specified) \n", fp );
   fputs( "socialNetworkWt: value between 0 and 1 indicating relative weighting of social network influences (must define a social network!) default = 0.0 if not specified) \n", fp );
   fputs( "===================================================================================\n", fp );
   fputs( "-->\n\n", fp );  

   fprintf( fp, _T("<actors default='%i'>\n"), m_defaultActorGroupIndex );

   int actorValueCount = this->m_pModel->GetActorValueCount();

   for ( int i=0; i < GetActorGroupCount(); i++ )
      {
      ActorGroup *pGroup = GetActorGroup( i );
      
      fprintf( fp, "\n\t<actor id='%i' name='%s' \n\t\toriginator='%s' \n\t\tdecisionFreq='%i' \n\t\tdecisionElements='%i' \n\t\tquery='%s' \n\t\tvalueStdDev='%g' \n\t\tlandscapeFeedbackWt='%f' \n\t\tactorValueWt='%f' \n\t\tutilityWt='%f' \n\t\tsocialNetworkWt='%f' >",
         pGroup->m_id, (LPCTSTR) pGroup->m_name, (LPCTSTR) pGroup->m_originator, pGroup->m_decisionFrequency, pGroup->m_decisionElements,
         (PCTSTR) pGroup->m_query, pGroup->m_valueStdDev, pGroup->m_altruismWt, pGroup->m_selfInterestWt, pGroup->m_utilityWt, pGroup->m_socialNetworkWt );

      fputs( "\n\t\t<goal_scores>", fp );
      ASSERT( pGroup->GetValueCount() == actorValueCount );

      for ( int j=0; j < actorValueCount; j++ )
         {
         float score = pGroup->GetValue( j );

         //int modelIndex = this->m_pModel->GetModelIndexFromActorValueIndex( j );
         //EnvEvaluator *pInfo = this->m_pModel->GetEvaluatorInfo( modelIndex );
         METAGOAL *pInfo = this->m_pModel->GetActorValueMetagoalInfo( j );

         CString goalName;
         GetXmlStr( pInfo->name, goalName );
         fprintf( fp, "\n\t\t\t<goal_score goal='%s' score='%g' />", (LPCTSTR) goalName, score );
         }

      fputs( "\n\t\t</goal_scores>", fp );

      fputs( "\n\t</actor>\n", fp );
      }

   fputs( "\n</actors>\n", fp );
   return 1;
   }


// creates and initialize a single actor from a group.
// DOESN'T set any polygons

Actor *ActorManager::CreateActorFromGroup( ActorGroup *pGroup, bool random )
   {
   Actor *pActor = new Actor;
   AddActor( pActor );     // sets actor index
   pActor->m_pGroup = pGroup;

   // set local decision weights.  note:  These are not probablistically defined at this point!!!!
   //pActor->m_altruismWt     = pGroup->m_altruismWt;
   //pActor->m_selfInterestWt = pGroup->m_selfInterestWt;
   //pActor->m_utilityWt      = pGroup->m_utilityWt;

   // set up actor values
   int actorValueCount = this->m_pModel->GetActorValueCount();

   pActor->SetValueCount( actorValueCount );

   for ( int j=0; j < actorValueCount; j++ )
      {
      float mean = pGroup->GetValue( j );

      if ( random )
         {
         float std  = pGroup->m_valueStdDev;
         float wt = (float) m_rand.RandValue( mean, std );

         if ( wt > 3.0f ) 
            wt = 3.0f;
         else if ( wt < -3.0f ) 
            wt = -3.0f;
      
         pActor->SetValue( j, wt );
         }
      else
         pActor->SetValue( j, mean );
      }

   return pActor;
   }


Actor *ActorManager::GetActorFromIDU( int idu, int *index )
   {
   switch( this->m_actorInitMethod )
      {
      case AIM_IDU_WTS: 
      case AIM_IDU_GROUPS:
      case AIM_QUERY:      
      case AIM_RANDOM:     
         {
         for ( int i=0; i < this->GetActorCount(); i++ )
            {
            Actor *pActor = GetActor( i );
            
            for ( int j=0; j < pActor->GetPolyCount(); j++ )
               {
               if ( pActor->GetPoly( j ) == idu )
                  {
                  if ( index != NULL )
                     *index = i;

                  return pActor;
                  }
               }
            }

         return NULL;
         }
      
      case AIM_UNIFORM:
         {
         Actor *pActor = GetActor( 0 );
         return pActor;
         }

      default:
         ASSERT( 0 );
         break;
      }

   return NULL;
   }


