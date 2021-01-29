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

#include <Vdataobj.h>
#include <FDATAOBJ.H>
#include <PtrArray.h>
#include <UNITCONV.H>
#include <randgen\randunif.hpp>

#include "F2R.h"
#include "ClimateManager.h"
#include "VSMB.h"


const int ANNUAL_CROP    = 2;   // LULC_A codes
const int PERENNIAL_CROP = 3;

const int CORN           = 147; // LULC_B codes
const int ALFALFA        = 122;
const int SPRING_WHEAT   = 140;
const int BARLEY         = 133;
const int POTATOES       = 177;
const int SOYBEANS       = 158;
const int FALLOW         = 198;

const int MAX_DAILY_CORN_PLANTING_AC = 100;   // maximum daily corn that can be planted

// output data object indices 
enum { DO_PRECIP=0, DO_TMIN, DO_TMEAN, DO_TMAX, 
       DO_RX1_JAN, DO_RX1_FEB, DO_RX1_MAR, DO_RX1_APR, DO_RX1_MAY, DO_RX1_JUN, 
       DO_RX1_JUL, DO_RX1_AUG, DO_RX1_SEP, DO_RX1_OCT, DO_RX1_NOV, DO_RX1_DEC, 
       DO_RX3_JAN, DO_RX3_FEB, DO_RX3_MAR, DO_RX3_APR, DO_RX3_MAY, DO_RX3_JUN, 
       DO_RX3_JUL, DO_RX3_AUG, DO_RX3_SEP, DO_RX3_OCT, DO_RX3_NOV, DO_RX3_DEC, 
       DO_CDD, DO_R10MM, DO_STORM10, DO_STORM100, 
       DO_SHORTDURPRECIP, DO_EXTHEAT, DO_EXTCOLD, DO_GSL, DO_CHU, DO_CERGDD, 
       DO_ALFGDD, DO_PDAYS, DO_COUNT /* DO_COUNT is always last*/ };

// crop stage flags
enum { CS_NONCROP=-99, CS_PREPLANT=1, CS_PLANTED=2, CS_LATEVEGETATION=3, CS_POLLINATION=4, CS_REPRODUCTIVE=5, CS_HARVESTED=6, CS_FAILED=7 };

//-----------------------------------------------------------------------------------------------------
// crop event (status) flags
// Note: to add new crop events:
//     1) add an approriate enum below, making sure that CE_EVENTCOUNT is always the LAST enum, 
//     2) add an appropriate "AddCropEvent()" call in FarmModel::CheckCropConditions()
//     3) add a label for the event in proper location in gCropEventNames (in FarmModel.cpp)
//     4) Add a data column to the outputs in SetupOutputVars
//-----------------------------------------------------------------------------------------------------
enum { CE_UNDEFINED=-99, CE_PLANTED=0, CE_POORSEEDCOND, 
       CE_EARLY_FROST, CE_MID_FROST, CE_FALL_FROST, CE_WINTER_FROST, CE_COOL_NIGHTS,
       CE_WARM_NIGHTS, CE_POL_HEAT, CE_R2_HEAT, CE_EXTREME_HEAT,
       CE_VEG_DROUGHT, CE_POL_DROUGHT, CE_R2_DROUGHT, CE_R3_DROUGHT, CE_R5_DROUGHT, CE_POD_DROUGHT, CE_SEED_DROUGHT,
       CE_EARLY_FLOOD_DELAY_CORN_PLANTING,
       CE_EARLY_FLOOD_DELAY_SOY_PLANTING,
       CE_EARLY_FLOOD_DELAY_SOY_A,
       CE_EARLY_FLOOD_DELAY_SOY_B,
       CE_EARLY_FLOOD_DELAY_SOY_C,
       CE_COOL_SPRING, CE_EARLY_FLOOD_KILL, CE_MID_FLOOD,
       CE_CORN_TO_SOY, CE_CORN_TO_FALLOW, CE_SOY_TO_FALLOW,
       CE_WHEAT_TO_CORN, CE_BARLEY_TO_CORN,
       CE_HARVEST, CE_YIELD_FAILURE, CE_INCOMPLETE,
       CE_EVENTCOUNT /* this must always be last */ 
      };

// corn stages
enum { C_PREPLANTING=0, C_PLANTING, C_EMERGENCE, C_V1, C_V4, C_V6, C_V8, C_V12, C_VT, C_R1, C_R2, C_R3, C_R4, C_R5, C_R6 };

// alfalfa stages
enum { A_PREEMERGENCE, A_EMERGENCE, A_PREBUD, A_FLOWERING, A_SEEDPOD };

// potatoes
enum { P_PREEMERGENCE, P_EMERGENCE, P_TUBER_INITIATION, P_TUBER_GROWTH, P_MAX_BULKING, P_CESS_BULKING, P_MATURE };

// barley, wheat
enum { B_PREEMERGENCE, B_GERMINATION, B_LEAF_PRODUCTION, B_TILLERING, B_STEM_ELONGATION, B_BOOT, B_HEAD_EMERGENCE, B_FLOWERING, B_MILK, B_DOUGH, B_RIPENING };

// soybeans
enum { S_PREPLANTING, S_PREEMERGENCE, S_VE, S_VC, S_V2, S_V3, S_V4, S_V5, S_V6, S_R1, S_R2, S_R3, S_R4, S_R5, S_R6, S_R7, S_R8 };


// Farm Events
enum {FE_UNDEFINED=-99, FE_BOUGHT=0, FE_SOLD, FE_JOINED, FE_CHANGEDTYPE, FE_FIELDSMERGED, FE_ELIMINATED, FE_RECOVERED, FE_EVENTCOUNT };


// Farm types - Note: These MUST match what is in idu.xml AND the FarmModel XML input file
enum FARMTYPE {
   FT_UNK = 0,
   FT_CCF = 1,    // cow-calf and field crop
   FT_CCO = 2,    // cow-calf only
   // dairy
   FT_DYO = 3,
   FT_DFC = 4,    // Dairy and Field Crop
   FT_DYH = 5,    // Dairy and hay
   // field crop
   FT_FCC = 6,    // Field Crop and Cow/calf
   FT_FCH = 7,    // Field Crop and Hogs
   FT_FCF = 8,    // Field Crop feeder
   FT_FCD = 9,    // Field Crop and Dairy
   FT_FCG =10,    // Field Crop and Grains?
   FT_FCO =11,    // Field Crop and OtherLvst 
   // feeder
   FT_FDO =12,    // Feeder only (unused)
   FT_FDF =13,    // Feeder and Field Crop
   // hog
   FT_HOG =14,    // Hog only (unused)
   FT_HFC =15,    // Hog and Field Crop
   // other livestock
   FT_OLF =16,    // Other livestock and field crop (unused)
   FT_OTL =17,    // Other livestock
   // poultry
   FT_PE  =18,    // poultry and Eggs (unused)
   FT_PEF =19,    // Poultry, Egg and Field Crop
   // other
   FT_FV  =20,     // Fruit and vegetable crops
   FT_OTH =21,     // other (unused)
   FT_BE = 22,
   FT_COUNT = 23
   };

class Farm;



// farm count trajectory info.  One of these is created for each <fct> entry
// in the input file, generally reflecting a unique type and region
struct FCT_INFO
   {
   int regionID;
   FARMTYPE farmType;
   float area;    // current area in this type (not currently used)
   int   count;   // number of farms of this type in the region
   float slope;   // delta farm count/year

   long Key(void) { return MAKELONG((short)regionID, (short)farmType); }
   static long Key(int _regionID, int _farmType ) { return MAKELONG((short)_regionID, (short)_farmType); }

   FCT_INFO( int _regionID, FARMTYPE _farmType, float _slope) : regionID( _regionID ), farmType( _farmType ),
      area( 0 ), count( 0 ), slope( _slope ), m_deficitCount( 0 ) { farmArray.m_deleteOnDelete = false;  }

   // runtime support
   PtrArray< Farm > farmArray;   // list of all farms of this type in this region
   int m_deficitCount;   // current deficit in farm counts for this FCT_INFO
   };


// farm size trajectory info
struct FST_INFO
   {
   int regionID;
   FARMTYPE farmType;
   float area;    // current area in this type (m2)
   int   count;   // number of farms

   float initAvgSizeHa;   // remembers starting value
   float avgSizeHa;       // average farms size (ha)
  
   float slope;   // delta farm avg size(ha)/year

   CArray< Farm*, Farm* > farmArray;

   long Key(void) { return MAKELONG((short)regionID, (short)farmType); }
   static long Key(int _regionID, int _farmType) { return MAKELONG((short)_regionID, (short)_farmType); }

   FST_INFO(int _regionID, FARMTYPE _farmType, float _slope) : regionID(_regionID), farmType(_farmType),
      area(0), initAvgSizeHa(0), avgSizeHa(0), slope(_slope) { }

   float GetAvgSize() { return avgSizeHa = ( count == 0 ? 0 : ( area * HA_PER_M2 ) / count ); }    // ha
   float GetTarget( int yearOffset ) { return initAvgSizeHa + yearOffset*slope; }  // ha
   };


class FarmRotation
{
public:
   CString m_name;
   int     m_rotationID;
   float   m_initPctArea;     // decimal percent (0-1)

   // output variables
   float   m_totalArea;       // ha
   float   m_totalAreaPct;    // percent (0-100)
   float   m_totalAreaPctAg;  // percent (0-100)

   CArray< int, int > m_sequenceArray;
   CMap< int, int, int, int > m_sequenceMap;   // key=Attr code, value=index in sequence array

   FarmRotation( void ) : m_rotationID( -1 ), m_initPctArea( 0 ),
      m_totalArea( 0 ), m_totalAreaPct( 0 ), m_totalAreaPctAg( 0 ) { }
   
   //bool LookupLulc( int lulc, int &index );
   bool ContainCereals();
};


// A FarmField is a collection of IDUs that are adjacent and managed homegeneously
class FarmField
{
public:
   FarmField( Farm *pFarm) : m_id( m_idCounter++ ), m_pFarm( pFarm ), m_totalArea( 0 ) { }
   FarmField( FarmField &f ) : m_id( /*f.m_id*/ m_idCounter++ ), m_pFarm( f.m_pFarm ), m_totalArea( f.m_totalArea ) { m_iduArray.Copy( f.m_iduArray ); }

   void operator=( FarmField &f ) { m_id = f.m_id; m_pFarm = f.m_pFarm; m_totalArea = f.m_totalArea; m_iduArray.Copy( f.m_iduArray ); }

   bool IsFieldAdjacent(FarmField *pField, MapLayer *pIDULayer);
   
   // member data
   static  int m_idCounter;
   int   m_id;         // unique field ID
   Farm *m_pFarm;      // current owner of the field
   float m_totalArea;  // area of the field (m2)
   CArray< int, int > m_iduArray;   // initially, a single IDU

   SoilInfo *m_pSoil;  // for VSMB, mmemory is managed by the VSMB Model Instance
};


class FarmType
{
public:
   FarmType() : m_searchRadiusHQ(-1), m_searchRadiusField(-1), m_type( FT_UNK )
      { m_expandTypes.Add( this ); } // can always expand to same FarmType 

   // member data
   FARMTYPE m_type;

   CString m_name;
   CString m_code;          // 1-3 letter code in FT_Extents field
   CString m_expandTypesStr;
   CString m_expandRegionsStr;

   CArray< FarmRotation* > m_rotationArray;     // this just contain pointers to the FarmRotations managed by the FarmModel

   float   m_searchRadiusHQ;     // used during ExpandFarms();
   float   m_searchRadiusField;  // used during ExpandFarms();

   CArray< FarmType* > m_expandTypes;
   CStringArray m_expandRegions;
};


class Farm
{
public:
   int m_id;         // unique identifier in coverage. -99=non-farm, -1=Farm with no type, >=0 unique ID
   int m_climateStationID;
   int m_cadID;      // associated cadaster

   FARMTYPE m_farmType;
   int m_farmHQ;         // idu containing farm headquarters
   bool m_isPurchased;   // true if Farm has bveen purchased and consolidated into another farm
   bool m_isRetired;     // true is the farm is currently retired
   bool m_isExpandable;
   int m_lastExpansion;  // year in which the farm last expanded

   ClimateStation *m_pClimateStation;           // memory managed by the climate manager
   FarmRotation   *m_pCurrentRotation;          // rotational scheme employed by this Farm

public:
   PtrArray< FarmField > m_fieldArray;


   // runtime support  
   float m_totalArea;                     // m2
   float m_dailyPlantingAcresAvailable;   // Acres

public:
   Farm(void) : m_id(-1),
      m_cadID(-1),
      m_dailyPlantingAcresAvailable(0),
      m_climateStationID(-1),
      m_farmType(FT_UNK),
      m_farmHQ(-1),
      m_isPurchased(false),
      m_isRetired(false),
      m_isExpandable( false ),
      m_lastExpansion( -1 ), 
      m_pClimateStation( NULL ), 
      m_pCurrentRotation( NULL ), 
      m_totalArea( 0 ) { }

   float Transfer( EnvContext*, Farm *pFarm, MapLayer* ); //transfers another farm's assets to 'this' one
   void  ChangeFarmType(EnvContext*, FARMTYPE farmType);
   float AdjustFieldBoundaries( EnvContext*, int &count);
   bool IsActive() { return ! (m_isPurchased || m_isRetired); }
};


class FarmModel
{
public:
   FarmModel( void );
   ~FarmModel( void );

   bool Init   ( EnvContext *pEnvContext, LPCTSTR initStr );
   bool InitRun( EnvContext *pEnvContext );
   bool Run    ( EnvContext *pContext, bool useAddDelta=true );
   bool EndRun ( EnvContext *pEnvContext );

   int m_id;     // model ID = FARM_MODEL
   int m_doInit;
   bool m_useVSMB;

   PtrArray< FarmType >     m_farmTypeArray;
   PtrArray< Farm >         m_farmArray;
   PtrArray< FarmRotation > m_rotationArray;

   ClimateManager m_climateManager;

// exposed variables
protected:
   int m_climateScenarioID;

protected:
   CString m_farmIDField;
   CString m_farmTypeField;
   CString m_cadIDField;   // cadastre ID - deprecated
   CString m_lulcField;
   CString m_rotationField;
   CString m_regionField;
   
   CMap< int, int, Farm*, Farm* > m_farmMap;    // key=FARMID value, value=corresponding Farm with that ID
   CMap< int, int, FarmRotation*, FarmRotation* > m_rotationMap;   // key=rotation code, value=correspnding rotation
   CMap< int, int, FarmType*, FarmType* > m_farmTypeMap;           // key=FarmType ID, value=correspnding farm type

   CArray< int, int >     m_rotLulcArray;     // all lulc values used in a rotation
   CArray< float, float > m_rotLulcAreaArray; // all lulc areas used in a rotation

   CArray< int, int > m_trackIDUArray;
   CMap< int, int, int, int > m_trackIDUMap;   // key=IDU to track, value=index in m_trackIDArray for this IDU

   CArray< int, int > m_trackCropEventArray;
   CMap< int, int, int, int > m_trackCropEventMap;   // key=event to track, value=index in m_trackEventArray for this event

   CArray< int, int > m_trackFarmEventArray;
   CMap< int, int, int, int > m_trackFarmEventMap;   // key=event to track, value=index in m_trackEventArray for this event

public:
   static RandUniform m_rn;

   int m_colSLC;        // soil landscapoe unit
   int m_colLulcA;      // ????? check this!!!
   int m_colCropStatus; // "CROPSTATUS"        - output
   int m_colCropStage;  // "CROPSTAGE"        - output
   int m_colPlantDate;  // "PLANTDATE"       - output
   int m_colYieldRed;   // "YIELDRED"      - output
   int m_colYield;      // "YIELD" - output

   int m_colDairyAvg;   // "DairyAvg",   - calculated during BuildFarms
   int m_colBeefAvg;    // "BeefAvg",    - calculated during BuildFarms
   int m_colPigsAvg;    // "PigsAvg",    - calculated during BuildFarms
   int m_colPoultryAvg; // "Poultry_Av", - calculated during BuildFarms

   int m_colCadID;      // "CAD_ID"
   int m_colSoilID;     //

   // the following are accessed by multiple classes
   static int m_colFarmHQ;
   static int m_colFarmID;     // FARM_ID
   static int m_colFieldID;    // FIELD_ID
   static int m_colRegion;     // REGION_ID
   static int m_colLulc;       // idu coverage    - input
   static int m_colFarmType;   // FARM_TYPE
   static int m_colRotation;   // rotation code (id) for current rotation scheme
   static int m_colRotIndex;   // 0-based index of current location in sequence, no data if not in sequence
   static int m_colArea;       // "AREA"
   static int m_colCLI;        // "CLI_d_upda"

   // output variables
   float m_avgYieldReduction;
   float m_avgFarmSizeHa;
   int m_adjFieldCount;
   float m_adjFieldArea;
   float m_adjFieldAreaHa;
   float m_avgFieldSizeHa;

   float m_cropEvents[ 1+CE_EVENTCOUNT ];  // current year crop event hectares
   float m_farmEvents[ 1+FE_EVENTCOUNT];   // current year farm event hectares

   float m_yrfThreshold;   // max yield reduction factor before crop is considered failed
                           // in yield reduction factor that triggers tracking
   FDataObj *m_pDailyData;

   // crop event count, totals
   FDataObj *m_pCropEventData;  // Time + CE_EVENTCOUNT cols; each col= ha of each crop event type, time=year
   VDataObj *m_pCropEventPivotTable; // year, doy, eventcode, ... , cropYRF - summaries for each day

   FDataObj *m_pFarmEventData;  // Time + FE_EVENTCOUNT cols; each col= ha of each crop event type, time=year
   VDataObj *m_pFarmEventPivotTable; // year, doy, eventcode, ... , cropYRF - summaries for each day



   // climate indicators
   PtrArray< FDataObj > m_dailyCIArray;   // one for each station, one total, daily
   PtrArray< FDataObj > m_annualCIArray;  // one for each station, one total, annual

   // farm expansion parameters
   bool m_enableFarmExpansion;
   FDataObj m_farmExpRateTable;
   int m_reExpandPeriod;
   int m_maxFarmArea;
   static int m_maxFieldArea;

   // Farm expansion data
   FDataObj *m_pFarmTypeExpandCountData;

   FDataObj *m_pFarmTypeCountData;
   VDataObj *m_pFarmExpTransMatrix;
   VDataObj *m_pFarmTypeTransMatrix;

   CSortableUIntArray m_regionsArray;
   CMap< int, int, int, int > m_regionsIndexMap;    // key = region, value = index in m_regionsArray
   CMap< int, int, int, int > m_farmTypesIndexMap;  // key =FARMTYPE, value = index in m_farmTypeArray

   // farm count by type and region
   bool m_enableFCT;
   PtrArray< FCT_INFO > m_fctArray;
   CMap< long, long, FCT_INFO*, FCT_INFO* > m_fctMap;

   FDataObj *m_pFCTRData;

   // farm size by type and region
   bool m_enableFST;
   PtrArray< FST_INFO > m_fstArray;     // farm size trajectories
   CMap< long, long, FST_INFO*, FST_INFO* > m_fstMap;   // key is Key(regionID,farmType), value is pointer to FST_INFO

   FDataObj *m_pFarmSizeTRData;        // farm size by Farm type and region, cols =1+fstCount
   FDataObj *m_pFarmSizeTData;         // aggregated - farm size by Farm Type, cols=1+farmTypeCount

   FDataObj *m_pFldSizeLData;    // field size by LULC_B
   FDataObj *m_pFldSizeLRData;   // field size by LULC_B and region
   FDataObj *m_pFldSizeTRData;   // field size by Farmt Type and Region

   CMap< int, int, int, int> m_agLulcBCols;    // key = lulc b class, value is column in data obj
   CMap< int, int, LPCTSTR, LPCTSTR > m_agLulcBNames;      // key = lulc b class, value is name



protected:
   float m_totalIDUArea;
   float m_totalIDUAreaAg;

   // initialization support
   bool LoadXml( EnvContext*, LPCTSTR filename );

   int  InitializeFarms( MapLayer *pMapLayer );
   void SetupOutputVars( EnvContext* );
   int  BuildFarms( MapLayer *pLayer );
   void AllocateInitialCropRotations( MapLayer *pLayer );
   FDataObj *BuildOutputClimateDataObj( LPCTSTR label, bool isDaily );
   FarmType *FindFarmTypeFromID( int id ){ FarmType *pType = NULL; return ( m_farmTypeMap.Lookup( id, pType ) ? pType : NULL ); }
   Farm *FindFarmFromID( int id ) { Farm *pFarm=NULL; return ( m_farmMap.Lookup( id, pFarm ) ? pFarm : NULL); }
   //bool IsFarmExpandable(Farm *pFarm, MapLayer *pIDULayer, int currentYear);

   // run support
   bool ResetAnnualData( EnvContext* );
   bool UpdateFarmCounts( EnvContext* );
   bool RotateCrops( EnvContext*, bool );
   bool GrowCrops( EnvContext*, bool );
   int  ConsolidateFields( EnvContext* );
   void UpdateAnnualOutputs( EnvContext*, bool useAddDelta );
   void UpdateDailyOutputs( int doy, EnvContext* );   // doy is one-based
   float AddCropEvent( EnvContext*, int idu, int status, float areaHa, int doy, float yrf, float priorCumYRF );

   void  AddFarmEvent( EnvContext*, int idu, int eventID, int farmID, int farmType, float areaHa );

   bool IsAnnualCrop( Farm *pFarm, MapLayer *pLayer );
   bool IsAg( MapLayer*, int idu );
   float CheckCropConditions( EnvContext*, Farm*, MapLayer*, int idu, int doy, int year, float priorCumYRF );
   void GetAnnualYield( int lulc,float precipJun1toAugust1, float heatUnitsAbove10, float &yield );
   bool ExpandFarms( EnvContext*);
   int  GetQualifiedNeighbors( MapLayer*, Farm*, float radius, CArray<int, int> &neighbors, CArray< float, float > &distances );
   
   //bool InitFSTInfo( MapLayer* );
   bool UpdateAvgFarmSizeByRegion( MapLayer *pLayer, bool initialize );
   bool UpdateFSTInfo(MapLayer *pLayer, Farm *pFarm, Farm *pNeighborFarm);

   VSMBModel m_vsmb;

protected:
   int m_maxProcessors;
   int m_processorsUsed;
   bool m_outputPivotTable;

};

inline
bool FarmModel::IsAg( MapLayer *pLayer, int idu )
   {
   int lulcA = -1;
   pLayer->GetData(idu, m_colLulcA, lulcA);

   float area = 0;
   pLayer->GetData(idu, m_colArea, area);

   if (lulcA == ANNUAL_CROP || lulcA == PERENNIAL_CROP)
      return true;
   else
      return false;
   }