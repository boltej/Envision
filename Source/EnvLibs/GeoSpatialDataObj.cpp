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
// GeoSpatialDataObj.cpp : Defines the initialization routines for the DLL.
//

#include "EnvLibs.h"
#pragma hdrstop

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "Report.h"
#include "GeoSpatialDataObj.h"
#include <string>
#include <map>
#include <complex>
using namespace std;
#include <cstdio>
#include <new>

#define   dNAD27_MB_ON   6      //names for datumId's
#define   dNAD27_Canada   15
#define   dNAD83_Canada   22
#define   dNAD83_ConUS   23
#define   dWGS84      27

GeoSpatialDataObj::GeoSpatialDataObj(UNIT_MEASURE m)
   : DataObj(m)
   , m_datasetH(0)
   , m_hDriver(0)
   , m_hBand(0)
   , m_gdal()
   , m_DataType(GDT_Unknown)
//   , m_gridpoly_method(CONTINUOUS_VAR)
   , m_pafScanband(NULL)
   , m_CellLatDegResolution(-1)
   , m_CellLonDegResolution(-1)
   , m_UpperLeftLat(-1)
   , m_UpperLeftLon(-1)
   //, m_geoTransform( NULL )
   //, m_invGeoTransform( NULL )
   , m_geoy(0)
   , m_geox(0)
   , m_imgNorthUp2(0)
   , m_imgNorthUp4(0)
   , m_nXSize(0)
   , m_nYSize(0)
   , m_nBuffXSize(0)
   , m_nBuffYSize(0)
   , m_CellDim(0)
   , m_nBands(0)
   , m_RasterXYZSize(0)
   , m_3DVal(0)
   , m_pafScanbandbool(NULL)
   , m_noDataValue(-99)
   , m_pMap(NULL)
   //, m_pPolyMapLayer(NULL)
   //, m_pGridMapLayer(NULL)
   //, m_pPolyGridLkUp(NULL)
   { }

GeoSpatialDataObj::~GeoSpatialDataObj(void) {
   if (m_pafScanband != NULL)
      delete m_pafScanband;
   //if (m_geoTransform != NULL)
   //   delete m_geoTransform;
   if (m_pafScanbandbool != NULL)
      delete m_pafScanbandbool;
   //if (m_pPolyGridLkUp != NULL)
   //   delete m_pPolyGridLkUp;
   }

float* GeoSpatialDataObj::NetCDF2GeoTransWarpRaster(LPCTSTR inputFile, LPCTSTR poprjWKTFile, int set3Dvalue, int &xSize, int &ySize)
   {
   GDALDatasetH  m_datasetSourceH, m_datasetDestH;
   GDALDriverH   hDriverSource, hDriverDest;
   GDALWrapper   m_gdal;
   GDALDataType  m_DataType;

   CString msg;

   const char* prjWKTFile = poprjWKTFile;

   double
      m_CellLatDegResolution,
      m_CellLonDegResolution,
      m_UpperLeftyCoord,
      m_UpperLeftxCoord,
      *m_geoTransform(NULL),
      m_imgNorthUp2,
      m_noDataValue,
      m_imgNorthUp4;

   int
      m_nXSize,
      m_nYSize,
      m_nBands;
   //m_RasterXYZSize;

   m_geoTransform = new double[6];

   //LPCTSTR m_GeoFilename  =  "\\Envision\\StudyAreas\\WW2100\\wms.xml";
   LPCTSTR m_GeoFilename = inputFile;

   m_gdal.Init();

   const TCHAR* cstr = (LPCTSTR)inputFile;

   //The GDAL Open function will recognise the wms.xml file as a Web Map Service Request.  The WMS driver
   //and mini driver will be selected.  Returned from this process is a GDAL dataset, the backbone to the
   //GDAL method.  As part of the dataset a geotransform is created.
   m_datasetSourceH = m_gdal.Open(cstr, GA_ReadOnly);

   if (m_datasetSourceH == NULL)
      {
      msg.Format(_T("GeoSpatialDataObj::failed to open inputFile for tranformation and warp"));
      Report::ErrorMsg(msg);
      return NULL;
      }

   GDALRasterBandH rasterBandsourceH = m_gdal.GetRasterBand(m_datasetSourceH, set3Dvalue);

   m_gdal.GetGeoTransform(m_datasetSourceH, m_geoTransform);

   //for debugging to look at input files georeferencing info
   m_CellLonDegResolution = *(m_geoTransform + 1);
   m_CellLatDegResolution = *(m_geoTransform + 5);
   m_UpperLeftyCoord = *(m_geoTransform + 3);
   m_UpperLeftxCoord = *(m_geoTransform + 0);
   m_imgNorthUp4 = *(m_geoTransform + 4);
   m_imgNorthUp2 = *(m_geoTransform + 2);
   m_nBands = m_gdal.GetRasterCount(m_datasetSourceH);
   hDriverSource = m_gdal.GetDataSetDriver(m_datasetSourceH);
   m_DataType = m_gdal.GetRasterDataType(rasterBandsourceH);
   m_nXSize = m_gdal.GetRasterBandXSize(rasterBandsourceH);
   m_nYSize = m_gdal.GetRasterBandYSize(rasterBandsourceH);
   m_noDataValue = m_gdal.GetRasterNoDataValue(rasterBandsourceH, NULL);

   if (set3Dvalue <= 0)
      {
      msg.Format(_T("GeoSpatialDataObj::Beginning band index is 1 based, therfore index must be greater than or equal to 0"));
      Report::ErrorMsg(msg);
      return NULL;
      }

   if (set3Dvalue > m_nBands)
      {
      msg.Format(_T("GeoSpatialDataObj::Ending band index is 1 based, therefore index must less than or equal to number of bands in the input file"));
      Report::ErrorMsg(msg);
      return NULL;
      }

   //m_RasterXYZSize = m_nXSize * m_nYSize * m_nBands;

   //unsigned char *m_pafScanband =  new unsigned char[ m_RasterXYZSize ];

   //this reads input file into a raster in memory, not really used in this operation
   //m_gdal.DatasetRasterIO( m_datasetSourceH, GF_Read, 0, 0, m_nXSize, m_nYSize, 
   //               m_pafScanband, m_nXSize, m_nYSize, m_DataType, 
   //               m_nBands, NULL, 0, 0, 0 );

   //*************** TRANSFORM *******************************************

    // Get outputFile driver (GeoTIFF format)
   hDriverDest = m_gdal.GetDriverByName("MEM");

   // Get Source WKT coordinate system. 
   const char *pszSrcWKT;
   char   **pszDstWKT = NULL;
   const char *pszDst2WKT = NULL;
   pszSrcWKT = m_gdal.GetProjectionRef(m_datasetSourceH);

   //a few things not used, but might come in handy
   double LongTemp = (m_UpperLeftxCoord + 180) - floor((m_UpperLeftxCoord + 180) / 360) * 360 - 180; // -180.00 .. 179.9;

   int utmzone = (int)floor((LongTemp + 180) / 6) + 1;

   //user supplied WKT/prj file projection info
   pszDst2WKT = prjWKTFile;

   // Create a transformer that maps from source pixel/line coordinates
    // to destination georeferenced coordinates (not destination 
    // pixel line).  We do that by omitting the destination dataset
    // handle (setting it to NULL). 
   void *hTransformArg;

   //This function creates a transformation object that maps from pixel/line coordinates on one image 
   //to pixel/line coordinates on another image. The images may potentially be georeferenced in different 
   //coordinate systems, and may used GCPs to map between their pixel/line coordinates and georeferenced 
   //coordinates (as opposed to the default assumption that their geotransform should be used).
   hTransformArg = m_gdal.GDALCreateGenImgProjTransformer(m_datasetSourceH, pszSrcWKT, NULL, pszDst2WKT,
      FALSE, 0, 1);
   ASSERT(hTransformArg != NULL);

   // Get approximate output georeferenced bounds and resolution for file. 
   double adfDstGeoTransform[6];
   int nPixels, nLines;
   CPLErr eErr;

   //This function is used to suggest the size, and georeferenced extents appropriate given the 
  //indicated transformation and input file. It walks the edges of the input file (approximately 
  //20 sample points along each edge) transforming into output coordinates in order to get an extents box.

   eErr = m_gdal.GDALSuggestedWarpOutput(m_datasetSourceH,
      m_gdal.m_GDALGenImgProjTransformFn, hTransformArg,
      adfDstGeoTransform, &nPixels, &nLines);
   ASSERT(eErr == CE_None);

   m_gdal.GDALDestroyGenImgProjTransformer(hTransformArg);

   // Create the output file.  


  /*CString bandNumber, tmp_outfile;

  tmp_outfile = outputFile;

  bandNumber.Format("%d", set3Dvalue);

  bandNumber.Insert(0,'_');

  tmp_outfile.Insert(tmp_outfile.ReverseFind('.'),bandNumber);*/



   m_datasetDestH = m_gdal.Create(hDriverDest, "tmp_outfile", nPixels, nLines,
      1, m_DataType, NULL);

   //m_datasetDestH = m_gdal.GDALCreateCopy( hDriverDest, tmp_outfile, m_datasetSourceH, FALSE, 
                         //NULL, NULL, NULL );

   ASSERT(m_datasetDestH != NULL);

   if (m_datasetDestH == NULL)
      {
      msg.Format(_T("GeoSpatialDataObj::failed to create file driver for outputFile for tranformation and warp"));
      Report::ErrorMsg(msg);
      return NULL;
      }

   // Set projection information for output file. 

   m_gdal.SetProjection(m_datasetDestH, pszDst2WKT);

   m_gdal.SetGeoTransform(m_datasetDestH, adfDstGeoTransform);

   // Copy the color table, if required.
   //GDALColorTableH hCT;

   //hCT = m_gdal.GDALGetRasterColorTable( m_gdal.GetRasterBand(m_datasetSourceH,set3Dvalue) );

   //if( hCT != NULL )
   //         m_gdal.GDALSetRasterColorTable( m_gdal.GetRasterBand(m_datasetDestH,set3Dvalue), hCT );

   //************************** WARP *******************************************************

   GDALWarpOptions *psWarpOptions = m_gdal.GDALCreateWarpOptions();

   psWarpOptions->hSrcDS = m_datasetSourceH;
   psWarpOptions->hDstDS = m_datasetDestH;
   psWarpOptions->nBandCount = 1;
   psWarpOptions->panSrcBands = (int *)m_gdal.CPLMalloc(sizeof(int) * psWarpOptions->nBandCount);
   psWarpOptions->panDstBands = (int *)m_gdal.CPLMalloc(sizeof(int) * psWarpOptions->nBandCount);

   //set warp options
   psWarpOptions->panSrcBands[0] = set3Dvalue;
   psWarpOptions->panDstBands[0] = 1;


   // Establish reprojection transformer. 
   psWarpOptions->pTransformerArg =
      m_gdal.GDALCreateGenImgProjTransformer(m_datasetSourceH,
         m_gdal.GetProjectionRef(m_datasetSourceH),
         m_datasetDestH,
         m_gdal.GetProjectionRef(m_datasetDestH),
         FALSE, 0.0, 1);

   psWarpOptions->pfnTransformer = m_gdal.m_GDALGenImgProjTransformFn;

   // Initialize and execute the warp operation. 
   GDALWarpOperationH hWoperation = m_gdal.GDALCreateWarpOperation(psWarpOptions);

   m_gdal.GDALChunkAndWarpImage(hWoperation, 0, 0,
      m_gdal.GDALGetRasterXSize(m_datasetDestH),
      m_gdal.GDALGetRasterYSize(m_datasetDestH));

   //clean a few things up
   m_gdal.GDALDestroyGenImgProjTransformer(psWarpOptions->pTransformerArg);
   m_gdal.GDALDestroyWarpOptions(psWarpOptions);

   //intialize a few things for making a raster
   int m_RasterXYZSizeDest = (nPixels) * (nLines) * 1;

   float* m_pafScanbandDest = new float[m_RasterXYZSizeDest];

   //read the image into raster in memory
   m_gdal.DatasetRasterIO(m_datasetDestH, GF_Read, 0, 0, nPixels, nLines,
      m_pafScanbandDest, nPixels, nLines, m_DataType,
      1, NULL, 0, 0, 0);

   xSize = nPixels;
   ySize = nLines;

   float dat;

   for (int i = 0; i < m_RasterXYZSizeDest; i++)
      {
      dat = *(m_pafScanbandDest + i);
      }

   //output raster to image file on disk
   //m_gdal.DatasetRasterIO( m_datasetDestH, GF_Write, 0, 0, nPixels, nLines, 
      //            m_pafScanbandDest, nPixels, nLines, m_DataType, 
         //         1, NULL, 0, 0, 0 );

   delete[] m_geoTransform;

   //free memory, and close output file
   m_gdal.Close(m_datasetDestH);
   m_gdal.Close(m_datasetSourceH);

   return m_pafScanbandDest;
   }

bool GeoSpatialDataObj::NetCDF2GeoTransWarpImage(LPCTSTR inputFile, LPCTSTR outputFile, LPCTSTR poprjWKTFile, int set3Dvalue)
   {

   GDALDatasetH  m_datasetSourceH, m_datasetDestH;
   GDALDriverH   hDriverSource, hDriverDest;
   GDALWrapper   m_gdal;
   GDALDataType  m_DataType;

   CString msg;

   const char* prjWKTFile = poprjWKTFile;

   double
      m_CellLatDegResolution,
      m_CellLonDegResolution,
      m_UpperLeftyCoord,
      m_UpperLeftxCoord,
      *m_geoTransform,
      m_imgNorthUp2,
      m_noDataValue,
      m_imgNorthUp4;

   int
      m_nXSize,
      m_nYSize,
      m_nBands;
   //m_RasterXYZSize;

   m_geoTransform = new double[6];

   //LPCTSTR m_GeoFilename  =  "\\Envision\\StudyAreas\\WW2100\\wms.xml";
   LPCTSTR m_GeoFilename = inputFile;

   m_gdal.Init();

   const TCHAR* cstr = (LPCTSTR)inputFile;

   //The GDAL Open function will recognise the wms.xml file as a Web Map Service Request.  The WMS driver
   //and mini driver will be selected.  Returned from this process is a GDAL dataset, the backbone to the
   //GDAL method.  As part of the dataset a geotransform is created.
   m_datasetSourceH = m_gdal.Open(cstr, GA_ReadOnly);

   if (m_datasetSourceH == NULL)
      {
      msg.Format(_T("GeoSpatialDataObj::failed to open inputFile for tranformation and warp"));
      Report::ErrorMsg(msg);
      return false;
      }

   GDALRasterBandH rasterBandsourceH = m_gdal.GetRasterBand(m_datasetSourceH, set3Dvalue);

   m_gdal.GetGeoTransform(m_datasetSourceH, m_geoTransform);



   //for debugging to look at input files georeferencing info
   m_CellLonDegResolution = *(m_geoTransform + 1);
   m_CellLatDegResolution = *(m_geoTransform + 5);
   m_UpperLeftyCoord = *(m_geoTransform + 3);
   m_UpperLeftxCoord = *(m_geoTransform + 0);
   m_imgNorthUp4 = *(m_geoTransform + 4);
   m_imgNorthUp2 = *(m_geoTransform + 2);

   m_nBands = m_gdal.GetRasterCount(m_datasetSourceH);
   hDriverSource = m_gdal.GetDataSetDriver(m_datasetSourceH);
   m_DataType = m_gdal.GetRasterDataType(rasterBandsourceH);
   m_nXSize = m_gdal.GetRasterBandXSize(rasterBandsourceH);
   m_nYSize = m_gdal.GetRasterBandYSize(rasterBandsourceH);
   m_noDataValue = m_gdal.GetRasterNoDataValue(rasterBandsourceH, NULL);

   if (set3Dvalue <= 0)
      {
      msg.Format(_T("GeoSpatialDataObj::Beginning band index is 1 based, therfore index must be greater than or equal to 0"));
      Report::ErrorMsg(msg);
      return false;
      }

   if (set3Dvalue > m_nBands)
      {
      msg.Format(_T("GeoSpatialDataObj::Ending band index is 1 based, therefore index must less than or equal to number of bands in the input file"));
      Report::ErrorMsg(msg);
      return false;
      }

   //m_RasterXYZSize = m_nXSize * m_nYSize * m_nBands;

   //unsigned char *m_pafScanband =  new unsigned char[ m_RasterXYZSize ];

   //this reads input file into a raster in memory, not really used in this operation
   //m_gdal.DatasetRasterIO( m_datasetSourceH, GF_Read, 0, 0, m_nXSize, m_nYSize, 
   //               m_pafScanband, m_nXSize, m_nYSize, m_DataType, 
   //               m_nBands, NULL, 0, 0, 0 );

   //*************** TRANSFORM *******************************************

    // Get outputFile driver (GeoTIFF format)
   hDriverDest = m_gdal.GetDriverByName("MEM");

   // Get Source WKT coordinate system. 
   const char *pszSrcWKT;
   char   **pszDstWKT = NULL;
   const char *pszDst2WKT = NULL;
   pszSrcWKT = m_gdal.GetProjectionRef(m_datasetSourceH);

   //a few things not used, but might come in handy
   double LongTemp = (m_UpperLeftxCoord + 180) - floor((m_UpperLeftxCoord + 180) / 360) * 360 - 180; // -180.00 .. 179.9;

   int utmzone = (int)floor((LongTemp + 180) / 6) + 1;

   //user supplied WKT/prj file projection info
   pszDst2WKT = prjWKTFile;

   // Create a transformer that maps from source pixel/line coordinates
    // to destination georeferenced coordinates (not destination 
    // pixel line).  We do that by omitting the destination dataset
    // handle (setting it to NULL). 
   void *hTransformArg;

   //This function creates a transformation object that maps from pixel/line coordinates on one image 
   //to pixel/line coordinates on another image. The images may potentially be georeferenced in different 
   //coordinate systems, and may used GCPs to map between their pixel/line coordinates and georeferenced 
   //coordinates (as opposed to the default assumption that their geotransform should be used).
   hTransformArg = m_gdal.GDALCreateGenImgProjTransformer(m_datasetSourceH, pszSrcWKT, NULL, pszDst2WKT,
      FALSE, 0, 1);
   ASSERT(hTransformArg != NULL);

   // Get approximate output georeferenced bounds and resolution for file. 
   double adfDstGeoTransform[6];
   int nPixels, nLines;
   CPLErr eErr;

   //This function is used to suggest the size, and georeferenced extents appropriate given the 
  //indicated transformation and input file. It walks the edges of the input file (approximately 
  //20 sample points along each edge) transforming into output coordinates in order to get an extents box.

   eErr = m_gdal.GDALSuggestedWarpOutput(m_datasetSourceH,
      m_gdal.m_GDALGenImgProjTransformFn, hTransformArg,
      adfDstGeoTransform, &nPixels, &nLines);
   ASSERT(eErr == CE_None);

   m_gdal.GDALDestroyGenImgProjTransformer(hTransformArg);

   //Create the output file.  


   CString bandNumber, tmp_outfile;

   tmp_outfile = outputFile;

   bandNumber.Format("%d", set3Dvalue);

   bandNumber.Insert(0, '_');

   tmp_outfile.Insert(tmp_outfile.ReverseFind('.'), bandNumber);

   m_datasetDestH = m_gdal.Create(hDriverDest, "tmp_outfile", nPixels, nLines,
      1, m_DataType, NULL);

   ASSERT(m_datasetDestH != NULL);

   if (m_datasetDestH == NULL)
      {
      msg.Format(_T("GeoSpatialDataObj::failed to create file driver for outputFile for tranformation and warp"));
      Report::ErrorMsg(msg);
      return false;
      }

   // Set projection information for output file. 

   m_gdal.SetProjection(m_datasetDestH, pszDst2WKT);

   m_gdal.SetGeoTransform(m_datasetDestH, adfDstGeoTransform);

   // Copy the color table, if required.
   //GDALColorTableH hCT;

   //hCT = m_gdal.GDALGetRasterColorTable( m_gdal.GetRasterBand(m_datasetSourceH,set3Dvalue) );

   //if( hCT != NULL )
   //         m_gdal.GDALSetRasterColorTable( m_gdal.GetRasterBand(m_datasetDestH,set3Dvalue), hCT );

   //************************** WARP *******************************************************

   GDALWarpOptions *psWarpOptions = m_gdal.GDALCreateWarpOptions();

   psWarpOptions->hSrcDS = m_datasetSourceH;
   psWarpOptions->hDstDS = m_datasetDestH;
   psWarpOptions->nBandCount = 1;
   psWarpOptions->panSrcBands = (int *)m_gdal.CPLMalloc(sizeof(int) * psWarpOptions->nBandCount);
   psWarpOptions->panDstBands = (int *)m_gdal.CPLMalloc(sizeof(int) * psWarpOptions->nBandCount);

   //set warp options
   psWarpOptions->panSrcBands[0] = set3Dvalue;
   psWarpOptions->panDstBands[0] = 1;


   // Establish reprojection transformer. 
   psWarpOptions->pTransformerArg =
      m_gdal.GDALCreateGenImgProjTransformer(m_datasetSourceH,
         m_gdal.GetProjectionRef(m_datasetSourceH),
         m_datasetDestH,
         m_gdal.GetProjectionRef(m_datasetDestH),
         FALSE, 0.0, 1);

   psWarpOptions->pfnTransformer = m_gdal.m_GDALGenImgProjTransformFn;

   // Initialize and execute the warp operation. 
   GDALWarpOperationH hWoperation = m_gdal.GDALCreateWarpOperation(psWarpOptions);

   m_gdal.GDALChunkAndWarpImage(hWoperation, 0, 0,
      m_gdal.GDALGetRasterXSize(m_datasetDestH),
      m_gdal.GDALGetRasterYSize(m_datasetDestH));

   //clean a few things up
   m_gdal.GDALDestroyGenImgProjTransformer(psWarpOptions->pTransformerArg);
   m_gdal.GDALDestroyWarpOptions(psWarpOptions);

   //intialize a few things for making a raster
   int m_RasterXYZSizeDest = (nPixels) * (nLines) * 1;

   GDALDriverH hDriverOutput = m_gdal.GetDriverByName("GTiff");

   GDALDatasetH m_datasetOutputH = m_gdal.GDALCreateCopy(hDriverOutput, tmp_outfile, m_datasetDestH, FALSE,
      NULL, NULL, NULL);

   float *m_pafScanbandDest = new float[m_RasterXYZSizeDest];

   //read the image into raster in memory
   m_gdal.DatasetRasterIO(m_datasetOutputH, GF_Read, 0, 0, nPixels, nLines,
      m_pafScanbandDest, nPixels, nLines, m_DataType,
      1, NULL, 0, 0, 0);

   float dat;

   for (int i = 0; i < m_RasterXYZSizeDest; i++)
      {
      dat = *(m_pafScanbandDest + i);
      }

   //output raster to image file on disk
   m_gdal.DatasetRasterIO(m_datasetOutputH, GF_Write, 0, 0, nPixels, nLines,
      m_pafScanbandDest, nPixels, nLines, m_DataType,
      1, NULL, 0, 0, 0);

   delete[] m_geoTransform;
   delete[] m_pafScanbandDest;

   //free memory, and close output file
   m_gdal.Close(m_datasetDestH);
   m_gdal.Close(m_datasetOutputH);
   m_gdal.Close(m_datasetSourceH);

   return true;
   }

bool GeoSpatialDataObj::InitLibraries()
   {
   bool okpath = m_gdal.Init();

   if (!okpath)
      {
      CString msg;
      msg.Format(_T("GeoSpatialDataObj::Open Cannot find GDAL DLLs, check path and filename "));
      Report::ErrorMsg(msg);
      return false;
      }

   return true;
   }

bool GeoSpatialDataObj::Open(LPCTSTR filename, LPCTSTR variable)
   {
   CString msg;
   CString typ = "NETCDF:";
   CString var = variable;
   //Open the NETCDF subdataset NETCDF:filename:subdataset         
   //CString pszFilename = "NETCDF:\\gridded_obs_daily_Tavg_1999.nc:Tavg";
   if (!var.IsEmpty())
      m_GeoFilename = typ + filename + ":" + variable;
   else
      m_GeoFilename = filename;

   GDALDriverH m_hDriver = NULL;

   const TCHAR* cstr = (LPCTSTR)m_GeoFilename;

   //m_datasetH = m_gdal.Open( "NETCDF:/Envision/StudyAreas/Cimarron/climate_cim/hist_apcp_2001.nc:apcp", GA_ReadOnly );

   m_datasetH = m_gdal.Open(cstr, GA_ReadOnly);

   if (m_datasetH == NULL)
      {
      LPCTSTR errmsg = m_gdal.CPLGetLastErrorMsg();
      if (errmsg == NULL)
         errmsg = "";

      msg.Format(_T("GeoSpatialDataObj: Unable to open/read file '%s', check path, variable, and/or filename. %s"), (LPCTSTR)m_GeoFilename, errmsg);
      //msg += m_GeoFilename;
      Report::ErrorMsg(msg);
      return false;
      }
   //m_geoTransform = new double[6];   
   m_gdal.GetGeoTransform(m_datasetH, m_geoTransform);
   m_CellLonDegResolution = m_geoTransform[1];
   m_CellLatDegResolution = m_geoTransform[5];
   m_UpperLeftLat = m_geoTransform[3];
   m_UpperLeftLon = m_geoTransform[0];
   m_imgNorthUp4 = m_geoTransform[4];
   m_imgNorthUp2 = m_geoTransform[2];
   m_nBands = m_gdal.GetRasterCount(m_datasetH);
   m_hDriver = m_gdal.GetDataSetDriver(m_datasetH);
   GDALRasterBandH temp_Band = m_gdal.GetRasterBand(m_datasetH, 1);
   m_DataType = m_gdal.GetRasterDataType(temp_Band);
   m_nXSize = m_gdal.GetRasterBandXSize(temp_Band);
   m_nYSize = m_gdal.GetRasterBandYSize(temp_Band);
   m_noDataValue = m_gdal.GetRasterNoDataValue(temp_Band, NULL);
   m_RasterXYZSize = m_nXSize * m_nYSize * m_nBands;

   return true;
   }

bool GeoSpatialDataObj::GeoImage2GeoTransWarpImage(LPCTSTR inputFile, LPCTSTR outputFile, LPCTSTR poprjWKTFile)
   {
   GDALDatasetH  m_datasetSourceH, m_datasetDestH;
   GDALDriverH   hDriverSource, hDriverDest;
   GDALWrapper   m_gdal;
   GDALDataType  m_DataType;

   CString msg;

   const char* prjWKTFile = poprjWKTFile;

   double
      m_CellLatDegResolution,
      m_CellLonDegResolution,
      m_UpperLeftyCoord,
      m_UpperLeftxCoord,
      //*m_geoTransform(NULL),
      m_imgNorthUp2,
      m_noDataValue,
      m_imgNorthUp4;

   int
      m_nXSize,
      m_nYSize,
      m_nBands,
      m_RasterXYZSize;


   //LPCTSTR m_GeoFilename  =  "\\Envision\\StudyAreas\\WW2100\\wms.xml";
   LPCTSTR m_GeoFilename = inputFile;

   m_gdal.Init();

   const TCHAR* cstr = (LPCTSTR)inputFile;

   //The GDAL Open function will recognise the wms.xml file as a Web Map Service Request.  The WMS driver
   //and mini driver will be selected.  Returned from this process is a GDAL dataset, the backbone to the
   //GDAL method.  As part of the dataset a geotransform is created.
   m_datasetSourceH = m_gdal.Open(cstr, GA_ReadOnly);

   if (m_datasetSourceH == NULL)
      {
      msg.Format(_T("GeoSpatialDataObj::failed to open inputFile for tranformation and warp"));
      Report::ErrorMsg(msg);
      return false;
      }

   GDALRasterBandH rasterBandsourceH = m_gdal.GetRasterBand(m_datasetSourceH, 1);

   double *m_geoTransform = new double[6];

   m_gdal.GetGeoTransform(m_datasetSourceH, m_geoTransform);

   //for debugging to look at input files georeferencing info
   m_CellLonDegResolution = m_geoTransform[1];
   m_CellLatDegResolution = m_geoTransform[5];
   m_UpperLeftyCoord = m_geoTransform[3];
   m_UpperLeftxCoord = m_geoTransform[0];
   m_imgNorthUp4 = m_geoTransform[4];
   m_imgNorthUp2 = m_geoTransform[2];
   m_nBands = m_gdal.GetRasterCount(m_datasetSourceH);
   hDriverSource = m_gdal.GetDataSetDriver(m_datasetSourceH);
   m_DataType = m_gdal.GetRasterDataType(rasterBandsourceH);
   m_nXSize = m_gdal.GetRasterBandXSize(rasterBandsourceH);
   m_nYSize = m_gdal.GetRasterBandYSize(rasterBandsourceH);
   m_noDataValue = m_gdal.GetRasterNoDataValue(rasterBandsourceH, NULL);

   m_RasterXYZSize = m_nXSize * m_nYSize * m_nBands;

   unsigned char *m_pafScanband = new unsigned char[m_RasterXYZSize];

   //this reads input file into a raster in memory, not really used in this operation
   m_gdal.DatasetRasterIO(m_datasetSourceH, GF_Read, 0, 0, m_nXSize, m_nYSize,
      m_pafScanband, m_nXSize, m_nYSize, m_DataType,
      m_nBands, NULL, 0, 0, 0);

   //*************** TRANSFORM *******************************************

   // Get outputFile driver (GeoTIFF format)
   hDriverDest = m_gdal.GetDriverByName("GTiff");

   // Get Source WKT coordinate system. 
   const char *pszSrcWKT;
   char   **pszDstWKT = NULL;
   const char *pszDst2WKT = NULL;
   pszSrcWKT = m_gdal.GetProjectionRef(m_datasetSourceH);

   //a few things not used, but might come in handy
   double LongTemp = (m_UpperLeftxCoord + 180) - floor((m_UpperLeftxCoord + 180) / 360) * 360 - 180; // -180.00 .. 179.9;

   int utmzone = (int)floor((LongTemp + 180) / 6) + 1;

   //user supplied WKT/prj file projection info
   pszDst2WKT = prjWKTFile;

   // Create a transformer that maps from source pixel/line coordinates
   // to destination georeferenced coordinates (not destination 
   // pixel line).  We do that by omitting the destination dataset
   // handle (setting it to NULL). 
   void *hTransformArg;

   //This function creates a transformation object that maps from pixel/line coordinates on one image 
   //to pixel/line coordinates on another image. The images may potentially be georeferenced in different 
   //coordinate systems, and may used GCPs to map between their pixel/line coordinates and georeferenced 
   //coordinates (as opposed to the default assumption that their geotransform should be used).
   hTransformArg = m_gdal.GDALCreateGenImgProjTransformer(m_datasetSourceH, pszSrcWKT, NULL, pszDst2WKT,
      FALSE, 0, 1);
   ASSERT(hTransformArg != NULL);

   // Get approximate output georeferenced bounds and resolution for file. 
   double adfDstGeoTransform[6];
   int nPixels, nLines;
   CPLErr eErr;

   //This function is used to suggest the size, and georeferenced extents appropriate given the 
   //indicated transformation and input file. It walks the edges of the input file (approximately 
   //20 sample points along each edge) transforming into output coordinates in order to get an extents box.

   eErr = m_gdal.GDALSuggestedWarpOutput(m_datasetSourceH,
      m_gdal.m_GDALGenImgProjTransformFn, hTransformArg,
      adfDstGeoTransform, &nPixels, &nLines);
   ASSERT(eErr == CE_None);

   m_gdal.GDALDestroyGenImgProjTransformer(hTransformArg);

   // Create the output file.  
   m_datasetDestH = m_gdal.Create(hDriverDest, outputFile, nPixels, nLines,
      m_nBands, m_DataType, NULL);

   ASSERT(m_datasetDestH != NULL);

   if (m_datasetDestH == NULL)
      {
      msg.Format(_T("GeoSpatialDataObj::failed to create file driver for outputFile for tranformation and warp"));
      Report::ErrorMsg(msg);
      return false;
      }

   // Set projection information for output file. 

   m_gdal.SetProjection(m_datasetDestH, pszDst2WKT);

   m_gdal.SetGeoTransform(m_datasetDestH, adfDstGeoTransform);

   // Copy the color table, if required.
   GDALColorTableH hCT;

   for (int i = 1; i <= m_nBands; i++)
      {
      hCT = m_gdal.GDALGetRasterColorTable(m_gdal.GetRasterBand(m_datasetSourceH, i));
      if (hCT != NULL)
         m_gdal.GDALSetRasterColorTable(m_gdal.GetRasterBand(m_datasetDestH, i), hCT);
      }

   //************************** WARP *******************************************************

   GDALWarpOptions *psWarpOptions = m_gdal.GDALCreateWarpOptions();

   psWarpOptions->hSrcDS = m_datasetSourceH;
   psWarpOptions->hDstDS = m_datasetDestH;
   psWarpOptions->nBandCount = m_nBands;
   psWarpOptions->panSrcBands = (int *)m_gdal.CPLMalloc(sizeof(int) * psWarpOptions->nBandCount);
   psWarpOptions->panDstBands = (int *)m_gdal.CPLMalloc(sizeof(int) * psWarpOptions->nBandCount);

   //set warp options
   for (int j = 0; j < m_nBands; j++)
      {
      psWarpOptions->panSrcBands[j] = j + 1;
      psWarpOptions->panDstBands[j] = j + 1;
      }

   // Establish reprojection transformer. 
   psWarpOptions->pTransformerArg =
      m_gdal.GDALCreateGenImgProjTransformer(m_datasetSourceH,
         m_gdal.GetProjectionRef(m_datasetSourceH),
         m_datasetDestH,
         m_gdal.GetProjectionRef(m_datasetDestH),
         FALSE, 0.0, 1);

   psWarpOptions->pfnTransformer = m_gdal.m_GDALGenImgProjTransformFn;

   // Initialize and execute the warp operation. 
   GDALWarpOperationH hWoperation = m_gdal.GDALCreateWarpOperation(psWarpOptions);

   m_gdal.GDALChunkAndWarpImage(hWoperation, 0, 0,
      m_gdal.GDALGetRasterXSize(m_datasetDestH),
      m_gdal.GDALGetRasterYSize(m_datasetDestH));

   //clean a few things up
   m_gdal.GDALDestroyGenImgProjTransformer(psWarpOptions->pTransformerArg);
   m_gdal.GDALDestroyWarpOptions(psWarpOptions);

   //intialize a few things for making a raster
   int m_RasterXYZSizeDest = nPixels * nLines * m_nBands;

   ///?????? CHECK DELETE
   unsigned char *m_pafScanbandDest = new unsigned char[m_RasterXYZSizeDest];

   //read the image into raster in memory
   m_gdal.DatasetRasterIO(m_datasetDestH, GF_Read, 0, 0, nPixels, nLines,
      m_pafScanbandDest, nPixels, nLines, m_DataType,
      m_nBands, NULL, 0, 0, 0);

   //output raster to image file on disk
   m_gdal.DatasetRasterIO(m_datasetDestH, GF_Write, 0, 0, nPixels, nLines,
      m_pafScanbandDest, nPixels, nLines, m_DataType,
      m_nBands, NULL, 0, 0, 0);

   //free memory, and close output file
   m_gdal.Close(m_datasetDestH);
   m_gdal.Close(m_datasetSourceH);

   //delete m_geoTransform;
   if (m_geoTransform != NULL)
      {
      delete[] m_geoTransform;
      }

   return true;
   }

bool GeoSpatialDataObj::GeoImage2GeoTransWarpImage(LPCTSTR inputFile, LPCTSTR outputFile, LPCTSTR poprjWKTFile, LPCTSTR inFileWKTFile, float xResolution, float yResolution, float xUpperLeft, float yUpperLeft)
   {
   GDALDatasetH  m_datasetSourceH, m_datasetDestH;
   GDALDriverH   hDriverSource, hDriverDest;
   GDALWrapper   m_gdal;
   GDALDataType  m_DataType;

   CString msg;

   const char* prjWKTFile = poprjWKTFile;

   double
      m_CellLatDegResolution,
      m_CellLonDegResolution,
      m_UpperLeftyCoord,
      m_UpperLeftxCoord,
      //*m_geoTransform(NULL),
      m_imgNorthUp2,
      m_noDataValue,
      m_imgNorthUp4;

   int
      m_nXSize,
      m_nYSize,
      m_nBands,
      m_RasterXYZSize;


   //LPCTSTR m_GeoFilename  =  "\\Envision\\StudyAreas\\WW2100\\wms.xml";
   LPCTSTR m_GeoFilename = inputFile;

   m_gdal.Init();

   const TCHAR* cstr = (LPCTSTR)inputFile;

   //The GDAL Open function will recognise the wms.xml file as a Web Map Service Request.  The WMS driver
   //and mini driver will be selected.  Returned from this process is a GDAL dataset, the backbone to the
   //GDAL method.  As part of the dataset a geotransform is created.
   m_datasetSourceH = m_gdal.Open(cstr, GA_ReadOnly);

   if (m_datasetSourceH == NULL)
      {
      msg.Format(_T("GeoSpatialDataObj::failed to open inputFile for tranformation and warp"));
      Report::ErrorMsg(msg);
      return false;
      }

   GDALRasterBandH rasterBandsourceH = m_gdal.GetRasterBand(m_datasetSourceH, 1);

   double *m_geoTransform = new double[6];

   m_gdal.GetGeoTransform(m_datasetSourceH, m_geoTransform);

   //if (m_geoTransform == NULL)
   {
   m_geoTransform[1] = xResolution;
   m_geoTransform[5] = yResolution;
   m_geoTransform[3] = yUpperLeft;
   m_geoTransform[0] = xUpperLeft;
   m_geoTransform[4] = 0;
   m_geoTransform[2] = 0;
   }

   //for debugging to look at input files georeferencing info
   m_CellLonDegResolution = m_geoTransform[1];
   m_CellLatDegResolution = m_geoTransform[5];
   m_UpperLeftyCoord = m_geoTransform[3];
   m_UpperLeftxCoord = m_geoTransform[0];
   m_imgNorthUp4 = m_geoTransform[4];
   m_imgNorthUp2 = m_geoTransform[2];
   m_nBands = m_gdal.GetRasterCount(m_datasetSourceH);
   hDriverSource = m_gdal.GetDataSetDriver(m_datasetSourceH);
   m_DataType = m_gdal.GetRasterDataType(rasterBandsourceH);
   m_nXSize = m_gdal.GetRasterBandXSize(rasterBandsourceH);
   m_nYSize = m_gdal.GetRasterBandYSize(rasterBandsourceH);
   m_noDataValue = m_gdal.GetRasterNoDataValue(rasterBandsourceH, NULL);

   m_RasterXYZSize = m_nXSize * m_nYSize * m_nBands;

   unsigned char *m_pafScanband = new unsigned char[m_RasterXYZSize];

   //this reads input file into a raster in memory, not really used in this operation
   m_gdal.DatasetRasterIO(m_datasetSourceH, GF_Read, 0, 0, m_nXSize, m_nYSize,
      m_pafScanband, m_nXSize, m_nYSize, m_DataType,
      m_nBands, NULL, 0, 0, 0);

   //*************** TRANSFORM *******************************************

    // Get outputFile driver (GeoTIFF format)
   hDriverDest = m_gdal.GetDriverByName("GTiff");

   // Get Source WKT coordinate system. 
   const char *pszSrcWKT;
   char   **pszDstWKT = NULL;
   const char *pszDst2WKT = NULL;
   //pszSrcWKT = m_gdal.GetProjectionRef( m_datasetSourceH );

   pszSrcWKT = inFileWKTFile;

   //a few things not used, but might come in handy
   double LongTemp = (m_UpperLeftxCoord + 180) - floor((m_UpperLeftxCoord + 180) / 360) * 360 - 180; // -180.00 .. 179.9;

   int utmzone = (int)floor((LongTemp + 180) / 6) + 1;

   //user supplied WKT/prj file projection info
   pszDst2WKT = prjWKTFile;

   // Create a transformer that maps from source pixel/line coordinates
    // to destination georeferenced coordinates (not destination 
    // pixel line).  We do that by omitting the destination dataset
    // handle (setting it to NULL). 
   void *hTransformArg;

   //This function creates a transformation object that maps from pixel/line coordinates on one image 
   //to pixel/line coordinates on another image. The images may potentially be georeferenced in different 
   //coordinate systems, and may used GCPs to map between their pixel/line coordinates and georeferenced 
   //coordinates (as opposed to the default assumption that their geotransform should be used).
   hTransformArg = m_gdal.GDALCreateGenImgProjTransformer(m_datasetSourceH, pszSrcWKT, NULL, pszDst2WKT,
      FALSE, 0, 1);
   ASSERT(hTransformArg != NULL);

   // Get approximate output georeferenced bounds and resolution for file. 
   double adfDstGeoTransform[6];
   int nPixels, nLines;
   CPLErr eErr;

   //This function is used to suggest the size, and georeferenced extents appropriate given the 
  //indicated transformation and input file. It walks the edges of the input file (approximately 
  //20 sample points along each edge) transforming into output coordinates in order to get an extents box.

   eErr = m_gdal.GDALSuggestedWarpOutput(m_datasetSourceH,
      m_gdal.m_GDALGenImgProjTransformFn, hTransformArg,
      adfDstGeoTransform, &nPixels, &nLines);
   ASSERT(eErr == CE_None);

   m_gdal.GDALDestroyGenImgProjTransformer(hTransformArg);

   // Create the output file.  
   m_datasetDestH = m_gdal.Create(hDriverDest, outputFile, nPixels, nLines,
      m_nBands, m_DataType, NULL);

   ASSERT(m_datasetDestH != NULL);

   if (m_datasetDestH == NULL)
      {
      msg.Format(_T("GeoSpatialDataObj::failed to create file driver for outputFile for tranformation and warp"));
      Report::ErrorMsg(msg);
      return false;
      }

   // Set projection information for output file. 

   m_gdal.SetProjection(m_datasetDestH, pszDst2WKT);

   m_gdal.SetGeoTransform(m_datasetDestH, adfDstGeoTransform);

   // Copy the color table, if required.
   GDALColorTableH hCT;

   for (int i = 1; i <= m_nBands; i++)
      {
      hCT = m_gdal.GDALGetRasterColorTable(m_gdal.GetRasterBand(m_datasetSourceH, i));
      if (hCT != NULL)
         m_gdal.GDALSetRasterColorTable(m_gdal.GetRasterBand(m_datasetDestH, i), hCT);
      }

   //************************** WARP *******************************************************

   GDALWarpOptions *psWarpOptions = m_gdal.GDALCreateWarpOptions();

   psWarpOptions->hSrcDS = m_datasetSourceH;
   psWarpOptions->hDstDS = m_datasetDestH;
   psWarpOptions->nBandCount = m_nBands;
   psWarpOptions->panSrcBands = (int *)m_gdal.CPLMalloc(sizeof(int) * psWarpOptions->nBandCount);
   psWarpOptions->panDstBands = (int *)m_gdal.CPLMalloc(sizeof(int) * psWarpOptions->nBandCount);

   //set warp options
   for (int j = 0; j < m_nBands; j++)
      {
      psWarpOptions->panSrcBands[j] = j + 1;
      psWarpOptions->panDstBands[j] = j + 1;
      }

   // Establish reprojection transformer. 
   psWarpOptions->pTransformerArg =
      m_gdal.GDALCreateGenImgProjTransformer(m_datasetSourceH,
         m_gdal.GetProjectionRef(m_datasetSourceH),
         m_datasetDestH,
         m_gdal.GetProjectionRef(m_datasetDestH),
         FALSE, 0.0, 1);

   psWarpOptions->pfnTransformer = m_gdal.m_GDALGenImgProjTransformFn;

   // Initialize and execute the warp operation. 
   GDALWarpOperationH hWoperation = m_gdal.GDALCreateWarpOperation(psWarpOptions);

   m_gdal.GDALChunkAndWarpImage(hWoperation, 0, 0,
      m_gdal.GDALGetRasterXSize(m_datasetDestH),
      m_gdal.GDALGetRasterYSize(m_datasetDestH));

   //clean a few things up
   m_gdal.GDALDestroyGenImgProjTransformer(psWarpOptions->pTransformerArg);
   m_gdal.GDALDestroyWarpOptions(psWarpOptions);

   //intialize a few things for making a raster
   int m_RasterXYZSizeDest = nPixels * nLines * m_nBands;

   ///?????? CHECK DELETE
   unsigned char *m_pafScanbandDest = new unsigned char[m_RasterXYZSizeDest];

   //read the image into raster in memory
   m_gdal.DatasetRasterIO(m_datasetDestH, GF_Read, 0, 0, nPixels, nLines,
      m_pafScanbandDest, nPixels, nLines, m_DataType,
      m_nBands, NULL, 0, 0, 0);

   //output raster to image file on disk
   m_gdal.DatasetRasterIO(m_datasetDestH, GF_Write, 0, 0, nPixels, nLines,
      m_pafScanbandDest, nPixels, nLines, m_DataType,
      m_nBands, NULL, 0, 0, 0);

   //free memory, and close output file
   m_gdal.Close(m_datasetDestH);
   m_gdal.Close(m_datasetSourceH);

   //delete m_geoTransform;
   if (m_geoTransform != NULL)
      {
      delete[] m_geoTransform;
      }

   return true;
   }

bool GeoSpatialDataObj::ReadSpatialData(void)
   {
   CString msg;

   if (m_3DVal < 0)
      {
      msg.Format(_T("GeoSpatialDataObj::ReadSpatialData set3Dvalue must be from 0 to n3Dcount-1"));
      Report::ErrorMsg(msg);
      return false;
      }

   int size = m_nXSize* m_nYSize*m_nBands;   // defined in Open();
   switch (m_DataType)                       // defined in Open()
      {
      case GDT_Byte:      // Eight bit unsigned integer
      {
      m_pafScanband = new char[size];
      break;
      }
      case GDT_UInt16:    // Sixteen bit unsigned integer      
      {
      m_pafScanband = new unsigned short int[size];
      break;
      }
      case GDT_Int16:    // Sixteen bit signed integer
      {
      m_pafScanband = new short int[size];
      break;
      }
      case GDT_CInt16:    // Complex Int16   
      {
      m_pafScanband = new complex< short int >[size];
      break;
      }
      case GDT_UInt32:    // Thirty two bit unsigned integer
      {
      m_pafScanband = new unsigned long int[size];
      break;
      }
      case GDT_CInt32:    // Complex Int32
      {
      m_pafScanband = new complex< long int >[size];
      break;
      }
      case GDT_Int32:    // Thirty two bit signed integer
      {
      m_pafScanband = new long int[size];
      break;
      }
      case GDT_Float32:    // Thirty two bit floating point
      {
      m_pafScanband = new float[size];
      break;
      }
      case GDT_CFloat32:    // Complex Float32   
      {
      m_pafScanband = new complex< float>[size];
      break;
      }
      case GDT_Float64:    // Sixty four bit floating point
      {
      m_pafScanband = new double[size];
      break;
      }
      case GDT_CFloat64:  // Complex Float64
      {
      m_pafScanband = new complex< double >[size];
      break;
      }
      default:
         msg.Format(_T("GeoSpatialDataObj:  Unknown GDAL data type."));
         Report::ErrorMsg(msg);
         return false;
      }

   //float *pafScanband = (float*) gdal.CPLMalloc(sizeof(float)*nXSize*nYSize);
   //see gdal reference for options here, pafScanband is the full 3d or 2d band in memory
   m_gdal.DatasetRasterIO(m_datasetH, GF_Read, 0, 0, m_nXSize, m_nYSize,
      m_pafScanband, m_nXSize, m_nYSize, m_DataType,
      m_nBands, NULL, 0, 0, 0);

   bool gridPolyLookup = false;

   if (gridPolyLookup)
      {
      m_pafScanbandbool = new bool[m_nXSize * m_nYSize]; //initializes bool array necessary for polygrid lookup routine.
      switch (m_DataType)
         {
         case GDT_Byte:      // Eight bit unsigned integer
            BuildDataObj((char *)m_pafScanband, (char)m_noDataValue, m_pafScanbandbool, m_RasterXYZSize);
            break;

         case GDT_UInt16:    // Sixteen bit unsigned integer      
            //BuildDataObj((unsigned short int *) m_pafScanband,(unsigned short int ) m_noDataValue, m_pafScanbandbool,m_RasterXYZSize);
            break;

         case GDT_Int16:    // Sixteen bit signed integer
            BuildDataObj((short int *)m_pafScanband, (short int)m_noDataValue, m_pafScanbandbool, m_RasterXYZSize);
            break;

         case GDT_CInt16:    // Complex Int16   
            //BuildDataObj((complex< short int > *) m_pafScanband,(complex< short int > ) m_noDataValue, m_pafScanbandbool,m_RasterXYZSize); //create error
            break;

         case GDT_UInt32:    // Thirty two bit unsigned integer
            BuildDataObj((unsigned long int *) m_pafScanband, (unsigned long int) m_noDataValue, m_pafScanbandbool, m_RasterXYZSize);
            break;

         case GDT_CInt32:    // Complex Int32
            BuildDataObj((complex< long >*) m_pafScanband, (complex< long >) m_noDataValue, m_pafScanbandbool, m_RasterXYZSize);
            break;

         case GDT_Int32:    // Thirty two bit signed integer
            BuildDataObj((long int *)m_pafScanband, (long int)m_noDataValue, m_pafScanbandbool, m_RasterXYZSize);
            break;

         case GDT_Float32:    // Thirty two bit floating point
            BuildDataObj((float *)m_pafScanband, (float)m_noDataValue, m_pafScanbandbool, m_RasterXYZSize);
            break;

         case GDT_CFloat32:    // Complex Float32   
            BuildDataObj((complex< float > *) m_pafScanband, complex< float >(m_noDataValue), m_pafScanbandbool, m_RasterXYZSize);
            break;

         case GDT_Float64:    // Sixty four bit floating point
            BuildDataObj((double *)m_pafScanband, (double)m_noDataValue, m_pafScanbandbool, m_RasterXYZSize);
            break;

         case GDT_CFloat64:  // Complex Float64
            BuildDataObj((complex< double > *) m_pafScanband, (complex< double >) m_noDataValue, m_pafScanbandbool, m_RasterXYZSize);
            break;

         default:
            msg.Format(_T("GeoSpatialDataObj:  Unknown GDAL data type."));
            Report::ErrorMsg(msg);
            return false;
         }
      }

   return true;
   }

bool GeoSpatialDataObj::Close(void)
   {
   if (m_pafScanband != NULL)
      {
      delete[] m_pafScanband;
      m_pafScanband = NULL;
      }

   if (m_pafScanbandbool != NULL)
      {
      delete[] m_pafScanbandbool;
      m_pafScanbandbool = NULL;
      }

   //if ( m_geoTransform != NULL )
   //   {
   //   delete [] m_geoTransform;
   //   m_geoTransform = NULL;
   //   }

   m_gdal.Close(m_datasetH);

   return true;
   }
/*
bool GeoSpatialDataObj::LoadPolyGridLookup(MapLayer* m_pPolyMapLayer, int nsubgrids)
   {
   CString msg;
   ASSERT(0);

   int numPolys = m_pPolyMapLayer->GetPolygonCount();

   // instantiate the PolyGridLookups object
   m_pPolyGridLkUp = new PolyGridLookups(
      m_UpperLeftLat,
      m_UpperLeftLon,
      m_CellLatDegResolution,
      m_CellLonDegResolution,
      m_pafScanbandbool,
      m_nYSize,
      m_nXSize,
      m_pPolyMapLayer,
      nsubgrids,
      m_nYSize * m_nXSize * 2,
      0,
      -9999);

   int found = m_GeoFilename.Find("NETCDF:");

   if (found >= 0)
      {
      int colonloc = m_GeoFilename.Find(':', 0);

      int extloc = m_GeoFilename.ReverseFind('.');

      m_PglFilename = m_GeoFilename.Mid(colonloc + 1, extloc - colonloc - 1);
      }
   else
      {
      int dotloc2 = m_GeoFilename.Find(".nc", 0);
      m_PglFilename = m_GeoFilename.Left(dotloc2);
      }

   CString subgridtxt;
   CString n3Dvaltxt;

   subgridtxt.Format("%d", nsubgrids);
   n3Dvaltxt.Format("%d", m_3DVal);

   CString outputfile = "\\Envision\\StudyAreas\\Eugene" + m_PglFilename + "_nsubgrid" + subgridtxt + "_3dval" + n3Dvaltxt + ".pgl";

   //const char * outfile = "\\Envision\\StudyAreas\\Eugene\\test.pgl";

   m_pPolyGridLkUp->WriteToFile(outputfile);

   //??????delete [] m_pafScanbandbool; //delete object where no data in data file for creating polygrid lookup file

   return TRUE;
   }
   */

//int GeoSpatialDataObj::Get3DCount()
//   {
//   return m_nBands;
//   }

int GeoSpatialDataObj::GetUTMZoneFromPrjWKT(LPCTSTR prjWKTFile)
   {
   int utm = -1;

   // esri standard for specifing UTM zone in .prj file				 			
   CString wktZoneTxt = "UTM_Zone_";

   int sizeWKTZoneTxt = wktZoneTxt.GetLength();

   CString projectionWKT = prjWKTFile;

   int loc = projectionWKT.Find(wktZoneTxt);

   int locationOfZone = loc + sizeWKTZoneTxt;

   // UTM zones are only two digits
   CString csUTM = projectionWKT.Mid(locationOfZone, 2);

   utm = atoi(csUTM);

   //UTM zones are only from 1-60
   if (utm < 1 || utm > 60)
      {
      CString msg;
      msg.Format("GeoSpatialDataObj::GetUTMZoneFromPrjWKT: could not find valid UTM Zone", utm);
      Report::ErrorMsg(msg);
      }

   return utm;
   }

float GeoSpatialDataObj::Get(double xcoord, double ycoord, int &twoDindexvalue, int set3Dvalue, CString prjFile, bool isUTM)
   {
   m_3DVal = set3Dvalue;

   int threeDindexvalue = 0;

   if (m_3DVal > m_nBands - 1)
      {
      CString msg;
      msg.Format(_T("GeoSpatialDataObj::ReadSpatialData set3Dvalue is greater than total number of bands in 3d raster file "));
      msg += m_GeoFilename;
      Report::ErrorMsg(msg);
      return -9999.0f;
      }

   int xx, yy;

   double vrtGeoTransform[6];  // uninitialized 

   double northing, easting, nextlatnorthing, nextlateasting, nextlonnorthing, nextloneasting, northingres;

   if (isUTM)
      {
      int utmzone = GetUTMZoneFromPrjWKT(prjFile);
      northing = ycoord;
      easting = xcoord;
      }
   else
      {
      double LongTemp = (m_UpperLeftLon + 180) - floor((m_UpperLeftLon + 180) / 360) * 360 - 180; // -180.00 .. 179.9;

      int utmzone = (int)floor((LongTemp + 180) / 6) + 1;

      m_gdal.LLtoUTM(22, m_UpperLeftLat, m_UpperLeftLon, northing, easting, utmzone);

      m_gdal.LLtoUTM(22, m_UpperLeftLat + m_CellLatDegResolution, m_UpperLeftLon, nextlatnorthing, nextlateasting, utmzone);

      m_gdal.LLtoUTM(22, m_UpperLeftLat, m_UpperLeftLon + m_CellLonDegResolution, nextlonnorthing, nextloneasting, utmzone);
      }

   //this transform is directly related to the clipped netCDF Data assumes data is northern hemisphere and image up
   //this can be made more generic for inverted images and southern hemisphere
   vrtGeoTransform[0] = easting;    /* top left x */
   if (isUTM)
      {
      vrtGeoTransform[1] = m_CellLonDegResolution;

      northingres = m_CellLatDegResolution;
      }
   else
      {
      vrtGeoTransform[1] = abs(easting - nextloneasting);    /* w-e pixel resolution 9668.3975*/

      if (northing - nextlatnorthing > 0)
         {
         northingres = (northing - nextlatnorthing)*-1; //resolution is negagtive
         }
      else
         {
         northingres = northing - nextlatnorthing;
         }
      }
   vrtGeoTransform[2] = m_imgNorthUp2;          /* rotation, 0 if image is "north up" */
   vrtGeoTransform[3] = northing;    /* top left y */
   vrtGeoTransform[4] = m_imgNorthUp4;          /* rotation, 0 if image is "north up" */
   vrtGeoTransform[5] = northingres;   /* n-s pixel resolution -13888.226*/

   double lat = 0;
   double lng = 0;
   bool ok = m_gdal.Coord2LatLong(xcoord, ycoord, m_nXSize, m_nYSize, m_DataType, vrtGeoTransform, prjFile, lat, lng);
   ASSERT(ok);  // should check better!!!!
   if (!ok)
      {
      CString msg;
      msg.Format(_T("  GeoSpatialDataObj: Error getting climate info for location (%.1f, %.1f); 2DIndex=%i, setDValue=%i", xcoord, ycoord, twoDindexvalue, set3Dvalue));
      Report::LogError(msg);
      return -9999.0f;
      }

   double m_geox = 0;
   double m_geoy = 0;

   double invGeoTransform[6];


   if (isUTM)
      {
      if (m_gdal.InvGeoTransform(m_geoTransform, invGeoTransform))
         m_gdal.ApplyGeoTransform(invGeoTransform, xcoord, ycoord /**(latlong+0),*(latlong+1)*/, &m_geox, &m_geoy);
      else
         {
         CString msg;
         msg.Format(_T("GeoSpatialDataObj::Get(double xcoord,double ycoord) InvGeoTransform failed"));
         Report::ErrorMsg(msg);
         return -9999.0f;
         }
      }
   else
      {
      if (m_gdal.InvGeoTransform(m_geoTransform, invGeoTransform))
         m_gdal.ApplyGeoTransform(invGeoTransform, lng, lat /**(latlong+0),*(latlong+1)*/, &m_geox, &m_geoy);
      else
         {
         CString msg;
         msg.Format(_T("GeoSpatialDataObj::Get(double xcoord,double ycoord) InvGeoTransform failed"));
         Report::ErrorMsg(msg);
         return -9999.0f;
         }
      }
   xx = (int)floor(m_geox);
   yy = (int)floor(m_geoy);

   ASSERT(xx <= m_nXSize && yy <= m_nYSize);
   if (xx < 0 || xx >= m_nXSize || yy < 0 || yy >= m_nYSize)
      {
      CString msg;
      msg.Format("GeospatialDataObject::Get(): Bad centroid (%g,%g) passed in", (float)xcoord, (float)ycoord);
      //Report::ErrorMsg(msg);
      return -9999.0f;
      }

   if (m_3DVal > 1)
      {
      threeDindexvalue = (m_3DVal + 1)*((m_nXSize*m_nYSize)) - (m_nXSize*(m_nYSize - yy - 1)) - (m_nXSize - xx);
      }
   else
      {
      threeDindexvalue = ((m_nXSize*m_nYSize)) - (m_nXSize*(m_nYSize - yy - 1)) - (m_nXSize - xx);
      }

   if (threeDindexvalue < 0 || threeDindexvalue >= m_nYSize * m_nXSize * m_nBands)
      {
      CString msg;
      msg.Format("GeospatialDataObject::Get(): Bad 3Dindex value (%i)", threeDindexvalue);
      Report::ErrorMsg(msg);
      return -9999.0f;
      }

   twoDindexvalue = ((m_nXSize*m_nYSize)) - (m_nXSize*(m_nYSize - yy - 1)) - (m_nXSize - xx);

   if (twoDindexvalue < 0 || twoDindexvalue >= m_nYSize * m_nXSize)
      {
      CString msg;
      msg.Format("GeospatialDataObject::Get(): Bad 2Dindex value (%i)", twoDindexvalue);
      Report::ErrorMsg(msg);
      return -9999.0f;
      }

   float *myband = (float*)(m_pafScanband);

   float value = myband[threeDindexvalue];

   return value;
   }

short int GeoSpatialDataObj::GetAsShortInt(double xcoord, double ycoord, int &twoDindexvalue, int set3Dvalue, CString prjFile, bool isUTM)
   {
   m_3DVal = set3Dvalue;

   int threeDindexvalue = 0;

   if (m_3DVal > m_nBands - 1)
      {
      CString msg;
      msg.Format(_T("GeoSpatialDataObj::ReadSpatialData set3Dvalue is greater than total number of bands in 3d raster file "));
      msg += m_GeoFilename;
      Report::ErrorMsg(msg);
      return -9999;
      }

   int xx, yy;

   double vrtGeoTransform[6];  // uninitialized 

   double northing, easting, nextlatnorthing, nextlateasting, nextlonnorthing, nextloneasting, northingres;

   if (isUTM)
      {
      int utmzone = GetUTMZoneFromPrjWKT(prjFile);
      northing = ycoord;
      easting = xcoord;
      }
   else
      {
      double LongTemp = (m_UpperLeftLon + 180) - floor((m_UpperLeftLon + 180) / 360) * 360 - 180; // -180.00 .. 179.9;

      int utmzone = (int)floor((LongTemp + 180) / 6) + 1;

      m_gdal.LLtoUTM(22, m_UpperLeftLat, m_UpperLeftLon, northing, easting, utmzone);

      m_gdal.LLtoUTM(22, m_UpperLeftLat + m_CellLatDegResolution, m_UpperLeftLon, nextlatnorthing, nextlateasting, utmzone);

      m_gdal.LLtoUTM(22, m_UpperLeftLat, m_UpperLeftLon + m_CellLonDegResolution, nextlonnorthing, nextloneasting, utmzone);
      }

   //this transform is directly related to the clipped netCDF Data assumes data is northern hemisphere and image up
   //this can be made more generic for inverted images and southern hemisphere
   vrtGeoTransform[0] = easting;    /* top left x */
   if (isUTM)
      {
      vrtGeoTransform[1] = m_CellLonDegResolution;

      northingres = m_CellLatDegResolution;
      }
   else
      {
      vrtGeoTransform[1] = abs(easting - nextloneasting);    /* w-e pixel resolution 9668.3975*/

      if (northing - nextlatnorthing > 0)
         {
         northingres = (northing - nextlatnorthing)*-1; //resolution is negagtive
         }
      else
         {
         northingres = northing - nextlatnorthing;
         }
      }
   vrtGeoTransform[2] = m_imgNorthUp2;          /* rotation, 0 if image is "north up" */
   vrtGeoTransform[3] = northing;    /* top left y */
   vrtGeoTransform[4] = m_imgNorthUp4;          /* rotation, 0 if image is "north up" */
   vrtGeoTransform[5] = northingres;   /* n-s pixel resolution -13888.226*/

   double lat = 0;
   double lng = 0;
   bool ok = m_gdal.Coord2LatLong(xcoord, ycoord, m_nXSize, m_nYSize, m_DataType, vrtGeoTransform, prjFile, lat, lng);
   ASSERT(ok);  // should check better!!!!

   double m_geox = 0;
   double m_geoy = 0;

   double invGeoTransform[6];


   if (isUTM)
      {
      if (m_gdal.InvGeoTransform(m_geoTransform, invGeoTransform))
         m_gdal.ApplyGeoTransform(invGeoTransform, xcoord, ycoord /**(latlong+0),*(latlong+1)*/, &m_geox, &m_geoy);
      else
         {
         CString msg;
         msg.Format(_T("GeoSpatialDataObj::Get(double xcoord,double ycoord) InvGeoTransform failed"));
         Report::ErrorMsg(msg);
         return -9999;
         }
      }
   else
      {
      if (m_gdal.InvGeoTransform(m_geoTransform, invGeoTransform))
         m_gdal.ApplyGeoTransform(invGeoTransform, lng, lat /**(latlong+0),*(latlong+1)*/, &m_geox, &m_geoy);
      else
         {
         CString msg;
         msg.Format(_T("GeoSpatialDataObj::Get(double xcoord,double ycoord) InvGeoTransform failed"));
         Report::ErrorMsg(msg);
         return -9999;
         }
      }
   xx = (int)floor(m_geox);
   yy = (int)floor(m_geoy);

   ASSERT(xx < m_nXSize && yy < m_nYSize);
   if (xx < 0 || xx >= m_nXSize || yy < 0 || yy >= m_nYSize)
      {
      CString msg;
      msg.Format("GeospatialDataObject::Get(): Bad centroid (%g,%g) passed in", (float)xcoord, (float)ycoord);
      Report::ErrorMsg(msg);
      return -9999;
      }

   if (m_3DVal > 1)
      {
      threeDindexvalue = (m_3DVal + 1)*((m_nXSize*m_nYSize)) - (m_nXSize*(m_nYSize - yy - 1)) - (m_nXSize - xx);
      }
   else
      {
      threeDindexvalue = ((m_nXSize*m_nYSize)) - (m_nXSize*(m_nYSize - yy - 1)) - (m_nXSize - xx);
      }

   if (threeDindexvalue < 0 || threeDindexvalue >= m_nYSize * m_nXSize * m_nBands)
      {
      CString msg;
      msg.Format("GeospatialDataObject::Get(): Bad 3Dindex value (%i)", threeDindexvalue);
      Report::ErrorMsg(msg);
      return -9999;
      }

   twoDindexvalue = ((m_nXSize*m_nYSize)) - (m_nXSize*(m_nYSize - yy - 1)) - (m_nXSize - xx);

   if (twoDindexvalue < 0 || twoDindexvalue >= m_nYSize * m_nXSize)
      {
      CString msg;
      msg.Format("GeospatialDataObject::Get(): Bad 2Dindex value (%i)", twoDindexvalue);
      Report::ErrorMsg(msg);
      return -9999;
      }

   short int *myband = (short int*)(m_pafScanband);

   short int value = myband[threeDindexvalue];

   return value;
   }

float GeoSpatialDataObj::Get(double xcoord, double ycoord, int &row, int &col, int &twoDindexvalue, int set3Dvalue, CString prjFile)
   {
   m_3DVal = set3Dvalue;

   int indexvalue = 0;

   if (m_3DVal > m_nBands - 1)
      {
      CString msg;
      msg.Format(_T("GeoSpatialDataObj::ReadSpatialData set3Dvalue is greater than total number of bands in 3d raster file "));
      msg += m_GeoFilename;
      Report::ErrorMsg(msg);
      return -9999.0f;
      }

   int xx, yy;

   double vrtGeoTransform[6];

   double LongTemp = (m_UpperLeftLon + 180) - floor((m_UpperLeftLon + 180) / 360) * 360 - 180; // -180.00 .. 179.9;

   double northing, easting, nextlatnorthing, nextlateasting, nextlonnorthing, nextloneasting, northingres;

   int utmzone = (int)floor((LongTemp + 180) / 6) + 1;

   m_gdal.LLtoUTM(22, m_UpperLeftLat, m_UpperLeftLon, northing, easting, utmzone);

   m_gdal.LLtoUTM(22, m_UpperLeftLat + m_CellLatDegResolution, m_UpperLeftLon, nextlatnorthing, nextlateasting, utmzone);

   m_gdal.LLtoUTM(22, m_UpperLeftLat, m_UpperLeftLon + m_CellLonDegResolution, nextlonnorthing, nextloneasting, utmzone);

   //this transform is directly related to the clipped netCDF Data assumes data is northern hemisphere and image up
   //this can be made more generic for inverted images and southern hemisphere
   vrtGeoTransform[0] = easting;    /* top left x */
   vrtGeoTransform[1] = abs(easting - nextloneasting);    /* w-e pixel resolution 9668.3975*/
   vrtGeoTransform[2] = m_imgNorthUp2;          /* rotation, 0 if image is "north up" */
   vrtGeoTransform[3] = northing;    /* top left y */
   vrtGeoTransform[4] = m_imgNorthUp4;          /* rotation, 0 if image is "north up" */

   if (northing - nextlatnorthing > 0)
      {
      northingres = -(northing - nextlatnorthing); //resolution is negative
      }
   else
      {
      northingres = northing - nextlatnorthing;
      }

   vrtGeoTransform[5] = northingres;   /* n-s pixel resolution -13888.226*/

   //double *latlong = 
   double lat = 0;
   double lng = 0;
   bool ok = m_gdal.Coord2LatLong(xcoord, ycoord, m_nXSize, m_nYSize, m_DataType, vrtGeoTransform, prjFile, lat, lng);

   m_geox = 0;
   m_geoy = 0;

   double invGeoTransform[6];

   if (m_gdal.InvGeoTransform(m_geoTransform, invGeoTransform))
      {
      m_gdal.ApplyGeoTransform(invGeoTransform, lng, lat, &m_geox, &m_geoy);
      }
   else
      {
      CString msg;
      msg.Format(_T("GeoSpatialDataObj::Get(double xcoord,double ycoord) InvGeoTransform failed"));
      Report::ErrorMsg(msg);
      return -9999.0f;
      }

   //double jlat = *(latlong+0);   /// SOMETHING WRONG WITH ORDER HERE!~!!!!
   //double jlon = *(latlong+1);

   xx = int(floor(m_geox));
   yy = int(floor(m_geoy));

   row = yy;
   col = xx;

   if (xx > m_nXSize - 1 || yy > m_nYSize - 1)
      {
      //msg.Format( _T("GeoSpatialDataObj::Get(double xcoord,double ycoord) index greater than x or y size of input data file"));
      //Report::ErrorMsg( msg );
      //return false;
      ASSERT(xx > m_nXSize - 1 || yy > m_nYSize - 1);
      }
   else
      {
      int indexvalue = (m_3DVal + 1)*((m_nXSize*m_nYSize)) - (m_nXSize*(m_nYSize - yy - 1)) - (m_nXSize - xx);

      twoDindexvalue = ((m_nXSize*m_nYSize)) - (m_nXSize*(m_nYSize - yy - 1)) - (m_nXSize - xx);

      float *myband = (float *)(m_pafScanband);

      float value = myband[indexvalue];

      return value;
      }

   return -9999.0f;
   }


float GeoSpatialDataObj::Get(int &twoDindexvalue, int set3Dvalue)
   {
   int index = (set3Dvalue * (m_nXSize*m_nYSize)) + twoDindexvalue;

   if (index < 0 || index >= m_nYSize * m_nXSize * m_nBands)
      {
      CString msg;
      msg.Format("GeospatialDataObject::Get(): Bad 3Dindex value (%i)", index);
      Report::ErrorMsg(msg);
      return -9999.0f;
      }

   float *myband = (float *)(m_pafScanband);
   float value = myband[index];
   return value;
   }


float GeoSpatialDataObj::Getbyrowcol(int col, int row, int set3Dvalue)
   {
   m_3DVal = set3Dvalue;

   if (m_3DVal > m_nBands - 1)
      {
      CString msg;
      msg.Format(_T("GeoSpatialDataObj::ReadSpatialData set3Dvalue is greater than total number of bands in 3d raster file "));
      msg += m_GeoFilename;
      Report::ErrorMsg(msg);
      return -9999.0f;
      }

   int index = (m_3DVal + 1)*((m_nXSize*m_nYSize)) - (m_nXSize*(m_nYSize - row - 1)) - (m_nXSize - col);

   if (index < 0 || index >= m_nYSize * m_nXSize * m_nBands)
      {
      CString msg;
      msg.Format("GeospatialDataObject::Getbyrowcol(): Bad 3Dindex value (%i)", index);
      Report::ErrorMsg(msg);
      return -9999.0f;
      }

   float *myband = (float *)(m_pafScanband);

   float value = myband[index];

   return value;
   }


bool GeoSpatialDataObj::Get(int col, int row, float &value)
   {
   int index = (m_3DVal + 1)*((m_nXSize*m_nYSize)) - (m_nXSize*(m_nYSize - row - 1)) - (m_nXSize - col);

   if (index < 0 || index >= m_nYSize * m_nXSize * m_nBands)
      {
      CString msg;
      msg.Format("GeospatialDataObject::Get(): Bad 3Dindex value (%i)", index);
      Report::ErrorMsg(msg);
      return false;
      }

   float *myband = (float *)(m_pafScanband);

   value = myband[index];
   return true;
   }


bool GeoSpatialDataObj::Get(int col, int row, double &value)
   {
   int index = (m_3DVal + 1)*((m_nXSize*m_nYSize)) - (m_nXSize*(m_nYSize - row - 1)) - (m_nXSize - col);
   if (index < 0 || index >= m_nYSize * m_nXSize * m_nBands)
      {
      CString msg;
      msg.Format("GeospatialDataObject::Get(): Bad 3Dindex value (%i)", index);
      Report::ErrorMsg(msg);
      return false;
      }

   double *myband = (double *)(m_pafScanband);

   value = myband[index];

   return true;
   }

bool GeoSpatialDataObj::Get(int col, int row, COleVariant &v)
   {
   int index = (m_3DVal + 1)*((m_nXSize*m_nYSize)) - (m_nXSize*(m_nYSize - row - 1)) - (m_nXSize - col);
   if (index < 0 || index >= m_nYSize * m_nXSize * m_nBands)
      {
      CString msg;
      msg.Format("GeospatialDataObject::Get(): Bad 3Dindex value (%i)", index);
      Report::ErrorMsg(msg);
      return false;
      }

   float *myband = (float *)(m_pafScanband);

   v = myband[index];

   return true;
   }


bool GeoSpatialDataObj::Get(int col, int row, VData &v)
   {
   int index = (m_3DVal + 1)*((m_nXSize*m_nYSize)) - (m_nXSize*(m_nYSize - row - 1)) - (m_nXSize - col);

   if (index < 0 || index >= m_nYSize * m_nXSize * m_nBands)
      {
      CString msg;
      msg.Format("GeospatialDataObject::Get(): Bad 3Dindex value (%i)", index);
      Report::ErrorMsg(msg);
      return false;
      }

   float *myband = (float *)(m_pafScanband);

   v = myband[index];

   return true;
   }

bool GeoSpatialDataObj::Get(int col, int row, int &v)
   {
   int index = (m_3DVal + 1)*((m_nXSize*m_nYSize)) - (m_nXSize*(m_nYSize - row - 1)) - (m_nXSize - col);

   if (index < 0 || index >= m_nYSize * m_nXSize * m_nBands)
      {
      CString msg;
      msg.Format("GeospatialDataObject::Get(): Bad 3Dindex value (%i)", index);
      Report::ErrorMsg(msg);
      return false;
      }

   int *myband = (int*)(m_pafScanband);

   v = myband[index];

   return true;
   }

bool GeoSpatialDataObj::Get(int col, int row, short &v)
   {
   int index = (m_3DVal + 1)*((m_nXSize*m_nYSize)) - (m_nXSize*(m_nYSize - row - 1)) - (m_nXSize - col);

   if (index < 0 || index >= m_nYSize * m_nXSize * m_nBands)
      {
      CString msg;
      msg.Format("GeospatialDataObject::Get(): Bad 3Dindex value (%i)", index);
      Report::ErrorMsg(msg);
      return false;
      }

   short *myband = (short*)(m_pafScanband);

   v = myband[index];

   return true;
   }




bool GeoSpatialDataObj::Get(int col, int row, bool &) { return false; }

int GeoSpatialDataObj::Find(int col, VData &value, int startRecord)
   {

   return -1;
   }

bool GeoSpatialDataObj::Set(int col, int row, COleVariant &value)
   {

   return false;
   }

bool GeoSpatialDataObj::Set(int col, int row, float value)
   {

   int indexvalue = (m_3DVal + 1)*((m_nXSize*m_nYSize)) - (m_nXSize*(m_nYSize - row - 1)) - (m_nXSize - col);

   float *myband = (float *)(m_pafScanband);

   *(myband + indexvalue) = value;

   //(float *) m_pafScanband = myband;

   return true;

   }

bool GeoSpatialDataObj::Set(int col, int row, double value)
   {
   int indexvalue = (m_3DVal + 1)*((m_nXSize*m_nYSize)) - (m_nXSize*(m_nYSize - row - 1)) - (m_nXSize - col);

   double *myband = (double *)(m_pafScanband);

   *(myband + indexvalue) = value;

   //(double *) m_pafScanband = myband;

   return true;

   }

bool GeoSpatialDataObj::Set(int col, int row, int value)
   {
   int indexvalue = (m_3DVal + 1)*((m_nXSize*m_nYSize)) - (m_nXSize*(m_nYSize - row - 1)) - (m_nXSize - col);

   int *myband = (int *)(m_pafScanband);

   *(myband + indexvalue) = value;

   //(int *) m_pafScanband = myband;

   return true;
   }

bool GeoSpatialDataObj::Set(int col, int row, const VData &value)
   {

   return false;
   }

bool GeoSpatialDataObj::Set(int col, int row, LPCTSTR value)
   {

   return false;
   }

void GeoSpatialDataObj::Clear(void)
   {
   //????
   DataObj::Clear();  // clear parent
   if (m_pafScanband)
      {
      delete m_pafScanband;
      m_pafScanband = NULL;
      }
   }

//- set size: no Append() required, data uninitialized --//
bool GeoSpatialDataObj::SetSize(int _cols, int _rows)
   {
   return false;
   }

//-- various gets --//
int GeoSpatialDataObj::GetRowCount(void)
   {
   return m_nYSize;

   }

int GeoSpatialDataObj::GetColCount(void)
   {
   return m_nXSize;

   }

VARTYPE GeoSpatialDataObj::GetOleType(int /*col*/, int /*row=0*/)
   {
   return VT_NULL;
   }

TYPE GeoSpatialDataObj::GetType(int col, int row)
   {
   switch (m_DataType)
      {
      case GDT_Unknown:

         return TYPE_NULL;

         break;

      case GDT_Byte:
         return TYPE_CHAR;

         break;

      case GDT_UInt16:
         return TYPE_UINT;

         break;

      case GDT_Int16:
         return TYPE_SHORT;

         break;

      case GDT_UInt32:
         return TYPE_UINT;

         break;

      case GDT_Int32:
         return TYPE_LONG;

         break;

      case GDT_Float32:
         return TYPE_FLOAT;

         break;

      case GDT_Float64:
         return TYPE_DOUBLE;

         break;

      case GDT_CInt16:
         return TYPE_NULL;

         break;

      case GDT_CInt32:
         return TYPE_NULL;

         break;

      case GDT_CFloat32:
         return TYPE_NULL;

         break;

      case GDT_CFloat64:
         return TYPE_NULL;

         break;

      default: // ignore everything else
         return TYPE_NULL;
         break;
      }

   /*enum TYPE      // various data type flags
   {
   TYPE_NULL    = 0,
   TYPE_CHAR    = 1,
   TYPE_BOOL    = 2,
   TYPE_SHORT   = 3,
   TYPE_UINT    = 4,
   TYPE_INT     = 5,
   TYPE_ULONG   = 6,
   TYPE_LONG    = 7,
   TYPE_FLOAT   = 8,
   TYPE_DOUBLE  = 9,
   TYPE_PTR     = 10,  // void ptr
   TYPE_STRING  = 11,  // NULL-terminated string, statically allocated
   TYPE_DSTRING = 12,  // NULL-terminated string, dynamically allocated (new'ed)
   TYPE_CSTRING = 13,  // pointer to a MFC CString object
   TYPE_DATAOBJ = 14,  // pointer to a data object
   TYPE_DATE    = 15   // DATE type (float)

typedef enum {
    ! Unknown or unspecified type            GDT_Unknown = 0,
    ! Eight bit unsigned integer            GDT_Byte = 1,
    ! Sixteen bit unsigned integer          GDT_UInt16 = 2,
    ! Sixteen bit signed integer            GDT_Int16 = 3,
    ! Thirty two bit unsigned integer       GDT_UInt32 = 4,
    ! Thirty two bit signed integer         GDT_Int32 = 5,
    ! Thirty two bit floating point         GDT_Float32 = 6,
    ! Sixty four bit floating point         GDT_Float64 = 7,
    ! Complex Int16                         GDT_CInt16 = 8,
    ! Complex Int32                         GDT_CInt32 = 9,
    ! Complex Float32                       GDT_CFloat32 = 10,
    ! Complex Float64                       GDT_CFloat64 = 11,
    GDT_TypeCount = 12          /maximum type # + 1
} GDALDataType;
   };*/
   return TYPE_NULL;
   }

float GeoSpatialDataObj::GetAsFloat(int col, int row, int time)
   {

   CString msg;

   int my3Dval = time;

   if (col > m_nXSize - 1 || row > m_nYSize - 1 || my3Dval > m_3DVal - 1)
      {
      msg.Format(_T("GeoSpatialDataObj::GetAsFloat row, column, or time index exceeds input raster x or y size"));
      Report::ErrorMsg(msg);
      return -9999.0f;
      }
   else
      {

      int indexvalue = (my3Dval + 1)*((m_nXSize*m_nYSize)) - (m_nXSize*(m_nYSize - row - 1)) - (m_nXSize - col);

      float *myband = (float *)(m_pafScanband);

      float value = *(myband + indexvalue);

      return value;

      }

   }

float   GeoSpatialDataObj::GetAsFloat(int col, int row)
   {

   CString msg;

   if (col > m_nXSize - 1 || row > m_nYSize - 1)
      {
      msg.Format(_T("GeoSpatialDataObj::GetAsFloat row or column index exceeds input raster x or y size"));
      Report::ErrorMsg(msg);
      return -9999.0f;
      }
   else
      {

      int indexvalue = (m_3DVal + 1)*((m_nXSize*m_nYSize)) - (m_nXSize*(m_nYSize - row - 1)) - (m_nXSize - col);

      float *myband = (float *)(m_pafScanband);

      float value = *(myband + indexvalue);

      return value;

      }

   }

double  GeoSpatialDataObj::GetAsDouble(int col, int row)
   {
   int indexvalue = (m_3DVal + 1)*((m_nXSize*m_nYSize)) - (m_nXSize*(m_nYSize - row - 1)) - (m_nXSize - col);

   double *myband = (double *)(m_pafScanband);

   double value = *(myband + indexvalue);

   return value;
   }

int     GeoSpatialDataObj::GetAsInt(int col, int row)
   {
   int indexvalue = (m_3DVal + 1)*((m_nXSize*m_nYSize)) - (m_nXSize*(m_nYSize - row - 1)) - (m_nXSize - col);

   int *myband = (int *)(m_pafScanband);

   int value = *(myband + indexvalue);

   return value;
   }

UINT GeoSpatialDataObj::GetAsUInt(int col, int row)
   {
   int value = GetAsInt(col, row);

   return (UINT)value;
   }


void GeoSpatialDataObj::BuildDataObj(char *inputarr, char nodataval, bool *targetboolarray, int rasterxyzsize)
   {
   //boolean of true false that lets polygrid lookup know where there is a no data value

   for (int i = 0; i < m_nXSize*m_nYSize*m_nBands; i++)
      {
      if (*(inputarr + i) == nodataval) *(targetboolarray + i) = false; //where no data value make false at same location in bool for use in polygridlookups

      }

   }

void GeoSpatialDataObj::BuildDataObj(unsigned short int *inputarr, unsigned short int nodataval, bool *targetboolarray, int rasterxyzsize)
   {
   //boolean of true false that lets polygrid lookup know where there is a no data value


   for (int i = 0; i < m_nXSize*m_nYSize*m_nBands; i++)
      {
      if (*(inputarr + i) == nodataval) *(targetboolarray + i) = false; //where no data value make false at same location in bool for use in polygridlookups

      }
   }

void GeoSpatialDataObj::BuildDataObj(short int *inputarr, short int nodataval, bool *targetboolarray, int rasterxyzsize)
   {
   //boolean of true false that lets polygrid lookup know where there is a no data value

   for (int i = 0; i < m_nXSize*m_nYSize*m_nBands; i++)
      {
      if (*(inputarr + i) == nodataval) *(targetboolarray + i) = false; //where no data value make false at same location in bool for use in polygridlookups

      }

   }

void GeoSpatialDataObj::BuildDataObj(complex< short int > *inputarr, complex< short int > nodataval, bool *targetboolarray, int rasterxyzsize)
   {
   //boolean of true false that lets polygrid lookup know where there is a no data value

   for (int i = 0; i < m_nXSize*m_nYSize*m_nBands; i++)
      if (inputarr[i] == nodataval)
         targetboolarray[i] = false; //where no data value make false at same location in bool for use in polygridlookups
   }

void GeoSpatialDataObj::BuildDataObj(unsigned long int *inputarr, unsigned long int nodataval, bool *targetboolarray, int rasterxyzsize)
   {
   //boolean of true false that lets polygrid lookup know where there is a no data value

   for (int i = 0; i < m_nXSize*m_nYSize*m_nBands; i++)
      {
      if (*(inputarr + i) == nodataval) *(targetboolarray + i) = false; //where no data value make false at same location in bool for use in polygridlookups

      }

   }

void GeoSpatialDataObj::BuildDataObj(complex< long int > *inputarr, complex< long int > nodataval, bool *targetboolarray, int rasterxyzsize)
   {
   //boolean of true false that lets polygrid lookup know where there is a no data value

   for (int i = 0; i < m_nXSize*m_nYSize*m_nBands; i++)
      {
      if (*(inputarr + i) == nodataval) *(targetboolarray + i) = false; //where no data value make false at same location in bool for use in polygridlookups

      }

   }

void GeoSpatialDataObj::BuildDataObj(long int *inputarr, long int nodataval, bool *targetboolarray, int rasterxyzsize)
   {
   //boolean of true false that lets polygrid lookup know where there is a no data value

   for (int i = 0; i < m_nXSize*m_nYSize*m_nBands; i++)
      {
      if (*(inputarr + i) == nodataval) *(targetboolarray + i) = false; //where no data value make false at same location in bool for use in polygridlookups

      }

   }

void GeoSpatialDataObj::BuildDataObj(float *inputarr, float nodataval, bool *targetboolarray, int rasterxyzsize)
   {
   //boolean of true false that lets polygrid lookup know where there is a no data value

   for (int i = 0; i < m_nXSize*m_nYSize*m_nBands; i++)
      {
      if (*(inputarr + i) == nodataval) *(targetboolarray + i) = false; //where no data value make false at same location in bool for use in polygridlookups

      }
   }

void GeoSpatialDataObj::BuildDataObj(complex< float > *inputarr, complex< float > nodataval, bool *targetboolarray, int rasterxyzsize)
   {
   //boolean of true false that lets polygrid lookup know where there is a no data value

   for (int i = 0; i < m_nXSize*m_nYSize*m_nBands; i++)
      {
      if (*(inputarr + i) == nodataval) *(targetboolarray + i) = false; //where no data value make false at same location in bool for use in polygridlookups

      }

   }

void GeoSpatialDataObj::BuildDataObj(double *inputarr, double nodataval, bool *targetboolarray, int rasterxyzsize)
   {
   //boolean of true false that lets polygrid lookup know where there is a no data value

   for (int i = 0; i < m_nXSize*m_nYSize*m_nBands; i++)
      {
      if (*(inputarr + i) == nodataval) *(targetboolarray + i) = false; //where no data value make false at same location in bool for use in polygridlookups

      }

   }

void GeoSpatialDataObj::BuildDataObj(complex< double > *inputarr, complex< double > nodataval, bool *targetboolarray, int rasterxyzsize)
   {
   //boolean of true false that lets polygrid lookup know where there is a no data value

   for (int i = 0; i < m_nXSize*m_nYSize*m_nBands; i++)
      {
      if (*(inputarr + i) == nodataval) *(targetboolarray + i) = false; //where no data value make false at same location in bool for use in polygridlookups

      }

   }

