
#if !defined _RANDOM_HPP
#define      _RANDOM_HPP 1

#include "libs.h"


#include <randgen/Rand.hpp>
//#include <randgen/randerla.hpp>
#include <randgen/Randln.hpp>
//#include <randgen/randbeta.h>
#include <randgen/Randunif.hpp>
//#include <randgen/randbern.hpp>
#include <randgen/Randnorm.hpp>
#include <randgen/Randweib.hpp>
//#include <randgen/rand_f.h>
//#include <randgen/randduni.hpp>
//#include <randgen/randbino.hpp>
#include <randgen/RandExp.h>
//#include <randgen/randtri.hpp>
//#include <randgen/rand_t.hpp>
//#include <randgen/randchi.hpp>
//#include <randgen/randgama.hpp>
//#include <randgen/randpois.hpp>
//#include <randgen/randgeo.hpp>
//#include <randgen/randnbin.hpp>


//-- class for handling normally or uniformly distributed 
//-- stochastic variables

class RVAR
   {
   protected:
      static Rand *pRand;     // static pointer to Random Number generator

      float mean;             // minimum if uniform distribution
      float stdDev;           // maximum if uniform distribution

   public:
      //-- constructors --//
      RVAR( void ) 
            : mean( 0 ), stdDev( 0 ) { }

      RVAR( float _mean )
            : mean( _mean ), stdDev( 0 ) { }

      RVAR( float _mean, float _stdDev )
            : mean( _mean ), stdDev( _stdDev ) { }

      //-- set random number generator --//
      static void SetRandPtr( Rand *_pRand ) { pRand = _pRand; }

      //-- accessors --//
      void SetValue ( float _mean )   { SetMean( _mean ); }

      //-- for normal --//
      void SetMean  ( float _mean )   { mean   = _mean  ; }
      void SetStdDev( float _stdDev ) { stdDev = _stdDev; }

      //-- for uniform --//
      void SetMin   ( float _min )    { SetMean( _min ) ;  }
      void SetMax   ( float _max )    { SetStdDev( _max ); }

      //-- this needs to be expanded!! --//
      float GetValue( void );
   };

inline
float RVAR::GetValue( void )
   { 
   if ( pRand == NULL )
      return mean;
   else
      {
      float value = (float) pRand->RandValue();

      return value;
      }
   }





#endif
