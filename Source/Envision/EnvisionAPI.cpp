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
#include "StdAfx.h"
#include "EnvisionAPI.h"
#include <EnvModel.h>
#include <Maplayer.h>
#include <Policy.h>
#include <Scenario.h>
#include <Actor.h>
#include "MapPanel.h"
#include "EnvView.h"
//#include "PathManager.h"

#include "PolQueryDlg.h"
//#include <VideoRecorder.h>

extern MapPanel      *gpMapPanel; 
extern EnvModel      *gpModel;
extern MapLayer      *gpCellLayer;
extern PolicyManager *gpPolicyManager;
extern ActorManager  *gpActorManager;
extern ScenarioManager *gpScenarioManager;
extern CEnvView      *gpView;

ENVSETTEXT_PROC envSetLLMapTextProc = nullptr;
ENVREDRAWMAP_PROC envRedrawMapProc = nullptr;

DELTA & GetDelta( DeltaArray *da, INT_PTR index )  { return da->GetAt( index ); }

int ApplyDeltaArray( void ) { return gpModel->ApplyDeltaArray( gpCellLayer ); }

int GetEvaluatorCount() { return gpModel->GetEvaluatorCount(); }
EnvEvaluator* GetEvaluatorInfo( int i ) { return gpModel->GetEvaluatorInfo( i ); }
EnvEvaluator* FindModelInfo( LPCTSTR name ) { return gpModel->FindEvaluatorInfo( name ); }

int GetAutoProcessCount() { return gpModel->GetModelProcessCount(); }
EnvModelProcess* GetAutoProcessInfo( int i ) { return gpModel->GetModelProcessInfo( i ); }

int ChangeIDUActor( EnvContext *pContext, int idu, int groupID, bool randomize ) { return gpModel->ChangeIDUActor( pContext, idu, groupID, randomize ); }

// policy related methods
int     GetPolicyCount()          { return gpPolicyManager->GetPolicyCount(); }
int     GetUsedPolicyCount()      { return gpPolicyManager->GetUsedPolicyCount(); }
Policy *GetPolicy( int i )        { return gpPolicyManager->GetPolicy( i ); }
Policy *GetPolicyFromID( int id ) { return gpPolicyManager->GetPolicyFromID( id ); }


// Scenario related methods
int       EnvGetScenarioCount()            { return gpScenarioManager->GetCount(); }
Scenario *EnvGetScenario( int i )          { return gpScenarioManager->GetScenario( i ); }
Scenario *EnvGetScenarioFromName( LPCTSTR name, int *index )
   {
   Scenario *pScenario = gpScenarioManager->GetScenario( name );
   
   if ( index != NULL && pScenario != NULL )
      {
      *index = gpScenarioManager->GetScenarioIndex( pScenario );
      }

   return pScenario;
   }

//int EnvAddVideoRecorder( int type, LPCTSTR name, LPCTSTR path, int frameRate, int method, int extra )
//   {
//   VideoRecorder *pVR = new VideoRecorder( path, gpMapPanel, frameRate );
//
//   pVR->SetType( type );
//   pVR->SetName( name );
//   //pVR->SetPath( path );
//   //pVR->SetFrameRate( frameRate );
//   pVR->SetCaptureMethod( (VRMETHOD) method );
//   pVR->SetExtra( extra );
//
//   int index = gpView->AddVideoRecorder( pVR );
//   return index;   
//   }
//
//int EnvStartVideoCapture( int vrID )
//   {
//   VideoRecorder *pVR = gpView->GetVideoRecorder( vrID );
//
//   if ( pVR != NULL )
//      {
//      pVR->StartCapture();
//      return 1;
//      }
//
//   return 0;
//   }
//
//int EnvCaptureVideo( int vrID )
//   {
//   VideoRecorder *pVR = gpView->GetVideoRecorder( vrID );
//
//   if ( pVR != NULL )
//      {
//      if ( pVR->m_pCaptureWnd == gpMapPanel && pVR->m_extra > 0 )  // is there a column defined?  If so, make sure it is active
//         {
//         // get existing active layer
//         int activeCol = gpCellLayer->GetActiveField();
//
//         int col = (int) pVR->m_extra;
//
//         if ( col != activeCol )
//            {
//            gpMapPanel->m_mapFrame.SetActiveField( col, true );
//            gpMapPanel->RedrawWindow();
//            }
//
//         pVR->CaptureFrame();
//         if ( col != activeCol )
//            {
//            gpMapPanel->m_mapFrame.SetActiveField( activeCol, true );
//            gpMapPanel->RedrawWindow();
//            }
//         }
//      }
//   
//   return 1;
//   }
//
//int EnvEndVideoCapture( int vrID )
//   {
//   VideoRecorder *pVR = gpView->GetVideoRecorder( vrID );
//   
//   if ( pVR )
//      {
//      pVR->EndCapture();
//      return 1;
//      }
//
//   return 0;
//   }

void EnvSetLLMapText( LPCTSTR text )
   {
   if (envSetLLMapTextProc != nullptr)
      envSetLLMapTextProc(text); // gpMapPanel->UpdateLLText(text);
   }


void EnvRedrawMap(void)
   {
   if (envRedrawMapProc != nullptr)
      envRedrawMapProc(); 
   }


int EnvRunQueryBuilderDlg( LPTSTR qbuffer, int bufferSize )
   {
   PolQueryDlg dlg( gpCellLayer, NULL );

   dlg.m_queryString = qbuffer;

   int retVal = (int) dlg.DoModal();

   if ( retVal == IDOK )
      lstrcpyn( qbuffer, dlg.m_queryString, bufferSize );

   return retVal;
   }


//int EnvStandardizeOutputFilename( LPTSTR filename, LPTSTR pathAndFilename, int maxLength )
//   {
//   EnvCleanFileName( filename );
//    
//   CString path = PathManager::GetPath( PM_OUTPUT_DIR );  // {ProjectDir}\Outputs\CurrentScenarioName\
//   path += filename;
//   _tcsncpy( pathAndFilename, (LPCTSTR) path, maxLength );
//
//   return path.GetLength();
//   }


//int EnvCleanFileName (LPTSTR filename)
//   {
//   if ( filename == NULL )
//      return 0;
//
//   TCHAR *ptr = filename;
//
//   while ( *ptr != NULL )
//      {
//      switch( *ptr )
//         {
//         case ' ':   *ptr = '_';    break;  // blanks
//         case '\\':  *ptr = '_';    break;  // illegal filename characters 
//         case '/':   *ptr = '_';    break;
//         case ':':   *ptr = '_';    break;
//         case '*':   *ptr = '_';    break;
//         case '"':   *ptr = '_';    break;
//         case '?':   *ptr = '_';    break;
//         case '<':   *ptr = '_';    break;
//         case '>':   *ptr = '_';    break;
//         case '|':   *ptr = '_';    break;
//         }
//
//      ptr++;
//      }
//
//
//   return (int) _tcslen( filename );
//   }
      

