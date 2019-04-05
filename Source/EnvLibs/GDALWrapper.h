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
#include "EnvLibs.h"

#ifndef NO_MFC
#include "../../GDAL/release-1600-x64/include/cpl_minixml.h"
#include "../../GDAL/release-1600-x64/include/ogr_api.h"
//#include "../GDAL/release-1600-x64/include/ogr_spatialref.h" 
#include "../../GDAL/release-1600-x64/include/gdal.h"
#include "../../GDAL/release-1600-x64/include/ogr_srs_api.h"
#include "../../GDAL/release-1600-x64/include/gdal_alg.h"
#include "../../GDAL/release-1600-x64/include/gdalwarper2.h"
#else

#include <gdal/cpl_minixml.h>
#include <gdal/ogr_api.h>
#include <gdal/gdal.h>
#include <gdal/ogr_srs_api.h>
#include <gdal/gdal_alg.h>
#include <gdal/gdalwarper.h>

#endif
#include <cmath>			//2010-08-11: was <math.h>	  
#include <cstdio>			//2010-08-11: was <stdio.h>	  
#include <cstdlib>			//2010-08-11: was <stdlib.h>  
#include <cstring>			//2010-08-11: was <string.h>  
#include <cctype>			//2010-08-11: was <ctype.h>  
#include <iostream>			//2010-08-11: was <iostream.h>
#include <iomanip>			//2010-08-11: was <iomanip.h>

//#include "..\GDAL\release-1600-x64\include\ogr_spatialref.h"


///////////////////////////////////////
// STEPS TO ADD A NEW FUNCTION
// 1) add a typdef for the function using the GDAL signature
// 2) Add a corresponding function pointer data member (e.g. m_AllRegisterFn)
// 3) Add a corresponding member function (e.g. AllRegister)
// 4) Initialize the function ptr to NULL in the constructor
// 5) Add a call to AfxGetProcAddress() in GDAL::Init();
// 6) Provide an implementation for the member function (e.g. see GDAL::AllRegister)
//
//  To use the class:
/*
    for reference of function in gdal.h http://www.gdal.org/gdal_8h.html#f26fead53c02f8035150cc710c156752
	
	//instantiate object from GDALWrapper class
	GDALWrapper gdal; 
  
    //Path to GDAL Dlls. There should be a subdirectory called "gdalplugins" with the DLLs for different
	//GDAL plugins or formats (ie. netCDF). Only need to call this once.  Drivers for all formats are already loaded
	bool okpath = gdal.Init( "\\Envision\\src\\x64\\Debug" );   
  
	if ( ! okpath )
     {
     //errormsg...
     //return;
      }
	
	/*  //Can open many other file formats, below is example for netCDF.  If file has only
	//one image file there is no need to specify a subdataset, just the file name.
	
	//Open the NETCDF subdataset NETCDF:filename:subdataset         
	const char *pszFilename = "NETCDF:\\gridded_obs_daily_Tavg_1999.nc:Tavg";
	
	//loads all the drivers for different GDAL "formats" or datafile types, and opens the file for
	//reading or writing to that file
	GDALDatasetH jdataset = gdal.Open(pszFilename,GA_ReadOnly);
	
	//This will get the first band of the Raster, first band is indexed with 1.
	GDALRasterBandH hBand = gdal.GetRasterBand(jdataset,1);

	//retrieve what the no data value is
	double nodatavalue = gdal.GetRasterNoDataValue(hBand,NULL);
	
	//return the x size of the raster
	int nXSize = gdal.GetRasterBandXSize(hBand);

	//return the y size of the raster
	int nYSize = gdal.GetRasterBandYSize(hBand);

	//allocate memory for full 2d band
	float *pafScanband =  new float[ nXSize*nYSize ];
	
	//see gdal reference for options here, pafScanband is the full 2d band in memory
	//note: pafScanBand is organized in memory as reading the 2d band from left to right, and bottom to top.
	gdal.RasterIO(hBand,GF_Read,0,0,nXSize,nYSize,pafScanband,nXSize,nYSize,GDT_Float32,0,0);

	// free memory
	delete [] pafScanband;

	// close file
	gdal.Close(jdataset);
*/

// create a typedef for each GDAL function to call
typedef GDALDatasetH        (CPL_STDCALL *GDAL_GDALCREATECOPYFN)				( GDALDriverH, const char *, GDALDatasetH, int, char **, GDALProgressFunc, void * );
typedef GDALWarpOperationH  (CPL_STDCALL *GDAL_GDALCREATEWARPOPERATIONFN)		(const GDALWarpOptions* );
typedef void                (CPL_STDCALL *GDAL_GDALDESTROYWARPOPERATIONFN)		( GDALWarpOperationH );

typedef int					(CPL_STDCALL *GDAL_GDALGETRASTERCOUNTFN)		( GDALDatasetH );
typedef int					(CPL_STDCALL *GDAL_GDALGETRASTERXSIZEFN)      	( GDALDatasetH );
typedef int					(CPL_STDCALL *GDAL_GDALGETRASTERYSIZEFN)      	( GDALDatasetH );
typedef GDALWarpOptions  *	(CPL_STDCALL *GDAL_GDALCREATEWARPOPTIONSFN)   	(void);
typedef int					(CPL_STDCALL *GDAL_GDALTERMPROGRESSFN)        	( double, const char *, void *);
//typedef int                 (CPL_STDCALL *GDAL_GDALGENIMPPROJTRANSFORMFN) 	(void *, int, int ,double *, double *, double *, int * ); //put stdcall in
typedef void				(CPL_STDCALL *GDAL_GDALDESTROYWARPOPTIONSFN)  	( GDALWarpOptions * );

typedef CPLErr				(CPL_STDCALL *GDAL_GDALSETRASTERCOLORTABLEFN)	( GDALRasterBandH, GDALColorTableH );
typedef GDALColorTableH		(CPL_STDCALL *GDAL_GDALGETRASTERCOLORTABLEFN)	( GDALRasterBandH );
typedef CPLErr				(CPL_STDCALL *GDAL_GDALSUGGESTEDWARPOUTPUTFN)	( GDALDatasetH hSrcDS, GDALTransformerFunc pfnTransformer,void *pTransformArg,double *padfGeoTransformOut, int *pnPixels, int *pnLines );

typedef CPLErr				(CPL_STDCALL *GDAL_GDALCHUNKANDWARPIMAGEFN)		( GDALWarpOperationH hOperation,int nDstXOff, int nDstYOff,  int nDstXSize, int nDstYSize );
typedef void*				(CPL_STDCALL *GDAL_GDALCREATEGENIMGPROJTRANSFORMERFN)	(GDALDatasetH hSrcDS, const char *,GDALDatasetH hDstDS, const char *,int, double,int);
typedef void*				(CPL_STDCALL *GDAL_GDALCREATEGENIMGPROJTRANSFORMER2FN)	(GDALDatasetH hSrcDS, GDALDatasetH hDstDS,char ** );
typedef void*				(CPL_STDCALL *GDAL_GDALCREATEGENIMGPROJTRANSFORMER3FN)	(const char *,const double *,const char *,const double *);
typedef void				(CPL_STDCALL *GDAL_GDALDESTROYGENIMGPROJTRANSFORMERFN)	( void * );

typedef void (CPL_STDCALL *GDAL_ALLREGISTERFN)( void );
typedef void (CPL_STDCALL *GDAL_CPLERRORFN)( CPLErr eErrClass,int err_no,const char* fmt );
typedef GDALDatasetH (CPL_STDCALL *GDAL_OPENFN)( const char *,int );
typedef GDALDatasetH (CPL_STDCALL *GDAL_CREATEFN)( GDALDriverH hDriver, const char *, int, int, int, GDALDataType,char ** );
typedef GDALRasterBandH (CPL_STDCALL *GDAL_GDALGETRASTERBANDFN)(GDALDatasetH hDataSet,int nBand); 
typedef int (CPL_STDCALL *GDAL_GDALGETRASTERBANDXSIZEFN)(GDALRasterBandH hBand);
typedef int (CPL_STDCALL *GDAL_GDALGETRASTERBANDYSIZEFN)(GDALRasterBandH hBand);
typedef CPLErr (CPL_STDCALL *GDAL_GDALRASTERIOFN)(GDALRasterBandH hBand,GDALRWFlag eRWFlag,int nXOff,int nYOff,int nXSize, int nYSize,
		                     void * pData,int nBufXSize,int nBufYSize,GDALDataType eBufType,int nPixelSpace,int nLineSpace);

typedef CPLErr (CPL_STDCALL *GDAL_GDALDATASETRASTERIOFN)( 
    GDALDatasetH hDS, GDALRWFlag eRWFlag,
    int nDSXOff, int nDSYOff, int nDSXSize, int nDSYSize,
    void * pBuffer, int nBXSize, int nBYSize, GDALDataType eBDataType,
    int nBandCount, int *panBandCount, 
    int nPixelSpace, int nLineSpace, int nBandSpace);

typedef double (CPL_STDCALL *GDAL_GDALGETRASTERNODATAVALUEFN)( GDALRasterBandH hBand, int * );
typedef CPLErr (CPL_STDCALL *GDAL_GDALSETRASTERNODATAVALUEFN)( GDALRasterBandH, double );
typedef void *(CPL_STDCALL *GDAL_CPLMALLOCFN)( size_t );
typedef void (CPL_STDCALL  *GDAL_GDALCLOSEFN) ( GDALDatasetH );
typedef CPLErr (CPL_STDCALL *GDAL_GDALGETGEOTRANSFORMFN)( GDALDatasetH, double * );
typedef int (CPL_STDCALL *GDAL_GDALGETRASTERCOUNTFN)( GDALDatasetH );
typedef GDALDriverH (CPL_STDCALL *GDAL_GDALGETDATASETDRIVERFN)( GDALDatasetH );
typedef const char (CPL_STDCALL *GDAL_GDALGETDRIVERSHORTNAMEFN)( GDALDriverH );
typedef const char (CPL_STDCALL *GDAL_GDALGETDRIVERLONGNAMEFN)( GDALDriverH );
typedef GDALDataType (CPL_STDCALL *GDAL_GDALGETRASTERDATATYPEFN)( GDALRasterBandH );
typedef void (CPL_STDCALL *GDAL_GDALAPPLYGEOTRANSFORMFN)( double *, double, double, double *, double * );
typedef int  (CPL_STDCALL *GDAL_GDALINVGEOTRANSFORMFN)( double *padfGeoTransformIn,double *padfInvGeoTransformOut );

typedef CPLErr (CPL_STDCALL *GDAL_GDALSETPROJECTIONFN)( GDALDatasetH, const char * );
typedef const char* (CPL_STDCALL *GDAL_GDALGETPROJECTIONREFFN)( GDALDatasetH ); //might need extra *
typedef OGRSpatialReferenceH (CPL_STDCALL *GDAL_OSRNEWSPATIALREFERENCEFN)( const char * /* = NULL */);
typedef OGRSpatialReferenceH (CPL_STDCALL *GDAL_OSRCLONEGEOGCSFN)( OGRSpatialReferenceH );
typedef void (CPL_STDCALL *GDAL_OSRDESTROYSPATIALREFERENCEFN)( OGRSpatialReferenceH );
typedef int (CPL_STDCALL *GDAL_OCTTRANSFORMFN)( OGRCoordinateTransformationH hCT,int nCount, double *x, double *y, double *z );
typedef OGRCoordinateTransformationH (CPL_STDCALL *GDAL_OCTNEWCOORDINATETRANSFORMATIONFN)( OGRSpatialReferenceH hSourceSRS,OGRSpatialReferenceH hTargetSRS );
typedef const char (CPL_STDCALL *GDAL_GDALDECTODMSFN)( double, const char *, int );
typedef OGRErr (*GDAL_OSRIMPORTFROMWKTFN)( OGRSpatialReferenceH, char ** );
typedef GDALDriverH (CPL_STDCALL *GDAL_GDALGETDRIVERBYNAMEFN)( const char * );
typedef CPLErr (CPL_STDCALL *GDAL_GDALSETGEOTRANSFORMFN)( GDALDatasetH, double * );

typedef OGRErr ( *GDAL_OSRSETUTMFN)( OGRSpatialReferenceH hSRS, int nZone, int bNorth );
typedef OGRErr ( *GDAL_OSRSETWELLKNOWNGEOGCSFN)( OGRSpatialReferenceH hSRS, const char * pszName );
typedef OGRErr ( CPL_STDCALL *GDAL_OSREXPORTTOWKTFN)( OGRSpatialReferenceH, char ** );
typedef void (*GDAL_GDALSETMETADATAITEMFN)( GDALMajorObjectH, const char *, const char *, const char * );

typedef const char*( CPL_STDCALL *GDAL_CPLGETLASTERRORMSG)(void);


//typedef OGRErr CPL_DLL OSRImportFromWkt( OGRSpatialReferenceH, char ** );

/// add more here as needed

// Tim's additions
typedef char **(CPL_STDCALL *GDAL_GDALGETMETADATAFN)(GDALMajorObjectH, const char *);

class LIBSAPI GDALWrapper
{
public:
	GDALWrapper(void );
	~GDALWrapper( void );

   bool Init( void );

   // GDAL functions go here.
    GDALWarpOperationH	   GDALCreateWarpOperation	(const GDALWarpOptions* );
	void				   GDALDestroyWarpOperation	( GDALWarpOperationH );
	int					   GDALGetRasterCount		( GDALDatasetH );
	int					   GDALGetRasterXSize		( GDALDatasetH );
	int					   GDALGetRasterYSize		( GDALDatasetH );
	GDALWarpOptions  *	   GDALCreateWarpOptions	(void);
	int					   GDALTermProgress			( double, const char *, void *);
	int                    GDALGenImgProjTransform	(void *, int, int,double *, double *, double *, int * );
	void				   GDALDestroyWarpOptions	( GDALWarpOptions * );

	GDALTransformerFunc	   m_GDALGenImgProjTransformFn;  

   CPLErr           GDALSetRasterColorTable( GDALRasterBandH, GDALColorTableH );
   GDALColorTableH  GDALGetRasterColorTable( GDALRasterBandH );
   CPLErr           GDALSuggestedWarpOutput( GDALDatasetH,GDALTransformerFunc,void *,double *,int *, int * );
   
   GDALDatasetH     GDALCreateCopy( GDALDriverH, const char *, GDALDatasetH, int, char **, GDALProgressFunc, void * );

   CPLErr GDALChunkAndWarpImage( GDALWarpOperationH hOperation,int, int,  int, int);
   void* GDALCreateGenImgProjTransformer   (GDALDatasetH hSrcDS, const char *,GDALDatasetH hDstDS, const char *,int, double,int);
   void* GDALCreateGenImgProjTransformer2  (GDALDatasetH hSrcDS, GDALDatasetH hDstDS,char ** );
   void* GDALCreateGenImgProjTransformer3  (const char *pszSrcWKT,const double *,const char *,const double *);
   void GDALDestroyGenImgProjTransformer  ( void * );

   void SetMetadataItem( GDALMajorObjectH, const char *, const char *, const char * );
   GDALDatasetH Open(const char *filename , GDALAccess);
   void CPLError(CPLErr eErrClass,int err_no,const char* fmt);
   LPCTSTR CPLGetLastErrorMsg(void);
   GDALRasterBandH GetRasterBand(GDALDatasetH hDataSet,int nBand);
   int GetRasterBandXSize(GDALRasterBandH hBand);
   int GetRasterBandYSize(GDALRasterBandH hBand);
   CPLErr RasterIO(GDALRasterBandH hBand,GDALRWFlag eRWFlag,int nXOff,int nYOff,int nXSize, int nYSize,
		               void * pData,int nBufXSize,int nBufYSize,GDALDataType eBufType,int nPixelSpace,int nLineSpace);
   
   CPLErr DatasetRasterIO( 
    GDALDatasetH hDS, GDALRWFlag eRWFlag,
    int nDSXOff, int nDSYOff, int nDSXSize, int nDSYSize,
    void * pBuffer, int nBXSize, int nBYSize, GDALDataType eBDataType,
    int nBandCount, int *panBandCount, 
    int nPixelSpace, int nLineSpace, int nBandSpace);
   
   double GetRasterNoDataValue(GDALRasterBandH hBand, int *);
   CPLErr SetRasterNoDataValue(GDALRasterBandH hBand, double );
   void * CPLMalloc( size_t );
   void Close(GDALDatasetH);
   CPLErr GetGeoTransform(GDALDatasetH, double * geotransform);
   int GetRasterCount(GDALDatasetH);
   GDALDriverH GetDataSetDriver(GDALDatasetH);
   const char GetDriverShortName( GDALDriverH );
   const char GetDriverLongName( GDALDriverH );
   GDALDriverH GetDriverByName(const char *);
   void ApplyGeoTransform( double *, double, double, double *, double * );
   int InvGeoTransform(double *padfGeoTransformIn,double *padfInvGeoTransformOut);  
   double DistanceBetween(double lat1, double lng1, double lat2, double lng2,char unit);
   double deg2rad(double deg);
   double rad2deg(double rad);
   bool   Coord2LatLong(double xcoord,double ycoord,int xSize,int ySize,GDALDataType gdalDatatype,double *coordtransform,CString prjFile, double &lat, double &lng );

   // note two variations below.  I'd suggest the former, but either or both should work
   bool Create( GDALDriverH hDriver, const char *, int, int, int, GDALDataType, char **, GDALDatasetH *result ); // note - GDAL return value is passed by reference in arg list
   GDALDatasetH Create( GDALDriverH hDriver, const char *, int, int, int, GDALDataType, char ** );
   GDALDataType GetRasterDataType( GDALRasterBandH hBand );

   CPLErr SetProjection( GDALDatasetH, const char * );
  const char* GetProjectionRef( GDALDatasetH );
  OGRSpatialReferenceH OSRNewSpatialReferenceH( const char * /* = NULL */);
  OGRSpatialReferenceH OSRCloneGeogCSH( OGRSpatialReferenceH );
  void OSRDestroySpatialReferenceH( OGRSpatialReferenceH );
  int OCTTransformH( OGRCoordinateTransformationH hCT,int nCount, double *x, double *y, double *z );
  OGRCoordinateTransformationH OCTNewcoordinateTransformationH( OGRSpatialReferenceH hSourceSRS,OGRSpatialReferenceH hTargetSRS );
  const char DecToDMS( double, const char *, int );
  OGRErr OSRImportFromWKTH( OGRSpatialReferenceH, char ** );
  CPLErr SetGeoTransform ( GDALDatasetH, double * );

  // Tim's additions
  char **GetMetadata(GDALMajorObjectH hDataSet, const char *);

OGRErr OSRSetUTMH( OGRSpatialReferenceH hSRS, int nZone, int bNorth );
OGRErr OSRSetWellKnownGeogCSH( OGRSpatialReferenceH hSRS, const char * pszName );
OGRErr OSRExportToWKTH( OGRSpatialReferenceH, char ** );
  
void LLtoUTM(int eId, double Lat, double Long,  double& Northing, double& Easting, int& Zone);
void UTMtoLL(int eId, double Northing, double Easting, int Zone, double& Lat, double& Long);

protected:
   CString   m_binPath;
   HINSTANCE  m_hInst;

   // function pointers
   GDAL_ALLREGISTERFN						m_AllRegisterFn;
   GDAL_OPENFN								m_OpenFn;
   GDAL_CREATEFN							m_CreateFn;
   GDAL_CPLERRORFN							m_CPLErrorFn;
   GDAL_GDALGETRASTERBANDFN					m_GetRasterBandFn;
   GDAL_GDALGETRASTERBANDXSIZEFN			m_GetRasterBandXSizeFn;
   GDAL_GDALGETRASTERBANDYSIZEFN			m_GetRasterBandYSizeFn;
   GDAL_GDALRASTERIOFN						m_RasterIOFn;
   GDAL_GDALGETRASTERNODATAVALUEFN			m_GetRasterNoDataValueFn;
   GDAL_GDALSETRASTERNODATAVALUEFN			m_SetRasterNoDataValueFn;
   GDAL_CPLMALLOCFN							m_CPLMallocFn;
   GDAL_GDALCLOSEFN							m_CloseFn;
   GDAL_GDALGETGEOTRANSFORMFN				m_GetGeoTransformFn;
   GDAL_GDALGETRASTERCOUNTFN				m_GetRasterCountFn;
   GDAL_GDALGETDATASETDRIVERFN				m_GetDataSetDriverFn;
   GDAL_GDALGETDRIVERSHORTNAMEFN			m_GetDriverShortNameFn;
   GDAL_GDALGETDRIVERLONGNAMEFN				m_GetDriverLongNameFn;
   GDAL_GDALDATASETRASTERIOFN				m_DataSetRasterIOFn;
   GDAL_GDALGETRASTERDATATYPEFN				m_GetRasterDataTypeFn;
   GDAL_GDALAPPLYGEOTRANSFORMFN				m_ApplyGeoTransformFn;
   GDAL_GDALINVGEOTRANSFORMFN				m_InvGeoTransfromFn;
   
   GDAL_GDALSETPROJECTIONFN					m_SetProjectionFn;
   GDAL_GDALGETPROJECTIONREFFN				m_GetProjectionRefFn;
   GDAL_OSRNEWSPATIALREFERENCEFN			m_OSRNewSpatialReferenceFn;
   GDAL_OSRCLONEGEOGCSFN					m_OSRCloneGeogCSFn;
   GDAL_OSRDESTROYSPATIALREFERENCEFN		m_OSRDestroySpatialReferenceFn;
   GDAL_OCTTRANSFORMFN						m_OCTTransformFn;
   GDAL_OCTNEWCOORDINATETRANSFORMATIONFN	m_OCTNewCoordinateTransformationFn;
   GDAL_GDALDECTODMSFN						m_DecToDMSFn;
   GDAL_OSRIMPORTFROMWKTFN					m_OSRImportFromWKTFn;
   GDAL_GDALGETDRIVERBYNAMEFN				m_GetDriverByNameFn;
   GDAL_GDALSETGEOTRANSFORMFN				m_SetGeoTransformFn;
   GDAL_OSRSETUTMFN							m_OSRSetUTMFn;
   GDAL_OSRSETWELLKNOWNGEOGCSFN				m_OSRSetWellKnownGeogCSFn;
   GDAL_OSREXPORTTOWKTFN					m_OSRExportToWKTFn;
   GDAL_GDALSETMETADATAITEMFN				m_SetMetadataItemFn;

   GDAL_GDALCREATEGENIMGPROJTRANSFORMERFN   m_GDALCreateGenImgProjTransformerFn;
   GDAL_GDALCREATEGENIMGPROJTRANSFORMER2FN  m_GDALCreateGenImgProjTransformer2Fn;
   GDAL_GDALCREATEGENIMGPROJTRANSFORMER3FN	m_GDALCreateGenImgProjTransformer3Fn;
   GDAL_GDALDESTROYGENIMGPROJTRANSFORMERFN	m_GDALDestroyGenImgProjTransformerFn;
   GDAL_GDALCHUNKANDWARPIMAGEFN				m_GDALChunkAndWarpImageFn;

   GDAL_GDALSETRASTERCOLORTABLEFN			m_GDALSetRasterColorTableFn; 
   GDAL_GDALGETRASTERCOLORTABLEFN			m_GDALGetRasterColorTableFn;
   GDAL_GDALSUGGESTEDWARPOUTPUTFN			m_GDALSuggestedWarpOutputFn;

   GDAL_GDALGETRASTERXSIZEFN				m_GDALGetRasterXSizeFn;		
   GDAL_GDALGETRASTERYSIZEFN				m_GDALGetRasterYSizeFn;			
   GDAL_GDALCREATEWARPOPTIONSFN				m_GDALCreateWarpOptionsFn;	
   GDAL_GDALTERMPROGRESSFN					m_GDALTermProgressFn;			
   	
   GDAL_GDALDESTROYWARPOPTIONSFN			m_GDALDestroyWarpOptionsFn;
   GDAL_GDALCREATEWARPOPERATIONFN			m_GDALCreateWarpOperationFn;	
   GDAL_GDALDESTROYWARPOPERATIONFN			m_GDALDestroyWarpOperationFn;
   GDAL_GDALGETRASTERCOUNTFN                m_GDALGetRasterCountFn;
   GDAL_GDALCREATECOPYFN                    m_GDALCreateCopyFn;
   // Tim's Addition
   GDAL_GDALGETMETADATAFN m_GetMetadataFn;

   GDAL_CPLGETLASTERRORMSG m_CPLGetLastErrorMsgFn;

};

class Ellipsoid{
public:
	Ellipsoid(){};
	Ellipsoid(int id, const char* name, double radius, double fr){
		Name=(char*) name;  EquatorialRadius=radius;  eccSquared=2/fr-1/(fr*fr);
	}
	char* Name;
	double EquatorialRadius; 
	double eccSquared;
};


class Datum{
public:
	Datum(){};
	Datum(int id, const char* name, int eid, double dx, double dy, double dz){
		Name=(char*) name;  eId=eid;  dX=dx;  dY=dy;  dZ=dz;
	}
	char* Name;
	int   eId;
	double dX;
	double dY;
	double dZ;
};


