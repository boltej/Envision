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
#include "EnvLibs.h"

#define IB_USE_DOUBLE 1

#ifdef IB_USE_DOUBLE
typedef double SVTYPE;
#else
typedef float SVTYPE;
#endif

enum INT_METHOD { IM_UNDEF=0, IM_EULER=1, IM_RK4=2, IM_RKF=3, IM_OTHER=10 };

const float TIME_TOLERANCE = 0.00001f;



typedef void (*DERIVEFN)( double time, double timestep, int svCount, double *deriveArray, void *extra );

class LIBSAPI IntegrationBlock
{
public:
   IntegrationBlock( void );
   ~IntegrationBlock();

   void  Init( INT_METHOD method, int count, double timeStep );
   
   void  Clear( void );   // deletes arrays

   float Integrate( double time, double stopTime, DERIVEFN deriveFn, void *extra );
   bool  SetStateVar( SVTYPE *sv, int index ) { if (index >= m_svCount) return false; m_sv[index] = sv; return true; }

   double m_timeStep;                // days?
   double m_stopTime;
   float m_collectionInterval;
   double m_rkvTolerance;
   double m_rkvMaxTimeStep; 
   double m_rkvMinTimeStep;
   double m_rkvInitTimeStep;

protected:
   double  m_time;             // units???
   double  m_currentTimeStep;
   
   INT_METHOD m_method;

   double *m_k1;
   double *m_k2;
   double *m_k3;
   double *m_k4;
   double *m_k5;
   double *m_k6;
   //double *m_derive;
   SVTYPE  *m_svTemp;
   SVTYPE  **m_sv;
   int     m_svCount;    // number of state variables?
   
   template <typename T> T *AllocateArray( T *base, int size );      

   // specific integrators.  note:  These should update m_timeStep if they change it, but NOT m_time.
   bool IntEuler( DERIVEFN deriveFn, void *extra );
   bool IntRK4  ( DERIVEFN deriveFn, void *extra );
   bool IntRKF  ( DERIVEFN deriveFn, void *extra );
};


template <typename T>
T *IntegrationBlock::AllocateArray( T* base, int size )
   {
   if ( base != NULL )
      delete [] base;

   base = new T[ size ];

#ifndef NO_MFC
   if ( base == NULL )
      AfxMessageBox( "Out of memory allocating State Variable arrays" );
#endif

   return base;
   }

