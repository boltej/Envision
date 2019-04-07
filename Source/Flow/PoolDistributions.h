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

#include <afxtempl.h>
#include <tixml.h>
#include <PtrArray.h>

#include <QueryEngine.h>

using namespace std;

class HRU;

// a layer distribution consists of a pool index and a fraction
struct PoolDist
{
	int   poolIndex;
	float fraction;      // decimal percent
};

// layer consists of an array of layer distributions and an optional query 
struct Pool
{
	PtrArray< PoolDist > poolDistArray;
	Query *pPoolQuery;
   bool isRatio;

   Pool() : pPoolQuery(NULL), isRatio(false) { }
};

/*!
This class 
*/
class PoolDistributions
{
 public:
	 PoolDistributions();     // Default Constructor
	 ~PoolDistributions();    // Default Destructor

	 PtrArray< Pool > m_poolArray;

protected:
   CString m_poolDistStr;
   CString m_poolQueryStr;
   PtrArray< PoolDist > m_poolDistArray;
   QueryEngine *m_pPoolDistQE;

public:

	 // parses the layer distribution (index,fraction) into an array
	 // a layer's fractions of all the layer_distrubtions must total one
    int ParsePoolDistributions( PCTSTR ldString );

	 // parse the layers into an array
	 int ParsePools(TiXmlElement *pXmlElement, MapLayer *pIDULayer, LPCTSTR filename);

	 //determine which layer query matches the idu
	 int GetPoolIndex(int idu);

    int GetPoolCount() { return (int) m_poolArray.GetSize(); }
    Pool *GetPool( int i ) { return m_poolArray[ i ];}
};