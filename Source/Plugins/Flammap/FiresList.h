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
//FiresList
//Stuart Brittain 12/9/2011
#pragma once
#include "Fire.h"
#include <Vdataobj.h>

#include <randgen\Randunif.hpp>

#include <list>

using namespace std; // for queue

typedef list< Fire > FireList;     // a list of individual fires

class FiresList {
public:
   FiresList();
   void Init(const char * InitFName);

   ~FiresList();

   int FireCnt();
   int AdjustYear(int year, double pRatio);
   
   FireList firesList;
   CString m_headerString;
private:

   int m_maxYear;

   bool DoesColExist( LPCTSTR filename, VDataObj &data, int &col, LPCTSTR field );


}; // class FiresList
