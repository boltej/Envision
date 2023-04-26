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
// LulcTree.h: interface for the LulcTreec class.
//
//////////////////////////////////////////////////////////////////////
#pragma once
#include "EnvLibs.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef NO_MFC
#include <afxtempl.h>
#endif

#include <vector>


class TiXmlNode;


//struct  LIBSAPI LULC_INFO
//   {
//   LU_STATUS status;
//
//   LULC_INFO( void ) : status( LUS_IGNORED ) { }
//   LULC_INFO( LULC_INFO &info ) : status( info.status ) { }
//   };

//class  LIBSAPI LulcInfoArray : public CArray< LULC_INFO*, LULC_INFO* >
//   {
//   public:
//      ~LulcInfoArray( void ) { for  ( int i=0; i < GetSize(); i++ ) delete GetAt( i );  RemoveAll(); }
//
//      void DeepCopy( LulcInfoArray &a ) { for ( int i=0; i < a.GetSize(); i++ ) Add( new LULC_INFO( *(a.GetAt( i )) ) ); }
//   };
//
//class LulcNode;
//class LIBSAPI LulcNodeArray : public CArray< LulcNode*, LulcNode* >
//   { };
//

class LIBSAPI LulcNode
   {
   public:
      // constructor
      LulcNode( void ) : m_pParentNode( nullptr ), m_id( -1 ), m_data(0), m_name(), m_isExpanded( false )  { }
      LulcNode(LulcNode& node) : m_id(node.m_id), m_name(node.m_name), m_isExpanded(node.m_isExpanded), m_data(node.m_data), m_pParentNode(nullptr) {}

      ~LulcNode() {} // if (m_pLulcInfo != nullptr) delete m_pLulcInfo;

      // methods
      int GetNodeLevel( void )  // return 0 for root node, 1 for first level, 2 for second, etc...
         { if ( m_pParentNode == nullptr ) return 0; else return m_pParentNode->GetNodeLevel() + 1; }

   public:
      int      m_id;        // id for this node
      CString  m_name;      // associated land use name
      bool     m_isExpanded;    //  for display purposes only
      //LULC_INFO* m_pLulcInfo = nullptr;
      INT_PTR   m_data; // application-specific data

      LulcNode     *m_pParentNode;     // pointer to parent
      std::vector<LulcNode*> m_childNodeArray;  // pointer to children
   };


class LIBSAPI LulcTree
   {
   public:
      LulcTree( void );
      LulcTree( LulcTree &tree ) : m_pRootNode( nullptr ), m_pCurrentNode( nullptr ) { Copy( tree ); }
     ~LulcTree( void ) { RemoveAll(); }

      void Copy( LulcTree &tree );
      void RemoveAll( void );
      void RemoveBranch( LulcNode *pRoot );

      LulcNode *GetRootNode( void ) { return m_pCurrentNode = m_pRootNode; }
      LulcNode *GetNextNode( void );   // for iterating through the tree.  returns nullptr when done iterating 

      LulcNode *FindNode( int level, int id );  // starts at root
 
      int GetNodeCount( int level );  // level 0=root, level 1=LULC_A, etc.
      LulcNode *FindNodeFromIndex( int level, int index );  // level 0=root, level 1=LULC_A, etc.

      void CreateRootNode();

   protected:
      LulcNode      *m_pCurrentNode;
      //std::vector<LULC_INFO*> m_lulcInfoArray;
      CStringArray   m_classNames;    // doesn't include root
      CStringArray   m_fieldNames;    // doesn't include root

      LulcNode *GetNextSibling( LulcNode* );
      LulcNode *GetNextNode   ( LulcNode* );

      LulcNode *FindNode( LulcNode*, int level, int id );

      void CopyNode( LulcNode *pNode, LulcNode *pSourceNode );

   public:
      CString   m_path;                // location of loaded file (empty if embedded in .envx file, populated if specified as file in envx)
      CString   m_importPath;          // if imported, path of  import file
      int       m_loadStatus;          // -1=undefined, 0=loaded from envx, 1=loaded from xml file
      bool      m_includeInSave;       // when saving project with external ref, update this file?

      LulcNode *m_pRootNode;           // root of the tree
      
      int       LoadLulcInfo( LPCTSTR filename );  // returns number of nodes in the first level
      //bool      GetLulcInfo( int level, int landUse, LULC_INFO &info );

      int       LoadXml( LPCTSTR filename, bool isImporting, bool appendToExisting );       // returns number of nodes in the first level
      int       LoadXml( TiXmlNode *pLulcTree, bool appendToExisting );
      int       SaveXml( LPCTSTR filename );
      int       SaveXml( FILE *fp, bool includeHdr, bool useFileRef=true );

      CString &GetFieldName( int level ) { return m_fieldNames[ level-1 ]; }  // Note: these are 1-based.  0=root, 1=LULC_A, etc.

      void AddFieldName( LPCTSTR name ) { m_fieldNames.Add( name ); }
      void AddClassName( LPCTSTR name ) { m_classNames.Add( name ); }

      int GetLevels() { ASSERT( m_fieldNames.GetSize() == m_classNames.GetSize() ); return (int) m_fieldNames.GetSize(); }    //0 for root only, 1 for LULC_A, 2 for LULC_B, 3 for LULC_C

      int FindLevelFromFieldname( LPCTSTR fieldName );   //1-based - returns 1 for first level, etc

      int GetMaxLULC_C() /*const*/;

   };


inline
int LulcTree::FindLevelFromFieldname( LPCTSTR fieldName )
   { 
   int level = 0;  
   
   while ( level < (int) m_fieldNames.GetSize() )
      {
      CString &_fieldName = GetFieldName( level+1 );     // this fn is one-based as well

      if ( _fieldName.CompareNoCase( fieldName ) == 0 )
         return level+1;

      level++;
      }

   return -1;
   }


inline
LulcNode *LulcTree::FindNode( int level, int id )
   {
   LulcNode *pNode = GetRootNode();

   return FindNode( pNode, level, id );
   }


inline 
LulcNode *LulcTree::FindNode( LulcNode *pNode, int level, int id )
   {
   // if this node matches, just return it
   if ( pNode->GetNodeLevel() == level && pNode->m_id == id )
      return pNode;

   // no match - do any children match?
   for ( UINT_PTR i=0; i < (UINT_PTR) pNode->m_childNodeArray.size(); i++ )
      {
      LulcNode *pChild = pNode->m_childNodeArray[ i ];
      LulcNode *pFound = FindNode( pChild, level, id );

      if ( pFound != nullptr )
         return pFound;
      }

   return nullptr;
   }

