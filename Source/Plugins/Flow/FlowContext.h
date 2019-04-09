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

#include <IntegrationBlock.h>


class EnvContext;

class Reach;
class ReachNode;
class Reservoir;
class HRUPool;
class Flux;
class FluxInfo;
class FlowModel;
class HRU;
class Catchment; 
class StateVar;
//class ResConstraint;

/*! \file */
///\enum FLUXTYPE The type of flux
enum FLUXTYPE { FT_UNDEFINED = 0, FT_PRECIP = 1 };

///\enum FLUXDOMAIN The type of state variable that acts as the source for the flux
//enum FLUXDOMAIN   { FD_UNKNOWN=0, FD_REACH=1, FD_HRULAYER=2, FD_RESERVOIR=3, FD_GROUNDWATER=4, FD_IDU=5 };
///\enum FLUXSOURCE The method used to specify flux values

enum FLUXSOURCE { FS_UNDEFINED = 0, FS_FUNCTION = 1, FS_DATABASE = 2 };
///\enum HRULAYER_TYPE A descriptor of the type of state variable represented by the HRUPool
enum POOL_TYPE { PT_UNDEFINED = -1, PT_SOIL = 0, PT_IRRIGATED = 1, PT_SNOW = 2, PT_MELT = 3, PT_VEG = 4 };

///\enum GRCFLAG Flow::GetReachCount flags indicating if all or only modeled reaches should be included
enum GRCFLAG { GRC_ALL = 1, GRC_MODELED = 2 };

///\enum CDTYPE Climate data types, used in climate accessors
enum CDTYPE {
       CDT_UNKNOWN     = 0,
       CDT_PRECIP      = -1,
       CDT_TMEAN       = -2,
       CDT_TMIN        = -4,
       CDT_TMAX        = -8,
       CDT_SOLARRAD    = -16,
       CDT_RELHUMIDITY = -32,
       CDT_SPHUMIDITY  = -64,
       CDT_WINDSPEED   = -128, 
       CDT_VPD         = -256,
       CDT_STATIONDATA = -512 
   };
       
///\enum RSMETHOD Integration methods (including direct solutions)
//enum RSMETHOD {
//   // the first of these map into the IntegrationBlock defined methods
//   RSM_EULER=IM_EULER, RSM_RK4=IM_RK4, RSM_RKF=IM_RKF, 
//   // direct methods follow
//   RSM_EXTERNAL=IM_OTHER,
//   RSM_KINEMATIC=100 
//   };


///\enum HREXMETHOD lateral inflow/outflow solution method
//enum HREXMETHOD {
//   HREX_EXTERNAL = 0,
//   HREX_LINEARRESERVOIR=100,
//   HREX_INFLUXHANDLER=200
//   };
//
/////\enum VDMETHOD vertical drainage in HRU Layers
//enum VDMETHOD {
//   VD_EXTERNAL = 0,
//   VD_BROOKSCOREY=100,
//   VD_INFLUXHANDLER=200
//   };

///\enum VDMETHOD vertical drainage in HRU Layers
enum EXSVMETHOD {
   EXSV_EXTERNAL = 0,
   EXSV_INFLUXHANDLER=200
   };

///\enum RESMETHOD reservoir flux methods
enum RESMETHOD {
   RES_EXTERNAL = 0,
   RES_RESSIMLITE=100
   };

///\enum GWMETHOD reservoir flux methods (Note: No internal methods - yet)
//enum GWMETHOD {
//   GW_EXTERNAL = 0,
//   GW_NONE = 100
//   };


///\enum MOTYPE - ModelObject type flags
enum MOTYPE { 
   MOT_UNDEFINED  = 0,
   MOT_SUM        = 1,
   MOT_AREAWTMEAN = 2,
   MOT_PCTAREA    = 4
   };

///\enum MODOMAIN - ModelObject domain flags
enum MODOMAIN { 
   MOD_UNDEFINED  = 0,
   MOD_IDU        = 1,
   MOD_HRU        = 2,
   MOD_HRULAYER   = 4,
   MOD_REACH      = 8,
   MOD_RESERVOIR  = 16,
   MOD_HRU_IDU    = 32,
   MOD_GRID       = 64
   };

enum ZONE {      // Dam release zones.  
   ZONE_UNDEFINED        = -1,
   ZONE_TOP              = 0,
   ZONE_FLOODCONTROL     = 1,
   ZONE_CONSERVATION     = 2,
   ZONE_BUFFER           = 3,
   ZONE_ALTFLOODCONTROL1 = 4,
   ZONE_ALTFLOODCONTROL2 = 5 
   };

enum RCTYPE {  // ResConstraint Types
   RCT_UNDEFINED        = 0,
   RCT_MAX              = 1,
   RCT_MIN              = 2,
   RCT_INCREASINGRATE   = 3,
   RCT_DECREASINGRATE   = 4,
   RCT_CONTROLPOINT     = 5,
   RCT_POWERPLANT       = 6,
   RCT_REGULATINGOUTLET = 7,
   RCT_SPILLWAY         = 8
   };


enum WFAINDEX { // water_flux_array indexes for various HRUPool fluxes, used in AddFluxFromGlobalHandler
                // NOTE: passed in fluxes should generally be POSITIVE values except as noted below
   FL_UNDEFINED       = -1,
   FL_TOP_SINK        = 0,    // upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
   FL_TOP_SOURCE      = 1,    // down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
   FL_BOTTOM_SOURCE   = 2,    // upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
   FL_BOTTOM_SINK     = 3,    // down flux across lower surface of this layer (a loss)
   FL_STREAM_SINK     = 4,    // loss to streams (negative value are gains)
   FL_SINK            = 5,    // loss from the system (negative values are gains)
   FL_USER_DEFINED    = 6,
   FL_SIDE0_SOURCE    = 7,    // NW  (grid)
   FL_SIDE1_SOURCE    = 8,    // N   (grid)
   FL_SIDE2_SOURCE    = 9,    // NE  (grid)
   FL_SIDE3_SOURCE    = 10,    // W   (grid)
   FL_SIDE4_SOURCE    = 11,   // E   (grid)
   FL_SIDE5_SOURCE    = 12,   // SW  (grid)
   FL_SIDE6_SOURCE    = 13,   // S   (grid)
   FL_SIDE7_SOURCE    = 14,   // SE  (grid)
   };

enum // Global Method Timing flags
   {
   GMT_INIT       = 1,       // called during Init();
   GMT_INITRUN    = 2,       // called during InitRun();
   GMT_START_YEAR = 4,       // called at the beginning of a Envision timestep (beginning of each year of run)  (PreRun)
   GMT_START_STEP = 8,       // called at the beginning of a Flow timestep PreRun)
   GMT_CATCHMENT  = 16,      // called during GetCatchmentDerivatives() 
   GMT_REACH      = 32,      // called during GetReachDerivatives() 
   GMT_RESERVOIR  = 64,      // called during GetReservoirDerivatives() 
   GMT_END_STEP   = 128,     // called at the beginning of a Flow timestep PreRun)
   GMT_END_YEAR   = 256,     // called at the end of a Envision timestep (beginning of each year of run) (PostRun)
   GMT_ENDRUN     = 512      // called during EndRun()
   };
   
 
//----------------------------------------------------------------------------
//--------------------- P L U G - I N   D E F I N I T I O N S ----------------
//----------------------------------------------------------------------------



class FlowContext
{
public:
   FlowContext( void ) : pEnvContext( NULL ), time( 0 ), timeStep( 0 ), dayOfYear(0),
        pHRU( NULL ), pHRUPool( NULL ), 
        pReach( NULL ), reachSubnodeIndex( -1 ), iduIndex( -1 ),
        pFlux( NULL ), pFluxInfo( NULL ), pFlowModel( NULL ), timing( 0 ), 
        svArray( NULL ), svCount( 0 ), initInfo( NULL ) { }

   FlowContext( FlowContext &fc ) { *this = fc; }
   
   FlowContext &operator=( FlowContext &fc ) { pEnvContext = fc.pEnvContext; time = fc.time; timeStep = fc.timeStep;
         dayOfYear = fc.dayOfYear; pHRU = fc.pHRU; pHRUPool = fc.pHRUPool; pReach = fc.pReach; reachSubnodeIndex = fc.reachSubnodeIndex;
         iduIndex = fc.iduIndex; pFlux = fc.pFlux, pFluxInfo = fc.pFluxInfo; pFlowModel = fc.pFlowModel; 
         timing = fc.timing; svArray = fc.svArray; svCount = fc.svCount; initInfo = fc.initInfo; 
         return *this; }

   // data members
   EnvContext *pEnvContext;

   float time;             // time, in days. since the start of a run
   float timeStep;         // current time step used by the model
   int   dayOfYear;        // 0-based day of year

   HRUPool  *pHRUPool;   // memory managed elsewhere
   HRU       *pHRU;        // memory managed elsewhere
   Reach     *pReach;      // memory managed elsewhere
   int        reachSubnodeIndex;     // used for reach fluxes only
   int        iduIndex;              // used for IDU-level fluxes only
   Flux      *pFlux;       // memory managed elsewhere
   FluxInfo  *pFluxInfo;   // memory managed elsewhere
   FlowModel *pFlowModel;  // memory managed elsewhere

   int        timing;      // timing = 1   - called during Init();
                           // timing = 2   - called during InitRun();
                           // timing = 4   - called at the beginning of a Envision timestep (beginning of each year of run)  (PreRun)
                           // timing = 8   - called during GetCatchmentDerivatives() 
                           // timing = 16  - called during GetReachDerivatives() 
                           // timing = 32  - called during GetReservoirDerivatives() 
                           // timing = 64  - called at the end of a Envision timestep (beginning of each year of run) (PostRun)
                           // timing = 128 - called during EndRun()
                           // Note: timing values can be or'd together for multiple invocations
 
   SVTYPE     **svArray;   ///< array of references to state variables to be evaluated (memory managed elsewhere)
   int        svCount;     // size of the above array
   LPCTSTR    initInfo;

   void Reset( void ) { pHRUPool=NULL; pHRU=NULL; pReach=NULL; pFlux=NULL; pFluxInfo=NULL; reachSubnodeIndex = -1; iduIndex = -1; }

};


//-----------------------------------------------------------------------------
//------------------------- F L O W     I N T E R F A C E S -------------------
//-----------------------------------------------------------------------------
typedef float (PASCAL *FLUXFN)(FlowContext*);   ///< defines a catchment flux process (e.g. a "straw") within a catchment or reach.  Returns a flux

typedef BOOL  (PASCAL *FLUXINITFN)(FlowContext*, LPCTSTR initStr ); 
typedef BOOL  (PASCAL *FLUXINITRUNFN)(FlowContext*);  
typedef BOOL  (PASCAL *FLUXENDRUNFN)(FlowContext*);  

typedef FLUXFN DIRECTFN;

typedef FLUXINITFN    FLOWINITFN;
typedef FLUXINITRUNFN FLOWINITRUNFN;
typedef FLUXENDRUNFN  FLOWRUNFN;
typedef FLUXENDRUNFN  FLOWENDRUNFN;

