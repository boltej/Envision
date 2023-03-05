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
#include "stdafx.h"
#define NOMINMAX
#include <map>
#include <utility>
#include <sstream>
#include <limits.h>
#include <float.h>
#include <fstream>
#include <vector>
using std::vector;
using std::ifstream;
using std::istringstream;
using std::ostringstream;
using std::pair;
using std::map;
#include <direct.h>

#include ".\evaluatorlearning.h"
#include "EvaluatorLearnDlg.h"

#ifndef __INI_TEST__                         
#include <EnvException.h>
#include "Sandbox.h"                         
extern PolicyManager   *gpPolicyManager ;    
extern ScenarioManager *gpScenarioManager;   
extern EnvModel        *gpModel;             
#endif     
#undef NOMINMAX
////===================================================// 
//#ifdef __INI_TEST__                                  //
//class EnvException {                                 //
//public:                                              //
//   EnvException(const char *) {}                     //
//   };                                                //
//                                                     //
//vector <EnvEvaluator> gpModel->mi;                    //
//EnvEvaluator * gpModel->FindModelInfo( LPCTSTR name )  //
//   {                                                 //
//   gpModel->mi.push_back(EnvEvaluator());             //
//   return & gpModel->mi.back();                     //
//   }                                                 //
//int gpModel->FindModelIndex(LPCTSTR name)            //
//{                                                    // 
//   return 1;                                         //
//}                                                    //
////                                                   //
//#endif                                               //
//=======================================================================================
void EvaluatorLearningStats::SetGainOffset( )
   {
   //const double EMX=3;
   //const double EMN=-3;
   //const double ERNG = EMX-EMN;
   //double range;
   //for(int i=0; i<m_stats.GetCount(); ++i)
   //   {
   //   range = m_stats[i][MAX] - m_stats[i][MIN];
   //   if (range != 0.0)
   //      m_modelInfos[i]->gain = (float) (ERNG/range);
   //   else
   //      m_modelInfos[i]->gain = 1.f;
   //
   //   m_modelInfos[i]->offset = (float)(-0.5*(m_stats[i][MAX] + m_stats[i][MIN]));      
   //   }
   }
//----------------------------------------------------------------------------------------
void EvaluatorLearningStats::SetCDF( )
   {
   for(int i=0; i<m_densities.GetCount(); ++i)
      {
      m_densities.GetAt(i).convertToCDF();           

      //m_modelInfos[i]->apNpCdf = m_densities.GetAt(i).npcdf; 
      }
   }
//----------------------------------------------------------------------------------------
EvaluatorLearningStats::EvaluatorLearningStats(INT_PTR N)
   {
   m_stats.SetSize(N);
   for(int i=0; i<m_stats.GetSize(); ++i)
      {
      m_stats[i].SetSize(N_STATS);
      for (int j=0; j<m_stats[i].GetSize(); ++j)
         {
         m_stats[i][MIN] = DBL_MAX;
         m_stats[i][MAX] = -DBL_MAX;
         }
      }
   }   
//----------------------------------------------------------------------------------------
bool EvaluatorLearningStats::DoEvalYearBase0(int year) const
   {
   return (0 == year % m_evalFreq);
   }
//----------------------------------------------------------------------------------------
void EvaluatorLearningStats::operator()( size_t model_index, double value )
   {
   MaxMin(model_index, value);
   m_densities.GetAt( model_index ).accumData(value);
   }
//----------------------------------------------------------------------------------------
void EvaluatorLearningStats::MaxMin( size_t idx, double value ) 
   {
   if (m_stats[idx][MIN] > value) m_stats[idx][MIN] = value;
   if (m_stats[idx][MAX] < value) m_stats[idx][MAX] = value;
   }
//----------------------------------------------------------------------------------------
CString EvaluatorLearningStats::Trace( ) const
   {
   ASSERT(m_stats.GetSize() == m_modelInfos.GetSize());
   ostringstream oss;
   for(int i=0; i<m_stats.GetSize(); ++i)
      { 
      oss << m_modelInfos[i]->m_name << "(" ;

      for(int j=0; j<m_stats[i].GetSize(); ++j)
         {
         oss << m_stats[i][j];
         if (j<m_stats[i].GetSize()-1)
            oss << ", " ;
         }
      oss << ") ";
      }
   CString out = oss.str().c_str();
   return out;
   }
//=======================================================================================
void EvaluatorLearningIniFile::SetModelInfoLline(const char * modelName, float gain, float offset) 
   {
   map<string, int>::iterator  mit;
   mit = modelNameToFileLlines.find(modelName);
   if (modelNameToFileLlines.end() == mit)
      return;

   string line = llines[mit->second];

   vector<string> tokens;
   TokenizeModelLine( line, tokens, gain, offset, true);  // gain and offset are written to tokens

   ostringstream oss;
   unsigned i;
   for (i=0; i< tokens.size()-1; ++i)            
      oss << tokens[i] << ",";
   oss << tokens[i]; 

   llines[mit->second] = oss.str();
   }
//---------------------------------------------------------------------------------------
// Assumes [cdfs] section is at tail end of Envision.ini.
// Appends missing lines.
//
void EvaluatorLearningIniFile::SetModelInfoLlineCDF( const char * modelName, const  Non_param_cdf<>  & cdf)
{
   string out;
   HistogramCDF::writeQuantiles(out, cdf, modelName);

   map<string, int>::iterator  mit;
   mit = modelNameToFileLlinesCDF.find(modelName);

   if (modelNameToFileLlinesCDF.end() == mit)
   {
      llines.push_back(out);
      modelNameToFileLlinesCDF.insert(pair<string, int>(modelName, int( llines.size())-1));
   }
   else
   {
      llines[mit->second] = out;
   }
}
//----------------------------------------------------------------------------------------
void EvaluatorLearningIniFile::TokenizeModelLine(const string & line, vector<string> & tokens,
                                                 float & gain, float & offset,
                                                 bool bPutGainOffset) const
   {
   int tokN;
   size_t pos, posn;
   istringstream iss;

   tokens.assign(END_TOK, "");
   pos  = posn = tokN = 0;
   while(tokN < END_TOK)
      {
      if (string::npos == (posn = line.find(',', pos)))
         {
         tokens[tokN++] = line.substr(pos);
         break;
         }
      else 
         {
         tokens[tokN++] = line.substr(pos, posn-pos);
         pos = posn+1;
         }
      }

   if (false == bPutGainOffset)
      {   
      gain = offset = FLT_MAX;
      if (tokN == END_TOK)
         {
         iss.clear();
         iss.str(tokens[GAIN_TOK]);
         iss >> gain;

         iss.clear();
         iss.str(tokens[OFFSET_TOK]);
         iss >> offset;
         }
      else if (tokN <= GAIN_TOK)
         {
         gain = offset = FLT_MAX;
         }

      if (FLT_MAX == gain || FLT_MAX == offset)   
         {
         gain = 1.f;                offset = 0.f; 
         tokens[GAIN_TOK] = "1.0";  tokens[OFFSET_TOK] = "0.0";
         }
      }
   else
      {
      ostringstream oss;
      oss << gain;
      tokens[GAIN_TOK] = oss.str();
      oss.str("");
      oss << offset;
      tokens[OFFSET_TOK] = oss.str();
      }
   }
//-----------------------------------------------------------------------------------------
void EvaluatorLearningIniFile::SetModelInfo() const
   {
   float gain, offset;
   vector<string> tokens;                 
   map<string, int>::const_iterator mit;
   map<string, int>::const_iterator cit;
   for (mit = modelNameToFileLlines.begin(); mit != modelNameToFileLlines.end(); ++mit)
      {
      string line = llines[mit->second];
      TokenizeModelLine( line, tokens, gain, offset); // gets gain, offset

      // SET EnvEvaluator
      EnvEvaluator * mi = gpModel->FindEvaluatorInfo(tokens[0].c_str());
      if (mi) 
         {
         //mi->gain = gain;
         //mi->offset = offset;

         //cit = modelNameToFileLlinesCDF.find(tokens[0].c_str());
         //if (cit != modelNameToFileLlinesCDF.end())
         //   {
         //   Non_param_cdf<> * npcdf = new Non_param_cdf<>();
         //   HistogramCDF::readQuantiles(*npcdf, llines[cit->second]);
         //   mi->apNpCdf = auto_ptr< Non_param_cdf<> > ( npcdf );
         //   }
         }
      }
   }
//----------------------------------------------------------------------------------------
// Small Streams,              4, ecosystemhealth.dll,     0,      1,      1,  SmStrms, ecosystemhealth.dat
//
void EvaluatorLearningIniFile::ReadIni(const char * ifn)
   {
   float gain, offset;
   vector<string> tokens(END_TOK);
   string fn;
   string key;
   string line;
   istringstream iss;
   string cwd(_MAX_PATH, '\0');
   _getcwd( &cwd[0], _MAX_PATH );

   fn = (NULL != ifn) ? ifn : iniF;

   ifstream in(fn.c_str(),  std::ios_base::in );
   if (! in )  
      {
      ostringstream oss;
      oss << "Can't open for reading the Essential Envision File, " << iniF 
         << " in the current directory, " << cwd;
      throw new EnvException(oss.str().c_str());  //microsoft exception idiom requires new
      } 

   // Reinitialize
   llines.clear();
   modelNameToFileLlines.clear();
   modelNameToFileLlinesCDF.clear();

   int state = INIT;

   while (! in.eof() ) 		
      {
      if ( ! (REPARSE & state) )
         {
         line.erase();
         key.erase();
         std::getline(in, line);
         iss.clear();
         iss.str(line);
         iss >> key;

         llines.push_back(line);  // put every line of the .ini here
         }
      else
         {
         state &= ~REPARSE;
         }

      if (key.empty() || 0 == key.find("//")) 
         {
         continue;
         }
      else if (0 == key.compare(KEY_models))  // ^[key] is sought
         {
         state |= BEGIN_SECTION_MODELS;
         state &= ~REPARSE;
         continue;
         }
      else if (0 == key.compare(KEY_cdfs))  // ^[key] is sought
         {
         state |= BEGIN_SECTION_CDFS;
         state &= ~REPARSE;
         continue;
         }
     else if (BEGIN_SECTION_CDFS & state)  // this is a section we process
         {
         if (0 == key.find('[') && string::npos != key.find(']'))
            {
            state &= ~BEGIN_SECTION_CDFS;
            state |= REPARSE;
            continue;
            }
         // 
         HistogramCDF hcdf;
         hcdf.readQuantiles(line);
         modelNameToFileLlinesCDF.insert(pair<string,int>(hcdf.name, (int)llines.size()-1));
         }
      else if (BEGIN_SECTION_MODELS & state)  // this is a section we process
         {
         if (0 == key.find('[') && string::npos != key.find(']'))
            {
            state &= ~BEGIN_SECTION_MODELS;
            state |= REPARSE;
            continue;
            }
         
         TokenizeModelLine(line, tokens, gain, offset);
         
         // REWRITE THE LINE
         llines.pop_back();
         ostringstream oss;
         unsigned i;
         for (i=0; i< tokens.size()-1; ++i)            
            oss << tokens[i] << ",";
         oss << tokens[i]; // << std::endl;
         llines.push_back(oss.str());

         modelNameToFileLlines.insert(pair<string, int>(tokens[0].c_str(), (int)llines.size()-1));
         }
      }
   }
//----------------------------------------------------------------------------------------
////void EvaluatorLearningIniFile::CdfFromInputLine( Non_param_cdf<> & cdf, const string & line )
////   {
////   HistogramCDF hcdf;
////   hcdf.readQuantiles(line);  
////   cdf = *(hcdf.npcdf);
////
////   }
//----------------------------------------------------------------------------------------
////void EvaluatorLearningIniFile::OutputLineFromCdf( string & line, const Non_param_cdf<> & cdf, 
////                                                 const char * modelName )
////   {
////   HistogramCDF::writeQuantiles(line, cdf, modelName);
////   }
//----------------------------------------------------------------------------------------
void EvaluatorLearningIniFile::WriteIni(const char * ifn) const
   {
   string fn;
   string cwd(_MAX_PATH, '\0');
   _getcwd( &cwd[0], _MAX_PATH );

   fn = (NULL != ifn) ? ifn : iniF;

   std::ofstream out(fn.c_str(), std::ios_base::binary | std::ios_base::trunc);

   if (! out )  
      {
      ostringstream oss;
      oss << "Can't open for writing the Essential Envision File, " << iniF 
         << " in the current directory, " << cwd;
      throw new EnvException(oss.str().c_str()); //microsoft exception idiom requires new
      }  
   for (unsigned i=0; i<llines.size(); ++i)
      out << llines[i] << "\r\n";//std::endl; 
   }
//=======================================================================================
EvaluatorLearning::EvaluatorLearning(void)
   {
   }
//----------------------------------------------------------------------------------------
EvaluatorLearning::~EvaluatorLearning(void)
   {
   }
//----------------------------------------------------------------------------------------
void EvaluatorLearning::adUbiOmniHoc(CWnd * pWndParent)
   {
#ifndef __INI_TEST__
   ASSERT(gpScenarioManager);
   ASSERT(gpPolicyManager);
   ASSERT(gpModel);

   EvaluatorLearnDlg emlDlg;
   INT_PTR nRet = -1;
   nRet = emlDlg.DoModal();
   switch ( nRet )
      {
      case -1: 
         AfxMessageBox("Dialog box could not be created!");
         break;
      case IDABORT:
         return;
         break;
      case IDOK:
         break;
      case IDCANCEL:
         return;
         break;
      default:
         ASSERT(0);
         break;
      }
   EnvPolicy * pPolicy = NULL;
   int polID = -1;
   set<Scenario*>    & scenarioSet  =       emlDlg.m_scenarioSet;
   set<int>             & modelSet  =       emlDlg.m_idxModelInfo;
   map< int, EnvPolicy * > & policySet =       emlDlg.m_policySet;
   pair< map<int,EnvPolicy*>::iterator, bool > polPr;
   map< int, EnvPolicy * >::iterator           polIt;
   EnvEvaluator * mi = NULL;

   CArray<EnvPolicy *>  scenSchedPol;  

   CWaitCursor c;

   // GET THE Evaluators' statistics, e.g. MX MN CDF, overall policySet overal time.
   // Setup which models we want to run.   Including neutralizing any previous learning 
   // results.
   //
   CArray<bool, bool> modelFlags;// We use GetData to get bool[]; DO NOT USE specialization, vector<bool>.
   int modelCount = gpModel->GetEvaluatorCount();
   int selCount = (int) modelSet.size();
   emStats = auto_ptr<EvaluatorLearningStats>(new EvaluatorLearningStats(selCount));
   int m = -1, i, j;
   for ( i=0, j=0; i<modelCount; ++i) 
      {   
      modelFlags.SetAtGrow(i, false);
      if (modelSet.end() != modelSet.find(i))
         {
         modelFlags.SetAt(i, true); 
         mi = gpModel->GetEvaluatorInfo(i); 
         emStats->m_modelInfos.SetAtGrow( j, mi );
         //mi->gain = 1.0;
         //mi->offset = 0.0;

         emStats->m_densities.SetAtGrow( j , HistogramCDF());
         emStats->m_densities.GetAt( j ).name = (LPCTSTR) mi->m_name;
         //mi->apNpCdf.release();
         j++;
         }
      }

   int policyCount   = (int) policySet.size();
   bool bCallInitRun = !(gpModel->m_currentRun > 0);

   emStats->m_yearsToRun = 75;
   emStats->m_percentCoverage = 33;
   emStats->m_evalFreq = 10;

   Sandbox * pSandbox = NULL;

   int  scenCount = (int) scenarioSet.size(); 
   int  scenVarCount = -1;
   assert(scenCount >= 1);
   Scenario * pScenario = NULL; // can't const because Scenario has no const members

   // ITERATE OVER EACH SCENARIO SUCH THAT ITS SCHEDULED POLICIES CAN BE RUN WITH EACH 
   // POLICY FROM THE POLICY SET THAT THE SCENARIO USES.

   for( set<Scenario*>::iterator i = scenarioSet.begin(); i != scenarioSet.end(); ++i) 
      {
      scenSchedPol.RemoveAll();

      pScenario = *i;
      //scenVarCount = pScenario->GetVarCount();
      int scenPolCount = pScenario->GetPolicyCount();

      // GET THIS SCENARIO'S SCHEDULED POLICIES

      for (int i=0; i < scenPolCount; ++i)
         {
         polID = -1;
         pPolicy = NULL;
         POLICY_INFO &pi = pScenario->GetPolicyInfo( i );

         if ( pi.inUse == false) 
            continue;

         pPolicy = pi.pPolicy;
         polID  = pPolicy->m_id;

         // Take all scheduled policies.
         //
         //if (policySet.end() == policySet.find(polID))
         //   continue;

         if (pPolicy->m_isScheduled)
            scenSchedPol.Add(pPolicy);        
         }

      // RUN SANDBOX FOR EACH THIS SCENARIO'S POLICIES THAT CAN BE FOUND IN THE policySet; 

      for (int j = 0; j < scenVarCount; ++j)
         {
         polID = -1;
         pPolicy = NULL;
         POLICY_INFO &pi = pScenario->GetPolicyInfo(j);

         if ( pi.inUse == false ) 
            continue;

         pPolicy = pi.pPolicy;
         polID  = pPolicy->m_id;

         if (policySet.end() == policySet.find(polID))
            continue;

         CString title = "Evaluating Policy " + pPolicy->m_name + CString(":") + emStats->Trace();
         pWndParent->SetWindowText( title );

         if ( pSandbox )
            delete pSandbox;

         pSandbox = new Sandbox( gpModel, emStats->m_yearsToRun, float(emStats->m_percentCoverage)/100.0f, bCallInitRun );
         // bCallInitRun = false;  // [ ] BUG fails if we dont reinit.

         pSandbox->CalculateGoalScores ( scenSchedPol, pPolicy, emStats.get(), modelFlags.GetData() );

         delete pSandbox;
         pSandbox = NULL;

         //SetWindowText( "running stats show" );
         }
      }
   //
   // Sandbox done; stats available. Use them
   //
   emStats->SetGainOffset(); // UPDATES THE ENV_EVAL_MODELs in the emStats with emStats
   //
   emStats->SetCDF();
   //
   emIniFile = auto_ptr<EvaluatorLearningIniFile>(new EvaluatorLearningIniFile());
   emIniFile->ReadIni();
   //
   for (int i=0; i<emStats->m_modelInfos.GetCount(); ++i)
      {
      EnvEvaluator * mi = NULL;
      mi = emStats->m_modelInfos[i];
      ASSERT(mi);

      // Expect all names in the Envision.ini file
      ASSERT (emIniFile->modelNameToFileLlines.end()!= emIniFile->modelNameToFileLlines.find((LPCTSTR)mi->m_name));

      //emIniFile->SetModelInfoLline(mi->name, mi->gain, mi->offset); 

      //emIniFile->SetModelInfoLlineCDF( mi->name, *mi->apNpCdf);

      }
   emIniFile->WriteIni();
#endif
   }
//=======================================================================================