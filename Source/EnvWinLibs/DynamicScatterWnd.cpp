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
#include "DynamicScatterWnd.h"
#include <FDATAOBJ.H>

DynamicScatterWnd::DynamicScatterWnd(void)
: ScatterWnd()
, m_pCompositeDataObj( NULL)
{ }

DynamicScatterWnd::~DynamicScatterWnd(void)
   {
   for ( int i=0; i < m_dataObjArray.GetSize(); i++ )
      {
      //if ( m_dataObjArray[ i ] != NULL )      // assume plugins manage these
      //   delete m_dataObjArray[ i ];
      }

   if ( m_pCompositeDataObj != NULL )
      delete m_pCompositeDataObj;
   }


bool DynamicScatterWnd::CreateDataObjectFromComposites( DSP_FLAG  flag, bool makeDataLocal )
   {
   switch( flag )
      {
      case DSP_USELASTONLY:
         {
         //for ( int i=0; i < m_dataObjArray.GetSize()-1; i++ )
         //   ASSERT( m_dataObjArray[ i ] == NULL );
         if ( m_pCompositeDataObj != NULL )
            delete m_pCompositeDataObj;

         // NOTE - ASSUME source data is FDataObj - could be IDataObj
         //DataObj *pDO = m_dataObjArray [ m_dataObjArray.GetSize()-1 ]
         FDataObj *pLastDataObj = (FDataObj*) m_dataObjArray [ m_dataObjArray.GetSize()-1 ];
         ASSERT( pLastDataObj != NULL );
         m_pCompositeDataObj = new FDataObj( *pLastDataObj );   // HACK!!!!!

         this->SetDataObj( m_pCompositeDataObj, makeDataLocal );   // GraphWnd Method
         }
      return true;

      case DSP_USECOMPOSITETIME:      
      case DSP_USECOMPOSITESPACE:
         ASSERT(  0 );
      }

   return false;
   }


