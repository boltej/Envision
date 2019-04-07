//-------------------------------------------------------------------
//  PROGRAM: randunif.cpp
//-------------------------------------------------------------------

#include <EnvLibs.h>
#pragma hdrstop

//--------------------------- Includes ----------------------------//

#if !defined ( _RANDUNIF_HPP )
#include "Randunif.hpp"
#endif


extern "C"
{
#include <string.h>
#include <stdio.h>
}


//-- constructors --//

RandUniform::RandUniform(const char *_name )
   : Rand( 1, _name ), 
     lowerBound( 0 ), 
     upperBound( 1 )
   {
   SetDistName( "Uniform" );

   //-- set the coefficients --//
   char temp[ 40 ];
   sprintf_s( temp, 40, "%g", lowerBound );
   SetParameterA_Name( temp );
   sprintf_s( temp, 40, "%g", upperBound );
   SetParameterB_Name( temp );
   }


RandUniform::RandUniform( UINT _stream, long _seed, const char *_name )
   : Rand( _stream, _seed, _name ), 
     lowerBound( 0 ), 
     upperBound( 1 )
   { 
   SetDistName( "Uniform" );

   //-- set the coefficients --//
   char temp[ 40 ];
   sprintf_s( temp, 40, "%g", lowerBound );
   SetParameterA_Name( temp );
   sprintf_s( temp, 40, "%g", upperBound );
   SetParameterB_Name( temp );
   }

RandUniform::RandUniform( double _lowerBound, double _upperBound, 
      long _seed, const char *_name )
   : Rand( 1, _seed, _name ),
     lowerBound( _lowerBound ), 
     upperBound( _upperBound )
   { 
   SetDistName( "Uniform" );

   //-- set the coefficients --//
   char temp[ 40 ];
   sprintf_s( temp, 40, "%g", lowerBound );
   SetParameterA_Name( temp );
   sprintf_s( temp, 40, "%g", upperBound );
   SetParameterB_Name( temp );
   }

RandUniform::RandUniform( double _lowerBound, double _upperBound, \
      UINT _stream, long _seed, const char *_name )
   : Rand( _stream, _seed, _name ), 
     lowerBound( _lowerBound ), 
     upperBound( _upperBound )
   { 
   SetDistName( "Uniform" );

   //-- set the coefficients --//
   char temp[ 40 ];
   sprintf_s( temp, 40, "%g", lowerBound );
   SetParameterA_Name( temp );
   sprintf_s( temp, 40, "%g", upperBound );
   SetParameterB_Name( temp );
   }


double RandUniform::RandValue( void )
   {
   AddOneObservation();

   return ( lowerBound + ( upperBound - lowerBound ) * GetUnif01() );
   }


