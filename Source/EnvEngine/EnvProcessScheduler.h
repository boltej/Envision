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

#ifndef NO_MFC
#include "PtrArray.h"
#include "EnvExtension.h"
#include <map>
#include <omp.h>
#include <afxmt.h>



class EnvProcessScheduler
{
public:
	static int m_maxThreadNum;
	enum ThreadState
	{
		NOTSCHEDULED  =  0,
		RUNNING		  =  1,
		FINISHED      =  2
	};

	struct ThreadData
	{
		EnvProcess* id;
		int threadState;
		EnvContext* pEnvContext;
	};

public:	
	static void init(PtrArray<EnvProcess>& apInfoArray);
	static void signalThreadFinished( EnvProcess* id );
	static UINT workerThreadProc( LPVOID Param );	
	static void getRunnableProcesses(PtrArray<EnvProcess>& apInfoArray, PtrArray<EnvProcess>& runnableProcesses );
	static int countActiveThreads();

public:
	static std::map<EnvProcess*, ThreadData> m_ThreadDataMap;
	static CCriticalSection m_criticalSection;

public:
	EnvProcessScheduler(void);
	~EnvProcessScheduler(void);
};

typedef std::map<EnvProcess*, EnvProcessScheduler::ThreadData>::iterator ThreadDataMapIterator;
#endif
