//-------------------------------------------------------------------
//  PROGRAM: randnorm.cpp
//-------------------------------------------------------------------

#include <EnvLibs.h>
#pragma hdrstop

//--------------------------- Includes ----------------------------//

#if !defined ( _RANDNORM_HPP )
#include "Randnorm.hpp"
#endif

extern "C"
{

#include <string.h>
#include <stdio.h>
}

#include <math.h>

//-- constructors --//

RandNormal::RandNormal( double _meanNorm, double _stdNorm, 
      long _seed, const char *_name )
   : Rand( 1, _seed, _name ),
     rand1( (UINT) 1, _seed ), // use stream 1, these should be independent!
     rand2( (UINT) 2, _seed ),  // use stream 2, so use different streams
     secondValue( 0 ),    // uses both random numbers produced
     secondValueFlag( FALSE )   // by the polar method
   {
   SetDistName( "Normal" );

   SetParameterA_Name( "Mean" );
   SetParameterB_Name( "StdDev" );

   SetMean  ( _meanNorm );
	SetStdDev( _stdNorm );
	}


void RandNormal::SetMean( double _meanNorm )
	{
	meanNorm = _meanNorm;
   secondValueFlag = FALSE;

	//-- set the coefficients --//
   //char temp[ 32 ];
   //sprintf( temp, "%g", meanNorm );
   //SetParameterA_Name( temp );
   }


void RandNormal::SetStdDev( double _stdNorm )
   {
   stdNorm = fabs( _stdNorm );
   secondValueFlag = FALSE;

	//char temp[ 32 ];
   //sprintf( temp, "%g", stdNorm );
   //SetParameterB_Name( temp );
   }

double RandNormal::RandValue( double _meanNorm, double _stdNorm )
   {
   SetMean( _meanNorm );
   SetStdDev( _stdNorm );

   return RandValue();
   }

void RandNormal::SetSeed( long _seed )
   {
   seed = _seed;

   rand1.SetSeed( _seed );
   rand2.SetSeed( _seed );

   secondValueFlag = false;
   }

// polar method by Marsaglia & Bray (1964)
double RandNormal::RandValue( void )
   {
   AddOneObservation();

   double firstValue;

   double W;         // algorithm variables
   double V1;
   double V2;

   if ( secondValueFlag == FALSE )  // No second value available
      {
      secondValueFlag = TRUE;
      do    // these are not automatically included in observations
         {  // use 2 different streams, pick two uniform numbers fron -1 to 1.
         V1 = 2 * rand1.RandValue() - 1.;
         V2 = 2 * rand2.RandValue() - 1.;
         W = V1 * V1 + V2 * V2;
         }
      while ( W >= 1. );            // Make sure they're in the unit circle.

      double Y = sqrt( -2. * ::log( W ) / W ); // Box-Muller transformation.

      firstValue =  V1 * Y;
      secondValue = V2 * Y;                // Save one number for next time.

      return( stdNorm * firstValue + meanNorm );
      }

   else
      {                             // Extra value available.
      secondValueFlag = FALSE;      // Reset the secondValueFlag.

      return( stdNorm * secondValue + meanNorm );
      }

   }


