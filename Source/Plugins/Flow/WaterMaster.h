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

enum WR_PODSTATUS {
	WRPS_CANCELED = 1, WRPS_EXPIRED = 2, WRPS_MISFILED = 3, WRPS_NONCANCELED = 4,
	WRPS_REJECTED = 5, WRPS_SUSPENDED = 6, WRPS_SUPERCEDED = 7, WRPS_UNKNOWN = 8,
	WRPS_UNUSED = 9, WRPS_WITHDRAWN = 10
};

typedef struct {
	int iduIndex;     // WR ID or SnapID in POU input data file

} IDUIndexKeyClass;


typedef struct {
	int pouID;     // WR ID or SnapID in POU input data file

} PouKeyClass;

typedef struct {
	int       priorYr;
	int       priorDoy;
	int       beginDoy;
	int       endDoy;
	int       podStatus;
	int       appCode;
	WR_USE    useCode;
	WR_PERMIT permitCode;
	int       supp;
	float     podRate;
	int       podID;
	int       pouID;
	double    xCoord;
	double    yCoord;
} PodSortStruct;

struct podSortFunc {
	bool operator() (const PodSortStruct& lhs, const PodSortStruct& rhs) const
	{
		if (lhs.priorYr == rhs.priorYr)
		{
			if (lhs.priorDoy == rhs.priorDoy)
			{
				if (lhs.beginDoy == rhs.beginDoy)
				{
					if (lhs.endDoy == rhs.endDoy)
					{
						if (lhs.podStatus == rhs.podStatus)
						{
							if (lhs.appCode == rhs.appCode)
							{
								if (lhs.useCode == rhs.useCode)
								{
									if (lhs.permitCode == rhs.permitCode)
									{
										if (lhs.supp == rhs.supp)
										{
											if (lhs.podRate == rhs.podRate)
											{
												if (lhs.podID == rhs.podID)
												{
													if (lhs.pouID == rhs.pouID)
													{
														if (lhs.xCoord == rhs.xCoord)
														{
															return lhs.yCoord < rhs.yCoord;
														}
														else
														{
															return lhs.xCoord < rhs.xCoord;
														}
													}
													else
													{
														return lhs.pouID < rhs.pouID;
													}
												}
												else
												{
													return lhs.podID < rhs.podID;
												}
											}
											else
											{
												return lhs.podRate < rhs.podRate;
											}
										}
										else
										{
											return lhs.supp < rhs.supp;
										}
									}
									else
									{
										return lhs.permitCode < rhs.permitCode;
									}
								}
								else
								{
									return lhs.useCode < rhs.useCode;
								}
							}
							else
							{
								return lhs.appCode < rhs.appCode;
							}
						}
						else
						{
							return lhs.podStatus < rhs.podStatus;
						}
					}
					else
					{
						return lhs.endDoy < rhs.endDoy;
					}
				}
				else
				{
					return lhs.beginDoy < rhs.beginDoy;
				}
			}
			else
			{
				return lhs.priorDoy < rhs.priorDoy;
			}
		}
		else
		{
			return lhs.priorYr < rhs.priorYr;
		}
	}
};

// comparison structure for looking up POU index in POU input file/map, when given pouID
struct IDUIndexClassComp {
	bool operator() (const IDUIndexKeyClass& lhs, const IDUIndexKeyClass& rhs) const
	{
		return lhs.iduIndex < rhs.iduIndex;
	}
};

// comparison structure for looking up POU index in POU input file/map, when given pouID
struct PouKeyClassComp {
	bool operator() (const PouKeyClass& lhs, const PouKeyClass& rhs) const
	{
		return lhs.pouID < rhs.pouID;
	}
};

class WaterRight
{
public:
	WaterRight(void) : m_idu(-1), m_xCoord(-1.), m_yCoord(-1.), m_podID(-1), m_pouID(-1)
		, m_appCode(-1), m_permitCode(WRP_UNKNOWN), m_podRate(0.), m_useCode(WRU_UNKNOWN)
		, m_supp(-1), m_priorDoy(-1), m_priorYr(-1), m_beginDoy(-1)
		, m_endDoy(-1), m_pouRate(0.), m_podStatus(WRPS_UNKNOWN)
		, m_pctFlow(0), m_pReach(NULL), m_reachID(-1), /*m_pGW( NULL ),*/ m_inUse(true)
		, m_reachComid(-1), m_consecDaysNotUsed(0), m_inConflict(false)
		, m_lastDOYNotUsed(-1), m_nDaysPerYearSO(0), m_nDaysPerWeekSO(0), m_previousWeekConflict(false)
		, m_nPODSperWR(1), m_wrID(0), m_consecYearsNotUsed(0), m_priorYearConflict(0), m_dutyShutOffThreshold(2.5f)
		, m_suspendForYear(false), m_pouArea(0.0f), m_wrAnnualDuty(0.0f), m_surfaceHruLayer(0), m_use(WRU_UNKNOWN)
		, m_podUseRate(0.0f), m_distanceToReach(0.0f), m_maxDutyPOD(1000000.0f), m_stepShortageFlag(false), m_stepsSuspended(0)
		, m_stepRequest(0.0f), m_lastDOYShortage(0)
	{ }


	int m_idu;                  // which IDU is this associated with

	WR_USE m_use;

	//POD point of diversion variables
	int          m_wrID;             // Water right id
	double       m_xCoord;           // UTM zone x coordinate (m)
	double       m_yCoord;           // UTM zone y coordinate (m)
	int          m_podID;            // Water wright ID or SnapID in POD input data file
	int          m_pouID;            // Water wright ID or SnapID in POD input data file
	int          m_appCode;          // WR Application code http://www.oregon.gov/owrd/pages/wr/wrisuse.aspx
	WR_PERMIT    m_permitCode;       // WR Permit Code http://www.oregon.gov/owrd/pages/wr/wrisuse.aspx
	float        m_podRate;          // WR point of diversion rate (m3/sec)
	float        m_podUseRate;       // WR point of use rate (m3/sec)
	WR_USE       m_useCode;          // Use Code http://www.oregon.gov/owrd/pages/wr/wrisuse.aspx
	int          m_supp;             // supplemental code 0-Primary 1-Supplemental
	int          m_priorDoy;         // WR priority date day of year
	int          m_priorYr;          // WR priority date year
	int          m_beginDoy;         // WR seasonal begin day of year
	int          m_endDoy;           // WR season end day year
	float        m_pouRate;          // WR point of use max rate (m3/sec)
	float        m_pouArea;          // WR point of use area (m2)
	WR_PODSTATUS m_podStatus;        // WR point of diversion status code
	int          m_reachComid;       // WR Reach comid, relates to COMID in reach layer
	float        m_wrAnnualDuty;     // WR annual duty, used to compare to annual duty threshold
	float        m_distanceToReach;  // This is the distance from a POD to the closest vertex in a reach (m)
	float        m_maxDutyPOD;			// Max duty associated with POD (acre-ft / acre)
	float        m_stepRequest;      // the amount requested for current step (m3/sec)

	//POU point of use variables
	float        m_pctFlow;          // portion of the flow associated with this IDU

	//Water Right conflict variables
	int          m_consecDaysNotUsed;      // number of consequtive days WR is not used
	int          m_consecYearsNotUsed;     // number of consequtive years where Consequtive days threshold was exceeded
	bool         m_inConflict;             // if WR is in conflict. Not used, can be revoked
	int          m_priorYearConflict;      // prior year that WR was in Conflict
	int          m_lastDOYNotUsed;         // the last day of year WR not used
	int          m_nDaysPerYearSO;         // number of days per year water right is shut off
	int          m_nDaysPerWeekSO;         // number of day per week water right is shut off
	bool         m_previousWeekConflict;   // previous week was in conflict
	float        m_dutyShutOffThreshold;   // each water right will have a maximum duty it is allowed (default is 2.5 acre/feet/acre)
	bool			 m_suspendForYear;		   // if true water right is suspended for the rest of the year
	bool			 m_stepShortageFlag;			// within a growing season, if a senior water right experiences shortage, then keep track when this WR juniors are suspended from daily diversions  
	int			 m_stepsSuspended;			// number of consecutive steps a water right is in conflict
	int          m_lastDOYShortage;			// day of year water right is in shortage

	//economic variables
	//bool         m_wrIrrigateFlag;   //if economics restrict irrigating

	//demand related variables
	//bool         m_demandIrrigateFlag; //if no demand no irrigating 

	Reach        *m_pReach;          // for surface water rights only
	int          m_reachID;

	//???Groundwater  *m_pGW;              // for groundwater rights only

	// runtime info;
	bool         m_inUse;
	int          m_surfaceHruLayer;  // index of HRU layer where water is applied
	int          m_nPODSperWR;       // The number of Points of Diversion per Water Right
	
	bool IsSurface() { return true; }  //temporary
};

class WaterMaster
{
public:
	WaterMaster(FlowModel *pFlowModel, WaterAllocation *pWaterAllocation);
	~WaterMaster(void);

	// Timing methods
	bool Init(FlowContext *pFlowContext);
	bool InitRun(FlowContext *pFlowContext);
	bool StartStep(FlowContext *pFlowContext);
	bool Step(FlowContext *pFlowContext);
	bool EndStep(FlowContext *pFlowContext);
	bool EndYear(FlowContext *pFlowContext);
	bool EndRun(FlowContext *pFlowContext);
	bool StartYear(FlowContext *pFlowContext);

   bool LoadXml( WaterAllocation*, TiXmlElement*, LPCTSTR filename );

	// Method for selecting dynamically added water rights
	bool DynamicWaterRights( FlowContext *pFlowContext, int scenarioValue, int stepInterval ); // based on a Scenario, selects a method for adding water rights to the IDU layer

	// Methods for dynamically adding water rights
   bool ExtremeResWaterRights(FlowContext *pFlowContext, float radius ); // Adds a new irrigation water right to IDUs within a radius of reaches below a reservoir(s).	
	bool ZoneTargetResWaterRights(FlowContext *pFlowContext, int currentStep, int stepInterval, float radius); //Adds a new irrigation water right for reaches below reservoirs based on zones, and targets for the zones.

	// functions for times of shortage
	int  ApplyRegulation(FlowContext *pFlowContext, int regulationType, WaterRight *pWaterRight, int depth, float deficit);
	int  SuspendJuniorWaterRights(FlowContext *pFlowContext, WaterRight *pWaterRight, int depth, float deficit);
	int  NoteJuniorWaterRights(FlowContext *pFlowContext, WaterRight *pWaterRight, int depth, float deficit);
   int  GetJuniorWaterRights(FlowContext *pFlowContext, Reach *pReach, WaterRight *pWaterRight, int depth, CArray<WaterRight*, WaterRight*> *wrArray);

	// Utility functions
	bool OutputUpdatedWRLists(); // if dynamic water rights are used, might be helpful to see resulting list. if called output to Envision\testPOU.csv and Envision\testPOD.csv.
	bool AddWaterRight(FlowContext *pFlowContext, int idu, int streamLayerComid); // Utility for adding a water right for an IDU to the Point of Diversion and Place of Use lists
	bool ExportDistPodComid(FlowContext *pFlowContext, char units); // will export .csv file of distances between POD and centroid of stream assoceated with COMID
	int  SortWrData(FlowContext *pFlowContext, PtrArray<WaterRight> arrayIn); // Sorts the prior appropriation water right list (under construction)
	int  GetNearestReach(FlowContext *pFlowContext, int idu, CArray<int,int> *reaches, float radius ); // gets the nearest reach within a radius to an IDU from a supplied array of reaches.
	bool IsJunior(WaterRight *incumbentWR, WaterRight *canidateWR); // returns true or false if canidateWR is Junior to incumbentWR
	int  GetWRsInReach( CArray<Reach*, Reach*> *reachArray, CArray<WaterRight*, WaterRight*> *wrArray );
	bool IsUpStream( FlowContext *pFlowContext, WaterRight *incumbenWR, WaterRight *canidateWR ); // returns true or false if canidateWR is up stream from incumbentWR
	void UpdateReachLayerDischarge(FlowContext *pFlowContext); // this gets the available discharge from the reach layer
	bool SetDefaultIrrigationSeason(FlowContext *pFlowContext, WaterRight *pRight, int pouNdx, int iduNdx); // if a water right does not have a seasonality, the user can set this with an entry in the input .xml file
	void TotOutOfSeasonIrrRequest(FlowContext *pFlowContext, WaterRight *pRight, int iduNdx);

	// xml input variables
	CString  m_podTablePath;					// POD lookup file .csv
	CString  m_pouTablePath;					// POU lookup file .csv
	CString  m_dynamicWRTablePath;		   // dynamic water rights file .csv
	int      m_irrigatedHruLayer;				// index of HRU layer where irrigation water is applied
	int      m_nonIrrigatedHruLayer;			// index of HRU layer where non-irrigation water is applied
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
   FlowModel *m_pFlowModel;
   FlowContext *m_flowContext;

	CMap< Reach*, Reach*, WaterRight*, WaterRight* > m_wrMap; // used to lookup water right from reach
	PtrArray< WaterRight > m_podArray;

	//containers for input data files
	VDataObj m_pouDb; // Point of Use (POU) data object
	VDataObj m_podDb; // Point of Diversion (POD) data object
	VDataObj m_dynamicWRDb; // Point of Diversion (POD) data object

	// utility functions
	bool  AggregateIDU2HRU(FlowContext *pFlowContext);
	float GetAvailableSourceFlow(Reach *pReach);
	int   LoadWRDatabase(FlowContext *pFlowContext);
	int   LoadDynamicWRDatabase(FlowContext *pFlowContext);
	int   IDUIndexLookupMap(FlowContext *pFlowContext);

	// water allocation functions
	bool  AllocateSurfaceWR(FlowContext *pFlowContext, WaterRight *pRight, int iduNdx, float pctPou, float areaPou);
   bool  AllocateWellWR(FlowContext *pFlowContext, WaterRight *pRight, int iduNdx, float pctPou, float areaPou);

	// Conflict functions
	bool  LogWeeklySurfaceWaterIrrigationConflicts(FlowContext *pFlowContext, WaterRight *pRight, int iduNdx, float areaPou);
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
	CArray< float, float > m_iduSWMuniArrayDy;               // if Available stream flow (ASF) > Demand (D), then ASF-D (m3/sec)
	CArray< float, float > m_iduSWMuniArrayYr;               // if Available stream flow (ASF) > Demand (D), then ASF-D (m3/sec)
	CArray< float, float > m_iduSWUnAllocatedArray;          // if Available stream flow (ASF) > Demand (D), then ASF-D (m3/sec)
	CArray< float, float > m_iduSWUnsatIrrReqYr;             // if Irrigation request (D) > Available stream flow (ASF), then D-ASF (m3/sec)
	CArray< float, float > m_iduSWUnsatMunDmdYr;             // if Municipal Demand (D) > Available stream flow (ASF), then D-ASF (m3/sec)
	CArray< float, float > m_iduSWUnSatIrrArrayDy;           // if irrigation request (D) > Available stream flow (ASF), then D-ASF (m3/sec)
   CArray< float, float > m_iduSWUnSatMunArrayDy;           // if municipal request (D) > Available stream flow (ASF), then D-ASF (m3/sec)
	CArray< float, float > m_iduSWAppUnSatDemandArray;       // if Demand (D) > Point of Diversion Rate (PODR), then D-PODR (m3/sec)
	CArray< float, float > m_iduSWUnExerIrrArrayDy;          // if Point of Diversion Rate (PODR) > Demand (D), then PODR - D (m3/sec) 
	CArray< float, float > m_iduSWUnExerMunArrayDy;          // if Point of Diversion Rate (PODR) > Demand (D), then PODR - D (m3/sec) 
   CArray< float, float > m_iduAnnualIrrigationDutyArray;   // Annual irrigation duty (acrefeet)
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
	float						  m_unSatisfiedIrrSurfaceYr;			// Unsatisified surface irrigation Water (acre-ft per year)
   float                  m_wastedWaterRateDy;              // 0.0125 cfs - daily allocated irrigation water, summed over irrigated IDUs (m3/sec)
   float                  m_wastedWaterVolYr;               // vol of daily wasted water, accumulated over a one year period, for all irrigated IDUs (acre-ft/yr)
   float                  m_exceededWaterRateDy;            // m3/sec
   float                  m_exceededWaterVolYr;             // acre-ft/yr
   float                  m_daysPerYrMaxRateExceeded;       // annual count of days where max daily withdrawal rate exceeded, area weighted (days)
	FDataObj  m_timeSeriesSWIrrSummaries;							// for plotting at the end of run.
	FDataObj  m_timeSeriesSWIrrAreaSummaries;						// for plotting at the end of run. mm/day
	FDataObj  m_timeSeriesSWMuniSummaries;							// for plotting at the end of run.
	
	// GW = Ground Water  
	CArray< float, float > m_iduGWIrrArrayDy;						// Daily Allocated ground water for irrigation (mm/day) 
	CArray< float, float > m_iduGWIrrArrayYr;						// Annual Allocated ground water for irrigation (mm/day)
	CArray< float, float > m_iduGWIrrAreaArray;					// Area of Surface Water irrigation (m2)
	CArray< float, float > m_iduGWMuniArrayDy;					// if Available stream flow (ASF) > Demand (D), then ASF-D (m3/sec)
	CArray< float, float > m_iduGWMuniArrayYr;					// if Available stream flow (ASF) > Demand (D), then ASF-D (m3/sec)
	CArray< float, float > m_iduGWUnSatIrrArrayDy;           // Daily, if irrigation request (D) > Available stream flow (ASF), then D-ASF (m3/sec)
   CArray< float, float > m_iduGWUnSatMunArrayDy;           // Daily, if municipal request (D) > Available stream flow (ASF), then D-ASF (m3/sec)
	CArray< float, float > m_iduGWUnAllocatedArray;				// if Available well head (WH) > Demand (D), then Wh-D (m3/sec)
   CArray< float, float > m_iduGWUnExerIrrArrayDy;          // if Point of Diversion Rate (PODR) > Demand (D), then PODR - D (m3/sec) 
	CArray< float, float > m_iduGWUnExerMunArrayDy;          // if Point of Diversion Rate (PODR) > Demand (D), then PODR - D (m3/sec) 
	CArray< float, float > m_iduGWUnsatIrrReqYr;             // if Irrigation request (D) > Available stream flow (ASF), then D-ASF (m3/sec)
	CArray< float, float > m_iduGWUnsatMunDmdYr;             // if Municipal Demand (D) > Available stream flow (ASF), then D-ASF (m3/sec)
	CArray< float, float > m_iduGWAppUnSatDemandArray;			// if Demand (D) > Point of Diversion Rate (PODR), then D-PODR (m3/sec)
	float                  m_GWIrrAreaYr;							// yearly area of well irrigated IDUs (acres)
	float                  m_GWMuniWater;							// daily allocated municipal water from well m3/sec
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
	float							m_irrigatedWaterYr;						 // Allocated irrigation Water (acre-ft per year)
	float							m_unSatisfiedIrrigationYr;				 // Unsatisified Irrigation Water (acre-ft per year)
	float							m_unallocatedMuniWater;					 // daily unallocated municipal water m3/sec
	CArray< float, float >  m_maxTotDailyIrr;							 // Maximum total daily applied irrigation water (acre-ft)
	int							m_colMaxTotDailyIrr;						 // column in IDU layer for Maximum total daily applied irrigation water (acre-ft )
	CArray< float, float >  m_aveTotDailyIrr;							 // Average total daily applied irrigation water (acre-ft )
	CArray< int, int     >  m_nIrrPODsPerIDUYr;						 // number of Points of Diversions irrigating an idu, accumulated over the course of a yr
	int							m_colaveTotDailyIrr;						 // column in IDU layer for average total daily applied irrigation water (acre-ft )
	CArray< float, float >  m_iduUnsatIrrReqst_Dy;					 // if Demand (D) > Available stream flow (ASF), then D-ASF (mm/day)
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
	int   m_colIDUArea;                 // IDU column for area
	int   m_colIDUIndex;                // column in IDU layer for IDU_INDEX
	int   m_colWRShortG;                // WR irrigation unsatisfied irrigaton request in IDU layer.  1=level, 2=level , 0=request met
   int   m_colWRShutOff;               // WR irrigation unsatisfied irrigaton request in IDU layer.  1=level, 2=level , 0=request met
	int   m_colWRJuniorAction;          // some action is begin taken on a junion water right, depends on type of action
   int   m_colWRShutOffMun;            // WR municipal unsatisfied demand in IDU layer.  1=level, 2=level , 0=demand met
	int   m_colAllocatedIrrigation;     // Water Allocated for irrigation use (mm per year)
	int   m_colAllocatedIrrigationAf;   // Water Allocated for irrigation use (acre-ft per year)
	int   m_colSWAllocatedIrrigationAf; // Surface Water Allocated for irrigation use (acre-ft per year)
	int   m_colGWAllocatedIrrigationAf; // Ground Water Allocated for irrigation use (acre-ft per year)
	int	m_colUnsatIrrigationAf;			// Unsatisfied irrigation request (acre-ft per year) 
	int	m_colSWUnsatIrrigationAf;		// Unsatisfied surface irrigation request (acre-ft per year)
	int	m_colGWUnsatIrrigationAf;		// Unsatisfied ground irrigation request (acre-ft per year)
	int   m_colDailyAllocatedIrrigation;// Water Allocated for irrigation use (mm per day)
	int   m_colDemandFraction;			   // ratio of irrigation allocated / requested irrigation
	int   m_colAllocatedmunicipal;      // Water Allocated for municipal use (acre-ft per year)
	int   m_colUnAllocatedWR;           // Un-allocated water right
	int   m_colWaterDeficit;            // water deficit from ET calculations
	int   m_colRealUnSatisfiedDemand;   // Unsatisfied demand (mm per day)
	int   m_colUnSatIrrRequest;         // Unsatisfied irrigation request (mm per day)
	int   m_colVirtUnSatisfiedDemand;   // virtual UnSatisfied Demand
	int   m_colUnExercisedWR;           // Un-exercised WR
	int   m_colIrrApplied;              // irrigation applied, mm/day
	int   m_colDailyUrbanDemand;			// Daily urban water demand m3/sec
   int   m_colPlantDate;               // Crop planting date in IDU layer
	int   m_colHarvDate;						// Crop harvest date in IDU layer
	int   m_colMunPodRate;              // total POD rate for all PODs in IDU with municipal water right m3/sec
   int   m_colIrrPodRate;              // total POD rate for all PODs in IDU with irrigation water right m3/sec
   int   m_colIrrig_yr;                // total irrigation per year (mm)
	int   m_colLulc_A;						// Land Use/Land Cover (coarse). used for finding Agriculture IDUs
	int   m_colLulc_B;						// Land Use/Land Cover (medium). used for finding crop type
	int   m_colSWIrrDy;						// Daily surface water applied to IDU (mm/day)
	int   m_colGWIrrDy;						// Daily ground water applied to IDU (mm/day)
	int   m_colMaxDutyLayer;            // For irrigatable IDUs, if this attribute present in IDU layer, is  maximum duty for IDU
	int   m_colWRConflictYr;            // Attribute in stream layer that types a reach as being in "conflict" (not satisfying WR) 0- no conflict, 1-available < demand, 2-available < demand/2
	int   m_colWRConflictDy;            // Attribute in stream layer that types a reach as being in "conflict" (not satisfying WR) 0- no conflict, 1-level one, 2-level two
	int   m_colAllocatedmunicipalDay;   // column in IDU layer for daily municipal water allocations (m3/sec)
	int   m_colSWAlloMunicipalDay;      // column in IDU layer for daily municipal surface water allocations (m3/sec)
	int   m_colGWAlloMunicipalDay;      // column in IDU layer for daily municipal ground water allocations (m3/sec)
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

	map<IDUIndexKeyClass, std::vector<int>, IDUIndexClassComp> m_IDUIndexMap;  //used for idu index, finding IDUs associated with POUs
	IDUIndexKeyClass m_iduIndexInsertKey;
	IDUIndexKeyClass m_iduIndexLookupKey;

	// Point of Diversion (POD) input data file columns
	int   m_colWRID;          // WR id
	int   m_colDemand;        // desired demand (m3/sec) (in)
	int   m_colWaterUse;      // actual use (out)
	int   m_colXcoord;        // WR UTM zone x coordinate (m)
	int   m_colYcoord;        // WR UTM zone y coordinate (m)
	int   m_colPodID;         // WR column number POD ID input data file
	int   m_colPDPouID;       // WR column number POU ID in POD input data file
	int   m_colAppCode;       // WR Application code http://www.oregon.gov/owrd/pages/wr/wrisuse.aspx
	int   m_colPermitCode;    // WR Permit Code http://www.oregon.gov/owrd/pages/wr/wrisuse.aspx
	int   m_colPodRate;       // WR point of diversion max rate (m3/sec)
	int   m_colUseCode;       // WR Use Code http://www.oregon.gov/owrd/pages/wr/wrisuse.aspx
	int   m_colSupp;          // WR supplemental code 0-primary 1-Supplemental
	int   m_colPriorDoy;      // WR priority date day of year
	int   m_colPriorYr;       // WR priority date year
	int   m_colBeginDoy;      // WR seasonal begin day of year
	int   m_colEndDoy;        // WR seasonal end day year
	int   m_colPouRate;       // WR point of use max rate (m3/sec)
	int   m_colPodStatus;     // WR point of diversion status code
	int   m_colReachComid;    // WR Reach comid, column in POD lookup file relates to COMID in reach layer
	int   m_colStrLayComid;   // column ID of COMID in stream layer
	int   m_coliDULayComid;   // column ID of COMID in IDU layer
	int   m_colWRExists;      // A bitwise set attribute in IDU layer when decoded characterizes WR_USE and WR_PERMIT
	int   m_colPodUseRate;    // column for pod use rate m3/sec
	int   m_colDistPodReach;  // column for pod to reach distance (m)
	int   m_colMaxDutyFile;   // column for maximum duty in POD input data file (acre-ft / acre);   

	// Point of Use (POU) input data file columns
	int   m_colPUPouID;       // WR column number POU ID in POU input data file
	int   m_colIDUndx;        // Relates to the IDU_INDEX attribute in the IDU layer
	int   m_colPouPct;        // The areal percentage of the POU, for a SnapID, that over laps the IDU/IDU_INDEX
	int   m_colPouArea;       // Area (study area units)
	int   m_colPouDBNdx;      // a zero based index for the POU input data file itself

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
	

	// Miscellaneous
	int	  m_mostJuniorWR;												// the year of the most junior water right
	int     m_mostSeniorWR;												// the year of the most senior water right
	int     m_dynamicWRType;									      // input variable index
	GeoSpatialDataObj *m_myGeo = new GeoSpatialDataObj(U_UNDEFINED);

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


