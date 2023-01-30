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

#include <EnvExtension.h>

#include <Vdataobj.h>
#include <PtrArray.h>
#include <vector>
#include <map>
//#include <misc.h>
//#include <FDATAOBJ.H>
//
//#include <randgen\Randunif.hpp>
//
//#include <QueryEngine.h>
//#include <Query.h>

using namespace std;

typedef struct {
	int iduIndex; 
} IDUIndexKeyClass;

struct IDUIndexClassComp {
	bool operator() (const IDUIndexKeyClass& lhs, const IDUIndexKeyClass& rhs) const
	{
		return lhs.iduIndex < rhs.iduIndex;
	}
};


class MapSequence
{
public:
   MapSequence( void ) : m_pSourceData( NULL ), m_applyDate( -1 ), m_colSource( -1 ), m_colTarget( -1 ), m_use( true ) { }
   ~MapSequence( void ) { if ( m_pSourceData != NULL ) delete m_pSourceData; }

   // for all sources
   CString m_sourceTableName;
   CString m_sourceTablePath;
   VDataObj  *m_pSourceData;

   bool m_use;
   

   // for map layer-based sources
   CString m_sourceField;       // int sourceLayer
   CString m_targetField;       // in IDU layer
   int     m_applyDate;       // year in which to apply this layers information

   int m_colSource;
   int m_colTarget;

   bool Init( MapLayer *pLayer );
   bool IsLayerType( void );
};


class MapSequenceProcess : public EnvAutoProcess
{
public:
   MapSequenceProcess( void ): m_colIDUIndex(-1){};
   ~MapSequenceProcess( void ) { } // { if ( m_pQueryEngine ) delete m_pQueryEngine; }

   BOOL Init   ( EnvContext *pContext, LPCTSTR initStr );
   BOOL InitRun( EnvContext *pContext, bool useInitialSeed );
   BOOL Run    ( EnvContext *pContext ) { return Run( pContext, true ); }
   BOOL Setup  ( EnvContext *pContext, HWND hWnd )          { return FALSE; }
	
protected:
   PtrArray< MapSequence > m_seqArray;

   BOOL Run( EnvContext *pContext, bool AddDelta );

   bool LoadXml( LPCTSTR filename, MapLayer *pLayer );
	int IDUIndexLookupMap(EnvContext *pContext);

	int	m_colIDUIndex;		// column in IDU layer for IDU_INDEX
   IDUIndexKeyClass m_iduIndexInsertKey;
   IDUIndexKeyClass m_iduIndexLookupKey;
	map<IDUIndexKeyClass, std::vector<int>, IDUIndexClassComp> m_IDUIndexMap;  //used for idu index, finding IDUs
};
