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
#ifndef COCNH_H
#define COCNH_H

#include <EnvExtension.h>
#include <map>
#include <VDataobj.h>
#include <vector>
#include <misc.h>
#include <PtrArray.h>
#include <QueryEngine.h>
#include <IDATAOBJ.H>
#include <EnvConstants.h>

using namespace std;


#define _EXPORT __declspec( dllexport )


struct VEGKEY
   {
   __int32 veg;
   __int16 region;
   __int16 projectID;

   VEGKEY(__int32 _veg, __int16 _region, __int16 _projectID) : veg(_veg), region(_region), projectID(_projectID) { }

   __int64 GetKey(void) { __int64 key = MAKE< __int64 >(veg, (MAKE< __int32 >(region, projectID))); return key; }
   };


enum VEGCOLINDEX
   {
   BAA_GE_3,
   TreesPha,
   PUTR2_AVGCOV,
   JunipPha,
   LIVEBIO_MGHA,
   TOTDEADBIO_MGHA,
   LIVEC_MGHA,
   TOTDEADC_MGHA,
   VPH_GE_3,
   TOTDEADVOL_M3HA,
   VPH_3_25,
   VPH_25_50,
   VPH_GE_50,
   BPH_3_25,
	TPH_GE_50,
   VEGMAPCOLS
   };

class VegLookup : public CMap< __int64, __int64, int, int >
   {
   public:
      VegLookup(void) : m_inputTable(U_UNDEFINED), m_isInitialized(false) { }

      int m_colArray[VEGMAPCOLS];

      bool Init(LPCTSTR filename);

      //float GetColData( VEGKEY &key, VEGCOLINDEX colIndex )
      //   {
      //   int row = -1;
      //   BOOL found = Lookup( key.GetKey(), row );
      //
      //   if ( found && row >= 0 )
      //      {
      //      int col = m_colArray[ colIndex ];
      //      return m_inputTable.GetAsFloat( col, row );
      //      }
      //
      //   return 0;
      //   }
      //
      //float GetColData( int vegClass, int region, int pvt, VEGCOLINDEX colIndex )
      //   {
      //   ASSERT( m_isInitialized );
      //   int row = -1; 
      //   VEGKEY key( vegClass, __int16( region ), __int16( pvt ) );
      //   return GetColData( key, colIndex );
      //   }

      float GetColData(int row, VEGCOLINDEX colIndex)
         {
         ASSERT(m_isInitialized);
         int col = m_colArray[colIndex];
         return m_inputTable.GetAsFloat(col, row);
         }

   protected:
      VDataObj m_inputTable;

      bool m_isInitialized;
   };



struct FUELMODELKEY
   {
   __int32 veg;
   __int16 variant;
   __int8  pvt;
   __int8  region;

   FUELMODELKEY(__int32 _veg, __int16 _variant, __int8 _pvt, __int8 _region) : veg(_veg), variant(_variant), pvt(_pvt), region(_region) { }

   __int64 GetKey(void) { __int64 key = MAKE< __int64 >(veg, (MAKE< __int32 >(variant, MAKE< __int16 >(pvt, region)))); return key; }
   };


//enum FUELMODELCOLINDEX
//   {
//   REGION,
//   PVT,
//   VEGCLASS,
//   VARIANT_,
//   LCP_FUEL_MODEL,
//   FIRE_REGIME,
//   TIV,
//   FUELMODELMAPCOLS
//   };


class FuelModelLookup : public CMap< __int64, __int64, int, int >
   {
   public:
      FuelModelLookup(void)
         : m_isInitialized(false)
         , m_inputTable(U_UNDEFINED)
         , m_lutColPVT(-1)
         , m_lutColVegClass(-1)
         , m_lutColVariant(-1)
         , m_lutColRegion(-1)
         , m_lutColTIV(-1)
         , m_lutColFuelModel(-1)
         , m_lutColFireRegime(-1)
         , m_lutColSfSmoke(-1)
         , m_lutColMxSmoke(-1)
         , m_lutColSrSmoke(-1)
         { }

      //int m_colArray[ FUELMODELMAPCOLS ];

      bool Init(LPCTSTR filename);

      // lookup table cols
      int m_lutColPVT;
      int m_lutColVegClass;
      int m_lutColVariant;
      int m_lutColRegion;
      int m_lutColTIV;
      int m_lutColFuelModel;
      int m_lutColFireRegime;
      int m_lutColSfSmoke;
      int m_lutColMxSmoke;
      int m_lutColSrSmoke;
      
      float GetColData(int row, int colIndex)
         {
         ASSERT(m_isInitialized);
         //int col = m_colArray[ colIndex ];
         return m_inputTable.GetAsFloat(colIndex, row);
         }

   protected:
      VDataObj m_inputTable;

      bool m_isInitialized;

      //int m_colPVT;
      //int m_colVegClass;
      //int m_colVariant;
   };


// does this fuel model match the pvt, vegclass and variant?


struct PLAN_AREA_INFO
   {
   int    id;
   int    index;     // offset in array
   float  area;
   double score;
   int    lastUsed;  // year last used
   float  areaFracUsed;  // fraction of area actually used (treated) within the last ten yeaers

   CArray <float> areaArray;    // FIFO queue with year areas treated, size =   


   PLAN_AREA_INFO(void) : id(-1), index( -1 ), area(0), score(0), lastUsed(-1 ), areaFracUsed( 0 ) { }
   };




/*---------------------------------------------------------------------------------------------------------------------------------------
* general idea:
*    1) Create subclasses of EnvEvalModel, EnvAutoProcess for any models you want to implement
*    2) Add any exposed variables by calling
*         bool EnvExtension::DefineVar( bool input, int index, LPCTSTR name, void *pVar, TYPE type, MODEL_DISTR distType, VData paramLocation, VData paramScale, VData paramShape, LPCTSTR desc );
*         for any exposed variables
*    3) Override any of the methods defined in EnvExtension classes as needed
*    4) Update dllmain.cpp
*    5) Update *.def file
*
*
*  Required Project Settings (<All Configurations>)
*
*  Right-click on your project in Visual Studio’s Solution Pane and select <Properties>.  This allows you to
*  set various project settings.  Change the following, being sure to select <All Configurations>.
*
*  Required Project Settings (<All Configurations>)
*   1) General->Character Set: Not Set
*   2) C/C++ ->General->Additional Include Directories: ..\;..\libs;
*   3) Linker->General->Additional Library Directories:  $(SolutionDir)$(ConfigurationName)
*   4) Linker->Input->Additional Dependencies: libs.lib
*
*  For each configuration (Debug and Release):
*   1) C/C++ ->Preprocessor->Preprocessor Definitions: add "__EXPORT__=__declspec( dllimport )";  Do not include the quotes!
*---------------------------------------------------------------------------------------------------------------------------------------
*/


/////////////////////////////////////////////////////////////////////////
/// COCNHProcess - this a shared class among the three AutoProcesses
/////////////////////////////////////////////////////////////////////////

class COCNHProcess : public EnvModelProcess
   {
   public:
      COCNHProcess() : EnvModelProcess() { }
      ~COCNHProcess(void) { if ( m_pQueryEngine == NULL ) delete m_pQueryEngine; m_pQueryEngine = NULL; }
      
      bool Init(EnvContext *pContext, LPCTSTR initStr);

   protected:
      bool LoadXml( LPCTSTR filename );

      //! Get IDU.shp column numbers.
      //! Get the column numbers for all columns required to read or write information
      //! used in Init()
      bool DefineColumnNumbers(EnvContext *pContext);

      //! Update variables related to cover type and structural stage.
      //! Update information, that is based on changes in VEGCLAss variable. This includes cover type,
      //! structural stage, canopy cover, size, layers.
      //! Used in COCNHProcess::Init(), COCHNProcessPost::Run()
      bool UpdateVegClassVars(EnvContext *pContext, bool useAddDelta);

      //! Used where needed in the COCNH process.  Scans delta array for VEGCLASS change and
      //! updates IDU layer attribute PRIOR_VEG
      bool UpdatePriorVeg(EnvContext *pContext);

      //! Write vegetation structure information to IDUs
      //! Write vegetation structure information derived from lookup table to IDU. This includes variables
      //! such as values basal area, biomass, standage....
      //! Used in COCNHProcess::Init(), COCHNProcessPre2::Run(), COCNHProcessPost::Run();
      bool UpdateVegParamsFromTable(EnvContext *pContext, bool useAddDelta);
		bool InitVegParamsFromTable(EnvContext *pContext, bool useAddDelta);

      // called in Pre2
      bool UpdateAvailVolumesFromTable(EnvContext *pContext, bool useAddDelta);

      //! Update disturbance value.
      //! This function sets the value in the DISTURB column to -DISTURB, if there is an entry > 0. This is related to 
      //! hardcoded entry in FlameLenDisturbHandler line 181: if ( disturb > 0 ) { newvariant = 1;...
      //! Used in COCNHProcessPre1::Run()
      bool UpdateDisturbanceValue(EnvContext *pContext, bool useAddDelta);

      //! Load the fuelmodel lookup table from .csv file.
      //! The lookup table includes the relationship between a specific CTSS numerical code and
      //! and the corresponding fire related values such as fuel model, fire regime, ....
      //! Used in COCNHProcess::Init()
      bool LoadLookupTableFuelModels(EnvContext *pContext, LPCSTR initStr);

      //! Write the fuelmodel information to IDU.shp.
      //! Write fire and fuel related information derived from lookup table to IDU.shp. This includes variables
      //! such as fuel model, fire regime,....
      //! Used in COCNHProcess::Init(), COCHNProcessPre2::Run(), COCNHProcessPost::Run();
      bool UpdateFuelModel(EnvContext *pContext, bool useAddDelta);

      //! Calculate fuel treatment costs.
      //! Calculate fuel treatment costs based on Calkin and Gebert (2006) Modeling Fuel Treatment Costs
      //! on Forest Service Lands in the Western United States. WJAF 21(4).
      //! Used in COCNHProcess::Init(), COCHNProcessPre2::Run(), COCNHProcessPost::Run();
      bool CalcTreatmentCost(EnvContext *pContext, bool useAddDelta);

      //! Sets PRIOR_VEG to the VEGCLASS value at the beginning of time step
      //! Used in COCHNProcessPre1::Run()
      bool SetPriorVegClass(EnvContext *pContext);

      //! Update the time since treatment variable TST.
      //! Set to 0 if treatment in same timestep, otherwise count 1 up.
      //! Used in COCHNProcessPre1::Run()
      bool UpdateTimeSinceTreatment(EnvContext *pContext);

      //! Update time since harvest variable TSH.
      //! Set to 0 if harvest in same timestep, otherwise count 1 up.
      //! Used in COCHNProcessPre1::Run()
      bool UpdateTimeSinceHarvest(EnvContext *pContext);

      //! Used in COCHNProcessPre1::Run()
      bool UpdateTimeSincePartialHarvest(EnvContext *pContext);

      //! Used in COCHNProcessPre1::Run()
      bool UpdateTimeSinceThinning(EnvContext *pContext);

      //! Update time in variant variable TIV.
      //! Set to 0 if variant changes in same timestep, otherwise count 1 up.
      //! Used in COCHNProcessPre1::Run()
      bool UpdateTimeInVariant(EnvContext *pContext);

      //! Used in COCHNProcessPre1::Run()
      bool UpdateTimeSinceFire(EnvContext *pContext);

      //! Used in COCHNProcessPre1::Run()
      bool UpdateTimeSincePrescribedFire(EnvContext *pContext);

      //! Update column "TESPECIES" - threated or endangered species (0/1).
      //! Update value in column "TESPECIES" based on habitat suitability models provided
      //! by Anita Morzillo from the ILAP projectk; defined in TerrBiodiv.dll.
      //! Used in COCHNProcess::Init(), COCHNProcessPost::Run()
      bool UpdateThreatenedSpecies(EnvContext *pContext, bool useAddDelta);

      //! Update WUI category.
      //! Update WUI category (interface, intermixed, subcategories) based on population density
      //! and landscape characteristics/neighborhood.
      //! Used in COCHNProcess::Init(), COCHNProcessPre1::Run()
      bool PopulateWUI(EnvContext *pContext, bool useAddDelta);

      //! Currently not implemented.
      bool PopulateLulc(EnvContext *pContext, bool useDelta);

      //!  Not currently used
      bool PopulateStructure(EnvContext *pContext, bool useAddDelta);

      //!  Not currently used
      bool UpdateAvgTreesPerHectare(EnvContext *pContext);

      //!  Not currently used
      bool UpdateAvgCondFlameLength(EnvContext *pContext);

      //! Used in COCHNProcessPre2::Run()
      bool UpdateFireOccurrences(EnvContext *pContext);

      //! Used by spatial allocator, Used in COCHNProcessPre2::Run()
      bool ScoreAllocationAreas(EnvContext *pContext);
      bool ScoreAllocationAreasFire(EnvContext *pContext);
		
		// calculates probability and sets FIREWISE attribute in IDU layer for home owner, 1-TRUE, 0-FALSE 
		bool CalculateFirewise(EnvContext *pContext);
      /*
      //!
      bool IsResidential( int zone );
      //!
      bool IsCommInd( int zone );
      //!
      bool IsUrban( int zone );
      //!
      bool IsRuralRes( int zone );
      */


      /* not required, koch 02/2013
      //! Load the Lookup Table from .csv file.
      //! The lookup table includes the relationship between a specific CTSS numerical codes and
      //! and the corresponding values basal area, biomass, standage....
      bool LoadLookupTableTreatCosts( EnvContext *pContext, LPCSTR initStr );
      */


   public:
      static bool m_isInitialized;

      static int m_wuiUpdateFreq;
      static int m_initWUI;

      //! Column number in the IDU table for VEGCLASS, which is the CTSS identifier.
      static int m_colVegClass;
      //! Colum number in the IDU table for Cover Type, which is the dominant species.
      static int m_colCoverType;
      //! Column number in the IDU table for Structural Stage.
      static int m_colStructStg;
      //! Column number in the IDU table for Canopy Cover.
      static int m_colCanopyCover;
      //! Column number in the IDU table for Size class.
      static int m_colSize;
      //! Column number in the IDU table for number of layers
      static int m_colLayers;
      //! Column number in the IDU table for basal area.
      static int m_colBasalArea;
      //!Column number in the IDU table for applied decision rule identifier.
      static int m_colPolicyID;
      //! Column number in the IDU table for Time Since Distrubance.
      static int m_colTSD;
      //! Column number in the IDU table for Time Since Treatment.
      static int m_colTST;
      //! Column number in the IDU table for Time Since Thinning.
      static int m_colTSTH;
      //! Column number in the IDU table for Population Density.
      static int m_colPopDens;
      //! Column number in the IDU table for the IDU area.
      static int m_colArea;
      //! Column number in the IDU table for Wildland Urban Interface category.
      static int m_colWUI;
      //! Column number in the IDU table for Disturbance code.
      static int m_colDisturb;
      //! Column number in the IDU table for Potential Disturbance code.
      static int m_colPdisturb;    
		//! Column number in the IDU table for Potential Flame Length code.
		static int m_colPotentialFlameLen;
		//! Column number in the IDU table for Potential Vegetation Type.
      static int m_colPVT;
      //!
      //static int m_colPVTType;
      //! Column number in the IDU table for Time Since last Harvest.
      static int m_colTSH;
      //! Column number in the IDU table for Time Since last Partial Harvest.
      static int m_colTSPH;
      //!
      static int m_colTSF;
      //!
      static int m_colTSPF;
      //! Column number in the IDU table for Fuel Model.
      static int m_colFuelModel;
      //! Column number in the IDU table for aspect.
      static int m_colAspect;
      //! Column number in the IDU table for elevation.
      static int m_colElevation;
      //! Column number in the IDU table for prescribed fire fuel treatment w/o hand pile.
      static int m_colCostsPrescribedFire;
      //! Column number in the IDU table for prescribed fire fuel treatment w/ hand pile.
      static int m_colCostsPrescribedFireHand;
      //! Column number in the IDU table for mechanical fuel treatment w/o biomass.
      static int m_colCostsMechTreatment;
      //! Column number in the IDU table for mechanical fuel treatment w/ biomass.
      static int m_colCostsMechTreatmentBio;
      //! Column number in the IDU table for treatment identifier.
      static int m_colTreatment;
      //! Column number in the IDU table for time in current fuel model variant
      static int m_colTimeInVariant;

      //! Column number in the IDU table for VDDT region.
      static int m_colRegion; // REGION

      //! Column number in the IDU table for fuel model variant.
      static int m_colVariant;
      //! Column number in the IDU table for Trees Per Hectare.
      static int m_colTreesPerHectare;
      //! Column number in the IDU table for Junipers Per Hectare
      static int m_colJunipersPerHectare;
      //! Column number in the IDU table for percentage of Bitterbrush Cover (Purshia).
      static int m_colBitterbrushCover;
      //! Column number in the IDU table for Live Carbon [ Mg ha-1].
      static int m_colLCMgha;
      //! Column number in the IDU table for Dead Carbon [ Mg ha-1].
      static int m_colDCMgha;
      //! Column number in the IDU table for Total Carbon [ Mg ha-1].
      static int m_colTCMgha;
      //! Column number in the IDU table for Live Biomass Volume [ m3 ha-1].
      static int m_colLVolm3ha;
      //! Column number in the IDU table for Dead Biomass Volume [ m3 ha-1].
      static int m_colDVolm3ha;
      //! Column number in the IDU table for Total Biomass Volume [ m3 ha-1].
      static int m_colTVolm3ha;
      //! Column number in the IDU table for Live Biomass amount [ Mg ha-1].
      static int m_colLBioMgha;
      //! Column number in the IDU table for Dead Biomass amount [ Mg ha-1].
      static int m_colDBioMgha;
      //! Column number in the IDU table for Total Biomass amount [ Mg ha-1].
      static int m_colTBioMgha;
      //!
      static int m_colVPH_3_25;
      //!
      static int m_colBPH_3_25;
      //! Column number in the IDU table for Fire Regime.
      static int m_colLVol25_50;
      //!
      static int m_colSawTimberVolume;
		//!
      static int m_colSawTimberHarvVolume;
      //!
      static int m_colLVolGe50;
      //!
      static int m_colFireReg;
      //! Column number in the IDU table for Threatened or Endangered Species.
      static int m_colThreEndSpecies;
      //!
      static int m_colFire1000;
		//!
      static int m_colFire500;
      //!
      static int m_colSRFire1000;
      //!
      static int m_colMXFire1000;
		//!
      static int m_colPotentialFire500;
      //!
      static int m_colPotentialFire1000;
		//!
      static int m_colAvePotentialFlameLength1000;
      //!
      static int m_colPotentialSRFire1000;
      //!
      static int m_colPotentialMXFire1000;
      //!
      static int m_colFire2000;
      //!
      static int m_colFire10000;
		//!
		static int m_colPrescribedFire10000;
      //!
      static int m_colPrescribedFire2000;
      //!
      static int m_colCondFlameLength270;
      //!
      static int m_colTreesPerHect500;
      //!
      static int m_colStructure;

      static int m_colFireKill;
      static int m_colHarvestVol;

      static int m_colPriorVeg;
      static int m_colPFDeadBio;
      static int m_colPFDeadCarb;

      static int m_colPlanAreaScore;
      static int m_colPlanAreaScoreFire;

      static int m_colNDU;

      static int m_colNewDU;
      static int m_colSmoke; // smoke generated in kg/ha

      static int m_colPlanArea;  // ALLOC_PLAN
      static int m_colPlanAreaFr;  // PA_FRAC
      static int m_colPlanAreaFireFr;  // PAF_FRAC
		static int m_colPFSawVol; // post fire saw timber volume, decays after fire event
	   static int m_colPFSawHarvestVol; // post fire saw timber volume, decays after fire event
		static int m_colOwner; // OWNER attribute in IDU layer
		static int   m_colAvailVolTFB;
		static int	m_colAvailVolPH;
		static int	m_colAvailVolSH;
		static int	m_colAvailVolPHH;
		static int	m_colAvailVolRH;
		static int  m_colFireWise;
		static int  m_colVegTranType;
		static IDataObj m_initialLandscpAttribs;
      static bool m_restoreValuesOnInitRun;

   protected:
      CString
         //! Error message.
         msg,
         //! Error identifier.
         failureID;

      static VegLookup        m_vegLookupTable;
      static FuelModelLookup  m_fuelModelLookupTable;

      //FDataObj m_structureInputTable;         // vegetation structure input table.  
      //FDataObj m_fuelmodelInputTable;

      //! Map for vegetation structure information from lookup table.
      //map< vector< int >, vector< float >> m_mapLookupVegStructure;

      //!
      map< int, float> m_mapLookupTreatCostsFloat;
		
      //! Map for fuel model information from lookup table.
      //map< vector< int >, vector< int > >  m_mapLookupFuelModel;

      vector<string> m_path;
		static RandUniform   m_rn;

   public:
      static QueryEngine *m_pQueryEngine;   // delete???

   protected:
      static CString m_planAreaQueryStr;
      static float   m_minPlanAreaFrac;  // decimal
      static int     m_minPlanAreaReuseTime;
      static int     m_planAreaTreatmentWindow;
      static Query  *m_pPAQuery;

      static CString m_planAreaFireQueryStr;
      static float   m_minPlanAreaFireFrac;
      static int     m_planAreaFireTreatmentWindow;
      static int     m_minPlanAreaFireReuseTime;
      static Query  *m_pPAFQuery;

		static float   m_percentOfLivePassedOn3;
		static float   m_percentOfLivePassedOn20;
		static float   m_percentOfLivePassedOn21;
		static float   m_percentOfLivePassedOn23;
		static float   m_percentOfLivePassedOn29;
		static float   m_percentOfLivePassedOn31;
		static float   m_percentOfLivePassedOn32;
		static float   m_percentOfLivePassedOn51;
		static float   m_percentOfLivePassedOn52;
		static float   m_percentOfLivePassedOn55;
		static float   m_percentOfLivePassedOn56;
		static float   m_percentOfLivePassedOn57;

		static float   m_percentPreTransStructAvailable55;
		static float	m_percentPreTransStructAvailable3;
		static float	m_percentPreTransStructAvailable52;
		static float	m_percentPreTransStructAvailable57;
		static float	m_percentPreTransStructAvailable1;

		static float m_priorBasalArea;
		static float m_priorTreesPerHectare;
		static float m_priorBitterbrushCover;
		static float m_priorJunipersPerHa;
		static float m_priorLiveBiomass;
		static float m_priorDeadBiomass;
		static float m_priorLiveCarbon;
		static float m_priorDeadCarbon;
		static float m_priorTotalCarbon;
		static float m_priorLiveVolumeGe3;
		static float m_priorLiveVolume_3_25;
		static float m_priorLiveBiomass_3_25;
		static float m_priorLiveVolume_25_50;
		static float m_priorLiveVolumeGe50;
		static float m_priorDeadVolume;
		static float m_priorTotalVolume;
		static float m_priorSmallDiaVolume;
		static float m_priorSawTimberVol;

		static CArray< int, int >  m_accountedForFireThisStep;
		static CArray< int, int >  m_accountedForNoTransThisStep;
		static CArray< int, int >  m_accountedForSuccessionThisStep;
		static CArray< int, int >  m_timeSinceFirewise;

		static CArray< float, float > m_priorLiveVolumeGe3IDUArray;   
		static CArray< float, float > m_priorLiveVolumeSawTimberIDUArray;
		static CArray< float, float > m_priorLiveBiomassIDUArray;
		static CArray< float, float > m_priorLiveCarbonIDUArray;
		static CArray< float, float > m_priorLiveVolume_3_25IDUArray;

   public:
      // info for plan area scoring
      static PtrArray< PLAN_AREA_INFO > m_planAreaArray;
      static CMap< int, int, PLAN_AREA_INFO*, PLAN_AREA_INFO* > m_planAreaMap;  // key=plan area ID

      static PtrArray< PLAN_AREA_INFO > m_planAreaFireArray;
      static CMap< int, int, PLAN_AREA_INFO*, PLAN_AREA_INFO* > m_planAreaFireMap;  // key=plan area ID
   
public:
      
      //! Path to file with lookup table for vegetation structure variables.
      static CString m_vegStructurePath;

      //! Paht to file with lookup table for fuel model variables.
      static CString m_fuelModelPath;
   };


/////////////////////////////////////////////////////
/// COCNHProcessPre
/////////////////////////////////////////////////////

class COCNHProcessPre1 : public COCNHProcess
   {
   public:
      bool Run(EnvContext *pContext);
   };


/////////////////////////////////////////////////////
/// COCNHProcessPre2
/////////////////////////////////////////////////////

class COCNHProcessPre2 : public COCNHProcess
   {
   public:
      COCNHProcessPre2(void) : COCNHProcess() { }

      bool Init(EnvContext *pContext, LPCTSTR initStr);
      bool InitRun(EnvContext *pContext, bool useInitSeed);

      bool Run(EnvContext *pContext);

	protected:
	   int  CalcFireEffect(EnvContext*, bool useAddDelta);
		int  DecayDeadResiduals(EnvContext*, bool useAddDelta);
		
   };


/////////////////////////////////////////////////////
/// COCNHProcessPost
/////////////////////////////////////////////////////

class COCNHProcessPost1 : public COCNHProcess
   {
   public:
      COCNHProcessPost1();
      ~COCNHProcessPost1(void);

      bool Init(EnvContext *pContext, LPCTSTR initStr);
      bool Run(EnvContext *pContext);

   protected:
      float m_disturb3Ha;
      float m_disturb29Ha;
      float m_disturb55Ha;
      float m_disturb51Ha;
      float m_disturb54Ha;
      float m_disturb52Ha;
      float m_disturb6Ha;
      float m_disturb1Ha;
   };


class COCNHProcessPost2 : public COCNHProcess
   {
   public:

      COCNHProcessPost2(void) : COCNHProcess() { }

      bool Run(EnvContext *pContext);

   protected:
      int  CalcHarvestBiomass(EnvContext*, bool useAddDelta);
      int  UpdatePlanAreas( EnvContext *pContext );
   };


class COCNHSetTSD : public COCNHProcess
   {
   public:
      COCNHSetTSD(void) : COCNHProcess()  { }
      ~COCNHSetTSD(void) { }

      bool Run(EnvContext *pContext);
   protected:
   };

#endif
