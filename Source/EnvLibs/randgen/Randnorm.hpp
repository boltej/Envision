//-------------------------------------------------------------------
// PROGRAM: randnorm.hpp
//
// PURPOSE: Generates normal random numbers
//
// NOTE:    Uses polar method
//-------------------------------------------------------------------

/*
---------------------------------------------------------------------
   Note: Formula obtained from Law & Kelton, see reference
   Given X ~ N( 0, 1 ), we can obtain X' ~ N( mean, variance ) by
   setting X' = mean + variance * X. [ Law and Kelton, 1991 ]

   Normal random variates can also be transformed directly into random
   variates from other distributions, e.g., the lognormal.
   
   The most famous implementation of normal random variate generation is
the Box-Muller method. An improvement to the Box-Muller method was
described by Marsaglia and Bray ( 1964 ), and is called the polar method.
This normal random variate generator implements the polar method.

   1. Generate U1 and U2 as IID U( 0, 1 ), let Vi = 2Ui - 1 for i = 1, 2,
      and let W = V1 * V1 + V2 * V2.
   2. If W > 1, go back to step 1. Otherwise, let Y = sqrt( ( -2* ln W ) / W )
      and X1 = V1 * Y, and X2 = V2 * Y. The  X1 and X2 are IID N( 0, 1 ) 
      random variates. [ Law and Kelton, 1991 ]

   
Parameters: The mean is specified as a real number, and the standard
            deviation is specified as a non-negative real number.

Range:      [ -infinity, +infinity ]

Mean:       mean

Var:        variance

   "The normal distribution is used in situations in which the central
limit theorem applies -- i.e., quantities that are sums of other quantities.
It is also used empirically for many processes that are known to have a 
symmetric distribution and for which the mean and standard deviation can
be estimated. Because the theoretical range is from -infinity to +infinity,
the distribution should only be used for processing times when the mean is
at least three standard deviations above 0." [ Pegden et al, 1990 ]
            
Reference: Simulation Modeling & Analysis by Law and Kelton, 1982, 
           pg., 259-260.

           Introduction to Simulation Using SIMAN by C. Dennis Pegden, 
           Robert E. Shannon, and Randall P. Sadowski, 1990, pg., 562.
---------------------------------------------------------------------
*/

#if !defined _RANDNORM_HPP
#define      _RANDNORM_HPP 1

//--------------------------- Includes ----------------------------//

#if !defined ( _RANDUNIF_HPP )
#include "Randunif.hpp"
#endif



class  LIBSAPI  RandNormal : public Rand
   {

   private:

      RandUniform rand1;             // independent U( 0, 1 ) streams
      RandUniform rand2;

      double meanNorm;
      double stdNorm;
      //double varNorm;
      double secondValue;    // uses both random numbers produced
      int secondValueFlag;   // by the polar method

   public:

      //-- constructor --//
      RandNormal( double _meanNorm, double _stdNorm, long _seed,
         const char *_name = "-" );

      //-- destructor --//
      virtual ~RandNormal( void ) { }

      //-- returns a normal random variable --//
      virtual double RandValue( void );
      double RandValue( double _meanNorm, double _stdNorm );

      void SetMean  ( double _meanNorm ); // adjust dynamically
      void SetStdDev( double _stdNorm );

      void SetSeed( long _seed );
      
   };


#endif
