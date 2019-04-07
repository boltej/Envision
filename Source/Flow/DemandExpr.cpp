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
// DemandExpr.cpp : Implements global evapotransporation methods for Flow
//

#include "stdafx.h"
#pragma hdrstop

#include "GlobalMethods.h"
#include "Flow.h"
#include <UNITCONV.H>
#include <omp.h>
#include <PathManager.h>


extern FlowProcess *gpFlow;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


DemandExpr::~DemandExpr()
   {
   }


bool DemandExpr::Init( FlowContext *pFlowContext )
   {
   GlobalMethod::Init( pFlowContext );  // compile query
   
   int iduCount = pFlowContext->pEnvContext->pMapLayer->GetRecordCount();

   //gpFlow->CheckCol( pFlowContext->pEnvContext->pMapLayer, m_colET, "ET", TYPE_FLOAT, CC_AUTOADD );
   //gpFlow->CheckCol( pFlowContext->pEnvContext->pMapLayer, m_colIduSoilID, "SoilID", TYPE_LONG, CC_MUST_EXIST );
   //gpFlow->CheckCol( pFlowContext->pEnvContext->pMapLayer, this->m_colStartGrowSeason, "PLANTDATE", TYPE_INT, CC_AUTOADD );
   //gpFlow->CheckCol( pFlowContext->pEnvContext->pMapLayer, this->m_colEndGrowSeason, "HARVDATE",  TYPE_INT, CC_AUTOADD );

   int hruCount = pFlowContext->pFlowModel->GetHRUCount(); 


   return true;
   }


bool DemandExpr::InitRun( FlowContext *pFlowContext )
   {
   GlobalMethod::InitRun( pFlowContext );

 
   //run dependent setup
   //m_currYear  = -1;
   //m_currMonth = -1; 
   //m_currDay   = -1;
   //m_flowContext = pFlowContext;
   //m_pCurrHru = NULL;

   return true;
   }

bool DemandExpr::StartStep( FlowContext *pFlowContext )
   {

   return true;
   }


bool DemandExpr::StartYear( FlowContext *pFlowContext )
   {
   GlobalMethod::StartYear( pFlowContext );  // this inits the m_hruQueryStatusArray to 0;

   // initialized shared arrays
   int iduCount = pFlowContext->pEnvContext->pMapLayer->GetRecordCount();

   //for ( int i=0; i < iduCount; i++ )
   //   {
   //   m_iduETArray[ i ] = 0;
   //   m_iduAnnAvgETArray[ i ] = 0;
   //   }

   //run dependent setup
   //m_currYear  = -1;
   //m_currMonth = -1; 
   //m_currDay   = -1;
   //m_flowContext = pFlowContext;
   //m_pCurrHru = NULL;
   //
   //m_runCount = 0;
   //
   //FillLookupTables( pFlowContext );
   //SetQueryStatus( pFlowContext->pFlowModel );
   return true;
   }


bool DemandExpr::Run( FlowContext *pFlowContext )
   {
   if ( GlobalMethod::Run( pFlowContext ) == true )
      return true;

   if ( pFlowContext == NULL )
      return false;

   // set local run-level variables
   //m_flowContext = pFlowContext;
   //
   //if ( pFlowContext->pHRULayer != NULL )
   //   m_pCurrHru = pFlowContext->pHRULayer->m_pHRU;
   //
   //m_currYear = pFlowContext->pEnvContext->currentYear;
   //               
   //::GetCalDate(pFlowContext->dayOfYear, &m_currYear, &m_currMonth, &m_currDay, TRUE );
   // 
   //// makes sure any given month can't have more than 30 days
   //if ( m_currDay > 30)
   //   m_currDay = 30;
   // 
   //int hruCount = pFlowContext->pFlowModel->GetHRUCount();
   //
   //// iterate through hrus/hrulayers 
  ////    #pragma omp parallel for firstprivate( pFlowContext )
   //for ( int h=0; h < hruCount; h++ )
   //   {
   //   float aet = 0.0f;
   //   float pet = 0.0f;
   //
   //   m_pCurrHru = pFlowContext->pHRU = pFlowContext->pFlowModel->GetHRU(h);
   //
   //   if ( DoesHRUPassQuery( h ) )
   //      {
   //      GetHruET( pFlowContext, h, pet, aet);  // returns mm/d of actual ET
   //
   //      m_pCurrHru->m_currentET  = aet;    // mm/day (area weighted)
   //      m_pCurrHru->m_currentPET = pet;    // mm/day
   //      }
   //
   //   // add this demand as a negative flux from the appropriate HRU layers
   //
   //   if ( aet > 0 )
   //      {
   //      for ( int i=0; i < (int) m_layerDistrArray.GetSize(); i++ )
   //         {
   //         LayerDistr *pLD = m_layerDistrArray[ i ];
   //         HRULayer *pHRULayer = m_pCurrHru->GetLayer( pLD->layerIndex );
   //
   //         float flux = aet * pLD->fraction * m_pCurrHru->m_area / MM_PER_M;  // KELLIE - PLEASE CONFIRM THIS!
   //         pHRULayer->AddFluxFromGlobalHandler( flux, FL_TOP_SINK  );     // m3/d
   //         }
   //      }
   //   }
   //
   //m_runCount++;

   return TRUE;
   }


bool DemandExpr::EndYear( FlowContext *pFlowContext )
   {
   FlowModel *pModel = pFlowContext->pFlowModel;
   MapLayer  *pIDULayer = (MapLayer*) pFlowContext->pEnvContext->pMapLayer;

   int iduCount = (int) pFlowContext->pEnvContext->pMapLayer->GetRecordCount();
   
   // write annual ET to map
   //for ( int i=0; i < iduCount; i++ )
   //   {
   //   m_iduAnnAvgETArray[ i ] /= m_runCount;   // runcount =365*4
   //   gpFlow->UpdateIDU( pFlowContext->pEnvContext, i, m_colET, m_iduAnnAvgETArray[ i ], true );
   //   }
   //
   //// write planting dates to map
   //for ( int h=0; h < pModel->GetHRUCount(); h++ )
   //   {
   //   HRU *pHRU = pModel->GetHRU( h );
   //
   //   for ( int i=0; i < (int) pHRU->m_polyIndexArray.GetSize(); i++ )
   //      {
   //      int idu = pHRU->m_polyIndexArray[ i ];
   //
   //   	int lulc = -1;
	//		if ( pIDULayer->GetData( idu, m_colLulc, lulc ) )   //pFlowContext->pFlowModel->GetHRUData( pHRU, m_colLulc, lulc, DAM_FIRST ) )
	//		   {
	//		   // have lulc for this HRU, find the corresponding row in the cc table
	//			int row = m_pCropTable->Find( m_colCropTableLulc, VData( lulc ), 0 ); 
	//			
   //         int startDate = -1;
	//		   int harvestDate = -1;
   //         if (row >= 0 )
   //            {
   //            startDate = this->m_phruCropStartGrowingSeasonArray->Get( h, row );
   //            harvestDate = this->m_phruCropEndGrowingSeasonArray->Get( h, row );
   //            }
   //
   //         gpFlow->UpdateIDU( pFlowContext->pEnvContext, idu, m_colStartGrowSeason, startDate, true );
   //         gpFlow->UpdateIDU( pFlowContext->pEnvContext, idu, m_colEndGrowSeason, harvestDate, true );
   //         }
   //      }
   //   }
   //
   //  for ( int h=0; h < pModel->GetHRUCount(); h++ )
   //   {
   //   for (int i = 0; i < m_cropCount; i++)
   //      {
   //       m_phruCropStartGrowingSeasonArray->Set(h, i, 365);
   //       m_phruCropEndGrowingSeasonArray->Set(h, i, 364);
   //       m_phruCurrCGDDArray->Set(h, i, 0.0f);
   //      }
   //    }

   return true;
   }


int DemandExpr::ParseLayerDistributions( void )
   {
   if ( this->m_layerDistributions.IsEmpty() )
      return 0;

   TCHAR *values = new TCHAR[ this->m_layerDistributions.GetLength() + 2 ];
   lstrcpy( values, this->m_layerDistributions );

   TCHAR *nextToken = NULL;
   LPTSTR token = _tcstok_s( values, _T(",() ;\r\n"), &nextToken );

   while ( token != NULL )
      {
      LayerDistr *pLD = new LayerDistr;
      pLD->layerIndex = atoi( token );
      token   = _tcstok_s( NULL, _T(",() ;\r\n"), &nextToken );
      pLD->fraction = (float) atof( token );
      token   = _tcstok_s( NULL, _T(",() ;\r\n"), &nextToken );
   
      m_layerDistrArray.Add( pLD );
      }

   delete [] values;

   return (int) m_layerDistrArray.GetSize();
   }


DemandExpr *DemandExpr::LoadXml( TiXmlElement *pXmlDemandExpr, MapLayer *pIDULayer, LPCTSTR filename )
	  {
	  if ( pXmlDemandExpr == NULL )
		 return NULL;

//	  // add "WaterDemand" directory to the Envision search paths
//	  CString wdDir = PathManager::GetPath( PM_PROJECT_DIR );    // envx file directory, slash-terminated
//	  wdDir += _T( "WaterDemand" );
//
//	  PathManager::AddPath( wdDir );
//
//	  // get attributes
//	  LPTSTR name=NULL;
//	  LPTSTR cropTablePath=NULL;
//	  LPTSTR soilTablePath=NULL;
//	  LPTSTR method=NULL;
//	  LPTSTR layerDistributions=NULL;
//	  LPTSTR query=NULL;
//	  bool use = true;
//	  LPTSTR lulcCol = NULL;
//	  LPTSTR soilCol = NULL;
//	  float latitude = 45;
//	  float longitude=123;
//
//   
//	  XML_ATTR attrs[] = {
//		 // attr                 type          address               isReq  checkCol
//		 { "name",               TYPE_STRING,    &name,              false,   0 },
//		 { "method",             TYPE_STRING,    &method,            true,    0 },
//		 { "query",              TYPE_STRING,    &query,             false,   0 },
//		 { "use",                TYPE_BOOL,      &use,               false,   0 },
//		 { "lulc_col",           TYPE_STRING,    &lulcCol,           true,    CC_MUST_EXIST },      
//		 { "soil_col",           TYPE_STRING,    &soilCol,           true,    CC_MUST_EXIST },      
//		 { "crop_table",         TYPE_STRING,    &cropTablePath,         false,   0 },
//		 { "soil_table",         TYPE_STRING,    &soilTablePath,         false,   0 },
//		 { "layer_distributions",TYPE_STRING,    &layerDistributions,false,   0 },
//		 { "latitude",           TYPE_FLOAT,     &latitude,          true,    0 },
//		 { "longitude",          TYPE_FLOAT,     &longitude,         true,    0 },
//		 { NULL,                 TYPE_NULL,      NULL,               false,  0 } };
//
//	  bool ok = TiXmlGetAttributes( pXmlDemandExpr, attrs, filename, pIDULayer );
//	  if ( ! ok )
//		 {
//		 CString msg; 
//		 msg.Format( _T("Flow: Misformed element reading <evap_trans> attributes in input file %s"), cropTablePath );
//		 Report::ErrorMsg( msg );
//		 return NULL;
//		 }
//
//	  DemandExpr *pDemandExpr = new DemandExpr( name );
//
//	  pDemandExpr->m_colLulc = pIDULayer->GetFieldCol( lulcCol );
//	  pDemandExpr->m_colIduSoilID = pIDULayer->GetFieldCol( soilCol );
//	  pDemandExpr->m_colArea = pIDULayer->GetFieldCol( "AREA" );
//
//	  if ( method != NULL )
//		 {
//		 switch( method[ 0 ] )
//			{
//			case 'f':
//			case 'F':
//			   {
//			   if ( method[ 1 ] =='a' || method[ 1 ] == 'A' )
//				  {
//				  pDemandExpr->SetMethod( GM_FAO56 );
//				  }
//			   else if ( method[ 1 ] =='n' || method[ 1 ] == 'N' )
//				  {
//				  pDemandExpr->m_extSource = method;
//				  FLUXSOURCE sourceLocation = ParseSource( pDemandExpr->m_extSource, pDemandExpr->m_extPath, pDemandExpr->m_extFnName,
//					 pDemandExpr->m_extFnDLL, pDemandExpr->m_extFn );
//   
//				  if ( sourceLocation != FS_FUNCTION )
//					 {
//					 CString msg;
//					 msg.Format( "Error parsing <external_method> 'fn=%s' specification for '%s'.  This method will not be invoked.", method, name );
//					 Report::ErrorMsg( msg );
//					 pDemandExpr->SetMethod( GM_NONE );
//					 pDemandExpr->m_use = false;
//					 }
//				  else
//					 pDemandExpr->SetMethod( GM_EXTERNAL );
//				  }
//			   }
//			   break;
//
//			case 'p':
//			case 'P':
//			   pDemandExpr->SetMethod( GM_PENMAN_MONTIETH );
//			   pDemandExpr->m_ETEq.SetMode( ETEquation::PENN_MONT );
//			   break;
//
//			case 'h':
//			case 'H':
//			   pDemandExpr->SetMethod( GM_HARGREAVES );
//			   pDemandExpr->m_ETEq.SetMode( ETEquation::HARGREAVES );
//			   break;
//
//			default:
//			   pDemandExpr->SetMethod( GM_NONE );
//			}
//		 }
//
//	  if ( cropTablePath == NULL )
//		 {
//		 CString msg;
//		 msg.Format( "DemandExpr: Missing crop_table attribute when reading '%s'.  This method will be ignored", cropTablePath );
//		 Report::LogMsg( msg, RT_WARNING );
//		 pDemandExpr->SetMethod( GM_NONE );
//		 return pDemandExpr;
//		 }
//
//	  // does file exist?
//	  CString tmpPath;
//
//	  if ( PathManager::FindPath( cropTablePath, tmpPath ) < 0 )
//		 {
//		 CString msg;
//		 msg.Format( "DemandExpr: Specified crop table '%s' for method '%s' can not be found, this method will be ignored", cropTablePath, name );
//		 Report::LogMsg( msg, RT_WARNING );
//		 pDemandExpr->SetMethod( GM_NONE );
//		 return pDemandExpr;
//		 }
//
//
//	  pDemandExpr->m_cropTablePath = tmpPath;
//	  pDemandExpr->m_pCropTable = new VDataObj();
//
//	  pDemandExpr->m_cropCount = pDemandExpr->m_pCropTable->ReadAscii( tmpPath, ',');
//
//	  if ( pDemandExpr->m_cropCount <= 0 )
//	  {      
//	  CString msg;
//	  msg.Format("DemandExpr::LoadXML could not load Crop .csv file \n");
//	  Report::InfoMsg( msg );
//	  return false;
//	  }
//
//
//	  if ( soilTablePath == NULL )
//		 {
//		 CString msg;
//		 msg.Format( "DemandExpr: Missing soil_table attribute for method '%s'.  This method will be ignored", name );
//		 Report::LogMsg( msg, RT_WARNING );
//		 pDemandExpr->SetMethod( GM_NONE );
//		 return pDemandExpr;
//		 }
//
//	  // does file exist?
//	  if ( PathManager::FindPath( soilTablePath, tmpPath ) < 0 )
//		 {
//		 CString msg;
//		 msg.Format( "DemandExpr: Specified soil table '%s' for method '%s' can not be found, this method will be ignored", soilTablePath, name );
//		 Report::LogMsg( msg, RT_WARNING );
//		 pDemandExpr->SetMethod( GM_NONE );
//		 return pDemandExpr;
//		 }
//
//	  
//	  pDemandExpr->m_soilTablePath = soilTablePath;
//	  pDemandExpr->m_pSoilTable = new FDataObj();
//	  int soilCount = pDemandExpr->m_pSoilTable->ReadAscii( tmpPath, ',' );
//
//	  if ( soilCount <= 0 )
//	  {      
//	  CString msg;
//	  msg.Format("DemandExpr::LoadXML could not load Soil .csv file \n");
//	  Report::InfoMsg( msg );
//	  return false;
//	  }
//
//	  // get column number for column headers
//	  pDemandExpr->m_colCropTableLulc   = pDemandExpr->m_pCropTable->GetCol( lulcCol );
//	  pDemandExpr->m_plantingMethodCol = pDemandExpr->m_pCropTable->GetCol( "Planting Date Method" );
//     pDemandExpr->m_plantingThresholdCol = pDemandExpr->m_pCropTable->GetCol( "Planting Threshold" );
//	  pDemandExpr->m_gddEquationTypeCol = pDemandExpr->m_pCropTable->GetCol( "GDD Equation" );
//	  pDemandExpr->m_gddBaseTempCol = pDemandExpr->m_pCropTable->GetCol( "GDD Base Temp [C]" );
//	  pDemandExpr->m_termMethodCol = pDemandExpr->m_pCropTable->GetCol( "Term Date Method" );
//	  pDemandExpr->m_termTempCol = pDemandExpr->m_pCropTable->GetCol( "Killing Frost Temp [C]" );
//	  pDemandExpr->m_termCGDDCol = pDemandExpr->m_pCropTable->GetCol( "CGDD P to T");
//
//	  pDemandExpr->m_binToEFCCol = pDemandExpr->m_pCropTable->GetCol( "GDD per 10% P to EFC" );
//	  pDemandExpr->m_cGDDtoEFCCol = pDemandExpr->m_pCropTable->GetCol( "CGDD P/GU to EFC" );
//	  pDemandExpr->m_binEFCtoTermCol = pDemandExpr->m_pCropTable->GetCol( "GDD per 10% EFC to T" );
//	  pDemandExpr->m_binMethodCol = pDemandExpr->m_pCropTable->GetCol( "Interp Method EFC to T (1-CGDD; 2-constant)" );
//	  pDemandExpr->m_constCropCoeffEFCtoTCol = pDemandExpr->m_pCropTable->GetCol( "Kc value EFC to T if constant" );
//
//	  if ( pDemandExpr->m_colCropTableLulc < 0 || pDemandExpr->m_plantingThresholdCol < 0 || 
//		   pDemandExpr->m_plantingThresholdCol < 0 || pDemandExpr->m_gddEquationTypeCol < 0 || 
//		   pDemandExpr->m_gddBaseTempCol < 0 || pDemandExpr->m_termMethodCol < 0 || pDemandExpr->m_termTempCol < 0 || 
//		   pDemandExpr->m_termCGDDCol < 0 || pDemandExpr->m_binToEFCCol < 0 || pDemandExpr->m_cGDDtoEFCCol < 0 || 
//		   pDemandExpr->m_binEFCtoTermCol < 0 || pDemandExpr->m_binMethodCol < 0 || pDemandExpr->m_constCropCoeffEFCtoTCol < 0 )          
//	  {
//	  CString msg;
//	  msg.Format("DemandExpr::LoadXML: One or more column headings are incorrect in Crop .csv file\n");
//	  Report::ErrorMsg( msg );
//	  return false;
//	  }
//
//	  int per0Col = pDemandExpr->m_pCropTable->GetCol( "0%" );
//	  int per10Col = pDemandExpr->m_pCropTable->GetCol( "10%" );
//	  int per20Col = pDemandExpr->m_pCropTable->GetCol( "20%" );
//	  int per30Col = pDemandExpr->m_pCropTable->GetCol( "30%" );
//	  int per40Col = pDemandExpr->m_pCropTable->GetCol( "40%" );
//	  int per50Col = pDemandExpr->m_pCropTable->GetCol( "50%" );
//	  int per60Col = pDemandExpr->m_pCropTable->GetCol( "60%" );
//	  int per70Col = pDemandExpr->m_pCropTable->GetCol( "70%" );
//	  int per80Col = pDemandExpr->m_pCropTable->GetCol( "80%" );
//	  int per90Col = pDemandExpr->m_pCropTable->GetCol( "90%" );
//	  int per100Col = pDemandExpr->m_pCropTable->GetCol( "100%" );
//	  int per110Col = pDemandExpr->m_pCropTable->GetCol( "110%" );
//	  int per120Col = pDemandExpr->m_pCropTable->GetCol( "120%" );
//	  int per130Col = pDemandExpr->m_pCropTable->GetCol( "130%" );
//	  int per140Col = pDemandExpr->m_pCropTable->GetCol( "140%" );
//	  int per150Col = pDemandExpr->m_pCropTable->GetCol( "150%" );
//	  int per160Col = pDemandExpr->m_pCropTable->GetCol( "160%" );
//	  int per170Col = pDemandExpr->m_pCropTable->GetCol( "170%" );
//	  int per180Col = pDemandExpr->m_pCropTable->GetCol( "180%" );
//	  int per190Col = pDemandExpr->m_pCropTable->GetCol( "190%" );
//	  int per200Col = pDemandExpr->m_pCropTable->GetCol( "200%" );
//
//	  int colCount = pDemandExpr->m_cropCount + 1;
//
//	  pDemandExpr->m_pCropCoefficientLUT = new FDataObj(colCount, 21);
//
//	  float percentile;
//	  for (int i = 0; i < 21; i ++)
//	  {
//	  percentile = i * 10.0f;
//	  pDemandExpr->m_pCropCoefficientLUT->Set( 0, i, percentile );
//	  }
//
//	  for ( int i=0; i < pDemandExpr->m_cropCount; i++ )
//	  {
//		int j = 0;
//		pDemandExpr->m_pCropCoefficientLUT->Set( i + 1, j++, pDemandExpr->m_pCropTable->GetAsFloat(per0Col, i));
//		pDemandExpr->m_pCropCoefficientLUT->Set( i + 1, j++, pDemandExpr->m_pCropTable->GetAsFloat(per10Col, i));
//		pDemandExpr->m_pCropCoefficientLUT->Set( i + 1, j++, pDemandExpr->m_pCropTable->GetAsFloat(per20Col, i));
//		pDemandExpr->m_pCropCoefficientLUT->Set( i + 1, j++, pDemandExpr->m_pCropTable->GetAsFloat(per30Col, i));
//		pDemandExpr->m_pCropCoefficientLUT->Set( i + 1, j++, pDemandExpr->m_pCropTable->GetAsFloat(per40Col, i));
//		pDemandExpr->m_pCropCoefficientLUT->Set( i + 1, j++, pDemandExpr->m_pCropTable->GetAsFloat(per50Col, i));
//		pDemandExpr->m_pCropCoefficientLUT->Set( i + 1, j++, pDemandExpr->m_pCropTable->GetAsFloat(per60Col, i));
//		pDemandExpr->m_pCropCoefficientLUT->Set( i + 1, j++, pDemandExpr->m_pCropTable->GetAsFloat(per70Col, i));
//		pDemandExpr->m_pCropCoefficientLUT->Set( i + 1, j++, pDemandExpr->m_pCropTable->GetAsFloat(per80Col, i));
//		pDemandExpr->m_pCropCoefficientLUT->Set( i + 1, j++, pDemandExpr->m_pCropTable->GetAsFloat(per90Col, i));
//		pDemandExpr->m_pCropCoefficientLUT->Set( i + 1, j++, pDemandExpr->m_pCropTable->GetAsFloat(per100Col, i));
//		pDemandExpr->m_pCropCoefficientLUT->Set( i + 1, j++, pDemandExpr->m_pCropTable->GetAsFloat(per110Col, i));
//		pDemandExpr->m_pCropCoefficientLUT->Set( i + 1, j++, pDemandExpr->m_pCropTable->GetAsFloat(per120Col, i));
//		pDemandExpr->m_pCropCoefficientLUT->Set( i + 1, j++, pDemandExpr->m_pCropTable->GetAsFloat(per130Col, i));
//		pDemandExpr->m_pCropCoefficientLUT->Set( i + 1, j++, pDemandExpr->m_pCropTable->GetAsFloat(per140Col, i));
//		pDemandExpr->m_pCropCoefficientLUT->Set( i + 1, j++, pDemandExpr->m_pCropTable->GetAsFloat(per150Col, i));
//		pDemandExpr->m_pCropCoefficientLUT->Set( i + 1, j++, pDemandExpr->m_pCropTable->GetAsFloat(per160Col, i));
//		pDemandExpr->m_pCropCoefficientLUT->Set( i + 1, j++, pDemandExpr->m_pCropTable->GetAsFloat(per170Col, i));
//		pDemandExpr->m_pCropCoefficientLUT->Set( i + 1, j++, pDemandExpr->m_pCropTable->GetAsFloat(per180Col, i));
//		pDemandExpr->m_pCropCoefficientLUT->Set( i + 1, j++, pDemandExpr->m_pCropTable->GetAsFloat(per190Col, i));
//		pDemandExpr->m_pCropCoefficientLUT->Set( i + 1, j++, pDemandExpr->m_pCropTable->GetAsFloat(per200Col, i));
//	  }
//
//	  //_pDemandExpr->m_colIduSoilID =        // "SoilID"
//
//     pDemandExpr->m_colSoilTableSoilID = pDemandExpr->m_pSoilTable->GetCol( soilCol );
//  
//
//	  pDemandExpr->m_colWP = pDemandExpr->m_pSoilTable->GetCol( "WP" );
//	  //pDemandExpr->m_colFC = pDemandExpr->m_pSoilTable->GetCol( "FC" );
//	  pDemandExpr->m_colLP = pDemandExpr->m_pSoilTable->GetCol( "LP" );
//     pDemandExpr->m_layerDistributions=layerDistributions;
//     pDemandExpr->ParseLayerDistributions();
//   
//
//	  //pDemandExpr->m_colCropTableLulc   = pDemandExpr->m_pCropTable->GetCol( "LULC" );
//	  ////_pDemandExpr->m_colIrriEff         = _pDemandExpr->m_pCropTable->GetCol( "irriEff" );                   // irrigation efficiency for croptype
//	  ////_pDemandExpr->m_colConveyEff       = _pDemandExpr->m_pCropTable->GetCol( "conveyEff" );                 // conveyance efficiency for croptype
//	  //pDemandExpr->m_colStartGrowSeason = pDemandExpr->m_pCropTable->GetCol( "startDate_doy" );           // doy (=day of year) for start of growing season
//	  //pDemandExpr->m_colEndGrowSeason   = pDemandExpr->m_pCropTable->GetCol( "termDate_doy" );             // doy (=day of year) for end of growing season
//	  //_pDemandExpr->m_colPercentIrri     = _pDemandExpr->m_pCropTable->GetCol( "percent_irrigated" );               // percent of irri in total area for croptype
//
//	  pDemandExpr->m_latitude = latitude;
//	  pDemandExpr->m_longitude = longitude;
//
//	  if ( query )
//		 pDemandExpr->m_query = query;
//
//	  return pDemandExpr;
return NULL;

	  }

	
                     


