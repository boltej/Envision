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
#include "StdAfx.h"
#include ".\histogramcdf.h"
#include <ios>
#include <vector>
#include <sstream>
#include <cfloat>
#include <algorithm>
using std::streambuf;
using std::sort;
using std::ostringstream;
using std::istringstream;
using std::string;
using std::vector;
//=======================================================================================
HistogramCDF::HistogramCDF() : tailsProb(0.025), state(NA)
{
   vcum.reserve(RESERVE);
}
//=======================================================================================
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
float HistogramCDF::getEnvScore( const Non_param_cdf<> & cdf, float score)
   {
   score = float( 6.0 * cdf.prob(score) - 3.0 );
   return score;
   }
inline float HistogramCDF::getEnvScore(float score) const
   {
      if (npcdf.get())
         score = HistogramCDF::getEnvScore(*npcdf, score);
      return score;
   }
//-----------------------------------------------------------------------------------
void HistogramCDF::accumData(double val)
{
   vcum.push_back(val);
}
//-----------------------------------------------------------------------------------
void HistogramCDF::initBinLimitsAndBin(std::vector<double> &val)    // for learning a model of unknown bounds.
{
   sort(val.begin(), val.end());
   // 
   double binMin = val.front();
   double binMax = val.back();
   double binStep = (binMax - binMin) / (BIN_NO + 1);

   // Pathological case
   if (fabs(binStep - 0.0) < 0.001/BIN_NO)
   {
      initEnvisionStandardBinLimits();
      binData(val);
      return;
   }

   // Usual case
   limits.push_back(binMin-binStep);
   for(double bin = binMin; bin < binMax+binStep; bin+=binStep)
   {
      limits.push_back(bin);
   }

   counts.resize(limits.size(), 0);
   quantiles.resize(limits.size(), 0.0);
   state |= BIN_LIMITS_KNOWN;
   binData(val);
}
//=======================================================================================
void HistogramCDF::initEnvisionStandardBinLimits()  // for learning a model that already is bounded within [-3..3]
{
   double binMin = -3.;
   double binMax = 3.;
   double binStep = 0.10;

   limits.push_back(binMin-binStep);
   for(double bin = binMin; bin < binMax+binStep; bin+=binStep)
   {
      limits.push_back(bin);
   }
   counts.resize(limits.size(), 0);
   quantiles.resize(limits.size(), 0.0);
   state |= BIN_LIMITS_KNOWN;
}
//-----------------------------------------------------------------------------------
//==========================================================================================================
//                                 ---- CDFS  ----
//==========================================================================================================
// <name><,>[path]<,><format><,><data>[
//         if path empty, read here
//                    format of {TEXT, N_BIN, xmlschema=...}
//                                 data depends of format; if TEXT, spc separated pairs of score and Pr(SCORE <= score) 
//----------------------------------------------------------------------------------------------------------------------------
string HistogramCDF::readQuantiles(Non_param_cdf<> & cdf, const std::string & in)
   {
      HistogramCDF hcdf;
      hcdf.readQuantiles(in);
      cdf = *(hcdf.npcdf);
      return hcdf.name;
   }
//-------------------------------------------------------
int HistogramCDF::readQuantiles(const std::string & line)
{
   std::istringstream iss;
   iss.str(line);
   float z , p;

   // members    string name, path, format;
   getline(iss, name,   ','); 
   assert(iss.good());
   getline(iss, path,   ','); 
   assert(iss.good());
   getline(iss, format, ','); 
   assert(iss.good());

   while ( ! iss.eof())
   {
      z = p = FLT_MAX;
      iss >> z >> p ; 

      if(z==FLT_MAX && p==FLT_MAX)
         break;
      else if (z==FLT_MAX)
         return -1;

      assert(p>=0. && p<=1.);
      //TRACE("z(%f), p(%f)\n", z, p);
      limits.push_back(z);
      quantiles.push_back(p); 
   }
   
   npcdf =  auto_ptr<Non_param_cdf<> >( new Non_param_cdf<>(limits.begin(), limits.end(), quantiles.begin()));

   clearCLQ();

   return 0;
}
//-----------------------------------------------------------------------------------
int  HistogramCDF::writeQuantiles(std::string & out, const Non_param_cdf<> & cdf, const string & modelName)
   {
   Non_param_cdf<>::const_z_iterator zit; 
   Non_param_cdf<>::const_p_iterator pit; 
   Non_param_cdf<>::const_z_iterator zend = cdf.z_end();

   ostringstream oss;
   oss << modelName << ", " /*<< path*/ << ", " /*<< format*/ << ", ";

   for(zit = cdf.z_begin(), pit = cdf.p_begin(); zit != zend; ++pit, ++zit)
   {
      oss << " " << *zit << " " << *pit;
   }
   // oss << std::endl;
   out.clear();
   out = oss.str();

   return 0;
   }
//-------------------------------------------------
int  HistogramCDF::writeQuantiles(std::string & out) const
{ 
   return writeQuantiles(out, *npcdf, name);
}
//==================================================================================
void HistogramCDF::convertToCDF() 
{
   size_t i;
   if (limits.empty() && ! vcum.empty())
   {
      initBinLimitsAndBin(vcum);
   }
   size_t sz = limits.size();

   assert(0 == counts[0]);
   double count = 0.0;
   for(i=0; i<sz; ++i)
   {
      count += counts.at(i);
      quantiles.at(i) = count;
   }
   if (count > 0.0)
   {
      for(i = 0; i < sz; ++i)
      {
         quantiles[i] /= count;
      }
   }

   npcdf =  auto_ptr<Non_param_cdf<> >( new Non_param_cdf<>(limits.begin(), limits.end(), quantiles.begin()));

   clearCLQ();
}
//-----------------------------------------------------------------------------------
// [] ensure to test with out of bounds on the top end
void HistogramCDF::binData(double val)
{
   assert(BIN_LIMITS_KNOWN & state);
	vector<double>::iterator         lb = lower_bound(limits.begin(), limits.end(), val);
	vector<double>::difference_type dif = distance(limits.begin(), lb);
   ++counts[dif];
}
void HistogramCDF::binData(std::vector<double> & val)
{
   for ( size_t i=0; i<val.size(); ++i)
      binData(val[i]);
}
//=======================================================================================
//=======================================================================================
//=======================================================================================
//=======================================================================================
   ////////This nomenclature follows from GsTL....
   ////////vector<float> Z;  // On reading, Z is the value of the variable (and NOT the normal transform).
   ////////vector<float> P;  // P[i] is the cumulative probability of the cdf corresponding to Z[i].
   ////////npcdf.z_set(Z.begin(), Z.end());
   ////////npcdf.p_set(P.begin(), P.end());
   ////////if (npcdf.make_valid())
   ////////   state = 0;//S_GOOD;

