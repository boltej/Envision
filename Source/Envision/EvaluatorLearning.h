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
#define NOMINMAX
#include "HistogramCDF.h"
#ifndef __INI_TEST__
#include <EnvModel.h>
#endif
#include <string>
#include <memory>
#include <vector>
#include <map>
using std::map;
using std::vector;
using std::string;
using std::auto_ptr;
#undef NOMINMAX
//=======================================================================================
/*   USE CASES:
   1. ***Read Envision.ini and set EnvEvaluator gain and offset.
      EvaluatorLearningIniFile::ReadIni()
      EvaluatorLearningIniFile::SetModelInfo()
   2. ***Calculate Gain and Offset
      EvaluatorLearning::AdUbiOmniHoc()
   3. ***Update Envision.ini
      EvaluatorLearningIniFile::SetModelInfoLine()
*/
// Every model that is used in either or both altruism and self-interest is a metagoal.
// So metagoal and scarcity semantically are not identical. 
// LOTS OF CODE TO MAKE SURE DEALING WITH SAME SET OF MODELS.

// ISSUE: Envision DEVIANT-STYLE CODING:  
//    capitalize member function
//    hungarian notation
//    indentation
//    openspace abundance
//    | dangerously omit const in arg to copy ctor and operator=
//    | to screw up conscientious code 
//======================================================================================= 
// For unit testing define. (INTELLISENSE GETS SCREWED UP UNLESS YOU ALSO COMMENT OUT WHEN NOT UNIT-TESTING) 
////
//#ifdef  __INI_TEST__  //========//
//class EnvEvaluator {            //
//public:                       //
//   CString         name;       //
//   float           gain;         //
//   float           offset;          //
//   auto_ptr< Non_param_cdf<> > apNpCdf; //
//   };                                    //
//class EnvModel {                          //
//public:                                         //
//   static vector <EnvEvaluator> mi;                   //
//   static EnvEvaluator *FindModelInfo( LPCTSTR name );    //  
//   static int FindModelIndex(LPCTSTR name);                //
//   };                                                        //
//class Policy {                                                //
//public:                                                        //
//   EnvPolicy() : m_name("nul"), m_id(-1) {}                       //
//   EnvPolicy(LPCTSTR name, int id) : m_name(name), m_id(id) {}   //
//   int m_id;                                                //
//   CString m_name;                                        //
//   };                                                   //
//#endif                                                //
//
//======================================================================================= 
// Accumulator of learned information.  From Sandbox, operator() is called repeatedly
// and processes data about model behavior that yields statistical knowledge.
class EvaluatorLearningStats 
{
public:
   enum {
      MIN=0, 
      MAX, 
      N_STATS
      };
   // Each vector cell corresponds to each Model::m_scarcityToModelMap cell. 
   CArray< CArray<double, double>, CArray<double, double> & > m_stats;      // 
   CArray< EnvEvaluator*, EnvEvaluator* >                         m_modelInfos; //
   CArray< HistogramCDF, HistogramCDF & >                     m_densities;

   int m_evalFreq;
   int m_yearsToRun;
   int m_percentCoverage;
   bool DoEvalYearBase0(int year) const;
   CString Trace() const ;

   EvaluatorLearningStats(INT_PTR N); // number of models to keep stats for

   // operator () calls other methods, e.g. MaxMin()
   void operator()( size_t model_index, double value );  
  
   // MaxMin arg, idx, is the first index in m_stats corresponding to EnvEvaluator.
   // MaxMin accumulates the max & min stat over a series of calls
   void MaxMin( size_t idx, double value );

   // Uses accumulated Stats and writes to m_modelInfos
   void SetGainOffset( );

   // Completes HistogramCDF stuff and updates EnvEvaluator
   void SetCDF( );
};
//======================================================================================= 
// READ/WRITE OF Envision.ini, esp wrt EnvEvaluator and gain, offset
class EvaluatorLearningIniFile
   {
   public:
   const string KEY_models  ;                 // [models] section of the ini file we want to process.
   const string KEY_cdfs    ;                 // [cdfs] ditto
   const string iniF  ;                       // Envision.ini
   enum {GAIN_TOK = 8, OFFSET_TOK, END_TOK};  // the field positions of interest in the record.
   enum {INIT=0, REPARSE=128,
         BEGIN_SECTION_MODELS=1, BEGIN_SECTION_CDFS=2}; // state for parsing the Envision.in.

   EvaluatorLearningIniFile() : KEY_models("[models]"), KEY_cdfs("[cdfs]"), iniF("Envsion.ini") {}

	vector<string> llines;                  // simply all the lines of the Envision.ini file.
   map<string, int> modelNameToFileLlines; // <EnvEvaluator::name, idx for llines> for the [models] section
   map<string, int> modelNameToFileLlinesCDF; // ditto but for the [cdfs] section

   // Rewrites lline to incorporate the new values of gain and offset (passed in).
   void SetModelInfoLline(const char * modelName, float gain, float offset) ;
   //
   void SetModelInfoLlineCDF( const char * modelName, const Non_param_cdf<> & cdf);

   // IF ifn != NULL, then use that path instead of standard.  
   // Reads the Envision.ini file; stores it in llines; assigns modelNameToFileLlines;
   void ReadIni(const char * ifn = NULL);
   // Simply writes llines out to Envision .ini
   void WriteIni(const char * ifn=NULL) const;

   // initializes global EnvEvaluator structs.
   void SetModelInfo() const;

   // Tokenizes a line from the Envision.ini that is a EnvEvaluator record.
   // If bPutGainOffset is true, then args, gain and offset, are put into the toks;
   // and if false then gain and offset return what was in the line.
   void TokenizeModelLine(const string & line, vector<string> & toks, 
      float & gain, float & offset, bool bPutGainOffset=false) const;

   ////void CdfFromInputLine( Non_param_cdf<> & cdf, const string & line );
   ////void OutputLineFromCdf( string & line, const Non_param_cdf<> & cdf, const char * modelName );

   };
//======================================================================================= 
// Use case for learning.  Interacts with Scenario
class EvaluatorLearning
{
public:
   auto_ptr<EvaluatorLearningStats> emStats;
   auto_ptr<EvaluatorLearningIniFile> emIniFile;

   EvaluatorLearning(void);
   ~EvaluatorLearning(void);

   // Use Case: For all scenarios over all policies and all time learn this
   void adUbiOmniHoc(CWnd * pWndParent);
};
