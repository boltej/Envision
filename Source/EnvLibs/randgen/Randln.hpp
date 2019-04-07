//-------------------------------------------------------------------
// PROGRAM: randln.hpp
//
// PURPOSE: Generates lognormal distributed random numbers
//-------------------------------------------------------------------

/*
---------------------------------------------------------------------
   Note: Formula obtained from Law & Kelton, see reference
   "A special property of the lognormal distribution, namely, 
   that if Y ~ N( mean, var ) then exp( Y ) ~ LN( mean, var ), 
   is used to obtain the following algorithm:
      1. Generate Y ~ N( mean, var )
      2. Set X = exp( Y ) 

      where meanLogNorm = mean of lognormal distribution
            varLogNorm  = var of lognormal distribution

      mean = ln[ meanLogNorm ^ 2 / sqrt( varLogNorm + meanLogNorm ^ 2 ) ]
      var  = ln[ ( varLogNorm + meanLogNorm ^ 2 ) / meanLogNorm ^ 2 ]
      
      Then get Y ~ N( mean, var )
      and the X = exp( Y ) [ Law and Kelton, 1982 ]

Parameters: The mean and the standard deviation specified as positive
            real numbers.

Range:      [ 0, +infinity ]

Mean:       meanLogNorm
Var:        varLogNorm

   "The lognormal distribution is used in situations in which the 
quantity is the product of a large number of random quantities. It is
also used to represent task times that have a non-symmetric distribution".
[ Pegden et al, 1990 ]
            
Reference: Simulation Modeling & Analysis by Law and Kelton, 1982, 
           pg., 259-260.

           Introduction to Simulation Using SIMAN by C. Dennis Pegden, 
           Robert E. Shannon, and Randall P. Sadowski, 1990, pg., 562.
---------------------------------------------------------------------
*/

#if !defined _RANDLN_HPP
#define      _RANDLN_HPP 1

//----------------------------- Includes --------------------------//

#if !defined ( _RAND_HPP )
#include "Rand.hpp"
#endif


//-------------------------- Forward reference --------------------//

class RandNormal;


class LIBSAPI RandLogNormal : public Rand
   {

   private:

      RandNormal *pRNG;              // RandNormal object ptr

      //-- parameters --//
      double meanLogNorm;            // Log normal mean
      double stdLogNorm;             // Log normal standard deviation
      double varLogNorm;             // Log normal variance

      double meanNorm;               // Normal mean
      double stdNorm;                // Normal standard deviation
      double varNorm;                // Normal variance

      void SetNormalMean( void );       // create the normal mean
      void SetNormalVariance( void );   // create the normal variance

   public:

      //-- constructor --//
      RandLogNormal( double meanLogNorm, double stdLogNorm,
            long _seed, const char *_name = "-" );

      //-- destructor --//
      virtual ~RandLogNormal( void ); 

      //-- returns a log normal random variable --//
      virtual double RandValue( void );

      virtual double RandValue(double meanNorm, double stdNorm);

   };

#endif

 
