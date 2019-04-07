//-------------------------------------------------------------------
//  PROGRAM: randweib.cpp
//-------------------------------------------------------------------
#include <EnvLibs.h>
#pragma hdrstop

//--------------------------- Includes ----------------------------//

#if !defined ( _RANDWEIB_HPP )
#include "Randweib.hpp"
#endif

extern "C"
{
#include <math.h>
#include <string.h>
#include <stdio.h>
}


//-- constructors --//

RandWeibull::RandWeibull( double _shape, double _scale, const char *_name )
   : Rand( 1, _name ),
     shape( fabs( _shape ) ),
     scale( fabs( _scale ) ),
     oneOverShape( 1 / shape )
   {
   SetDistName( "Weibull" );

   //-- set the coefficients --//
   char temp[ 40 ];
   sprintf_s( temp, 40, "%g", shape );
   SetParameterA_Name( temp );
   sprintf_s( temp, 40, "%g", scale );
   SetParameterB_Name( temp );
   }


RandWeibull::RandWeibull( double _shape, double _scale,
      UINT _stream, long _seed, const char *_name )
   : Rand( _stream, _seed, _name ),
     shape( fabs( _shape ) ),
     scale( fabs( _scale ) ),
     oneOverShape( 1 / shape )
   {
   SetDistName( "Weibull" );

   //-- set the coefficients --//
   char temp[ 40 ];
   sprintf_s( temp, 40, "%g", shape );
   SetParameterA_Name( temp );
   sprintf_s( temp, 40, "%g", scale );
   SetParameterB_Name( temp );
   }


double RandWeibull::RandValue( void )
   {
   AddOneObservation();

   return ( scale * pow( -log( GetUnif01() ), oneOverShape ) );
   }

