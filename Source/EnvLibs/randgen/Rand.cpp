//-------------------------------------------------------------------
//  PROGRAM: rand.cpp
//-------------------------------------------------------------------

#include <EnvLibs.h>
#pragma hdrstop

//----------------------------- Includes --------------------------//

#if !defined ( _RAND_HPP )
#include "Rand.hpp"
#endif

#if !defined ( __MATH_H )
#include <math.h>
#endif

#include <string.h>

#if !defined _MFC
#include <limits.h> //const long LONG_MAX=2147483647;
#endif

long Rand::defaultSeed = 0;            // static to entire class
                                       // which means every instance

//-- Rand::Rand() ---------------------------------------------------
//
// constructors
//-------------------------------------------------------------------

Rand::Rand( UINT _multiplierType, const char *_name )
   : seed( 0 ),
     initialSeed( 0 ),
     multiplier( 0 ),
     observations( 0 )
   {
   SetName( _name );
   SetDistName( "-" );
   SetParameterA_Name( "-" );
   SetParameterB_Name( "-" );
   SetParameterC_Name( "-" );

   SelectMultiplierType( _multiplierType );
   SetDefaultSeed();
   }


Rand::Rand( UINT _multiplier, long _seed, const char *_name )
   : seed( 0 ),
     initialSeed( 0 ),   
     multiplier( 0 ), 
     observations( 0 )
   {
   SetName( _name );
   SetDistName( "-" );
   SetParameterA_Name( "-" );
   SetParameterB_Name( "-" );
   SetParameterC_Name( "-" );

   SelectMultiplierType( _multiplier );
   SetSeed( _seed );
   }



const char* Rand::GetHeader( void )
   {
   if ( header.empty() == false )
      return (const char* const) header.c_str();
      
   //-- make the header string from the distName and parameter? strings

   UINT strlength = distName.size();
   strlength += parameterA.size();
   strlength += parameterB.size();
   strlength += parameterC.size();

   header = distName + "(" + parameterA +  "," + parameterB + "," + parameterC + ")";

   return header.c_str();
   }


void Rand::SetName( const char  *_name )
   {
    if (_name)
        name = _name;
    else
        name.resize(0);
   }

void Rand::SetDistName( const char  *_distName )
   {
    if (_distName)
        distName = _distName;
    else
        distName.resize(0);
   }


void Rand::SetParameterA_Name( const char  *_parameterA )
   {
    if (_parameterA)
        parameterA = _parameterA;
    else
        parameterA.resize(0);
   }


void Rand::SetParameterB_Name( const char  *_parameterB )
   {
    if (_parameterB)
        parameterB = _parameterB;
    else
        parameterB.resize(0);
   }


void Rand::SetParameterC_Name( const char  *_parameterC )
   {
    if (_parameterC)
        parameterC = _parameterC;
    else
        parameterC.resize(0);
   }


//-- return a U(0,1) number from generator --//
double Rand::GetUnif01( void ) 
   {
   seed = (long) fmodl( ( (long double) multiplier * (long double) seed ),
                          (long double) LONG_MAX );
   return( (double) seed / (double) LONG_MAX );
   }

void Rand::SetDefaultSeed( void )
   {
   seed = GetDefaultSeed();

   initialSeed = seed;     // the user still has a chance to 
                           // reset the initial seed by calling 
                           // SetSeed( long _seed );

   return;
   }

long Rand::GetDefaultSeed( void )
   {
   //-- since defaultSeed is static, it is shared by all classes and
   //   hence all instances

   if ( defaultSeed >= 2145000000 )       // 2 billion 145 million
      defaultSeed = 0;
   
   defaultSeed+=1000000;                  // 1 million

   return defaultSeed;
   }


void Rand::SetMultiplier( long _multiplier )
   {
   if ( _multiplier <= 1 || _multiplier >= LONG_MAX )
      SelectMultiplierType1();
   else
      multiplier = _multiplier;

   return;
   }


void Rand::SetSeed( long _seed )
   {
   if ( _seed <= 0 || _seed >= LONG_MAX )
      SetDefaultSeed();
   else
      seed = _seed;
   
   SetInitialSeed( seed );

   return;
   }


void Rand::SetInitialSeed( long _seed )
   {
   static UINT initialSeedSet = 0;     // static to an instance of the class

   if ( initialSeedSet )               // only set once per instance of class
      return;

   initialSeedSet = 1;

   initialSeed = _seed;

   return;
   }


void Rand::SelectMultiplierType( long _multiplier )
   {

   switch( _multiplier )
      {
      case 2:
         SelectMultiplierType2();
         break;
   
      case 3:
         SelectMultiplierType3();
         break;

      case 4:
         SelectMultiplierType4();
         break;      

      case 5:
         SelectMultiplierType5();
         break;

      case 6:
         SelectMultiplierType6();
         break;

      case 7:
         SelectMultiplierType7();
         break;

      case 8:
         SelectMultiplierType8();
         break;

      default:
         SelectMultiplierType1();
         
      } // end of switch

   return;
   }

int Rand::SampleFromWeightedDistribution( int *weights, int count, int maxValue )
   {
   double _randVal = GetUnif01();
   int randVal = (int) (_randVal * maxValue);

   int valueSoFar = 0;
   for ( int i=0; i < count; i++ )
      {
      valueSoFar += weights[ i ];
      if ( valueSoFar >= randVal )
         return i;
      }

   return -1;
   }

int Rand::SampleFromWeightedDistribution( float *weights, int count, float maxValue )
   {
   float randVal = (float) GetUnif01();
   randVal *= maxValue;

   float valueSoFar = 0;
   for ( int i=0; i < count; i++ )
      {
      valueSoFar += weights[ i ];
      if ( valueSoFar >= randVal )
         return i;
      }

   return -1;
   }
