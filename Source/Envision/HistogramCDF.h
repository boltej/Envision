/*
This file is part of Envision.

Envision is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Envision is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Envision.  If not, see <http://www.gnu.org/licenses/>

Copywrite 2012 - Oregon State University

*/
#pragma once
#define NOMAXMIN

#include <string>
#include <istream>
#include <vector>
#include <memory>

#include "GsTL/cdf.h"

using std::auto_ptr;
using std::string;
using std::vector;

//======================================================================================= 
/* Literature shows lots of uncertainty regarding probability density estimation; here
the problem of histograms.  How many bins is the eternal question.  Same size bins or 
adaptive size bins another eternal question.  Concept of binning is deprecated in light of
spline and kernel smoothing methods.
Here in the absence of better algorithms (spline fitting, kernels, AMISE) we resort to heuristics
Note also the use of tailsProb.
*/
//======================================================================================= 
//template<class R>
class HistogramCDF
{
public:
   HistogramCDF();  

   double tailsProb;                             // For the GsTL algorithm; allow some tail room.
   enum {NA=0, BIN_LIMITS_KNOWN=1, BIN_NO=127, RESERVE=512};  // BIN_NO at least 32
   int state;

   // For reading the line
   string name;   // e.g. EnvEvaluator::name
   string path;
   string format;
private:
public:
   auto_ptr<Non_param_cdf<> >    npcdf;  // The cumulative density function
   //Non_param_cdf<>  & getCdf() const { return *npcdf);  // DOESN'T COMPILE
   double   prob( Non_param_cdf<>::value_type  z )  const { return npcdf->prob(z); }
   Non_param_cdf<>::value_type   inverse( double p ) const { return npcdf->inverse(p); }
   inline float   getEnvScore( float score) const ; 
   static float   getEnvScore( const Non_param_cdf<> & cdf, float score);
   int            readQuantiles(const string &in);   // initialize from persisted data
   static string  readQuantiles(Non_param_cdf<> & cdf, const std::string & in);
   int            writeQuantiles(string & out) const;
   static int     writeQuantiles(string & out, const Non_param_cdf<> & cdf, const string & modelName);

   void convertToCDF();           // converts counts to quantiles and initializes npcdf

   void initEnvisionStandardBinLimits();  // Initializes binlimits to [-3, ..., ..., 3+)

   // Called repeatedly and increments the counts[i] corresponding to val; 
   // i.e. updates histogram when BIN_LIMITS_KNOWN
   // Subsequently call convertToCDF()
   void binData(double val);
   void binData(vector<double> & val);

   // Called repeatedly and accumulates data into vcum when  ! BIN_LIMITS_KNOWN.
   // Subsequently call initBins() to get limits and counts;
   // And Finally call convertToCDF(). 
public:
   void accumData(double val);
private:
   // initBinLimitsAndBin(.) usu called with arg, member vcum, for learning a model of unknown bounds; 
   // requires val filled and sorts val.
   void initBinLimitsAndBin( vector<double> & val );
   std::vector<double> vcum;                 // Z (e.g. scores in Envision) are accumulated here before binning.
      
   void clearCLQ() { counts.clear(); limits.clear(); quantiles.clear() ; }
   std::vector<size_t> counts;
   std::vector<double> limits;    // limits are same as Z is GsTL, value of variable.
   std::vector<double> quantiles; // quantiles are same as p in GsTL, cumulative prob corresponding to Z.
};
