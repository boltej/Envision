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

#include "globalMethods.h"
#include "WaterMaster.h"
#include "FlowContext.h"
#include <EnvExtension.h>
#include <PtrArray.h>
#include <Vdataobj.h>
#include <vector>
#include <map>
#include <EnvConstants.h>
//#include <EnvInterface.h>
#include "reachtree.h"
#include "GeoSpatialDataObj.h"

#include "FlowInterface.h"

using namespace std;

#define MAX_UGA_NDX 72

enum IrrigationState
   {
   IRR_NOT_STARTED, IRR_FULL, IRR_PARTIAL, IRR_SUSPENDED, IRR_SHUT_OFF_BY_REG_ACTION 
   };

class AltWaterMaster
{
public:
	AltWaterMaster(WaterAllocation *pWaterAllocation);
	~AltWaterMaster(void);

	// Timing methods
	bool Init(FlowContext *pFlowContext);
	bool InitRun(FlowContext *pFlowContext);
	bool Step(FlowContext *pFlowContext);
	bool EndStep(FlowContext *pFlowContext);
	bool EndYear(FlowContext *pFlowContext);
	bool StartYear(FlowContext *pFlowContext);

   bool LoadXml( WaterAllocation*, TiXmlElement*, LPCTSTR filename );
	// Method for selecting dynamically added water rights
	bool DynamicWaterRights( FlowContext *pFlowContext, int scenarioValue, int stepInterval ); // based on a Scenario, selects a method for adding water rights to the IDU layer

	// Methods for dynamically adding water rights
   bool ExtremeResWaterRights(FlowContext *pFlowContext, float radius ); // Adds a new irrigation water right to IDUs within a radius of reaches below a reservoir(s).	
	bool ZoneTargetResWaterRights(FlowContext *pFlowContext, int currentStep, int stepInterval, float radius); //Adds a new irrigation water right for reaches below reservoirs based on zones, and targets for the zones.

	// functions for times of shortage
	int  ApplyRegulation(FlowContext *pFlowContext, int regulationType, WaterRight *pWaterRight, int depth, float deficit);
	int  RegulatoryShutOff(FlowContext *pFlowContext, WaterRight *pWaterRight, int depth, float deficit);
	int  NoteJuniorWaterRights(FlowContext *pFlowContext, WaterRight *pWaterRight, int depth, float deficit);
   int  GetJuniorWaterRights(FlowContext *pFlowContext, Reach *pReach, WaterRight *pWaterRight, int depth, CArray<WaterRight*, WaterRight*> *wrArray);

	// Utility functions
   int IDUatXY(double xCoord, double yCoord, int uga, MapLayer *pLayer);
   bool OutputUpdatedWRLists(); // if dynamic water rights are used, might be helpful to see resulting list. if called output to Envision\testPOU.csv and Envision\testPOD.csv.
	bool AddWaterRight(FlowContext *pFlowContext, int idu, int streamLayerComid); // Utility for adding a water right for an IDU to the Point of Diversion and Place of Use lists
	bool ExportDistPodComid(FlowContext *pFlowContext, char units); // will export .csv file of distances between POD and centroid of stream assoceated with COMID
	int  SortWrData(FlowContext *pFlowContext, PtrArray<WaterRight> arrayIn); // Sorts the prior appropriation water right list (under construction)
	int  GetNearestReach(FlowContext *pFlowContext, int idu, CArray<int,int> *reaches, float radius ); // gets the nearest reach within a radius to an IDU from a supplied array of reaches.
	bool IsJunior(WaterRight *incumbentWR, WaterRight *canidateWR); // returns true or false if canidateWR is Junior to incumbentWR
	int  GetWRsInReach( CArray<Reach*, Reach*> *reachArray, CArray<WaterRight*, WaterRight*> *wrArray );
	bool IsUpStream( FlowContext *pFlowContext, WaterRight *incumbenWR, WaterRight *canidateWR ); // returns true or false if canidateWR is up stream from incumbentWR

	// xml variables
	CString  m_podTablePath;					// POD lookup file .csv
	CString  m_pouTablePath;					// POU lookup file .csv
	CString  m_dynamicWRTablePath;		   // dynamic water rights file .csv
	int      m_irrigatedHruLayer;				// index of HRU layer where irrigation water is applied
	int      m_nonIrrigatedHruLayer;			// index of HRU layer where non-irrigation water is applied
	int      m_nDaysWRNotUsedLimit;			// the number of consequtive days a Water Right is not used before it is canceled
	int      m_nYearsWRNotUsedLimit;			// number of consequtive years where Consequtive days threshold was exceeded 
	int      m_nDaysWRConflict1;				// The number of days per week Water Right is in conflict to meet Level 1.
	int      m_nDaysWRConflict2;				// The number of days per week, or consecutive weeks a Water Right is in conflict to meet Level 2.
	int      m_irrDefaultBeginDoy;			// The default irrigation begin Day of Year if not specified in Water Right. Set in .xml file (1 base)
	int      m_irrDefaultEndDoy;				// The default irrigation end Day of Year if not specified in Water Right.  Set in .xml file (1 base)
	int      m_exportDistPodComid;			// If set equal to 1, export .csv file with distances between stream centroid and POD xy coordinates
	float    m_maxDuty;							// For irrigatable IDUs, maximum duty. This value is default if not specifed in IDU layer or POD input data file 
	int      m_maxDutyHalt;						// If 1-yes then halt irrigation to IDU if maximum duty is reached
	float    m_maxRate;							// For irrigatable IDUs, maximum daily rate, (cfs/acre)
	float		m_maxIrrDiversion;				// The maximum irrigation diversion allowed per day mm/day
	CString  m_minFlowColName;					// column name of minimum flow in stream layer   
	CString  m_wrExistsColName;				// column name of  bitwise set attribute in IDU layer when decoded characterizes WR_USE and WR_PERMIT
	CString  m_irrigateColName;				// column name of binary in IDU layer to irrigate or not
	CString  m_maxDutyColName;					// column name of attribute in IDU layer, or POD input file, as specified in .xml file.  If both exist, defaults to value in IDU layer.
	float    m_maxDistanceToReach;			// the maximum distance between pod and reach before a water right is exercised (m)
	bool     m_debug;								// if 1-yes then will invoke helpful debugging stuff
	float    m_fractionDischargeAvail;	   // this value is multiplied against the discharge in a reach to determine water available to water rights
	float    m_dynamicWRRadius;				// the maximum radius from a reach to consider adding a dynamic water right (m)
	int      m_maxDaysShortage;				// the maximum number of shortage days before a Water Right is canceled for the growing season.
	int      m_recursiveDepth;					// in times of shortage, the recursive depth of upstream reaches to possibly suspend junior water rights
	float    m_pctIDUPOUIntersection;      // percentage of an IDU that is intersected by an water right's place of use (%)
	int      m_dynamicWRAppropriationDate; // Dynamic water right appropriation date.  If set to -1, then equals year of run

	WaterAllocation *m_pWaterAllocation;   // pointer to the containing class

protected:
	CMap< Reach*, Reach*, WaterRight*, WaterRight* > m_wrMap; // used to lookup water right from reach	PtrArray< WaterRight > m_podArray;
   PtrArray< WaterRight > m_podArray;

	FlowContext *m_flowContext;

	//containers for input data files
	VDataObj m_pouDb; // Point of Use (POU) data object
	VDataObj m_podDb; // Point of Diversion (POD) data object
	VDataObj m_dynamicWRDb; // Point of Diversion (POD) data object

	// utility functions
	bool  AggregateIDU2HRU(FlowContext *pFlowContext);
   void CalculateWRattributes(MapLayer *pLayer, int pouRecords);
	float GetAvailableSourceFlow(Reach *pReach);
	int   LoadWRDatabase(FlowContext *pFlowContext);
	int   LoadDynamicWRDatabase(FlowContext *pFlowContext);
	int   IDUIndexLookupMap(FlowContext *pFlowContext);

	// water allocation functions
	bool  AllocateSurfaceWR(FlowContext *pFlowContext, WaterRight *pRight, int iduNdx, float pctPou, float areaPou);
   bool  AllocateWellWR(FlowContext *pFlowContext, WaterRight *pRight, int iduNdx, float pctPou, float areaPou);
   bool AllocateSWtoUGA(WaterRight *pRight, MapLayer *pStreamLayer, int uga, float request, float *allocated_water); // quantities in m3/sec H2O
   bool AllocateGWtoUGA(WaterRight *pRight, int uga, float request, float *allocated_water); // quantities in m3/sec H2O
   void FateOfUGA_UrbanWater(FlowContext * pFlowContext);

	// Conflict functions
   bool  LogWeeklySurfaceWaterIrrigationConflicts(FlowContext *pFlowContext, WaterRight *pRight, int iduNdx, float areaPou);
   void LogWeeklySurfaceWaterIrrigationShortagesByIDU(int iduNdx, bool shortageFlag);
   bool  LogWeeklySurfaceWaterMunicipalConflicts(FlowContext *pFlowContext, WaterRight *pRight, int iduNdx, float areaPou);

	// runtime info - these all track allocations during the course of a single invocation of Run()
	/*	a.      Regulatory demand	– instream flow requirements “minimum flow” for BiOP, instream water rights, etc. 
		b.      Allocated water		– rate or quantity of water actually diverted from the stream for urban or irrigation use. 
		c.      Appropriated water – water that is potentially usable based on existing water right maximum rates and duties. 
		d.      Unexercised water	– the difference between the appropriated amount of water (what is potentially usable under the law) and the allocated water (actually being put to use), including when a farmer has decided not to irrigate a given field at all. 
		f.      Available water		– The difference between current stream discharge, and regulatory demand.

	*/
	// SW = Surface Water  
	CArray< float, float > m_iduSWIrrArrayDy;                // Daily Allocated surface water for irrigation (mm/day) 
	CArray< float, float > m_iduSWIrrArrayYr;                // Annual Allocated surface water for irrigation (mm/day) 
	CArray< float, float > m_iduSWIrrAreaArray;              // Area of Surface Water irrigation (m2)
	CArray< float, float > m_iduSWMuniArrayDy;               // m3/sec
	CArray< float, float > m_iduSWMuniArrayYr;               // m3 H2O
	CArray< float, float > m_iduSWUnAllocatedArray;          // if Available stream flow (ASF) > Demand (D), then ASF-D (m3/sec)
	CArray< float, float > m_iduSWUnsatMunDmdYr;             // if Municipal Demand (D) > Available stream flow (ASF), then D-ASF (m3/sec)
	CArray< float, float > m_iduSWUnSatIrrArrayDy;           // if irrigation request (D) > Available stream flow (ASF), then D-ASF (m3/sec)
   CArray< float, float > m_iduSWUnSatMunArrayDy;           // if municipal request (D) > Available stream flow (ASF), then D-ASF (m3/sec)
	CArray< float, float > m_iduSWAppUnSatDemandArray;       // if Demand (D) > Point of Diversion Rate (PODR), then D-PODR (m3/sec)
	CArray< float, float > m_iduSWUnExerIrrArrayDy;          // if Point of Diversion Rate (PODR) > Demand (D), then PODR - D (m3/sec) 
	CArray< float, float > m_iduSWUnExerMunArrayDy;          // if Point of Diversion Rate (PODR) > Demand (D), then PODR - D (m3/sec) 
   CArray< float, float > m_iduAnnualIrrigationDutyArray;   // Annual irrigation duty (acrefeet)
   float                  m_fromOutsideBasinDy;             // daily water brought in from outside the basin (m3/sec)
   float                  m_fromOutsideBasinYr;             // yearly water brought in from outside the basin (m3)
   float                  m_SWIrrAreaYr;                    // yearly area of irrigated IDUs (acres)
	float                  m_SWIrrAreaDy;                    // daily area of irrigated IDUs (acres)
	float                  m_SWMuniWater;							// daily allocated municipal water from stream m3/sec
	float                  m_SWIrrWater;							// daily allocated irrigation water from stream m3/sec
	float                  m_SWIrrWaterMmDy;						// daily allocated irrigation water from stream mm/day
	float                  m_SWUnSatIrr;							// daily unsatisfied irrigation from stream m3/sec
	float						  m_SWUnExIrr;								// daily unexercised irrigation from surface water m3/sec
	float                  m_SWUnExMun;								// daily unexercised municipal from surface water m3/sec
	float                  m_SWUnAllocIrr;							// daily unallocated irrigatin from surface water m3/sec 
	float						  m_regulatoryDemand;               // daily instream water right demand m3/sec
	float                  m_SWunallocatedIrrWater;          // daily unallocated irrigation surface water m3/sec
	float                  m_SWunsatIrrWaterDemand;          // daily unsatisfied ag water demand m3/sec
	float                  m_SWIrrDuty;								// daily surface water irrigation duty m3
	float                  m_GWIrrDuty;								// daily ground water irrigation duty m3
	int                    m_inStreamUseCnt;						// instream water right use count
	int                    m_idusPerPou;							// the number of IDUs per WR POU
	float                  m_remainingIrrRequest;            // as multiple PODs satisfy Irrigation Request in IDU this is what is remaining
	float                  m_SWCorrFactor;							// correction factor for surface water POD rate diversion (unitless)
	float                  m_SWIrrUnSatDemand;               // if available surface water is less than half Irrigation Request m3/sec
	float                  m_SWMunUnSatDemand;               // if available surface water is less than half municipal demand m3/sec
	float						  m_irrigatedSurfaceYr;					// Allocated surface water for irrigation (acre-ft per year)
   float                  m_wastedWaterRateDy;              // 0.0125 cfs - daily allocated irrigation water, summed over irrigated IDUs (m3/sec)
   float                  m_wastedWaterVolYr;               // vol of daily wasted water, accumulated over a one year period, for all irrigated IDUs (acre-ft/yr)
   float                  m_exceededWaterRateDy;            // m3/sec
   float                  m_exceededWaterVolYr;             // acre-ft/yr
   float                  m_daysPerYrMaxRateExceeded;       // annual count of days where max daily withdrawal rate exceeded, area weighted (days)
	FDataObj  m_timeSeriesSWIrrSummaries;							// for plotting at the end of run.
	FDataObj  m_timeSeriesSWIrrAreaSummaries;						// for plotting at the end of run. mm/day
	FDataObj  m_timeSeriesSWMuniSummaries;							// for plotting at the end of run.
	
   // UW = Urban Water, i.e. water for residential, commercial, or industrial use
   float m_ugaLocalUWdemandArray[MAX_UGA_NDX+1];              // m3/sec H2O
   float m_ugaUWallocatedDay[MAX_UGA_NDX+1];                  // m3/sec H2O
   float m_ugaUWallocatedYr[MAX_UGA_NDX+1];                  // m3 H2O
   int m_ugaDischargeComID[MAX_UGA_NDX + 1];                // reach ComID of UGA discharge point
   float m_ugaUWshortageYr[MAX_UGA_NDX + 1];                // m3 H2O
   float m_ugaUWfromSW[MAX_UGA_NDX + 1];                    // m3 H2O
   float m_ugaUWfromGW[MAX_UGA_NDX + 1];                    // m3 H2O

   // GW = Ground Water  
   CArray< float, float > m_iduGWIrrArrayDy;						// Daily Allocated ground water for irrigation (mm/day) 
	CArray< float, float > m_iduGWIrrArrayYr;						// Annual Allocated ground water for irrigation (mm/day)
	CArray< float, float > m_iduGWIrrAreaArray;					// Area of Surface Water irrigation (m2)
	CArray< float, float > m_iduGWMuniArrayDy;					// m3/sec
	CArray< float, float > m_iduGWMuniArrayYr;					// m3 H2O
   CArray< float, float > m_iduGWUnSatMunArrayDy;           // Daily, if municipal request (D) > Available stream flow (ASF), then D-ASF (m3/sec)
	CArray< float, float > m_iduGWUnAllocatedArray;				// if Available well head (WH) > Demand (D), then Wh-D (m3/sec)
   CArray< float, float > m_iduGWUnExerIrrArrayDy;          // if Point of Diversion Rate (PODR) > Demand (D), then PODR - D (m3/sec) 
	CArray< float, float > m_iduGWUnExerMunArrayDy;          // if Point of Diversion Rate (PODR) > Demand (D), then PODR - D (m3/sec) 
	CArray< float, float > m_iduGWUnsatIrrReqYr;             // if Irrigation request (D) > Available stream flow (ASF), then D-ASF (m3/sec)
	CArray< float, float > m_iduGWUnsatMunDmdYr;             // if Municipal Demand (D) > Available stream flow (ASF), then D-ASF (m3/sec)
	CArray< float, float > m_iduGWAppUnSatDemandArray;			// if Demand (D) > Point of Diversion Rate (PODR), then D-PODR (m3/sec)
	float                  m_GWIrrAreaYr;							// yearly area of well irrigated IDUs (acres)
   float                  m_GWMuniWater;							// daily allocated municipal water from well m3/sec
   float                  m_GWnoWR_Yr_m3;							// groundwater pumped without a water right for rural domestic use, m3 per year
   float                  m_GWIrrWater;							// daily allocated irrigation water from well m3/sec
	float                  m_GWIrrWaterMmDy;						// daily allocated irrigation water from well mm/day
	float                  m_GWIrrAreaDy;                    // daily area of irrigated IDUs (acres)
	float                  m_GWUnSatIrr;							// daily unsatisfied irrigation from ground water m3/sec
	float						  m_GWUnExIrr;								// daily unexercised irrigation from ground water m3/sec
   float                  m_GWUnAllocIrr;							// daily unallocated irrigatin from ground water m3/sec
	float                  m_GWCorrFactor;							// correction factor for ground water POD rate diversion (unitless)
	float                  m_GWIrrUnSatDemand;               // if available ground water is less than half irrigation request m3/sec
	float                  m_GWMunUnSatDemand;               // if available ground water is less than half municipal demand m3/sec
	float						  m_irrigatedGroundYr;					// Allocated ground water for irrigation (acre-ft per year)
	float						  m_unSatisfiedIrrGroundYr;			// Unsatisified ground irrigation Water (acre-ft per year)
	FDataObj  m_timeSeriesGWIrrSummaries;							// for plotting at the end of run.
	FDataObj  m_timeSeriesGWIrrAreaSummaries;						// for plotting at the end of run. mm/day
	FDataObj  m_timeSeriesGWMuniSummaries;							// for plotting at the end of run.

	// Irrigation Request 
	float						  m_irrWaterRequestYr;				// Total Irrigation Request (acre-ft per year)
	float						  m_irrigableIrrWaterRequestYr;	// Irrigable Irrigation Request (acre-ft per year)
	CArray< float, float > m_iduIrrWaterRequestYr;			// Annual Irrigation Request (acre-ft per year)
	CArray< float, float > m_iduActualIrrRequestDy;			// Actual daily Irrigation Request (mm per day)
	int						  m_colIrrRequestYr;					// Annual Potential Irrigation Request column in IDU layer (acre-ft per year)
	int						  m_colIrrRequestDy;					// Daily Potential Irrigation Request column in IDU layer ( mm per day )
	int                    m_colActualIrrRequestDy;       // Daily Actual irrigation request ( mm per day )
	float                  m_IrrWaterRequestDy;           // Total daily irrigation request (mm per day)
	CArray< int, int >     m_iduIsIrrOutSeasonDy;			// A daily binary indicationg if irrigation of IDU is outside WR seasonality
	CArray< float, float > m_iduLocalIrrRequestArray;	   // a local copy of m_iduIrrRequestArray (mm/day)

	// for Summaries total irrigation
	FDataObj						m_timeSeriesUnallUnsatIrrSummaries;	 // for plotting at the end of run.
	FDataObj						m_dailyMetrics;							 // for plotting at the end of Flow's time step.
	FDataObj						m_annualMetrics;							 // for plotting at the end of Envision time step.
	FDataObj						m_dailyMetricsDebug;						 // for plotting at the end of Flow's time step.
   FDataObj						m_annualMetricsDebug;					 // for plotting at the end of Envision time step.
   FDataObj						m_QuickCheckMetrics;					    // annual metrics for rapid assessment of simulation runs
   double                  m_basin_discharge_accumulator_m3;    // accumulates daily m_mvCurrentStreamFlow for the basin discharge reach (for WRB, COMID 23735691)
   float							m_irrigatedWaterYr;						 // Allocated irrigation Water (acre-ft per year)
	float							m_unSatisfiedIrrigationYr;				 // Unsatisified Irrigation Water (acre-ft per year)
	float							m_unallocatedMuniWater;					 // daily unallocated municipal water m3/sec
	CArray< float, float >  m_maxTotDailyIrr;							 // Maximum total daily applied irrigation water (acre-ft)
	int							m_colMaxTotDailyIrr;						 // column in IDU layer for Maximum total daily applied irrigation water (acre-ft )
	CArray< float, float >  m_aveTotDailyIrr;							 // Average total daily applied irrigation water (acre-ft )
	CArray< int, int     >  m_nIrrPODsPerIDUYr;						 // number of Points of Diversions irrigating an idu, accumulated over the course of a yr
	int							m_colaveTotDailyIrr;						 // column in IDU layer for average total daily applied irrigation water (acre-ft )
   CArray< float, float >  m_iduUnsatIrrReqst_Yr;					 // cumulative unsatisfied irrigation demand for the year (acre-ft)
   CArray< float, float >  m_iduWastedIrr_Yr;					    // 0.0125 cfs - annual allocated irrigation water, for each irrigated IDUs (acre-ft/yr)
	CArray< float, float >  m_iduExceededIrr_Yr;					    // (acre-ft)
	CArray< float, float >  m_iduWastedIrr_Dy;					    // 0.0125 cfs - annual allocated irrigation water, for each irrigated IDUs (acre-ft/yr)
	CArray< float, float >  m_iduExceededIrr_Dy;					    // (acre-ft)

	//Used to lookup Percent POU for an IDUINDEX, when given PodID, and IDU_INDEX
	map<PouKeyClass, std::vector<int>, PouKeyClassComp> m_pouInputMap;  //built from POU data input file
	PouKeyClass m_pouInsertKey;
	PouKeyClass m_pouLookupKey;

	// Column names
	INT_PTR m_typesInUse;               // bit flags based on types
	int   m_colAREA;                 // IDU column for area
   int   m_colUGB;
   int   m_colPOP;
   int   m_colH2ORESIDNT;
   int   m_colH2OINDCOMM;
   int   m_colGAL_CAP_DY;
   int   m_colIDUIndex;                // column in IDU layer for IDU_INDEX
   int   m_colWRShortG;                // WR irrigation unsatisfied irrigaton request in IDU layer.  1=level, 2=level , 0=request met
	int   m_colWRShutOff;               // WR irrigation unsatisfied irrigaton request in IDU layer.  1=level, 2=level , 0=request met
	int   m_colWRJuniorAction;          // some action is begin taken on a junion water right, depends on type of action
   int   m_colWRShutOffMun;            // WR municipal unsatisfied demand in IDU layer.  1=level, 2=level , 0=demand met
	int   m_colAllocatedIrrigation;     // Water Allocated for irrigation use (mm per year)
	int   m_colAllocatedIrrigationAf;   // Water Allocated for irrigation use (acre-ft per year)
	int   m_colSWAllocatedIrrigationAf; // Surface Water Allocated for irrigation use (acre-ft per year)
	int   m_colIRGWAF_Y;                // Ground Water Allocated for irrigation use (acre-ft per year)
	int	m_colUnsatIrrigationAf;			// Unsatisfied irrigation request (acre-ft per year) 
	int	m_colSWUnsatIrrigationAf;		// Unsatisfied surface irrigation request (acre-ft per year)
	int	m_colGWUnsatIrrigationAf;		// Unsatisfied ground irrigation request (acre-ft per year)
	int   m_colDailyAllocatedIrrigation;// Water Allocated for irrigation use (mm per day)
	int   m_colDemandFraction;			   // ratio of irrigation allocated / requested irrigation
	int   m_colUnAllocatedWR;           // Un-allocated water right
	int   m_colWaterDeficit;            // water deficit from ET calculations
	int   m_colRealUnSatisfiedDemand;   // Unsatisfied demand (mm per day)
	int   m_colUnSatIrrRequest;         // Unsatisfied irrigation request (mm per day)
	int   m_colVirtUnSatisfiedDemand;   // virtual UnSatisfied Demand
	int   m_colUnExercisedWR;           // Un-exercised WR
	int   m_colIrrApplied;              // irrigation applied, mm/day
	int   m_colUDMAND_DY;			      // Daily urban water demand m3/sec
   int   m_colPlantDate;               // Crop planting date in IDU layer
	int   m_colHarvDate;						// Crop harvest date in IDU layer
	int   m_colMunPodRate;              // total POD rate for all PODs in IDU with municipal water right m3/sec
   int   m_colIrrig_yr;                // total irrigation per year (mm)
	int   m_colLulc_A;						// Land Use/Land Cover (coarse). used for finding Agriculture IDUs
	int   m_colLulc_B;						// Land Use/Land Cover (medium). used for finding crop type
	int   m_colSWIrrDy;						// Daily surface water applied to IDU (mm/day)
	int   m_colGWIrrDy;						// Daily ground water applied to IDU (mm/day)
	int   m_colMaxDutyLayer;            // For irrigatable IDUs, if this attribute present in IDU layer, is  maximum duty for IDU
	int   m_colWRConflictYr;            // Attribute in stream layer that types a reach as being in "conflict" (not satisfying WR) 0- no conflict, 1-available < demand, 2-available < demand/2
	int   m_colWRConflictDy;            // Attribute in stream layer that types a reach as being in "conflict" (not satisfying WR) 0- no conflict, 1-level one, 2-level two
	int   m_colMUNIALLO_D;              // column in IDU layer for daily municipal water allocations (m3/sec)
	int   m_colSWAlloMunicipalDay;      // column in IDU layer for daily municipal surface water allocations (m3/sec)
   int   m_colSWMUNALL_Y;              // column in IDU layer for annual municipal SURFACE water allocations (m3 H2O)
   int   m_colGWAlloMunicipalDay;      // column in IDU layer for daily municipal ground water allocations (m3/sec)
   int   m_colGWMUNALL_Y;              // column in IDU layer for annual municipal ground water allocations (m3 H2O)
   int   m_colIrrUseOrCancel;          // column in IDU layer: 1=yes Water right canceled, 0=water right active. Irrigation
	int   m_colMunUseOrCancel;          // column in IDU layer: 1=yes Water right canceled, 0=water right active. Municipal
	int   m_colSWUnexIrr;					// when appropriated irrigation surface water POD rate is greater than request, POD rate - demand (m3/sec)
	int   m_colGWUnexIrr;					// when appropriated irrigation ground water POD rate is greater than request, POD rate - demand (m3/sec)
	int   m_colSWUnexMun;					// when appropriated municipal surface water POD rate is greater than request, POD rate - demand (m3/sec)
	int   m_colSWUnSatMunDemandDy;	   // daily surface water unsatisfied municipal demand (m3/sec)
	int   m_colGWUnSatMunDemandDy;	   // daily ground water unsatisfied municipal demand (m3/sec)
	int   m_colSWUnSatIrrRequestDy;     // daily surface water unsatisfied irrigation request (m3/sec)
	int   m_colGWUnSatIrrRequestDy;		// daily ground water unsatisfied irrigation request (m3/sec)
	int   m_colWastedIrrDy;					// 0.0125 cfs - daily allocated irrigation water, summed over irrigated IDUs (m3/sec)
	int   m_colWastedIrrYr;             // 0.0125 cfs - annual allocated irrigation water, summed over irrigated IDUs (m3/sec)
	int   m_colExcessIrrDy;					// (m3/sec)
   int   m_colExcessIrrYr;					// (acre-ft)
	int   m_colIrrExceedMaxDutyYr;      // 1- yes, 0-no IDU exceeded maximum annual irrigation duty 0-no
	int   m_colDSReservoir;             // 1- yes, 0-no reach is downstream from reservoir
   int m_colWREXISTS; // col for bit-mapped water rights for this IDU
   int m_colWR_MUNI; // col for 1=> municipal water right
   int m_colWR_INSTRM; // col for 1=> instream water right
   int m_colWR_IRRIG_S; // col for 1=> surface water irrigation right
   int m_colWR_IRRIG_G; // col for 1=> groundwater irrigation right
   int m_colSTRM_ORDER; // col for order of stream reach associated with this IDU
   int m_colIDUH2O_EST;
   int m_colIRR_STATE;
   int m_colPRECIP_YR;
   int m_colET_YR;
   int m_colHRU_ID;

	map<IDUIndexKeyClass, std::vector<int>, IDUIndexClassComp> m_IDUIndexMap;  //used for idu index, finding IDUs associated with POUs
	IDUIndexKeyClass m_iduIndexInsertKey;
	IDUIndexKeyClass m_iduIndexLookupKey;

	// Point of Diversion (POD) input data file columns http://www.oregon.gov/owrd/pages/wr/wrisuse.aspx
	int   m_colWRID;          // WR id
	int   m_colDemand;        // desired demand (m3/sec) (in)
	int   m_colWaterUse;      // actual use (out)
	int   m_colXcoord;        // WR UTM zone x coordinate (m)
	int   m_colYcoord;        // WR UTM zone y coordinate (m)
	int   m_colPodID;         // WR column number POD ID input data file
	int   m_colPDPouID;       // WR column number POU ID in POD input data file
	int   m_colPermitCode;    // WR Permit Code http://www.oregon.gov/owrd/pages/wr/wrisuse.aspx
	int   m_colPodRate;       // WR point of diversion max rate (m3/sec)
	int   m_colUseCode;       // WR Use Code http://www.oregon.gov/owrd/pages/wr/wrisuse.aspx
	int   m_colPriorDoy;      // WR priority date day of year
	int   m_colPriorYr;       // WR priority date year
	int   m_colBeginDoy;      // WR seasonal begin day of year
	int   m_colEndDoy;        // WR seasonal end day year
	int   m_colPouRate;       // WR point of use max rate (m3/sec)
	int   m_colReachComid;    // WR Reach comid, column in POD lookup file relates to COMID in reach layer
	int   m_colStrLayComid;   // column ID of COMID in stream layer
	int   m_coliDULayComid;   // column ID of COMID in IDU layer
	int   m_colPodUseRate;    // column for pod use rate m3/sec
	int   m_colDistPodReach;  // column for pod to reach distance (m)
	int   m_colMaxDutyFile;   // column for maximum duty in POD input data file (acre-ft / acre);   

	// Point of Use (POU) input data file columns
	int   m_colPUPouID;       // WR column number POU ID in POU input data file
	int   m_colPouIDU_INDEX;  // IDU_INDEX column in POU data input file
	int   m_colPouPct;        // The areal percentage of the POU, for a SnapID, that over laps the IDU/IDU_INDEX
	int   m_colPouArea;       // Area (study area units)
	int   m_colPouDBNdx;      // a zero based index for the POU input data file itself
	int m_colPouUSECODE;
	int m_colPouPERMITCODE;

   // Dyanmic Water right input data file columns and member variables
	int  m_colDynamTimeStep;   // column for time step specified in dynamic WR input file, zero based
	int  m_colDynamPermitCode; // column for permit code specified in dynamic WR input file
	int  m_colDynamUseCode;	   // column for use code specified in dynamic WR input file
	int  m_colDynamIsLease;    // column if water right is a lease specified in dynamic WR input file
	int  m_colWRZone;				// column name of WRZONE in IDU layer
	bool m_DynamicWRs;          // a dynamic water rights input file has loaded
	CStringArray m_zoneLabels; // container for zone labels from input file, should also be present in IDU layer
	
	// counters
	int   m_weekOfYear;       // the week of the year (zero based 0-51)
	int   m_weekInterval;     // week interval. Zero based (0,6,13,19..). Assumes no leap years (365)
	int   m_envTimeStep;      // 0 based year of run
	
	// Irrigate members
	int   m_irrigateDecision;   // binary decision to irrigate, 0=false no irrigate, 1=true yes irrigate
	int   m_colIrrigate;        // column number if present in IDU layer, name of column specified in .xml 
	bool  m_demandIrrigateFlag; // for each flow time step if right exist and demand is zero, flag is false. default true.
	bool  m_wrIrrigateFlag;     // If economic to irrigate at envision time step is 1, then true. elseif  0 then false.

	// Stream layer members
	float     m_minFlow;                                     // minimum reach flow in IDU layer, zero if column does not exist
	int       m_colMinFlow;                                  // column number if present in IDU layer, name of column specified in .xml
   int       m_colReachLength;                              // column number of the attrigute "LENGTH" in the stream layer
	int       m_colNInConflict;									   // column number in stream layer for number of times reach in conflict
	float     m_basinReachLength;										// total lenght of stream reaches in the study area
	float     m_irrLenWtReachConflictYr;					      // if reach is in conflict, annual total of reach lenght/ basin reach length
	MapLayer *m_pStreamLayer;                                // study area stream layer
   CArray< int, int > m_reachDaysInConflict;						// the number of days per year a reach is in "conflic" 
	CArray< int, int > m_dailyConflictCnt;						   // the number of times per day a reach is in "conflic" 
	CArray< int, int > m_reachSrWRyear; // the priority year of most junior WR which has not been shut off in this reach DRC

	// Surface Water Conflict metrics
	int     m_nSWLevelOneIrrWRSO;                            // number of irrigation IDUs with level one conflict, water right shut off (SO)
	int     m_nSWLevelTwoIrrWRSO;                            // number of irrigation IDUs with level two conflict, water right shut off (SO)
	int     m_nSWLevelOneMunWRSO;                            // number of municipal IDUs with level one conflict, water right shut off (SO)
	int     m_nSWLevelTwoMunWRSO;                            // number of muncipal IDUs with level two conflict, water right shut off (SO)
	float   m_pouSWAreaLevelOneIrrWRSO;                      // POU irrigation area of level one conflict, water right shut off (SO)
	float   m_pouSWAreaLevelTwoIrrWRSO;                      // POU irrigation area of level two conflict, water right shut off (SO)
	float   m_iduSWAreaLevelOneIrrWRSO;                      // IDU irrigation area of level one conflict, water right shut off (SO)
	float   m_iduSWAreaLevelTwoIrrWRSO;                      // IDU irrigation area of level two conflict, water right shut off (SO)
	float   m_areaDutyExceeds;											// area of irrigated land exceeding duty threshold
	float   m_demandDutyExceeds;									   // acre-ft of demand not satisfied when irrigated land exceeding duty threshold
	float   m_demandOutsideBegEndDates;							   // acre-ft of demand requested outside of regulatory begin/end diversion dates
	float   m_unSatInstreamDemand;									// if the available source flow is less than regulatory in-stream demand in a reach, accumlate the difference (m3/sec) 
   CArray< int, int > m_iduConsecutiveShortages; // DRC number of days in a row that this IDU has experienced a surface water irrigation shortage
   CArray< int, int > m_iduSWIrrWRSOIndex;                  // this tracks to which irrigation IDU's surface water right was shut off (SO)
	CArray< int, int > m_iduSWMunWRSOIndex;                  // this tracks to which municipal IDU's surface water right was shut off (SO)
	CArray< float, float > m_iduSWLevelOneIrrWRSOAreaArray;  // this tracks the irrigation IDU area associated with level 1 surface Water Right Shut Off (SO)
   CArray< float, float > m_iduSWLevelOneMunWRSOAreaArray;  // this tracks the municipal IDU area associated with level 1 surface Water Right Shut Off (SO)
	CArray< float, float > m_pouSWLevelOneIrrWRSOAreaArray;  // this tracks the irrigation POU area intersecting IDU associated with level 1 surface Water Right Shut Off (SO)
	CArray< float, float > m_pouSWLevelOneMunWRSOAreaArray;  // this tracks the municipal POU area intersecting IDU associated with level 1 surface Water Right Shut Off (SO)
	CArray< float, float > m_iduSWLevelTwoIrrWRSOAreaArray;  // this tracks the irrigation IDU area associated with  level 2 surface Water Right Shut Off (SO)
	CArray< float, float > m_iduSWLevelTwoMunWRSOAreaArray;  // this tracks the municipal IDU area associated with  level 2 surface Water Right Shut Off (SO)
	CArray< float, float > m_pouSWLevelTwoIrrWRSOAreaArray;  // this tracks the irrigation POU area intersecting IDU associated with level 2 surface Water Right Shut Off (SO)
	CArray< float, float > m_pouSWLevelTwoMunWRSOAreaArray;  // this tracks the municipal POU area intersecting IDU associated with level 2 surface Water Right Shut Off (SO)
	CArray< int, int > m_iduSWIrrWRSOWeek;                   // this tracks the week (expressed as day of year) an irrigation IDU is associated with surface Water Right Shut Off (SO)
	CArray< int, int > m_iduSWMunWRSOWeek;                   // this tracks the week (expressed as day of year) an municipal IDU is associated with surface Water Right Shut Off (SO)
	CArray< int, int > m_iduSWIrrWRSOLastWeek;               // previous weeks irrigation m_iduSWIrrWRSOWeek
	CArray< int, int > m_iduSWMunWRSOLastWeek;               // previous weeks municipal m_iduSWIrrWRSOWeek
	CArray< int, int > m_iduExceedDutyLog;							// binary if idu exceeds max duty
	int m_regulationType;							               // in times of shortage, index of type of regulation; 0 - no regulation, > 0 see manual for regulation types

   CArray<int, int> m_iduWR; // DRC the water right ID of the water right most recently exercised for this IDU

	// Miscellaneous
	int	  m_mostJuniorWR;												// the year of the most junior water right
	int     m_mostSeniorWR;												// the year of the most senior water right
	int     m_dynamicWRType;									      // input variable index
	GeoSpatialDataObj *m_myGeo = new GeoSpatialDataObj;

	// Debug metrics
	float m_dyGTmaxPodArea21;											// vineyards and tree farms area > maxPOD(acres)"); //21
	float m_dyGTmaxPodArea22;											// Grass seed area > maxPOD(acres)"); //22
	float m_dyGTmaxPodArea23;											// Pasture area > maxPOD(acres)"); //23
	float m_dyGTmaxPodArea24;											// Wheat area > maxPOD(acres)"); //24
	float m_dyGTmaxPodArea25;											// Fallow area > maxPOD(acres)"); //25
	float m_dyGTmaxPodArea26;											// Corn area > maxPOD(acres)"); //26
	float m_dyGTmaxPodArea27;											// Clover area > maxPOD(acres)"); //27
	float m_dyGTmaxPodArea28;											// Hay area > maxPOD(acres)"); //28
	float m_dyGTmaxPodArea29;											// Other crops area > maxPOD(acres)"); //29	
	
	float m_anGTmaxDutyArea21;											// vineyards and tree farms area > maxDuty(acres)"); //21
	float m_anGTmaxDutyArea22;											// Grass seed area > maxDuty(acres)"); //22
	float m_anGTmaxDutyArea23;											// Pasture area > maxDuty(acres)"); //23
	float m_anGTmaxDutyArea24;											// Wheat area > maxDuty(acres)"); //24
	float m_anGTmaxDutyArea25;											// Fallow area > maxDuty(acres)"); //25
	float m_anGTmaxDutyArea26;											// Corn area > maxDuty(acres)"); //26
	float m_anGTmaxDutyArea27;											// Clover area > maxDuty(acres)"); //27
	float m_anGTmaxDutyArea28;											// Hay area > maxDuty(acres)"); //28
	float m_anGTmaxDutyArea29;											// Other crops area > maxDuty(acres)"); //29	
	float m_IrrFromAllocationArrayDy;								// check to see if array used to flux is same as values written to map ( mm/day ) action item to eventually remove
	float m_IrrFromAllocationArrayDyBeginStep;					// check to see if array used to flux is same as values written to map ( mm/day ) action item to eventually remove
	int   m_pastureIDUGTmaxDuty;                             // a pasture IDU_INDEX where max duty is exceeded
	float m_pastureIDUGTmaxDutyArea;                         // a pasture area of IDU_INDEX where max duty is exceeded (m2)
};


