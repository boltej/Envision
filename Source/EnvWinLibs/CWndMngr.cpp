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
#include "EnvWinLibs.h"
#pragma hdrstop


#include <cwndmngr.h>



//------------------- CWndManager methods --------------------//

//-- constructor --//
CWndManager::CWndManager( void ) 
  : CArray< CWnd*, CWnd*>(),
    pActive( NULL )
  {
  deleteOnDelete = TRUE;
  }


CWndManager::~CWndManager()
   {
   for ( int i=0; i < GetSize(); i++ )
      Remove( GetAt( i ) );

   RemoveAll();
   }


void CWndManager::Add( CWnd *pWnd ) 
   {
   CArray<CWnd*, CWnd*>::Add( pWnd );
   pActive = pWnd;
   }
   
   
void CWndManager::Remove(CWnd *pWnd )
   {
   int i;
   bool found = false;

   for ( i=0; i < GetSize(); i++ )
      {
      if ( GetAt( i ) == pWnd )
         {
         found = true;
         break;
         }
      }

   if ( found )
      {
      RemoveAt( i );

      if ( deleteOnDelete )
         delete pWnd;
      }

   if ( pWnd == pActive )
      {
      if ( GetSize() > 0 )
         pActive = GetAt( 0 );
      else
         pActive = NULL;
      }
   }
         

