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
#include "EnvLoader.h"
#include "EnvModel.h"
#include <libs.h>
#include <MAP.h>
#include <Maplayer.h>
#include <no_mfc_files/NoMfcFiles.h>
#include <iostream>
#include <vector>
#include <algorithm>


//==================== GLOBALS =================
//
EnvLoader gLoader;
EnvModel* gpModel;
Map gMap;
PolicyManager* gpPolicyManager;
ActorManager* gpActorManager;
ScenarioManager* gpScenarioManager;
bool gUseTextColor;

namespace CLIComponents
{

  enum 
    { 
      //Envision commands
      CMD_LOADPROJECT=0
     ,CMD_EXPORT
     ,CMD_QUERY
     ,CMD_OUTPUT
     ,CMD_FROMFILE
     ,CMD_RUN
     ,CMD_MULTIRUN

      //CLI commands
     ,CMD_VERBOSE
     ,CMD_TXTCOLOR
    };

//copied from Envision.cpp
struct CLI 
   {
   int command; 
   CString argument;

     CLI( void ) : command( 0 ) { }
   CLI( int cmd, const char* arg ) : command( cmd ), argument( arg ) { }
   };

typedef std::vector<CLI> CLIParamArray;

//
//==================== PROTOS ==================
//

  inline CLI* FindCmd(CLIParamArray & array,int command);
  inline void ParseEnvisionParam(CLI & argEntry,const char* param, const bool & isSwitch);
  inline void CollectEnvisionParams(CLIParamArray & argArray,const int & argc, const char** argv);
  inline bool ProcessArgs(CLIParamArray & argArray);

void ReportMsg( LPCTSTR msg, LPCTSTR hdr, int type );
void InfoMsg( LPCTSTR msg );
  void StartUp();
};


//
//==================== MAIN ====================
//
int main(const int argc, const char** argv)
{
  using namespace CLIComponents;

  CLIParamArray args;

  gpModel           = new EnvModel;
  gpPolicyManager   = new PolicyManager;
  gpActorManager    = new ActorManager;
  gpScenarioManager = new ScenarioManager;
  
  gUseTextColor=false;

  
  CollectEnvisionParams(args,argc,argv);
  ProcessArgs(args);

  //EnvModel testEM;

  delete gpModel;
  delete gpPolicyManager;
  delete gpActorManager;
  delete gpScenarioManager;

  return 0;
}

//
//============= SUPPORT FUNCS ==================
//

//convenience for finding args
CLIComponents::CLI* CLIComponents::FindCmd(CLIParamArray & array,int command)
  {
    auto itr=std::find_if(array.begin(),array.end(),[&command](CLI & cli){return cli.command==command;});
    return itr!=array.end() ? &(*itr) : NULL;
  }

//evaluate single param
void CLIComponents::ParseEnvisionParam(CLI & argEntry,const char* param, const bool & isSwitch)
{
  if(param==NULL)
    return;

  if(isSwitch)
    {
      const char* arg=param+3;

      //format: /c:arg or -c:arg
      switch(param[1])
	{
	case 'e':
	  argEntry=CLI(CMD_EXPORT,arg);
	  break;
	case 'q':
      	  argEntry=CLI(CMD_QUERY,arg);
	  break;
	case 'o':
	  argEntry=CLI(CMD_OUTPUT,arg);
	  break;
	case 'c':
	  argEntry=CLI(CMD_FROMFILE,arg);
	  break;
	case 'r':
	  argEntry=CLI(CMD_RUN,arg);
	  break;
	case 'm':
	  argEntry=CLI(CMD_MULTIRUN,arg);
	  break;
	case 'v':
	  argEntry=CLI(CMD_VERBOSE,arg);
	  break;
	case 't':
	  argEntry=CLI(CMD_TXTCOLOR,arg);
	  break;
	default:
	  break;
	}
    }
  else
    {
      argEntry=CLI(CMD_LOADPROJECT,param);
    }
}

//parse out command args
void CLIComponents::CollectEnvisionParams(CLIParamArray & argArray,const int & argc, const char** argv)
{
  //argv[0] is the invocation; skip

  //size argArray based on argc
  argArray.resize(argc-1);
  for(int i=1; i<argc;++i)
    ParseEnvisionParam(argArray[i-1],argv[i],(argv[i][0]=='-' || argv[i][0]=='/'));
}

//do something with args
bool CLIComponents::ProcessArgs(CLIParamArray & argArray)
{
  if (argArray.empty())
    return true;

  CLI *pCmdLoadProject = FindCmd(argArray,CMD_LOADPROJECT);
  CLI *pCmdExport      = FindCmd(argArray,CMD_EXPORT);
  CLI *pCmdQuery       = FindCmd(argArray,CMD_QUERY);
  CLI *pCmdOutput      = FindCmd(argArray,CMD_OUTPUT); 
  CLI *pCmdFromFile    = FindCmd(argArray,CMD_FROMFILE);
  CLI *pCmdRun         = FindCmd(argArray,CMD_RUN);
  CLI *pCmdMultiRun    = FindCmd(argArray,CMD_MULTIRUN);
  CLI *pCmdVerbose    = FindCmd(argArray,CMD_VERBOSE);
  CLI *pCmdTxtColor    = FindCmd(argArray,CMD_TXTCOLOR);

   if(pCmdVerbose)
     {
       //write any messages to commandline
       Report::reportMsgProc=ReportMsg;
       Report::statusMsgProc=InfoMsg;
     }
   if(pCmdTxtColor)
     //will only work if verbose is enabled
     gUseTextColor=true;

   // /r:scenarioIndex (-1 for all scenarios)
   if ( pCmdLoadProject )
      {
	gLoader.LoadProject(pCmdLoadProject->argument,&gMap,gpModel,gpPolicyManager,gpActorManager,gpScenarioManager);
      }

   if ( pCmdFromFile )
      {
      // have xml string, start parsing
      TiXmlDocument doc;
      bool ok = doc.LoadFile( pCmdFromFile->argument );

      if ( ! ok )
         {
	   std::cerr<<"Error reading configuration file, "<< pCmdFromFile->argument<<": "<< doc.ErrorDesc()<<std::endl;
         return false;
         }

      // start interating through the nodes
      TiXmlElement *pXmlRoot = doc.RootElement();     // <command_info>
      
      TiXmlElement *pXmlCmd = pXmlRoot->FirstChildElement();

      while ( pXmlCmd != NULL )
         {
         CString cmd( pXmlCmd->Value() );

         //------ export command ----------------
         if ( cmd.CompareNoCase( _T("export" )) == 0 )     // export command?
            {
            LPCTSTR layer = NULL;
            LPCTSTR query = NULL;
            LPCTSTR output = NULL;
            
            XML_ATTR attrs[] = 
               { // attr      type          address  isReq checkCol
               { "layer",     TYPE_STRING,   &layer,  true,   0 },
               { "query",     TYPE_STRING,   &query,  true,   0 },
               { "output",    TYPE_STRING,   &output, true,   0 },
               { NULL,        TYPE_NULL,     NULL,    false,  0 } };
  
            bool ok = TiXmlGetAttributes( pXmlCmd, attrs, pCmdFromFile->argument );
            if ( layer == NULL || query == NULL  || output == NULL )
               return false;

            MapLayer *pLayer = gMap.AddShapeLayer( layer, true );
            if ( pLayer == NULL )
               return false;

            QueryEngine qe( pLayer );
            Query *pQuery = qe.ParseQuery( query, 0, "Envision Command Line" );

            qe.SelectQuery( pQuery, false ); //, false );
            pLayer->SaveShapeFile( output, true );
            }

         //else if ( cmd.CompareNoCase( _T( "some other command" )
         //   ;

         pXmlCmd = pXmlCmd->NextSiblingElement();
         }  // end of: while ( pXmlCmd != NULL )

      return false;
      }  // end of: cmd = conigure from file

   // /r:scenarioIndex (-1 for all scenarios)
   int scen_start=gpScenarioManager->GetDefaultScenario();
   int scen_count=scen_start+1;

   if ( pCmdRun )
      {
      if ( isdigit( pCmdRun->argument[0] ) == 0 )
         {
	   std::cerr<< "Bad parameter passed to command line /r:scnindex"<<std::endl;
         return false;
         }

      int index = atoi( pCmdRun->argument );

      if(index>=0)
	{
	  if(index==0)//run all scenarios
	    {
	      scen_start=0;
	      scen_count=gpScenarioManager->GetCount();
	    }
	  else
	    {
	      scen_start=index-1;
	      scen_count=index;
	    }
	}
      
      // gpDoc->m_cmdRunScenario = index;  // (Note: this is one-based, 0=run all)
      }

   int runCount=1;
   // /m:scenarioIndex (-1 for all scenarios)
   if ( pCmdMultiRun )
      {
      if ( isdigit( pCmdMultiRun->argument[0] ) == 0 )
         {
	   std::cerr<< "Bad parameter passed to command line /r:scnindex" <<std::endl;
         return false;
         }

      runCount = atoi( pCmdMultiRun->argument );
      if(runCount<1)
	runCount=1;
      }


   if(runCount<=1)
     {
       //... ?
     }
   else
     {
       //... ?
     }

   //Perform start routines that EnvDoc would normally carry out
   StartUp();

   //start year?
   gpModel->m_currentYear=gpModel->m_startYear;
   //years to run?
   gpModel->m_endYear = gpModel->m_startYear+gpModel->m_yearsToRun;
   //export interval?

   gpModel->m_iterationsToRun=runCount;
   for(int i=scen_start; i<scen_count; i++)
     {
       Scenario *pScen=gpScenarioManager->GetScenario(i);
       gpModel->SetScenario(pScen);
       gpModel->Reset();
       if(gpModel->m_areModelsInitialized==false)
	 gpModel->InitModels();
       int cellCount=gpActorManager->GetManagedCellCount();
       if(!cellCount)
	 cellCount=1;

       
       runCount>1 ? gpModel->RunMultiple() : gpModel->Run(0);
     }
	   

   return true;

}

void CLIComponents::ReportMsg( LPCTSTR msg, LPCTSTR hdr, int type )
{
  using namespace std;

  //TODO: add flag to disable the use of color codes

  //color codes
  const char* CLEAR_CODE="\x1b[0m";
  const char* BLACK_CODE="\x1b[30m";
  const char* RED_CODE="\x1b[31m";
  const char* GREEN_CODE="\x1b[32m";
  const char* YELLOW_CODE="\x1b[33m";
  const char* BLUE_CODE="\x1b[34m";  
  const char* MAGENTA_CODE="\x1b[35m";
  const char* CYAN_CODE="\x1b[36m";  
  const char* WHITE_CODE="\x1b[37m";

  if(gUseTextColor)
    {
      switch(type)
	{
	case RT_INFO:
	  //no color
	  break;
	case RT_WARNING:
	  //yellow color
	  cout<<YELLOW_CODE;
	  break;
	case RT_ERROR:
	case RT_FATAL:
	case RT_SYSTEM:
	  //red color
	  cout<<RED_CODE;
	}
    }
  cout<<hdr<<": "<<msg;

  if(gUseTextColor)
    cout<<CLEAR_CODE;

  cout<<endl;
}


void CLIComponents::InfoMsg( LPCTSTR msg )
{
  ReportMsg(msg,"Info",RT_INFO);
}

void CLIComponents::StartUp()
{
   for ( int i=0; i < EnvModel::GetActorValueCount(); i++ )
      {
      METAGOAL *pInfo =  EnvModel::GetActorValueMetagoalInfo( i );
      CString label( pInfo->name );
      label += " Actor Value";

      CString field;
      field.Format( "ACTOR%s", pInfo->name );
      if (field.GetLength() > 10)
         field.Truncate(10);
      field.Replace( ' ', '_' );

      gpModel->m_pIDULayer->AddFieldInfo( field, 0, label, _T(""), _T(""), TYPE_FLOAT, MFIT_QUANTITYBINS, BCF_GREEN_INCR , 0, true );
      }
    
   int defaultScenario = gpScenarioManager->GetDefaultScenario();
   gpModel->SetScenario( gpScenarioManager->GetScenario( defaultScenario ) );  /* -- */
   gpModel->Reset();

   int levels = EnvModel::m_lulcTree.GetLevels();

   switch( levels )
      {
      case 1:  gpModel->m_pIDULayer->CopyColData( EnvModel::m_colStartLulc, EnvModel::m_colLulcA );   break;
      case 2:  gpModel->m_pIDULayer->CopyColData( EnvModel::m_colStartLulc, EnvModel::m_colLulcB );   break;
      case 3:  gpModel->m_pIDULayer->CopyColData( EnvModel::m_colStartLulc, EnvModel::m_colLulcC );   break;
      case 4:  gpModel->m_pIDULayer->CopyColData( EnvModel::m_colStartLulc, EnvModel::m_colLulcD );   break;
      }


}
