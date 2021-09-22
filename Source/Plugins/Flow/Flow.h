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
#include <EnvExtension.h>

#include "FlowContext.h"
#include "GlobalMethods.h"

#include <EnvExtension.h>
#include <MapLayer.h>
#include <reachtree.h>
#include <FDataObj.h>
#include <PtrArray.h>
#include <QueryEngine.h>
#include <Vdataobj.h>
#include <IntegrationBlock.h>
#include <DataAggregator.h>
#include <MapExprEngine.h>
#include "GDALWrapper.h"
#include "GeoSpatialDataObj.h"
#include <randgen\Randunif.hpp>
#include <randgen\Randnorm.hpp>
#include <exception>

using namespace std;


#ifdef BUILD_FLOW
#define FLOWAPI __declspec( dllexport )
#else 
#define FLOWAPI __declspec( dllimport )
#endif


/*! \mainpage A brief introduction to Flow:  A framework for the development of continuous-time simulation models within Envision
 *
 * \section intro_sec Introduction
 *
 * Flow is a modeling framework that allows a user of Envision to develop spatially and temporally distributed simulation models that can take advantage...
 * The user interacts with the framework through an xml-based format that allows for the definition of state variables, fluxes and solution procedures.

 * Basic idea - provides a plug-in framework for hydrologic modeling.  The framework assumes that
 * that the baisc construct for hydrologic modeling consists of catchments that intercept water and route that
 * water to a stream network.  Catchments can have any number of fluxes that contribute or remove
 * water from the catchment
 *
 * \section install_sec The Flow framework provides the following:
 *
 * \par 1. Two spatial representations relevant to hydrological modeling
 *
 * -# an instream linear network consisting of a multi-rooted tree
 * -# an upslope catchment coverage, consisting of polygons defined by a shape file
 *  
 * \par 2: a set of interface methods for solving the system of ODEs that results
 *
 *  etc.
 */



// Basic idea - provides a plug-in framework for hydrologic modeling.  The framework assumes that
// that the baisc construct for hydrologic modeling consists of catchments that intercept water and route that
// water to a stream network.  Catchments can have any number of fluxes that contribute or remove
// water from the catchment

// The Flow framework provides the following:
//
// 1) Two spatial representations relevant to hydrol modeling 
//    - a instream linear network consisting of a multi-rooted tree
//    - an upslope catchment coverage, consisting of polygons defined be a shape file
//
// 2) a set of interface methods for 





//--------------------------------------------------------------------------------------------------
// Network simplification process:
//
// basic idea: iterate through the the stream network starting with the leaves and
// working towards the roots.  For each reach, if it doesn't satisfy the stream query
// (meaning it is to be pruned), move its corresponding HRUs to the first downstream 
// that satisfies the query.
//
// The basic idea of simplification is to aggregate Catchments using two distinct methods: 
//   1) First, aggregate upstream catchments for any reach that does not satisify a
//      user-defined query.  This is accomplished in SimplifyTree(), a recursive function 
//      that iterates through a reach network, looking for catchments to aggregate based 
//      on a reach-based query.  For any reach that DOES NOT satisfy the query, all catchments 
//      associated with all upstream reaches are aggregated into the catchment associated with
//      the reach with the failed query.  
//
//      SimplifyTree() combines upstream catchments (deleting pruned Catchments) and prunes 
//           all upstream Reaches (deleted) and ReachNodes (converted to phantom nodes).
//
//   2) Second, minimum and/or maximum area constraints area applied to the catchments.  This is 
//      an iterative process that combines Catchments that don't satisfy the constraints.
//      The basic rule is:
//         if the catchment for this reach is too small, combine it with another
//         reach using the following rules:
//           1) if one of the upstream catchment is below the minimum size, add this reaches
//              catchment to the upstream reach.  (Note that this works best with a root to leaf search)
//           2) if there is no upstream capacity, combine this reach with the downstream reach (note
//              that this will work best with a leaf to root, breadth first search.)
//
//       ApplyCatchmentConstraints() applies these rules.  For any catchment that is combined,
// 
//--------------------------------------------------------------------------------------------------

class Reach;
class Reservoir;
class HRUPool;
class FlowModel;
class HRU;
class Catchment; 
class StateVar;
class VideoRecorder;
class ResConstraint;

class Gridcell;

#include <MovingWindow.h>


// global functions
void SetGeometry( Reach *pReach, float discharge);
float GetDepthFromQ( Reach *pReach, float Q, float wdRatio );  // ASSUMES A SPECIFIC CHANNEL GEOMETRY
FLUXSOURCE ParseSource( LPCTSTR sourceStr, CString &path, CString &source, HINSTANCE &hDLL, FLUXFN &fn );
float GetVerticalDrainage( float wc );
float GetBaseflow(float wc);
float CalcRelHumidity(float specHumid, float tMean, float elevation);

// exception handling

//struct FlowException : public std::exception
//   {
//   TCHAR msg[ 256 ];
//   const TCHAR *what () const throw ()
//   {
//   return "Flow Exception";
//   }
//};
 
//int main()
//{
//  try
//  {
//    throw MyException();
//  }
//  catch(MyException& e)
//  {
//    std::cout << "MyException caught" << std::endl;
//    std::cout << e.what() << std::endl;
//  }

class PoolInfo
{
public:
   CString m_name;
   POOL_TYPE m_type;
   float m_depth;
   float m_initWaterContent;
   float m_initWaterTemp;

public:
   PoolInfo() : m_type( PT_UNDEFINED ), m_depth(0), m_initWaterContent(0), m_initWaterTemp(0) {}
   };



// a FluxContainer is a object (e.g. HRU, Reach) that can contain fluxes. 
class FluxContainer
{
friend class Flow;

protected:
    FluxContainer() : m_globalHandlerFluxValue( 0.0f ) { }
   ~FluxContainer() { }

protected:
   float m_globalHandlerFluxValue;        // current value of the global flux  - m3/day

public:
   virtual void AddFluxFromGlobalHandler(float value, WFAINDEX index = FL_UNDEFINED)
      { m_globalHandlerFluxValue += value; }   // negative values are sinks, positive values are sources (m3/day)

   float GetFluxValue( ) { return m_globalHandlerFluxValue; } 
   void  ResetFluxValue( ) { m_globalHandlerFluxValue=0.0f;} 
};


class FLOWAPI ParamTable
   {
   friend class FlowModel;
   friend class FlowProcess;

   protected:
      ParamTable(void) : m_pTable(NULL), m_pInitialValuesTable(NULL), m_iduCol(-1), m_type(DOT_NULL) { }

   public:
      ~ParamTable(void) { if (m_pTable) delete m_pTable; /* if ( m_pInitialValuesTable ) delete m_pInitialValuesTable; */ }

   protected:
      DataObj *m_pTable;
      DataObj *m_pInitialValuesTable;

   protected:
      CMap< VData, VData&, int, int > m_lookupMap;
   public:
      int CreateMap();

      CString m_description;
      CString m_fieldName;    // IDU fieldname for lookups
      int     m_iduCol;       // IDU column for lookups
      DO_TYPE m_type;  //  { DOT_VDATA, DOT_FLOAT, DOT_INT, DOT_NULL, DOT_OTHER };

      // external methods 
      DataObj *GetDataObj(void) { return m_pTable; }
      DataObj *GetInitialDataObj(void) { return m_pInitialValuesTable; }

      int GetFieldCol(LPCTSTR field) { if (m_pTable) return m_pTable->GetCol(field); else return -1; }

      bool Lookup(int key, int col, int   &value) { return Lookup(VData(key), col, value); }
      bool Lookup(int key, int col, float &value) { return Lookup(VData(key), col, value); }

      bool Lookup(VData key, int col, int   &value);
      bool Lookup(VData key, int col, float &value);
   };

//class ExternalMethod
//{
//public:
//   CString m_name;
//   CString m_path;
//   //CString m_entryPt;
//
//   CString m_entryPtInit;
//   CString m_entryPtInitRun;
//   CString m_entryPtRun;
//   CString m_entryPtEndRun;
//   CString m_initInfo;
//
//   int     m_timing; 
//   bool    m_use;
//
//   HMODULE       m_hDLL;
//   FLOWINITFN    m_initFn;
//   FLOWINITRUNFN m_initRunFn;
//   FLOWRUNFN     m_runFn;
//   FLOWENDRUNFN  m_endRunFn;
//
//   ExternalMethod( void ) : m_timing( 2 ), m_use( true ), m_hDLL( 0 ), m_initFn( NULL ), 
//      m_initRunFn( NULL ), m_runFn( NULL ), m_endRunFn( NULL ) { }
//
//};
//

class StateVar
{
public: 
   CString m_name;
   int     m_uniqueID;
   int     m_index;

   bool    m_isIntegrated;
   float   m_scalar;
   int     m_cropCol;
   int     m_soilCol;
   VDataObj *m_paramTable; //this table stores parameters.  
   PtrArray <ParamTable> m_tableArray;//the array stores tables for each application strategy for the pesticide
   //it maps back to the idu (crop) and so is a ParamTable, rather than a dataobj

   StateVar(void) : m_uniqueID(-1), m_isIntegrated(true), m_scalar(1.0f), m_cropCol(-1), m_soilCol(-1){}
   };


//-- StateVarCountainer ----------------------------------------
//
// stores arrays of state variables, manages state var memory
// First state variable is always quantity of water, units=m3
//--------------------------------------------------------------

class StateVarContainer
{
friend class FlowModel;
friend class ReachRouting;

protected:
   StateVarContainer() : m_svArray( NULL ), m_svArrayTrans( NULL ), m_svIndex( -1 ) { }
   ~StateVarContainer() { if ( m_svArray != NULL ) delete [] m_svArray; if ( m_svArrayTrans != NULL ) delete [] m_svArrayTrans; }

protected:
   SVTYPE *m_svArray;       // contains state variables (memory managed here)
   int     m_svIndex;       // index in the associated integration block for the first statevar contained by this container
   
public:
   SVTYPE  GetStateVar( int i ) { return m_svArray[ i ]; }
   void    SetStateVar( int i, SVTYPE value ) { m_svArray[ i ] = value; }

   int   AllocateStateVars( int count ) { ASSERT( count > 0 ); if ( m_svArray != NULL ) delete [] m_svArray; if ( m_svArrayTrans != NULL ) delete [] m_svArrayTrans; m_svArray = new SVTYPE[ count ]; m_svArrayTrans = new SVTYPE[ count ]; return count; }
   SVTYPE *m_svArrayTrans;   // ???? (memory managed here)
   void  AddExtraSvFluxFromGlobalHandler(int svOffset, float value ) { m_svArrayTrans[svOffset] += value; } 

   float GetExtraSvFluxValue(int svOffset ) { return (float)m_svArrayTrans[svOffset]; } 
   void  ResetExtraSvFluxValue(int svOffset ) { m_svArrayTrans[svOffset]=0.0f; } 
   };


// groundwater
class FLOWAPI Groundwater : public FluxContainer
{
public:
   SVTYPE m_volumeGW;    // m3
   
   Groundwater( void ) : m_volumeGW(0.0f) { } 
};


class ResConstraint
{
public:
   ResConstraint( void ) : m_pRCData( NULL ) /*, m_pControlPoint( NULL )*/ { }
   //ResConstraint( const ResConstraint &rc ) { *this = rc; }

   ~ResConstraint( void ) { if ( m_pRCData != NULL ) delete m_pRCData; }

   //ResConstraint &operator = ( const ResConstraint &rc )
   //   { m_constraintFileName = rc.m_constraintFileName; m_pRCData = NULL;  if( rc.m_pRCData ) m_pRCData = new FDataObj( *m_pRCData );
   //     m_type = rc.m_type; m_comID = rc.m_comID;  return *this; }


   CString   m_constraintFileName;
   FDataObj *m_pRCData;          // (memory managed here)
   //Reach    *m_pControlPoint;
   RCTYPE     m_type;   //Type = min (1) , max (2) , IncreasingRate (3), DecreasingRate (4)
   int       m_comID;
  
};


class ZoneInfo
{
public:
   ZONE m_zone;
   //CArray< ResConstraint*, ResConstraint* > m_resConstraintArray;  // (memory managed elsewhere)
   PtrArray< ResConstraint > m_resConstraintArray;    ///???

   ZoneInfo( void ) : m_zone( ZONE_UNDEFINED ) { m_resConstraintArray.m_deleteOnDelete = false; }
};


//-----------------------------------------------------------------------------
//---------------------------- H R U L A Y E R   ------------------------------
/// Represents the fundamental unit of calculation for upslope regions.  An HRU
/// is comprised of n HRULayers representing project specific state variables.
/// These may include water on vegetation, snow, water in a particular soil layer,
/// etc.  They are specified based upon project-specific spatial queries
//-----------------------------------------------------------------------------

/// The fundamental unit describing upslope state variables

class FLOWAPI HRUPool : public FluxContainer, public StateVarContainer
{
public:
    HRUPool();
    ~HRUPool( void ) { }
   // State Variables
   SVTYPE m_volumeWater;                // m3
   float m_depth ;  //total depth of the HRUPool

   CArray< float, float > m_waterFluxArray;  // for gridded state variable representation

   // additional variables
   float m_contributionToReach;
   float m_verticalDrainage;
   float m_horizontalExchange;//only used for grids.  Populated by Flow PlugIn
   
   // calculated stuff
   float m_wc;                         // water content (mm/mm)
   float m_wDepth;                     // water content (mm)

   // summary fluxes
   float m_volumeLateralInflow;        // m3/day
   float m_volumeLateralOutflow;
   float m_volumeVerticalInflow;
   float m_volumeVerticalOuflow;

   // general info
   HRU *m_pHRU;            // containing HRU    (memory managed in FlowModel::m_hruArray)
   int  m_pool;            // zero-based layer number - 0 = first pool
   POOL_TYPE m_type;   // soil, snow or veg
   
   // methods for computed variables
   //float GetSaturatedDepth();       // m
   //float GetSWC();                  // m3/day                     // soil water content

   static MTDOUBLE m_mvWC;
   static MTDOUBLE m_mvWDepth;
   static MTDOUBLE m_mvESV;
   
public:
   HRUPool( HRU *pHRU );
   HRUPool( HRUPool &l ) { *this=l; }
   
   HRUPool & operator = ( HRUPool &l );

   Reach *GetReach( void );

   virtual void AddFluxFromGlobalHandler( float value, WFAINDEX index  );  // m3/day
  
   float GetExtraStateVarValue( int k ) {return (float)m_svArray[k];}
   };


//-----------------------------------------------------------------------------
//----------------------------------- H R U -----------------------------------
//-----------------------------------------------------------------------------
/// HRU - fundamental unit of spatial representation.  This corresponds to a 
/// collection of similar polygons in the Maplayer. It contains 1 or more 
/// HRULayers that contain the relevant state variable information for the model
//-----------------------------------------------------------------------------
/// A collection of HRU Layers.

class FLOWAPI HRU
{

public:
   HRU();
   ~HRU();  

   int m_id;            // unique identifier for this HRU
   float m_area;        // area of the HRU

   // climate variables
   float m_precip_wateryr;    // mm/year
   float m_precip_yr;         // mm/year, accumulates throughout the year
   MovingWindow m_precip_10yr;   // mm/year (avg)
   float m_precipCumApr1;     // mm accumulated from Jan 1 through April 1

   float m_gwRecharge_yr;       // mm
   float m_gwFlowOut_yr;

   float m_rainfall_yr;       // mm/year, accumulate throughout the year
   float m_snowfall_yr;       // mm/year, accumulate throughout the year
   float m_areaIrrigated;

   float m_temp_yr;           // C, average over year
   MovingWindow m_temp_10yr;     // C, 20 year mov avg

   float m_soilTemp;
   float m_biomass;
   float m_swc;
   float m_irr; //irrigation rate

   // SWE-related variables
   float     m_depthMelt;          // depth of water in snow (m)
   float     m_depthSWE;           // depth of ice in snow (m)
   float     m_depthApr1SWE_yr;    // (m)
   MovingWindow m_apr1SWE_10yr;       // 20 year moving avg (m)
   
   MovingWindow m_aridityIndex;       // 20 year moving avg
   
   Vertex m_centroid;   //centroid of the HRU
   float m_elevation;
   float m_currentPrecip;     // UNITS????
   float m_currentRainfall;
   float m_currentSnowFall;
   float m_currentAirTemp;
	float m_currentMinTemp;
   float m_currentMaxTemp;
   float m_currentRadiation;
   float m_currentET;         // mm/day
   float m_currentMaxET;      // mm/day
	float m_currentCGDD;       // Celsius
   float m_currentRunoff;     // mm/day
   float m_currentGWRecharge; // mm/day
   float m_currentGWFlowOut;  // mm/day
   float m_currentSediment;
   float m_maxET_yr;          // mm/year
   float m_elws;                //elevation (map units) of the water surface
   float m_et_yr;             // mm/year      
   float m_runoff_yr;         // mm/year
   float m_initStorage;
   float m_endingStorage;     // UNITS?????
   float m_storage_yr;        // mm/year?
   float m_percentIrrigated;  // fraction of area (0-1)
   float m_meanLAI;           // dimensionless
   float m_meanAge;

   int m_climateIndex;  //the index of the climate grid cell representing the HRU
   int m_climateRow;    // row, column for this HRU in the climate data grid
   int m_climateCol;

   int m_demRow;  //for gridded state variable representation
   int m_demCol;  //for gridded state variable representation
   CArray< float, float> m_waterFluxArray;  //for gridded state variable representation
   bool m_lowestElevationHRU;

   Catchment *m_pCatchment;        // (memory managed in FlowModel::m_catchmentArray)
   
   // static PtrArray< PoolInfo > m_poolInfoArray;     // just templates, actual pools are in m_poolArray

   PtrArray< HRUPool > m_poolArray;          // (memory managed locally)
   CArray< HRU*, HRU* > m_neighborArray;     // (memory managed by FlowModel::m_hruArray)
   CUIntArray m_polyIndexArray;

   // the following static members hold values for model variables
   static MTDOUBLE m_mvDepthMelt;  // volume of water in snow
   static MTDOUBLE m_mvDepthSWE;   // volume of ice in snow
   static MTDOUBLE m_mvCurrentPrecip;
   static MTDOUBLE m_mvCurrentRainfall;
   static MTDOUBLE m_mvCurrentSnowFall;
   static MTDOUBLE m_mvCurrentRecharge;
   static MTDOUBLE m_mvCurrentGwFlowOut;
   static MTDOUBLE m_mvCurrentAirTemp;
	static MTDOUBLE m_mvCurrentMinTemp;
   static MTDOUBLE m_mvCurrentMaxTemp;
   static MTDOUBLE m_mvCurrentRadiation;
   static MTDOUBLE m_mvCurrentET;
   static MTDOUBLE m_mvCurrentMaxET;
	static MTDOUBLE m_mvCurrentCGDD;
   static MTDOUBLE m_mvCumET; 
   static MTDOUBLE m_mvCumRunoff;
   static MTDOUBLE m_mvCumMaxET;   
   static MTDOUBLE m_mvCumP; 
   static MTDOUBLE m_mvElws; 
   static MTDOUBLE m_mvDepthToWT;
   static MTDOUBLE m_mvCurrentSediment;

   static MTDOUBLE m_mvCurrentSWC;
   static MTDOUBLE m_mvCurrentIrr;

   static MTDOUBLE m_mvCumRecharge;
   static MTDOUBLE m_mvCumGwFlowOut;

public:
   int GetPoolCount() { return (int) m_poolArray.GetSize(); }
   HRUPool *GetPool( int i ) { return m_poolArray[ i ]; }
   int AddPools( FlowModel *pFlowModel,  /* int pools, float initWaterContent, float initTemperature,*/ bool grid );
   //int AddLayers( int soilLayerCount, int snowLayerCount, int vegLayerCount, float initWaterContent, float initTemperature, bool grid );

};



//-----------------------------------------------------------------------------
//---------------------------- C A T C H M E N T ------------------------------
//-----------------------------------------------------------------------------
/// A catchment consists of one or more HRUs, representing areal 
/// subdivision of the catchment.  It is a distinct watersheds.  It may be 
/// connected to a groundwater system.
/// They may also be connected to zero or one reaches.
//-----------------------------------------------------------------------------

/// A collection of model units that are connected to a reach.

class FLOWAPI Catchment
{
public:
   // data - static
   int   m_id;
   float m_area;

   // data - dynamic
   float m_currentAirTemp;
   float m_currentPrecip;
   float m_currentThroughFall;
   float m_currentSnow;
   float m_currentET;
   float m_currentGroundLoss;
   float m_meltRate;
   float m_contributionToReach;    // m3/day
   int m_climateIndex;
   CArray< HRU*, HRU* > m_hruArray;     // (memory managed in FlowModel::m_hruArray)


protected:
   // layers
   // topology - catchments (not currently used)
   CArray< Catchment*, Catchment* > m_upslopeCatchmentArray;      // (memory managed in FlowModel::m_catchmentArray)
   Catchment *m_pDownslopeCatchment;

   // topology - reach
public:
   Reach *m_pReach;     // ptr to associated Reach.  NULL if not connected to a reach  (memory managed by FlowModel::m_reachArray)
                        
   // topology - groundwater
   Groundwater *m_pGW;  // not used????  managed by???

public:
   // Constructor/destructor
   Catchment( void );

   int  GetHRUCount( void )   { return (int) m_hruArray.GetSize(); }
   //int  AddHRU( HRU *pHRU )   { m_hruArray.Add( pHRU ); pHRU->m_pCatchment = this; return (int) m_hruArray.GetSize(); }
   HRU *GetHRU( int i )       { return m_hruArray[ i ]; }
   //void RemoveHRU( int i )    { return m_hruArray.RemoveAt( i ); }
   //void RemoveAllHRUs( void ) { m_area = 0; m_hruArray.RemoveAll(); }

   void ComputeCentroid( void );
};


class ReachSubnode : public SubNode, public StateVarContainer
{
public:
   SVTYPE m_volume;
   SVTYPE m_previousVolume;
   float  m_lateralInflow;       // m3/day
   float  m_discharge;           // m3/sec;
   float  m_previousDischarge;   // m3/sec;
   float  m_temperature;         // degrees C

   static SubNode *CreateInstance( void ) { return (SubNode*) new ReachSubnode; }

   ReachSubnode(void) : SubNode(), StateVarContainer(), m_discharge(0.11f), m_previousDischarge(0.11f), m_volume(0.0f), m_previousVolume(0.0f), m_temperature(0.0f) { }
   virtual ~ReachSubnode( void ) { }
};



class FLOWAPI Reach : public ReachNode, public FluxContainer          // extends the ReachNode class
{
public:  
   Reach();
   virtual ~Reach();

   ReachSubnode *GetReachSubnode( int i ) { return (ReachSubnode*) m_subnodeArray[ i ]; }
   static ReachNode *CreateInstance( void ) { return (ReachNode*) new Reach; }

   Reservoir *GetReservoir( void ) { return (Reservoir*) m_pReservoir; }
   void  SetGeometry( float wdRatio );
   float GetDepthFromQ( float Q, float wdRatio );  // ASSUMES A SPECIFIC CHANNEL GEOMETRY
   float GetDischarge( int subnode=-1 );  // -1 mean last (lowest) subnode - m3/sec
   float GetUpstreamInflow(); 
   bool GetUpstreamInflow(float &QLeft, float &QRight);
   bool GetUpstreamTemperature(float &tempLeft, float &tempRight);
   float GetCatchmentArea( void );

public:
   // reach-level parameters
   float m_width;             // width of the stream reach (m)   ASSSUMES RECTANGULAR
   float m_depth;             // depth of the stream reach (m)
   float m_wdRatio;
   float m_cumUpstreamArea;   // cumulative upslope area, in map units, above this reach in the network
   float m_cumUpstreamLength;
   float m_instreamWaterRightUse;  //used to keep track of instream water right use for this reach
   float m_availableDischarge;//keep track of the amount of water diverted from the reach during each timestep
	float m_currentStreamTemp;

   int m_climateIndex;

   // kinematic wave parameters
   float m_alpha;
   float m_beta;
   float m_n;              // Mannings "N"

   // summary variables
   //float m_lateralInflow;         // flow rate into this reach
   float m_meanYearlyDischarge;     // ft3/sec (cfs)
   float m_maxYearlyDischarge;    
   float m_sumYearlyDischarge;

   bool m_isMeasured;

   CArray< Catchment*, Catchment* > m_catchmentArray;

   Reservoir *m_pReservoir;            // (memory managed in FlowModel::m_reservoirArray)

   FDataObj *m_pStageDischargeTable;   // (memory managed here)

   // the following static members hold value for model variables
   static MTDOUBLE m_mvCurrentStreamFlow;
	static MTDOUBLE m_mvCurrentStreamTemp;
   static MTDOUBLE m_mvCurrentTracer;
};

enum ResType
{
	ResType_FloodControl = 1,
	ResType_RiverRun = 2,
	ResType_CtrlPointControl = 4,
   ResType_Optimized = 8
};

class FLOWAPI Reservoir : public FluxContainer, public StateVarContainer
{
friend class FlowModel;

public:
   Reservoir(void);
   ~Reservoir(void);	

   // XML variables
   CString m_name;
   int     m_id;
   CString m_dir;
   CString m_avdir;
   CString m_rcdir;
   CString m_redir;
   CString m_rpdir;
   CString m_cpdir;
   int     m_col;
   float   m_dam_top_elev;       // Top of dam elevation
   float   m_inactive_elev;      // Inactive elevation. Below this point, no water can be released
   float   m_fc1_elev;           // Top of primary flood control zone
   float   m_fc2_elev;           // Seconary flood control zone (if applicable)
   float   m_fc3_elev;           // Tertiary flood control zone (if applicable)
   float   m_buffer_elevation;   //Below this elevation, rules for buffer zone apply
   float   m_tailwater_elevation;
   float   m_turbine_efficiency;
   float   m_minOutflow;
   float   m_maxVolume;
	int	  m_releaseFreq;        // how often the reservoir will release averages over this number of days
   
   float   m_gateMaxPowerFlow;   //Physical limitation of gate
   float   m_maxPowerFlow;       //Operations limit, depends on rules applied
   float   m_minPowerFlow;       //Operations limit, depends on rules applied
   float   m_powerFlow;          //Flow through Powerhouse in current timestep
   
   float   m_gateMaxRO_Flow;     //Physical limitation of gate
   float   m_maxRO_Flow;         //Operations limit, depends on rules applied
   float   m_minRO_Flow;         //Operations limit, depends on rules applied
   float   m_RO_flow;            //Flow through Regulating Outlet in current timestep
   
   float   m_gateMaxSpillwayFlow;//Physical limitation of gate
   float   m_maxSpillwayFlow;    //Operations limit, depends on rules applied
   float   m_minSpillwayFlow;    //Operations limit, depends on rules applied
   float   m_spillwayFlow;       //Flow through Spillway in current timestep

	ResType m_reservoirType;      //  enumerated type which indicates Reservoir Type 
   LPCTSTR m_activeRule;           //Rule that is controlling release (last implemented).
   int m_zone;
   int m_daysInZoneBuffer;

   float   m_inflow;
   float   m_outflow;
   float   m_elevation;          //current pool elevation
   float   m_power;

   int     m_filled;  // -1 means never reached, updated annually

   float m_probMaintenance;

   /////////////////////Read in from ResSIM output files for model comparison.  mmc 5/22/2013///////////
   float m_resSimInflow;
   float m_resSimOutflow;
   float m_resSimVolume;
   CString resSimActiveRule;
   ///////////////////////////////////////////////

   SVTYPE m_volume;   // this is the same as m_svArray[ 0 ].

   CString m_areaVolCurveFilename;
   CString m_ruleCurveFilename;
   CString m_bufferZoneFilename;
   CString m_releaseCapacityFilename;
   CString m_RO_CapacityFilename;
   CString m_spillwayCapacityFilename;
	CString m_rulePriorityFilename; 
   CString m_ressimFlowOutputFilename;     //Inflow, outflows and pool elevations
   CString m_ressimRuleOutputFilename;     //Active rules
   

   bool ProcessRulePriorityTable( EnvModel*, FlowModel*);
	void InitializeReservoirVolume(float volume) { m_volume = volume; }
   
   PtrArray< ZoneInfo > m_zoneInfoArray;              // (memory managed here)
   PtrArray< ResConstraint > m_resConstraintArray;    // (memory managed here)

protected:
   CArray<Reach*, Reach*> m_reachArray;   // reaches associated with this reservoir
   Reach *m_pDownstreamReach;   // associated downstream reach (NULL if Reservoir not connected to Reaches) (memory managed in FlowModel::m_reachArray)

   Reach *m_pFlowfromUpstreamReservoir;   // For testing with ResSIM.  2s reservoir (Foster and Lookout) (memory managed in FlowModel::m_reachArray)
                                          // have upstream inputs from other reservoirs.  This points to the stream
                                          // reaches containing those inflow (just upstream).
   void InitDataCollection( FlowModel *pFlowModel );
   void CollectData();
   
   float GetPoolElevationFromVolume();
   float GetPoolVolumeFromElevation(float elevation);
   float GetTargetElevationFromRuleCurve( int dayOfYear );
   float GetBufferZoneElevation( int dayOfYear );
   float GetResOutflow( FlowModel *pFlowModel, Reservoir *pRes, int dayOfYear);
	float GetResOutflow(Reservoir *pRes, int doy, LPCTSTR filename);
   float AssignReservoirOutletFlows( Reservoir *pRes, float outflow );
   void  UpdateMaxGateOutflows( Reservoir *pRes, float currentPoolElevation );
   void  CalculateHydropowerOutput( Reservoir *pRes );

   ResConstraint *FindResConstraint( LPCTSTR name );
   // data collection
public:
   FDataObj *m_pResData;                              // (memory managed here)
protected:
   FDataObj *m_pAreaVolCurveTable;                    // (memory managed here)
   FDataObj *m_pRuleCurveTable;                       // (memory managed here)
   FDataObj *m_pBufferZoneTable;                      // (memory managed here)
   FDataObj *m_pCompositeReleaseCapacityTable;        // (memory managed here)
   FDataObj *m_pRO_releaseCapacityTable;              // (memory managed here)
   FDataObj *m_pSpillwayReleaseCapacityTable;         // (memory managed here)
	FDataObj *m_pDemandTable;									// (memory managed here)
   VDataObj *m_pRulePriorityTable;                    // (memory managed here)


   //FDataObj *m_pResMetData;
   
   //Comparison with ResSIM
   FDataObj *m_pResSimFlowOutput;                     //Stores output from the ResSim model (from text file)
   VDataObj *m_pResSimRuleOutput;
   FDataObj *m_pResSimFlowCompare;
   VDataObj *m_pResSimRuleCompare;                    // Enables side by side comparison of ResSim and Envision outputs

   // temporary output variable!!!!!!!
   float m_avgPoolVolJunAug;    // average pool volume june to august
   float m_avgPoolElevJunAug;    // average pool volume june to august

};


class ControlPoint
{
public:
   CString m_name;
   CString m_dir;
   CString m_controlPointFileName;
   CString m_reservoirsInfluenced;
   int m_id;
   int m_location;
   RCTYPE m_type;
   float m_localFlow;   ///Used in comparison to ResSim
   int localFlowCol;   //  Used in comparison to ResSim

   FDataObj *m_pResAllocation;                  // stores allocations for each attached reservoir
   FDataObj *m_pControlPointConstraintsTable;

   CArray< Reservoir*, Reservoir* > m_influencedReservoirsArray;
   Reach *m_pReach;   // associated reach  
  
   // methods
   ControlPoint(void) : m_id( -1 ), m_location( -1 ), m_type( RCT_UNDEFINED ), 
      m_pResAllocation( NULL ), m_pControlPointConstraintsTable( NULL ), m_pReach( NULL ) { }

   ~ControlPoint( void );

   bool InUse( void ) { return m_pReach != NULL; }
};


class ReachSampleLocation
{
public:
   ReachSampleLocation( void ) 
      : m_name(_T("")), m_fieldName(_T("")), m_id(-1), m_iduCol(-1), m_pReach(NULL), m_pHRU( NULL ),m_pMeasuredData(NULL), row(-1),col(-1) { }

   ~ReachSampleLocation( void ) {}

   CString m_name;
   CString m_fieldName;
   int m_iduCol;
   int m_id;
   Reach* m_pReach;
   HRU* m_pHRU;
   FDataObj *m_pMeasuredData;
   int row;
   int col;

};


// ModelObject stores 
class ModelOutput
{
public:
   ModelOutput() : m_pQuery( NULL ), m_pMapExpr( NULL ), m_pDataObj( NULL ), m_pDataObjObs( NULL), m_modelType( MOT_UNDEFINED ), m_modelDomain( MOD_UNDEFINED ), m_inUse( true ), m_value( 0 ), m_totalQueryArea( 0 ), m_number(0), m_esvNumber(0), m_coord(0.0f,0.0) { }
   ~ModelOutput() { if ( m_pDataObj != NULL ) delete m_pDataObj; if ( m_pDataObjObs != NULL ) delete m_pDataObjObs; }   // note that m_pQuery is managed by the queryengine

   ModelOutput( ModelOutput &mo ) { *this = mo; }

   ModelOutput &operator = ( ModelOutput &mo ) { m_name = mo.m_name; m_queryStr = mo.m_queryStr;
      m_exprStr = mo.m_exprStr; m_pQuery = NULL, m_pMapExpr = NULL; m_modelType = mo.m_modelType;
      m_modelDomain = mo.m_modelDomain; m_inUse = mo.m_inUse; m_pDataObj = NULL; m_pDataObjObs = NULL;
      m_value = 0; m_totalQueryArea = 0; m_number = 0; m_esvNumber=0; return *this; }
   
   CString m_name;
   CString m_queryStr;
   CString m_exprStr;

   Query    *m_pQuery;           // memory managed by QueryEngine
   MapExpr  *m_pMapExpr;         // memory managed by MapExprEngine

   MOTYPE    m_modelType;
   MODOMAIN  m_modelDomain;
   bool      m_inUse;
   FDataObj *m_pDataObj;
   FDataObj *m_pDataObjObs;
   COORD2d   m_coord;

   float m_value;
   float m_totalQueryArea;    // total area satisfied by the query, 

   bool Init( EnvContext* );
   bool InitStream( FlowModel *pFlowModel, MapLayer* );

   int m_number;
   int m_esvNumber;
   int m_siteNumber; //an integer that can be used to locate a reservoir (from m_reservoirArray directly - the query engine cannot be used in this case)

};


class ModelOutputGroup : public PtrArray< ModelOutput >
{
public:
   ModelOutputGroup( void ) : PtrArray< ModelOutput >(), m_pDataObj( NULL ), 
         m_moCountIdu( 0 ), m_moCountHru( 0 ), m_moCountHruLayer( 0 ), m_moCountReach( 0 ) { }

   ~ModelOutputGroup( void ) { if ( m_pDataObj ) delete m_pDataObj; m_pDataObj = NULL; }

   ModelOutputGroup( ModelOutputGroup &mog ) { *this = mog; }

   ModelOutputGroup &operator = ( ModelOutputGroup &mog );
   
   CString   m_name;
   FDataObj *m_pDataObj;

   int m_moCountIdu;
   int m_moCountHru;
   int m_moCountHruLayer;
   int m_moCountReach;
};


class ModelVar
{
public:
   CString  m_label;
   MapVar  *m_pMapVar;
   MTDOUBLE m_value;

   ModelVar( LPCTSTR label ) : m_label( label ), m_pMapVar( NULL ), m_value( 0 ) { }
};


class ParameterValue
{
public:
   ParameterValue( void ) 
      : m_table(_T("")), m_name(_T("")), m_value(0.5f), m_minValue(0.0f), m_maxValue(1.0f), m_distribute(true) { }
   ~ParameterValue( void ) {}
   CString m_table;
   CString m_name;
   float m_value;
   float m_minValue;
   float m_maxValue;
   bool m_distribute;
};


//class ClimateStationInfo
// {
// public:
//    float m_elevation;
//    CString m_path;
//
//    ClimateStationInfo( ) : m_elevation(0) { }
// };


class ClimateDataInfo
{
public:
   CDTYPE m_type;    // see enum.  Note that for CDT_STATIONDATA
   CString m_path;

   // valid if source = Weather station data
   int m_id;  // idu STATIONID value associated with this weather station
   float m_stationElevation;
   FDataObj *m_pStationData;

   // valid if source = NETCDF
   GeoSpatialDataObj *m_pNetCDFData;
   PtrArray<GeoSpatialDataObj> m_pNetCDFDataArray;
   CString m_varName;
   bool  m_useDelta;
   float m_delta;

   ClimateDataInfo( void ) : m_type( CDT_UNKNOWN ), m_stationElevation(0), m_pStationData(NULL), m_pNetCDFData( NULL ),m_pNetCDFDataArray(NULL), m_useDelta( false ), m_delta( 0 ) { }
   ~ClimateDataInfo(void) { if (m_pNetCDFData) delete m_pNetCDFData; if ( m_pStationData) delete m_pStationData; }
};


class FlowScenario
{
public:
   CString m_name;
   int     m_id;
   bool    m_useStationData; // if true, current scenario uses point-level time series climate data, rather than gridded climated data.
   
   PtrArray< ClimateDataInfo > m_climateInfoArray;  
   CMap< int, int, ClimateDataInfo*, ClimateDataInfo* > m_climateDataMap;
};


struct SCENARIO_MACRO
{
   //int envScenarioIndex;      // Envision scenarion index associated with this macro
   CString envScenarioName;
   CString value;             // substitution text

   //SCENARIO_MACRO( void ) : envScenarioIndex( -1 ) { }
};


class ScenarioMacro
{
public:
   CString m_name; 
   CString m_defaultValue;

   PtrArray< SCENARIO_MACRO > m_macroArray;
};


class FLOWAPI FlowModel : public EnvModelProcess
{
friend class FlowProcess;
friend class HRU;
friend class Catchment;
friend class Reservoir;
friend class ModelOutput;
friend class ReachRouting;

public:
   FlowModel();
   ~FlowModel();

protected:
   FlowContext m_flowContext;

   PtrArray< PoolInfo > m_poolInfoArray;     // just templates, actual pools are in m_poolArray

public:
   bool Init( EnvContext*, LPCTSTR initStr);
   bool InitRun( EnvContext*, bool useInitialSeed);
   bool Run( EnvContext*  );
   bool EndRun( EnvContext* );
      
   bool StartYear( FlowContext* ); // start of year initializations go here
   bool StartStep( FlowContext* );
   bool EndStep  ( FlowContext* );
   bool EndYear  ( FlowContext* );
   // manage global methods
   //void RunGlobalMethods( void );          

   bool LoadXml(LPCTSTR filename, EnvContext *pContext);

   Reach *GetReachFromStreamIndex( int index ) { return (Reach*) m_reachTree.GetReachNodeFromPolyIndex( index ); }
   Reach *GetReachFromNode( ReachNode *pNode );
   Reach *GetReach( int index ) { return m_reachArray[ index ]; }  // from internal array
   int    GetReachCount( void ) { return (int) m_reachArray.GetSize(); }

   float  GetTotalReachLength( void );

   int AddReservoir(Reservoir *pRes) { return (int)m_reservoirArray.Add(pRes); }

   int AddHRU( HRU *pHRU, Catchment *pCatchment ) { m_hruArray.Add( pHRU ); pCatchment->m_hruArray.Add( pHRU ); pHRU->m_pCatchment = pCatchment; return (int) m_hruArray.GetSize(); }
   int GetHRUCount( void ) { return (int)m_hruArray.GetSize(); }

   int GetHRUPoolCount( void ) { return (int) m_poolInfoArray.GetSize(); }
   //int GetHRULayerCount( void ) { return m_soilLayerCount + m_vegLayerCount + m_snowLayerCount; }
   HRU *GetHRU(int i) {return m_hruArray[i];}
   
   int GetCatchmentCount( void ) { return (int)m_catchmentArray.GetSize(); }
   Catchment *GetCatchment( int i ) { return m_catchmentArray[ i ]; }
   Catchment *AddCatchment( int id, Reach *pReach ) { Catchment *pCatchment = new Catchment; pCatchment->m_id = id; pCatchment->m_pReach = pReach; pReach->m_catchmentArray.Add( pCatchment ); m_catchmentArray.Add( pCatchment ); return pCatchment; }
   Reach *FindReachFromIndex( int index ) {  for ( int i=0; i < m_reachArray.GetCount(); i++ ) if ( m_reachArray[i]->m_reachIndex == index ) return m_reachArray[ i ]; return NULL; }

   StateVar *GetStateVar(int i) { return m_stateVarArray.GetAt(i); }
   StateVar *FindStateVar(LPCTSTR varName) { for (int i = 0; i < m_stateVarArray.GetCount(); i++) if (m_stateVarArray[i]->m_name.CompareNoCase(varName) == 0) return m_stateVarArray[i]; return NULL; }


protected:
   bool InitReaches( void  );
   bool InitReachSubnodes( void );
   bool InitCatchments( void );
   bool InitHRUPools( void );
   bool InitReservoirs( void );
   bool InitReservoirControlPoints(EnvModel*);
   bool InitRunReservoirs( EnvContext* );
   bool UpdateReservoirControlPoints( int doy );
   void UpdateReservoirWaterYear( int doy );
   int  SimplifyNetwork( void );
   int  SimplifyTree( ReachNode *pNode );
   int  ApplyCatchmentConstraints( ReachNode *pNode );
   int  ApplyAreaConstraints( ReachNode *pNode );
   int  SaveState();
   int  ReadState();

   int  OpenDetailedOutputFiles();
   int  SaveDetailedOutputIDU( CArray< FILE*, FILE* > & );
   int  SaveDetailedOutputGridTracer(CArray< FILE*, FILE* > &);
   int  SaveDetailedOutputGrid(CArray< FILE*, FILE* > &);
   int  SaveDetailedOutputReach( CArray< FILE*, FILE* > & );
   void  CloseDetailedOutputFiles(); 

   // helper functions
   int  OpenDetailedOutputFilesIDU  ( CArray< FILE*, FILE* > & );
   int  OpenDetailedOutputFilesGrid(CArray< FILE*, FILE* > &);
   int  OpenDetailedOutputFilesGridTracer(CArray< FILE*, FILE* > &);
   int  OpenDetailedOutputFilesReach( CArray< FILE*, FILE* > & );
 
   int  m_detailedOutputFlags;  //1=dailyIDU, 2=annualIDU, 4=daily reach, 8=annual reach

   CArray< FILE*, FILE* > m_iduAnnualFilePtrArray;   // list of file pointers
   CArray< FILE*, FILE* > m_iduDailyFilePtrArray;   // list of file pointers
   CArray< FILE*, FILE* > m_iduDailyTracerFilePtrArray;   // list of file pointers
   CArray< FILE*, FILE* > m_reachAnnualFilePtrArray;   // list of file pointers
   CArray< FILE*, FILE* > m_reachDailyFilePtrArray;   // list of file pointers
   
   bool m_saveStateAtEndOfRun;
      
   int  MoveCatchments( Reach *pToReach, ReachNode *pStartNode, bool recurse );  // recursive
   int  CombineCatchments( Catchment *pTargetCatchment, Catchment *pSourceCatchment, bool deleteSource ); // copy the HRUs from one catchment to another, optionally deleting the source Catchment
   
   void  PopulateTreeID( ReachNode *pNode, int treeID ); // recursive
   float PopulateCatchmentCumulativeAreas( void );
   float PopulateCatchmentCumulativeAreas( ReachNode *pNode );

   int  RemoveReaches( ReachNode *pStartNode );       // recursive
   bool RemoveReach( ReachNode *pNode );              // a single reach
   bool RemoveCatchment( ReachNode *pNode );          //single catchment - 
   int  GetUpstreamCatchmentCount( ReachNode *pStartNode );
   bool BuildCatchmentsFromQueries( void );
   bool BuildCatchmentsFromGrids( void );
   bool BuildCatchmentsFromLayer( void );
   int  RemoveCatchmentHRUs( Catchment *pCatchment );

   //void PopulateHRUArray(void);

   int  BuildCatchmentFromPolygons( Catchment*, int iduArray[], int count );

   bool AggregateModel( void );
   bool CalculateUpstreamGeometry( void );

   bool SetCatchmentAttributes(void);
   bool SetHRUAttributes();
   int  GenerateAggregateQueries( MapLayer *pLayer /*in*/, CUIntArray &colsAgg /*in*/, CArray< Query*, Query* > &queryArray /*out*/ );
   int  ParseCols( MapLayer *pLayer /*in*/, CString &aggCols /*in*/, CUIntArray &colsAgg /*out*/ );
   //int  ParseHRULayerDetails(CString &aggCols, int detail);
   bool ConnectCatchmentsToReaches( void );
   //void UpdateCatchmentFluxes( float time, float timeStep, FlowContext *pFlowContext );

   //float GetReachOutflow( ReachNode *pReachNode );

   bool InitIntegrationBlocks( void );
   void AllocateOutputVars();
   int  AddStateVar( StateVar *pSV ) { return ( pSV->m_index = (int) m_stateVarArray.Add( pSV ) ); }
   int  GetStateVarCount( void ) { return (int) m_stateVarArray.GetSize(); }

   Reservoir *FindReservoirFromID( int id ) {  for ( int i=0; i < m_reservoirArray.GetCount(); i++ ) if ( m_reservoirArray[i]->m_id == id ) return m_reservoirArray[ i ]; return NULL; }
   Reach *FindReachFromID( int id ) {  for ( int i=0; i < m_reachArray.GetCount(); i++ ) if ( m_reachArray[i]->m_reachID == id ) return m_reachArray[ i ]; return NULL; }

   void UpdateSummaries() { }

   void SummarizeIDULULC(void);
   void AddReservoirLayer( void );
   bool WriteDataToMap( EnvContext* );
   void UpdateAprilDeltas( EnvContext*  );
   void GetMaxSnowPack(EnvContext*);
   void UpdateYearlyDeltas( EnvContext*  );
   //bool RedrawMap( EnvContext* );
   bool ResetFluxValuesForStep( EnvContext* );
   bool ResetCumulativeYearlyValues();
   bool ResetCumulativeWaterYearValues();

   //  direct reach solution methods
   //float GetLateralInflow( Reach *pReach );
   //float GetReachInflow( Reach *pReach, int subNode );
   //float EstimateReachOutflow( Reach *pReach, int i, float timeStep, float lateralInflow);

   //float GetReachSVInflow( Reach *pReach, int subNode, int sv );
   //float GetLateralSVInflow( Reach *pReach , int sv);
   //float GetReachSVOutflow( ReachNode *pReachNode, int sv ) ;

   //void  SetGeometry( Reach *pReach, float discharge );
   //float GetDepthFromQ( Reach *pReach, float Q, float wdRatio );  // ASSUMES A SPECIFIC CHANNEL GEOMETRY

   //Default Upslope Fluxes
   //bool  SolveHRUDirect( FlowContext* );     // high level method PLACEHOLDER
   //float GetReachFluxes( Reach *pReach );
   //float GetVerticalDrainage(float wc);
   //float GetBaseflow(float wc);
   
   // global methods
   //bool  SolveReachDirect( void );     // high level method
   //bool  SolveReachKinematicWave( void );

   //bool  SetGlobalHruToReachExchanges( void );  // high-level method
   //bool  SetGlobalHruToReachExchangesLinearRes( void );

   //bool  SetGlobalHruVertFluxes( void  );  // high-level method
   //bool  SetGlobalHruVertFluxesBrooksCorey( void );

   bool  SetGlobalExtraSVRxn( void );
   
   bool  SetGlobalReservoirFluxes( void );  // high-level method
   bool  SetGlobalReservoirFluxesResSimLite( void );
   bool  SetGlobalClimate( int dayOfYear );
   bool  SetGlobalFlows( void );

   bool  SolveGWDirect( void );     // high level method
   bool  SetGlobalGroundWater( void );

protected:
   // various collections
   PtrArray< Catchment > m_catchmentArray;         // list of catchments included in this model
   PtrArray< HRU >       m_hruArray;               // list of HRUs included in this model
   IDataObj              *m_pHruGrid;               // a gridded form of the HRU array (useful for gridded models only)

   PtrArray< ControlPoint > m_controlPointArray;   // list of control points included in this model
     
public:
   PtrArray< Reservoir > m_reservoirArray;         // list of reservoirs included in this model
   CArray< Reach*, Reach* > m_reachArray;       // list of reaches included in this model (note: memory managed by ReachTree)
   MapLayer *m_pGrid;
   int  m_detailedSaveInterval;
   int  m_detailedSaveAfterYears;
   CString m_projectionWKT;

protected:
   // state var information
   PtrArray< StateVar > m_stateVarArray;
   PtrArray< Vertex > m_vertexArray;
// XML attributes

protected:
   CString m_filename;
   CString m_name;
   CString m_streamLayer;
   CString m_catchmentLayer;
   //CString m_climateFilePath;
   CString m_path;
   CString m_grid;
 
   //float m_climateStationElev;

   CString m_streamQuery;
   CString m_catchmentQuery;
   

   CString m_areaCol;      // polygon area
   CString m_catchmentAreaCol; //catchment area
   CString m_elevCol;
   
   CString m_orderCol;
   CString m_catchIDCol;
   CString m_streamJoinCol;
   CString m_hruIDCol;
   CString m_catchmentJoinCol;
   CString m_treeIDCol;
   CString m_toCol;
   CString m_fromCol;

   CString m_initConditionsFileName;

   int     m_minStreamOrder;
   float   m_subnodeLength;
   int     m_buildCatchmentsMethod;
   //int     m_soilLayerCount;
   //int     m_snowLayerCount;
   //int     m_vegLayerCount;
   //int     m_poolCount;
 
   float   m_initTemperature;
   float   m_wdRatio;     // global width/depth ratio 
   float   m_n;           // global value for mannings n
   float   m_waterYearType;   //Variable used in reservoir operations model

   float   m_minCatchmentArea;
   float   m_maxCatchmentArea;
   int     m_numReachesNoCatchments;
   int     m_numHRUs;

public:
   int m_hruSvCount;
   int m_reachSvCount;
   int m_reservoirSvCount;
   float m_minElevation;

// catchment layer required columns
   int m_colCatchmentArea;
   int m_colArea;
   int m_colElev;
   int m_colLai;
   int m_colLulcB;
   int m_colLulcA;

   int m_colCatchmentReachIndex;
   int m_colCatchmentCatchID;
   int m_colCatchmentHruID;
   int m_colCatchmentReachId;
   int m_colCatchmentJoin;     ///////

// stream layer required columns
   int m_colStreamFrom;    // Used by BuildTree() to establish stream reach topology
   int m_colStreamTo;      // Used by BuildTree() to establish stream reach topology
   int m_colStreamOrder;
   int m_colStreamJoin;      
   int m_colHruSWC;
   int m_colHruPrecip;
   int m_colHruPrecipYr;
   int m_colHruPrecip10Yr;
   int m_colHruTemp;
   int m_colHruTempYr;
   int m_colHruTemp10Yr;
   int m_colHruSWE;
   int m_colHruMaxSWE;
   int m_colHruApr1SWE;
   int m_colHruApr1SWE10Yr;

   //int m_colHruAnnualSnow;
   int m_colHruDecadalSnow;
   int m_colStreamReachOutput;
	//int m_colTempMax;  // not used
   int m_colReachArea;

   int m_colSnowIndex;
   int m_colAridityIndex;
   int m_colRefSnow;
   int m_colET_yr;
   int m_colHRUPercentIrrigated;
   int m_colHRUMeanLAI;
   int m_colMaxET_yr;
   int m_colIrrigation_yr;
   int m_colIrrigation;
   int m_colPrecip_yr;
   int m_colRunoff_yr;
   int m_colStorage_yr;
   int m_colAgeClass;

   // optional cols
   int m_colStreamCumArea;
   int m_colCatchmentCumArea;
   int m_colTreeID;
   int m_colStationID;  // needed if more than on weather staion used

// reservoir column (in stream coverage)
   int m_colResID;

   // CSV climate station columns (for climate station source)
   int m_colCsvYear;  
   int m_colCsvMonth; 
   int m_colCsvDay;   
   int m_colCsvHour;  
   int m_colCsvMinute;

   int m_colCsvPrecip;
   int m_colCsvTMean;
   int m_colCsvTMin;
   int m_colCsvTMax;
   int m_colCsvSolarRad;
   int m_colCsvWindspeed;
   int m_colCsvRelHumidity;
   int m_colCsvSpHumidity;
   int m_colCsvVPD;

// boolean that Initial Conditions file is successfully read
   bool m_isReadStateOK;

   int m_provenClimateIndex;

// layer structures
   Map *m_pMap;
   MapLayer *m_pCatchmentLayer;
   MapLayer *m_pStreamLayer;
   MapLayer *m_pResLayer;  // visual display only
   ReachTree m_reachTree;

// Catchment information
   CString m_catchmentAggCols;   // columns for aggregating catchment
   CUIntArray m_colsCatchmentAgg;

// HRU information
   CString m_hruAggCols;         // columns for aggregating HRU's
   CUIntArray m_colsHruAgg;

   INT_METHOD m_integrator;
   float m_minTimeStep;
   float m_initTimeStep;
   float m_maxTimeStep;
   float m_errorTolerance;

   const MapLayer* GetGrid() { return m_pGrid; }

public:

   // Storage for model results
   //FDataObj *m_pReachDischargeData;
   //FDataObj *m_pHRUPrecipData;     
   //FDataObj *m_pHRUETData;  // contains HRUPool precip info for each sample location
   FDataObj *m_pTotalFluxData;  // for mass balance, acre-ft/yr

      //FDataObj *m_pReservoirData;
   PtrArray < FDataObj > m_reservoirDataArray;
   PtrArray < FDataObj > m_reachHruLayerDataArray;   // each element is an FDataObj that stores modeled hrulayer water content (m_wc) for a reach sample point
   PtrArray < FDataObj > m_hruLayerExtraSVDataArray;    
   PtrArray < FDataObj > m_reachMeasuredDataArray;

   // exposed outputs
   float m_volumeMaxSWE;
   int m_dateMaxSWE;

   float m_annualTotalET;           // acre-ft
   float m_annualTotalPrecip;       // acre-ft
   float m_annualTotalDischarge;    //acre-ft
   float m_annualTotalRainfall;     // acre-ft
   float m_annualTotalSnowfall;     // acre-ft

   int IdentifyMeasuredReaches( void );
   //bool InitializeReachSampleArray(void);
   //bool InitializeHRULayerSampleArray(void);
   //bool InitializeReservoirSampleArray(void);
   void CollectData( int dayOfYear );
   void ResetStateVariables( void );
   void ResetDataStorageArrays( EnvContext *pEnvContext );
   //void CollectHRULayerData( void );
   //void CollectHRULayerExtraSVData( void );
   //void CollectHRULayerDataSingleObject( void );
   //void CollectDischargeMeasModelData( void );
   bool CollectModelOutput( void );
   void UpdateHRULevelVariables(EnvContext *pEnvContext);

   void CollectReservoirData();
 
   int m_mapUpdate;  // Update the map with Results from the Hydrology model?  0: no update, 1: update each year, 2: update each flow timestep" ); 

   int m_gridClassification;

   // simulation 
   double  m_currentTime;   // days since 0000
protected:
   double  m_stopTime;      // note: All time units are days
  
   double  m_timeInRun;     // days in run (0-based, starting in startYear)
   double  m_timeStep;      // for internal synchronization/scheduling
   int     m_year;          // 
   int     m_month;         // current month of year (1-based)
   int     m_day;           // current day of month (1-based)
   int     m_timeOffsett;
   float   m_collectionInterval;
   bool    m_useVariableStep;
   double  m_rkvTolerance;
   double  m_rkvMaxTimeStep; 
   double  m_rkvMinTimeStep;
   double  m_rkvInitTimeStep;
   double  m_rkvTimeStep;

   // ptrs to required global methods - (note: memore is managed elsewhere
   ReachRouting    *m_pReachRouting;   // can this be moved to externasl (really, global) methods array?
   LateralExchange *m_pLateralExchange;
   HruVertExchange *m_pHruVertExchange;


   // hruLayer extra sv Reaction methods
   EXSVMETHOD  m_extraSvRxnMethod;
   CString m_extraSvRxnExtSource;
   CString m_extraSvRxnExtPath;
   CString m_hextraSvRxnExtFnName;
   HINSTANCE m_extraSvRxnExtFnDLL;
   DIRECTFN m_extraSvRxnExtFn;

   // reservoir flux methods
   RESMETHOD  m_reservoirFluxMethod;
   CString m_reservoirFluxExtSource;
   CString m_reservoirFluxExtPath;
   CString m_reservoirFluxExtFnName;
   HINSTANCE m_reservoirFluxExtFnDLL;
   DIRECTFN m_reservoirFluxExtFn;
   FDataObj *m_pResInflows;  //Use to store inflow from ResSIM for model testing.  mmc 01/04/2013
   FDataObj *m_pCpInflows;  //Use to store inflow at control points from ResSIM for model testing.  mmc 06/04/2013

   // groundwater methods
   //GWMETHOD m_gwMethod;
   //CString m_gwExtSource;
   //CString m_gwExtPath;
   //CString m_gwExtFnName;
   //HINSTANCE m_gwExtFnDLL;
   //DIRECTFN m_gwExtFn;

   // simulation control
   IntegrationBlock m_hruBlock;
   IntegrationBlock m_reachBlock;
   IntegrationBlock m_reservoirBlock;
   IntegrationBlock m_totalBlock;         // for overall mass balance

   // cumulative variables for mass balance error check
   float m_totalWaterInputRate;
   float m_totalWaterOutputRate;
   SVTYPE m_totalWaterInput;
   SVTYPE m_totalWaterOutput;

   FDataObj *m_pGlobalFlowData;
   
   static void GetCatchmentDerivatives( double time, double timestep, int svCount, double *derivatives, void *extra );
   static void GetReservoirDerivatives( double time, double timestep, int svCount, double *derivatives, void *extra ) ;
   //static void GetReachDerivatives( float time, float timestep, int svCount, double *derivatives, void *extra );
   static void GetTotalDerivatives( double time, double timestep, int svCount, double *derivatives, void *extra ) ;
   
protected:
// parameter tables
   PtrArray <ParamTable> m_tableArray;
   PtrArray <ReachSampleLocation> m_reachSampleLocationArray;

   PtrArray <ParameterValue> m_parameterArray;
   int m_numQMeasurements;
   int ReadUSGSFlow( LPCSTR fileName, FDataObj &dataObj, int dataType=0 );

   PtrArray< ModelOutputGroup > m_modelOutputGroupArray;
   PtrArray< ModelVar > m_modelVarArray;     // for any flow-defined variables registered with the MapExprEngine
 
public:
   int GetModelOutputGroupCount( void ) { return (int) m_modelOutputGroupArray.GetSize(); }
   ModelOutputGroup *GetModelOutputGroup( int i ) { return m_modelOutputGroupArray[ i ]; }

public:
   ParamTable *GetTable( LPCTSTR name );     // get a table
   bool        GetTableValue( LPCTSTR name, int col, int row, float &value );
   // note: for more efficient ParamTable data access, get (and store) a ParamTable ptr
   // and use ParamTable::Lookup() methods
   int AddModelVar( LPCTSTR label, LPCTSTR varName, MTDOUBLE *value );

   // various GetData()'s
   bool GetHRUData( HRU *pHRU, int col, VData &value, DAMETHOD method ); 
   bool GetHRUData( HRU *pHRU, int col, bool  &value, DAMETHOD method ); 
   bool GetHRUData( HRU *pHRU, int col, int   &value, DAMETHOD method ); 
   bool GetHRUData( HRU *pHRU, int col, float &value, DAMETHOD method ); 

   bool GetCatchmentData( Catchment *pCatchment, int col, VData &value, DAMETHOD method ); 
   bool GetCatchmentData( Catchment *pCatchment, int col, bool  &value, DAMETHOD method ); 
   bool GetCatchmentData( Catchment *pCatchment, int col, int   &value, DAMETHOD method ); 
   bool GetCatchmentData( Catchment *pCatchment, int col, float &value, DAMETHOD method ); 

   bool SetCatchmentData( Catchment *pCatchment, int col, VData value ); 
   bool SetCatchmentData( Catchment *pCatchment, int col, bool  value ); 
   bool SetCatchmentData( Catchment *pCatchment, int col, int   value ); 
   bool SetCatchmentData( Catchment *pCatchment, int col, float value ); 

   bool GetReachData( Reach *pReach, int col, VData &value );
   bool GetReachData( Reach *pReach, int col, bool  &value ); 
   bool GetReachData( Reach *pReach, int col, int   &value ); 
   bool GetReachData( Reach *pReach, int col, float &value );

   const char* GetPath() {return m_path.GetString();}

   // misc utility functions
   int  GetCurrentYear( void ) { return this->m_flowContext.pEnvContext->currentYear; }
   int  GetCurrentYearOfRun( void ) { return this->m_flowContext.pEnvContext->yearOfRun; }


protected:
   PtrArray< FlowScenario > m_scenarioArray;
   int m_currentFlowScenarioIndex;
   int m_currentEnvisionScenarioIndex;

   // scenario macros 
   PtrArray< ScenarioMacro > m_macroArray;
   void ApplyMacros( EnvModel*, CString &str );
   

// climate data (assumes daily values, one years worth of data)
protected:
   //PtrArray< ClimateDataInfo > m_climateDataInfoArray;
   //CMap< int, int, ClimateDataInfo*, ClimateDataInfo* > m_climateDataMap;

   //void InitRunClimate( void );
   bool LoadClimateData(EnvContext *pEnvContext, int year );
   void CloseClimateData( void );

public:
   //int   GetAvailableClimateInfo( void );   // see CDT_xxx enum above for values, OR'd together
   bool  GetReachClimate( CDTYPE type, Reach *pReach, int dayOfYear, float &value );
   bool  GetHRUClimate( CDTYPE type, HRU *pHRU, int dayOfYear, float &value ); 
   //float GetCatchmentClimate( CDTYPE type, Catchment *pCatchment );

   ClimateDataInfo   *GetClimateDataInfo( int typeOrStationID ); // note: if station data, type
      //FlowScenario *pScenario = m_scenarioArray.GetAt( m_currentFlowScenarioIndex ); ClimateDataInfo *pInfo = NULL;  if ( pScenario && pScenario->m_climateDataMap.Lookup( type, pInfo ) ) return pInfo; return NULL; }
   GeoSpatialDataObj *GetClimateDataObj ( CDTYPE type ) { FlowScenario *pScenario = m_scenarioArray.GetAt( m_currentFlowScenarioIndex ); ClimateDataInfo *pInfo = NULL;  if ( pScenario && pScenario->m_climateDataMap.Lookup( type, pInfo ) ) return pInfo->m_pNetCDFData; return NULL; }

protected:
   // Query
   QueryEngine   *m_pStreamQE;
   QueryEngine   *m_pCatchmentQE;     // memory managed by EnvModel
   QueryEngine   *m_pModelOutputQE;
   QueryEngine   *m_pModelOutputStreamQE;
   MapExprEngine *m_pMapExprEngine;
   
   Query *m_pStreamQuery;
   Query *m_pCatchmentQuery;
   
   // plugin interfaces this only apply to DLLs with global methods
   bool InitPlugins   ( void );
   bool InitRunPlugins( void );
   bool EndRunPlugins ( void );
   
  //Parameter Estimation
public:
   void GetNashSutcliffe(float *ns);
   void GetNashSutcliffeGrouped(float ns);
   float GetObjectiveFunction(FDataObj *pData, float &ns, float &nsLN, float &ve);
   float GetObjectiveFunctionGrouped(FDataObj *pData, float &ns, float &nsLN, float &ve);

   int LoadTable(EnvModel*, LPCTSTR filename, DataObj** pDataObj, int type );  // type=0->FDataObj, type=1->VDataObj;

   PtrArray< FDataObj > m_mcOutputTables;
   FDataObj *m_pErrorStatData;
   FDataObj *m_pParameterData;
   FDataObj *m_pDischargeData;
   FDataObj *m_pOtherTimeSeriesData;
   FDataObj *m_pOtherObjectiveFunctions;
   RandUniform m_randUnif1;
   RandNormal m_randNorm1;
   long m_rnSeed;
   bool InitializeParameterEstimationSampleArray(void);
   bool m_estimateParameters;
   int m_numberOfRuns;
   int m_numberOfYears;
   int m_saveResultsEvery;
   float m_nsThreshold;
   bool m_climateStationRuns;
   bool m_runFromFile;
   //CString m_paramEstOutputPath;   // defaults to 'm_path'\outputs\

   void UpdateMonteCarloOutput(EnvContext *pEnvContext, int runNumber);
   void UpdateMonteCarloInput(EnvContext *pEnvContext, int runNumber);
   
   //FDataObj *m_pClimateStationData;
   int ReadClimateStationData( ClimateDataInfo*, LPCSTR filename );
   int m_timeOffset;
   void GetTotalStorage(float &channel, float &terrestrial);

   // various timers
   float m_loadClimateRunTime;   
   float m_globalFlowsRunTime;   
   float m_externalMethodsRunTime;   
   float m_gwRunTime;   
   float m_hruIntegrationRunTime;   
   float m_reservoirIntegrationRunTime;   
   float m_reachIntegrationRunTime;   
   float m_massBalanceIntegrationRunTime; 
   float m_totalFluxFnRunTime;                 // time in Flux::Evaluate()
   float m_reachFluxFnRunTime;
   float m_hruFluxFnRunTime;                   // time in GetCatchmentDerivatives()
   float m_collectDataRunTime;
   float m_outputDataRunTime;

   int m_checkMassBalance;    // do mass balance check?

   void ReportTimings( LPCTSTR format, float timing ) { CString msg; msg.Format( format, timing ); Report::Log( msg ); }   
   void ReportTimings( LPCTSTR format, float timing, float timing2 ) { CString msg; msg.Format( format, timing, timing2 ); Report::Log( msg ); }   
   bool CheckHRUPoolValues( int doy, int year);

// video recording
//protected:
   bool  m_useRecorder;
   CArray< int > m_vrIDArray;
   //int   m_frameRate;

public:
   int m_maxProcessors;
   int m_processorsUsed;
};


/*
class FLOWAPI FlowProcess : public EnvModelProcess
{
public:
   FlowProcess();
   FlowProcess(FlowProcess&) {}
   ~FlowProcess();

   virtual bool Init   ( EnvContext *pEnvContext, LPCTSTR initStr );
   virtual bool InitRun( EnvContext *pEnvContext );
   virtual bool Run    ( EnvContext *pEnvContext );
   virtual bool EndRun ( EnvContext *pEnvContext );
   virtual bool Setup  ( EnvContext*, HWND );

protected:
   PtrArray< FlowModel > m_flowModelArray;

public:
   int AddModel( FlowModel *pModel ) { return (int) m_flowModelArray.Add( pModel ); }

protected:
   FlowModel *GetFlowModelFromID( int id );
   bool LoadXml( LPCTSTR filename, EnvContext *pContext );

public:
   int m_maxProcessors;
   int m_processorsUsed;
};
*/


/*
inline
void HRUPool::AddFluxFromGlobalHandler(float value, WFAINDEX wfaIndex)
   {
   switch (wfaIndex)
      {
      case FL_TOP_SINK:
      case FL_BOTTOM_SINK:
      case FL_STREAM_SINK:
      case FL_SINK:  //negative values are gains
         value = -value;
      break;
   default:
      break;
      }

   m_waterFluxArray[wfaIndex] += value;
   m_globalHandlerFluxValue += value;
   } 
   */

inline
float Reach::GetCatchmentArea( void )
   {
   float area = 0;
   for ( int i=0; i < (int) m_catchmentArray.GetSize(); i++ )
      area += m_catchmentArray[ i ]->m_area;
   return area;
   }


inline
ResConstraint *Reservoir::FindResConstraint( LPCTSTR name )
   {
   for ( INT_PTR i=0; i < m_resConstraintArray.GetSize(); i++ ) 
      {
      ResConstraint *pConstraint = m_resConstraintArray[ i ];
      
      if ( pConstraint->m_constraintFileName.CompareNoCase( name ) == 0 ) 
         return m_resConstraintArray[ i ];  
      }

   return NULL;
   }

inline
bool FlowModel::SetCatchmentData( Catchment *pCatchment, int col, bool  value )
   {
   VData _value( value );
   return SetCatchmentData( pCatchment, col, _value );
   }

inline
bool FlowModel::SetCatchmentData( Catchment *pCatchment, int col, int  value )
   {
   VData _value( value );
   return SetCatchmentData( pCatchment, col, _value );
   }

inline
bool FlowModel::SetCatchmentData( Catchment *pCatchment, int col, float value )
   {
   VData _value( value );
   return SetCatchmentData( pCatchment, col, _value );
   }


inline 
Reach *FlowModel::GetReachFromNode( ReachNode *pNode )
   {
   if ( pNode == NULL ) 
      return NULL;
   
   if ( pNode->m_reachIndex < 0 )
      return NULL;

   if ( pNode->IsPhantomNode() )
      return NULL;
   
   return static_cast<Reach*>( pNode );
   } 
