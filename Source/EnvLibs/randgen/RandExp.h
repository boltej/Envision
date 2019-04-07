#pragma once
#include "Rand.hpp"

////////////////////////////////////////////////////////////////////////////////////////////
// Generates exponential random numbers N
// Parameters: mean is the rate parameter and is > 0
// Range: [ 0, +infinity ]
// Mean: Beta( mean )
// Var: bate^2
////////////////////////////////////////////////////////////////////////////////////////////

class LIBSAPI  RandExponential : public Rand
{
private:
   double mean; // rate parameter

public:
   // -- constructor -- 
   RandExponential( double _mean, const char* _name = " " );
   RandExponential( double _mean, long _seed, const char* _name = " " );
   RandExponential( double _mean, UINT _stream, long _seed, const char* _name = " " );

   // - destructor - 
   virtual ~RandExponential( void ) { }

   // - return an exponential random number -
   virtual double RandValue( void );

   void SetMean( double _mean ) { mean = _mean; }
};
