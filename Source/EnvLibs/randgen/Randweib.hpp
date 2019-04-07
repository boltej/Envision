//-------------------------------------------------------------------
// PROGRAM: randweib.hpp
//
// PURPOSE: Generates weibull distributed random numbers
//-------------------------------------------------------------------

/*
---------------------------------------------------------------------
Note: Inverse transform method where
      X = scale * ( - ln( U( 0,1 ) ) ^ ( 1 / shape ) )
      [ Bulgren, 1982 ]

Parameters: shape parameter (alpha) and scale parameter (beta) are
            non-negative reals, ( 1 / scale ) is similar to that of
            the mean in the exponential dist.

Range:      [ 0, +infinity ]

Mean: see [ Pegden et al, 1990 ]
Var:  see [ Pegden et al, 1990 ]

   "The Weibull distribution is widely used in reliability models to
represent the lifetime of a device. If a system consists of a large number
of parts that fail independently, and if the system fails when any single
part fails, then the time between failures can be approximated by the
Weibull distribution. This distribution is also used to represent non-
negative task times that are skewed to the left". [ Pegden et al, 1990 ]


References: Discrete System Simulation by William G. Bulgren 1982, pg. 169.

            Introduction to Simulation Using SIMAN by C. Dennis Pegden, 
            Robert E. Shannon, and Randall P. Sadowski, 1990, pg., 568.
---------------------------------------------------------------------
*/

#if !defined _RANDWEIB_HPP
#define      _RANDWEIB_HPP 1

//----------------------------- Includes --------------------------//

#if !defined ( _RAND_HPP )
#include "Rand.hpp"
#endif



class LIBSAPI RandWeibull : public Rand
   {

   private:

      double shape;
      double scale;
      double oneOverShape;

   public:

      //-- constructors --//
      RandWeibull( double _shape, double _scale, const char *_name = "-" );
      RandWeibull( double _shape, double _scale, UINT _stream, long _seed, 
         const char *_name = "-" );

      //-- destructor --//
      virtual ~RandWeibull( void ) { }

      //-- return a weibull random number --//
      virtual double RandValue( void );

   };


#endif
