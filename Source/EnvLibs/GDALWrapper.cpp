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
#include "EnvLibs.h"
#pragma hdrstop

#ifndef NO_MFC
#include <afxdll_.h>
#endif

#include "GDALWrapper.h"
#include "Maplayer.h"
#include "Report.h"
#include "..\..\GDAL\include\cpl_conv.h"
#include <math.h>
#include <cstdlib>
#include <Path.h>
//#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include<iostream>
#include<fstream>
#include <string>
using namespace std;

#define pi 3.14159265358979323846
#define BUFSIZE 4096
const double PI       =	pi;
const double deg2rad  =	PI/180;
const double rad2deg  =	180/PI;
const double k0       =	0.9996;

GDALWrapper::GDALWrapper( void ) 
: 
m_hInst( 0 )
,m_AllRegisterFn( NULL )
,m_OpenFn( NULL )
,m_CreateFn( NULL )
,m_CPLErrorFn( NULL )
,m_CPLGetLastErrorMsgFn( NULL )
,m_GetRasterBandFn( NULL )
,m_GetRasterBandXSizeFn( NULL )
,m_GetRasterBandYSizeFn( NULL )
,m_RasterIOFn( NULL )
,m_GetRasterNoDataValueFn( NULL )
,m_SetRasterNoDataValueFn( NULL )
,m_CPLMallocFn( NULL )
,m_CloseFn( NULL )
,m_GetGeoTransformFn( NULL )
,m_GetRasterCountFn( NULL )
,m_GetDataSetDriverFn( NULL )
,m_GetDriverShortNameFn( NULL )
,m_GetDriverLongNameFn( NULL )
,m_DataSetRasterIOFn( NULL )
,m_GetRasterDataTypeFn( NULL )
,m_ApplyGeoTransformFn( NULL )
,m_InvGeoTransfromFn( NULL )
,m_SetProjectionFn( NULL )
,m_GetProjectionRefFn( NULL )
,m_OSRNewSpatialReferenceFn( NULL )
,m_OSRCloneGeogCSFn( NULL )
,m_OSRDestroySpatialReferenceFn( NULL )
,m_OCTTransformFn( NULL )
,m_OCTNewCoordinateTransformationFn( NULL )
,m_DecToDMSFn( NULL )
,m_OSRImportFromWKTFn( NULL )
,m_GetDriverByNameFn( NULL )
,m_SetGeoTransformFn( NULL )
,m_OSRSetUTMFn( NULL )
,m_OSRSetWellKnownGeogCSFn( NULL )
,m_OSRExportToWKTFn( NULL )
,m_SetMetadataItemFn( NULL )
,m_GDALCreateGenImgProjTransformerFn( NULL )
,m_GDALCreateGenImgProjTransformer2Fn( NULL )
,m_GDALCreateGenImgProjTransformer3Fn( NULL )
,m_GDALDestroyGenImgProjTransformerFn( NULL )
,m_GDALChunkAndWarpImageFn( NULL )
,m_GDALSetRasterColorTableFn( NULL )
,m_GDALGetRasterColorTableFn( NULL )
,m_GDALSuggestedWarpOutputFn( NULL )
,m_GDALGetRasterXSizeFn( NULL )		
,m_GDALGetRasterYSizeFn( NULL )		
,m_GDALCreateWarpOptionsFn( NULL )	
,m_GDALGenImgProjTransformFn( NULL )
,m_GDALDestroyWarpOptionsFn( NULL )
,m_GDALCreateWarpOperationFn( NULL )
,m_GDALDestroyWarpOperationFn( NULL )
,m_GDALGetRasterCountFn( NULL ) 
,m_GDALCreateCopyFn( NULL )
//.. add more as needed
   { }

GDALWrapper::~GDALWrapper( void ) 
{ 
  if ( m_hInst != 0 ) 
    AfxFreeLibrary( m_hInst ); 
}

bool GDALWrapper::Init( void )
   {
  #if defined( _WIN64 )
   m_hInst = ::AfxLoadLibrary("gdal18.dll");
  #else
   m_hInst = ::AfxLoadLibrary("gdal19.dll");
  #endif

   if ( m_hInst == 0 )
      {
      CString msg( "GDALWrapper: Unable to find GDAL binaries" );
     
      Report::ErrorMsg( msg );
      return false;
      }
  #if defined( _WIN64 )
   m_GDALCreateCopyFn                       = (GDAL_GDALCREATECOPYFN)					GetProcAddress(m_hInst,"GDALCreateCopy");
   m_GDALCreateWarpOperationFn				= (GDAL_GDALCREATEWARPOPERATIONFN  )		GetProcAddress(m_hInst,"GDALCreateWarpOperation");
   m_GDALDestroyWarpOperationFn				= (GDAL_GDALDESTROYWARPOPERATIONFN )		GetProcAddress(m_hInst,"GDALDestroyWarpOperation");
   m_GDALGetRasterCountFn					= (GDAL_GDALGETRASTERCOUNTFN)				GetProcAddress(m_hInst,"GDALGetRasterCount");
   m_GDALGetRasterXSizeFn					= (GDAL_GDALGETRASTERXSIZEFN)				GetProcAddress(m_hInst,"GDALGetRasterXSize");		
   m_GDALGetRasterYSizeFn					= (GDAL_GDALGETRASTERYSIZEFN)				GetProcAddress(m_hInst,"GDALGetRasterYSize");		
   m_GDALCreateWarpOptionsFn				= (GDAL_GDALCREATEWARPOPTIONSFN)			GetProcAddress(m_hInst,"GDALCreateWarpOptions");	
   m_GDALGenImgProjTransformFn				= (GDALTransformerFunc)						GetProcAddress(m_hInst,"GDALGenImgProjTransform");
   m_GDALDestroyWarpOptionsFn				= (GDAL_GDALDESTROYWARPOPTIONSFN)			GetProcAddress(m_hInst,"GDALDestroyWarpOptions");
   m_GDALSetRasterColorTableFn				= (GDAL_GDALSETRASTERCOLORTABLEFN)			GetProcAddress(m_hInst,"GDALSetRasterColorTable");
   m_GDALGetRasterColorTableFn				= (GDAL_GDALGETRASTERCOLORTABLEFN)			GetProcAddress(m_hInst,"GDALGetRasterColorTable");
   m_GDALSuggestedWarpOutputFn				= (GDAL_GDALSUGGESTEDWARPOUTPUTFN)			GetProcAddress(m_hInst,"GDALSuggestedWarpOutput");
   m_GDALChunkAndWarpImageFn				= (GDAL_GDALCHUNKANDWARPIMAGEFN)			GetProcAddress(m_hInst,"GDALChunkAndWarpImage");        
   m_GDALCreateGenImgProjTransformerFn		= (GDAL_GDALCREATEGENIMGPROJTRANSFORMERFN)  GetProcAddress(m_hInst,"GDALCreateGenImgProjTransformer");
   m_GDALCreateGenImgProjTransformer2Fn		= (GDAL_GDALCREATEGENIMGPROJTRANSFORMER2FN) GetProcAddress(m_hInst,"GDALCreateGenImgProjTransformer2");
   m_GDALCreateGenImgProjTransformer3Fn		= (GDAL_GDALCREATEGENIMGPROJTRANSFORMER3FN) GetProcAddress(m_hInst,"GDALCreateGenImgProjTransformer3");
   m_GDALDestroyGenImgProjTransformerFn		= (GDAL_GDALDESTROYGENIMGPROJTRANSFORMERFN) GetProcAddress(m_hInst,"GDALDestroyGenImgProjTransformer");
   m_AllRegisterFn							= (GDAL_ALLREGISTERFN)						GetProcAddress(m_hInst,"GDALAllRegister"); 
   m_OpenFn									= (GDAL_OPENFN)								GetProcAddress(m_hInst,"GDALOpen"); 
   m_CreateFn								= (GDAL_CREATEFN)							GetProcAddress(m_hInst,"GDALCreate"); 
   m_CPLErrorFn								= (GDAL_CPLERRORFN)							GetProcAddress(m_hInst,"CPLError");
   m_CPLGetLastErrorMsgFn              = (GDAL_CPLGETLASTERRORMSG)            GetProcAddress(m_hInst, "CPLGetLastErrorMsg");
   m_GetRasterBandFn						= (GDAL_GDALGETRASTERBANDFN)				GetProcAddress(m_hInst,"GDALGetRasterBand");
   m_GetRasterBandXSizeFn					= (GDAL_GDALGETRASTERBANDXSIZEFN)			GetProcAddress(m_hInst,"GDALGetRasterBandXSize");
   m_GetRasterBandYSizeFn					= (GDAL_GDALGETRASTERBANDYSIZEFN)			GetProcAddress(m_hInst,"GDALGetRasterBandYSize");
   m_RasterIOFn								= (GDAL_GDALRASTERIOFN)						GetProcAddress(m_hInst,"GDALRasterIO");
   m_GetRasterNoDataValueFn					= (GDAL_GDALGETRASTERNODATAVALUEFN)			GetProcAddress(m_hInst,"GDALGetRasterNoDataValue");
   m_SetRasterNoDataValueFn					= (GDAL_GDALSETRASTERNODATAVALUEFN)			GetProcAddress(m_hInst,"GDALSetRasterNoDataValue");
   m_CPLMallocFn							= (GDAL_CPLMALLOCFN)						GetProcAddress(m_hInst,"CPLMalloc");
   m_CloseFn								= (GDAL_GDALCLOSEFN)						GetProcAddress(m_hInst,"GDALClose");
   m_GetGeoTransformFn						= (GDAL_GDALGETGEOTRANSFORMFN)				GetProcAddress(m_hInst,"GDALGetGeoTransform");
   m_ApplyGeoTransformFn					= (GDAL_GDALAPPLYGEOTRANSFORMFN)			GetProcAddress(m_hInst,"GDALApplyGeoTransform");
   m_InvGeoTransfromFn						= (GDAL_GDALINVGEOTRANSFORMFN)				GetProcAddress(m_hInst,"GDALInvGeoTransform");
   m_GetRasterCountFn						= (GDAL_GDALGETRASTERCOUNTFN)				GetProcAddress(m_hInst,"GDALGetRasterCount");
   m_GetDataSetDriverFn						= (GDAL_GDALGETDATASETDRIVERFN)				GetProcAddress(m_hInst,"GDALGetDatasetDriver");
   m_GetDriverShortNameFn					= (GDAL_GDALGETDRIVERSHORTNAMEFN)			GetProcAddress(m_hInst,"GDALGetDriverShortName");
   m_GetDriverLongNameFn					= (GDAL_GDALGETDRIVERLONGNAMEFN) 			GetProcAddress(m_hInst,"GDALGetDriverLongName");
   m_DataSetRasterIOFn						= (GDAL_GDALDATASETRASTERIOFN)				GetProcAddress(m_hInst,"GDALDatasetRasterIO");
   m_GetRasterDataTypeFn					= (GDAL_GDALGETRASTERDATATYPEFN)			GetProcAddress(m_hInst,"GDALGetRasterDataType");
   m_SetProjectionFn						= (GDAL_GDALSETPROJECTIONFN )				GetProcAddress(m_hInst,"GDALSetProjection");           
   m_GetProjectionRefFn						= (GDAL_GDALGETPROJECTIONREFFN)				GetProcAddress(m_hInst,"GDALGetProjectionRef");          
   m_OSRNewSpatialReferenceFn				= (GDAL_OSRNEWSPATIALREFERENCEFN)			GetProcAddress(m_hInst,"OSRNewSpatialReference");        
   m_OSRCloneGeogCSFn						= (GDAL_OSRCLONEGEOGCSFN )					GetProcAddress(m_hInst,"OSRCloneGeogCS");               
   m_OSRDestroySpatialReferenceFn			= (GDAL_OSRDESTROYSPATIALREFERENCEFN )		GetProcAddress(m_hInst,"OSRDestroySpatialReference");   
   m_OCTTransformFn							= (GDAL_OCTTRANSFORMFN )					GetProcAddress(m_hInst,"OCTTransform");                 
   m_OCTNewCoordinateTransformationFn		= (GDAL_OCTNEWCOORDINATETRANSFORMATIONFN)	GetProcAddress(m_hInst,"OCTNewCoordinateTransformation");
   m_DecToDMSFn								= (GDAL_GDALDECTODMSFN)						GetProcAddress(m_hInst,"GDALDecToDMS");                  
   m_OSRImportFromWKTFn						= (GDAL_OSRIMPORTFROMWKTFN)					GetProcAddress(m_hInst,"OSRImportFromWkt");              
   m_GetDriverByNameFn						= (GDAL_GDALGETDRIVERBYNAMEFN)				GetProcAddress(m_hInst,"GDALGetDriverByName");
   m_SetGeoTransformFn						= (GDAL_GDALSETGEOTRANSFORMFN)				GetProcAddress(m_hInst,"GDALSetGeoTransform");
   m_OSRSetUTMFn							= (GDAL_OSRSETUTMFN)						GetProcAddress(m_hInst,"OSRSetUTM");
   m_OSRSetWellKnownGeogCSFn				= (GDAL_OSRSETWELLKNOWNGEOGCSFN)			GetProcAddress(m_hInst,"OSRSetWellKnownGeogCS");
   m_OSRExportToWKTFn						= (GDAL_OSREXPORTTOWKTFN)					GetProcAddress(m_hInst,"OSRExportToWkt");
   m_SetMetadataItemFn						= (GDAL_GDALSETMETADATAITEMFN)				GetProcAddress(m_hInst,"GDALSetMetadataItem");
   m_GetMetadataFn							= (GDAL_GDALGETMETADATAFN)					GetProcAddress(m_hInst,"GDALGetMetadata");

    //add more as needed....
   #else
   m_GDALCreateGenImgProjTransformerFn   = (GDAL_GDALCREATEGENIMGPROJTRANSFORMERFN)  GetProcAddress( m_hInst,  "GDALCreateGenImgProjTransformer");
   m_GDALCreateGenImgProjTransformer2Fn  = (GDAL_GDALCREATEGENIMGPROJTRANSFORMER2FN) GetProcAddress( m_hInst,  "GDALCreateGenImgProjTransformer2");
   m_GDALCreateGenImgProjTransformer3Fn  = (GDAL_GDALCREATEGENIMGPROJTRANSFORMER3FN) GetProcAddress( m_hInst,  "GDALCreateGenImgProjTransformer3");
   m_GDALDestroyGenImgProjTransformerFn  = (GDAL_GDALDESTROYGENIMGPROJTRANSFORMERFN) GetProcAddress( m_hInst,  "GDALDestroyGenImgProjTransformer");
   m_AllRegisterFn   = (GDAL_ALLREGISTERFN) GetProcAddress( m_hInst, "_GDALAllRegister@0"); 
   m_OpenFn          = (GDAL_OPENFN) GetProcAddress( m_hInst, "_GDALOpen@8"); 
   m_CreateFn        = (GDAL_CREATEFN)      GetProcAddress( m_hInst, "_GDALCreate@28"); 
   m_CPLErrorFn      = (GDAL_CPLERRORFN) GetProcAddress( m_hInst, "CPLError");
   m_GetRasterBandFn = (GDAL_GDALGETRASTERBANDFN) GetProcAddress( m_hInst, "_GDALGetRasterBand@8");
   m_GetRasterBandXSizeFn = (GDAL_GDALGETRASTERBANDXSIZEFN) GetProcAddress(m_hInst, "_GDALGetRasterBandXSize@4");
   m_GetRasterBandYSizeFn = (GDAL_GDALGETRASTERBANDYSIZEFN) GetProcAddress(m_hInst, "_GDALGetRasterBandYSize@4");
   m_RasterIOFn = (GDAL_GDALRASTERIOFN) GetProcAddress(m_hInst, "_GDALRasterIO@48");
   m_GetRasterNoDataValueFn = (GDAL_GDALGETRASTERNODATAVALUEFN) GetProcAddress(m_hInst, "_GDALGetRasterNoDataValue@8");
   m_SetRasterNoDataValueFn = (GDAL_GDALSETRASTERNODATAVALUEFN) GetProcAddress(m_hInst, "_GDALSetRasterNoDataValue@12");
   m_CPLMallocFn = (GDAL_CPLMALLOCFN) GetProcAddress(m_hInst, "CPLMalloc");
   m_CloseFn = (GDAL_GDALCLOSEFN) GetProcAddress(m_hInst, "_GDALClose@4");
   m_GetGeoTransformFn = (GDAL_GDALGETGEOTRANSFORMFN) GetProcAddress(m_hInst, "_GDALGetGeoTransform@8");
   m_ApplyGeoTransformFn = (GDAL_GDALAPPLYGEOTRANSFORMFN) GetProcAddress(m_hInst, "_GDALApplyGeoTransform@28");
   m_InvGeoTransfromFn = (GDAL_GDALINVGEOTRANSFORMFN) GetProcAddress(m_hInst, "_GDALInvGeoTransform@8");
   m_GetRasterCountFn = (GDAL_GDALGETRASTERCOUNTFN) GetProcAddress(m_hInst,"_GDALGetRasterCount@4");
   m_GetDataSetDriverFn = (GDAL_GDALGETDATASETDRIVERFN) GetProcAddress(m_hInst, "_GDALGetDatasetDriver@4");
   m_GetDriverShortNameFn = (GDAL_GDALGETDRIVERSHORTNAMEFN) GetProcAddress(m_hInst,"_GDALGetDriverShortName@4");
   m_GetDriverLongNameFn = 	(GDAL_GDALGETDRIVERLONGNAMEFN) 	GetProcAddress(m_hInst,"_GDALGetDriverLongName@4");
   m_DataSetRasterIOFn = (GDAL_GDALDATASETRASTERIOFN) GetProcAddress(m_hInst,"_GDALDatasetRasterIO@60");
   m_GetRasterDataTypeFn = (GDAL_GDALGETRASTERDATATYPEFN) GetProcAddress(m_hInst,"_GDALGetRasterDataType@4");
   m_SetProjectionFn = (GDAL_GDALSETPROJECTIONFN ) GetProcAddress(m_hInst,"_GDALSetProjection@8");           
   m_GetProjectionRefFn =  (GDAL_GDALGETPROJECTIONREFFN) GetProcAddress(m_hInst,"_GDALGetProjectionRef@4");          
   m_OSRNewSpatialReferenceFn = (GDAL_OSRNEWSPATIALREFERENCEFN) GetProcAddress(m_hInst,"_OSRNewSpatialReference@4");        
   m_OSRCloneGeogCSFn = (GDAL_OSRCLONEGEOGCSFN ) GetProcAddress(m_hInst,"_OSRCloneGeogCS@4");               
   m_OSRDestroySpatialReferenceFn = (GDAL_OSRDESTROYSPATIALREFERENCEFN ) GetProcAddress(m_hInst,"_OSRDestroySpatialReference@4");   
   m_OCTTransformFn =  (GDAL_OCTTRANSFORMFN ) GetProcAddress(m_hInst,"_OCTTransform@20");                 
   m_OCTNewCoordinateTransformationFn =  (GDAL_OCTNEWCOORDINATETRANSFORMATIONFN) GetProcAddress(m_hInst,"_OCTNewCoordinateTransformation@8");
   m_DecToDMSFn = (GDAL_GDALDECTODMSFN) GetProcAddress(m_hInst,"_GDALDecToDMS@16");                  
   m_OSRImportFromWKTFn =  (GDAL_OSRIMPORTFROMWKTFN)  GetProcAddress(m_hInst,"OSRImportFromWkt");              
   m_GetDriverByNameFn = (GDAL_GDALGETDRIVERBYNAMEFN) GetProcAddress(m_hInst,"_GDALGetDriverByName@4");
   m_SetGeoTransformFn = (GDAL_GDALSETGEOTRANSFORMFN) GetProcAddress(m_hInst,"_GDALSetGeoTransform@8");
   m_OSRSetUTMFn             =(GDAL_OSRSETUTMFN)             GetProcAddress(m_hInst,"OSRSetUTM");
   m_OSRSetWellKnownGeogCSFn =(GDAL_OSRSETWELLKNOWNGEOGCSFN) GetProcAddress(m_hInst,"OSRSetWellKnownGeogCS");
   m_OSRExportToWKTFn        =(GDAL_OSREXPORTTOWKTFN)        GetProcAddress(m_hInst," _OSRExportToWkt@8");
   m_SetMetadataItemFn       =(GDAL_GDALSETMETADATAITEMFN)   GetProcAddress(m_hInst,"_GDALSetMetadataItem@16");
   m_GetMetadataFn = (GDAL_GDALGETMETADATAFN)GetProcAddress(m_hInst, "_GDALGetMetadata@8");
   // add more as needed....
   #endif
  

   return true;
   }

GDALWarpOperationH GDALWrapper::GDALCreateWarpOperation(const GDALWarpOptions* psGWo )
{
return m_GDALCreateWarpOperationFn(psGWo);
}

void GDALWrapper::GDALDestroyWarpOperation( GDALWarpOperationH gWop )
{
return m_GDALDestroyWarpOperationFn(gWop);
}

int GDALWrapper::GDALGetRasterCount( GDALDatasetH hDstDS )
{
return m_GDALGetRasterCountFn(hDstDS);
}

int	GDALWrapper::GDALGetRasterXSize ( GDALDatasetH hDstDS)
{
return m_GDALGetRasterXSizeFn(hDstDS);
}

int	GDALWrapper::GDALGetRasterYSize	( GDALDatasetH hDstDS)
{
return m_GDALGetRasterYSizeFn (hDstDS);
}

GDALWarpOptions  *GDALWrapper::GDALCreateWarpOptions (void)
{
return m_GDALCreateWarpOptionsFn();
}

GDALDatasetH GDALWrapper::GDALCreateCopy( GDALDriverH hDriver, const char * a, GDALDatasetH hDstDS, int b, char ** c, GDALProgressFunc, void * d )
{

	return m_GDALCreateCopyFn(hDriver,a,hDstDS,b,c,NULL,d);

}

int GDALWrapper::GDALGenImgProjTransform (void *pTransformArg, int bDstToSrc, int nPointCount,double *x, double *y, double *z, int *panSuccess )
{
return m_GDALGenImgProjTransformFn(pTransformArg, bDstToSrc, nPointCount,x, y, z, panSuccess );
}

void GDALWrapper::GDALDestroyWarpOptions ( GDALWarpOptions * hDopWP)
{
return m_GDALDestroyWarpOptionsFn( hDopWP);
}

CPLErr GDALWrapper::GDALSetRasterColorTable ( GDALRasterBandH hRb, GDALColorTableH hCt)
{
return m_GDALSetRasterColorTableFn( hRb, hCt);
}

GDALColorTableH  GDALWrapper::GDALGetRasterColorTable ( GDALRasterBandH hRb )
{
return m_GDALGetRasterColorTableFn( hRb ); 
}

CPLErr GDALWrapper::GDALSuggestedWarpOutput( GDALDatasetH hSrcDS, GDALTransformerFunc pfnTransformer,void *pTransformArg,double *padfGeoTransformOut,int *pnPixels, int *pnLines )
{
return m_GDALSuggestedWarpOutputFn( hSrcDS, pfnTransformer,pTransformArg,padfGeoTransformOut,pnPixels, pnLines );
}
   
CPLErr GDALWrapper::GDALChunkAndWarpImage( GDALWarpOperationH hOperation,int nDstXOff, int nDstYOff,  int nDstXSize, int nDstYSize )
{
return m_GDALChunkAndWarpImageFn ( hOperation,nDstXOff, nDstYOff,  nDstXSize, nDstYSize );
}

void* GDALWrapper::GDALCreateGenImgProjTransformer   (GDALDatasetH hSrcDS, const char *pszSrcWKT,GDALDatasetH hDstDS, const char *pszDstWKT,int bGCPUseOK, double dfGCPErrorThreshold,int nOrder)
{
return m_GDALCreateGenImgProjTransformerFn(hSrcDS, pszSrcWKT,hDstDS, pszDstWKT,bGCPUseOK, dfGCPErrorThreshold,nOrder);
}

void* GDALWrapper::GDALCreateGenImgProjTransformer2  (GDALDatasetH hSrcDS, GDALDatasetH hDstDS,char **papszOptions )
{
return m_GDALCreateGenImgProjTransformer2Fn (hSrcDS, hDstDS,papszOptions );
}

void* GDALWrapper::GDALCreateGenImgProjTransformer3  (const char *pszSrcWKT,const double *padfSrcGeoTransform,const char *pszDstWKT,const double *padfDstGeoTransform)
{
return m_GDALCreateGenImgProjTransformer3Fn(pszSrcWKT,padfSrcGeoTransform,pszDstWKT,padfDstGeoTransform);
}

void GDALWrapper::GDALDestroyGenImgProjTransformer  ( void * pTransformerArg )
{
return m_GDALDestroyGenImgProjTransformerFn(pTransformerArg );
}

char ** GDALWrapper::GetMetadata(GDALMajorObjectH hDataSet, const char * in_charP)
{
   return(m_GetMetadataFn(hDataSet, in_charP));
}

void GDALWrapper::SetMetadataItem( GDALMajorObjectH, const char *, const char *, const char * )
{
//poVRTDS->GetRasterBand(1)->SetMetadataItem("source_0",pszFilterSourceXML,
//                                             "new_vrt_sources");


}

OGRErr  GDALWrapper::OSRSetUTMH( OGRSpatialReferenceH hSRS, int nZone, int bNorth )
{

return m_OSRSetUTMFn(hSRS,nZone,bNorth);
}

OGRErr  GDALWrapper::OSRSetWellKnownGeogCSH( OGRSpatialReferenceH hSRS, const char * pszName )
{

return m_OSRSetWellKnownGeogCSFn(hSRS,pszName);
}

OGRErr  GDALWrapper::OSRExportToWKTH( OGRSpatialReferenceH hSRS, char **ppszName )
{
return m_OSRExportToWKTFn(hSRS,ppszName);
}

GDALDriverH GDALWrapper::GetDriverByName(const char *dName)
{

return m_GetDriverByNameFn(dName);
}

bool GDALWrapper::Coord2LatLong(double xcoord,double ycoord,int xsize,int ysize,GDALDataType gdalDatatype,double *adfGeoTransform, CString prjFile, double &lat, double &lng )
{
OGRCoordinateTransformationH hTransform = NULL;

GDALDriverH hDriver = GetDriverByName("MEM");

char **papszOptions = NULL;

GDALDatasetH layerDS =  Create(  hDriver, "", xsize, ysize, 1, gdalDatatype, papszOptions);

SetGeoTransform(layerDS,adfGeoTransform);

const char *SRS_WKT = prjFile;

SetProjection(layerDS, SRS_WKT );

const char  *pszProjection = NULL;

pszProjection = SRS_WKT;


/* -------------------------------------------------------------------- */
/*      Setup projected to lat/long transform if appropriate.           */
/* -------------------------------------------------------------------- */

if( GetGeoTransform( layerDS, adfGeoTransform ) == CE_None )
        //pszProjection =  (char *) GetProjectionRef(layerDS);

    if( pszProjection != NULL && strlen(pszProjection) > 0 )
    {
        OGRSpatialReferenceH hProj, hLatLong = NULL;

        hProj = OSRNewSpatialReferenceH( pszProjection );
        if( hProj != NULL )
         hLatLong = OSRCloneGeogCSH( hProj );

        if( hLatLong != NULL )
        {
            //CPLPushErrorHandler( CPLQuietErrorHandler );
            hTransform = OCTNewcoordinateTransformationH( hProj, hLatLong );
            //CPLPopErrorHandler();
            
            OSRDestroySpatialReferenceH ( hLatLong );
        }

        if( hProj != NULL )
            OSRDestroySpatialReferenceH( hProj );
    }

	//double      dfGeoX, dfGeoY;

/* -------------------------------------------------------------------- */
/*      Transform to latlong and report.                                */
/* -------------------------------------------------------------------- */
	
	//ouble *longlat= new double[2];
	//xcoord and ycoord are modified in place, so input UTM and Lat Long show up
	if( hTransform != NULL  && OCTTransformH(hTransform,1,&xcoord,&ycoord,NULL) )//xcoord and ycoord are modifie
    {
		

	   lng = xcoord; // *(longlat+0) = xcoord;
      lat = ycoord; //*(longlat+1) = ycoord;
		 
		return true;
	}
	else
	{
	   //*(longlat+0) = NULL;
      //*(longlat+1) = NULL;
		// 
		return false;	
	}
}

CPLErr  GDALWrapper::SetGeoTransform ( GDALDatasetH hDS, double *geotrans )
{

return m_SetGeoTransformFn(hDS,geotrans);
}

CPLErr GDALWrapper::SetProjection( GDALDatasetH Dset, const char * name)
{
return m_SetProjectionFn(Dset,name);
}

const char* GDALWrapper::GetProjectionRef( GDALDatasetH Dset )
{
return m_GetProjectionRefFn(Dset);
}

OGRSpatialReferenceH GDALWrapper::OSRNewSpatialReferenceH( const char * name/* = NULL */)
{
return m_OSRNewSpatialReferenceFn(name);
}

OGRSpatialReferenceH GDALWrapper::OSRCloneGeogCSH( OGRSpatialReferenceH Sparef )
{
return m_OSRCloneGeogCSFn(Sparef);
}

void GDALWrapper::OSRDestroySpatialReferenceH( OGRSpatialReferenceH Sparef )
{
return m_OSRDestroySpatialReferenceFn(Sparef);
}

int GDALWrapper::OCTTransformH( OGRCoordinateTransformationH hCT,int nCount, double *x, double *y, double *z )
{
return m_OCTTransformFn(hCT,nCount,x,y,z);
}

OGRCoordinateTransformationH GDALWrapper::OCTNewcoordinateTransformationH( OGRSpatialReferenceH hSourceSRS,OGRSpatialReferenceH hTargetSRS )
{
return m_OCTNewCoordinateTransformationFn(hSourceSRS,hTargetSRS);
}

const char GDALWrapper::DecToDMS( double geo, const char * ll, int n)
{
return m_DecToDMSFn(geo,ll,n);
}

OGRErr GDALWrapper::OSRImportFromWKTH( OGRSpatialReferenceH sparef, char ** prjfile )
{
return m_OSRImportFromWKTFn(sparef,prjfile);
}

GDALDataType GDALWrapper::GetRasterDataType( GDALRasterBandH hband)
{
return m_GetRasterDataTypeFn(hband);
}

CPLErr  GDALWrapper::DatasetRasterIO( GDALDatasetH hDataSet, GDALRWFlag eRWFlag,
    int nDSXOff, int nDSYOff, int nDSXSize, int nDSYSize,
    void * pBuffer, int nBXSize, int nBYSize, GDALDataType eBDataType,
    int nBandCount, int *panBandCount, 
    int nPixelSpace, int nLineSpace, int nBandSpace)
{

	return m_DataSetRasterIOFn(hDataSet, eRWFlag,
								nDSXOff, nDSYOff, nDSXSize, nDSYSize,
								pBuffer, nBXSize, nBYSize, eBDataType,
								nBandCount, panBandCount, 
								nPixelSpace, nLineSpace, nBandSpace);

}

const char GDALWrapper::GetDriverLongName (GDALDriverH hDriver)
{

	return m_GetDriverLongNameFn(hDriver);

}

const char GDALWrapper::GetDriverShortName (GDALDriverH hDriver)
{

	return m_GetDriverShortNameFn(hDriver);

}

GDALDriverH GDALWrapper::GetDataSetDriver(GDALDatasetH hDataSet )
{
     
	
	 return m_GetDataSetDriverFn(hDataSet);
 
 }

int GDALWrapper::GetRasterCount(GDALDatasetH hDataSet)
	{
		return m_GetRasterCountFn(hDataSet);
	}

void GDALWrapper::ApplyGeoTransform( double *geotransform, double pixelloc, double linepos, double *geo_x, double *geo_y )
{

	return m_ApplyGeoTransformFn(geotransform,pixelloc,linepos,geo_x,geo_y);

}

CPLErr GDALWrapper::GetGeoTransform(GDALDatasetH hDataSet, double *geotransform)
	{
		return m_GetGeoTransformFn(hDataSet,geotransform);

	}

int GDALWrapper::InvGeoTransform(double *padfGeoTransformIn,double *padfInvGeoTransformOut)
{


	return m_InvGeoTransfromFn(padfGeoTransformIn,padfInvGeoTransformOut);
}

void GDALWrapper::Close(GDALDatasetH hDataSet)

	{
   if ( hDataSet != 0 )
		m_CloseFn(hDataSet);
	}

void * GDALWrapper::CPLMalloc(size_t memsize)

	{
		 return m_CPLMallocFn(memsize);
	}

CPLErr GDALWrapper::SetRasterNoDataValue(GDALRasterBandH hBand, double newnodata)
	{

	return m_SetRasterNoDataValueFn(hBand,newnodata);

	}

double GDALWrapper::GetRasterNoDataValue(GDALRasterBandH hBand, int *success)
	{
	
	return m_GetRasterNoDataValueFn(hBand, success);

	}

CPLErr GDALWrapper::RasterIO(GDALRasterBandH hBand,GDALRWFlag eRWFlag ,int nXOff,int nYOff,int nXSize, int nYSize,
		               void * pData,int nBufXSize,int nBufYSize,GDALDataType eBufType,int nPixelSpace,int nLineSpace)
	{

	return m_RasterIOFn(hBand,eRWFlag,nXOff,nYOff,nXSize, nYSize,
		               pData,nBufXSize,nBufYSize,eBufType,nPixelSpace,nLineSpace);
	}

int GDALWrapper::GetRasterBandYSize(GDALRasterBandH hBand)
	{

	return m_GetRasterBandYSizeFn(hBand);
	}

int GDALWrapper::GetRasterBandXSize(GDALRasterBandH hBand)
	{

	return m_GetRasterBandXSizeFn(hBand);
	}

GDALRasterBandH GDALWrapper::GetRasterBand(GDALDatasetH hDataSet,int nBand)
	{

	
    return m_GetRasterBandFn(hDataSet,nBand);
	}

GDALDatasetH GDALWrapper::Open(const char *filename , GDALAccess)
   {    
   
   m_AllRegisterFn();

   if ( m_AllRegisterFn == NULL )
      {
      CString msg( "GDALWrapper::Open cannot register drivers, check path to gdalplugins DLLs" );
      Report::ErrorMsg( msg );
      return false;
      }

   return m_OpenFn(filename,GA_ReadOnly);
   }

//void GDALWrapper::GDALRegister_WMS()
//	{ 
//	
//		 m_GDALRegister_WMSFn();
//		
//		
//	}

void GDALWrapper::CPLError(CPLErr eErrClass,int err_no,const char* fmt)
	{ 
	m_CPLErrorFn(eErrClass,err_no,fmt);
	}


LPCTSTR GDALWrapper::CPLGetLastErrorMsg( void )
   {
   return (LPCTSTR) m_CPLGetLastErrorMsgFn();
   }

bool GDALWrapper::Create( GDALDriverH hDriver, const char *a, int b, int c, int d, GDALDataType t, char **e, GDALDatasetH *dataset )
   {
   if ( m_CreateFn == NULL )
      return false;

   *dataset = m_CreateFn( hDriver, a, b, c, d, t, e );

   return ( *dataset != NULL ) ? true : false;
   }

GDALDatasetH GDALWrapper::Create( GDALDriverH hDriver, const char *a, int b, int c, int d, GDALDataType t, char **e )
   {
   if ( m_CreateFn == NULL )
      return NULL;

   return m_CreateFn( hDriver, a, b, c, d, t, e );
   }

double GDALWrapper::DistanceBetween(double lat1, double lon1, double lat2, double lon2,char unit)
	
{
double theta, dist; 
theta = lon1 - lon2; 
dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta)); 
dist = acos(dist); 
dist = rad2deg(dist); 
dist = dist * 60 * 1.1515; 
switch(unit) { case 'mi': break; 
			   case 'km': dist = dist * 1.609344; break;
			   case 'm': dist =dist * 1609.344; break;
			   case 'N': dist = dist * 0.8684; break; } 

return (dist);
}

double GDALWrapper::deg2rad(double deg) { return (deg * pi / 180); }

double GDALWrapper::rad2deg(double rad) { return (rad * 180 / pi); }

static Ellipsoid ellip[] = {		//converted from PeterDana website, by Eugene Reimer 2002dec
//		 eId,  Name,		   EquatorialRadius,    1/flattening;
	Ellipsoid( 0, "Airy1830",		6377563.396,	299.3249646),
	Ellipsoid( 1, "AiryModified",		6377340.189,	299.3249646),
	Ellipsoid( 2, "AustralianNational",	6378160,	298.25),
	Ellipsoid( 3, "Bessel1841Namibia",	6377483.865,	299.1528128),
	Ellipsoid( 4, "Bessel1841",		6377397.155,	299.1528128),
	Ellipsoid( 5, "Clarke1866",		6378206.4,	294.9786982),
	Ellipsoid( 6, "Clarke1880",		6378249.145,	293.465),
	Ellipsoid( 7, "EverestIndia1830",	6377276.345,	300.8017),
	Ellipsoid( 8, "EverestSabahSarawak",	6377298.556,	300.8017),
	Ellipsoid( 9, "EverestIndia1956",	6377301.243,	300.8017),
	Ellipsoid(10, "EverestMalaysia1969",	6377295.664,	300.8017),	//Dana has no datum that uses this ellipsoid!
	Ellipsoid(11, "EverestMalay_Sing",	6377304.063,	300.8017),
	Ellipsoid(12, "EverestPakistan",	6377309.613,	300.8017),
	Ellipsoid(13, "Fischer1960Modified",	6378155,	298.3),
	Ellipsoid(14, "Helmert1906",		6378200,	298.3),
	Ellipsoid(15, "Hough1960",		6378270,	297),
	Ellipsoid(16, "Indonesian1974",		6378160,	298.247),
	Ellipsoid(17, "International1924",	6378388,	297),
	Ellipsoid(18, "Krassovsky1940",		6378245,	298.3),
	Ellipsoid(19, "GRS80",			6378137,	298.257222101),
	Ellipsoid(20, "SouthAmerican1969",	6378160,	298.25),
	Ellipsoid(21, "WGS72",			6378135,	298.26),
	Ellipsoid(22, "WGS84",			6378137,	298.257223563)
};
#define	eClarke1866	5		//names for ellipsoidId's
#define	eGRS80		19
#define	eWGS72		21
#define	eWGS84		22

static Datum datum[] = {		//converted from PeterDana website, by Eugene Reimer 2002dec
//	      Id,  Name,			eId,		dX,	dY,	dZ;	//when & where this datum is applicable
	Datum( 0, "NAD27_AK",			eClarke1866,	-5,	135,	172),	//NAD27 for Alaska Excluding Aleutians
	Datum( 1, "NAD27_AK_AleutiansE",	eClarke1866,	-2,	152,	149),	//NAD27 for Aleutians East of 180W
	Datum( 2, "NAD27_AK_AleutiansW",	eClarke1866,	2,	204,	105),	//NAD27 for Aleutians West of 180W
	Datum( 3, "NAD27_Bahamas",		eClarke1866,	-4,	154,	178),	//NAD27 for Bahamas Except SanSalvadorIsland
	Datum( 4, "NAD27_Bahamas_SanSalv",	eClarke1866,	1,	140,	165),	//NAD27 for Bahamas SanSalvadorIsland
	Datum( 5, "NAD27_AB_BC",		eClarke1866,	-7,	162,	188),	//NAD27 for Canada Alberta BritishColumbia
	Datum( 6, "NAD27_MB_ON",		eClarke1866,	-9,	157,	184),	//NAD27 for Canada Manitoba Ontario
	Datum( 7, "NAD27_NB_NL_NS_QC",		eClarke1866,	-22,	160,	190),	//NAD27 for Canada NewBrunswick Newfoundland NovaScotia Quebec
	Datum( 8, "NAD27_NT_SK",		eClarke1866,	4,	159,	188),	//NAD27 for Canada NorthwestTerritories Saskatchewan
	Datum( 9, "NAD27_YT",			eClarke1866,	-7,	139,	181),	//NAD27 for Canada Yukon
	Datum(10, "NAD27_CanalZone",		eClarke1866,	0,	125,	201),	//NAD27 for CanalZone (ER: is that Panama??)
	Datum(11, "NAD27_Cuba",			eClarke1866,	-9,	152,	178),	//NAD27 for Cuba
	Datum(12, "NAD27_Greenland",		eClarke1866,	11,	114,	195),	//NAD27 for Greenland (HayesPeninsula)
	Datum(13, "NAD27_Carribean",		eClarke1866,	-3,	142,	183),	//NAD27 for Antigua Barbados Barbuda Caicos Cuba DominicanRep GrandCayman Jamaica Turks
	Datum(14, "NAD27_CtrlAmerica",		eClarke1866,	0,	125,	194),	//NAD27 for Belize CostaRica ElSalvador Guatemala Honduras Nicaragua
	Datum(15, "NAD27_Canada",		eClarke1866,	-10,	158,	187),	//NAD27 for Canada
	Datum(16, "NAD27_ConUS",		eClarke1866,	-8,	160,	176),	//NAD27 for CONUS
	Datum(17, "NAD27_ConUS_East",		eClarke1866,	-9,	161,	179),	//NAD27 for CONUS East of Mississippi Including Louisiana Missouri Minnesota
	Datum(18, "NAD27_ConUS_West",		eClarke1866,	-8,	159,	175),	//NAD27 for CONUS West of Mississippi Excluding Louisiana Missouri Minnesota
	Datum(19, "NAD27_Mexico",		eClarke1866,	-12,	130,	190),	//NAD27 for Mexico
	Datum(20, "NAD83_AK",			eGRS80,		0,	0,	0),	//NAD83 for Alaska Excluding Aleutians
	Datum(21, "NAD83_AK_Aleutians",		eGRS80,		-2,	0,	4),	//NAD83 for Aleutians
	Datum(22, "NAD83_Canada",		eGRS80,		0,	0,	0),	//NAD83 for Canada
	Datum(23, "NAD83_ConUS",		eGRS80,		0,	0,	0),	//NAD83 for CONUS
	Datum(24, "NAD83_Hawaii",		eGRS80,		1,	1,	-1),	//NAD83 for Hawaii
	Datum(25, "NAD83_Mexico_CtrlAmerica",	eGRS80,		0,	0,	0),	//NAD83 for Mexico CentralAmerica
	Datum(26, "WGS72",			eWGS72,		0,	0,	0),	//WGS72 for world
	Datum(27, "WGS84",			eWGS84,		0,	0,	0)	//WGS84 for world
};


void GDALWrapper::LLtoUTM(int eId, double Lat, double Long,  double& Northing, double& Easting, int& Zone){
   // converts LatLong to UTM coords;  3/22/95: by ChuckGantz chuck.gantz@globalstar.com, from USGS Bulletin 1532.
   // Lat and Long are in degrees;  North latitudes and East Longitudes are positive.
   double a = ellip[eId].EquatorialRadius;
   double ee= ellip[eId].eccSquared;
   Long -= int((Long+180)/360)*360;			//ensure longitude within -180.00..179.9
   double N, T, C, A, M;
   double LatRad = deg2rad(Lat);
   double LongRad = deg2rad(Long);

   Zone = int((Long + 186)/6);
   if( Lat >= 56.0 && Lat < 64.0 && Long >= 3.0 && Long < 12.0 )  Zone = 32;
   if( Lat >= 72.0 && Lat < 84.0 ){			//Special zones for Svalbard
      if(      Long >= 0.0  && Long <  9.0 )  Zone = 31;
      else if( Long >= 9.0  && Long < 21.0 )  Zone = 33;
      else if( Long >= 21.0 && Long < 33.0 )  Zone = 35;
      else if( Long >= 33.0 && Long < 42.0 )  Zone = 37;
   }
   double LongOrigin = Zone*6 - 183;			//origin in middle of zone
   double LongOriginRad =  deg2rad(LongOrigin);

   double EE = ee/(1-ee);

   N = a/sqrt(1-ee*sin(LatRad)*sin(LatRad));
   T = tan(LatRad)*tan(LatRad);
   C = EE*cos(LatRad)*cos(LatRad);
   A = cos(LatRad)*(LongRad-LongOriginRad);

   M= a*((1 - ee/4    - 3*ee*ee/64 - 5*ee*ee*ee/256  ) *LatRad 
	    - (3*ee/8 + 3*ee*ee/32 + 45*ee*ee*ee/1024) *sin(2*LatRad)
	    + (15*ee*ee/256 + 45*ee*ee*ee/1024	  ) *sin(4*LatRad)
	    - (35*ee*ee*ee/3072			  ) *sin(6*LatRad));
   
   Easting = k0*N*(A+(1-T+C)*A*A*A/6+(5-18*T+T*T+72*C-58*EE)*A*A*A*A*A/120) + 500000.0;

   Northing = k0*(M+N*tan(LatRad)*(A*A/2+(5-T+9*C+4*C*C)*A*A*A*A/24
			    + (61-58*T+T*T+600*C-330*EE)*A*A*A*A*A*A/720));
}

void GDALWrapper::UTMtoLL(int eId, double Northing, double Easting, int Zone, double& Lat, double& Long){
	// converts UTM coords to LatLong;  3/22/95: by ChuckGantz chuck.gantz@globalstar.com, from USGS Bulletin 1532.
	// Lat and Long are in degrees;  North latitudes and East Longitudes are positive.
	double a = ellip[eId].EquatorialRadius;
	double ee = ellip[eId].eccSquared;
	double EE = ee / (1 - ee);
	double e1 = (1 - sqrt(1 - ee)) / (1 + sqrt(1 - ee));
	double N1, T1, C1, R1, D, M, mu, phi1Rad;
	double x = Easting - 500000.0;			//remove 500,000 meter offset for longitude
	double y = Northing;
	double LongOrigin = Zone * 6 - 183;			//origin in middle of zone

	M = y / k0;
	mu = M / (a*(1 - ee / 4 - 3 * ee*ee / 64 - 5 * ee*ee*ee / 256));

	phi1Rad = mu + (3 * e1 / 2 - 27 * e1*e1*e1 / 32) *sin(2 * mu)
		+ (21 * e1*e1 / 16 - 55 * e1*e1*e1*e1 / 32) *sin(4 * mu)
		+ (151 * e1*e1*e1 / 96) *sin(6 * mu);
	N1 = a / sqrt(1 - ee*sin(phi1Rad)*sin(phi1Rad));
	T1 = tan(phi1Rad)*tan(phi1Rad);
	C1 = EE*cos(phi1Rad)*cos(phi1Rad);
	R1 = a*(1 - ee) / pow(1 - ee*sin(phi1Rad)*sin(phi1Rad), 1.5);
	D = x / (N1*k0);

	Lat = phi1Rad - (N1*tan(phi1Rad) / R1)*(D*D / 2 - (5 + 3 * T1 + 10 * C1 - 4 * C1*C1 - 9 * EE)*D*D*D*D / 24
		+ (61 + 90 * T1 + 298 * C1 + 45 * T1*T1 - 252 * EE - 3 * C1*C1)*D*D*D*D*D*D / 720);
	Lat *= 180.0 / pi;
	Long = (D - (1 + 2 * T1 + C1)*D*D*D / 6 + (5 - 2 * C1 + 28 * T1 - 3 * C1*C1 + 8 * EE + 24 * T1*T1)*D*D*D*D*D / 120) / cos(phi1Rad);
	Long = LongOrigin + Long*180.0 / pi;
}






