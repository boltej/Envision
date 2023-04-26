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
// LulcTree.cpp: implementation of the LulcTreec class.
//
//////////////////////////////////////////////////////////////////////

#include "EnvLibs.h"
#pragma hdrstop

#include "LULCTREE.H"
#include "Report.h"
#include "tixml.h"
#include "PathManager.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
LulcTree::LulcTree( void )
: m_pRootNode( nullptr )
, m_pCurrentNode( nullptr )
, m_loadStatus( -1 )
, m_includeInSave( true )
   {
   CreateRootNode();
   m_pCurrentNode = m_pRootNode;
   }


void LulcTree::Copy( LulcTree &tree )
   {
   RemoveAll();

   m_pRootNode = new LulcNode( *(tree.m_pRootNode) );

   CopyNode( m_pRootNode, tree.m_pRootNode );  

   m_pCurrentNode = tree.m_pCurrentNode;
   m_path = tree.m_path;
   m_importPath = tree.m_importPath;
   m_loadStatus = tree.m_loadStatus;
   m_includeInSave = tree.m_includeInSave;
   //m_lulcInfoArray.DeepCopy( tree.m_lulcInfoArray );

   m_fieldNames.Copy( tree.m_fieldNames );
   }


void LulcTree::CopyNode( LulcNode *pNode, LulcNode *pSourceNode )
   {
   for ( int i=0; i < pSourceNode->m_childNodeArray.size(); i++ )
      {
      LulcNode *pChildNode = pSourceNode->m_childNodeArray[i];
      LulcNode *pNewNode = new LulcNode( *pChildNode );
      pNewNode->m_pParentNode = pNode;
      pNode->m_childNodeArray.push_back( pNewNode );

      CopyNode( pNewNode, pChildNode );
      }
   }


void LulcTree::CreateRootNode()
   {
   ASSERT( m_pRootNode == nullptr );
   m_pRootNode = new LulcNode;
   m_pRootNode->m_name = _T("Land Use/Land Cover classes");
   }
   

void LulcTree::RemoveAll( void )
   {
   if ( m_pRootNode != nullptr )
      RemoveBranch( m_pRootNode );

   m_pRootNode = nullptr;
   }


void LulcTree::RemoveBranch( LulcNode *pNode )
   {
   for ( int i=0; i < pNode->m_childNodeArray.size(); i++ )
      RemoveBranch( pNode->m_childNodeArray[ i ] );   // branches of the tree

   delete pNode;
   }


LulcNode *LulcTree::GetNextSibling( LulcNode *pNode )
   {
   LulcNode *pParent = pNode->m_pParentNode;

   if ( pParent == nullptr )
      return nullptr;  // no parent, no siblings

   for ( int i=0; i < pParent->m_childNodeArray.size(); i++ )
      {
      if ( pNode == pParent->m_childNodeArray[ i ] )     // found self
         {
         if ( i == pParent->m_childNodeArray.size()-1 )  // if this one is the last child node,
            return nullptr;    // then no more siblings 
         else
            return pParent->m_childNodeArray[ i+1 ];   // return next root node
         }
      }  // end of:  for ( i < m_rootNodeArray )

   ASSERT( 0 );
   return nullptr;
   }



LulcNode *LulcTree::GetNextNode( LulcNode *pNode )
   {
   // if a child node exists, use first one
   if ( pNode->m_childNodeArray.size() > 0 )
      return pNode->m_childNodeArray[ 0 ];

   // no children, are there more siblings?
   LulcNode *pSibling = GetNextSibling( pNode );

   if ( pSibling != nullptr )    // if a next sibling exists, return it;
      return pSibling;

   // no sibling exists, see is a parent sibling exists.
   LulcNode *pParent = pNode->m_pParentNode;
   if ( pParent == nullptr )
      return nullptr;

   pSibling = GetNextSibling( pParent );
   if ( pSibling != nullptr )
      return pSibling;
   
   // no parent sibling exists, try grandparent sibling
   pParent = pParent->m_pParentNode;
   if ( pParent == nullptr )
      return nullptr;

   pSibling = GetNextSibling( pParent );
   if ( pSibling != nullptr )
      return pSibling;
   
   // no grandparent sibling exists, try great grandparent sibling
   pParent = pParent->m_pParentNode;
   if ( pParent == nullptr )
      return nullptr;

   pSibling = GetNextSibling( pParent );
   if ( pSibling != nullptr )
      return pSibling;
   
   // no grandparent sibling exists, try great grandparent sibling
   pParent = pParent->m_pParentNode;
   if ( pParent == nullptr )
      return nullptr;

   pSibling = GetNextSibling( pParent );
   if ( pSibling != nullptr )
      return pSibling;

   // give up!!!
   return nullptr;
   }


LulcNode* LulcTree::GetNextNode( void )
   {
   return m_pCurrentNode = GetNextNode( m_pCurrentNode );
   }


int LulcTree::GetNodeCount( int level )
   {
   int count = 0;
   LulcNode *pNode = GetRootNode();

   while ( pNode != nullptr )
      {
      if ( level == pNode->GetNodeLevel() )
         count++;

      pNode = GetNextNode( pNode );
      }

   return count;
   }


LulcNode *LulcTree::FindNodeFromIndex( int level, int index )
   {
   int count = 0;
   LulcNode *pNode = GetRootNode();

   while ( pNode != nullptr )
      {
      if ( level == pNode->GetNodeLevel() )
         {
         if ( index == count )
            return pNode;
         else
            count++;
         }

      pNode = GetNextNode( pNode );
      }

   return nullptr;
   }


int LulcTree::LoadLulcInfo( LPCTSTR filename )
   {
   // look for extension.  If XML, switch to XML reader
   const char *ext = strrchr( filename, '.' );
   if ( ext != nullptr && _strnicmp( ext+1, "xml", 3 ) == 0 )
      {
      return LoadXml( filename, false, true );
      }

   RemoveAll();
   
   FILE *fp = nullptr;
   //fopen_s( &fp, filename, "rt" );
   int retVal = PathManager::FileOpen( filename, &fp, "rt" );
   if ( retVal <= 0 || fp == nullptr )
      {
      CString msg;
      msg.Format( "Unable to open LULC file %s", filename );

      Report::ErrorMsg( msg );
      return -1;
      }

   char buffer[ 256 ];
   int  currentLevel = 0;

   if ( m_pRootNode == nullptr )
      CreateRootNode();

   LulcNode *pLastNode = m_pRootNode;

   // get initial comment lines
   // assume the first 4 lines are comments
   for ( int i=0; i<4; i++ )
      fgets( buffer, 255, fp );

   while ( ! feof( fp ) )
      {
      fgets( buffer, 255, fp );

      // skip any bogus lines
      if ( buffer[ 0 ] != ',' && ! isalpha( buffer[ 0 ] ) )
         continue;

      LulcNode *pNode = new LulcNode;
      //LULC_INFO *pInfo = new LULC_INFO;
      //pNode->m_pLulcInfo = pInfo;
      pNode->m_pParentNode = nullptr;
      pNode->m_data = 0;
      //m_lulcInfoArray.push_back( pInfo ); // store the info locally

      int id = -1;
      //float landValue, soilValue, income, rentalRate, employment;
      int red, green, blue;
      char name[ 64 ];
      name[0] = 0;

      // read the name section
      char *p = buffer;
      int commaCount = 0;
      int newLevel = 0;

      while ( *p != 0 )
      {
         if ( isalpha(*p) )   // then read name
         {
            if ( name[0] != 0 )
            {
               CString msg( "Error parsing LULC information at: " );
               msg += buffer;
               Report::ErrorMsg( msg);
               fclose( fp );
               return -3;
            }
            newLevel=commaCount;
            int offSet=0;
            while( *p != ',' )
            {
               name[offSet] = *p;
               offSet++;
               p++;
            }
            name[offSet]=0;
         }
         if ( *p == ',' )
            commaCount++;
         p++;

         ASSERT( commaCount <= 3);
         if ( commaCount == 3 )
            break;
      }

      // store the name
      pNode->m_name = name;

      // scan the data section
      int count = sscanf_s( p, "%i,%i,%i,%i", &id, &red, &green, &blue );           // %f,%f,%f,%f,%f", &id, &red, &green, &blue, &landValue, &soilValue, &income, &rentalRate, &employment );
      if ( count != 4 )
         {
         CString msg( "Error parsing LULC information at: " );
         msg += buffer, 
         Report::ErrorMsg( msg);
         fclose( fp );
         return -3;
         }
         
      // load rest of the info
      pNode->m_id = id;

      // place the node at an appropriate location in the tree;
      int change = newLevel - currentLevel;
      currentLevel = newLevel;

      if ( newLevel == 0 ) // a root of the tree?
         {
         m_pRootNode->m_childNodeArray.push_back( pNode );
         pNode->m_pParentNode = m_pRootNode;
         }
      else
         {
         switch ( change ) 
            {
            case 1:     // going one level deeper, add node to last node
               pLastNode->m_childNodeArray.push_back( pNode );
               pNode->m_pParentNode = pLastNode;
               break;

            case 0:     // same as current level, add node to last nodes parent node array
               {
               LulcNode *pParentNode = pLastNode->m_pParentNode;
               pParentNode->m_childNodeArray.push_back( pNode );
               pNode->m_pParentNode = pParentNode;
               }
               break;

            case -1:    // one level higher in the heirarchy, add node to grandparent node array
               {
               LulcNode *pParentNode = pLastNode->m_pParentNode;
               pParentNode = pParentNode->m_pParentNode;      // get grandparent
               pParentNode->m_childNodeArray.push_back( pNode );
               pNode->m_pParentNode = pParentNode;
               }
               break;
         
            default:
               {
               Report::ErrorMsg( "Illegal heirarchy found loading LULC data" );
               fclose( fp );
               return -2;
               }
            }  // end of:  switch( change )
         }  // end of:  else( leadingTabCount != 0 )

      pLastNode = pNode;
      }  // end of:  while ( ! feof( fp ) )

   fclose( fp );

   m_path = filename;

   return (int) m_pRootNode->m_childNodeArray.size();
   }


int LulcTree::LoadXml( LPCTSTR _filename, bool isImporting, bool appendToExisting )
   {
   if ( appendToExisting == false )
      RemoveAll();

   if ( m_pRootNode == nullptr )
      CreateRootNode();

   CString filename;
   if ( PathManager::FindPath( _filename, filename ) < 0 ) //  return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 
      {
      CString msg;
      msg.Format( "LulcTree: Input file '%s' not found - no LULC classes will be loaded", _filename );
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
      msg.Format("Error reading LULC input file, %s", filename);
      AfxGetMainWnd()->MessageBox( doc.ErrorDesc(), msg );
#endif
      return -1;
      }

   // get first root <lulctree>
   TiXmlElement *pXmlRoot = doc.RootElement();

   int count = LoadXml( pXmlRoot, appendToExisting );
      
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


int LulcTree::LoadXml( TiXmlNode *pLulcTree, bool appendToExisting )
   {
   m_loadStatus = 0;

   if ( pLulcTree == nullptr )
      return -1;

   // file specified?.
   LPCTSTR file = pLulcTree->ToElement()->Attribute( _T("file") );
   
   if ( file != nullptr && file[0] != 0 )
      return LoadXml( file, true, appendToExisting );

   bool loadSuccess = true;

   // set up the root node
   if ( m_pRootNode == nullptr )
      CreateRootNode();

   LulcNode *pLastNode = m_pRootNode;
   
   // iterate through 
   TiXmlNode *pXmlClassNode = nullptr;
   while( pXmlClassNode = pLulcTree->IterateChildren( pXmlClassNode ) )
      {
      if ( pXmlClassNode->Type() != TiXmlNode::ELEMENT )
            continue;

      // get the classification
      TiXmlElement *pXmlClassElement = pXmlClassNode->ToElement();

      LPCTSTR name = pXmlClassElement->Attribute( "name" );
      LPCTSTR sLevel = pXmlClassElement->Attribute( "level" );
      LPCTSTR col  = pXmlClassElement->Attribute( _T("fieldname") );

      if ( name == nullptr || sLevel == nullptr )
         {
         CString msg( "Misformed <classification> element reading" );
         msg += m_path;

         Report::InfoMsg( msg );
         loadSuccess = false;
         continue;
         }

      if ( col == nullptr )
         col = name;

      int level = atoi( sLevel );
      this->AddClassName( name );
      this->AddFieldName( col );

      //CArray< int, int > seenIDArray;
      //seenIDArray.RemoveAll();

      // we are in a classification section now.  Iterate through the children to get the lulc classes for this level
      TiXmlNode *pXmlLulcNode = nullptr;
      while( pXmlLulcNode = pXmlClassNode->IterateChildren( pXmlLulcNode ) )
         {
         if ( pXmlLulcNode->Type() != TiXmlNode::ELEMENT )
            continue;

         TiXmlElement *pXmlLulcElement = pXmlLulcNode->ToElement();

         const char *name      = pXmlLulcElement->Attribute( "name" );
         const char *sID       = pXmlLulcElement->Attribute( "id" );
         const char *sParentID = pXmlLulcElement->Attribute( "parentID" );
         
         if ( name == nullptr || sID == nullptr || (level > 1 && sParentID == nullptr) )
            {
            CString msg( "Misformed <lulc> element reading" );
            msg += m_path;

            Report::WarningMsg( msg );
            loadSuccess = false;
            continue;
            }
         
         // have lulc element, add corresponding LulcNode
         LulcNode *pNode = new LulcNode;
         //LULC_INFO *pInfo = new LULC_INFO;
         //pNode->m_pLulcInfo= pInfo;
         pNode->m_pParentNode = nullptr;
         pNode->m_name = name;
         pNode->m_id = atoi( sID );

         // make sure this 'id' hasn't been used before
         LulcNode *pExistingNode = FindNode( level, pNode->m_id );
         if ( pExistingNode != nullptr )
            {
            CString msg;
            msg.Format(  "LulcTree: Duplicate 'id' attribute (%i) found for %s class %s was already used for class %s. This should be resolved before proceeeding!",
               pNode->m_id, col, name, (LPCTSTR) pExistingNode->m_name );
            Report::ErrorMsg( msg );
            }

         // get parent
         if ( level == 1 )
            {
            pNode->m_pParentNode = m_pRootNode;
            m_pRootNode->m_childNodeArray.push_back( pNode );
            }
         else
            {
            int parentID = atoi( sParentID );

            LulcNode *pParentNode = FindNode( level-1, parentID );
            if ( pParentNode == nullptr )
               {
#ifndef NO_MFC
               CString msg;
               msg.Format(  "Missing parent class for <lulc> level %i element %s:  Parent=%i", level, (LPCTSTR) name, parentID );
               Report::LogError( msg );
#endif
               loadSuccess = false;
               delete pNode;
               //delete pInfo;
               continue;
               }

            pNode->m_pParentNode = pParentNode;
            pParentNode->m_childNodeArray.push_back( pNode );
            }

         //m_lulcInfoArray.push_back( pInfo ); // store the info locally
         }
      }

   if ( loadSuccess == false )
      Report::WarningMsg( "There was a problem loading one or more lulc classes.  See the Message Tab for details." );

/*
   LulcNode *pNode = GetRootNode();

   while ( pNode != nullptr )
      {
      int level = pNode->GetNodeLevel();

      CString msg;
      for ( int i=0; i < level; i++ )
         msg += "..";

      msg += pNode->m_name;
      msg += "\n";

      TRACE( msg );
      pNode = GetNextNode( pNode );
      }  */

   return (int) m_pRootNode->m_childNodeArray.size();
   }


int LulcTree::SaveXml( LPCTSTR filename )
   {
   if ( m_includeInSave == false )
      return 0;

   // open the file and write contents
   FILE *fp;
   fopen_s( &fp, filename, "wt" );
   if ( fp == nullptr )
      {
#ifndef NO_MFC
	//not sure if this does anything; may just be called to clear error stack
      LONG s_err = GetLastError();
#endif
      CString msg = "Failure to open " + CString(filename) + " for writing.  ";
      Report::ErrorMsg( msg );
      return -1;
      }

   bool useFileRef = ( m_loadStatus == 1 ? true : false );
   int count = SaveXml( fp, true, useFileRef );
   fclose( fp );
  
   return count;
   }


int LulcTree::SaveXml( FILE *fp, bool includeHdr, bool useFileRef /*=true*/ )
   {
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
      fprintf( fp, _T("<lulcTree file='%s' />\n"), (LPCTSTR) m_importPath );
      SaveXml( m_importPath );
      m_loadStatus = loadStatus;

      return 1;
      }

   // not a file ref, so write out tree

   fputs( "<lulcTree>\n", fp );

   int levels = GetLevels();  
   for ( int level=1; level <= levels; level++ )
      {
      fprintf( fp, "\n\t<classification name='%s' col='%s' level='%i'>", 
               (LPCTSTR) m_classNames[ level-1 ], (LPCTSTR) m_fieldNames[ level-1 ], level );

      int nodeCount = GetNodeCount( level );
      for ( int i=0; i < nodeCount; i++ )
         {
         LulcNode *pNode = FindNodeFromIndex( level, i );
         ASSERT( pNode );

         if ( level == 1 )
            fprintf( fp, "\n\t\t<lulc id='%i' name='%s' />", pNode->m_id, (LPCTSTR) pNode->m_name );
         else
            fprintf( fp, "\n\t\t<lulc id='%i' parentID='%i' name='%s' />", pNode->m_id, pNode->m_pParentNode->m_id, (LPCTSTR) pNode->m_name );
         }

      fputs( "\n\t</classification>\n", fp );
      }

   fputs( "\n</lulcTree>\n", fp );
   
   return 1;
   }

//
//bool LulcTree::GetLulcInfo( int level, int landUse, LULC_INFO &info )
//   {
//   LulcNode *pNode = FindNode( level, landUse );
//
//   if ( pNode != nullptr )
//      {
//      info.status = pNode->m_pLulcInfo->status);
//      return true;
//      }
//
//   else
//      return false;
//   }


int LulcTree::GetMaxLULC_C() /*const*/
{
   int mx=-1;
   LulcNode * nd = GetRootNode();
   while (nullptr != nd) {
      if (3 == nd->GetNodeLevel()) // How do we *really* know this is LULC_C?   
         mx = (nd->m_id > mx) ? nd->m_id : mx;
      nd = GetNextNode();
   }
   return mx;
}
