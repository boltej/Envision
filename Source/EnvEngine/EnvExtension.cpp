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

#include "EnvExtension.h"
#include <Maplayer.h>

/*---------------------------------------------------------------------------------------------------------------------------------------
 * general idea:
 *    1) Create subclasses of EnvEvaluator, EnvAutoProcess for any models you wnat to implement
 *    2) Add any exposed variables by calling
 *         bool EnvExtension::DefineVar( bool input, int index, LPCTSTR name, void *pVar, TYPE type, MODEL_DISTR distType, VData paramLocation, VData paramScale, VData paramShape, LPCTSTR desc );
 *         for any exposed variables
 *    3) Override any of the methods defined in EnvExtension classes as needed
 *    4) Update dllmain.cpp
 *    5) Update *.def file
 *
 *
 *  Required Project Settings (<All Configurations>)
 *    1) General->Character Set: Not Set
 *    2) C/C++ ->General->Additional Include Directories: $(SolutionDir);$(SolutionDir)\libs;
 *    3) C/C++ ->Preprocessor->Preprocessor Definitions: WIN32;_WINDOWS;_AFXEXT;"__EXPORT__=__declspec( dllimport )";
 *    4) Linker->General->Additional Library Directories: $(SolutionDir)\$(ConfigurationName)
 *    5) Linker->Input->Additional Dependencies: libs.lib
 *
 *    Debug Configuration Only:  C/C++ ->Preprocessor->Preprocessor Definitions:  Add DEBUG;_DEBUG
 *    Release Configuration Only:  C/C++ ->Preprocessor->Preprocessor Definitions:  Add NDEBUG
 *---------------------------------------------------------------------------------------------------------------------------------------
 */

EnvExtension::EnvExtension() 
   : m_inputVars()
   , m_outputVars()
   , m_id(-1)
   , m_handle(-1)      // unique handle for this model/process, passed in the context
   , m_name()          // name of the model
   , m_path()          // path to the dll
   , m_initInfo()      // string passed to the model, specified in the project file
   , m_description()   // description, from ENVX file
   , m_imageURL()      // not currently used
   , m_use(false)      // use this model during the run?
   , m_hDLL((HINSTANCE)-1)
   , m_type(EET_UNDEFINED)         // Envision Extension type - see EnvContext.h for EET_xxx values; can be multiple types, or'd together
   { }

EnvExtension::EnvExtension( int inputCount, int outputCount )
   : m_inputVars()
   , m_outputVars()
   , m_id(-1)
   , m_handle(-1)      // unique handle for this model/process, passed in the context
   , m_name()          // name of the model
   , m_path()          // path to the dll
   , m_initInfo()      // string passed to the model, specified in the project file
   , m_description()   // description, from ENVX file
   , m_imageURL()      // not currently used
   , m_use(false)      // use this model during the run?
   , m_hDLL((HINSTANCE)-1)
   , m_type(EET_UNDEFINED)         // Envision Extension type - see EnvContext.h for EET_xxx values; can be multiple types, or'd together
   {
   if ( inputCount > 0 )
      m_inputVars.SetSize( inputCount );

   if ( outputCount > 0 )
      m_outputVars.SetSize( outputCount );
   }


EnvExtension::EnvExtension( EnvExtension &ext )
   : m_inputVars()
   , m_outputVars()
   , m_id(ext.m_id)
   , m_handle(ext.m_handle)      // unique handle for this model/process, passed in the context
   , m_name(ext.m_name)
   , m_path(ext.m_path)
   , m_initInfo(ext.m_initInfo)
   , m_description(ext.m_description)
   , m_imageURL(ext.m_imageURL)      // not currently used
   , m_use(ext.m_use)      // use this model during the run?
   , m_hDLL(ext.m_hDLL)
   , m_type(ext.m_type)         // Envision Extension type - see EnvContext.h for EET_xxx values; can be multiple types, or'd together
   {
   m_inputVars.Copy(ext.m_inputVars);
   m_outputVars.Copy(ext.m_outputVars);
   }



EnvExtension::~EnvExtension()
   {
   }


int EnvExtension::InputVar ( int id, MODEL_VAR** modelVar )
   {
   if ( m_inputVars.GetCount() > 0  )
      *modelVar = m_inputVars.GetData();
   else
      *modelVar = NULL;
   
   return (int) m_inputVars.GetCount();
   }



int EnvExtension::OutputVar( int id, MODEL_VAR** modelVar )
   {
   if ( m_outputVars.GetCount() > 0  )
      *modelVar = m_outputVars.GetData();
   else
      *modelVar = NULL;
   
   return (int) m_outputVars.GetCount();
   }



int EnvExtension::AddVar( bool input, LPCTSTR name, void *pVar, TYPE type, MODEL_DISTR distType, VData paramLocation, VData paramScale, VData paramShape, LPCTSTR description, int flags /*=0*/ )
   {
   MODEL_VAR modelVar( name, pVar, type, distType, paramLocation, paramScale, paramShape, description, flags );

   if ( input )
      return (int) m_inputVars.Add( modelVar );
   else
      return (int) m_outputVars.Add( modelVar );
   }


bool EnvExtension::DefineVar( bool input, int index, LPCTSTR name, void *pVar, TYPE type, MODEL_DISTR distType, VData paramLocation, VData paramScale, VData paramShape, LPCTSTR description )
   {
   if ( input )
      {
      if ( index >= m_inputVars.GetSize() )
         return false;

      MODEL_VAR &modelVar = m_inputVars.GetAt( index );
      modelVar.name = name;
      modelVar.pVar = pVar;
      modelVar.type = type;
      modelVar.distType = distType;
      modelVar.paramLocation = paramLocation;
      modelVar.paramScale = paramScale;
      modelVar.paramShape = paramShape;
      modelVar.description = description;
      modelVar.flags = 0;
      modelVar.extra = 0;
      return true;
      }
   else
      {
      if ( index >= m_outputVars.GetSize() )
         return false;

      MODEL_VAR &modelVar = m_outputVars.GetAt( index );
      modelVar.name = name;
      modelVar.pVar = pVar;
      modelVar.type = type;
      modelVar.distType = distType;
      modelVar.paramLocation = paramLocation;
      modelVar.paramScale = paramScale;
      modelVar.paramShape = paramShape;
      modelVar.description = description;
      return true;
      }
   }


INT_PTR EnvExtension::AddDelta( EnvContext *pContext, int idu, int col, VData newValue )
   {
   // valid AddDelta ptr?  (NULL means add delta clled during Init() )
   if ( pContext->ptrAddDelta != NULL )
      return pContext->ptrAddDelta( pContext->pEnvModel, idu, col, pContext->currentYear, newValue, pContext->handle );
   else
      {
      MapLayer *pMapLayer = (MapLayer*) pContext->pMapLayer;
      pMapLayer->SetData( idu, col, newValue );
      return -1;
      }
   }


bool EnvExtension::CheckCol( const MapLayer *pLayer, int &col, LPCTSTR label, TYPE type, int flags )
   {
   return ((MapLayer*) pLayer)->CheckCol( col, label, type, flags );
   }


/*
template <class T>
bool EnvExtension::_UpdateIDU(EnvContext *pContext, int idu, int col, T value, int flags)
   {
   if ((flags & (ADD_DELTA | SET_DATA)) == 0)
      {
      Report::LogWarning(_T("UpdateIDU() - invalid flag encountered - must be ADD_DELTA and/or SET_DATA"));
      return false;
      }

   bool retVal = true;
   if (flags & ADD_DELTA)
      {
      if (flags & FORCE_UPDATE)
         this->AddDelta(pContext, idu, col, value);

      else
         {
         int oldValue;
         pContext->pMapLayer->GetData(idu, col, oldValue);

         if (oldValue != value)
            this->AddDelta(pContext, idu, col, value);
         else
            retVal = false;
         }
      }

   if (flags & SET_DATA)
      {
      if (pContext->pMapLayer->m_readOnly)
         {
         if (flags & FORCE_UPDATE)
            {
            ((MapLayer*)pContext->pMapLayer)->m_readOnly = false;
            ((MapLayer*)pContext->pMapLayer)->SetData(idu, col, value);
            ((MapLayer*)pContext->pMapLayer)->m_readOnly = true;
            }
         else
            retVal = false;
         }
      else
         ((MapLayer*)pContext->pMapLayer)->SetData(idu, col, value);
      }

   return retVal;
   }

*/

bool EnvExtension::UpdateIDU( EnvContext *pContext, int idu, int col, VData &value, int flags )
   {
   if ( (flags & ( ADD_DELTA | SET_DATA ) ) == 0 )
      {
      Report::LogWarning( _T("UpdateIDU() - invalid flag encountered - must be ADD_DELTA and/or SET_DATA"));
      return false;
      }

   bool retVal = true;
   if ( flags & ADD_DELTA )
      {
      VData oldValue;
      pContext->pMapLayer->GetData( idu, col, oldValue );
      if ( oldValue != value )
         this->AddDelta( pContext, idu, col, value );
      else
         retVal = false;
      }

   if ( flags & SET_DATA )
      ((MapLayer*)(pContext->pMapLayer))->SetData( idu, col, value );

   return retVal;
   }

/*

bool EnvExtension::UpdateIDU( EnvContext *pContext, int idu, int col, float value, int flags )
   {
   if ( (flags & ( ADD_DELTA | SET_DATA ) ) == 0 )
      {
      Report::LogWarning( _T("UpdateIDU() - invalid flag encountered - must be ADD_DELTA and/or SET_DATA"));
      return false;
      }

   bool retVal = true;
   if ( flags & ADD_DELTA )
      {
      float oldValue;
      pContext->pMapLayer->GetData( idu, col, oldValue );
      if ( oldValue != value )
         this->AddDelta( pContext, idu, col, value );
      else
         retVal = false;
      }

   if ( flags & SET_DATA )
      ((MapLayer*)(pContext->pMapLayer))->SetData( idu, col, value );

   return retVal;
   }

bool EnvExtension::UpdateIDU( EnvContext *pContext, int idu, int col, int value, int flags )
   {
   if ( (flags & ( ADD_DELTA | SET_DATA ) ) == 0 )
      {
      Report::LogWarning( _T("UpdateIDU() - invalid flag encountered - must be ADD_DELTA and/or SET_DATA"));
      return false;
      }

   bool retVal = true;
   if ( flags & ADD_DELTA )
      {
      if ( flags & FORCE_UPDATE )
         this->AddDelta(pContext, idu, col, value);

      else
         {
         int oldValue;
         pContext->pMapLayer->GetData( idu, col, oldValue );

         if ( oldValue != value )
            this->AddDelta( pContext, idu, col, value );
         else
            retVal = false;
         }
      }

   if ( flags & SET_DATA )
      {
      if ( pContext->pMapLayer->m_readOnly )
         {
         if ( flags & FORCE_UPDATE )
            {
            ((MapLayer*) pContext->pMapLayer)->m_readOnly = false;
            ((MapLayer*) pContext->pMapLayer)->SetData(idu, col, value);
            ((MapLayer*) pContext->pMapLayer)->m_readOnly = true;
            }
         else
            retVal = false;
         }
      else
         ((MapLayer*) pContext->pMapLayer)->SetData(idu, col, value);
      }

   return retVal;
   }

bool EnvExtension::UpdateIDU( EnvContext *pContext, int idu, int col, double value, int flags )
   {
   if ( (flags & ( ADD_DELTA | SET_DATA ) ) == 0 )
      {
      Report::LogWarning( _T("UpdateIDU() - invalid flag encountered - must be ADD_DELTA and/or SET_DATA"));
      return false;
      }

   bool retVal = true;
   if ( flags & ADD_DELTA )
      {
      if (flags & FORCE_UPDATE)
         this->AddDelta(pContext, idu, col, value);
      
      else
         {
         double oldValue;
         pContext->pMapLayer->GetData( idu, col, oldValue );
         if ( oldValue != value )
            this->AddDelta( pContext, idu, col, VData( value ) );
         else
            retVal = false;
         }
      }

   if (flags & SET_DATA)
      {
      if (pContext->pMapLayer->m_readOnly)
         {
         if (flags & FORCE_UPDATE)
            {
            ((MapLayer*)pContext->pMapLayer)->m_readOnly = false;
            ((MapLayer*)pContext->pMapLayer)->SetData(idu, col, value);
            ((MapLayer*)pContext->pMapLayer)->m_readOnly = true;
            }
         else
            retVal = false;
         }
      else
         ((MapLayer*)pContext->pMapLayer)->SetData(idu, col, value);
      }

   return retVal;
   }


bool EnvExtension::UpdateIDU( EnvContext *pContext, int idu, int col, short value, int flags )
   {
   if ( (flags & ( ADD_DELTA | SET_DATA ) ) == 0 )
      {
      Report::LogWarning( _T("UpdateIDU() - invalid flag encountered - must be ADD_DELTA and/or SET_DATA"));
      return false;
      }

   bool retVal = true;
   if ( flags & ADD_DELTA )
      {
      int oldValue;
      pContext->pMapLayer->GetData( idu, col, oldValue );
      if ( oldValue != value )
         this->AddDelta( pContext, idu, col, value );
      else
         retVal = false;
      }

   if ( flags & SET_DATA )
      ((MapLayer*)(pContext->pMapLayer))->SetData( idu, col, value );

   return retVal;
   }


bool EnvExtension::UpdateIDU( EnvContext *pContext, int idu, int col, bool value, int flags )
   {
   if ( (flags & ( ADD_DELTA | SET_DATA ) ) == 0 )
      {
      Report::LogWarning( _T("UpdateIDU() - invalid flag encountered - must be ADD_DELTA and/or SET_DATA"));
      return false;
      }

   bool retVal = true;
   if ( flags & ADD_DELTA )
      {
      bool oldValue;
      pContext->pMapLayer->GetData( idu, col, oldValue );
      if ( oldValue != value )
         this->AddDelta( pContext, idu, col, value );
      else
         retVal = false;
      }

   if ( flags & SET_DATA )
      ((MapLayer*)(pContext->pMapLayer))->SetData( idu, col, value );

   return retVal;
   }

*/