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

#ifdef NO_MFC
#include<no_mfc_files/NoMfcFiles.h>
#endif

#include "EnvLoader.h"
#include "EnvModel.h"
#include "DataManager.h"
#include "EnvConstants.h"
#include "Actor.h"
#include "Scenario.h"

#include <MAP.h>

#include <Path.h>
#include <PathManager.h>
#include <QueryEngine.h>


#ifdef BUILD_ENVISION
#include "../EnvView.h"
#include "../SpatialIndexDlg.h"
#include "../MapPanel.h"
#include <Maplayer.h>
#include <MapWnd.h>

//extern MapLayer  *gpCellLayer;
extern CEnvView  *gpView;
extern MapPanel  *gpMapPanel;
#endif

using namespace nsPath;


EnvLoader::EnvLoader( void )
: m_pMap( NULL )
, m_pIDULayer( NULL )
, m_pEnvModel( NULL )
, m_pQueryEngine( NULL )
   {
   }


EnvLoader::~EnvLoader()
   {

   }


int EnvLoader::LoadProject( LPCTSTR filename, Map *pMap, EnvModel *pEnvModel, MAPPROC mapFn/*=NULL*/ )
   {
   //AFX_MANAGE_STATE(AfxGetStaticModuleState());
   m_pMap    = pMap;
   m_pEnvModel  = pEnvModel;
   
   ASSERT( m_pMap      != NULL );
   ASSERT( m_pEnvModel != NULL );

   AddGDALPath();

   //???Check
   //Report::errorMsgProc  = NULL; // ErrorReportProc;
   //Report::statusMsgProc = NULL; // StatusMsgProc;
   //Report::reportFlag    = NULL; // ERF_CALLBACK;      // disable onscreen messageboxes while loading

     //???Check
   //Clear();

   //???Check
   //InitDoc();

   // set up default paths.
   // first, ENVISION.EXE executable directory
   TCHAR _path[ MAX_PATH ];  // e.g. _path = "D:\Envision\src\x64\Debug\Envision.exe"
   _path[0] = NULL;
   int count = GetModuleFileName( NULL, _path, MAX_PATH );

   if ( count == 0 )
      return -1;

   CPath appPath( _path, epcTrim | epcSlashToBackslash );
   appPath.RemoveFileSpec();           // e.g. c:\Envision - directory for executables
   PathManager::AddPath( appPath );

   // second, the directory of the envx file
   CPath envxPath( filename, epcTrim | epcSlashToBackslash );
   envxPath.RemoveFileSpec();
   PathManager::AddPath( envxPath );

   
   CString logFile( envxPath );
   logFile += "/EnvisionStartup.log";
   Report::OpenFile( logFile );

   // third, the IDU path - this is determined below
   LPCTSTR paths = NULL;

   bool cellLayerInitialized = false;
   //TCHAR actorInitArgs[ 128 ];
   MAP_UNITS mapUnits = MU_UNKNOWN;

   // have xml string, start parsing
   TiXmlDocument doc;
   bool ok = doc.LoadFile( filename );

   bool loadSuccess = true;

   if ( ! ok )
      {
      CString msg( _T("Error reading input file ") );
      msg += filename;
      Report::ErrorMsg( doc.ErrorDesc(), msg );
      return -1;
      }

   // start interating through the document
   // general structure is of the form:
   // <Envision ver='x.y'>
   TiXmlElement *pXmlRoot = doc.RootElement();

   double ver = 0;
   LPCTSTR exportBmpCol = NULL;
   int logMsgLevel = 0;
    
   pXmlRoot->Attribute( "ver", &ver );
   
   TiXmlElement *pXmlSettings = pXmlRoot->FirstChildElement( _T("settings") );
   if ( pXmlSettings == NULL )
      Report::ErrorMsg( _T("Error finding <settings> section reading project file" ) );
   else
      {
      m_pEnvModel->m_pActorManager->m_actorAssociations = 0;
      int loadSharedPolicies = 1;
      int debug = 0;
      int aim = 0;
      int noBuffer = 1;
      int multiRunDecadalMapsModulus = 10;
      int defaultPeriod = 25;
      int multiRunIterations = 20;
      int dynamicUpdate = 3;
      int spatialIndexDistance = 0;
      int startYear = 0;
      double policyPrefWt = 0.33;
      double areaCutoff = 0;
      int deltaAllocationSize = 0;
      int decisionElements = -1;
      int decisionMethod = -1; 
      int dynamicActors  = 1;
      int shuffleActorPolys = 0;
      int runParallel = 0;
      int addReturnsToBudget = 0;
      double exportBmpSize=0;
      int exportMaps = -1;
      int exportMapInterval = 0;
      int exportBmpInterval = 0;
      int exportOutputs = 0;
      int exportDeltas = 0;
      int collectPolicyData = 1;
      int discardMultiRunDeltas = 0;
      int startRunNumber = 0;
      LPCTSTR fieldInfoFiles       = NULL;
      LPCTSTR exportBmpCols        = NULL;
      LPCTSTR exportDeltaFieldList = NULL;
      LPCTSTR constraints          = NULL; 
      LPCTSTR _mapUnits            = NULL;

      XML_ATTR attrs[] = 
         { // attr                             type        address                  isReq checkCol
         // environment setup
         { _T("debug"),                      TYPE_INT,      &debug,                 false,  0 },
         { _T("logMsgLevel"),                TYPE_INT,      &logMsgLevel,           false,  0 },
         { _T("noBuffering"),                TYPE_INT,      &noBuffer,              false,  0 },  // deprecated
         { _T("parallel"),                   TYPE_INT,      &runParallel,           false,  0 },
         { _T("path"),                       TYPE_STRING,   &paths,                 false,  0 },
         { _T("deltaAllocationSize"),        TYPE_INT,      &deltaAllocationSize,   false,  0 },

         // project setup
         { _T("spatialIndexDistance"),       TYPE_INT,      &spatialIndexDistance,  false,  0 },
         { _T("areaCutoff"),                 TYPE_DOUBLE,   &areaCutoff,            false,  0 },
         { _T("fieldInfoFiles"),            TYPE_STRING,   &fieldInfoFiles,        false,  0 },
         { _T("policyPreferenceWt"),         TYPE_DOUBLE,   &policyPrefWt,          false,  0 },
         { _T("addReturnsToBudget"),         TYPE_INT,      &addReturnsToBudget,    false,  0 },
         { _T("mapUnits"),                   TYPE_STRING,   &_mapUnits,             false,  0 },
         { _T("constraints"),                TYPE_STRING,   &constraints,           false,  0 },

         // runtime control
         { _T("startYear"),                  TYPE_INT,      &startYear,             false,  0 },
         { _T("startRunNumber"),             TYPE_INT,      &startRunNumber,        false,  0 },
         { _T("defaultPeriod"),              TYPE_INT,      &defaultPeriod,         false,  0 },
         { _T("multiRunIterations"),         TYPE_INT,      &multiRunIterations,    false,  0 },
         { _T("dynamicUpdate"),              TYPE_INT,      &dynamicUpdate,         false,  0 },
         { _T("discardMultiRunDeltas"),      TYPE_INT,      &discardMultiRunDeltas, false,  0 },

         // actor setup
         { _T("actorInitMethod"),            TYPE_INT,      &aim,                   false,  0 },
         { _T("actorAssociations"),          TYPE_INT,      &m_pEnvModel->m_pActorManager->m_actorAssociations, false, 0 },
         { _T("loadSharedPolicies"),         TYPE_INT,      &loadSharedPolicies,    false,  0 },
         { _T("actorDecisionElements"),      TYPE_INT,      &decisionElements,      false,  0 },
         { _T("actorDecisionMethod"),        TYPE_INT,      &decisionMethod,        false,  0 },
         { _T("dynamicActors"),              TYPE_INT,      &dynamicActors,         false,  0 },
         { _T("shuffleActorPolys"),          TYPE_INT,      &shuffleActorPolys,     false,  0 },

         // output control
         { _T("collectPolicyData" ),         TYPE_INT,      &collectPolicyData,     false,  0 },
         { _T("exportMaps"),                 TYPE_INT,      &exportMaps,            false,  0 },
         { _T("exportMapInterval"),          TYPE_INT,      &exportMapInterval,     false,  0 },
         { _T("exportBmpInterval"),          TYPE_INT,      &exportBmpInterval,     false,  0 },
         { _T("exportBmpPixelSize"),         TYPE_DOUBLE,   &exportBmpSize,         false,  0 },
         { _T("exportBmpCols"),              TYPE_STRING,   &exportBmpCols,         false,  0 },
         { _T("exportOutputs"),              TYPE_INT,      &exportOutputs,         false,  0 },
         { _T("exportDeltas"),               TYPE_INT,      &exportDeltas,          false,  0 },
         { _T("exportDeltaCols"),            TYPE_STRING,   &exportDeltaFieldList,  false,  0 }, 
         { _T("multiRunDecadalMapsModulus"), TYPE_INT,      &multiRunDecadalMapsModulus, false, 0 },
         { NULL,                             TYPE_NULL,     NULL,                   false,  0 } };

      bool ok = TiXmlGetAttributes( pXmlSettings, attrs, filename );
      if ( ! ok )
         return -2;

      //???Check
      //if ( constraints != NULL )
      //   this->m_constraintArray.Add( constraints );  // NEEDS WORK!!!!!! - multiple constraints, check quer
         
      m_pEnvModel->m_shuffleActorIDUs = shuffleActorPolys ? true : false;
      m_pEnvModel->m_debug = debug;

      if ( _mapUnits )
         {
         switch( _mapUnits[ 0 ] )
            {
            case 'f':      mapUnits = MU_FEET;     break;
            case 'm':      mapUnits = MU_METERS;   break;
            case 'd':      mapUnits = MU_DEGREES;  break;
            default:       mapUnits = MU_UNKNOWN;  break;
            }
         }

      //m_pEnvModel->m_logMsgLevel = logMsgLevel;
      //BufferOutcomeFunction::m_enabled = noBuffer ? false : true;
      m_pEnvModel->m_pDataManager->multiRunDecadalMapsModulus = multiRunDecadalMapsModulus;
      m_pEnvModel->m_dynamicUpdate = dynamicUpdate;
      m_pEnvModel->m_allowActorIDUChange = dynamicActors ? true : false;
      
      m_pEnvModel->m_startYear  = startYear;
      m_pEnvModel->m_currentYear = startYear;
      m_pEnvModel->m_yearsToRun = defaultPeriod;
      m_pEnvModel->m_endYear = startYear + defaultPeriod;
      m_pEnvModel->m_iterationsToRun = multiRunIterations;
      m_pEnvModel->m_startRunNumber = startRunNumber;

      m_pEnvModel->m_runParallel   = runParallel ? true : false;
      m_pEnvModel->m_addReturnsToBudget = addReturnsToBudget ? true : false;
      if ( exportMaps < 0) // not specified
         m_pEnvModel->m_exportMaps = exportMapInterval > 0 ? true : false;
      else
         m_pEnvModel->m_exportMaps = exportMaps > 0 ? true : false;
      
      m_pEnvModel->m_exportMapInterval = exportMapInterval;

      m_pEnvModel->m_exportBmps = (exportBmpSize > 0 && exportBmpInterval > 0 ) ? true : false;
      m_pEnvModel->m_exportBmpInterval = exportBmpInterval;
      m_pEnvModel->m_exportBmpFields = exportBmpCols;
      m_pEnvModel->m_exportBmpSize = (float) exportBmpSize;

      m_pEnvModel->m_exportOutputs = exportOutputs > 0 ? true : false;
      m_pEnvModel->m_exportDeltas  = exportDeltas > 0 ? true : false;
      m_pEnvModel->m_discardMultiRunDeltas = discardMultiRunDeltas ? true : false;

      if ( exportDeltaFieldList != NULL )
         m_pEnvModel->m_exportDeltaFields;

      m_pEnvModel->m_collectPolicyData = collectPolicyData;

      // take care of cols after idu layer loaded

      if ( deltaAllocationSize > 0 )
         DeltaArray::m_deltaAllocationSize = deltaAllocationSize;
      
      if ( /*actorAltruismWt > 0 || actorSelfInterestWt > 0  || */ policyPrefWt  >= 0 )
         {
         //this->m_altruismWt = (float) actorAltruismWt;
         //this->m_selfInterestWt = (float) actorSelfInterestWt;
         m_pEnvModel->m_policyPrefWt = (float) policyPrefWt;

         //::NormalizeWeights( this->m_altruismWt, this->m_selfInterestWt, this->m_policyPrefWt );
         }

      if ( decisionElements <= 0 )
         decisionElements = DE_ALTRUISM | DE_SELFINTEREST | DE_GLOBALPOLICYPREF;

      m_pEnvModel->SetDecisionElements( decisionElements );

      if ( decisionMethod > 0 )
         m_pEnvModel->m_decisionType = (DECISION_TYPE) decisionMethod;
      else
         m_pEnvModel->m_decisionType = DT_PROBABILISTIC;

      m_pEnvModel->m_spatialIndexDistance = spatialIndexDistance;
      m_pEnvModel->m_areaCutoff = (float) areaCutoff;

#ifdef BUILD_ENVISION
      if ( fieldInfoFiles )
         {
         TCHAR buffer[ 1024];
         lstrcpy( buffer, fieldInfoFiles );
         LPTSTR next;
         TCHAR *fiFile = _tcstok_s( buffer, _T(",\n"), &next );

         //???Check
         //while ( fiFile != NULL )
         //   {
         //   m_fieldInfoFileArray.Add( fiFile );
         //   fiFile = _tcstok_s( NULL, _T( ",\n" ), &next );
         //   }
         }
#endif
      if ( aim < AIM_NONE || aim >= AIM_END )
         Report::ErrorMsg("Invalid actorInitialization flag in .envx file - must be 0-4");

      m_pEnvModel->m_pActorManager->m_actorInitMethod = (ACTOR_INIT_METHOD) aim;
      }  // end of: else ( pSettings != NULL )

   // next - layers
   TiXmlElement *pXmlLayers = pXmlRoot->FirstChildElement( _T("layers") );

   if ( pXmlLayers == NULL )
      Report::ErrorMsg( _T("Error finding <layers> section reading project file" ) );
   else
      {
      // iterate through layers
      TiXmlElement *pXmlLayer = pXmlLayers->FirstChildElement( _T("layer") );

      while ( pXmlLayer != NULL )
         {
         int type = -1;
         int data = 1;
         int records = -1;
         int labelSize = 0;
         int linesize = 0;
         int expandLegend = 0;
         LPCTSTR color = _T("140,140,140");    // lt gray
         LPCTSTR labelColor = _T("255,255,255");     // white
         LPCTSTR labelFont="Arial";
         LPCTSTR name=NULL, path=NULL, initField=NULL, overlayFields=NULL, fieldInfoFile=NULL, labelField=NULL, labelQuery=NULL;

         XML_ATTR layerAttrs[] = 
            { // attr                 type        address                    isReq checkCol
	         { _T("name"),            TYPE_STRING,   &name,             true,   0 },
            { _T("path"),            TYPE_STRING,   &path,             true,   0 },
            { _T("initField"),       TYPE_STRING,   &initField,        false,  0 },
            { _T("overlayFields"),   TYPE_STRING,   &overlayFields,    false,  0 },
            { _T("color"),           TYPE_STRING,   &color,            false,  0 },
            { _T("size"),            TYPE_INT,      &linesize,         false,  0 },
            { _T("fieldInfoFile"),   TYPE_STRING,   &fieldInfoFile,    false,  0 },
            { _T("labelField"),      TYPE_STRING,   &labelField,       false,  0 },
            { _T("labelFont"),       TYPE_STRING,   &labelFont,        false,  0 },
            { _T("labelSize"),       TYPE_INT,      &labelSize,        false,  0 },
            { _T("labelColor"),      TYPE_STRING,   &labelColor,       false,  0 },
            { _T("labelQuery"),      TYPE_STRING,   &labelQuery,       false,  0 },
            { _T("type"),            TYPE_INT,      &type,             false,   0 },
            { _T("records"),         TYPE_INT,      &records,          false,  0 },
            { _T("includeData"),     TYPE_INT,      &data,             false,  0 },  // deprecated. ignored
            { _T("expandLegend"),    TYPE_INT,      &expandLegend,     false,  0 },

            { NULL,                  TYPE_NULL,     NULL,              false,  0 } };
		 
		
         bool ok = TiXmlGetAttributes( pXmlLayer, layerAttrs, filename );
         if ( ! ok )
            return -3;

         // parse color
         int red=0, green=0, blue=0;
         int count = sscanf_s( (TCHAR*)color, "%i,%i,%i", &red, &green, &blue );
         if ( count != 3 )
            {
            CString msg;
            msg.Format( "Misformed color information for layer %s", name );
            Report::ErrorMsg( msg );
            }
         
         bool loadFieldInfo = true;
         if ( fieldInfoFile != NULL && fieldInfoFile[ 0 ] != '\0' )
            loadFieldInfo = false;
         
         int layerIndex = LoadLayer( m_pMap, name, path, (AML_TYPE) type, red, green, blue, 0, records, initField, overlayFields, loadFieldInfo, expandLegend ? true : false );
         if ( layerIndex < 0 )
            {
            CString msg;
            msg.Format( "Bad layer in input file, %s; please fix the problem before continuing.", (LPCTSTR) filename );
            Report::ErrorMsg( msg );
            return -4;
            }

         if (linesize > 0)
            m_pMap->GetLayer(layerIndex)->m_lineWidth = linesize;

         // if IDU, add path for shape file to the PathManager
         if ( layerIndex == 0 )
            {
            m_pIDULayer = m_pMap->GetLayer( 0 );

            m_pEnvModel->SetIDULayer( m_pIDULayer ); // creates query engine for IDU layer
            m_pQueryEngine = m_pEnvModel->m_pQueryEngine;
            m_pEnvModel->m_pPolicyManager->m_pQueryEngine = m_pQueryEngine;

            m_pIDULayer->SetNoOutline();
            m_pIDULayer->m_expandLegend = true;

         // No better time than now to execute this.

            if ( m_pEnvModel->m_spatialIndexDistance > 0 && m_pIDULayer->LoadSpatialIndex( NULL, (float) m_pEnvModel->m_spatialIndexDistance ) < 0 )
               {
#ifdef BUILD_ENVISION
               SpatialIndexDlg dlg;
               //gpCellLayer = m_pIDULayer;
               dlg.m_maxDistance = m_pEnvModel->m_spatialIndexDistance;

               if ( dlg.DoModal() == IDOK )
                  m_pEnvModel->m_spatialIndexDistance = dlg.m_maxDistance;
#endif
               Report::LogInfo("Starting Spatial Index Build");
               if ( mapFn != NULL )
                  m_pIDULayer->m_pMap->InstallNotifyHandler(mapFn, (LONG_PTR)this);

               m_pIDULayer->CreateSpatialIndex(NULL, 10000, (float) m_pEnvModel->m_spatialIndexDistance, SIM_NEAREST);

               if( mapFn != NULL )
                  m_pIDULayer->m_pMap->RemoveNotifyHandler(mapFn, (LONG_PTR)this);

               Report::LogInfo("Saving Spatial Index...");

               //if (!m_canceled)
               //   {
               //   MessageBox(_T("Successful creating spatial index"), _T("Success"), MB_OK);
               //   CDialog::OnOK();
               //   }



               }

            // if neighbor table exists, load it
            m_pEnvModel->m_pIDULayer->LoadNeighborTable();

            CString fullPath;
            PathManager::FindPath( path, fullPath );

            CPath _fullPath( fullPath );
            _fullPath.RemoveFileSpec();

            PathManager::AddPath( _fullPath );

            m_pIDULayer = m_pMap->GetLayer( 0 );
            }

         MapLayer *pMapLayer = pMap->GetLayer( layerIndex );

         // take care of labels stuff
         if ( labelField != NULL )
            {
            int labelCol = pMapLayer->GetFieldCol( labelField );

            if ( labelCol >= 0 )
               {
               pMapLayer->SetLabelField( labelCol );              
               pMapLayer->SetLabelFont( labelFont );

               float _labelSize = 120;
               if ( labelSize > 0 )
                  _labelSize = (float) labelSize;

               pMapLayer->SetLabelSize( _labelSize );

               int count = sscanf_s( labelColor, "%i,%i,%i", &red, &green, &blue );
               if ( count != 3 )
                  {
                  CString msg;
                  msg.Format( "Misformed label color information for layer %s", name );
                  Report::ErrorMsg( msg );
                  }
               else
                  pMapLayer->SetLabelColor( RGB( red, green, blue ) );

               pMapLayer->ShowLabels();

               if ( labelQuery != NULL )
                  pMapLayer->m_labelQueryStr = labelQuery;
               }
            }

         if ( fieldInfoFile != NULL && fieldInfoFile[ 0 ] != NULL )
            pMapLayer->LoadFieldInfoXml( fieldInfoFile, layerIndex == 0 );
         
         // look for any join tables defined for this layer
         TiXmlElement *pXmlJoinTable = pXmlLayer->FirstChildElement( _T("join_table") );
         while ( pXmlJoinTable != NULL )
            {
            LPTSTR path      = NULL;
            LPTSTR layerCol  = NULL;
            LPTSTR joinCol   = NULL;
            XML_ATTR joinAttrs[] = {
               // attr         type           address   isReq  checkCol
               { "path",       TYPE_STRING,  &path,     true,   0 },
               { "layerCol",   TYPE_STRING,  &layerCol, true,   0 },
               { "joinCol",    TYPE_STRING,  &joinCol,  true,   0 },
               { NULL,         TYPE_NULL,    NULL,      false,  0 } };
         
            ok = TiXmlGetAttributes( pXmlJoinTable, joinAttrs, filename );
            if ( ok )
               {
               // find file
               CString fullPath;
               if ( PathManager::FindPath( path, fullPath ) < 0 )
                  {
                  CString msg;
                  msg.Format( "Join table '%s' not found.  This table will not be loaded.", path );
                  Report::ErrorMsg( msg );
                  }
               else
                  {
                  int colLayer = pMapLayer->GetFieldCol( layerCol );

                  if ( colLayer < 0 )
                     {
                     CString msg;
                     msg.Format( "Join Table: Layer field '%s' not found in layer '%s'.  This table will not be loaded", 
                        layerCol, (LPCTSTR) pMapLayer->m_name );
                     Report::ErrorMsg( msg );
                     }
                  else
                     {
                     DbTable *pJoinTable = new DbTable( DOT_VDATA );
                     pJoinTable->LoadDataCSV( fullPath );

                     // look for join col
                     int colJoin = pJoinTable->GetFieldCol( joinCol );

                     if ( colJoin < 0 )
                        {
                        CString msg;
                        msg.Format( "Join Table: Join field '%s' not found in Join Table '%s'.  This table will not be loaded", 
                           joinCol, (LPCTSTR) fullPath );
                        Report::ErrorMsg( msg );

                        delete pJoinTable;
                        }
                     else  // finally - success
                        {
                        pMapLayer->m_pDbTable->AddJoinTable( pJoinTable, layerCol, joinCol, true );
                        }
                     }
                  }
               }

            pXmlJoinTable = pXmlJoinTable->NextSiblingElement( _T("join_table" ) );
            }

         pXmlLayer = pXmlLayer->NextSiblingElement( _T("layer" ) );
         }

      // done loading layer, load background if defined
      //<layers bkgr='\Envision\StudyAreas\WW2100\transformed_warped_ww2100.tif' left='' top='' right='' bottom=''>
      LPTSTR bkgrPath=NULL, left=NULL, top=NULL, right=NULL, bottom=NULL, prjFile=NULL;
      // any additional layers info provided?
	   XML_ATTR layersAttrs[] = {
         // attr         type           address   isReq  checkCol
         { "bkgr",       TYPE_STRING,  &bkgrPath, false,   0 },
         { "left",       TYPE_STRING,  &left,     false,   0 },
         { "right",      TYPE_STRING,  &right,    false,   0 },
         { "top",        TYPE_STRING,  &top,      false,   0 },
         { "bottom",     TYPE_STRING,  &bottom,   false,   0 },
         { "prjFile",    TYPE_STRING,  &prjFile,  false,   0 },
         { NULL,         TYPE_NULL,    NULL,      false,   0 } };

      ok = TiXmlGetAttributes( pXmlLayers, layersAttrs, filename );
      if ( ok )
         {
         if ( bkgrPath != NULL )
            {
            REAL _left, _top, _right, _bottom;
            m_pIDULayer->GetExtents( _left, _right, _bottom, _top );

            if ( left != NULL && *left != NULL )
               _left = (float) atof( left );
            
            if ( top != NULL && *top != NULL )
               _top = (float) atof( top );

            if ( right != NULL && *right != NULL )
               _right = (float) atof( right );

            if ( bottom != NULL && *bottom != NULL )
               _bottom = (float) atof( bottom );

            m_pMap->m_bkgrPath = bkgrPath;
            m_pMap->m_bkgrLeft   = _left;
            m_pMap->m_bkgrTop    = _top;
            m_pMap->m_bkgrRight  = _right;
            m_pMap->m_bkgrBottom = _bottom;

#ifdef BUILD_ENVISION
            //m_pMap->LoadBkgrImage(bkgrPath, _left, _top, _right, _bottom );
#endif
            }
         }
      }

   // IDU loaded, so process path setting if anything defined
   if ( paths != NULL )
      {
      m_pEnvModel->m_searchPaths = paths;

      TCHAR *buffer = new TCHAR[ lstrlen( paths ) + 1 ];
      lstrcpy( buffer, paths );

      // parse the path, adding as you go.
      LPTSTR next;
      TCHAR *token = _tcstok_s( buffer, _T(",;|"), &next );
      while ( token != NULL )
         {
         PathManager::AddPath( token );
         token = _tcstok_s( NULL, _T( ",;|" ), &next );
         }

      delete [] buffer;
      }

   // next - lulcTree
   Report::StatusMsg("Loading LULC Tree information");
   m_pEnvModel->m_lulcTree.m_path.Empty(); // = filename;
   TiXmlNode *pXmlLulcTree = pXmlRoot->FirstChild(_T("lulcTree"));
   int lulcCount = m_pEnvModel->m_lulcTree.LoadXml(pXmlLulcTree, false);

   // this has to wait until the LULC tree is loaded
   if ( !cellLayerInitialized)
      {
#ifndef NO_MFC
      CWaitCursor c;
#endif
      m_pEnvModel->StoreIDUCols();    // requires models needed to be loaded before this point
      m_pEnvModel->VerifyIDULayer();     // requires StoreCellCols() to be call before this is called, but not models
      cellLayerInitialized = true;
      }

#ifdef BUILD_ENVISION
   // visualizers
   TiXmlElement *pVisualizers = pXmlRoot->FirstChildElement( _T("visualizers") );
   if ( pVisualizers != NULL )
      {
      // iterate through layers
      TiXmlElement *pViz = pVisualizers->FirstChildElement( _T("visualizer") );

      while ( pViz != NULL )
         {
         LPCTSTR name     = pViz->Attribute( _T("name") );
         LPCTSTR path     = pViz->Attribute( _T("path") );
         LPCTSTR type     = pViz->Attribute( _T("type") );
         LPCTSTR initInfo = pViz->Attribute( _T("initInfo") );
         LPCTSTR use      = pViz->Attribute( _T("use" ) );
		 int vizID=0;
		 pViz->QueryIntAttribute("id", &vizID);
         // load visualizer stuff here
         CString msg( "Loading Visualizer: " );
         msg += name;
         msg += " (";
         msg += path;
         msg += ")";
         Report::Log( msg );

         int _type = atoi( type );
         int _use  = atoi( use );
         LoadVisualizer( name, path, _use, _type,vizID, initInfo );

         msg.Format( "Loaded Visualizer %s (%s)", name, path );
         Report::Log( msg );
         
         pViz = pViz->NextSiblingElement( _T("visualizer" ) );
         }
      }
#endif

   // next - app vars
   TiXmlElement *pXmlAppVars = pXmlRoot->FirstChildElement( _T("app_vars") );
   if ( pXmlAppVars != NULL )
      {
      // iterate through layers
      TiXmlElement *pXmlAppVar = (TiXmlElement*) pXmlAppVars->FirstChildElement( _T("app_var") );

      while ( pXmlAppVar != NULL )
         {
         LPCTSTR name  = NULL;
         LPCTSTR desc  = NULL;
         LPCTSTR value = NULL;
         LPCTSTR col = NULL;
         int timing = 3;

         XML_ATTR appVarAttrs[] = 
            { // attr          type        address    isReq checkCol
            { "name",        TYPE_STRING,   &name,     true,   0 },
            { "description", TYPE_STRING,   &desc,     false,  0 },
            { "value",       TYPE_STRING,   &value,    true,  0 },
            { "fieldname",   TYPE_STRING,   &col,      false,  CC_AUTOADD },
            { "timing",      TYPE_INT,      &timing,   false,  0 },
            { NULL,          TYPE_NULL,     NULL,      false,  0 } };
  
         bool ok = TiXmlGetAttributes( pXmlAppVar, appVarAttrs, filename, m_pIDULayer );

         if ( name == NULL )
            {
            CString msg;
            //if ( name != NULL )
            //   msg.Format( _T("Error reading <app_var> '%s': missing value attribute" ), name );
            //else
               msg = _T("Error reading <app_var>: missing name attribute" );

            Report::ErrorMsg( msg );
            }
         else
            {
            AppVar *pVar = new AppVar( name, desc, value );   // needs expression support

            pVar->m_avType = AVT_APP;
            
            //if ( value != NULL && value[ 0 ] != NULL )      // note: timing = 1 until set below
            //   pVar->Evaluate();

            pVar->m_timing = timing;

            if ( pVar->m_timing == -1 ) // hack for now
               pVar->SetValue( VData( (float) atof( value ) ) );

            m_pEnvModel->AddAppVar( pVar, true );    // create and evaluate using map expression evaluator
            }

         pXmlAppVar = pXmlAppVar->NextSiblingElement( _T("app_var" ) );
         }
      }  // end of: if ( pAppVars != NULL )


   // next - autonomous processes
   TiXmlElement *pXmlModels = pXmlRoot->FirstChildElement( _T("models") );
   if ( pXmlModels == NULL )
      Report::ErrorMsg( _T("Error finding <models> section reading project file" ) );
   else
      {
      // iterate through layers
      TiXmlElement *pXmlModel = pXmlModels->FirstChildElement( _T("model") );
 
      while ( pXmlModel != NULL )
         {
         int id = 0;
         int use = 1;
         int freq = 1;
         int timing = 0;
         int sandbox = 0;
         int initRunOnStartup = 0;

         CString name;
         CString path;
         CString fieldName;
         CString initInfo;
         CString dependencyNames;
         CString description;
         CString imageURL;
         XML_ATTR attrs[] = {
            // attr                     type          address              isReq  checkCol
               { _T("name"),              TYPE_CSTRING,  &name,               true,    0 },
               { _T("path"),              TYPE_CSTRING,  &path,               true,    0 },
               { _T("fieldName"),         TYPE_CSTRING,  &fieldName,          false,   CC_AUTOADD },
               { _T("initInfo"),          TYPE_CSTRING,  &initInfo,           false,   0 },
               { _T("id"),                TYPE_INT,      &id,                 false,   0 },
               { _T("timing"),            TYPE_INT,      &timing,             false,   0 },
               { _T("sandbox"),           TYPE_INT,      &sandbox,            false,   0 },
               { _T("use"),               TYPE_INT,      &use,                false,   0 },
               { _T("freq"),              TYPE_FLOAT,    &freq,               false,   0 },
               { _T("initRunOnStartup"),  TYPE_INT,      &initRunOnStartup,   false,   0 },  // deprecated
               { _T("dependencies"),      TYPE_CSTRING,  &dependencyNames,    false,   0 },
               { _T("description"),       TYPE_CSTRING,  &description,        false,   0 },
               { _T("imageURL"),          TYPE_CSTRING,  &imageURL,           false,   0 },
               { NULL,                    TYPE_NULL,     NULL,                false,   0 } };

         ok = TiXmlGetAttributes(pXmlModel, attrs, filename, m_pIDULayer);
         if (!ok)
            {
            CString msg;
            msg.Format(_T("Misformed element reading <model> attributes in input file %s"), filename);
            Report::ErrorMsg(msg);
            return false;
            }

         // load model stuff here
         CString msg("Loading model: ");
         msg += name;
         msg += " (";
         msg += path;
         msg += ")";
         Report::Log(msg);

         int useCol = 0;
         if (fieldName.IsEmpty() == false)
            useCol = 1;

         bool ok = LoadModel(m_pEnvModel, name, path, description, imageURL, id, use, timing, initInfo, dependencyNames);
         if ( ok )
            msg.Format( "Loaded Model %s (%s)", name, path );
         else
            msg.Format( "Unable to Load Model %s (%s)", name, path );

         Report::Log( msg );
         pXmlModel = pXmlModel->NextSiblingElement( _T("model" ) );
         }
      }

	// next - evaluators
	TiXmlElement *pXmlEvaluators = pXmlRoot->FirstChildElement(_T("evaluators"));
   if (pXmlEvaluators == NULL)
      ; //  Report::ErrorMsg(_T("Error finding <evaluators> section reading project file"));
	else
		{
		// iterate through layers
		TiXmlElement *pXmlEvaluator = pXmlEvaluators->FirstChildElement(_T("evaluator"));

		while (pXmlEvaluator != NULL)
			{
			int id = 0;
			int use = 1;
			float freq = 1;
			int showInResults = 1;
			int initRunOnStartup = 0;
			float gain = 1;
			float offset = 0;

         CString name;
         CString path;
         CString fieldName;
         CString initInfo;
         CString dependencyNames;
         CString description;
         CString imageURL;

         XML_ATTR attrs[] = {
            // attr                     type          address              isReq  checkCol
            { _T("name"),              TYPE_CSTRING,  &name,               true,    0 },
            { _T("path"),              TYPE_CSTRING,  &path,               true,    0 },
            { _T("fieldname"),         TYPE_CSTRING,  &fieldName,          false,   CC_AUTOADD },
            { _T("initInfo"),          TYPE_CSTRING,  &initInfo,           false,   0 },
            { _T("id"),                TYPE_INT,      &id,                 false,   0 },
            { _T("use"),               TYPE_INT,      &use,                false,   0 },
            { _T("freq"),              TYPE_FLOAT,    &freq,               false,   0 },
            { _T("showInResults"),     TYPE_INT,      &showInResults,      false,   0 },
            { _T("gain"),              TYPE_FLOAT,    &gain,               false,   0 },
            { _T("offset"),            TYPE_FLOAT,    &offset,             false,   0 },
            { _T("initRunOnStartup"),  TYPE_INT,      &initRunOnStartup,   false,   0 },   // deprecated
            { _T("dependencies"),      TYPE_CSTRING,  &dependencyNames,    false,   0 },
            { _T("description"),       TYPE_CSTRING,  &description,        false,   0 },
            { _T("imageURL"),          TYPE_CSTRING,  &imageURL,           false,   0 },
            { NULL,                    TYPE_NULL,     NULL,                false,   0 } };

         ok = TiXmlGetAttributes(pXmlEvaluator, attrs, filename, NULL);
         if (!ok)
            {
            CString msg;
            msg.Format(_T("Misformed element reading <evaluator> attributes in input file %s"), filename);
            Report::ErrorMsg(msg);
            return false;
            }
         
			// load evaluator stuff here
			CString msg("Loading evaluator: ");
			msg += name;
			msg += " (";
			msg += path;
			msg += ")";
			Report::Log(msg);

			int useCol = 0;
			if (fieldName.IsEmpty() == false )
				useCol = 1;

         bool ok = LoadEvaluator(m_pEnvModel, name, path, description, imageURL, id, use, showInResults, initInfo, dependencyNames);

			if (ok)
				msg.Format("Loaded Evaluator %s (%s)", name, path);
			else
				msg.Format("Unable to Load Evaluator %s (%s)", name, path);

			Report::Log(msg);

			pXmlEvaluator = pXmlEvaluator->NextSiblingElement(_T("evaluator"));
			}
		}

	// next - metagoals
   TiXmlElement *pXmlMetagoals = pXmlRoot->FirstChildElement( _T("metagoals") );
   if ( pXmlMetagoals == NULL )
      Report::ErrorMsg( _T("Error finding <metagoals> section reading project file" ) );
   else
      {
      // iterate through layers
      TiXmlElement *pXmlMetagoal = pXmlMetagoals->FirstChildElement( _T("metagoal") );
 
      while ( pXmlMetagoal != NULL )
         {
         int decisionUse = 1;
         LPCTSTR name = pXmlMetagoal->Attribute( _T("name") );
         LPCTSTR model = pXmlMetagoal->Attribute( _T("model") );
         pXmlMetagoal->Attribute( _T("decision_use"), &decisionUse );
         EnvEvaluator *pModel = NULL;

         if ( decisionUse <= 0 || decisionUse > 3 )
            {
            CString msg( _T("Error reading metagoal <") );
            msg += name;
            msg += _T( "> - 'decision_use' attribute must be 1,2 or 3.  This metagoal will be ignored." );
            Report::ErrorMsg( msg );
            continue;
            }

         if ( decisionUse & DU_ALTRUISM )
            {
            if ( model == NULL )  // look for corresponding model
               model = name;

            pModel = m_pEnvModel->FindEvaluatorInfo( model );
            if ( pModel == NULL )
               {
               CString msg( _T( "Error finding evalative model <") );
               msg += model;
               msg += _T("> when reading metagoal. You must provide an appropriate model reference.  Altruistic decision-making will be disabled for this metagoal");
               Report::ErrorMsg( msg );
               
               if ( decisionUse & DU_SELFINTEREST )
                  decisionUse = DU_SELFINTEREST;
               else
                  decisionUse = 0;
               }
            }

         if ( decisionUse > 0 )
            {
            METAGOAL *pMeta = new METAGOAL;
            pMeta->name = name;
            pMeta->pEvaluator = pModel;
            pMeta->decisionUse = decisionUse;
            m_pEnvModel->AddMetagoal( pMeta );
            }

         pXmlMetagoal = pXmlMetagoal->NextSiblingElement( _T("metagoal" ) );
         }
      }

   Report::StatusMsg( "Initializing Models and Processes..." );
   m_pEnvModel->InitModels();     // EnvContext gets set up here

   // basic setting loaded, take care of everything else
   if ( m_pIDULayer == NULL )
      return -4;
   
   // fixup exportBmpCol is needed
   //if ( exportBmpCol != NULL )
   //   this->m_exportBmpCol = m_pIDULayer->GetFieldCol( exportBmpCol );    

   //for ( int i=0; i < m_fieldInfoFileArray.GetSize(); i++ )
   //   {
   //   CString msg( "Loading Field Information File: " );
   //   msg += m_fieldInfoFileArray[ i ];
   //   Report::StatusMsg( msg );
   //   LoadFieldInfoXml( NULL, m_fieldInfoFileArray[ i ] );
   //   }

   Report::StatusMsg( "Finished loading Field Information - Reconciling data types..." );

   //ReconcileDataTypes();      // require map layer to be loaded, plus map field infos?
   SetDependencies();

   // next - policies 
   Report::StatusMsg( "Loading Policies..." );
   m_pEnvModel->m_pPolicyManager->m_path = filename;   // envx
   TiXmlNode *pXmlPolicies = pXmlRoot->FirstChild( _T("policies") ); 
   int policyCount = m_pEnvModel->m_pPolicyManager->LoadXml( pXmlPolicies, false /* not appending*/ );

   CString msg;
   msg.Format( "Loaded %i policies from %s", policyCount, m_pEnvModel->m_pPolicyManager->m_path );
   Report::Log( msg );
   //gpView->m_policyEditor.ReloadPolicies( false );

   // next - actors
   Report::StatusMsg( "Loading Actors..." );   
   if (m_pEnvModel->m_pActorManager->m_actorInitMethod != AIM_NONE )
      {
      m_pEnvModel->m_pActorManager->m_path.Empty(); // = filename;
      TiXmlNode *pXmlActors = pXmlRoot->FirstChild( _T("actors") );
      int actorCount = m_pEnvModel->m_pActorManager->LoadXml( pXmlActors, false );  // creates groups only

      if (m_pEnvModel->m_pActorManager->m_path.IsEmpty() )
         msg.Format( "Loaded %i actors from %s", actorCount, filename );
      else
         msg.Format( "Loaded %i actors from %s", actorCount, (PCTSTR)m_pEnvModel->m_pActorManager->m_path );
      Report::Log( msg );
      }
   
   m_pEnvModel->m_pActorManager->CreateActors();             // this makes the actors from the groups
   

   // next, RtViews
#ifdef BUILD_ENVISION

   TiXmlElement *pXmlViews = pXmlRoot->FirstChildElement( _T("views") );
   if ( pXmlViews != NULL )
      {
      TiXmlElement *pXmlPage = pXmlViews->FirstChildElement( _T("page") );

      while ( pXmlPage != NULL )
         {
         LPTSTR name;
         XML_ATTR pageAttrs[] = {
            // attr                 type          address    isReq  checkCol
            { "name",               TYPE_STRING,  &name,     true,   0 },
            { NULL,                 TYPE_NULL,     NULL,     false,  0 } };
            
         ok = TiXmlGetAttributes( pXmlPage, pageAttrs, filename );
         if ( ! ok )
            {
            CString msg; 
            msg.Format( _T("Envision: Misformed <page> element reading input file %s"), filename );
            Report::ErrorMsg( msg );
            }
         else
            {
            // process page
            RtViewPage *pPage = gpView->m_viewPanel.AddPage( name );
            TiXmlElement *pXmlElement = pXmlPage->FirstChildElement( _T("element") );
      
            while ( pXmlElement != NULL )
               {
               LPTSTR name=NULL, type=NULL, col=NULL;
               int left=0, top=0, right=0, bottom=0;

               XML_ATTR elemAttrs[] = {
                  // attr          type          address    isReq  checkCol
                  { "name",        TYPE_STRING,  &name,     true,   0 },
                  { "left",        TYPE_INT,     &left,     true,   0 },
                  { "right",       TYPE_INT,     &right,    true,   0 },
                  { "top",         TYPE_INT,     &top,      true,   0 },
                  { "bottom",      TYPE_INT,     &bottom,   true,   0 },
                  { "type",        TYPE_STRING,  &type,     true,   0 },
                  { "col",         TYPE_STRING,  &col,      false,  0 },
                  //{ "layer",       TYPE_STRING,  &col,      false,  0 },
                  { NULL,          TYPE_NULL,     NULL,     false,  0 } };
                  
               ok = TiXmlGetAttributes( pXmlElement, elemAttrs, filename );
               if ( ! ok )
                  {
                  RTV_TYPE t = RTVT_MAP;
                  if ( *type == 'v' )
                     t = RTVT_VISUALIZER;

                  RtViewWnd *pView = NULL;
                  CRect rect( left, top, right, bottom );
                  switch( t )
                     {
                     case RTVT_MAP:
                        {
                        int _col= m_pIDULayer->GetFieldCol( col );

                        if ( _col >= 0 )
                           pView = pPage->AddView( t, _col, 0, rect );
                        }
                        break;

                     case RTVT_VISUALIZER:
                        break;
                     }
                  }

               pXmlElement = pXmlElement->NextSiblingElement( _T("element") );
               }
            }

         pXmlPage = pXmlPage->NextSiblingElement( _T("page") );
         }
      }  // end of: if ( pXmlView != NULL );

#endif

#ifdef BUILD_ENVISION

   // next, Zooms
   TiXmlElement *pXmlZooms = pXmlRoot->FirstChildElement( _T("zooms") );
   if ( pXmlZooms != NULL )
      {
      LPCTSTR defaultZoom = pXmlZooms->Attribute( "default" );
      
      TiXmlElement *pXmlZoom = pXmlZooms->FirstChildElement( _T("zoom") );

      while ( pXmlZoom != NULL )
         {
         LPTSTR name = NULL;

         float xMin, yMin, xMax, yMax;

         XML_ATTR zoomAttrs[] = {
            // attr      type          address    isReq  checkCol
            { "name",    TYPE_STRING,  &name,     true,   0 },
            { "left",    TYPE_FLOAT,   &xMin,     true,   0 },
            { "right",   TYPE_FLOAT,   &xMax,     true,   0 },
            { "bottom",  TYPE_FLOAT,   &yMin,     true,   0 },
            { "top",     TYPE_FLOAT,   &yMax,     true,   0 },
            { NULL,      TYPE_NULL,     NULL,     false,  0 } };
            
         ok = TiXmlGetAttributes( pXmlZoom, zoomAttrs, filename );
         if ( ! ok )
            {
            CString msg; 
            msg.Format( _T("Envision: Misformed <zoom> element reading input file %s"), filename );
            Report::ErrorMsg( msg );
            }
         else
            {
            // process page
            int index = gpView->AddZoom( name, xMin, yMin, xMax, yMax );

            if ( defaultZoom != NULL && lstrcmp( defaultZoom, name ) == 0 )
               {
               gpView->m_pDefaultZoom = gpView->GetZoomInfo( index );
               gpView->SetZoom( index );
               }
            }

         pXmlZoom = pXmlZoom->NextSiblingElement( _T("zoom") );
         }
      }  // end of: if ( pXmlZoom != NULL );
#endif

   // next, probes
   /*
   TiXmlNode *pXmlProbes = pXmlRoot->FirstChild( _T("probes") );
   if ( pXmlProbes != NULL )
      {
      TiXmlNode *pXmlProbe = pXmlProbes->FirstChild( "probe" );

      while ( pXmlProbe != NULL )
         {
         LPCTSTR type;
         int id;
         
         XML_ATTR attrs[] = {
            // attr     type          address   isReq  checkCol
            { "type",   TYPE_STRING,   &type,    true,     0 },
            { "id",     TYPE_INT,      &id,      true,     0 },
            { NULL,     TYPE_NULL,     NULL,     false,    0 } };

         ok = TiXmlGetAttributes( pXmlProbe->ToElement(), attrs, filename );
         if ( ! ok )
            {
            CString msg; 
            msg.Format( _T("Misformed probe element reading <probe> attributes in input file %s"), filename );
            Report::ErrorMsg( msg );
            }
         else
            {
            Policy *pPolicy = m_pPolicyManager->GetPolicyFromID( id );

            if ( pPolicy == NULL )
               {
               CString msg; 
               msg.Format( _T("Unable to probe policy '%i' reading %s - this probe will be ignored"),
                  id, filename );
               Report::ErrorMsg( msg );
               }
            else
               {
               Probe *pProbe = new Probe( pPolicy );
               m_pEnvModel->AddProbe( pProbe );
               }
            }
         pXmlProbe = pXmlProbe->NextSibling( "probe" );
         }
      }
      */
   // set all layers MAP_UNITS
   m_pMap->m_mapUnits = mapUnits;

   for ( int i=0; i < m_pMap->GetLayerCount(); i++ )
      {
      MapLayer *pLayer = m_pMap->GetLayer( i ) ;
      pLayer->m_mapUnits = mapUnits;

      if ( i == 0 ) // first layer?
         {
         // classify data;
         if ( m_pEnvModel->m_lulcTree.GetLevels() > 0 )
            {
            int col = pLayer->GetFieldCol(m_pEnvModel->m_lulcTree.GetFieldName( 1 ) );

            if ( col < 0 )
               col = pLayer->m_pDbTable->AddField( _T("LULC_A"), TYPE_INT, true );

            ASSERT( col >= 0 );
            pLayer->SetActiveField( col );
            Report::StatusMsg( "Setting up map layer bins..." );
            Map::SetupBins( m_pMap, 0, -1 );
            }
         }
      }
 
   // next, add output directory (e.g. projPath= "C:\Envision\MyProject\Outputs" )
   CString outputPath = PathManager::GetPath( PM_PROJECT_DIR );  // note: always terminated with a '\'
   outputPath += "Outputs\\";
   PathManager::AddPath( outputPath ); // note: this will get scenario name appended below, after scenarios loaded
   
   // next, initialize models and visualizers
   //Report::StatusMsg( "Initializing Models and Processes..." );
   //m_pEnvModel->InitModels();     // EnvContext gets set up here

   if ( m_pEnvModel->m_decisionElements & DE_SOCIALNETWORK )
      m_pEnvModel->InitSocialNetwork();

   InitVisualizers();

   // next - scenarios
   Report::StatusMsg( "Loading Scenarios..." );   
   Report::Log( "Loading Scenarios" );
   m_pEnvModel->m_pScenarioManager->m_path.Empty(); // = filename;
   TiXmlNode *pXmlScenarios = pXmlRoot->FirstChildElement( _T("scenarios") );
   int defaultIndex = 0;
   pXmlScenarios->ToElement()->Attribute( "default", &defaultIndex );
   int scenarioCount = m_pEnvModel->m_pScenarioManager->LoadXml( pXmlScenarios, false );

   if ( defaultIndex < 0  )
      defaultIndex = 0;
      
   // create a default scenario if none was specified in the ENVX file
   if (m_pEnvModel->m_pScenarioManager->GetCount() == 0 )
      {
      Scenario *pScenario = new Scenario(m_pEnvModel->m_pScenarioManager, "Default Scenario" );
      m_pEnvModel->m_pScenarioManager->AddScenario( pScenario );
      m_pEnvModel->m_pScenarioManager->SetDefaultScenario( 0 );
      }
   else
      {
      if ( defaultIndex >= m_pEnvModel->m_pScenarioManager->GetCount() )
         defaultIndex = 0;

      m_pEnvModel->m_pScenarioManager->SetDefaultScenario( defaultIndex );
      }

   m_pEnvModel->SetScenario(m_pEnvModel->m_pScenarioManager->GetScenario( defaultIndex ) );

   if (m_pEnvModel->m_pScenarioManager->m_path.IsEmpty() )
      msg.Format( "Loaded %i scenarios from %s", scenarioCount, filename );
   else
      msg.Format( "Loaded %i scenarios from %s", scenarioCount, (PCTSTR)m_pEnvModel->m_pScenarioManager->m_path );
   Report::Log( msg );

   // postrun commands
   TiXmlElement *pXmlPostRun = pXmlRoot->FirstChildElement( _T("postrun") );
   if ( pXmlPostRun != NULL )
      {
      TiXmlElement *pXmlCmd = pXmlPostRun->FirstChildElement( _T("cmd") );

      while ( pXmlCmd != NULL )
         {
         LPTSTR cmd = NULL;

         XML_ATTR cmdAttrs[] = {
            // attr      type          address    isReq  checkCol
            { "cmdline", TYPE_STRING,  &cmd,     true,   0 },
            { NULL,      TYPE_NULL,     NULL,    false,  0 } };
            
         ok = TiXmlGetAttributes( pXmlCmd, cmdAttrs, filename );
         if ( ! ok )
            {
            CString msg; 
            msg.Format( _T("Envision: Misformed <cmd> element reading input file %s"), filename );
            Report::ErrorMsg( msg );
            }
         else
            {
            m_pEnvModel->m_postRunCmds.Add( cmd );
            }

         pXmlCmd = pXmlCmd->NextSiblingElement( _T("cmd") );
         }
      }  // end of: if ( pXmlPostRun != NULL );

   // reset output path to correct directory for output
   // output directory (e.g. "C:\Envision\MyStudyArea\MyIDU\Outputs\<scenarioName>\" )
   outputPath = PathManager::GetPath( PM_IDU_DIR );
   outputPath += "Outputs\\";
   outputPath += m_pEnvModel->m_pScenarioManager->GetScenario( defaultIndex )->m_name;
   PathManager::SetPath( PM_OUTPUT_DIR, outputPath );

   // make sure directory exists
#ifndef NO_MFC
   SHCreateDirectoryEx( NULL, outputPath, NULL );
#else
         mkdir(outputPath,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif

   // create app variable for exposed model outputs and add to the global query engine
   //m_pEnvModel->CreateModelAppVars();  // from model outputs, scores, rawscores (NOTE: MOVED TO InitModels()


   Report::StatusMsg( "Compiling Policies" );
   Report::Log( "Compiling Policies" );
   m_pEnvModel->m_pPolicyManager->CompilePolicies( m_pIDULayer );

   //gpView->m_policyEditor.ReloadPolicies( false );
   //gpView->AddStandardRecorders();

   //this->m_envContext.pMapLayer = m_pIDULayer;
   //gpDoc->UnSetChanged( CHANGED_ACTORS | CHANGED_POLICIES | CHANGED_SCENARIOS | CHANGED_PROJECT );

   // all done
   msg = "Done loading ";
   msg += filename;
   Report::StatusMsg( msg  );
   Report::Log( msg );   

   Report::CloseFile();

   m_pEnvModel->m_logMsgLevel = logMsgLevel;

   MapLayer *pLayer = m_pMap->GetLayer(0);
   msg.Format("IDU layer: %i columns, %i rows", pLayer->GetColCount(), pLayer->GetRowCount());
   Report::Log(msg);


   return 1;   
   }



int EnvLoader::LoadLayer( Map *pMap, LPCTSTR name, LPCTSTR path, AML_TYPE type,  int red, int green, int blue, int extraCols, int records, LPCTSTR initField, LPCTSTR overlayFields, bool loadFieldInfo, bool expandLegend )
   {
   m_pMap = pMap;
   ASSERT ( m_pMap != NULL );
   int layerIndex = m_pMap->GetLayerCount();

   // if first map, add extra cols for models
   // note: name will point to a name of the coverage, this is not the same as the path or filename
   //       path will be the fully or partially qualified path/filename
   // note: ExtraCols will be set to not save by default
   MapLayer *pLayer = AddMapLayer( path, type, extraCols, records, loadFieldInfo, expandLegend );

   if ( pLayer != NULL )      // map successfully added?
      {
      // add in any other relevent information
      pLayer->m_name = name;
      pLayer->SetOutlineColor( RGB( red, green, blue ) );

      if ( records > 0 )
         {
         if ( pLayer->GetActiveField() < 0 )
            pLayer->SetActiveField( 0 );

         ASSERT( pLayer->m_pDbTable != NULL );
         }

      CString msg( "Success loaded map layer: " );
      msg += path;
      Report::Log( msg );

      // is an overlay layer is defined, set it up
      if ( overlayFields != NULL )
         {
         pLayer->m_overlayFields = overlayFields;
         /*
         CStringArray _overlayFields;
         int overlayCount = Tokenize( overlayFields, ",;", _overlayFields );

         for ( int k=0; k < overlayCount; k++ )
            {
            int overlayCol = pLayer->GetFieldCol( _overlayFields[ i ] );

            if ( overlayCol < 0 )
               {
               CString msg( "Unable to locate overlay field [" );
               msg += overlay;
               msg += "] ... The Overlay layer will not be created";
               Report::ErrorMsg( msg );
               }
            else
               {
               MapLayer *pOverlay = pMap->CreatePointOverlay( pLayer, overlayCol ); 
               if ( pOverlay == NULL )
                  {
                  CString msg( "Unable to create Overlay layer [" );
                  msg += overlay;
                  msg += "]";
                  Report::ErrorMsg( msg );
                  }
               else
                  {
                  pLayer->m_overlayCol = overlayCol;
                  pOverlay->SetActiveField( overlayCol );
                  pOverlay->UseVarWidth( 6 );
                  }
               }
            } */
         } 

      // is an initial field defined, set it up
      if ( initField && lstrlen( initField ) > 0 )
         {
         int initFieldCol = pLayer->GetFieldCol( initField );
         pLayer->m_initInfo = initField;
         
         if ( initFieldCol < 0 )
            {
            CString msg( "Unable to locate initialization field [" );
            msg += initField;
            msg += "] while loading layer ";
            msg += name;
            msg += "... ignored...";
            Report::LogWarning( msg );
            }
         else
            pLayer->SetActiveField( initFieldCol );
         }

      //CheckValidFieldNames( pLayer );      
      }

   else
      {
      layerIndex = -1; // because failed
      }

   return layerIndex;
   }


MapLayer *EnvLoader::AddMapLayer( LPCTSTR _path, AML_TYPE type, int extraCols, int records, bool loadFieldInfo, bool expandLegend )
   {
   // add the layer to the map
   ASSERT( m_pMap != NULL );
   if ( m_pMap == NULL )
      return NULL;

  // VData::m_useWideChar = false;
// ESRI DBF strings are often double byte...kbv 02/09/2010
   VData::SetUseWideChar( true );

   MapLayer *pLayer = NULL;

   CString path;
   PathManager::FindPath( _path, path );
   
   // load layer stuff here
   CString msg;
   msg.Format( "Loading layer %s", (LPCTSTR) path );
   Report::Log( msg );

   if ( type == AMLT_INFER )      // infer from file name
      {
      const TCHAR *ext = _tcsrchr( _path, _T('.') );
      ext++;

      if ( _tcsicmp( ext, "shp") )
         type = AMLT_SHAPE;
      else if ( _tcsicmp(ext, "flt") )
         type = AMLT_FLOAT_GRID;
      else if (_tcsicmp(ext, "grd"))
         type = AMLT_INT_GRID;
      else if (_tcsicmp(ext, "asc"))
         type = AMLT_INT_GRID;
      }

   switch ( type )
      {
      case AMLT_SHAPE:     // shape file
         {
         pLayer = m_pMap->AddShapeLayer( path, records ? true : false, extraCols, records );
         
         if ( pLayer != NULL && loadFieldInfo )
            {
            // add a field info file if one exists
            nsPath::CPath fiPath( path );
            fiPath.RenameExtension( "xml" );

            if ( fiPath.Exists() )
               pLayer->LoadFieldInfoXml( fiPath, m_pMap->GetLayerCount() == 1 );
            }
         // all done
         //return pLayer;
         }
         break;
         
      case AMLT_INT_GRID:     // grid file
         pLayer = m_pMap->AddGridLayer( path, DOT_INT );
         break;

      case AMLT_FLOAT_GRID:     // grid file
         pLayer = m_pMap->AddGridLayer( path, DOT_FLOAT );
         break;
      }

   if ( pLayer != NULL )
      {
      pLayer->m_expandLegend = expandLegend;

      CString msg;
      msg.Format( "Loaded layer %s: %i (%i) columns, %i rows", (PCTSTR) path, pLayer->GetColCount(), pLayer->m_pData->GetColCount(), pLayer->GetRowCount() );
      Report::Log( msg );
      }
   else  // pLayer = NULL
      {
      CString msg( "Unable to load Map Layer " );
      msg += path;
      msg += "... Would you like to browse for the file?";

#ifdef BUILD_ENVISION

      if ( gpView->MessageBox( msg, "Layer Not Found", MB_YESNO ) == IDYES )
         {
         char *cwd = _getcwd( NULL, 0 );

         CFileDialog dlg( TRUE, NULL, path, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
               "Shape files|*.shp|Grid Files|*.grd|Grid Files|*.flt|Grid Files|*.asc|All files|*.*||");

         if ( dlg.DoModal() == IDOK )
            {
            CString filename( dlg.GetPathName() );

            _chdir( cwd );
            free( cwd );
            return AddMapLayer( filename, type, extraCols, records, loadFieldInfo, expandLegend );
            }
         
         _chdir( cwd );
         free( cwd );
         }
#endif
      }  // end of: if ( pLayer == NULL )

   m_pMap->m_name = path;

   return pLayer;
   }  

   
bool EnvLoader::LoadEvaluator(EnvModel *pEnvModel, LPCTSTR name, LPCTSTR path, LPCTSTR description, LPCTSTR imageURL, int id, int use, int showInResults, LPCTSTR initInfo, LPCTSTR dependencyNames)
   {
   HINSTANCE hDLL = LOAD_LIBRARY(path);
   if (hDLL == NULL)
      {
      CString msg("Unable to find ");
      msg += path;
      msg += _T(" or a dependent DLL");
      Report::ErrorMsg(msg);

      Report::BalloonMsg(msg);
      return false;
      }

   pEnvModel->m_envContext.id = id;


   FACTORYFN factory = (FACTORYFN)PROC_ADDRESS(hDLL, "Factory");

   if (!factory)
      {
      CString msg("Unable to find function 'Factory' in ");
      msg += path;
      Report::ErrorMsg(msg);
      FREE_LIBRARY(hDLL);
      return false;
      }
   else
      {
      EnvEvaluator *pEval = (EnvEvaluator*) factory(&pEnvModel->m_envContext);

      if (pEval != nullptr)
         {
         pEval->m_id = id;                  // model id as specified by the project file
         pEval->m_name = name;              // name of the model
         pEval->m_path = path;              // path to the dll
         pEval->m_initInfo = initInfo;      // string passed to the model, specified in the project file
         pEval->m_description = description;  // description, from ENVX file
         pEval->m_imageURL = imageURL;     // not currently used
         pEval->m_use = use;          // use this model during the run?
         pEval->m_hDLL = hDLL;

         //pModel->m_frequency;    // how often this process is executed
         pEval->m_dependencyNames = dependencyNames;
         pEval->m_showInResults = showInResults;
         pEnvModel->AddEvaluator(pEval);
         }
      }

   return true;
   }

bool EnvLoader::LoadModel( EnvModel *pEnvModel, LPCTSTR name, LPCTSTR path, LPCTSTR description, LPCTSTR imageURL, int id, int use, int timing, LPCTSTR initInfo, LPCTSTR dependencyNames)
   {
   HINSTANCE hDLL = LOAD_LIBRARY( path ); 
   if ( hDLL == NULL )
      {
      CString msg( "Unable to find " );
      msg += path;
      msg += _T(" or a dependent DLL" );
      Report::ErrorMsg( msg );

      Report::BalloonMsg( msg );
      return false;
      }

   FACTORYFN factory = (FACTORYFN) PROC_ADDRESS(hDLL, "Factory");

   if ( ! factory )
      {
      CString msg( "Unable to find function 'Factory' in " );
      msg += path;
      Report::ErrorMsg( msg );
      FREE_LIBRARY( hDLL );
      return false;
      }
   else
      {
      pEnvModel->m_envContext.id = id;
      EnvModelProcess *pModel = (EnvModelProcess*) factory(&pEnvModel->m_envContext);

      if (pModel != nullptr)
         {
         pModel->m_id = id;           // model id as specified by the project file
         pModel->m_name = name;         // name of the model
         pModel->m_path = path;         // path to the dll
         pModel->m_initInfo = initInfo;     // string passed to the model, specified in the project file
         pModel->m_description = description;  // description, from ENVX file
         pModel->m_imageURL = imageURL;     // not currently used
         pModel->m_use = use;          // use this model during the run?
         pModel->m_hDLL = hDLL;
         
         pModel->m_timing = timing;
         //pModel->m_frequency;    // how often this process is executed
         pModel->m_dependencyNames = dependencyNames;

         pEnvModel->AddModelProcess(pModel);
         }
      }
   
   return true;
   }


bool EnvLoader::LoadExtension( LPCTSTR name, LPCTSTR path, LPCTSTR description, LPCTSTR imageURL, int modelID, int use, int showInResults, int decisionUse, int useCol, LPCTSTR fieldname, LPCTSTR initInfo, float gain, float offset )
   {/*
   HINSTANCE hDLL = LOAD_LIBRARY( path ); 
   if ( hDLL == NULL )
      {
      CString msg( "Unable to find " );
      msg += path;
      msg += _T(" or a dependent DLL" );
      Report::ErrorMsg( msg );

      SystemErrorMsg( GetLastError() );
      return false;
      }

   RUNFN runFn = (RUNFN) PROC_ADDRESS( hDLL, "EMRun" );
   if ( ! runFn )
      {
      CString msg( "Unable to find function 'EMRun()' in " );
      msg += path;
      Report::ErrorMsg( msg );
      FREE_LIBRARY( hDLL );
      return false;
      }
   else
      {
      INITRUNFN  initRunFn  = (INITRUNFN) PROC_ADDRESS( hDLL, "EMInitRun" );
      ENDRUNFN   endRunFn   = (ENDRUNFN)  PROC_ADDRESS( hDLL, "EMEndRun" );
      INITFN     initFn     = (INITFN)    PROC_ADDRESS( hDLL, "EMInit" );
      PUSHFN     pushFn     = (PUSHFN)    PROC_ADDRESS( hDLL, "EMPush" );
      POPFN      popFn      = (POPFN)     PROC_ADDRESS( hDLL, "EMPop"  );
      SETUPFN setupFn = (SETUPFN) PROC_ADDRESS( hDLL, "EMSetup" );
      INPUTVARFN inputVarFn = (INPUTVARFN) PROC_ADDRESS( hDLL, "EMInputVar" );
      OUTPUTVARFN outputVarFn = (OUTPUTVARFN) PROC_ADDRESS( hDLL, "EMOutputVar" );
                        
      int index = m_pEnvModel->AddEvaluator( new EnvEvaluator( hDLL, runFn, initFn, initRunFn, endRunFn, pushFn, 
            popFn, setupFn, inputVarFn, outputVarFn, modelID, showInResults, decisionUse, initInfo, name, path ) );
      EnvEvaluator *pInfo =  m_pEnvModel->GetEvaluatorInfo( index );
      pInfo->m_use = use ? true : false;
      pInfo->col = useCol ? 0 : -1;
      pInfo->fieldName = fieldname;
      pInfo->gain = gain;
      pInfo->offset = offset;
      }
   */
   return true;
   }



bool EnvLoader::LoadVisualizer(LPCTSTR name, LPCTSTR path, LPCTSTR description, LPCTSTR imageURL, int use, int type, int vizID, LPCTSTR initInfo)
   {/*
   HINSTANCE hDLL = LOAD_LIBRARY( path ); 
   if ( hDLL == NULL )
      {
      CString msg( "Unable to find " );
      msg += path;
      msg += _T(" or a dependent DLL" );
      Report::ErrorMsg( msg );

      Report::BalloonMsg( msg );
      return false;
      }

   INITFN      initFn       = (INITFN)      PROC_ADDRESS( hDLL, "VInit" );
   INITRUNFN   initRunFn    = (INITRUNFN)   PROC_ADDRESS( hDLL, "VInitRun" );
   ENDRUNFN    endRunFn     = (ENDRUNFN)    PROC_ADDRESS( hDLL, "VEndRun" );
   RUNFN       runFn        = (RUNFN)       PROC_ADDRESS( hDLL, "VRun" );
   INITWNDFN   initWndFn    = (INITWNDFN)   PROC_ADDRESS( hDLL, "VInitWindow" );
   UPDATEWNDFN updateWndFn  = (UPDATEWNDFN) PROC_ADDRESS( hDLL, "VUpdateWindow" );
   SETUPFN     setupFn      = (SETUPFN)     PROC_ADDRESS( hDLL, "VSetup" );
   GETCONFIGFN getConfigFn = (GETCONFIGFN)PROC_ADDRESS(hDLL, "VGetConfig");
   SETCONFIGFN setConfigFn = (SETCONFIGFN)PROC_ADDRESS(hDLL, "VSetConfig");

   ENV_VISUALIZER *pViz = new ENV_VISUALIZER( hDLL, runFn, initFn, initRunFn, endRunFn, initWndFn, updateWndFn, setupFn, getConfigFn, setConfigFn, vizID, type, initInfo, name, path, description, imageURL, false );

   m_pEnvModel->AddVisualizer( pViz );

   pViz->m_use = use ? true : false;
   */
   return true;
   }


void EnvLoader::SetDependencies( void )
   {
   TCHAR buffer[ 256 ];
   LPTSTR nextToken;

   PtrArray< EnvProcess> tempArray( false );

   for ( int i=0; i < m_pEnvModel->GetEvaluatorCount(); i++ )
      {
      EnvEvaluator *pInfo = m_pEnvModel->GetEvaluatorInfo( i );
      if ( ! pInfo->m_dependencyNames.IsEmpty() )
         {
         lstrcpy( buffer, pInfo->m_dependencyNames );

         // parse buffer
         // Establish string and get the first token:
         LPTSTR modelName = _tcstok_s( buffer, _T(" ,;"), &nextToken ); // C4996
         while( modelName != NULL )
            {
            EnvEvaluator *pDependency = m_pEnvModel->FindEvaluatorInfo( modelName );

            if ( pDependency == NULL )
               {
               CString msg;
               msg.Format( _T("Unable to find dependency <%s> specified for model %s"), modelName, (LPCTSTR) pInfo->m_name );
               Report::ErrorMsg( msg );
               }
            else
               tempArray.Add( pInfo );

            modelName = _tcstok_s( NULL, _T(" ,;"), &nextToken );
            }
         }

      int count = (int) tempArray.GetSize();
      
      if ( count > 0 )
         {
         pInfo->m_dependencyCount = count;
         pInfo->m_dependencyArray = new EnvProcess*[ count ];
         }

      tempArray.Clear();
      }  // end of: for ( i < gpModel->GetEvaluatorCount() )


   for ( int i=0; i < m_pEnvModel->GetModelProcessCount(); i++ )
      {
      EnvModelProcess *pInfo = m_pEnvModel->GetModelProcessInfo( i );
      if ( ! pInfo->m_dependencyNames.IsEmpty() )
         {
         lstrcpy( buffer, pInfo->m_dependencyNames );

         // parse buffer
         // Establish string and get the first token:
         LPTSTR apName = _tcstok_s( buffer, _T(" ,;"), &nextToken ); // C4996
         while( apName != NULL )
            {
            EnvModelProcess *pDependency = m_pEnvModel->FindModelProcessInfo( apName );

            if ( pDependency == NULL )
               {
               CString msg;
               msg.Format( _T("Unable to find dependency <%s> specified for autonomous process %s"), apName, (LPCTSTR) pInfo->m_name );
               Report::ErrorMsg( msg );
               }
            else
               tempArray.Add( pInfo );

            apName = _tcstok_s( NULL, _T(" ,;"), &nextToken );
            }
         }

      int count = (int) tempArray.GetSize();
      
      if ( count > 0 )
         {
         pInfo->m_dependencyCount = count;
         pInfo->m_dependencyArray = new EnvProcess*[ count ];
         }

      tempArray.Clear();
      }  // end of: for ( i < gpModel->GetEvaluatorCount() )
   }


// Note: InitVisualizers should be called AFTER the models are initialized but BEFORE
// any models are run.
void EnvLoader::InitVisualizers()   
   {
   /*
   //----------------------------------------------------------------
   // ---------- Initialize Visualizers -----------------------------
   //----------------------------------------------------------------
   EnvContext &context = m_pEnvModel->m_envContext;  //( m_pIDULayer );

   context.showMessages = (m_pEnvModel->m_debug > 0);
   context.pLulcTree = &m_pEnvModel->m_lulcTree;             // lulc tree used in the simulation

   // Call the init function (if available) for the visualizer
   for ( int i=0; i < m_pEnvModel->GetVisualizerCount(); i++ )
      {
      ENV_VISUALIZER *pInfo = m_pEnvModel->GetVisualizerInfo( i );

      if ( pInfo->m_use && pInfo->initFn != NULL )
         {
         INITFN initFn = pInfo->initFn;

         if ( initFn != NULL )
            {
            CString msg = "Initializing ";
            msg += pInfo->m_name;
            Report::StatusMsg( msg );
            context.id  = pInfo->m_id;
            context.handle = pInfo->m_handle;
            context.pEnvExtension = pInfo;
            initFn( &context, (LPCTSTR) pInfo->initInfo );
            context.extra = 0;
            }
         }
      }

   Report::StatusMsg( "" ); */
   return;
   }


int EnvLoader::AddGDALPath()
   {
   // handle GDAL path
   size_t requiredSize;
   _tgetenv_s(&requiredSize, NULL, 0, "PATH");

   // Get the value of the LIB environment variable.
   TCHAR *path = new TCHAR[requiredSize + 256];
   _tgetenv_s(&requiredSize, path, requiredSize, "PATH");

   // Attempt to change path. Note that this only affects
   // the environment variable of the current process. The command
   // processor's environment is not changed.
   if (1) // _tcsstr( path, "GDAL" ) == NULL )    // no GDAL in path?
      {
      TCHAR _path[MAX_PATH];  // e.g. _path = "D:\Envision\src\x64\Debug\Envision.exe"
      _path[0] = NULL;
      int count = GetModuleFileName(NULL, _path, MAX_PATH);

      if (count == 0)
         return -1;

      //???Check - does this return Envisionexe or EnvEngine.dll?


      for (int i = 0; i < lstrlen(_path); i++)
         _path[i] = tolower(_path[i]);

      // special cases - contains "Debug" or "Release"? (on a developer machine?)
      TCHAR *debug = _tcsstr(_path, _T("debug"));
      TCHAR *release = _tcsstr(_path, _T("release"));
      TCHAR *end = NULL;
      if (debug || release)
         {
         end = _tcsstr(_path, _T("envision"));
         if (end != NULL)
            {
            end += 8 * sizeof(TCHAR);
            *end = NULL;
            }
         }

      else     // installed as an application
         {
         end = _tcsrchr(_path, '\\');
         if (end == NULL)
            end = _tcsrchr(_path, '/');
         if (end != NULL)
            *end = NULL;
         }

      CString gdalPath(_path);
      CString gdalPluginPath(_path);
      CString gdalPluginData(_path);

#if defined( _WIN64 )
      gdalPath += "\\GDAL\\release-1600-x64\\bin";
      gdalPluginPath += "\\GDAL\\release-1600-x64\\bin\\gdalplugins";
      gdalPluginData += "\\GDAL\\release-1600-x64\\bin\\gdal-data";
#else
      gdalPath += "\\GDAL\\release-1600-x86\\bin";
      gdalPluginPath += "\\GDAL\\release-1600-x86\\bin\\gdalplugins";
      gdalPluginData += "\\GDAL\\release-1600-x86\\bin\\gdal-data";
#endif      
      lstrcat(path, ";");
      lstrcat(path, gdalPath);

      _putenv_s("PATH", path);
      _putenv_s("GDAL_DRIVER_PATH", gdalPluginPath);
      _putenv_s("GDAL_DATA", gdalPluginData);

      CString msg("Appending GDAL to PATH: ");
      msg += gdalPath;
      Report::Log(msg);
      //
      msg = "Setting GDAL_DRIVER_PATH: ";
      msg += gdalPluginPath;
      Report::Log(msg);
      }  // no GDAL in PATH
   else
      {
      Report::Log("GDAL: Using existing PATH settings to locate GDAL files...");
      }

   delete[] path;
   return 1;
   }