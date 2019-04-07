//-------------------------------------------------------------------
// PROGRAM: rand.hpp
//
// PURPOSE: Generates uniform ( 0, 1 ) random numbers
//
// NOTE:    "Intuitively, the random variables X and Y ( whether
//          discrete or continuous ) are independent if knowing the
//          value that one random variable takes on tells us nothing
//          about the distribution of the other. Also, if X and Y are
//          not independent, we say they are dependent."
//          [ Law & Kelton, 1991, pg. 275 ]
//
// RandNumGen.ZIP version 1.00, Copyright (C) 1992, Joseph A Fisher
// RandNumGen.ZIP comes with ABSOLUTELY NO WARRANTY and is copyrighted under
// the terms of the GNU General Public License.
//-------------------------------------------------------------------

/*
---------------------------------------------------------------------
This generator should have the following properties:
   1. The computer code is fast and uses a small amount of memory
   2. It generates repeatable sequences
   3. The resulting values pass resonable tests for being from the 
      uniform distribution
   4. The resulting sequences pass a number of different tests for randomness.
   5. A lattice structure is not obviously present.
      [ Thesen and Travis, 1992 ]
   6. Portability

   This generator implements the recommendations of Peter W. Lewis for
prime modulus multiplicative congruential random number generators (PMMCG).
[ Lewis and Orav, 1989 ]

   The formula is: 2^31 - 1 = LONG_MAX = 2,147,483,647 = m
                   seed(i+1) = ( multiplier*seed(i) ) mod( m )
                   U(0,1)    = seed(i+1) / m;
   
   multiplier: a -- is an integer in the range [ 2, 3, ..., m - 1 ]
   seed: z -- is an integer in the range [ 1, 2, ..., m - 1 ]
   modulus: m -- is a large prime integer ( usually 2^31 - 1 )
   uniform zero-one: U(0,1) -- is a real number between zero and one

   Note: The seed should never be 0 or LONG_MAX!

   FishMan & Moore find the 5 optimal full period multipliers for the 
(PMMCG) with prime modulus 2^31 -1. They present a battery of statistical
tests for evidence. They also discuss some poor results of some previously 
used multipliers. [ FishMan and Moore, 1986 ]

FishMan & Moore propose the following 5 as the best multipliers:
      multiplier  =   950,706,376
                  =   742,938,285
                  = 1,226,874,159
                  =    62,089,911
                  = 1,343,714,438

   If the user dosen't select a starting seed, I must provide a unique
default starting seed for them. Since I want many different default 
starting seeds, I will use the static 'defaultSeed' to increment the 
starting default seeds. The range of initial seeds is 2^31 - 2, 
so I'll space them apart by 1 million and get 2,000 different streams 
for each multiplier. Using all 5 different multipliers will give
5 * 2,000 = 10,000 different starting seeds.


STREAMS IMPLEMENTED
//----------------- 1 - 5 are recommend ! -------------//
1  FishMan & Moore 1 (1985)   (MCG) a =   950,706,376
1  IMSL3(1987)                (MCG) a =   950,706,376
2  FishMan & Moore 2 (1985)   (MCG) a =   742,938,285
3  FishMan & Moore 3 (1985)   (MCG) a = 1,226,874,159
4  FishMan & Moore 4 (1985)   (MCG) a =    62,089,911
5  FishMan & Moore 5 (1985)   (MCG) a = 1,343,714,438

6  SIMSCRIPT II.5 (1983)      (MCG) a =   630,360,016
7  GPSS/PC(1974)              (MCG) a =   397,204,094
7  SAS(1982)                  (MCG) a =   397,204,094
7  IMSL2(1984)                (MCG) a =   397,204,094
8  SPSS(1983)                 (MCG) a =        16,807
8  APL(1970)                  (MCG) a =        16,807
8  IMSL1(1984)                (MCG) a =        16,807


References: Simulation for Decision Making by Arne Thesen and
            Laurel E. Travis, 1992, 372-373.

            Simulation methodology for Statisticians, Operations Analysts,
            and Engineers by Peter Lewis and E. Orav, Volume 1, 1989.,
            pg., 89.

            An exhaustive analysis of multiplicative congruential random
            number generators with modulus 2^31 - 1" by George S. Fishman
            and Louis R. Moore III, SIAM J. Sci. Stat. Comput, Vol. 7,
            No. 1, January 1986, pgs., 24 - 45.
---------------------------------------------------------------------
*/

#if !defined _RAND_HPP
#define      _RAND_HPP 1

#include "libs.h"

//------------------------- Typedefs ------------------------------//

typedef unsigned int  UINT;
typedef unsigned long ULONG;

#define  FALSE 0
#define  TRUE  1

//------------------------- Constants -----------------------------//

const int CAPTIONLENGTH = 32;


class  LIBSAPI  Rand
   {

   //-- Prime modulus multiplicative congruential generator --//
   protected:

      static long defaultSeed;         // increments seeds by 1,000,000

      long seed;                       // current RNG seed
      long initialSeed;                // initial RNG seed
      long multiplier;                 // RNG multiplier
      long observations;               // # of times this generator was used
      char *name;                      // statistic name
      char *distName;                  // generator name
      char *parameterA;                // parameter a stored as a string
      char *parameterB;                // parameter b " "
      char *parameterC;                // parameter c " "
      char *header;                    // eg. "Uniform(0.0,1.0)"

      void SetDefaultSeed( void );
      long GetDefaultSeed( void );
      void SetInitialSeed( long _seed );  // can only be set once!

      void SelectMultiplierType ( long _multiplier );
      void SelectMultiplierType1( void ) { multiplier =  950706376L; }
      void SelectMultiplierType2( void ) { multiplier =  742938285L; }
      void SelectMultiplierType3( void ) { multiplier = 1226874159L; }
      void SelectMultiplierType4( void ) { multiplier =   62089911L; }
      void SelectMultiplierType5( void ) { multiplier = 1343714438L; }
      void SelectMultiplierType6( void ) { multiplier =  630360016L; }
      void SelectMultiplierType7( void ) { multiplier =  397204094L; }
      void SelectMultiplierType8( void ) { multiplier =      16807L; }

   protected:

      //-- constructors --//
      Rand( UINT _stream, const char *_name = "-" );
      Rand( UINT _stream, long _seed, const char *_name = "-" );

      //-- get U( 0,1 ) random number from generator --//
      double GetUnif01( void );

      void AddOneObservation( void ) { observations++; }

   public:

      //-- destructor --//
      virtual ~Rand( void );

      //-- pure virtual, child must return random number --//
      virtual double RandValue( void ) = 0;

      //-- reset the statistics of the RNG --//
      void ReInitializeStats( void ) { observations = 0; }

      //-- Get: output information --//
      const char* const GetName( void ) { return (const char* const) name; }
      const char* const GetDistName( void ) 
            { return (const char* const) distName; }
      const char* const GetA( void ) { return (const char* const) parameterA; }
      const char* const GetB( void ) { return (const char* const) parameterB; }
      const char* const GetC( void ) { return (const char* const) parameterC; }
      const char* const GetHeader( void );

      void SetName           ( const char *_name );
      void SetDistName       ( const char *_name );
      void SetParameterA_Name( const char *_name );
      void SetParameterB_Name( const char *_name );
      void SetParameterC_Name( const char *_name );

      long GetInitialSeed( void )  { return initialSeed; }
      long GetMultiplier( void )   { return multiplier; }
      long GetObservations( void ) { return observations; }
      long GetSeed( void )         { return seed; }

      void SetMultiplier( long _multiplier );
      void SetSeed( long _seed );

      // Additional methods
      int SampleFromWeightedDistribution( int   *weights, int count, int   maxValue );
      int SampleFromWeightedDistribution( float *weights, int count, float maxValue );

   };

#endif
