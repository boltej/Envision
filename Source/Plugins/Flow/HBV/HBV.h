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
#include <FDataObj.h>
class FlowContext;
class Reach;


class HBV
{
public:
   HBV( void ) : m_pClimateData( NULL )
      , m_col_cfmax (-1)      
      , m_col_tt (-1)  
      , m_col_sfcf (-1) 
      , m_col_cwh  (-1)
      , m_col_cfr (-1)  
      , m_col_lp (-1)  
      , m_col_fc (-1)  
      , m_col_beta (-1)  
      , m_col_kperc(-1) 
      , m_col_wp(-1) 
      , m_col_k0 (-1)  
      , m_col_k1 (-1)  
      , m_col_uzl (-1)  
      , m_col_k2 (-1) 
      , m_col_ksoilDrainage(-1)
      , m_colTMax(-1)
	   , m_colDailyUrbanDemand(-1)
	   , m_colH2OResidnt(-1)
	   , m_colH2OIndComm(-1)
	   , m_colIDUArea(-1)
	   , m_colUGB( -1 )
      , m_col_rain(-1)
      , m_col_snow(-1)
      , m_col_sfcfCorrection(-1)
      , m_precipTotal(0.0f)
    { }

   ~HBV( void ) { if ( m_pClimateData ) delete m_pClimateData; }

  protected:
   int m_col_cfmax ;      
   int m_col_tt ;  
   int m_col_sfcf ; 
   int m_col_cwh  ;
   int m_col_cfr ;  
   int m_col_lp ;  
   int m_col_fc ;  
   int m_col_beta ;  
   int m_col_kperc; 
   int m_col_wp; 
   int m_col_k0 ;  
   int m_col_k1 ;  
   int m_col_uzl ; 
   int m_col_ksoilDrainage;
   int m_col_k2 ; 
   int m_col_rain;
   int m_col_snow;
   int m_col_sfcfCorrection;
   float m_precipTotal;
   int m_col_SSURGO_WP;
   int m_col_SSURGO_FC;
   int m_col_thickness;

   float Musle( FlowContext *pFlowContext );
   FDataObj *m_pClimateData;

   // HBV Vertical
   float Snow(float waterDepth, float precip, float temp, float CFMAX, float CFR, float TT );
   float Melt(float waterDepth, float precip, float temp, float CFMAX, float CFR, float TT );
   float GroundWaterRecharge(float precip,float waterDepth, float FC,  float Beta); 
   float GroundWaterRechargeFraction(float precip,float waterDepth, float FC,  float Beta); 
   float Percolation(float waterDepth, float kPerc);
   float PercolationHBV(float waterDepth, float kPerc);

   //HBV Horizontal
   float Q0( float waterDepth, float k0, float k1, float UZL );
   float Q2( float waterDepth , float k2 );

   // initialization
   float InitHBV(FlowContext *pFlowContext, LPCTSTR); 
   float InitHBV_WithQuickflow(FlowContext *pFlowContext, LPCTSTR);
   
   // miscellaneous other stuff
public:  // these should be phased out
   float HBV_HorizontalwSnow( FlowContext *pFlowContext );
   float HBV_VerticalwSnow( FlowContext *pFlowContext );

   float ExchangeFlowToCalvin(FlowContext *pFlowContext);
   float ExchangeCalvinToFlow(FlowContext *pFlowContext);

protected:
   float ET( float waterDepth, float fc, float lp, float wp, float temp, int month, int doy, float &etRc);
   float CalcET(  float temp, int month, int doy ) ;//hargreaves equation

protected:
   CArray< float > m_tMaxArray;     // one for each reach calculated
   int   m_colTMax;

protected:
	int   m_colDailyUrbanDemand;	// Calculated Daily Urband Water Demand m3/day
	int   m_colH2OResidnt;			// Annual Residential Demand ccf/day/acre
	int 	m_colH2OIndComm;			// Annual Residential Demand ccf/day/acre
	int   m_colIDUArea;				// IDU area from IDU layer
	int   m_colUGB;					// IDU UGB from IDU layer


   float GetMean(FDataObj *pData, int col, int startRow, int numRows);
// public (exported) methods
public:
   //-------------------------------------------------------------------
   //------ models -----------------------------------------------------
   //-------------------------------------------------------------------
   float HBV_Basic( FlowContext *pFlowContext );          // formerly HBV_Global
   float HBV_WithIrrigation(FlowContext *pFlowContext);   // formerly HBV_IrrigSoil
   float HBV_WithQuickflow(FlowContext *pFlowContext);    // formerly HBV_Agriculture
   float HBV_WithRadiationDrivenSnow(FlowContext *pFlowContext);   // formerly HBV_Global_Radiation

   float HBV_OnlyGroundwater(FlowContext *pFlowContext);       

   //-------------------------------------------------------------------
   //------ exchange handlers ------------------------------------------
   //-------------------------------------------------------------------
   float HBV_Horizontal( FlowContext *pFlowContext );
   float HBV_Vertical( FlowContext *pFlowContext );
   
   //-------------------------------------------------------------------
   //------ flux handlers ----------------------------------------------
   //-------------------------------------------------------------------
   float ETFluxHandler( FlowContext *pFlowContext );
   float PrecipFluxHandler( FlowContext *pFlowContext );
   float PrecipFluxHandlerNoSnow( FlowContext *pFlowContext );
   float N_Deposition_FluxHandler( FlowContext *pFlowContext );
   float N_ET_FluxHandler( FlowContext *pFlowContext  );
   float SnowFluxHandler( FlowContext *pFlowContext );
   float RunoffFluxHandler( FlowContext *pFlowContext );
   float UplandFluxHandler( FlowContext *pFlowContext );
   float RuninFluxHandler( FlowContext *pFlowContext );

   //-------------------------------------------------------------------
   //------ state variable methods -------------------------------------
   //-------------------------------------------------------------------
   float N_Transformations( FlowContext *pFlowContext );

   //-------------------------------------------------------------------
   //------ stream temperature -----------------------------------------
   //-------------------------------------------------------------------
   BOOL StreamTempInit( FlowContext *pFlowContext, LPCTSTR initStr );     // invoked during Init()
   BOOL StreamTempRun ( FlowContext *pFlowContext );     // invoked daily over the course of a year
   BOOL StreamTempReg( FlowContext *pFlowContext );     // invoked during Init() and EndStep()
 
   //-------------------------------------------------------------------  
   //------- urban water demand ----------------------------------------
   //-------------------------------------------------------------------  
	//BOOL CalcDailyUrbanWater(FlowContext *pFlowContext);
	//BOOL CalcDailyUrbanWaterInit(FlowContext *pFlowContext);
	//BOOL CalcDailyUrbanWaterRun(FlowContext *pFlowContext);


};