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
#include "EnvLibs.h"
#pragma hdrstop

#include "IntegrationBlock.h"
#include "Report.h"

#include <math.h>
#include <omp.h>


////////////////////////////////////////////////////////
// I N T E G R A T I O N   B L O C K 
////////////////////////////////////////////////////////

IntegrationBlock::IntegrationBlock( void )
 : m_method( IM_RK4 )
 , m_time( 0.0f )
 , m_stopTime( 0.0f )
 , m_timeStep( 1.0f )                // days?
 , m_currentTimeStep( 1.0f )
 , m_collectionInterval( 1.0f )
 //, m_useVariableStep( true )
 , m_rkvTolerance( 0.001f )
 , m_rkvMaxTimeStep(5.0f)
 , m_rkvMinTimeStep(0.001f)
 , m_k1( NULL )
 , m_k2( NULL )
 , m_k3( NULL )
 , m_k4( NULL )
 , m_k5( NULL )
 , m_k6( NULL )
 //, m_derive( NULL )
 , m_svTemp( NULL )
 , m_sv( NULL )
 , m_svCount( -1 )
   { }

IntegrationBlock::~IntegrationBlock( void )
   {
   Clear();
   }


float IntegrationBlock::Integrate( double time, double stopTime,  DERIVEFN deriveFn, void *extra )
   {
   m_time = time;
   m_stopTime = stopTime;
   m_currentTimeStep = m_timeStep;

   if ( m_time > m_stopTime )
      return false;   // not quite right, should make sure that the block has been integrated to the stop time

   while ( m_time < ( m_stopTime - TIME_TOLERANCE ) )
      {
      switch( m_method )
         {
         case IM_EULER:
            IntEuler( deriveFn, extra );         
            break;

         case IM_RK4:
            IntRK4( deriveFn, extra );
            break;

         case IM_RKF:
            IntRKF( deriveFn, extra );
            CString msg;
            msg.Format("RKF: timeStep: %6.4f",m_timeStep);
            Report::StatusMsg(msg);
            break;
         }  // end of: switch ( m_method )

      m_time += m_currentTimeStep;

      // are we close to the end?
      float timeRemaining = float( m_stopTime - m_time );

      if ( timeRemaining  < ( m_timeStep + TIME_TOLERANCE ) )
         m_currentTimeStep = m_stopTime - m_time;
      }  // end of: while ( m_time < m_stopTime )   // note - doesn't guarantee alignment with stop time
   
   return (float) m_time;
   }


void IntegrationBlock::Init( INT_METHOD method, int count, double timeStep )
   {

   m_timeStep=timeStep;
   m_method = method;

   if ( m_svCount > 0 )
      Clear();

   m_svCount = count;

   if ( method == IM_OTHER || method== IM_UNDEF)
      m_svCount = 0;

   switch( m_method )
      {
      case IM_EULER:
         m_sv = AllocateArray( m_sv, m_svCount );
         m_k1 = AllocateArray( m_k1, m_svCount );
         m_svTemp = AllocateArray( m_svTemp, m_svCount );
         break;

      case IM_RK4:
         m_sv = AllocateArray( m_sv, m_svCount );
         m_k1 = AllocateArray( m_k1, m_svCount );
         m_k2 = AllocateArray( m_k2, m_svCount );
         m_k3 = AllocateArray( m_k3, m_svCount );
         m_k4 = AllocateArray( m_k4, m_svCount );
         m_svTemp = AllocateArray( m_svTemp, m_svCount );
         break;

      case IM_RKF:
         m_sv = AllocateArray( m_sv, m_svCount );
         m_k1 = AllocateArray( m_k1, m_svCount );
         m_k2 = AllocateArray( m_k2, m_svCount );
         m_k3 = AllocateArray( m_k3, m_svCount );
         m_k4 = AllocateArray( m_k4, m_svCount );
         m_k5 = AllocateArray( m_k5, m_svCount );
         m_k6 = AllocateArray( m_k6, m_svCount );
         m_svTemp = AllocateArray( m_svTemp, m_svCount );
         break;
      }
   }


void IntegrationBlock::Clear( void )
   {
   if ( m_k1     != NULL ) delete [] m_k1;
   if ( m_k2     != NULL ) delete [] m_k2;
   if ( m_k3     != NULL ) delete [] m_k3;
   if ( m_k4     != NULL ) delete [] m_k4;
   if ( m_k5     != NULL ) delete [] m_k5;
   if ( m_k6     != NULL ) delete [] m_k6;
   if ( m_sv     != NULL ) delete [] m_sv;
   if ( m_svTemp != NULL ) delete [] m_svTemp;

   m_k1 = NULL;
   m_k2 = NULL;
   m_k3 = NULL;
   m_k4 = NULL;
   m_k5 = NULL;
   m_k6 = NULL;
   m_sv = NULL;
   m_svTemp = NULL;

   m_svCount = -1;
   }


bool IntegrationBlock::IntEuler( DERIVEFN deriveFn, void *extra )
   {
   float currentTime = (float) m_time;

   //-- get derivative values from object at current time --//  
   deriveFn( currentTime, m_timeStep, m_svCount, m_k1, extra );
 
   // #pragma omp parallel for   
   for (int i=0; i < m_svCount; i++ )
      {     
      *(m_sv[i]) = float( *m_sv[i] + m_k1[i] * m_timeStep ); 

 //     if (fabs(*(m_sv[i])) < 1E-20f )
  //       *(m_sv[i]) = 1E-20f;
      }
   return true;
   }


bool IntegrationBlock::IntRK4( DERIVEFN deriveFn, void *extra )
   {
   double currentTime = m_time;

   //-- Step 1: get derivative values from object at current time --//  
   deriveFn( currentTime, m_timeStep, m_svCount, m_k1, extra );
 
#pragma omp parallel for   
   for (int i=0; i < m_svCount; i++ )
      m_svTemp[i] = *(m_sv[i]);     // remember original values

   //-- Step 2: get derivative values from object at current time + 1/2--//
   //#pragma omp parallel for
   for (int i=0; i < m_svCount; i++ )
      {
      *(m_sv[i]) = SVTYPE( m_svTemp[i] + m_k1[i] * m_timeStep / 2 );

      //if (fabs(*(m_sv[i])) < 1E-30f)
     //   *(m_sv[i]) = 0.0f;   
      }  

   currentTime = m_time + m_timeStep/2;
   deriveFn( currentTime, m_timeStep, m_svCount, m_k2, extra );

   // Step 3: halfway across with improved values
   //#pragma omp parallel for
   for (int i=0; i < m_svCount; i++)
      {
      *(m_sv[i]) = SVTYPE( m_svTemp[i] + ( m_k2[i] * m_timeStep / 2 ) );
      
    //  if (fabs(*(m_sv[i])) < 1E-30f)
  //      *(m_sv[i]) = 0.0f;
      }

   deriveFn( currentTime, m_timeStep, m_svCount, m_k3, extra );

   // Step 4: all the way across
   //#pragma omp parallel for
   for (int i=0; i < m_svCount; i++)
      {
      *(m_sv[i]) = SVTYPE( m_svTemp[i] + ( m_k3[i] * m_timeStep ) );

   //   if (fabs(*(m_sv[i])) < 1E-30f)
  //       *(m_sv[i]) = 0.0f;
      }

   currentTime = m_time + m_timeStep;
   deriveFn( currentTime, m_timeStep, m_svCount, m_k4, extra );
   
   //--------------- done with k values ------------//
   //#pragma omp parallel for
   for (int i=0; i < m_svCount; i++ )
      {
      *(m_sv[ i ]) = m_svTemp[ i ] + (SVTYPE( m_k1[i] + m_k2[i]*2 + m_k3[i]*2 + m_k4[i] ) * m_timeStep / 6 );
//	  if (fabs(*(m_sv[i])) < 1E-30f)
 //        *(m_sv[i]) = 0.0f;
      }

   return true;
   }


//-- IntRKF ( ) -------------------------------------------------------
//
//-- Runge-Kutta-Fehlberg Method
//
//-- This is a fifth order adaptive method. It is best
//   to start with small timeStep and let the function find the right 
//   timeStep, as it proceeds it will come to the right step size.
//
//   Based on the six seperate calls to subroutine State, (RKF45) computes
//   an estimate for each state variable for one timeStep, an estimate of
//   the error for this step, and a new proposed size for the next step.
//
//   The step is usually specified with minimum and maximum values. (RKF45)
//   tries to use the largest stepsize, but the next proposed size may be
//   smaller in order to meet the minimum desired accuracy for the state
//   variables.
//
//   If the error is too large, the integration is repeated with a smaller
//   stepsize.
//---------------------------------------------------------------------

bool IntegrationBlock::IntRKF( DERIVEFN deriveFn, void *extra )
   {
#ifdef NO_MFC
     using namespace std; //for max()
#endif

   double error;
   double sum;
   double timeStep = m_timeStep;
   double currentTime = m_time;

   //-- get derivative values from object at current time --//  
   deriveFn( currentTime, timeStep, m_svCount, m_k1, extra );
 
   // #pragma omp parallel for   
   for (int i=0; i < m_svCount; i++ )
      m_svTemp[i] = *(m_sv[i]);

   #pragma omp parallel for
   for (int i=0; i < m_svCount; i++ )
      {
      *(m_sv[i]) = SVTYPE( m_svTemp[i] + m_k1[i] * timeStep / 4 );

 //     if (*(m_sv[i]) < 1E-30f)
 //       *(m_sv[i]) = 0.0f;   
      }  


   //-- get derivative values from object at current time + 1/4--//
   currentTime = m_time + timeStep/4;
   deriveFn( currentTime, timeStep, m_svCount, m_k2, extra );

   #pragma omp parallel for
   for (int i=0; i < m_svCount; i++)
      {
      *(m_sv[i]) = SVTYPE( m_svTemp[i] + ( ( ( m_k1[i]*3 + m_k2[i] * 9 ) / 32 ) * timeStep ) );
      
//      if (*(m_sv[i]) < 1E-30f)
 //       *(m_sv[i]) = 0.0f;
      }

   //-- get derivative values from object at current time + -//
   currentTime = m_time + timeStep*3/8;
   deriveFn( currentTime, timeStep, m_svCount, m_k3, extra );

   #pragma omp parallel for
   for (int i=0; i < m_svCount; i++)
      {
      *(m_sv[i]) = SVTYPE( m_svTemp[i] + ( ( ( m_k1[i]*1932 - m_k2[i]*7200 + m_k3[i]*7296) / 2197 ) * timeStep ) );

//      if (*(m_sv[i]) < 1E-30f)
//         *(m_sv[i]) = 0.0f;
      }

   //-- get derivative values from object at current time + -//
   currentTime = m_time + timeStep/2;
   deriveFn( currentTime, timeStep, m_svCount, m_k4, extra );

   //-- get derivative values from object at current time + -//
   currentTime = m_time + timeStep*12/13;
   deriveFn( currentTime, timeStep, m_svCount, m_k5, extra );

   #pragma omp parallel for
   for (int i=0; i < m_svCount; i++)
      {
      *(m_sv[i]) = SVTYPE( m_svTemp[i] 
         - ( ( m_k1[i]*439/216 - 8*m_k2[i] + m_k3[i]*3680/513 - m_k4[i]*845/4104) * timeStep  ) );

//      if (*(m_sv[i]) < 1E-30f)
//        *(m_sv[i]) = 0.0f;
      }

   //-- get derivative values from object at current time + -//
   currentTime = m_time+timeStep;
   deriveFn( currentTime, timeStep, m_svCount, m_k6, extra );

   #pragma omp parallel for
   for (int i=0; i < m_svCount; i++)
      {
      *(m_sv[i]) = SVTYPE( m_svTemp[i]
          - ( ( m_k1[i]*8/27 + 2*m_k2[i]+m_k3[i]*3544/2565-m_k4[i]*1859/4104-m_k5[i]*11/40 ) * timeStep ) );

//      if (*(m_sv[i]) < 1E-30f)
 //        *(m_sv[i]) = 0.0f;
      }

   //--------------- done with k values ------------//
   sum = 0.0f;
   error = 0.0f;
   float errorTe=0.0f;
   int maxError=-1;

   //-- all k values calculated, calculate 4th and 5th degree terms --//
   //#pragma omp parallel for
   for (int i=0; i < m_svCount; i++ )
      {
      sum = fabs ( ( m_k1[i]/360 - m_k3[i]*128/4275 - m_k4[i]*2197/75204 + m_k5[i]/50 + m_k6[i]*2/55) * timeStep );

      errorTe = (float) error;
      if ( *(m_sv)[ i ] != 0.0f )
         error = max( error, fabs( sum / *(m_sv)[ i ] ) );

      if (errorTe != error)
         maxError=i;
      }

   //-- update state variables --//
   #pragma omp parallel for
   for (int i=0; i < m_svCount; i++ )
      {
      *(m_sv[ i ]) = SVTYPE( m_svTemp[ i ] + (SVTYPE)( ( m_k1[i]*16/135 + m_k3[i]*6656/12825 + m_k4[i]* 
            28561/56430 - m_k5[i]*9/50 + m_k6[i]*2/55 ) * timeStep ) );
//      if (*(m_sv[i]) < 1E-30f)
//         *(m_sv[i]) = 0.00f;
      }

   // NOTE::: FOLLOWING CODE NEEDS TO BE VERIFIED!!!!!
   // adjust stepsize as needed
   float delta = 0;

   //-- increment time  --//
   currentTime = (m_time+timeStep);

   if ( error == 0 )
      delta = 5;
   else
      delta = (float) pow( m_rkvTolerance / ( 2*fabs( error ) ),  0.25 );  /// todo

   //-- don't change the timeStep dramatically because it makes a
   //-- difference by factor of 16

   //-- never increase delta more than 5 --//
   if ( delta > 5 )        // get time step multiplier value...
      delta = 5;
   else if ( delta < 0.1f )
      delta = 0.1f;

   //-- calculate new timeStep --//  
   timeStep *= delta;

   //ASSERT( timeStep > 0 );

   //-- check the sign --//
   if (  timeStep > m_rkvMaxTimeStep )
     timeStep = m_rkvMaxTimeStep;

   else if ( timeStep < m_rkvMinTimeStep )
     timeStep = m_rkvMinTimeStep;

   //-- check singular differential equation case --//
   //ASSERT( fabs( timeStep ) >= m_rkvMinTimeStep );
   if ( ( timeStep > 0 ) && ((currentTime+timeStep) > m_stopTime ) )         
      {
      //-- step just past end of simulation --//
//      timeStep = m_stopTime - curTime;
      currentTime = m_stopTime; 
      }

   ASSERT( timeStep > 0.0f ); 
   m_timeStep =  timeStep;

  return TRUE;
  }
