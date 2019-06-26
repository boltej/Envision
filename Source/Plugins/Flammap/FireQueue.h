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
/* FireQueue.h
** 
** This class is designed to do the following:
**
** - Read a file of possible fires, one line at a time
** - Create an object of class Fire for the line
** - Queue the fire if it is to be used during the run
** - Provide access to the parameters associated with each 
**   fire
** - Allow the fire at the head of the queue to be removed
**
** 2011.03.10 - tjs - starting implementation.
** 2011.03.14 - tjs - dropping in a version of this developed
**   and tested outside of FlamMapAP
*/

#pragma once
#include "Fire.h"
#include <queue>

using namespace std; // for queue

class FireQueue {
private:
	queue<Fire *> Fires;

public:
	FireQueue(CString InitFName) {Init(InitFName.GetString());}
	FireQueue(const char * InitFName) {Init(InitFName);}
	void Init(const char * InitFName);

	~FireQueue();

	int GetHdYr();
	int GetHdJulDate();
	int GetHdWindSpd();
	int GetHdWindAzmth();
	double GetHdBurnProb();
	int GetHdBurnPeriod();
	bool GetHdDoRun();
	int FireCnt();
	void DeleteHd(){
		if(!Fires.empty()) {
			delete Fires.front();
			Fires.pop();
		}
	} // DeleteHd()

}; // class FireQueue
