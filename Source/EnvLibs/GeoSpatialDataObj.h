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

#include "GDALWrapper.h"

#include "Maplayer.h"
#include "FDATAOBJ.H"
//#include "PolyGridLookups.h"
#include <algorithm>
#include <map>
#include <iostream>
#include <memory>
#include <new>
#include <complex>

//#include <EnvContext.h>
//#include <EnvExtension.h>

using namespace std;

#if !defined _GEOSPATIALDATAOBJ_H
#define      _GEOSPATIALDATAOBJ_H 1

#if !defined _DATAOBJ_H
#include "DATAOBJ.H"
#endif

//enum GRIDPOLY_METHOD
//   {
//   CONTINUOUS_VAR =  0,  //proportions
//   CATEGORICAL_VAL = 1,  //plurality
//   };

class LIBSAPI GeoSpatialDataObj : public DataObj
{
public:
	  GeoSpatialDataObj (UNIT_MEASURE m);
	
	   //LPCTSTR variable="" for no varible name
	  bool Open(LPCTSTR filename , LPCTSTR variable);
	  bool InitLibraries();
	  //set3Dvalue 0 to n3Dcount-1
	  bool  ReadSpatialData(void); 
	  
	  //CString variable="" for no varible name, set3Dvalue 0 to n3Dcount-1
	  //bool  ReadSpatialData(CString filename , CString variable, float set3Dvalue); 
	
	  bool Close(void);
	  
     int   Get3DCount( void ) { return m_nBands; }

	  double GetNoDataVal(void);

	  //Call ReadSpatialData first.  Loads grid2poly and poly2grid lookup created from data input file and writes a polygrid lookup .pgl file
	  //bool  LoadPolyGridLookup(MapLayer*,int subgrids ); 

	  //loads grid2poly and poly2grid lookup from a polygrid lookup .pgl file
	 // bool  LoadPolyGridLookup(LPCTSTR ); 
		
	  //bool  SetPoly( int colname,int poly, GRIDPOLY_METHOD);
	  		
	  //-- destructor --//
      virtual ~GeoSpatialDataObj (void); 

      //-----NON POLYGRID 2D LOOKUP FUNCTIONS--- pure virtual functions ------------------//
      virtual DO_TYPE GetDOType() { return DOT_GEOSPATIAL; };

      //-- get element --//
	  
	  //inputfile georeferenced image, WKT or StudyArea prj file, and outputFile is Geotiff file transformed and warped image.
	  bool GeoImage2GeoTransWarpImage(LPCTSTR inputFile, LPCTSTR outputFile, LPCTSTR prjWKTFile); //inputfile georeferenced image, WKT or StudyArea prj file, and outputFile is Geotiff file transformed and warped image
	  bool GeoImage2GeoTransWarpImage(LPCTSTR inputFile, LPCTSTR outputFile, LPCTSTR studyAreaWKTFile, LPCTSTR inFileWKTFile, float xResolution, float yResolution, float xUpperLeft,float yUpperleft);
	  //inputfile georeferenced netCDF file, return pointer of transformed and warped data object of xSize*ySize. 1st element lower left corner.
	  //netCDF file name should have the form: "NETCDF:\\datafile.nc:Prcp";
	  float* NetCDF2GeoTransWarpRaster(LPCTSTR inputFile, LPCTSTR prjWKTFile,int set3Dvalue, int &xSize, int &ySize);
	  
	  //inputfile georeferenced netCDF file, and output file is Geotiff transformed and warped image.
	  //netCDF file name should have the form: "NETCDF:\\datafile.nc:Prcp";
	  bool NetCDF2GeoTransWarpImage(LPCTSTR inputFile, LPCTSTR outputFile, LPCTSTR poprjWKTFile,int set3Dvalue);

	  int GetUTMZoneFromPrjWKT( LPCTSTR prjWKTFile );
	  
	  float Get(double xcoord,double ycoord,int &twoDindexvalue, int set3Dvalue,CString prjFile, bool isUTM );
	  short int GetAsShortInt(double xcoord,double ycoord,int &twoDindexvalue, int set3Dvalue,CString prjFile, bool isUTM );
      float Get(double xcoord,double ycoord,int &row, int &col, int &twoDindexvalue, int set3Dvalue,CString prjFile);
      float Get(int &twoDindexvalue, int set3Dvalue );
	  float Getbyrowcol( int col, int row, int set3Dvalue );
      virtual bool Get( int col, int row, float &value);
      virtual bool Get( int col, int row, double &value);
      virtual bool Get( int col, int row, COleVariant& );
      virtual bool Get( int col, int row, VData &      );
      virtual bool Get( int col, int row, int &        );
      virtual bool Get( int col, int row, short &);
      virtual bool Get( int col, int row, bool &       );
      virtual bool Get( int col, int row, DataObj*&  ) { return false; } // only implemented by VDataObj; 
      virtual bool Get( int col, int row, CString &  ) { return false; } // only implemented by VDataObj; 

      virtual int Find( int col, VData &value, int startRecord=-1 );
      virtual bool Set( int col, int row, COleVariant &value );
      virtual bool Set( int col, int row, float value        );
      virtual bool Set( int col, int row, double value       );
      virtual bool Set( int col, int row, int value          );
      virtual bool Set( int col, int row, const VData &value       );
      virtual bool Set( int col, int row, LPCTSTR value       );

      virtual void Clear( void );

      //- set size: no Append() required, data uninitialized --//
      virtual bool SetSize( int _cols, int _rows );

      virtual bool GetMinMax( int col, float *minimum, float *maximum, int startRow=0 ){return false;} //need to account for nodata values

      //-- concatenation --//
      virtual int  AppendCols( int count  ){return 0;}
      virtual int  AppendCol( LPCTSTR name ){return 0;}
      virtual bool CopyCol( int toCol, int fromCol ){return false;}
      virtual int  InsertCol( int insertBefore, LPCTSTR label ){return 0;}

      virtual int  AppendRow( VData *array, int length ){return -1;};   // these always work, subclasses can use specialized types
      virtual int  AppendRow( COleVariant *array, int length ) { return -1;};
      virtual int  AppendRows( int count ){return 0;};
      virtual int  DeleteRow( int row ){return 0;};

      //-- various gets --//
      virtual int GetRowCount( void );
      virtual int GetColCount( void );
      
      virtual VARTYPE GetOleType(  int /*col*/, int /*row=0*/ );// underconstruction
      virtual TYPE    GetType   ( int col, int row );

      virtual float   GetAsFloat ( int col, int row  );
		virtual float   GetAsFloat(int col, int row, int time);
      virtual double  GetAsDouble( int col, int row  );
	   virtual CString GetAsString( int col, int row  ){ return CString( "" );} 
      virtual int     GetAsInt   ( int col, int row  );
      virtual UINT    GetAsUInt  ( int col, int row  );
      virtual bool    GetAsBool  (int col, int row   ) { return GetAsInt(col,row) == 1 ? true : false; }

      //-- File I/O --//
	  virtual int ReadAscii ( LPCTSTR fileName, char delimiter=',', BOOL showMsg=TRUE ){return 0;}
      virtual int ReadAscii ( FILE *fp,        char delimiter=',', BOOL showMsg=TRUE ){return 0;}
	  virtual int WriteAscii( LPCTSTR fileName, char delimiter=',', int colWidth=0    ){return 0;}

protected:
	void BuildDataObj(char *inputarr,char nodataval, bool *boolarray,int rasterxysize);	
	void BuildDataObj(unsigned short int *inputarr,unsigned short int nodataval, bool *boolarray, int rasterxysize);
	void BuildDataObj(short int *inputarr,short int nodataval, bool *boolarray,int rasterxysize);	
	void BuildDataObj(std::complex< short int > *inputarr,std::complex< short int > nodataval, bool *boolarray,int rasterxysize);	
	void BuildDataObj(unsigned long int *inputarr,unsigned long int nodataval, bool *boolarray,int rasterxysize);	
	void BuildDataObj(std::complex< long int > *inputarr,std::complex< long int > nodataval, bool *boolarray,int rasterxysize);	
	void BuildDataObj(long int *inputarr,long int nodataval, bool *boolarray,int rasterxysize);	
	void BuildDataObj(float *inputarr,float nodataval, bool *boolarray,int rasterxysize);	
	void BuildDataObj(std::complex< float > *inputarr,std::complex< float > nodataval, bool *boolarray,int rasterxysize);	
	void BuildDataObj(double *inputarr,double nodataval, bool *boolarray,int rasterxysize);	
	void BuildDataObj(std::complex< double > *inputarr,std::complex< double > nodataval, bool *boolarray,int rasterxysize);	
	
	GDALDatasetH m_datasetH;
	GDALDriverH  m_hDriver;
	GDALRasterBandH m_hBand;
	GDALWrapper m_gdal;
	GDALDataType m_DataType;

	//GRIDPOLY_METHOD m_gridpoly_method; 
	
	CString
		m_GeoFilename,
		m_PglFilename;

	void *m_pafScanband;

	double 
		m_CellLatDegResolution,
		m_CellLonDegResolution,
		m_UpperLeftLat,
		m_UpperLeftLon,
		m_geoTransform[ 6 ],
		m_invGeoTransform[ 6 ],
		m_geoy,    
		m_geox,
		m_imgNorthUp2,
		m_imgNorthUp4;
			
	int 
		m_nXSize,
	    m_nYSize,
		m_nBuffXSize,
		m_nBuffYSize,
		m_CellDim,
		m_nBands,
		m_RasterXYZSize,
		m_3DVal;

	bool
		*m_pafScanbandbool;

	double
		m_noDataValue;
	
	Map *m_pMap;
	
	//MapLayer
   //   *m_pPolyMapLayer,
   //   *m_pGridMapLayer;

	//PolyGridLookups
   //   *m_pPolyGridLkUp;
	
};
#endif
