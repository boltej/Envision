//-------------------------------------------------------------------
//  PROGRAM: randln.cpp
//-------------------------------------------------------------------

#include <EnvLibs.h>
#pragma hdrstop

//--------------------------- Includes ----------------------------//

#if !defined ( _RANDLN_HPP )
#include "Randln.hpp"
#endif

#if !defined ( _RANDNORM_HPP )
#include "Randnorm.hpp"
#endif


extern "C"
{

#include <string.h>
#include <stdio.h>
}
#include <math.h>

//-- constructor --//
RandLogNormal::RandLogNormal( double _meanLogNorm, double _stdLogNorm,
      long _seed, const char *_name )
   : Rand( 1, _seed, _name ),
     meanLogNorm( fabs( _meanLogNorm ) ),
     stdLogNorm( fabs( _stdLogNorm ) ),
     varLogNorm( stdLogNorm * stdLogNorm )
   {
   SetDistName( "LogNormal" );

   //-- set the coefficients --//
   char temp[ 40 ];
   sprintf_s( temp, 40, "%g", meanLogNorm );
   SetParameterA_Name( temp );
   sprintf_s( temp, 40, "%g", stdLogNorm );
   SetParameterB_Name( temp );

   SetNormalMean();
   SetNormalVariance();

   //-- run this constructor after mean and var have been set --//
   pRNG = new RandNormal( meanNorm, stdNorm, _seed );
   }


RandLogNormal::~RandLogNormal( void )
   {
   if ( pRNG )
      delete pRNG;
   }


double RandLogNormal::RandValue( void )
   {
   AddOneObservation();

   double Y = pRNG->RandValue();
   return( exp( Y ) );
   }


double RandLogNormal::RandValue(double _meanLogNorm, double _stdLogNorm)
   {
   meanLogNorm = _meanLogNorm;
   stdLogNorm = _stdLogNorm;
   varLogNorm = (stdLogNorm * stdLogNorm);

   SetNormalMean();
   SetNormalVariance();

   AddOneObservation();

   double Y = pRNG->RandValue(meanNorm,stdNorm);
   return(exp(Y));
   }



void RandLogNormal::SetNormalMean( void )
   {
   meanNorm = log( ( meanLogNorm * meanLogNorm /
         sqrt( varLogNorm + meanLogNorm * meanLogNorm ) ) );
   }


void RandLogNormal::SetNormalVariance( void )
   {
   varNorm = log( ( varLogNorm + meanLogNorm * meanLogNorm ) /
                   ( meanLogNorm * meanLogNorm ) );

   stdNorm = sqrt( varNorm );
   }



