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
#pragma hdrstop

#include "parseUtils.h"


void ParseUtils::TrimSpaces(char *pcStr) {

  int
    nOrigSz,
    nSrcNdx = 0,
    nTgtNdx = 0;

  nOrigSz = (int)strlen(pcStr);

  while(isspace((int)pcStr[nSrcNdx]))
    nSrcNdx++;

  int nOffset = nSrcNdx;
  
  for(int i=nSrcNdx; i<=nOrigSz; i++) {
    pcStr[i-nOffset] = pcStr[i];
  }
  
  nTgtNdx = (int)strlen(pcStr) - 1;
  while(isspace((int)pcStr[nTgtNdx])) {
    pcStr[nTgtNdx] = '\0';
    nTgtNdx--;
  }

}; // void TrimLeadSpaces(char *pcStr)