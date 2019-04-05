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
#include "EnvProcessScheduler.h"
//
#include "EnvModel.h"

std::map<EnvProcess*, EnvProcessScheduler::ThreadData> EnvProcessScheduler::m_ThreadDataMap;
CCriticalSection EnvProcessScheduler::m_criticalSection;
int EnvProcessScheduler::m_maxThreadNum = omp_get_num_procs();

EnvProcessScheduler::EnvProcessScheduler(void)
{
}


EnvProcessScheduler::~EnvProcessScheduler(void)
{
}

void EnvProcessScheduler::init(PtrArray<EnvProcess>& apInfoArray)
{
	m_ThreadDataMap.clear();
	int count = (int) apInfoArray.GetSize();
	
	for ( INT_PTR i=0; i < count; i++ )
	{
		EnvProcess *pInfo = apInfoArray[ i ];
		ThreadData status;
		status.threadState = NOTSCHEDULED;
		status.id = pInfo;
		status.pEnvContext = NULL;
		m_ThreadDataMap.insert( std::make_pair(status.id, status) );
	}
}

void EnvProcessScheduler::signalThreadFinished( EnvProcess* id )
{
	CSingleLock lock( &m_criticalSection );
	lock.Lock();
	
	ThreadDataMapIterator itr = m_ThreadDataMap.find( id );
	if( itr != m_ThreadDataMap.end() )
	{
		itr->second.threadState = FINISHED;
	}
	lock.Unlock();
}

UINT EnvProcessScheduler::workerThreadProc( LPVOID Param )
{
	EnvContext* pEnvContext = (EnvContext*)Param;
	EnvProcess *pInfo = static_cast<EnvProcess*>(pEnvContext->pEnvExtension);
	pInfo->Run( pEnvContext );
	signalThreadFinished( pInfo );
	return 0;
}

void EnvProcessScheduler::getRunnableProcesses(PtrArray<EnvProcess>& apInfoArray, PtrArray<EnvProcess>& runnableProcesses )
   {
   CSingleLock lock(&m_criticalSection);
   lock.Lock();
   EnvProcess* pInfo = NULL;
   ThreadDataMapIterator itr;
   
   for (itr = m_ThreadDataMap.begin(); itr != m_ThreadDataMap.end(); itr++)
      {
      ThreadData status = itr->second;
      if (status.threadState == NOTSCHEDULED)
         {
         //get EnvProcess			
         int count = (int)apInfoArray.GetSize();
         for (int j = 0; j < count; j++)
            {
            EnvProcess *pInfoItr = apInfoArray[j];
            if (pInfoItr == status.id)
               {
               pInfo = pInfoItr;
               break;
               }
            }
   
         bool bAllDepsDone = true;
   
         // see if all its dependent are done.
         if (pInfo != NULL)
            {
            for (int j = 0; j < pInfo->m_dependencyCount; j++)
               {
               EnvProcess* pDependentInfo = pInfo->m_dependencyArray[j];
               ThreadDataMapIterator itr1 = m_ThreadDataMap.find(pDependentInfo);
               if (itr1 != m_ThreadDataMap.end())
                  {
                  if (itr1->second.threadState != FINISHED)
                     {
                     bAllDepsDone = false;
                     break;
                     }
                  }
               }
            if (bAllDepsDone)
               {
               runnableProcesses.Add(pInfo);
               }
            }
         }
      }
   
   lock.Unlock();
   
   return;
   }


int EnvProcessScheduler::countActiveThreads()
   {
   CSingleLock lock(&m_criticalSection);
   lock.Lock();

   int count = 0;

   ThreadDataMapIterator itr;
   for (itr = m_ThreadDataMap.begin(); itr != m_ThreadDataMap.end(); itr++)
      {
      if (itr->second.threadState == RUNNING)
         {
         count++;
         }
      }
   lock.Unlock();

   return count;
}
