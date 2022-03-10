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


#include <EnvExtension.h>
#include <MapLayer.h>
#include <VData.h>
#include <afxtempl.h>
#include <PtrArray.h>

#define _EXPORT __declspec( dllexport )


//#include "resource.h"		// main symbols

struct SYNC_OUTCOME
{
public:
   VData targetValue;        // note:  null type matches any value
   float probability;  // decimal percent 
   
   SYNC_OUTCOME( void ) : probability( 1.0f ) { }
};


// a SyncOutcomeArray holds zero or more possible target outcomes for a single value of a soruce variable 
class SyncOutcomeArray : public CArray< SYNC_OUTCOME*, SYNC_OUTCOME* >
{
public:
   ~SyncOutcomeArray( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); RemoveAll(); }
};



// a MapEntry corresponds to the 'map' element in a sync_map entry
class MapElement
{
public:
   bool m_isRange;
   
   VData m_sourceValue;    // for single-valued elements
   VData m_srcMinValue;    // next two are for elements defined with ranges
   VData m_srcMaxValue;
   
   SyncOutcomeArray m_syncOutcomeArray;

   MapElement( void ) : m_isRange( false ) { }
   MapElement(MapElement&);
   bool IsWildcard( void ) { return ( m_sourceValue.type == NULL ) ? true : false; }

   void ApplyOutcome( EnvContext *pContext, int idu, int colTarget );
};

// SyncMap map values from a source column to corresponding value(s) in a target column
// It corresponds to a 'sync_map' element in an input file.
class SyncMap
{
public:
   CString m_name;

   enum METHOD { USE_DELTA=0, USE_MAP=1 };
   CString m_sourceCol;
   CString m_targetCol;
   int m_colSource;
   int m_colTarget;
   int m_init;
   bool m_inUse;

   METHOD m_method;

   MapElement *FindMapElement( VData &value );

   int AddMapElement( MapElement *pMap );  // { int index = m_mapElementArray.Add( pMap ); m_valueMap[ pMap->m_sourceValue.val.vUInt ] = pMap; return index; }
   PtrArray< MapElement > m_mapElementArray;   

   CMap< UINT, UINT, MapElement*, MapElement* > m_valueMap;    // maps MapElement source values to th assocated array of outcomes

   SyncMap() : m_colSource( -1 ), m_colTarget( -1 ), m_method( USE_DELTA ), m_init(0), m_inUse( true ) { }
   SyncMap(SyncMap &);
   ~SyncMap();   // deletes all SyncArrays in the map
};


// SyncProcess - this corresponds to one .envx file entry and one input file
// (or a <sync_map> entry in the input file
class SyncProcess
{
public:
   SyncProcess( int id ) : m_processID( id ) { }
   ~SyncProcess() { }

   bool LoadXml( LPCTSTR filename, MapLayer *pLayer );
   bool Run    ( EnvContext *pContext );   

public:
   int m_processID;            // model id specified in envx file
   CString m_filename;  // input file namew

   int GetSyncMapCount( void ) { return (int) m_syncMapArray.GetSize(); }
   SyncMap *GetSyncMap( int i ) { return m_syncMapArray.GetAt( i ); }
   
protected:
   PtrArray< SyncMap > m_syncMapArray;
};


// this is a wrapper class for interfacing with Envision's AP extension
class _EXPORT SyncProcessCollection : public EnvModelProcess
{
public:
   SyncProcessCollection() : EnvModelProcess() { }
   ~SyncProcessCollection() { }

   // overrides
   bool Init   ( EnvContext *pContext, LPCTSTR initStr /*xml input file*/ );
   bool InitRun( EnvContext *pContext, bool ) { return TRUE; }
   bool Run    ( EnvContext *pContext );   
  
   SyncProcess *GetSyncProcessFromID( int id, int *index=NULL );

protected:
   PtrArray< SyncProcess > m_syncProcessArray;

};