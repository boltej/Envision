//-------------------------------------------------------------------
// PROGRAM: randunif.hpp
//
// PURPOSE: Generates uniform ( 0, 1 ) random numbers
//          or uniform ( lowerBound, upperBound ) random numbers
//          where lowerBound < upperBound
//-------------------------------------------------------------------

/*
---------------------------------------------------------------------
   Note: Inverse transform method for U( lowerBound, upperBound ) 
   numbers is X = lowerBound + ( upperBound - lowerBound ) * U( 0,1 ) 
   [ Bulgren, 1982 ]

Parameters: The minimum and maximum value for the distribution specified
            as real numbers.

Range:      [ lowerBound, upperBound ]

Mean:       ( lowerBound + upperBound ) / 2

Variance:   ( ( upperBound - lowerBound ) ^ 2 ) / 12

   "The uniform distribution is used when all values over a finite range
are considered to be equally likely. It is sometimes used when no
information other than the range is available. The uniform distribution
has a larger variance than other distributions that are used when information
is lacking( e.g., the triangular distribution ). Because of its large
variance, the uniform distribution generally produces "worst case" results."
[ Pegden et al, 1990 ]


References: Discrete System Simulation by William G. Bulgren, 1982, pg. 167.

            Introduction to Simulation using SIMAN by C. Dennis Pegden, 
            Robert E. Shannon, and Randall P. Sadowski, 1990, pg., 567.
---------------------------------------------------------------------
*/

#if !defined _RANDUNIF_HPP
#define      _RANDUNIF_HPP 1

//----------------------------- Includes --------------------------//

#if !defined ( _RAND_HPP )
#include "Rand.hpp"
#endif


class  LIBSAPI  RandUniform : public Rand
   {
   private:

      //-- scale the U( 0,1 ) to a new range --//
      double lowerBound;
      double upperBound;

   public:

      //-- constructors --//
      RandUniform( const char *_name = "-" );
      RandUniform( UINT _stream, long _seed, const char *_name = "-" );
      RandUniform( double _lowerBound, double _upperBound, long _seed, 
            const char *_name = "-" );
      RandUniform( double _lowerBound, double _upperBound, \
            UINT _stream, long _seed, const char *_name = "-" );

      //-- destructor --//
      virtual ~RandUniform( void ) { }

      //-- return a uniform number --//
      virtual double RandValue( void );

      double RandValue( double _lowerBound, double _upperBound )
         { lowerBound = _lowerBound; upperBound = _upperBound; return RandValue(); } 

      //-- special case for the uniform function --//
      double GetUnif01( void ) { return Rand::GetUnif01(); }
      inline double operator() () { return  Rand::GetUnif01(); } // for GsTL

   };


#endif
