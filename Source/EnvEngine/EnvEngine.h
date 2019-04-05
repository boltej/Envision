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
// EnvEngine.h : main header file for the EnvEngine DLL
//

#pragma once

#ifndef NO_MFC
#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#endif

#include "EnvAPI.h"
#include "EnvModel.h"

#if defined BUILD_ENGINE || defined USE_ENGINE

#include <MAP.h>
#include <PtrArray.h>


// CEnvEngineApp
// See EnvEngine.cpp for the implementation of this class
//

class EnvJob
   {
   public:
      EnvModel *m_pEnvModel;
      Map *m_pMap;

      EnvJob(EnvModel *pModel, Map *pMap)
         : m_pEnvModel(pModel)
         , m_pMap(pMap)
         { }

      ~EnvJob() {
         if (m_pEnvModel) delete m_pEnvModel;
         if (m_pMap) delete m_pMap;
         }
   };

class CEnvEngineApp : public CWinApp
{
public:
	CEnvEngineApp();

// Overrides
public:
	virtual BOOL InitInstance();

   int AddJob( EnvModel *pModel, Map *pMap )
      { 
      return (int) m_jobs.Add(new EnvJob(pModel, pMap));
      }

   void RemoveJobs()
      {
      m_jobs.RemoveAll();
      }

   DECLARE_MESSAGE_MAP()


protected:
   PtrArray<EnvJob> m_jobs;
  };



#endif // BUILD_ENGINE || USE_ENGINE

