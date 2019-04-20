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

#include <EnvEngine.h>
#include <EnvContext.h>
#include <Policy.h>
#include <Scenario.h>
#include <EnvConstants.h>

#include <DeltaArray.h>

//#include <VideoRecorder.h>




#ifdef _WINDOWS

#ifdef BUILD_ENVISION
#define ENVISIONAPI __declspec( dllexport )
#else 
#define ENVISIONAPI __declspec( dllimport )
#endif

#endif


typedef void(*ENVSETTEXT_PROC)(LPCTSTR text);
typedef void(*ENVREDRAWMAP_PROC)();


#ifdef __cplusplus
extern "C" {  // only need to export C interface if used by C++ source code
#endif

// map window interactions
ENVISIONAPI int EnvAddVideoRecorder( int type, LPCTSTR name, LPCTSTR path, int frameRate, int method, int extra );  // method: see VRMETHOD enum in videorecorder.h
ENVISIONAPI int EnvStartVideoCapture( int vrID );
ENVISIONAPI int EnvCaptureVideo( int vrID );
ENVISIONAPI int EnvEndVideoCapture( int vrID );

ENVISIONAPI void EnvSetLLMapText( LPCTSTR text );
ENVISIONAPI void EnvRedrawMap( void );
ENVISIONAPI int EnvRunQueryBuilderDlg( LPTSTR buffer, int bufferSize );


// Standard Path Information
//ENV_EXPORT int EnvStandardOutputFilename( LPTSTR filename, LPCTSTR pathAndFilename, int maxLength );
//ENV_EXPORT int EnvCleanFileName( LPTSTR filename );

#ifdef __cplusplus
}
#endif


