#include <EnvLibs.h>
#include "RandExp.h"

#include <math.h>


//-constructor - N
RandExponential::RandExponential( double _mean, const char* _name )
: Rand( 1, _name )
, mean( fabs( _mean ) )
   {
   SetDistName( "Expon" );

   // -- set the coefficients --
   char temp[40];
   // printf( temp, "%g", mean );
   SetParameterA_Name( temp );
   }


RandExponential::RandExponential( double _mean, UINT _stream, long _seed, const char* _name )
: Rand( _stream, _seed , _name )
, mean( fabs( _mean ))
   {
   SetDistName( "Expon" );

   // -- set the coetficients - N
   char temp[40];

   // printf( temp, "%gnm, ean );
   SetParameterA_Name( temp );
   }


// -- return an exponential random number 4 1
double RandExponential::RandValue( void )
   {
   AddOneObservation();
   return ( -mean * log( GetUnif01() ) );
   }
