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
// envcmd.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "envcmd.h"

#include <tixml.h>
#include <MAP.h>
#include <Maplayer.h>
#include <QueryEngine.h>
#include <Query.h>
#include <PtrArray.h>
#include <reachtree.h>
#include <Path.h>
#include <misc.h>
#include <EnvEngine.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;

int  ProcessCmdLine( int argc, TCHAR *argv[] );
bool ProcessCmdFile( LPCTSTR cmdFile );
void PrintUsage( void );

enum CMD { UNDEFINED=0, INPUTFILE, OUTPUTFILE, SUBSET, CONVERT, SAVEFIELDS, RUNCMDFILE, FIXTOPOLOGY, ADDFIELD }; 

struct CmdInfo
   {
   CMD command; 
   CString argument;

   CmdInfo( void ) : command( UNDEFINED ) { }
   CmdInfo( CMD cmd, LPCTSTR arg ) : command( cmd ), argument( arg ) { }
   };

CmdInfo *FindCmdInfo( CMD cmd );
int Subset( MapLayer *pLayer,  LPCTSTR queryStr );
int Convert(MapLayer *pLayer, LPCTSTR queryStr);
int FixTopology( MapLayer *pLayer, LPCTSTR uniqueIdField, PCTSTR outfile );
int AddField( MapLayer *pLayer,  LPCTSTR queryStr );
int SetFieldsToSave(MapLayer *pLayer, LPCTSTR fieldSpec);
int RunEnvision(LPCTSTR projectFile, int scenario);



// globals
PtrArray <CmdInfo> cmds;  // list of command line args


int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	HMODULE hModule = ::GetModuleHandle(NULL);

	if (hModule != NULL)
	{
		// initialize MFC and print and error on failure
		if (!AfxWinInit(hModule, NULL, ::GetCommandLine(), 0))
		{
			// TODO: change error code to suit your needs
			_tprintf(_T("Fatal Error: MFC initialization failed\n"));
			nRetCode = 1;
		}
		else
		{
      ProcessCmdLine( argc, argv );
		}
	}
	else
	{
		// TODO: change error code to suit your needs
		_tprintf(_T("Fatal Error: GetModuleHandle failed\n"));
		nRetCode = 1;
	}

	return nRetCode;
}

int ProcessCmdLine( int argc, TCHAR *argv[] )
   {
   if ( argc < 2 )
      {
      PrintUsage(); 
      return -1;
      }

   // check for special case of project file
   TCHAR *cmd = argv[1];
   if (*cmd != '/')
      {
      // indicates this should run an Envision scenario
      // and the argument should be a project file, followed by
      // an '/r:' switch
      nsPath::CPath envxFile(cmd);
      CString ext = envxFile.GetExtension();

      if (ext.CompareNoCase("envx") == 0)
         {
         // get 'r' switch
         cmd = argv[2];
         if (*cmd == '/' && cmd[1] == 'r')
            {
            int scn = atoi(cmd + 3);   // skip the ':'!
            RunEnvision(argv[1], scn);
            return 1;
            }
         else
            {
            PrintUsage();
            return -1;
            }
         }

      else
         {
         PrintUsage();
         return -1;
         }
      }


   for ( int i=1; i < argc; i++ )
      {
      TCHAR *cmd = argv[ i ];
      if ( *cmd != '/' )
         {
         PrintUsage();
         return -1;
         }

      cmd += 1;  // skip the ':'
      LPTSTR arg = cmd+2;

      switch( *cmd )
         {
         case 'i':   // input shapefile
            cmds.Add( new CmdInfo( INPUTFILE, arg ) );
            break;
   
         case 'o':    // output file
            cmds.Add( new CmdInfo( OUTPUTFILE, arg ) );
            break;

         case 'q':     // subset based on query
         case 's':     // subset based on query (deprecated)
            cmds.Add( new CmdInfo( SUBSET, arg ) );
            break;
   
         case 'x':   // convert type
            cmds.Add(new CmdInfo(CONVERT, arg));
            break;

         case 'f':   // field list (for writing output, all is not included 
            cmds.Add(new CmdInfo(SAVEFIELDS, arg));
            break;
            
         case 'c':    // run command file
            cmds.Add( new CmdInfo( RUNCMDFILE, arg ) );
            break;

         case 't':    // fix topology
            cmds.Add( new CmdInfo( FIXTOPOLOGY, arg ) );
            break;
            
         case 'a':    // addfield
            cmds.Add( new CmdInfo( ADDFIELD, arg ) );
            break;

         default:
            break;
         }
      }

   // done reading args, now interpret and run
   CmdInfo *pCmdInfo = FindCmdInfo( RUNCMDFILE );  //   /c:cmdFile.xml
   
   // run from cmd file?
   if ( pCmdInfo ) 
      return ProcessCmdFile( pCmdInfo->argument );

   // otherwise run the specified command
   CmdInfo *pCmdInput = FindCmdInfo( INPUTFILE );
   if ( pCmdInput == NULL )
      {
      printf( "\nNo input shape file was specified!!!\n" );
      return -2;
      }

   CmdInfo *pCmdOutput = FindCmdInfo( OUTPUTFILE );
   if ( pCmdOutput == NULL )
      {
      printf( "\nNo output file was specified!!!\n" );
      return -3;
      }

   CString msg;
   msg.Format( "\nLoading input file '%s'", (PCTSTR) pCmdInput->argument );
   printf( msg );

   // open the input map
   Map map;
   MapLayer *pLayer = map.AddShapeLayer( pCmdInput->argument, true );
   if ( pLayer == NULL )
      return false;

   bool saveSelectedOnly = false;

   // process arguments
   CmdInfo *pCmdSubset = FindCmdInfo( SUBSET );
   if ( pCmdSubset )
      {
      Subset( pLayer, pCmdSubset->argument );
      saveSelectedOnly = true;
      }

   CmdInfo *pCmdConvert = FindCmdInfo(CONVERT);
   if (pCmdConvert)
      {
      Convert(pLayer, pCmdConvert->argument);
      }

   CmdInfo *pCmdFix = FindCmdInfo( FIXTOPOLOGY );
   if ( pCmdFix )
      FixTopology( pLayer, pCmdFix->argument, pCmdOutput->argument );

   CmdInfo *pCmdAddField = FindCmdInfo( ADDFIELD );
   if ( pCmdAddField )
      AddField( pLayer, pCmdAddField->argument );
      
   int fieldsWritten = pLayer->GetFieldInfoCount(1);
   CmdInfo *pCmdSaveFields = FindCmdInfo(SAVEFIELDS);
   if (pCmdSaveFields)
      fieldsWritten = SetFieldsToSave(pLayer, pCmdSaveFields->argument);
   
   // save results
   msg.Format("\n  Saving output map to %s", pCmdOutput->argument );
   printf( msg );
   int recordsWritten = pLayer->SaveShapeFile( pCmdOutput->argument, saveSelectedOnly );

   printf( "\n  Saved %i records, %i fields to file %s\n\n", recordsWritten, fieldsWritten, (LPCTSTR) pCmdOutput->argument );  

   return 0;
   }


int Subset( MapLayer *pLayer, LPCTSTR queryStr )
   {
   CString msg;
   msg.Format("\n Subsetting - query: %s", queryStr );
   printf( msg );

   QueryEngine qe( pLayer );
   Query *pQuery = qe.ParseQuery( queryStr, 0, "Envcmd Query" );

   int count = qe.SelectQuery( pQuery, false );

   msg.Format("\n  %i polys selected...", count );
   printf( msg );

   printf( "\n  Subsetting complete" );
   return count;
   }



int Convert(MapLayer *pLayer, LPCTSTR arg)
   {
   CStringArray args;
   ::Tokenize(arg, "|", args);
   
   CString field = args[0];
   CString binSpec;
   if (args.GetSize() > 1)
      binSpec = args[1];

   CString msg;
   msg.Format("\n Convert layer: field [%s]", args[0]);
   printf(msg);

   int ptCount = pLayer->GetPolygonCount();

   pLayer->ConvertToLine(field, binSpec);

   int lineCount = pLayer->GetPolygonCount();
   printf("\n  Conversion complete: %i points became %i lines", ptCount, lineCount);
   
   return lineCount;
   }


int AddField( MapLayer *pLayer, LPCTSTR fieldSpec )
   {
   // parse command line arg

   CStringArray fields;
   ::Tokenize( fieldSpec, ";,", fields );
   int count = 0;

   for ( int i=0; i < fields.GetSize(); i++ )
      {
      CString field( fields[ i ] );

      int curPos = 0;
      CString fieldName = field.Tokenize( ":", curPos );
      CString fieldType = field.Tokenize( ":", curPos );

      TYPE type = TYPE_NULL;

      if ( fieldName.IsEmpty() == false )
         {
         if ( pLayer->GetFieldCol( (PCTSTR) fieldName ) < 0 )   // make sure field doesn't already exist
            {
            switch( fieldType[0] )
               {
               case 'f':   type = TYPE_FLOAT;   break;
               case 'd':   type = TYPE_DOUBLE;   break;
               case 'l':   type = TYPE_LONG;   break;
               case 'i':   type = TYPE_INT;   break;
               }

            if ( type != TYPE_NULL )
               {
               CString msg;
               msg.Format( "\n  Adding field '%s' of type '%s'", (PCTSTR) fieldName, ::GetTypeLabel(type) );
               printf( msg );

               pLayer->m_pDbTable->AddField( fieldName, type );
               count++;
               }         
            }
         }
      }

   CString msg;
   msg.Format("\n  %i fields added...", count );
   printf( msg );
   return count;
   }


int SetFieldsToSave(MapLayer *pLayer, LPCTSTR fieldSpec)
   {
   // parse command line arg
   CStringArray fields;
   ::Tokenize(fieldSpec, ";,", fields);
   int count = 0;

   // turn all fields in the layer off
   
   int fieldCount = pLayer->m_pDbTable->GetColCount();  // > GetFieldInfoCount(1);
   for (int i = 0; i < fieldCount; i++)
      pLayer->m_pDbTable->GetFieldInfo(i).save = false;
   
   for (int i = 0; i < fields.GetSize(); i++)
      {
      CString field = fields[i];

      for (int j = 0; j < fieldCount; j++)
         {
         if (field.CompareNoCase(pLayer->m_pDbTable->GetColLabel(j)) == 0)
            {
            count++;
            pLayer->m_pDbTable->GetFieldInfo(j).save = true;
            break;
            }
         }
      }

   CString msg;
   msg.Format("\n %i fields included in saved file...", count);
   printf(msg);
   return count;
   }

int FixTopology( MapLayer *pLayer, LPCTSTR uniqueIdField, PCTSTR outputFile )
   {
   // this assumes there is an input layer specified, and that the input layer:
   //   1) is a line coverage
   //   2) has a field that will uniquely identify each reach.  If this is empty, reach polygon indexes are used
   //      to populate the TNODE_ values
   int colUniqueID = -1;
   if ( uniqueIdField != NULL )
      {
      colUniqueID = pLayer->GetFieldCol( uniqueIdField );
      if ( colUniqueID < 0 )
         {
         printf( "\nInput file missing field [%s]!!!", uniqueIdField );
         return -2;
         }
      }

   if ( pLayer->GetType() != LT_LINE )
      {
      printf( "\nFix Topology requires a line type coverage!!!" );
      return -4;
      }

   CString msg;
   msg.Format("\n Fixing Topology \n" );
   //printf( msg );

   ReachTree reachTree;
 
   // build the network topology for the stream reaches
   int nodeCount = reachTree.BuildTree( pLayer, NULL, colUniqueID );   // NULL means no subclassed ReachNodes
   
   if ( nodeCount < 0 )
      {
      CString msg;
      msg.Format( "\nError building reach topology:  Error code %i", nodeCount );
      printf( msg );
      return -5;
      }

   int roots = reachTree.GetRootCount();
   printf( "\n  Found %i trees, %i nodes while building topology...", roots, nodeCount );

   // populate stream order if column provided
   //if ( m_colStreamOrder )
   //   m_reachTree.PopulateOrder( m_colStreamOrder, true );

   // have tree built, figuer what to save based on file type
   nsPath::CPath _outputFile( outputFile ); 
   CString ext = _outputFile.GetExtension();

   int type = 0; // text
   if ( ext.CompareNoCase( "shp" ) == 0 )
      type = 1;  // shp

   if ( type == 0 )
      reachTree.SaveTopology( outputFile );

   else
      {
      // basic idea - in the order of the shapes in teh shape file, iterate through the reaches
      // for each reach, get the downstream reach, the upstream reaches, and output to a csv
      int colTo   = pLayer->GetFieldCol( _T("TNODE_") );

      if ( colTo < 0 )
         {
         printf( "\n   Adding column TNODE_ ..." );
         colTo = pLayer->m_pDbTable->AddField( "TNODE_", TYPE_LONG );
         }

      for ( int i=0; i < pLayer->GetRecordCount(); i++ )
         {
         // find the corresponding reachnode in the tree
         ReachNode *pThisNode = reachTree.GetReachNode( i );

         // set ToNode for this record
         ReachNode *pDownNode = pThisNode->GetDownstreamNode( true );
         int toNode = -999;
         if ( pDownNode != NULL )
            {
            if ( colUniqueID >= 0 )
               pLayer->GetData( pDownNode->m_polyIndex, colUniqueID, toNode );
            else
               toNode = pDownNode->m_polyIndex;
            }

         pLayer->SetData( i, colTo, toNode  );
         }

      //pLayer->SaveDataDB( outputFile );
      }
   
   printf( "\n  Fix Topology completed" );
   return 0;
   }


bool ProcessCmdFile( LPCTSTR cmdFile )
   {
   // have xml string, start parsing
   TiXmlDocument doc;
   bool ok = doc.LoadFile( cmdFile );

   if ( ! ok )
      {
      CString msg;
      msg.Format("\nError reading command file, %s: %s\n", (LPCTSTR) cmdFile, doc.ErrorDesc() );
      printf( msg );
      return false;
      }

   CString msg;
   msg.Format("\n  loading command file, %s", (LPCTSTR) cmdFile );
   printf( msg );
   
   // start interating through the nodes
   TiXmlElement *pXmlRoot = doc.RootElement();     // <commands>
   
   TiXmlElement *pXmlCmd = pXmlRoot->FirstChildElement();

   while ( pXmlCmd != NULL )
      {
      CString cmd( pXmlCmd->Value() );

      //------ export command ----------------
      if ( cmd.CompareNoCase( _T( "subset" )) == 0 )     // export command?
         {
         LPTSTR layer = NULL;
         LPTSTR query = NULL;
         LPTSTR output = NULL;
         
         XML_ATTR attrs[] = 
            { // attr      type          address  isReq checkCol
            { "layer",     TYPE_STRING,   &layer,  true,   0 },
            { "query",     TYPE_STRING,   &query,  true,   0 },
            { "output",    TYPE_STRING,   &output, true,   0 },
            { NULL,        TYPE_NULL,     NULL,    false,  0 } };
  
         bool ok = TiXmlGetAttributes( pXmlCmd, attrs, cmdFile );
         if ( layer == NULL || query == NULL  || output == NULL )
            {
            printf( "\n  invalid <export> parameters...terminating" );
            return false;
            }

         Map map;
         MapLayer *pLayer = map.AddShapeLayer( layer );
         if ( pLayer == NULL )
            printf( "\nError loading shapefile %s", layer );
         else
            Subset( pLayer, query );
         }

      //else if ( cmd.CompareNoCase( _T( "some other command" )
      //   ;

      pXmlCmd = pXmlCmd->NextSiblingElement();
      }  // end of: while ( pXmlCmd != NULL )

   return true;
   }  // end of: cmd = conigure from file




CmdInfo *FindCmdInfo( CMD cmd )
   {
   for ( INT_PTR i=0; i < cmds.GetSize(); i++ ) 
      if ( cmds[ i ]->command == cmd ) 
         return cmds[ i ]; 
   
   return NULL;
   }

void PrintUsage( void )
   {
   printf( "\nusage:  envcmd /c:cmdFile   <-- run cmds in specified file  \n" );
   printf( "usage:  envcmd <switches> \n" );
   printf( "  /i:inputShapeFile                   -- specifies path to shape file to use for input\n");
   printf( "  /o:outputFile                       -- specifies output file\n" );
   printf( "  /s:\"query\"                          -- subset and export based on the specified query\n" );
   printf( "  /a:Field1:type1{;Field2:type2}...   -- subset and export based on the specified query\n" );
   printf( "  /t:ReachIDField                     -- fix topology (line shape files only) - ReachIDField is optional\n" );
   }


int RunEnvision(LPCTSTR projectFile, int scenario)
   {
   std::cout << "Starting Command Line Envision Run..." << std::endl;

   HINSTANCE hDLL = AfxLoadLibrary("EnvEngine.dll");

   if ( ! hDLL )
      {
      AfxMessageBox("Error: Cannot find EnvEngine.dll" );
      return -1;
      }

   INITENGINEFN initFn = (INITENGINEFN) ::GetProcAddress(hDLL, "EnvInitEngine");
   ASSERT(initFn != NULL);
   std::cout << "Initializing Engine..." << std::endl;
   initFn(0);

   LOADPROJECTFN loadFn = (LOADPROJECTFN) ::GetProcAddress(hDLL, "EnvLoadProject");
   ASSERT(loadFn != NULL);
   std::cout << "Loading Project " << projectFile << "..." << std::endl;
   EnvModel *pModel = loadFn(projectFile, 0);

   RUNSCENARIOFN runFn = (RUNSCENARIOFN) ::GetProcAddress(hDLL, "EnvRunScenario");
   ASSERT(runFn != NULL);
   std::cout << "Running Scenario..." << std::endl;
   runFn(pModel, scenario-1, 0 );   // note that scenario is one-based coming in, zero-based going out

   CLOSEPROJECTFN closePrjFn = (CLOSEPROJECTFN) ::GetProcAddress(hDLL, "EnvCloseProject");
   ASSERT(closePrjFn != NULL);
   std::cout << "Closing Project..." << std::endl;
   closePrjFn(pModel, 0);
   
   CLOSEENGINEFN closeFn = (CLOSEENGINEFN) ::GetProcAddress(hDLL, "EnvCloseEngine");
   ASSERT(closeFn != NULL);
   std::cout << "Closing Engine..." << std::endl;
   closeFn();

   AfxFreeLibrary(hDLL);
   return 1;
   }

   