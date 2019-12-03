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
// WET_Hydro.h: interface for the WET_Hydro class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WET_HYDRO_H__C9203D32_B4B1_11D3_95C1_00A076B0010A__INCLUDED_)
#define AFX_WET_HYDRO_H__C9203D32_B4B1_11D3_95C1_00A076B0010A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxtempl.h>
class DataTable;
//#include <DataStore.h>

#include <maplayer.h>   // for RowColArray;
#include <reachtree.h>
#include <fdataobj.h>
#include <vdataobj.h>
//#include <nr.h>
//#include <nrutil.h>
#include <randgen\randunif.hpp>
#include <PtrArray.h>
//#include <map3d/avigenerator.h>

enum WH_NOTIFY_TYPE { WH_UPDATE, WH_STARTRUN, WH_ENDRUN, WH_UPDATE_MONTE_CARLO, WH_ALLOCATEOBJECTS, WH_BUILDTOPOLOGY, WH_UPDATERADARRAIN};

class WET_Hydro;
typedef bool (*WH_NOTIFYPROC)( WET_Hydro*, WH_NOTIFY_TYPE, float, float, long, float );

const int WH_DISTRIBUTE_WATERSHEDS =           0;
const int WH_DISTRIBUTE_HILLRIPARIAN =         1;
const int WH_DISTRIBUTE_CENTROIDDISTANCEBINS = 2;
const int WH_DISTRIBUTE_BUFFERDISTANCEBINS   = 3;

const int WH_DISTRIBUTE_GEOMORPH              =4;
const int WH_DISTRIBUTE_LUMPED                =5;
const int WH_DISTRIBUTE_FPGRID                =6; 
const int WH_DISTRIBUTE_WTHEIGHT              =7;

const int WH_PARAM_WATERSHEDS      =            0;
const int WH_PARAM_HILLRIPARIAN     =           1;
const int WH_PARAM_GEOMORPHOLOGY =              2;
const int WH_PARAM_GEOMHILLRIP =                3;
const int WH_PARAM_SOILS =                      4;

const int ET_HARGREAVES = 0;
const int ET_PENMAN     = 1;
const int ET_PENMAN_MONTEITH = 2;
const int ET_MEASURED = 3;
const int UNSAT_VANGEN = 0;
const int UNSAT_BROOKSCOREY= 1;
const int SAT_TOPMODEL = 0;
const int SAT_SEIBERTMCD= 1;
const int DRAIN_P_CONSTANT = 0;
const int DRAIN_P_EXPONENTIAL = 1;

const int WH_NETWORK_SUM = 0;  // instream routing is a simple sum (assumed instantaneous)
const int WH_NETWORK_EXPLICIT = 1; //  instream routing is distributed.

const  int WH_SINGLE_SIMULATION = 0;
const  int WH_MONTE_CARLO = 1;

enum STATISTIC { MEAN=1, MAX=2 };

typedef float REAL; 

class StreamCellIndexArray : public CArray< CUIntArray, CUIntArray& >
   {

   };
class FPDistributionArray : public CArray< CUIntArray, CUIntArray& >
   {

   };
class AirTempArray : public CArray< CUIntArray, CUIntArray& >
   {

   };


struct DYNAMIC_MAP_INFO
	{
   CString fieldname;
	float minVal;
	float maxVal;
   DYNAMIC_MAP_INFO( void ) : fieldname(_T("")),minVal(0.5f), maxVal(1.0f) {}
	};

struct SOIL_INFO
   {
   CString   texture;
   CString   abbrev;
   float  c;
   float  phi;
   float  kSat;
   float  wiltPt;
   float  fieldCapac;

   SOIL_INFO( void ) : texture(_T("")), abbrev(_T("")), c( 0.0f ), phi( 0.0f ), kSat(0.0f), wiltPt(0.0f),fieldCapac(0.0f) { }
   };


class SoilInfoArray : public CArray< SOIL_INFO*, SOIL_INFO* >
   {
   public:
      // destructor cleans up any allocated structures
     ~SoilInfoArray( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };

struct GOODNESS_OF_FIT_INFO
   {
   float  r2;
   float  rmse;
   float  ns;
   float  d;


   GOODNESS_OF_FIT_INFO( void ) : r2( 0.0f ), rmse( 0.0f ), ns(1.0f), d(1.0f) { }
   };


class GoodnessOfFitInfoArray : public CArray< GOODNESS_OF_FIT_INFO*, GOODNESS_OF_FIT_INFO* >
   {
   public:
      // destructor cleans up any allocated structures
     ~GoodnessOfFitInfoArray( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };

struct COLLECT_DATA_CELL_OFFSET
   { //in a semi distributed, not grid model, we only use the first.  With a grid, first=x and second=y
   int firstOffset;
   int secondOffset;


   COLLECT_DATA_CELL_OFFSET( void ) : firstOffset( 0 ), secondOffset( 0 ) { }
   };


class CollectDataCellOffsetArray : public CArray< COLLECT_DATA_CELL_OFFSET*, COLLECT_DATA_CELL_OFFSET* >
   {
   public:
      // destructor cleans up any allocated structures
     ~CollectDataCellOffsetArray( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };

class COORD2d_Array : public CArray< COORD2d*, COORD2d* >
   {
   public:
      // destructor cleans up any allocated structures
     ~COORD2d_Array( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };

struct SAMPLE_LOCATION_INFO
   {
   CString name;
   COORD2d xyCoord;
   SAMPLE_LOCATION_INFO( void ) : name(_T("")), xyCoord() { }
   };


class SampleLocationInfoArray : public CArray< SAMPLE_LOCATION_INFO*, SAMPLE_LOCATION_INFO* >
   {
   public:
      // destructor cleans up any allocated structures
     ~SampleLocationInfoArray( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };

struct STREAM_SAMPLE_LOCATION_INFO
   {
   CString name;
   int hydro_id;
   int reachIndex;
   STREAM_SAMPLE_LOCATION_INFO( void ) : name(_T("")), hydro_id(-1), reachIndex(-1) { }
   };


class StreamSampleLocationInfoArray : public CArray< STREAM_SAMPLE_LOCATION_INFO*, STREAM_SAMPLE_LOCATION_INFO* >
   {
   public:
      // destructor cleans up any allocated structures
     ~StreamSampleLocationInfoArray( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };


struct AUXILLARY_MODEL_INFO
   {
   CString  name;
   int    landuse;
   float  c;
   float  p;

   AUXILLARY_MODEL_INFO( void ) : name(_T("")), landuse(0), c( 1.0f ), p( 1.0f ) { }
   };


class AuxillaryModelInfoArray : public CArray< AUXILLARY_MODEL_INFO*, AUXILLARY_MODEL_INFO* >
   {
   public:
      // destructor cleans up any allocated structures
     ~AuxillaryModelInfoArray( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };

struct VEG_INFO
   {
   float mass; // Units are kg/m2, where m2 is an area of plant.  From PRZM.
   float e ; // extraction coefficient (cm-1) (smith &carsel 85 suggest 0.1 as good
   float epsilon; //uptake efficiency factor (dimensionless)
   float kf; // first-order foliar degradation rate constant (day-1) 
   float ks; // degration constation
   float rateMassApp;
   float rateFoliarRunoff;
   float cumMassInput;
   float rateTransformation;
   float cumTransformation;
   float volumeWater;
   VEG_INFO( void ) : mass( 0.00f ), volumeWater(1.0f), e(0.1f), kf(0.7f),ks(0.1f), epsilon(0.1f),
      rateMassApp(0.0f), rateFoliarRunoff(0.0f), cumMassInput(0.0f) , cumTransformation(0.0f), rateTransformation(0.0f){ }
   };

struct SURF_WATER_INFO
   {
   float mass;
   float volumeWater;
   float concentration;
   float rateMassApp;
   float cumMassApp;
   float rateAdvection;
   float rateTransformation;
   float cumTransformation;
   float rateUptake;
   float cumUptake;
   float rateRunoff;
   float cumRunoff;
   float rateMassOutflow;
   float rateErosionLoss;
   float cumErosionLoss;
   float kg;  // lumped first order decay constant for the gas phase (1/day)
   float kh;  // henry's law constant
   float rom; // enrichment ratio for organic matter (g/g)
   float kd;  // adsorption partion coefficient (cm3/g)
   float contributeToStream;
   float bulkDensity;

   float ks;  // first order decay constant for solid and dissolved phases (1/day)
   SURF_WATER_INFO( void ) : mass( 0.00000f ),volumeWater(1.0f),concentration(0.00f), rateMassApp(0.0f), rateAdvection(0.0f), rateTransformation(0.0f), rateUptake(0.0f), rateRunoff(0.0f),rateMassOutflow(0.0f),
      ks(0.1f),kg(0.01f),kh(0.01f), rateErosionLoss(0.0f), rom(0.1f), kd(0.1f), bulkDensity(1350.0f) ,contributeToStream(0.0f),
                             cumMassApp(0.0f),cumTransformation(0.0f),cumUptake(0.0f), cumRunoff(0.0f), cumErosionLoss(0.0f){ }
   };
struct SURF_SOIL_INFO
   {
   float mass;
   float rateTransformation;
   float rateErosionLoss;
   float rateMassOutflow;
   float rom; // enrichment ratio for organic matter (g/g)
   float kd;  // adsorption partion coefficient (cm3/g)

   float bulkDensity;

   SURF_SOIL_INFO( void ) : mass( 0.0f ), rateTransformation(0.0f), rateErosionLoss(0.0f), rateMassOutflow(0.0f), rom(0.5f), kd(0.5f), bulkDensity(1350.0f) { }
   };
struct UNSAT_WATER_INFO
   {
   float mass;
   float volumeWater;
   float concentration;
   float rateMassApp;
   float rateAdvection;
   float rateTransformation;
   float cumTransformation;
   float rateUptake;
   float cumUptake;
   float rateMassOutflow;
   float uptakeEfficiency;
   float kg;  // lumped first order decay constant for the gas phase (1/day)
   float kh;  // henry's law constant
   float kd;
   float ks;

   UNSAT_WATER_INFO( void ) : mass( 0.0f ), volumeWater(1.0f),concentration(0.0f),rateMassApp(0.0f), rateAdvection(0.0f), rateTransformation(0.0f), 
      rateUptake(0.0f), kg(0.01f), kh(0.01f), kd(0.1f), ks(0.1f),rateMassOutflow(0.0f), uptakeEfficiency(0.5f),
      cumUptake(0.0f), cumTransformation(0.0f){ }
   };
struct UNSAT_SOIL_INFO
   {
   float mass;
   float rateTransformation;
   float rateMassOutflow;
   UNSAT_SOIL_INFO( void ) : mass( 0.0f ), rateTransformation(0.0f), rateMassOutflow(0.0f){ }
   };
struct SAT_WATER_INFO
   {
   float mass;
   float volumeWater;
   float concentration;
   float kd;
   float ks;
   float rateAdvection;
   float rateTransformation;
   float rateMassOutflow;
   float rateMassInflow;

   float cumTransformation;
   SAT_WATER_INFO( void ) : mass( 0.0f ), volumeWater(1.0f),concentration(0.0f), kd(0.01f),ks(0.01f),rateAdvection(0.0f), 
      rateTransformation(0.0f), rateMassOutflow(0.0f), rateMassInflow(0.0f), cumTransformation(0.0f){ }
   };
struct SAT_SOIL_INFO
   {
   float mass;
   float rateTransformation;
   float rateMassOutflow;
   SAT_SOIL_INFO( void ) : mass( 0.1f ), rateTransformation(0.0f), rateMassOutflow(0.0f){ }
   };

struct PESTICIDE_INFO
   {
   float appRate;
   int   appMonth;
   int   appDay;
   int   appYear;
   bool beenApplied;
   int offset;
   float oCarbon;
   float koc;
   CString pesticideName;
   float surfaceZoneDepth;
   float rootZoneDepth;
   VEG_INFO *pVeg;
   SURF_WATER_INFO *pSurfWater;
   SURF_SOIL_INFO  *pSurfSoil;
   UNSAT_WATER_INFO *pUnsatWater;
   UNSAT_SOIL_INFO *pUnsatSoil;
   SAT_WATER_INFO *pSatWater;
   SAT_SOIL_INFO *pSatSoil;
   PESTICIDE_INFO( void ) : offset(0), beenApplied(false), oCarbon(0.5f),koc(0.1f),surfaceZoneDepth(0.25f), rootZoneDepth(0.75f), 
      pVeg(NULL), pSurfWater(NULL), pSurfSoil(NULL), pUnsatWater(NULL), pesticideName(_T("")),
      pUnsatSoil(NULL), pSatWater(NULL), pSatSoil(NULL), appRate(0.0f), appMonth(1), appDay(1), appYear(1995)  { }
   ~PESTICIDE_INFO( void ) { delete pVeg; delete pSurfWater; delete pSurfSoil; delete pUnsatWater; delete pUnsatSoil; delete pSatWater; delete pSatSoil;}
   };

class PesticideArray : public CArray< PESTICIDE_INFO*, PESTICIDE_INFO* >
   {
   public:
      // destructor cleans up any allocated structures
     ~PesticideArray( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };


struct _2D_ARRAY_INFO
   {
   int    GW_Index;

   _2D_ARRAY_INFO( void ) :  GW_Index(0) { }
   };
class _2d_Array : public CArray< _2D_ARRAY_INFO*, _2D_ARRAY_INFO* >
   {
   public:
      // destructor cleans up any allocated structures
     ~_2d_Array( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };




struct CHEMICAL_APP_INFO
   {
   CString name;
   int    lulc_c;
   int    lulc_b;
   int    soil;
   float  rateMin;
   float  rateMax;
   int    beginMonth1;
   int    beginDay1;
   int    endMonth1;
   int    endDay1;
   int    appNum;
   int    year;
   CHEMICAL_APP_INFO( void ) : name(_T("")), lulc_c(0), lulc_b(0), soil(0),rateMin( 1.0f ),
      rateMax(10.0f),beginMonth1(10), beginDay1(1), endMonth1(1), endDay1(3), appNum(1), year(3) { }
   };
class ChemicalAppInfoArray : public CArray< CHEMICAL_APP_INFO*, CHEMICAL_APP_INFO* >
   {
   public:
      // destructor cleans up any allocated structures
     ~ChemicalAppInfoArray( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };

struct CHEMICAL_PARAM_INFO
   {
   CString name;
   float  e;
   float  kf;
   float  kg; //ks values are degradation rates, but kow/kd/koc have to do with sorption..
   float  kh;
   float  ksMin;
   float  ksMax;
   float  koc;
   float  rom;

   CHEMICAL_PARAM_INFO( void ) : name(_T("")), e(0.01f), kf(0.01f), ksMin(0.0f), ksMax(0.0f),kg( 1.0f ), 
      kh(10.0f), koc(0.0f),rom(10.0f)  { }
   };

class ChemicalParamInfoArray : public CArray< CHEMICAL_PARAM_INFO*, CHEMICAL_PARAM_INFO* >
   {
   public:
      // destructor cleans up any allocated structures
     ~ChemicalParamInfoArray( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };

struct PARAM_INFO
   {
   float initialValue;
   float value;
   float minValue;
   float maxValue;
   float perturbPercent;
   PARAM_INFO( void ) :  initialValue(1.0f),value(1.0f), minValue(1.0f), maxValue(1.0f), perturbPercent(1.0f)
   { }
   };
struct HYDROLOGIC_PARAM_INFO
   {
   CString landscapeType;
   PARAM_INFO  *m;
   PARAM_INFO  *kSat;
   PARAM_INFO  *wiltPt;
   PARAM_INFO  *fieldCapac;
   PARAM_INFO  *thetaTrace;  //Seibert and McDonnell horizontal drainage model
   PARAM_INFO  *k2;  //Deep reservoir discharge coefficient
   PARAM_INFO  *initSat;
   PARAM_INFO  *soilDepth;
   PARAM_INFO  *phi;
   PARAM_INFO  *kDepth;
   PARAM_INFO  *poreSizeIndex;  
   PARAM_INFO  *bubbling;
   PARAM_INFO  *powerLExp;
   PARAM_INFO *initUnSat;
   PARAM_INFO *tt;  //threshold temperature
   PARAM_INFO *cfmax;//degree day factor
   PARAM_INFO *cfr; //refreeze coeficient
   PARAM_INFO *condense;
   PARAM_INFO *cwh;//threshold for storage of melt water in snowpack
   PARAM_INFO *albedo;  //soil albedo ranges from 0.1 to 0.35 (wet to dry)
   HYDROLOGIC_PARAM_INFO( void ) : landscapeType(_T("")),m(NULL), kSat(NULL), wiltPt(NULL), fieldCapac(NULL),thetaTrace( NULL ), 
      k2(NULL), initSat(NULL), soilDepth(NULL),phi(NULL), poreSizeIndex(NULL),kDepth(NULL), powerLExp(NULL),initUnSat(NULL),
      tt(NULL),cfmax(NULL),cfr(NULL),cwh(NULL),condense(NULL), albedo(NULL),bubbling(NULL)
 { }
   };

//This is an array of HYDROLOGIC_PARAM_INFO.  It is necessary because different landscape Types may have different
//  parameter combinations (landscape type might be HILLSLOPE and RIPARIAN, for instance...)

class HydrologicParamInfoArray : public CArray< HYDROLOGIC_PARAM_INFO*, HYDROLOGIC_PARAM_INFO* >
   {
   public:
      // destructor cleans up any allocated structures
     ~HydrologicParamInfoArray( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };
struct INSTREAM_HYDRO_PARAM_INFO
   {
   CString landscapeType;
   float  n;
   float  wd; // channel width-depth ratio
   INSTREAM_HYDRO_PARAM_INFO( void ) : landscapeType(_T("")),n(0.04f), wd(15.0f) { }
   };

class InstreamHydroParamInfoArray : public CArray< INSTREAM_HYDRO_PARAM_INFO*, INSTREAM_HYDRO_PARAM_INFO* >
   {
   public:
      // destructor cleans up any allocated structures
     ~InstreamHydroParamInfoArray( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };

struct GROUNDWATER_PARAM_INFO
   {
   CString name;
   PARAM_INFO  *kGW;
   PARAM_INFO  *porosity;
   PARAM_INFO  *decline;
   PARAM_INFO  *initSat; 
   PARAM_INFO  *deepLossFraction;
   GROUNDWATER_PARAM_INFO( void ) : name(_T("")),kGW(NULL), porosity(NULL),decline(NULL), initSat(NULL) , deepLossFraction(NULL){ }
   };

class GroundwaterParamInfoArray : public CArray< GROUNDWATER_PARAM_INFO*, GROUNDWATER_PARAM_INFO* >
   {
   public:
      // destructor cleans up any allocated structures
     ~GroundwaterParamInfoArray( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };


struct MULTI_TRACER_INFO
   {
   float mass; 
   float concentration;
   float volumeWater;
   float rateMassInflow;
   float rateMassOutflow;
   float tracerOutflowMassSubsurface;
   float tracerOutflowMassSurface;
   float contributeToStream;
   float contributeToGroundwater;
   float precipConc;
   MULTI_TRACER_INFO( void ) : mass( 0.0f ), concentration(0.0f),volumeWater(1.0f), rateMassOutflow(0.0f),
   tracerOutflowMassSubsurface(0.0f), tracerOutflowMassSurface(0.0f), contributeToStream(0.0f),
   rateMassInflow(0.0f), precipConc(0.0f), contributeToGroundwater(0.0f) { }
   };
class MultiTracerArray : public CArray< MULTI_TRACER_INFO*, MULTI_TRACER_INFO* >
   {
   public:
      // destructor cleans up any allocated structures
     ~MultiTracerArray( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };

struct UPSLOPE_UNSAT_INFO
   {
   float volumeUnsat;
   float exfiltrationRate;
   float infiltrationRate;
   float voidVolume;
   float unsaturatedDepth;
   float usfVol;
   float swcUnsat;
   float kTheta;
   float tracerConc;
	float tracerMass;
   float currentET;
   float tracerOutflowMass;
   MultiTracerArray multiTracerArray;

   UPSLOPE_UNSAT_INFO( void ) : volumeUnsat(0.0f),infiltrationRate(0.0f),exfiltrationRate(0.0f),voidVolume(0.0f),unsaturatedDepth( 0.0f ), usfVol(0.0f), swcUnsat(0.0f),kTheta(0.0f), tracerConc(0.0f) , currentET(0.0f),

         tracerMass(0.0f), tracerOutflowMass(0.0f),multiTracerArray() {}
   };

struct VEGETATION_LAYER_PARAM_INFO
   {
   int layer;
   float imperviousFraction;
   float height;
   float maxResistance;
   float minResistance;
   float moistureThresh;
   float vpd;
   float rpc;
   double *LAI;
   double *albedo;
   VEGETATION_LAYER_PARAM_INFO( void ) : layer(0), imperviousFraction(15.0f),height(5.0),maxResistance(5000),minResistance(1000),moistureThresh(0.01),vpd(10),rpc(1),LAI(NULL)
      ,albedo(NULL) { }
   ~VEGETATION_LAYER_PARAM_INFO( void ) {  }
};

struct VEGETATION_PARAM_INFO
   {
   int vegCode;
   int harvest;
   CString type;
   BOOL  overstory;
   BOOL  understory; 
   float f;
   float trunkSpacing;
   float aerodynamicAtten;
   float radiationAttenuation;
   float maxSnowInterception;
   float massReleaseDripRatio;
   float snowInterceptionEff;
   float soilEvap;
   VEGETATION_LAYER_PARAM_INFO *pOverstoryParam;
   VEGETATION_LAYER_PARAM_INFO *pUnderstoryParam;
   VEGETATION_PARAM_INFO( void ) :vegCode(0), harvest(0), type(_T("")),overstory(TRUE), understory(TRUE),f(0.04f), trunkSpacing(15.0f), aerodynamicAtten(0.04f), radiationAttenuation(15.0f),maxSnowInterception(0.04f)
      , massReleaseDripRatio(15.0f),snowInterceptionEff(0.04f),pOverstoryParam(),pUnderstoryParam(),soilEvap(0.0f) { }
   ~VEGETATION_PARAM_INFO( void ) { delete pOverstoryParam; delete pUnderstoryParam;  }
   };
//This array will store different VEGETATION_PARAM_INFOs that the model may need to access
class VegetationParamInfoArray : public CArray< VEGETATION_PARAM_INFO*, VEGETATION_PARAM_INFO* >
   {
   public:
      // destructor cleans up any allocated structures
     ~VegetationParamInfoArray( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };


struct VEG_LAYER_STATE_INFO
   {
   int type;//0 is overstory and 1 is understory.
   float age;
   float storage;
   float evaporation;
   float transpiration;
   float throughfall;
   float albedo;
   float netLongwave;
   float netShortwave;
   float incomingLongwave;
   float incomingShortwave;
   float pET;
   VEG_LAYER_STATE_INFO( void ) : type(0),age(0.0f),storage(0.00001f),evaporation(0.0f),transpiration(0.0f),albedo(0.2f),throughfall(0.0f),
                          netLongwave(0.0f),netShortwave(0.0f),incomingLongwave(0.0f),incomingShortwave(0.0f),pET(0.0f){}
   };
class VegetationLayerStateArray : public CArray< VEG_LAYER_STATE_INFO*, VEG_LAYER_STATE_INFO* >
   {
   public:
      // destructor cleans up any allocated structures
     ~VegetationLayerStateArray( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };
//CANOPY_INFO stores information about the canopy for each model unit
struct VEGETATION_INFO
   {
   VegetationLayerStateArray vegetationLayerStateArray;
   VEGETATION_PARAM_INFO *pParameters;
   VEGETATION_INFO( void ) : vegetationLayerStateArray(),pParameters(NULL){}
   ~VEGETATION_INFO( void ) { delete pParameters; }
   };


struct MET_INFO
   {
   float temp;
   float dewPoint;
   float solarRad;
   float windSpeed;
   float precip;
   float vaporPressure;
   MET_INFO( void ) : temp(0.0f), dewPoint( 4.0f ),solarRad( 4.0f ),windSpeed(0.5f),precip(0.0f),vaporPressure(1.0) {}
   };

struct OUTFLOW_INFO
   {
   float outflow;
//   UPSLOPE_INFO *pOutflowTo;
   OUTFLOW_INFO( void ) : outflow(0)/*, pOutflowTo(NULL)*/ { }
   ~OUTFLOW_INFO( void ) {   }
};
class OutflowInfoArray : public CArray< OUTFLOW_INFO*, OUTFLOW_INFO* >
   {
   public:
      // destructor cleans up any allocated structures
     ~OutflowInfoArray( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };

struct UPSLOPE_INFO     // contains upslope unit information
   {
   OutflowInfoArray outflowArray;
   float griddedSoilDepth;  //when we use gridded soil information
   int GWoffset;
   int gridOffset;
   float volumeSat;     // volume in the saturated zone (m3)
   float voidVolume;   // volume in the unsaturated zone (m3)
   float volumeSWE;
   float volumeMelt;
   float volumeVeg;
   float vegDrainRate;
   float meltRate;
   float saturatedDepth;
   float ssfVol;
   float sdmm;
   float area;       // area (m2)
	int index;
	float areaTotal;
   float upstreamLength;
   int   basinOrder;
   float geoSource;
   float c;          // 
   float musleP;
   float oCarbon;
   float slope;      // rise/run
   float lengthSlope;
   float kFfact;
   int   isSaturated;
   float ltPrecip;     // long term yearly precip in m
	float minFPDist;
	float maxFPDist;
   int   sideOfStream;
   float lateralOutflowVol; 
   float surfaceOutflowVol;
	float lateralInflow;
   int lulc;
   int lulc_b;
   int ageClass;
	float swc;
   float swcUnsat;
   float pondedVolume;
	float tracerConc;
	float tracerMass;
   float initUpslopeTracerMass;
   float tracerOutflowMassSubsurface;
   float tracerOutflowMassSurface;
   float tracerInflowMassSubsurface;
   float tracerWeightedMeanTime;
   float sedimentMass;
   float sedimentOutflowMass;
   float musleC;
   int   colIndex;
   int   rowIndex;
   int   radarRow;
   int   radarCol;
   float currentPrecip;
   float currentGroundLoss;
   float currentSnow;
   float currentET;
   float currentAirTemp;
	float cumPrecipVol;
   float precipVol;
	float cumETVol;
   float cumGroundLoss;
   float etVol;
	float cumSWC;
   int hasWetlands; // are there wetland areas in the cell?
   float tileOutflowCoef;
   float tileDrainageVol;
   float cumTracerMassInPrecip;
   int producedOverlandFlow;
   float groundWaterChange; // the change (m) of the groundwater table (+ or -)
   float deltaSat;
   float deltaUnsat;
   float elev;
   float wtElev;
   bool isStream;
   float initTracerMass;
   float resTime;
   float firstMoment;
   float zerothMoment;
   int precipStationOffset;
   int tempStationOffset;
   bool gwBC;
   float previousVolumeSnow;
   float previousVolumeWater;
   BOOL lowestElevationGridCell;
   UPSLOPE_UNSAT_INFO *pUnsat;
   VEGETATION_INFO *pVegetation;
   MET_INFO *pMet;
   CUIntArray cellsRepresentedArray;
   int rkfIndex;
   UPSLOPE_INFO *pUpslope;
   PesticideArray pesticideArray;
   MultiTracerArray multiTracerArray;
   HYDROLOGIC_PARAM_INFO *pHydroParam;
   float *neighborFluxArray;
   class UpslopeInfoArray1 : public CArray< UPSLOPE_INFO*, UPSLOPE_INFO* >
      {
      public:
         ~UpslopeInfoArray1( void ) { RemoveAll(); }
         void RemoveAllElements( ) 
            { 
            for (int i=0; i < GetSize(); i++) 
               delete GetAt( i );			
			   RemoveAll();
			   }
            
      };
   UpslopeInfoArray1 neighborArray;
   

   UPSLOPE_INFO( void ) : volumeSat( 0.0f),  saturatedDepth(0.0f), voidVolume(0.0f),initUpslopeTracerMass(0.0f),
      ssfVol(0.0f),sdmm(0.0f), area( 1.0f ), index(0), isSaturated(0), oCarbon(0.1f),previousVolumeSnow(0.0f),previousVolumeWater(0.0f),
      areaTotal( 0.0f ),  swcUnsat(0.0f), groundWaterChange(0.0f),upstreamLength(0.0f),basinOrder(0),
      c( 0.0f ),  slope( 0.0f ), lengthSlope(0.0f), kFfact(0.32f),lulc_b(0),precipStationOffset(0),tempStationOffset(0),
      ltPrecip(1.5f), minFPDist(0.0f), musleP(0.0f), deltaSat(0.0f), deltaUnsat(0.0f),
      maxFPDist(-1.0f), sideOfStream(0), lateralOutflowVol( 0.0f), lateralInflow(0.0f), lulc( 0 ),
      swc(0.0f), pondedVolume(0.0f),tracerConc(0.0f), tracerMass(0.0f),tracerOutflowMassSurface(0.0f), tracerInflowMassSubsurface(0.0f),
      sedimentMass(1E20f), sedimentOutflowMass(0.0f), musleC(0.0f),volumeMelt(0.0f),
      currentPrecip(0.0f),currentET(0.00f),currentAirTemp(10.0f), cumPrecipVol(0.0f),precipVol(0.0f),
      etVol(0.0f), cumETVol(0.0f), cumSWC(0.0f), cumGroundLoss(0.0f),currentGroundLoss(0.0f),
      hasWetlands(0), tileOutflowCoef(0.0f), tileDrainageVol(0.0f),tracerWeightedMeanTime(0.0f),
      pUpslope(NULL), pUnsat(NULL), cumTracerMassInPrecip(0.0f), tracerOutflowMassSubsurface(0.0f), surfaceOutflowVol(0.0f),
      producedOverlandFlow(0),  pesticideArray(), colIndex(0), rowIndex(0), elev(0.0f), isStream(false), neighborArray(),
      radarRow(0),radarCol(0),initTracerMass(0.0f), volumeSWE(0.0f), meltRate(0.0f), currentSnow(0.0f), volumeVeg(0.0f),vegDrainRate(0.0f), resTime(0.0f),
	  firstMoment(0.0f), zerothMoment(0.0f), multiTracerArray(), geoSource(-1), griddedSoilDepth(0.0f),GWoffset(0),gridOffset(0),
      rkfIndex(0),ageClass(0),wtElev(0.0f),lowestElevationGridCell(FALSE), outflowArray(), gwBC(false)
   { }
   ~UPSLOPE_INFO( void ) { delete pUnsat; delete pVegetation; delete pMet; delete pUpslope; delete pHydroParam; delete [] neighborFluxArray;}
   };

class UpslopeInfoArray : public CArray< UPSLOPE_INFO*, UPSLOPE_INFO* >
   {
   public:
      ~UpslopeInfoArray( void ) { RemoveAll(); }

      void RemoveAllElements( ) 
         { 
         for (int i=0; i < GetSize(); i++) 
            delete GetAt( i );
			
			RemoveAll();
			}
         
   };
struct GW_INFO
   {
   float volumeGW;
   float lateralOutflowVol;
   float lateralInflowVol;
   float rechargeVol;
   float streamRecharge;
   float depth;
   float systemLoss;
   float cumSystemLoss;
   float totalDepth;
   float deepGroundLoss;
   float pondedVolume;
   GROUNDWATER_PARAM_INFO *pParam;
   int GWoffset;
   bool gwBC;
   float area;
   float elev;
   float wtElev;
   int rowIndex;
   int colIndex;
   int numVerticalNeighbors;
   int numHorizontalNeighbors;
   BOOL lowestElevationGridCell;

   CUIntArray upslopeInfoOffsetArray;
   MultiTracerArray multiTracerArray;
   UpslopeInfoArray upslopeInfoArray;
   float *neighborFluxArray;
   class GWInfoArray1 : public CArray< GW_INFO*, GW_INFO* >
      {
      public:
         ~GWInfoArray1( void ) { RemoveAll(); }

         void RemoveAllElements( ) 
            { 
            for (int i=0; i < GetSize(); i++) 
               delete GetAt( i );
   			
			   RemoveAll();
			   }
            
      };
   GWInfoArray1 neighborArray;

   GW_INFO( void ) : volumeGW(0.0f),lateralOutflowVol(0.0f), lateralInflowVol(0.0f), rechargeVol(0.0f), streamRecharge(0.0f),area(0.f),elev(0.0f),
                     depth(10.0f),systemLoss(0.0f),cumSystemLoss(0.0f),totalDepth(10.0),multiTracerArray(),upslopeInfoArray(),
                     deepGroundLoss(0.0f), pondedVolume(0.0f), GWoffset(0),upslopeInfoOffsetArray(),lowestElevationGridCell(FALSE),
                     rowIndex(0),colIndex(0),neighborArray(),numVerticalNeighbors(0),numHorizontalNeighbors(0),wtElev(0.0f), gwBC(false)
                     {}
   ~GW_INFO( void ) { delete pParam; delete [] neighborFluxArray;}
   };
   class GWInfoArray : public CArray< GW_INFO*, GW_INFO* >
      {
      public:
         ~GWInfoArray( void ) { RemoveAll(); }

         void RemoveAllElements( ) 
            { 
            for (int i=0; i < GetSize(); i++) 
               delete GetAt( i );
   			
			   RemoveAll();
			   }
            
      };
  // GWInfoArray neighborArray;

class UpslopeFor2D : public CArray< UpslopeInfoArray*, UpslopeInfoArray* >
   {
   public:
      ~UpslopeFor2D( void ) { RemoveAll(); }

      void RemoveAllElements( ) 
         { 
         for (int i=0; i < GetSize(); i++) 
            delete GetAt( i );
			
			RemoveAll();
			}
         
   };

struct INSTREAM_PESTICIDE_INFO
   { 
   CString pesticideName;
   float *pesticideMassArray;
   float *pesticideMassArrayUpstream;
   float *pesticideDischargeArray;
   float pesticideConc;
   float upslopeContribution;
   float cumPesticideOutflow;

   INSTREAM_PESTICIDE_INFO( void ) : pesticideName(_T("")),pesticideMassArray(NULL), pesticideMassArrayUpstream(NULL),pesticideDischargeArray(NULL), 
      upslopeContribution(0.0f),pesticideConc(0.0), cumPesticideOutflow(0.0)  { }
    ~INSTREAM_PESTICIDE_INFO( void ) { delete [] pesticideMassArray;delete [] pesticideMassArrayUpstream;delete [] pesticideDischargeArray;}
   };

class InstreamPesticideArray : public CArray< INSTREAM_PESTICIDE_INFO*, INSTREAM_PESTICIDE_INFO* >
   {
   public:
      // destructor cleans up any allocated structures
     ~InstreamPesticideArray( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };

struct INSTREAM_MULTI_TRACER_INFO
   { 
   CString tracerName;
   float *multiTracerMassArray;
   float *tracerDischargeArray;
   float *tracerHillslopeTransferConcArray;
   float tracerConc;
   float upslopeContribution;
   float cumTracerOutflow;
   float cumTracerInput;
   float *upslopeEndMemberConcentration;
   INSTREAM_MULTI_TRACER_INFO( void ) : tracerName(_T("")),multiTracerMassArray(NULL),tracerDischargeArray(NULL),
      upslopeContribution(0.0f), cumTracerInput(0.0f), cumTracerOutflow(0.0f),
      tracerConc(0.0), tracerHillslopeTransferConcArray(NULL),upslopeEndMemberConcentration(NULL)  { }
    ~INSTREAM_MULTI_TRACER_INFO( void ) { delete [] multiTracerMassArray;delete [] tracerDischargeArray;delete [] tracerHillslopeTransferConcArray;delete [] upslopeEndMemberConcentration;}
   };

class InstreamMultiTracerArray : public CArray< INSTREAM_MULTI_TRACER_INFO*, INSTREAM_MULTI_TRACER_INFO* >
   {
   public:
      // destructor cleans up any allocated structures
     ~InstreamMultiTracerArray( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };


struct REACH_INFO_HYDRO   // extends the REACH_INFO structure
   {
   float width;      // width of the stream reach (m)   ASSSUMES RECTANGULAR
   float depth;      // depth of the stream reach (m)
   float area;
   float alpha;
   float beta;
   float lateralInflow;      // flow rate into this reach
   float transferWaterFromUpslope;
   float transferSoluteFromUpslope;
   float cumInflow;
   float cumOutFlowVol;
   float cumLateralInflow;
   float cumLateralInflowGround;
   float cumPrecip;
   float cumGroundwater;
   float cumET;
   float initUnsatSoilVol;
   float initSatSoilVol;
   float finalUnsatSoilVol;
   float finalSatSoilVol;
   float finalSWE;
	int   streamOrder;
	float ttp;
	float maxFPDistSide1;
   float maxFPDistSide2;
   float tracerConc;
   float resTime;
   float firstMoment;
   float zerothMoment;
   float sedimentConc;
   float percentLengthAltered;
   int   incrFlow;
   float cumSedimentOutflow;
   InstreamPesticideArray instreamPesticideArray;
   InstreamMultiTracerArray multiTracerArray;
   float *sedimentMassArray;
   float *tracerMassArray;//correpsonds to reachTree qArray
   float cumTracerInput;
   float *waterVolumeArray;
   float *waterVolumeArrayPrevious;
   float cumTracerOutFlow;
   float *tracerDischargeArray;
  

   float *qArrayPrevious;
   float *sedimentDischargeArray;
   INSTREAM_HYDRO_PARAM_INFO *pInstreamHydroParam;
   UpslopeInfoArray upslopeInfoArray;


   REACH_INFO_HYDRO( void ) : width( 0.0f ), depth( 0.0f ), area(0.0f),
       alpha( 1.0f) , beta( 3/5.0f), lateralInflow( 0.0f), 
      cumInflow( 0.0f ), cumOutFlowVol( 0.0f ), cumLateralInflow( 0.0f ), cumLateralInflowGround(0.0f),cumPrecip(0.0f), cumGroundwater(0.0f),cumET(0.0f),
      initUnsatSoilVol(0.0f), initSatSoilVol(0.0f), finalUnsatSoilVol(0.0f), finalSatSoilVol(0.0f),
      streamOrder( 0 ), ttp( 0.0f ), maxFPDistSide1(0.0f), maxFPDistSide2(0.0f), tracerConc(0.0f),
      percentLengthAltered(0.0f), incrFlow(0), cumSedimentOutflow(0.0f), resTime(0.0f), firstMoment(0.0f),zerothMoment(0.0f),
      sedimentConc(0.0f), sedimentMassArray(NULL), cumTracerInput(0.0f),transferWaterFromUpslope(0.0f),transferSoluteFromUpslope(0.0f),
      tracerMassArray(NULL),cumTracerOutFlow(0.0f), waterVolumeArray(NULL), finalSWE(0.0f),
      tracerDischargeArray(NULL), qArrayPrevious(NULL), sedimentDischargeArray(NULL),waterVolumeArrayPrevious(NULL),
      upslopeInfoArray(), instreamPesticideArray(),multiTracerArray(),pInstreamHydroParam(NULL)
      { }

   ~REACH_INFO_HYDRO( void ) { delete [] sedimentMassArray;delete [] tracerMassArray;delete [] waterVolumeArray;delete [] waterVolumeArrayPrevious;delete [] tracerDischargeArray;delete [] qArrayPrevious;delete [] sedimentDischargeArray; delete pInstreamHydroParam;}
   };


class ReachInfoHydroArray : public CArray< REACH_INFO_HYDRO*, REACH_INFO_HYDRO* >
   {
   public:
      ~ReachInfoHydroArray( void ) {for ( int i=0; i < GetSize(); i++) delete GetAt( i ); }

   };

class GWArray : public CArray< GW_INFO*, GW_INFO* >
   {
   public:
      ~GWArray( void ) {for ( int i=0; i < GetSize(); i++) delete GetAt( i ); }

   };
struct MET_GRID_INFO
   {
   MapLayer *pMinT1;
   MapLayer *pMaxT1;
   MapLayer *pMinT2;
   MapLayer *pMaxT2;
   MapLayer *pMinT3;
   MapLayer *pMaxT3;
   MapLayer *pMinT4;
   MapLayer *pMaxT4;
   MapLayer *pMinT5;
   MapLayer *pMaxT5;
   MapLayer *pMinT6;
   MapLayer *pMaxT6;
   MapLayer *pMinT7;
   MapLayer *pMaxT7;
   MapLayer *pMinT8;
   MapLayer *pMaxT8;
   MapLayer *pMinT9;
   MapLayer *pMaxT9;
   MapLayer *pMinT10;
   MapLayer *pMaxT10;
   MapLayer *pMinT11;
   MapLayer *pMaxT11;
   MapLayer *pMinT12;
   MapLayer *pMaxT12;
  
   MET_GRID_INFO( void ) : pMinT1(NULL), pMaxT1(NULL), pMinT2(NULL), pMaxT2(NULL), pMinT3(NULL),
      pMaxT3(NULL), pMinT4(NULL), pMaxT4(NULL), pMinT5(NULL), pMaxT5(NULL), pMinT6(NULL), pMaxT6(NULL), 
      pMinT7(NULL), pMaxT7(NULL), pMinT8(NULL), pMaxT8(NULL), pMinT9(NULL), pMaxT9(NULL), 
      pMinT10(NULL), pMaxT10(NULL), pMinT11(NULL), pMaxT11(NULL), pMinT12(NULL), pMaxT12(NULL)  
   { }
 
   };

class MetGridInfoArray : public CArray< MET_GRID_INFO*, MET_GRID_INFO* >
   {
   public:
      // destructor cleans up any allocated structures
     ~MetGridInfoArray ( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };

struct CELL_INFO
   {
   UPSLOPE_INFO *pUpslope;

   CELL_INFO( void ) : pUpslope( NULL ) { }
   };

struct PESTICIDE_MASS_BALANCE_INFO
   { 
   CString pesticideName;
   float inVeg;
   float inSurf;
   float transSurf;
   float transRoot;
   float transSat;
   float transVeg;
   float upSurf;
   float upRoot;
   float runoffSurf;
   float erodeSurf;
   float cumOut;//discharge of pesticide from the stream

   PESTICIDE_MASS_BALANCE_INFO( void ) : pesticideName(_T("")),inVeg(0.0f),inSurf(0.0f), transSurf(0.0f),transRoot(0.0f),transSat(0.0f),
      upSurf(0.0f), upRoot(0.0f), runoffSurf(0.0f), erodeSurf(0.0f), cumOut(0.0f), transVeg(0.0f)  { }
   };

class PesticideMassBalanceArray : public CArray< PESTICIDE_MASS_BALANCE_INFO*, PESTICIDE_MASS_BALANCE_INFO* >
   {
   public:
      // destructor cleans up any allocated structures
     ~PesticideMassBalanceArray( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };

struct MASS_BALANCE_INFO
	{
	float rainInputVolume;
	float initChannelVolume;
	float initSoilVolume;
   float initUnsatSoilVolume;
   float finalSWE;
   float finalSoilVolume;
   float finalUnsatSoilVolume;
   float finalPondedVolume;
	float finalChannelVolume;
	float streamflowVolume;
	float etVolume;
   float groundLossVol;
   float initialTracerMass;
   float finalTracerMass;
   float tracerInputMass;
   float tracerOutputMass;
   float sedimentOutputMass;
   float r2;
   float rmse1;
	float ns1;
   float rmse2;
	float ns2;
   float rmse3;
	float ns3;
   float rmseMulti;
	float nsMulti;
   float perNewWater;
   float freqSat;
   float areaOver;
   float d;
   float hrt;
   float catchmentMrt;
   float upslopeMeanMrt;
   float upslopeMinMrt;
   float upslopeMaxMrt;
   float difference;

   float groundToStream;
   float initGroundwater;
   float finalGroundwater;
   float groundwaterSystemLoss;

   MASS_BALANCE_INFO( void ) : rainInputVolume(0.0f), initChannelVolume(0.0f), initSoilVolume(0.0f), finalSWE(0.0f),
      finalPondedVolume(0.0f), finalChannelVolume(0.0f), streamflowVolume(0.0f), etVolume(0.0f),
      initialTracerMass(0.0f), finalTracerMass(0.0f),tracerInputMass(0.0f),tracerOutputMass(0.0f),
      sedimentOutputMass(0.0f), rmse1(0.0f), ns1(0.0f), rmse2(0.0f), ns2(0.0f), 
      rmse3(0.0f), ns3(0.0f), rmseMulti(0.0f), nsMulti(0.0f),  perNewWater(0.0f), 
      freqSat(0.0f), areaOver(0.0f), d(0.0f), r2(0.0f), hrt(0.0f), catchmentMrt(0.0f), difference(0.0f),
      upslopeMeanMrt(0.0f),upslopeMinMrt(1E6f),upslopeMaxMrt(0.0f),initUnsatSoilVolume(0.0f),finalUnsatSoilVolume(0.0f),
      groundToStream(0.0f),initGroundwater(0.0f),finalGroundwater(0.0f),groundwaterSystemLoss(0.0f)
      {}
   
	};


struct PARTICLE_INFO
   { 
   UPSLOPE_INFO *pStartCell;
   UPSLOPE_INFO *pLocation;
   float startTime;
   PARTICLE_INFO( void ) : pStartCell(NULL), pLocation(NULL), startTime(0.0f) { }
   };

class ParticleArray : public CArray< PARTICLE_INFO*, PARTICLE_INFO* >
   {
   public:
      // destructor cleans up any allocated structures
     ~ParticleArray( void ) { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }      
   };

class CellInfoArray : public CArray< CELL_INFO*, CELL_INFO* >
   {
   public:
      ~CellInfoArray( void ) { Clear(); }

      void Clear( void ) {for ( int i=0; i < GetSize(); i++) delete GetAt( i ); RemoveAll(); }
   };

class ResultDataArray : public CArray< DataObj**, DataObj** >
   {
   public:
      // destructor cleans up any allocated structures
      ~ResultDataArray( void ) {Clear();}
      void Clear(void){
        for ( int i=0; i < GetSize(); i++ )

               delete GetAt(i); 
         RemoveAll(); 
        }      
   };

class WET_Hydro  
{
public:
	WET_Hydro();
	virtual ~WET_Hydro();
   CString m_localFolder;

   void ReverseByteOrder(short *in_array,int arraysize);
   UpslopeInfoArray m_arrayOfpUpslopes;
   RandUniform m_randUnif1;
   RandUniform m_randUnif2;
   RandUniform m_randUnif3;

 

   void  SetMapPtr( Map *pMap ) { m_pMap = pMap; }
   int   LoadClimateData( LPCSTR filename );
   int   LoadPenmanMontiethClimateData( LPCSTR filename );
   int   LoadSoilData( LPCSTR filename );
   int   LoadMeasuredDischargeData( LPCSTR filename );
   int   LoadMeasuredBCData( LPCSTR filename );
	int   LoadMeasuredEtData( LPCSTR  filename );
   int   LoadTracerInputData( LPCSTR  filename );
   int   LoadAuxillaryModelData( LPCSTR filename );
	int   LoadSyntheticqData( LPCSTR filename );
	int   LoadParameters( LPCSTR filename );
	int   SaveParameters( LPCSTR filename );
	int   SaveData( int runNumber );
   void  InitUpslopeDataCollection();
   void  InitCrossSectionCollection();
   void  CollectDataCrossSection();
   void  InitInstreamDataCollection();
   void  GetStateVarCount();
   void  AllocateStateVarArray();
   void  AllocateGWStateVarArray();
   void  InitMovieCapture();

   void  SetTimeStep( double timeStep ) { m_timeStep = timeStep; }
	void  SetDeltaX( float deltaX ) { m_deltaX = deltaX; }
	//void  SetWDRatio(float wdRatio) { m_wdRatio = wdRatio; }
   double GetTimeStep( void ) { return m_timeStep; }

	void  SetStartTime( double startTime ) { m_startTime = startTime; }
   void  SetStopTime( double stopTime ) { m_stopTime = stopTime; }
   double GetStopTime( void ) { return m_stopTime; }

   void  Run( );
   void  RunMC();
	void  RunSynthetic(  );
	void  CalcInitialConditions( double endCalib,  bool save=true );
	int   CalcElevSurface( void );



   //Methods translating maplayer data into model configurations
	void  CalcWatershedInfo(  );
   void  CreateSemiDistributed();
   void  CreateHillslopeRiparian();
   void  CreateSimpleDistributed();
   void  CreateGeomorphologic();
   void  CreateGeoHillRip();
   void  CreateSubnodeArray();
   void  BuildTopology();
   void  BuildGWTopology();
   void ResetVariableValues();
   float GetMinElevation();
      float GetMaxElevation();
///////////////////////////////////////////////////
   
   UPSLOPE_INFO *GetUpslopeInfoFromRowCol(int row,int col);
   void ApplyTracerToGridCell(int row, int col);

	float CalculateTotalArea( void );

   float CalcDInstreamTracer_dt(ReachNode *pNode, int i, float tracerInUpslope);
   float CalcDInstreamPesticide_dt(ReachNode *pNode, int i, float pesticideInUpslope, int j);
   float CalcDInstreamMultiTracer_dt(ReachNode *pNode, int i, float pesticideInUpslope, int j);

   float CalcDUnsatMultiTracer_dt(UPSLOPE_INFO *pUpslope, int i);
   float CalcDMultiTracer_dt(UPSLOPE_INFO *pUpslope, int i);
   float CalcDMultiTracerGW_dt(GW_INFO *pGW, int i);

   float CalcDUplandTracer_dt(UPSLOPE_INFO *pUpslope);
   float CalcDUnsatTracer_dt(UPSLOPE_INFO *pUpslope);
   float CalcDInstreamSediment_dt(ReachNode *pNode, int i);
   float CalcDUplandSediment_dt(UPSLOPE_INFO *pUpslope);
   //float CalcDSurfaceWater_dt(UPSLOPE_INFO *pUpslope);
   float CalcDVolume_dt(UPSLOPE_INFO *pUpslope);
   float CalcDVolumeUnsat_dt(UPSLOPE_INFO *pUpslope);
   float CalcDSWE_dt(UPSLOPE_INFO *pUpslope);
   float CalcDMelt_dt(UPSLOPE_INFO *pUpslope);
   float CalcDVegVolume_dt(UPSLOPE_INFO *pUpslope); 
   float CalcDGW_dt(GW_INFO *pGW);

   float CalcDPlantPesticide_dt(UPSLOPE_INFO *pUpslope,int i);
   float CalcDSurfacePesticideSoil_dt(UPSLOPE_INFO *pUpslope,int i);
   float CalcDSurfacePesticideWater_dt(UPSLOPE_INFO *pUpslope,int i);
   float CalcDUnsatPesticideSoil_dt(UPSLOPE_INFO *pUpslope,int i);
   float CalcDUnsatPesticideWater_dt(UPSLOPE_INFO *pUpslope,int i);
   float CalcDSatPesticideSoil_dt(UPSLOPE_INFO *pUpslope,int i);
   float CalcDSatPesticideWater_dt(UPSLOPE_INFO *pUpslope,int i);

   void UpdatePesticideParameters(PESTICIDE_INFO *pPesticide,UPSLOPE_INFO *pUpslope);
   void UpdatePesticideMassBalance(UPSLOPE_INFO *pUpslope);

   void GetSoilParametersFromXLS(UPSLOPE_INFO *pUpslope, int index);
   void GetModelParameters(UPSLOPE_INFO *pUpslope, int index);
   void SampleModelParameters();
	static void EstimateParams(float x, float a[], int ia[], float *y, float dyda[], int na, long extra, int init );
	void CalculateParameterDerivatives( float z, float a[], float *y, float dyda[], int na);
	
	void ProcessWETCells( void );

	void UpdateDeltaX();

   MapLayer *SetStreamLayer( LPCSTR shapeFileName );
   MapLayer *SetCellLayer( LPCSTR shapeFileName );
	MapLayer *SetDEMLayer( LPCSTR shapeFileName );
   MapLayer *SetWatershedLayer(LPCSTR filename);
   MapLayer *SetGWLayer( LPCSTR AsciiDEMFileName );
   MapLayer *SetFDLayer(LPCSTR filename);
   MapLayer *FlowDirection( MapLayer *pDEMLayer);
   //Raster  *m_pGroundWater; 
   float m_tempPrimet;
	//ScatterWnd *CreateScatter( DataObj *pScatterData);
   MapLayer *GetMonthlyGrid(int month, int minOrMax);
	void SaveData();
	void SaveState( double time); //  Type represents the reason behind the save.  See implem. for more detail
	void SaveStateEnergy( double time); //  Type represents the reason behind the save.  See implem. for more details
	
   void GetSpatialAverages(float *data);
   void RestoreState( int type );
	void PlotOutput();

	bool SaveToFile(LPCSTR filename, bool append=false );

	ResultDataArray m_resultDataArray;
	//DataStore  *m_pDataStore; 
	//SAVED_STATE_INFO *m_pSavedData;
	//SAVED_STATE_INFO *m_pInitialConditions;

	int GetFieldColFromInterfaceValue(int model=1);
   int CopyStateFromReachTreeToMapLayer(int model, int stateVar);
   int GetGWCount();
   void GetAverageGWSurfaceElevation();  //calculate the average elevation of the ground surface above a groundwater cell
   void GetMinGWSurfaceElevation();      //calculate the minium elevation of the ground surface above a groundwater cell
protected: 
   

   // various map layers
public:
   Map *m_pMap;
   MapLayer *m_pDEMLayer;
   //MapLayer *m_pUpslopeGWMap;
   MapLayer *m_pGeoDepthLayer;
   MapLayer *m_pSoilDepthLayer;
   MapLayer *m_pWatershedLayer;
   MapLayer *m_pStreamGridLayer;
   MapLayer *m_pFlowDirLayer;
   MapLayer *m_pStreamLayer;
   MapLayer *m_pCellLayer;
	int m_runNumber;
	int m_optNumber;
   ReachTree m_reachTree;
	double m_time;
   double m_timeStep;
   double m_curTimeStep;
	double m_streamTime;
   double m_streamTimeStep;
   double m_timeStepInternal;
   int   m_timeOffset;
   int m_gwTimeOffset;
	float  m_deltaX;
   double m_stopTime;
   double m_stopTimeHolder;
	double m_startTime;
   float m_minElev;
   float m_maxElev;
   int m_numShallowUnits;

   double m_nsBailOut;
   int m_nsCheckForBailOut;
   float m_ns;
   float m_nsMax;
   int m_writeToDiskEvery;
   double m_writeQwhenNS;
   double m_seed;
   int m_biomassScenario;


	float m_tracerAppLagTime;

   int m_collectDataCount;
   float m_areaTotal;
   int m_numberClimateStations;
   double m_gwGridReductionFactor;

   CString m_dataToMap;
   DYNAMIC_MAP_INFO *m_pDynamicMapInfo;
   void  UpdateParameterVector();
protected:
   MapLayer *m_pSurfaceWaterLayer;
   MapLayer *m_pSubSurfaceWaterLayer;

   // tree to contain reach information
   CellInfoArray       m_cellInfoArray;

   // simulation control

   float m_cellSize;
   float m_maxReachDeltaX;

   // miscellaneous
   float m_noDataValue;  // for all constructed layers
   float m_surfaceWaterThreshold;  // mm  
   ROW_COL m_pourPoint;     // col/row
   float m_gridResolution;
   float m_gwGridResolution;




public:
   double m_gwLowerBoundaryCondition;
   double m_swLowerBoundaryCondition;
	// model parameters available to the optimization routines
   float m_beta;                   // surface roughness???
   float m_ddExp;                  // depth/discharge exponent (see Bedient and Huber, 1992,  pg 276)
	float m_et;
   float m_stationLtPrecip;
	double m_collectionInterval;
   int m_spinUp;
   float m_initTracerConc;
   float m_bufferWidth;
	BOOL m_updateStream;
   BOOL m_updateUpland;
   BOOL m_checkSideOfStream;
   BOOL m_checkUseSoilList ;
   BOOL m_checkMassBalance;
   int m_routeChannelWaterMethod;
   BOOL m_checkSimulateWetlands;
   BOOL m_checkThreeBasins;
   BOOL m_checkUnsaturated;
   BOOL m_checkEnergyBalance;
   BOOL m_checkSnow;
   BOOL m_checkGridMet;
   BOOL m_radar;
   BOOL m_checkDistributeInitCond;
   BOOL m_useLapseRate;

   //Tracer Interface
   float m_editTracerMgL;
   int m_radioTracerInput;
   BOOL m_checkTracerModel;
   BOOL m_checkMultiTracer;
   int  m_numberMultiTracer;
   int  m_numTracerDays;
   BOOL m_checkPesticide;
   BOOL m_checkGround;
   BOOL m_aggregateGrids;
   double  m_aggregateAmount;
   BOOL m_pesticideApp;  //flag to indicate that a pesticide application is currently happening
   void GetPesticideAppRateSimple();
   void GetPesticideAppRate();
   void GetPesticideAppTiming();
   void GetPesticideAppRateMultipleApplications();
   CUIntArray m_pesticideOffsets;

	int  m_streamDataToCollect;  //interface code
	int  m_cellDataToCollect;   //interface code
   BOOL m_checkPrism;

	float m_maxCentroidDistance;
	int   m_numBands;
	float m_widthBand;

   int  m_modelStructureMethod; //how the model is to be run (cells, watersheds, or distributed)
	int  m_modelParamMethod ; 
	BOOL m_ksAsParam;
	BOOL m_sdAsParam;
   
   int m_etMethod;
   int m_satMethod;
   int m_unsatMethod;
   float m_drainShape;
   BOOL m_checkEtModel;
   BOOL m_radioPorosity;
   BOOL m_checkInterception;
   BOOL m_checkSoilDepthMap;
   int  m_routeChannelSolutesMethod;

   BOOL m_checkUplandSediment;
//	BOOL m_checkReduceSediment;
   BOOL m_checkApplySedOptions;
   BOOL m_saveEmfOutput;
   BOOL m_checkIncrFlow;
   BOOL m_collectGriddedData;
   BOOL m_exportAllFluxesandStates;
   int m_checkMonteCarlo;
   int m_numIterations;
   CTime m_incrFlowBegin;
   CTime m_incrFlowEnd;
   float m_incrFlowPercent;
	float m_reduceSedPercent;
   BOOL  m_checkModelTileDrains;
   float m_tileOutflowCoef;
   float m_tileThreshold;

   int m_numberPesticides;

   float m_latitude;
	float m_ac ;
	float m_airDensity ;
	float m_bc ;
	float m_bioGeoAbsorp ;
	float m_extraTerrRadiation ;
	float m_clearFraction ;
	float m_cloudFraction ;
	float m_gradientSatVapor ;
	float m_heatCond ;
	float m_heightOfHumid ;
	float m_heightOfWind ;
	float m_shortWave ;
	float m_latentHeatEvap ;
	float m_psychrometric ;
	float m_relativeHumid ;
	float m_satVapPres ;
	float m_stephanBoltz ;
	float m_specificHeat ;
	float m_storedE ;
	float m_windSpeed ;

   float m_rtIntegral;
   float m_totalDischarge;

public:
   // data storage
   FDataObj *m_pClimateData;
   FDataObj *m_pMeasuredDischargeData;
	FDataObj *m_pMeasuredEtData;
	FDataObj *m_pMeasuredBCData;
   FDataObj *m_pMeasuredTracerInputData;
   FDataObj *m_pTracerData;
   FDataObj *m_pSwcData;
   FDataObj *m_pRateData;
   FDataObj *m_pPestVegData;
   FDataObj *m_pPestInstreamData;

   FDataObj *m_pPestSurfData;
   FDataObj *m_pPestUnsatData;
   FDataObj *m_pPestSatData;
   FDataObj *m_pPestMassBalanceData;

   FDataObj *m_pMultiTracerUnsatConcData;
   FDataObj *m_pMultiTracerGWConcData;
   FDataObj *m_pMultiTracerSatConcData;
   FDataObj *m_pMultiTracerConcChannelData;
   FDataObj *m_pMultiTracerEndMemberConc;

   FDataObj *m_pTracerConcData;
   FDataObj *m_pTracerConcChannelData;
   FDataObj *m_pTracerMassChannelData;
	FDataObj *m_pModelClimateData;
   FDataObj *m_pETData;
   FDataObj *m_pEnergyBalanceData;
   FDataObj *m_pEnergyBalanceData1;
   FDataObj *m_pCrossSectionData;
   FDataObj *m_pInstreamData;
   FDataObj *m_pGeneralData;
   FDataObj *m_pMassBalanceData;
   FDataObj *m_pUplandStorageData;
	FDataObj *m_pSyntheticqData;
	FDataObj *m_pReachStatistics;
   FDataObj *m_pMonteCarloResults;
   FDataObj *m_pMonteCarloTracerTimeSeries;
   FDataObj *m_pMonteCarloQTimeSeries;
   FDataObj *m_pMonteCarloGWTimeSeries;
   FDataObj *m_pMonteCarloSatAreaTimeSeries;

   FDataObj *m_pMonteCarloPesticideTimeSeries;//kbv 10/26/2006
   FDataObj *m_pInstreamSedimentData;
   FDataObj *m_pUplandMRTData;
   FDataObj *m_pTotalSedimentOutflowData;
   FDataObj *m_pSedimentOutflowData;
   FDataObj *m_pUnsatDepthData;
   FDataObj *m_pSatDepthData;
   FDataObj *m_pSatDeficitData;

   FDataObj *m_pGWDegSatData;
   FDataObj *m_pGWDepthData;
   FDataObj *m_pSatAreaData;
   FDataObj *m_pAverageVegetationData;

   FDataObj *m_pSWEData;  
   FDataObj *m_pMeltData;
   FDataObj *m_pSnowFallData;
   FDataObj *m_pGWMap;

   ReachInfoHydroArray m_reachInfoHydroArray;
   GWArray m_GWArray;
   MASS_BALANCE_INFO m_massBalanceInfo;
//   void UpdateMassBalance();
   float CalcDPrecipVol_dt(UPSLOPE_INFO *pUpslope);
   float CalcDEtVol_dt(UPSLOPE_INFO *pUpslope);
   float CalcDGroundVol_dt(UPSLOPE_INFO *pUpslope); 
   float CalcDGroundLossVol_dt(GW_INFO *pGW);
   float CalcDTracerMassInPrecip_dt(UPSLOPE_INFO *pUpslope);
   float CalcDPesticideMassInput_dt(UPSLOPE_INFO *pUpslope, int i);

   float CalcInitChannelVol();
   void CalcInitSoilVol(float &initSoilVol,float &initUnsatSoilVol, float &initTracerMass, float &initGroundVolume);
   bool CalcFinalChannelVol(float &finalChannelVol, float &finalChannelTracerMass);
   float CalcFinalSoilVol(float &finalSoilVol, float &finalUnsatSoilVol,float &finalPondedVol, float &finalTracerMass, float &finalGroundVolume);
   float CalcFinalSWE();
   float CalcFinalGWVol();
   float CalcInitGWVol();
   float CalcCumulativeGWVol();
   void CalcCumulativeUplandVolume(float &cumETVol, float &cumPrecipVol, float &cumTracerTracerMassInPrecip, float &cumGroundLoss, float &cumGroundToStream, float &systemGroundLoss);
   void CalcCumulativeChannelVolume(float &cumOutflowVol,float &cumTracerOutFlowMass, float &cumSedimentOutflowMass, float &cumGroundInflow);
   void CalcAverageRates(float &averageETRate, float &averageGroundLossRate, float &outflowRate, float &snowStorage, float &soilStorage);
   void CalcCumulativeUplandPest();
   void CalcCumulativeInstreamPest();

   float GetAreaOfSaturation();
   
   //  Landuse Change affects on water quality and hydrology
   void RestoreToModelStructure();
   void ApplyRestorationOptions();
   void ApplyStructuralRestorationOptions();

public:
	UpslopeInfoArray m_cellDataArray;         // cell offsets to collect data on
   UpslopeInfoArray m_cellCrossSectionDataArray;         // array of pointers to upslope infos for cross section plotting
   
   //CollectDataCellOffsetArray m_cellDataArray;
	CWordArray m_reachDataArray;        // reach offsets to collect data on

	CUIntArray m_cellStreamIndexArray;
   CUIntArray *m_2d_Array;

	StreamCellIndexArray m_streamCellIndexArray;
   FPDistributionArray m_fpDistributionArray;
   FPDistributionArray m_fpDistributionArrayStream;
   AirTempArray m_pAirTempArray;
   MET_GRID_INFO m_metGridInfo;
   void GetAverageAirTemp();
 
   UpslopeFor2D m_outputArray; //array of upslopeInfoArrays...for Matlab gaphics

   void AllocateMatlabStorageArray();

   ChemicalParamInfoArray m_chemicalParamInfoArray;
   ChemicalAppInfoArray   m_chemicalAppInfoArray;

   HydrologicParamInfoArray  m_hydrologicParamInfoArray;
   VegetationParamInfoArray m_vegetationParamInfoArray;
   InstreamHydroParamInfoArray m_instreamHydroParamInfoArray;
   GroundwaterParamInfoArray m_groundwaterParamInfoArray;
   void UpdatePesticideConcentration(UPSLOPE_INFO *pUpslope);

    void UpdateMultiTracer(UPSLOPE_INFO *pUpslope);
    void UpdateMultiTracerGW(GW_INFO *pGW);

   PtrArray< UPSLOPE_INFO > m_upslopeInfoArray;
protected:
   // soils information
   SoilInfoArray m_soilInfoArray;
   AuxillaryModelInfoArray m_auxillaryModelInfoArray;
   GoodnessOfFitInfoArray m_goodnessOfFitInfoArray;
   PesticideMassBalanceArray m_pesticideMassBalanceArray;


public:
   int m_colArea;
   int m_colSlope;
   int m_colLengthSlope;
   int m_colZ;
   int m_colWatershedID;
   int m_colStreamID;
   int m_colAbbrev;
   int m_colLulc;
   int m_colLulc_b;
   int m_colSoilDepth;
   int m_colSoilType;
    int m_colGWType;
   int m_colIsRiparian;
	int m_colCentroid;
	int m_colBuffDist;
   int m_colSideOfStream;
   int m_colLtPrecip;
   int m_colOCarbon;
   int m_colKffact;
   int m_colAltLulc;
   int m_colAltArea;
   int m_colAltern;
   int m_colPrecipOffset;
   int m_colGWBC;
   int m_colGeoCode;
   int m_colGeoSource;
   int m_colStandAge;

   BOOL m_checkCStruc;
   BOOL m_checkCEq;
   BOOL m_checkCM;
   BOOL m_checkCKsat;
   BOOL m_checkCInit;
   BOOL m_checkCPhi;
   BOOL m_checkCFC;
   BOOL m_checkCSD;
   BOOL m_checkCKD;
   BOOL m_checkCVan;
   BOOL m_checkCK1;
   BOOL m_checkCK2;
   BOOL m_checkCWD;
   BOOL m_checkCN;
   BOOL m_CEditNumIter;
   BOOL m_CNumIterText;
   BOOL m_checkWP;
   BOOL m_checkPLE2;

   BOOL m_checkCCfmax;
   BOOL m_checkCTT;
   BOOL m_checkCRefreeze;
   BOOL m_checkCCondense;
   BOOL m_checkCCwh; 
   
   // integration support
   BOOL   m_useVariableStep;
   double m_rkvMaxTimeStep; 
   double m_rkvMinTimeStep;
   double m_rkvTolerance;

protected:
   double *m_k1;
   double *m_k2;
   double *m_k3;
   double *m_k4;
   double *m_k5;
   double *m_k6;
   double *m_derivative;
   double *m_svTemp;
   float **m_stateVar;
   float **m_stateVarStream;
   //double *m_surfaceFlow;
   //float **m_2dGrid;

   //Store gridded data for output

public:
   int m_svCount;
   int m_svCountStream;
   int m_uplandUnitCount;
   float m_simulationTime;

   void LoadDistributedAirTemp(); 
   void  GetUpslopeMetData(UPSLOPE_INFO *pUpslope);
   REACH_INFO_HYDRO *GetReachInfoHydroFromReachID( int reachID );

   int   GetReachCount();

   float m_maxGWDepth;
   float m_minGWDepth;

	float GetKinematicWaveCelerityFromDepth( ReachNode *pNode , float depth);
   int   GetCellCount();
   int   GetCellReachID( int cell );
   float GetCellArea( int cell );
   float GetCellSlope( int cell );
   float GetCellDepth( int cell );
   int   GetCellLulc( int cell );
   int   GetCellLulc_b( int cell );
   int   GetCellPrecipOffset( int cell );
   bool  GetGWBC(int cell);
	float GetCellCentroidDistance( int cell );
   float GetCellSoilDepth( int cell );
   int   GetCellIsRiparian( int cell );
	float GetCellBufferDistance( int cell );
   int   GetCellSideOfStream( int cell );
   float GetCellLtPrecip( int cell );
   float GetCellOCarbon( int cell );
   float GetCellLengthSlope( int cell );
   float GetCellKffact( int cell );
   int   GetCellAltLulc( int cell );
   float GetCellAltArea( int cell );
   int   GetCellAltern( int cell );

   UPSLOPE_INFO *GetCellUpslopeInfo( int cell );

   void CalcGridFlowFractions();
   void CalcCellFlowFractions( int row, int col, bool useSurface );

   // upslope model stuff
   int   RunUpslopeModel( double time );
   float GetSurfaceInOutFlows( int row, int col, float &totalInflow, float &totalOutflow );
   void  GetSurfaceFlowFractions( int row, int col, float inflowFraction[], float outflowFraction[] );
   float CalcET ( float precip, float temp, double time );
   float PenmanMonteith(float T, float dewPoint, float solarRad, float windSpeed, int ageClass);
   float CalcHoltanF( float awa );
   float CalcSW( float precip, float temp, int datejul, int i  );
   int GetDateJulFromTime(float time);
   void SimulateET( int method, UPSLOPE_INFO *pUpslope, float curTime=0);
   void GetCanopyStorage(VEGETATION_INFO *pVegetation, MET_INFO *pMet, int story);
   void GetPenmanMonteithET(UPSLOPE_INFO *pUpslope, MET_INFO *pMet, VEGETATION_INFO *pVegetation,  int story);
   void DHSVM_RadiationBalance();
   float EstimateRl(MET_INFO *pMet);
   float EstimateRs();
   float  LongwaveRadiationFromHandbookHydrology(UPSLOPE_INFO *pUpslope);
   float IncomingLongwavePomeroy(MET_INFO *pMet);
   float GetIncomingSolar(float temp);
   // instream model stuff

   int   RunInstreamModel( double time );
   bool  SolveReach( ReachNode *pNode );
   bool  SolveReachSolute( ReachNode *pNode );
   bool  SolveReachPesticide( ReachNode *pNode );
   bool  SolveReachMultiTracer( ReachNode *pNode );

   ParticleArray m_particleArray;

   // model setup, etc.
   void  InitRun( void ) ;
   void  CollectData( double time );
   void  CollectDataSingleRun( );
   void  CalcInitGridTracerMass();
   void  CalcInstreamResidenceTime();
   void  CalcUpslopeResidenceTime();
   void  CalcUpslopeEndMemberConcentration();
   void CollectDataGrid();
   void CollectDataSemi(); 
   void CollectDataInstream();

   UPSLOPE_INFO *CollectDataGetUpslope(int i, int &count);
   UPSLOPE_INFO *CollectDataGetGridUpslope(int i, int &count );
   UPSLOPE_INFO *CollectDataGetSemiUpslope(int i );

   COORD2d_Array m_xyCoordArray;
   SampleLocationInfoArray m_sampleLocations;
   COORD2d_Array m_xyCoordCrossSectionArray;
   StreamSampleLocationInfoArray m_streamSampleLocations;

   int CollectGetNumGrid(void);
   int CollectGetNumInstream(void);
   int CollectGetNumSemi(void);
   


   void  CollectDataUpslopeWater(  int dataToCollect );
   void  CollectDataUpslopeTracer(   int dataToCollect );
   void  CollectDataUpslopeSnow(int dataToCollect);
   void  CollectDataUpslopeMultiTracer(   int dataToCollect );
   void  CollectDataUpslopePesticide(   int dataToCollect );
   void  CollectDataInstreamMultiTracer(   int dataToCollect );
   void  CollectDataInstreamWater(  int dataToCollect );
   void  CollectDataInstreamTracer(   int dataToCollect );
   void  CollectDataInstreamPesticide(   int dataToCollect );
   void  CollectDataMassBalance(  );
   void  CollectDataUpslopeAverageEnergyBalance();

   int  CollectMonteCarloData(double time);

   void  BuildSamplingArray();

   void  SetPreviousStorageTerms();

   int SaveStateForInitialConditions();
   int ExportResults();
   int ReadState();

   void  EndRun( );
   void  EndRunMC();
   void  GetGoodnessOfFitValues();
   float GetNashSutcliffe();
   float GetDepthFromQ( ReachNode *pNode, float Q, float wdRatio );   // note: this function is closely coupled with SetReachGeometry
   float GetVolumeFromQ( ReachNode *pNode, float Q, float wdRatio);
   void  SetReachGeometry( ReachNode *pNode, int node, float wdRatio );
   SOIL_INFO *GetSoilParameters( int cell );
   AUXILLARY_MODEL_INFO * GetAuxillaryModelParameters( int cell);

   bool  IntRKF( double curTime, int svCount, float **stateVar, bool modelToIntegrate );
   void  GetDerivatives(  double *derivative, double curTime );
   void  GetDerivativesParallel(  double *derivative, double curTime );
   void  GetInstreamDerivatives(  double *derivativeInstream, double curTime );
   void  UpdateAuxillaryVars();
   void  UpdateAuxillaryVarsPrecip();
   void  InitializeMultiTracer();
   void  UpdateRates(double time);
   void  UpdateSurfaceWaterExchangeRates(double timeStep);
   void  UpdateGWRates();
   void  UpdateInstreamAuxillaryVars();
   void  CalcVolumeToStream();
   void  RunAuxillaryInstreamModel(double time);


   //  After integrating, these methods calculate auxillary values based on volumes
   void  CalcSatDeficit(UPSLOPE_INFO *pUpslope) ;  
 
   float GetDegSatFromSWC( UPSLOPE_INFO *pUpslope );
   void  UpdateInputs(UPSLOPE_INFO *pUpslope, double time);
   void  CalcStorage(UPSLOPE_INFO *pUpslope);
   void  CalcVoidVolume(UPSLOPE_INFO *pUpslope);
   void  CalcInitSat(UPSLOPE_INFO *pUpslope);
   void  CalcVanGenuctenKTheta(UPSLOPE_INFO *pUpslope);
   void  CalcBrooksCoreyKTheta(UPSLOPE_INFO *pUpslope);
   void  CalcSatZone(UPSLOPE_INFO *pUpslope) ; 
   void  CalcUnsatZone(UPSLOPE_INFO *pUpslope) ; 
   void  CalcKSat(UPSLOPE_INFO *pUpslope) ; 
   void  CalcAccountForZeroArea(UPSLOPE_INFO *pUpslope) ; 
   void  CalcPondedVolume(UPSLOPE_INFO *pUpslope) ; 

   void  GetInfiltrationRate(UPSLOPE_INFO *pUpslope,double timeStep);

   void  CalcSatWaterFluxes(UPSLOPE_INFO *pUpslope);
   void  CalcGroundWaterFluxes(GW_INFO *pGW);
   void  CalcGroundWaterRecharge(GW_INFO *pGW);
   void  UpdateGridSlopeMultiTracer(UPSLOPE_INFO *pUpslope);
   void  UpdateGridSlopeMultiTracerGW(GW_INFO *pGW);

   void  CalcGroundToStream(GW_INFO *pUpslope);
   void  CalcGWInitSat(GW_INFO *pGW);

   void  CalcD8Flux(UPSLOPE_INFO *pUpslope);
   void  UpdatePesticideWaterVolume(UPSLOPE_INFO *pUpslope);
   void  CalcSatPestFluxes(UPSLOPE_INFO *pUpslope);
   
   void  CalcPesticideTransferToStream(UPSLOPE_INFO *pUpslope);

   float m_initSoilSatVol;

   float m_initSoilUnsatVol;
   float m_initGroundVol;

protected:
   WH_NOTIFYPROC m_notifyFn;
   long m_extra;
   long m_extra1;
   bool Notify( WH_NOTIFY_TYPE t, float time, float timeStep, long l, long l1) { if ( m_notifyFn != NULL ) return m_notifyFn( this, t, (float)time, (float)timeStep , m_extra , m_extra1); else return true; }

public:
   WH_NOTIFYPROC InstallNotifyHandler( WH_NOTIFYPROC p, long extra=0L ) { WH_NOTIFYPROC temp = m_notifyFn; m_notifyFn = p; m_extra = extra; return temp; }
/*   int   GetUpslopeDerivatives( double *derivative , double curTime);*/
int GetNeighbors(int colToSubset, LPCSTR filename1 , LPCSTR filename2, LPCSTR filename3,LPCSTR filename4);

   // movie stuff follows
   //CAVIGenerator** AVIGenerators;
   struct AuxMovieStuff* auxMovieStuff;
   int numMovies, movieFrameRate;

   void WriteFrame();

   int GetCellGeoCode(int cell);
   int GetCellGeoSource(int cell);
   int GetCellStandAge(int cell);
   int CreateLumped(void);
   int CreateFPGrid(void);
   int CreateGWModel(void);
   MapLayer *AggregateGrid(int factor, MapLayer *pGridToAggregate, STATISTIC stat);
   void ConnectUpslopeToGW(FDataObj *pGWMap);
   int GetGeoSourceCount( void );

};

// this is all just for precalculating and storing things to avoid doing it per-frame
struct AuxMovieStuff
{
   CDC bitmapDC;
   CBitmap bitmap;
   BITMAPINFO bi;
   LPBITMAPINFO lpbi;
   BYTE* bitAddress;
};


#endif // !defined(AFX_WET_Hydro_H__C9203D32_B4B1_11D3_95C1_00A076B0010A__INCLUDED_)

