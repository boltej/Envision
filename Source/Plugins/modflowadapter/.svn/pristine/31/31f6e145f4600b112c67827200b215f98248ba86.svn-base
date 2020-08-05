// ModflowAdapter.cpp : Defines the initialization routines for the DLL.
//
#include "stdafx.h"
#include "ModflowAdapter.h"
#include <Flow\Flow.h>
#include <map.h>
#include <maplayer.h>
#include <sys/stat.h>
#include "PolyGridLookups.h"
#include <Path.h>
#include <PathManager.h>
#include <EnvEngine\EnvConstants.h>
#define FailAndReturn(FailMsg) \
	{msg = failureID + (_T(FailMsg)); \
		Report::ErrorMsg(msg); \
		return false; }

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
ModflowAdapter::~ModflowAdapter( void ){};

ModflowAdapter::ModflowAdapter( void ):
  m_hInst			( 0 )
, m_GetNamFilFn	( NULL )
, m_OpenNamFilFn	( NULL )
, m_GWF2BAS7ARFn	( NULL )
, m_GWF2LPF7ARFn	( NULL )  
, m_GWF2WEL7ARFn	( NULL )  
, m_GWF2DRN7ARFn	( NULL )  
, m_GWF2RIV7ARFn	( NULL )
, m_GWF2CHD7ARFn	( NULL )
, m_GWF2RCH7ARFn	( NULL )  
, m_GMG7ARFn		( NULL )      
, m_OBS2BAS7ARFn	( NULL )  
, m_GWF2BAS7STFn	( NULL )  
, m_GWF2WEL7RPFn	( NULL )  
, m_GWF2DRN7RPFn	( NULL )  
, m_GWF2RIV7RPFn	( NULL )
, m_GWF2CHD7RPFn  ( NULL )
, m_GWF2RCH7RPFn	( NULL )  
, m_GWF2BAS7ADFn	( NULL )  
, m_GWF2LPF7ADFn	( NULL )
, m_GWF2CHD7ADFn  ( NULL )
, m_UMESPRFn		( NULL )      
, m_GWF2BAS7FMFn	( NULL )  
, m_GWF2LPF7FMFn	( NULL )  
, m_GWF2WEL7FMFn	( NULL )  
, m_GWF2DRN7FMFn	( NULL )  
, m_GWF2RIV7FMFn	( NULL )  
, m_GWF2RCH7FMFn	( NULL )  
, m_GMG7PNTFn		( NULL )      
, m_GMG7APFn		( NULL ) 
, m_GWF2BAS7OCFn	( NULL )   
, m_GWF2LPF7BDSFn	( NULL )  
, m_GWF2LPF7BDCHFn	( NULL ) 
, m_GWF2LPF7BDADJFn	( NULL )
, m_GWF2WEL7BDFn	( NULL )   
, m_GWF2DRN7BDFn	( NULL )   
, m_GWF2RIV7BDFn	( NULL )   
, m_GWF2LAK7STFn	( NULL )   
, m_GWF2RCH7BDFn	( NULL )   
, m_GWF2BAS7OTFn	( NULL )   
, m_GLO1BAS6ETFn	( NULL )   
, m_SGWF2BAS7PNTFn	( NULL ) 
, m_GWF2WEL7DAFn	( NULL )   
, m_GWF2DRN7DAFn	( NULL )   
, m_GWF2RIV7DAFn	( NULL )   
, m_GWF2RCH7DAFn	( NULL )   
, m_GWF2LPF7DAFn	( NULL )
, m_GWF2CHD7DAFn	( NULL )
, m_OBS2BAS7DAFn	( NULL )   
, m_GWF2BAS7DAFn	( NULL )
, m_PCG7PNTFn	   ( NULL )
, m_PCG7APFn      ( NULL )
//, m_PCG7DAFn      ( NULL )
, m_inUnit		   ( 99 )
, m_iGrid		   ( 1 )
, m_maxunit		   ( 100 )
, m_iudis			( 24 )
, m_iuzon			( 31 )
, m_iumlt			( 32 )
, m_iuoc			   ( 12 )
, m_iupval		   ( 26 )
, m_cellDim         ( -1 )
, m_numSubGridCells ( 3 )
, m_pafScanbandbool( NULL )
, m_nStressPeriods( 0 )
	{};

BOOL ModflowAdapter::Init(  FlowContext *pFlowContext,LPCTSTR initStr)
	{
	     
	//m_hInst = ::AfxLoadLibrary("\\Users\\james\\Documents\\OSU\\Envision\\modflow\\mf2005v1_9_01\\ModFlow2005v1_9_01\\x64\\Debug\\ModFlow2005v1_9_01.dll");
   #if defined( _DEBUG )
   m_hInst = ::AfxLoadLibrary("\\Envision\\ModFlow\\x64\\debug\\ModFlow2005v1_9_01.dll");
   #else
   m_hInst = ::AfxLoadLibrary("\\Envision\\ModFlow\\x64\\release\\ModFlow2005v1_9_01.dll");
   #endif
   
   if ( m_hInst == 0 )
      {
      CString msg( "modflow: Unable to find modflow's .dll" );
     
      Report::ErrorMsg( msg );
      return FALSE;
      }

	//m_HNEW            = (MODFLOW_GLOBAL_mp_HNEWFN)   GetProcAddress(m_hInst,"GLOBAL_mp_HNEW");
	m_GetNamFilFn     = (MODFLOW_GETNAMFILFN)		    GetProcAddress(m_hInst,"GETNAMFIL");
	m_OpenNamFilFn    = (MODFLOW_OPENNAMFILFN)		 GetProcAddress(m_hInst,"OPENNAMFIL");
	m_GWF2BAS7ARFn    = (MODFLOW_GWF2BAS7ARFN)		 GetProcAddress(m_hInst,"GWF2BAS7AR");  												 
	m_GWF2LPF7ARFn    = (MODFLOW_GWF2LPF7ARFN)		 GetProcAddress(m_hInst,"GWF2LPF7AR");      
	m_GWF2WEL7ARFn    = (MODFLOW_GWF2WEL7ARFN)		 GetProcAddress(m_hInst,"GWF2WEL7AR");  	
	m_GWF2DRN7ARFn    = (MODFLOW_GWF2DRN7ARFN)		 GetProcAddress(m_hInst,"GWF2DRN7AR");  	
	m_GWF2RIV7ARFn    = (MODFLOW_GWF2RIV7ARFN)		 GetProcAddress(m_hInst,"GWF2RIV7AR");  	
	m_GWF2RCH7ARFn    = (MODFLOW_GWF2RCH7ARFN)		 GetProcAddress(m_hInst,"GWF2RCH7AR");
   m_GWF2CHD7ARFn    = (MODFLOW_GWF2CHD7ARFN)		 GetProcAddress(m_hInst,"GWF2CHD7AR");
	m_GMG7ARFn        = (MODFLOW_GMG7ARFN)		       GetProcAddress(m_hInst,"GMG7AR");      	
	m_OBS2BAS7ARFn    = (MODFLOW_OBS2BAS7ARFN)		 GetProcAddress(m_hInst,"OBS2BAS7AR");  	
	m_GWF2BAS7STFn    = (MODFLOW_GWF2BAS7STFN)		 GetProcAddress(m_hInst,"GWF2BAS7ST");  	
	m_GWF2WEL7RPFn    = (MODFLOW_GWF2WEL7RPFN)		 GetProcAddress(m_hInst,"GWF2WEL7RP");  	
	m_GWF2DRN7RPFn    = (MODFLOW_GWF2DRN7RPFN)		 GetProcAddress(m_hInst,"GWF2DRN7RP");  	
	m_GWF2RIV7RPFn    = (MODFLOW_GWF2RIV7RPFN)		 GetProcAddress(m_hInst,"GWF2RIV7RP");
   m_GWF2CHD7RPFn    = (MODFLOW_GWF2CHD7RPFN)		 GetProcAddress(m_hInst,"GWF2CHD7RP");
	m_GWF2RCH7RPFn    = (MODFLOW_GWF2RCH7RPFN)		 GetProcAddress(m_hInst,"GWF2RCH7RP");  	
	m_GWF2BAS7ADFn    = (MODFLOW_GWF2BAS7ADFN)		 GetProcAddress(m_hInst,"GWF2BAS7AD");  	
	m_GWF2LPF7ADFn    = (MODFLOW_GWF2LPF7ADFN)		 GetProcAddress(m_hInst,"GWF2LPF7AD");
   m_GWF2CHD7ADFn    = (MODFLOW_GWF2CHD7ADFN)		 GetProcAddress(m_hInst,"GWF2CHD7AD");
	m_UMESPRFn        = (MODFLOW_UMESPRFN)		       GetProcAddress(m_hInst,"UMESPR");      	
	m_GWF2BAS7FMFn    = (MODFLOW_GWF2BAS7FMFN)		 GetProcAddress(m_hInst,"GWF2BAS7FM");  	
	m_GWF2LPF7FMFn    = (MODFLOW_GWF2LPF7FMFN)		 GetProcAddress(m_hInst,"GWF2LPF7FM");  	
	m_GWF2WEL7FMFn    = (MODFLOW_GWF2WEL7FMFN)		 GetProcAddress(m_hInst,"GWF2WEL7FM");  	
	m_GWF2DRN7FMFn    = (MODFLOW_GWF2DRN7FMFN)		 GetProcAddress(m_hInst,"GWF2DRN7FM");  	
	m_GWF2RIV7FMFn    = (MODFLOW_GWF2RIV7FMFN)		 GetProcAddress(m_hInst,"GWF2RIV7FM");  	
	m_GWF2RCH7FMFn    = (MODFLOW_GWF2RCH7FMFN)		 GetProcAddress(m_hInst,"GWF2RCH7FM");  	
	m_GMG7PNTFn       = (MODFLOW_GMG7PNTFN)	       GetProcAddress(m_hInst,"GMG7PNT");      	
	m_GMG7APFn        = (MODFLOW_GMG7APFN)			    GetProcAddress(m_hInst,"GMG7AP"); 		
	m_GWF2BAS7OCFn    = (MODFLOW_GWF2BAS7OCFN)       GetProcAddress(m_hInst,"GWF2BAS7OC");    
	m_GWF2LPF7BDSFn   = (MODFLOW_GWF2LPF7BDSFN)      GetProcAddress(m_hInst,"GWF2LPF7BDS");   
	m_GWF2LPF7BDCHFn  = (MODFLOW_GWF2LPF7BDCHFN)     GetProcAddress(m_hInst,"GWF2LPF7BDCH");  
	m_GWF2LPF7BDADJFn = (MODFLOW_GWF2LPF7BDADJFN)    GetProcAddress(m_hInst,"GWF2LPF7BDADJ"); 
	m_GWF2WEL7BDFn    = (MODFLOW_GWF2WEL7BDFN)		 GetProcAddress(m_hInst,"GWF2WEL7BD");    
	m_GWF2DRN7BDFn    = (MODFLOW_GWF2DRN7BDFN)		 GetProcAddress(m_hInst,"GWF2DRN7BD");    
	m_GWF2RIV7BDFn    = (MODFLOW_GWF2RIV7BDFN)		 GetProcAddress(m_hInst,"GWF2RIV7BD");    
	m_GWF2LAK7STFn    = (MODFLOW_GWF2LAK7STFN)		 GetProcAddress(m_hInst,"GWF2LAK7ST");    
	m_GWF2RCH7BDFn    = (MODFLOW_GWF2RCH7BDFN)		 GetProcAddress(m_hInst,"GWF2RCH7BD");    
	m_GWF2BAS7OTFn    = (MODFLOW_GWF2BAS7OTFN)		 GetProcAddress(m_hInst,"GWF2BAS7OT");    
	m_GLO1BAS6ETFn    = (MODFLOW_GLO1BAS6ETFN)		 GetProcAddress(m_hInst,"GLO1BAS6ET");    
	m_SGWF2BAS7PNTFn  = (MODFLOW_SGWF2BAS7PNTFN)     GetProcAddress(m_hInst,"SGWF2BAS7PNT");  
	m_GWF2WEL7DAFn    = (MODFLOW_GWF2WEL7DAFN)		 GetProcAddress(m_hInst,"GWF2WEL7DA");    
	m_GWF2DRN7DAFn    = (MODFLOW_GWF2DRN7DAFN)		 GetProcAddress(m_hInst,"GWF2DRN7DA");    
	m_GWF2RIV7DAFn    = (MODFLOW_GWF2RIV7DAFN)		 GetProcAddress(m_hInst,"GWF2RIV7DA");    
	m_GWF2RCH7DAFn    = (MODFLOW_GWF2RCH7DAFN)		 GetProcAddress(m_hInst,"GWF2RCH7DA");    
	m_GWF2LPF7DAFn    = (MODFLOW_GWF2LPF7DAFN)		 GetProcAddress(m_hInst,"GWF2LPF7DA");
   m_GWF2CHD7DAFn    = (MODFLOW_GWF2CHD7DAFN)		 GetProcAddress(m_hInst,"GWF2CHD7DA");
	m_OBS2BAS7DAFn    = (MODFLOW_OBS2BAS7DAFN)		 GetProcAddress(m_hInst,"OBS2BAS7DA");    
	m_GWF2BAS7DAFn    = (MODFLOW_GWF2BAS7DAFN)		 GetProcAddress(m_hInst,"GWF2BAS7DA"); 
   m_PCG7ARFn        = (MODFLOW_PCG7ARFN)		       GetProcAddress(m_hInst,"PCG7AR");
   m_PCG7PNTFn       = (MODFLOW_PCG7PNTFN)		    GetProcAddress(m_hInst,"PCG7PNT");
   //m_PCG7DAFn       = (MODFLOW_PCG7DAFN)		       GetProcAddress(m_hInst,"PCG7DA");
   m_PCG7APFn       = (MODFLOW_PCG7APFN)		       GetProcAddress(m_hInst,"PCG7AP");

   if ( m_hInst != 0 )
	{

	msg.Format( "ModFlowAdapter::Init successfull" );
   
	Report::LogMsg( msg, RT_INFO );
	}


//************************************************
// run dependent stuff
   m_issFlag       = new int[12]; //size equals number of stress periods
//***************************

  m_hNew           = new double[66][117][6];
  m_col            = new int[1];
  m_row            = new int[1];
  m_lay            = new int[1];
  m_ibdt           = new int[8]; 
  m_iunit          = new int[100];
  m_nper           = new int[1];
  m_nstp           = new int[4];
  m_iout           = new int[1];
  m_stoper         = new double[1];
  m_rhs    		    = new float[66][117][6];   
  m_hcof	          = new float[66][117][6];  
  m_hNewLast       = new float[1][1][1];
  m_iBound	       = new int[66][117][6];
  m_nodes          = new int[1];
  m_cc   		    = new float[66][117][6];   
  m_cv		       = new float[66][117][6];   
  m_cr		       = new float[66][117][6];
  m_nrchop         = new int[1];
  m_recharge       = new float[66][117];
  m_mxiter         = new int[1];
  m_iter1          = new int[1];
  m_dampSS         = new float[1];
  m_dampTR         = new float[1];
  m_npcond         = new int[1];
  m_ihcofadd       = new int[1];
  m_hClose         = new float[1];
  m_rClose         = new float[1];
  m_relax          = new float[1];
  m_nbpol          = new int[1];
  m_iprpcg         = new int[1];
  m_mutpcg         = new int[1];
  m_iadampgmg      = new int[1];
  m_iunitmhc       = new int[1]; 
  m_iiter          = new int[1];
  m_dampgmg        = new float[1];
  m_ioutgmg        = new int[1];
  m_ism            = new int[1];
  m_isc            = new int[1];
  m_chglimit       = new float[1];
  m_dup            = new float[1];
  m_dlow           = new float[1];
  m_gmgid          = new int[1];
  m_hOld           = new double[66][117][6];
  m_niter          = new int[1];
  m_v              = new double[66][117][6];
  m_ss             = new double[66][117][6];
  m_p              = new double[66][117][6];
  m_cd             = new double[66][117][6];
  m_hpcg           = new double[66][117][6];
  m_res            = new float [66][117][6];
  m_hcsv           = new double[66][117][6];
  m_hchg           = new float[600];
  m_lhch           = new int[3][600]; // 20 30 1 0 # MXITER, ITER1, NPCOND, IHCOFADD
  m_rchg           = new float[600];
  m_lrch           = new int[3][600];
  m_icnvg          = new int[1];
  m_it1            = new int[600];
  m_iss            = new int[1];
  m_hdry           = new float[1];
  m_siter          = new int[1];
  m_tsiter         = new int[1];
  m_bigheadchg     = new double[1];
  m_hnoflo         = new float[1];
  m_budperc        = new double[1];
  locbudperc       = new double[1];
  m_nWells         = new int[1];
  m_nDrains        = new int[1];
  m_nRivers        = new int[1];
  m_iNrecharge     = new int[1];
  m_nCHDS          = new int[1];
  m_wells          = new float[8,2013]; //layer,row,col,rate,
  m_ncverr = 0; //run

  float HDRY1 = 1.e30;
  m_hdry[0]      = HDRY1;
 

//*************************************************
   
   return TRUE;
	}

BOOL ModflowAdapter::InitRun( FlowContext *pFlowContext )
	{
	char *fileName="\\Envision\\StudyAreas\\WW2100\\ModFlow\\TRR.nam";
	
   char locVersion[17] = {"1.9.01 5/01/2012"};

	char *TEXT11="SOLVING FOR HEAD";
	
	char *TEXT22="                ";

   char locMFVNAM[6]  ={"-2005"};

    m_nWells[0] = 0;

    m_nDrains[0] = 0;

    m_nRivers[0] = 0;

    m_iNrecharge[0] = 0;

    m_nCHDS[0] = 0;

	char locCUnit[100][5] =  {"BCF6", "WEL ", "DRN ", "RIV ", "EVT ", "gfd ", "GHB ",  //  7 init
						      "RCH ", "SIP ", "DE4 ", "    ", "OC  ", "PCG ", "lmg ",  // 14
						      "gwt ", "FHB ", "RES ", "STR ", "IBS ", "CHD ", "HFB6",  // 21
						      "LAK ", "LPF ", "DIS ", "    ", "PVAL", "    ", "HOB ",  // 28
						      "    ", "    ", "ZONE", "MULT", "DROB", "RVOB", "GBOB",  // 35
						      "STOB", "HUF2", "CHOB", "ETS ", "DRT ", "    ", "GMG ",  // 42
						      "HYD ", "SFR ", "    ", "GAGE", "LVDA", "    ", "LMT6",  // 49
						      "MNW2", "MNWI", "MNW1", "KDEP", "SUB ", "UZF ", "gwm ",  // 56
						      "SWT ", "cfp ", "PCGN", "    ", "    ", "    ", "nrs ",  // 63
						      "    ", "    ", "    ", "    ", "    ", "    ", "    ",  // 70
	                       "    ", "    ", "    ", "    ", "    ", "    ", "    ",  // 77
						      "    ", "    ", "    ", "    ", "    ", "    ", "    ",  // 84
						      "    ", "    ", "    ", "    ", "    ", "    ", "    ",  // 91
						      "    ", "    ", "    ", "    ", "    ", "    ", "    ",  // 98
						      "    ", "    "};                                         // 100
    //init
	char locHeading[2][81] = {"                                                                                ","                                                                                "};
   
   double *tmpHNEW  =  static_cast<double*>(m_hNew);
   double *tmpHOLD  =  static_cast<double*>(m_hOld);
   double *tmpHNEWLAST  =  static_cast<double*>(m_hNewLast);
   int *tmpIBOUND  =  static_cast<int*>(m_iBound);
   float *tmpCR  =  static_cast<float*>(m_cr);
   float *tmpCC  =  static_cast<float*>(m_cc);
   float *tmpCV  =  static_cast<float*>(m_cv);
   float *tmpHCOF  =  static_cast<float*>(m_hcof);
   float *tmpRHS  =  static_cast<float*>(m_rhs);
   double *tmpV  =  static_cast<double*>(m_v);
   double *tmpSS  =  static_cast<double*>(m_ss);
   double *tmpP  =  static_cast<double*>(m_p);
   float *tmpCD  =  static_cast<float*>(m_cd);
   double *tmpHPCG  =  static_cast<double*>(m_hpcg);
   float *tmpRES  =  static_cast<float*>(m_res);
   float *tmpHCSV  =  static_cast<float*>(m_hcsv);

   HNEW1d     = new double [46332] ;
   IBOUND1d     = new int [46332] ;
   CR1d     = new float [46332] ;
   CC1d     = new float [46332] ;
   CV1d     = new float [46332] ;
   HCOF1d     = new float [46332] ;
   RHS1d     = new float [46332] ;
   V1d     = new double [46332] ;
   SS1d     = new double [46332] ;
   P1d     = new double [46332] ;
   CD1d     = new float [46332] ;
   HPCG1d     = new double [46332] ;
   RES1d     = new float [46332] ;
   HCSV1d     = new float [46332] ;


	GetNamFil(fileName);
	
	OpenNamFil(&m_inUnit,fileName,m_ibdt);

	GWF2BAS7AR(&m_inUnit,locCUnit,locVersion,&m_iudis,&m_iuzon,&m_iumlt,
		        &m_maxunit,&m_iGrid,&m_iuoc,locHeading,&m_iupval,locMFVNAM,
				  m_iunit,m_nper,m_nstp,m_iout,m_col,m_row,
				  m_lay,m_stoper,m_hNew,m_rhs,
				  m_hcof,m_hNewLast,m_iBound,m_nodes,m_issFlag);
  
   int N = (23 + (31-1)*66 + (1-1)*46332)-1;
   double hnewtest  = *(tmpHNEW+N);
   double holdtest  = *(tmpHOLD+N);
   int    iboundtest   = *(tmpIBOUND+N);
   float  crtest     = *(tmpCR+N);
   float  cctest     = *(tmpCC+N);
   float  cvtest     = *(tmpCV+N);
   float  hcoftest   = *(tmpHCOF+N);
   float  rhstest    = *(tmpRHS+N);
   double vtest     = *(tmpV+N);
   double sstest    = *(tmpSS+N);
   double ptest     = *(tmpP+N);
   float  cdtest     = *(tmpCD+N);
   double hpcgtest  = *(tmpHPCG+N);
   float  restest    = *(tmpRES+N);
   float  hcsvtest   = *(tmpHCSV+N);
 	
   m_pPolyMapLayer = (MapLayer*) pFlowContext->pEnvContext->pMapLayer;

   // Initialize polygrid lookups
   //m_pPolyMapLayer = m_pMap->GetLayer(1);
	
	bool lpgl = LoadPolyGridLookup( m_pPolyMapLayer, m_col, m_row );
   
	if( ! lpgl )
      FailAndReturn("ModflowAdapter: LoadPolyGridLookup() returned false");

  if ( m_iunit[23-1] > 0 ) GWF2LPF7AR(&m_iunit[23-1],&m_iGrid,m_cc,m_cv,m_cr);//************************************************************************
	hnewtest  = *(tmpHNEW+N);
   holdtest  = *(tmpHOLD+N);
   iboundtest   = *(tmpIBOUND+N);
   crtest     = *(tmpCR+N);
   cctest     = *(tmpCC+N);
   cvtest     = *(tmpCV+N);
   hcoftest   = *(tmpHCOF+N);
   rhstest    = *(tmpRHS+N);
   vtest     = *(tmpV+N);
   sstest    = *(tmpSS+N);
   ptest     = *(tmpP+N);
   cdtest     = *(tmpCD+N);
   hpcgtest  = *(tmpHPCG+N);
   restest    = *(tmpRES+N);
   hcsvtest   = *(tmpHCSV+N);  
//	  if ( m_iunit[37-1] > 0 ) GWF2HUF7AR(m_iunit[37-1],m_iunit[47-1],m_iunit[53-1],&m_iGrid);
	if ( m_iunit[2-1] > 0 )  GWF2WEL7AR(&m_iunit[2-1],&m_iGrid);//*******************************************************
	if ( m_iunit[3-1] > 0 )  GWF2DRN7AR(&m_iunit[3-1],&m_iGrid);//************************************************************************
	if ( m_iunit[4-1] > 0 )  GWF2RIV7AR(&m_iunit[4-1],&m_iGrid);//************************************************************************
//	  if ( m_iunit[5-1] > 0 )  GWF2EVT7AR(m_iunit[5-1],&m_iGrid);
//	  if ( m_iunit[7-1] > 0 )  GWF2GHB7AR(m_iunit[7-1],&m_iGrid);
	if ( m_iunit[8-1] > 0 )  GWF2RCH7AR(&m_iunit[8-1],&m_iGrid,m_nrchop);//************************************************************************
//	  if ( m_iunit[16-1] > 0 ) GWF2FHB7AR(m_iunit[16-1],&m_iGrid);
//	  if ( m_iunit[17-1] > 0 ) GWF2RES7AR(m_iunit[17-1],&m_iGrid);
//	  if ( m_iunit[18-1] > 0 ) GWF2STR7AR(m_iunit[18-1],&m_iGrid);
//	  if ( m_iunit[19-1] > 0 ) GWF2IBS7AR(m_iunit[19-1],m_iunit[54-1],&m_iGrid);
	  if ( m_iunit[20-1] > 0 ) GWF2CHD7AR(&m_iunit[20-1],&m_iGrid);
//	  if ( m_iunit[21-1] > 0 ) GWF2HFB7AR(m_iunit[21-1],&m_iGrid);
//	  if ( m_iunit[44-1] > 0 ) GWF2SFR7AR(m_iunit[44-1],m_iunit[1-1],m_iunit[23-1],m_iunit[37-1],m_iunit[15-1],NSOL,IOUTS,&m_iGrid);
//	  if ( m_iunit[55-1] > 0 ) GWF2UZF1AR(m_iunit[55-1],m_iunit[1-1],m_iunit[23-1],m_iunit[37-1],&m_iGrid);
//	  if ( m_iunit[22-1] > 0 || m_iunit[44-1] > 0 ) GWF2LAK7AR(m_iunit[22-1],m_iunit[44-1],m_iunit[15-1],m_iunit[55-1],NSOL,&m_iGrid);
//	  if ( m_iunit[46-1] > 0 ) GWF2GAG7AR(m_iunit[46-1],m_iunit[44-1],m_iunit[22-1],&m_iGrid);
//	  if ( m_iunit[39-1] > 0 ) GWF2ETS7AR(m_iunit[39-1],&m_iGrid);
//	  if ( m_iunit[40-1] > 0 ) GWF2DRT7AR(m_iunit[40-1],&m_iGrid);
//	  if ( m_iunit[54-1] > 0 ) GWF2SUB7AR(m_iunit[54-1],&m_iGrid);
//	  if ( m_iunit[9-1] > 0 )  SIP7AR(m_iunit[9-1],MXITER,&m_iGrid);
//	  if ( m_iunit[10-1] > 0 ) DE47AR(m_iunit[10-1],MXITER,&m_iGrid);
	if ( m_iunit[13-1] > 0 ) PCG7AR(&m_iunit[13-1],m_mxiter,&m_iGrid,
                                    m_iter1,
                                    m_dampSS,
                                    m_dampTR,
                                    m_npcond,
                                    m_ihcofadd,
                                    m_hClose,
                                    m_rClose,
                                    m_relax,
                                    m_nbpol,
                                    m_iprpcg,
                                    m_mutpcg);
//	  if ( m_iunit[14-1] > 0 ) LMG7AR(m_iunit[14-1],MXITER,&m_iGrid)
	if ( m_iunit[42-1] > 0 ) GMG7AR(&m_iunit[42-1],
										m_mxiter,
										&m_iGrid,
										m_iadampgmg,
										m_iunitmhc,
										m_rClose,
										m_iiter,    
										m_hClose,
										m_dampgmg,  
										m_ioutgmg,  
										m_ism,      
										m_isc,      
										m_chglimit, 
										m_dup,      
										m_dlow,
										m_gmgid);     


//	  if ( m_iunit[59-1] > 0 ) PCGN2AR(m_iunit[59-1],IFREFM,MXITER,&m_iGrid);
//	  if ( m_iunit[50-1] > 0 ) GWF2MNW27AR(m_iunit[50-1],&m_iGrid);
//	  if ( m_iunit[51-1] > 0 ) GWF2MNW2I7AR(m_iunit[51-1],m_iunit[50-1],&m_iGrid);
//	  if ( m_iunit[52-1] > 0 ) GWF2MNW17AR(m_iunit[52-1],m_iunit[9-1], m_iunit[10-1],0,m_iunit[13-1],m_iunit[42-1],m_iunit[59-1],fileName,&m_iGrid);
//	  if ( m_iunit[57-1] > 0 ) GWF2SWT7AR(m_iunit[57-1],&m_iGrid);
//	  if ( m_iunit[43-1] > 0 ) GWF2HYD7BAS7AR(m_iunit[43-1],&m_iGrid);
//	  if ( m_iunit[43-1] > 0 && m_iunit[19-1] > 0 ) GWF2HYD7IBS7AR(m_iunit[43-1],&m_iGrid);
//	  if ( m_iunit[43-1] > 0 && m_iunit[54-1] > 0 ) GWF2HYD7SUB7AR(m_iunit[43-1],&m_iGrid);
//	  if ( m_iunit[43-1] > 0 && m_iunit[18-1] > 0 ) GWF2HYD7STR7AR(m_iunit[43-1],&m_iGrid);
//	  if ( m_iunit[43-1] > 0 && m_iunit[44-1] > 0 ) GWF2HYD7SFR7AR(m_iunit[43-1],&m_iGrid);
//	  if ( m_iunit[49-1] > 0 ) LMT7BAS7AR(&m_inUnit,locCUnit,&m_iGrid);

//	  Observation allocate and read
//	  OBS2BAS7AR(&m_iunit[28-1],&m_iGrid);
//	  if ( m_iunit[33-1] > 0 ) OBS2DRN7AR(m_iunit[33-1],m_iunit[3-1],&m_iGrid);
//	  if ( m_iunit[34-1] > 0 ) OBS2RIV7AR(m_iunit[34-1],m_iunit[4-1],&m_iGrid);
//	  if ( m_iunit[35-1] > 0 ) OBS2GHB7AR(m_iunit[35-1],m_iunit[7-1],&m_iGrid);
//	  if ( m_iunit[36-1] > 0 ) OBS2STR7AR(m_iunit[36-1],m_iunit[18-1],&m_iGrid);
//	  if ( m_iunit[38-1] > 0 ) OBS2CHD7AR(m_iunit[38-1],&m_iGrid);

	msg.Format( "ModFlowAdapter::InitRun successfull" );
   
	Report::LogMsg( msg, RT_INFO );

return TRUE;
}

BOOL ModflowAdapter::Run( FlowContext *pFlowContext )
	{

   double *tmpHNEW  =  static_cast<double*>(m_hNew);
   double *tmpHOLD  =  static_cast<double*>(m_hOld);
   double *tmpHNEWLAST  =  static_cast<double*>(m_hNewLast);
   int *tmpIBOUND  =  static_cast<int*>(m_iBound);
   float *tmpCR  =  static_cast<float*>(m_cr);
   float *tmpCC  =  static_cast<float*>(m_cc);
   float *tmpCV  =  static_cast<float*>(m_cv);
   float *tmpHCOF  =  static_cast<float*>(m_hcof);
   float *tmpRHS  =  static_cast<float*>(m_rhs);
   double *tmpV  =  static_cast<double*>(m_v);
   double *tmpSS  =  static_cast<double*>(m_ss);
   double *tmpP  =  static_cast<double*>(m_p);
   float *tmpCD  =  static_cast<float*>(m_cd);
   double *tmpHPCG  =  static_cast<double*>(m_hpcg);
   float *tmpRES  =  static_cast<float*>(m_res);
   float *tmpHCSV  =  static_cast<float*>(m_hcsv);

   int N = (23 + (31-1)*66 + (1-1)*46332)-1;
   double hnewtest  = *(tmpHNEW+N);
   double holdtest  = *(tmpHOLD+N);
   int    iboundtest   = *(tmpIBOUND+N);
   float  crtest     = *(tmpCR+N);
   float  cctest     = *(tmpCC+N);
   float  cvtest     = *(tmpCV+N);
   float  hcoftest   = *(tmpHCOF+N);
   float  rhstest    = *(tmpRHS+N);
   double vtest     = *(tmpV+N);
   double sstest    = *(tmpSS+N);
   double ptest     = *(tmpP+N);
   float  cdtest     = *(tmpCD+N);
   double hpcgtest  = *(tmpHPCG+N);
   float  restest    = *(tmpRES+N);
   float  hcsvtest   = *(tmpHCSV+N);

   float time_step = pFlowContext->timeStep;
   float time_time = pFlowContext->time;
   int time_doy = pFlowContext->dayOfYear;

	char *TEXT1="SOLVING FOR HEAD";
	
	char *TEXT2="                ";
//------SIMULATE EACH STRESS PERIOD.
//      DO 100 KPER = 1, NPER
	  for(int locKPER=1;locKPER < m_nper[0]+1;locKPER++)
		{
        int *locKKPER = &locKPER;
        m_iss = &m_issFlag[locKPER-1];
        GWF2BAS7ST(locKKPER,&m_iGrid);
//        if ( m_iunit[19-1] > 0 ) GWF2IBS7ST(locKKPER,&m_iGrid);
//        if ( m_iunit[54-1] > 0 ) GWF2SUB7ST(locKKPER,&m_iGrid);
//        if ( m_iunit[57-1] > 0 ) GWF2SWT7ST(locKKPER,&m_iGrid);

//-----READ AND PREPARE INFORMATION FOR STRESS PERIOD.
//----------READ USING PACKAGE READ AND PREPARE MODULES.
        // only read the input data file on the very first call
         if ( m_iunit[2-1] > 0 )  GWF2WEL7RP(&m_iunit[2-1],&m_iGrid,m_nWells);//************************************************************************
         if ( m_iunit[3-1] > 0 )  GWF2DRN7RP(&m_iunit[3-1],&m_iGrid,m_nDrains);//************************************************************************
         if ( m_iunit[4-1] > 0 )  GWF2RIV7RP(&m_iunit[4-1],&m_iGrid,m_nRivers);//************************************************************************
//         if ( m_iunit[5-1] > 0 )  GWF2EVT7RP(m_iunit[5-1],&m_iGrid);
//         if ( m_iunit[7-1] > 0 )  GWF2GHB7RP(m_iunit[7-1],&m_iGrid);

         for ( int r = 0; r < m_row[0]; r++ )
            {
            for ( int c = 0; c < m_col[0]; c++ )
               {
               int ptrIndex =  ( r * m_col[0] ) + c;
               
               *( (float*) m_recharge +ptrIndex) = (float) r;
               }
            }

         UpdateGrid( pFlowContext, m_pPolyGridLkUp, (float *) m_recharge );

         if ( m_iunit[8-1] > 0 )  GWF2RCH7RP( &m_iunit[8-1], &m_iGrid, m_iNrecharge, m_recharge );//************************************************************************
//         if ( m_iunit[17-1] > 0 ) GWF2RES7RP(m_iunit[17-1],&m_iGrid);
//         if ( m_iunit[18-1] > 0 ) GWF2STR7RP(m_iunit[18-1],&m_iGrid);
//         if ( m_iunit[43-1] > 0 && m_iunit[18-1] > 0 ) GWF2HYD7STR7RP(m_iunit[43-1],locKKPER,&m_iGrid);
         if ( m_iunit[20-1] > 0 ) GWF2CHD7RP(&m_iunit[20-1],&m_iGrid,m_iBound,m_nCHDS);
//         if ( m_iunit[44-1] > 0 ) GWF2SFR7RP(m_iunit[44-1],m_iunit[15-1], m_iunit[22-1],locKKPER,KKSTP,NSOL,IOUTS,m_iunit[1-1],m_iunit[23-1],m_iunit[37-1],&m_iGrid);
//         if ( m_iunit[43-1] > 0 && m_iunit[44-1] > 0 ) GWF2HYD7SFR7RP(m_iunit[43-1],locKKPER,&m_iGrid);
//         if ( m_iunit[55-1] > 0 ) GWF2UZF1RP(m_iunit[55-1],locKKPER,m_iunit[44-1],&m_iGrid);
//         if ( m_iunit[22-1] > 0 ) GWF2LAK7RP(m_iunit[22-1],m_iunit[1-1],m_iunit[15-1],m_iunit[23-1],m_iunit[37-1],m_iunit[44-1],m_iunit[55-1],locKKPER,NSOL,IOUTS,&m_iGrid);
//         if ( m_iunit[46-1] > 0 && locKKPER == 1) GWF2GAG7RP(m_iunit[15-1],m_iunit[22-1],m_iunit[55-1],NSOL,&m_iGrid);
//         if ( m_iunit[39-1] > 0 ) GWF2ETS7RP(m_iunit[39-1],&m_iGrid);
//         if ( m_iunit[40-1] > 0 ) GWF2DRT7RP(m_iunit[40-1],&m_iGrid);
//         if ( m_iunit[50-1] > 0 ) GWF2MNW27RP(m_iunit[50-1],locKKPER,m_iunit[9-1],m_iunit[10-1],0,m_iunit[13-1],m_iunit[42-1],m_iunit[59-1],0,&m_iGrid);
//         if ( m_iunit[51-1] > 0 && locKKPER == 1) GWF2MNW2I7RP(m_iunit[51-1],0,&m_iGrid);
//         if ( m_iunit[52-1] > 0 ) GWF2MNW17RP(m_iunit[52-1],m_iunit[1-1],m_iunit[23-1],m_iunit[37-1],locKKPER,&m_iGrid);
       
//-----SIMULATE EACH TIME STEP.
//        DO 90 KSTP = 1, NSTP(KPER)
		for(int locKSTP=1; locKSTP < m_nstp[0]+1;locKSTP++)
		  {
			int *locKKSTP = &locKSTP;

//----CALCULATE TIME STEP LENGTH. SET HOLD=HNEW.
          GWF2BAS7AD(locKKPER,locKKSTP,&m_iGrid,m_hNew,m_hOld);
          hnewtest=*(tmpHNEW+N);
          holdtest  = *(tmpHOLD+N);

          if ( m_iunit[20-1] > 0 ) GWF2CHD7AD(locKKPER,&m_iGrid,m_hNew,m_hOld);
          hnewtest=*(tmpHNEW+N);
//          if ( m_iunit[1-1] > 0 )  GWF2BCF7AD(locKKPER,&m_iGrid);
//          if ( m_iunit[17-1] > 0 ) GWF2RES7AD(locKKSTP,locKKPER,&m_iGrid);
          if ( m_iunit[23-1] > 0 ) GWF2LPF7AD(locKKPER,&m_iGrid,m_hOld);//************************************************************************
//          if ( m_iunit[37-1] > 0 ) GWF2HUF7AD(locKKPER,&m_iGrid);
//          if ( m_iunit[16-1] > 0 ) GWF2FHB7AD(&m_iGrid);
//          if ( m_iunit[22-1] > 0 ) GWF2LAK7AD(locKKPER,locKKSTP,m_iunit[15-1],&m_iGrid);
//          if ( m_iunit[44-1] > 0 && NUMTAB > 0  ) GWF2SFR7AD(m_iunit[22-1]);
		 //  if ( m_iunit[50-1] > 0 ) ;
			//{
   //         if (m_iunit[1-1] > 0 ) GWF2MNW27BCF(locKPER,&m_iGrid);
			//else if (m_iunit[23-1] > 0 )  GWF2MNW27LPF(locKPER,&m_iGrid);//************************************************************************
			//else if ( m_iunit[37-1] > 0 ) GWF2MNW27HUF(locKPER,&m_iGrid);
			//else
			//	{
			//	CString msg( "MNW1 and MNW2 cannot both be active in the same simulation" );  
			//	Report::ErrorMsg( msg );
			//	return false;
			//	}
   //         GWF2MNW27AD(locKKSTP,locKKPER,&m_iGrid);
			//}

//          if ( m_iunit[52-1] > 0 ) GWF2MNW17AD(m_iunit[1-1],m_iunit[23-1],m_iunit[37-1],&m_iGrid);

//---------INDICATE IN PRINTOUT THAT SOLUTION IS FOR HEADS
          UMESPR(TEXT1,TEXT2,&m_iout[0]);
          //WRITE(*,25)locKPER,KSTPFORMAT(' Solving:  Stress period: ',i5,4x,'Time step: ',i5,4x,'Ground-Water Flow Eqn.')

//----ITERATIVELY FORMULATE AND SOLVE THE FLOW EQUATIONS.
//          DO 30 KITER = 1, MXITER
			for(int locKITER=1;locKITER <= m_mxiter[0];locKITER++)
			{
            int * locKKITER = &locKITER;

//---FORMULATE THE FINITE DIFFERENCE EQUATIONS.
            GWF2BAS7FM(&m_iGrid,m_hcof,m_rhs);
//            if ( m_iunit[1-1] > 0 )  GWF2BCF7FM(locKKITER,locKKSTP,locKKPER,&m_iGrid);
            if ( m_iunit[23-1] > 0 ) GWF2LPF7FM(locKKITER,locKKSTP,locKKPER,&m_iGrid,m_cc,m_cv,m_cr,m_hcof,m_hOld,m_rhs,m_hNew,m_iBound);//************************************************************************
            hnewtest=*(tmpHNEW+N);
             holdtest  = *(tmpHOLD+N);
            iboundtest   = *(tmpIBOUND+N);
            crtest     = *(tmpCR+N);
            cctest     = *(tmpCC+N);
            cvtest     = *(tmpCV+N);
            hcoftest   = *(tmpHCOF+N);
            rhstest    = *(tmpRHS+N);
            vtest     = *(tmpV+N);
            sstest    = *(tmpSS+N);
            ptest     = *(tmpP+N);
            cdtest     = *(tmpCD+N);
            hpcgtest  = *(tmpHPCG+N);
            restest    = *(tmpRES+N);
            hcsvtest   = *(tmpHCSV+N);
            //            if ( m_iunit[37-1] > 0 ) GWF2HUF7FM(locKKITER,locKKSTP,locKKPER,m_iunit[47-1],&m_iGrid);
//            if ( m_iunit[21-1] > 0 ) GWF2HFB7FM(&m_iGrid);
            if ( m_iunit[2-1] > 0 )  GWF2WEL7FM(&m_iGrid,m_rhs);//************************************************************************
            if ( m_iunit[3-1] > 0 )  GWF2DRN7FM(&m_iGrid,m_rhs,m_hcof,m_hNew);//************************************************************************
            hnewtest=*(tmpHNEW+N);
            if ( m_iunit[4-1] > 0 )  GWF2RIV7FM(&m_iGrid,m_hcof,m_rhs,m_hNew);//************************************************************************
            hnewtest=*(tmpHNEW+N);
			//if ( m_iunit[5-1] > 0 )
			//	{
			//	if ( m_iunit[22-1] > 0 && NEVTOP == 3) GWF2LAK7ST(0,&m_iGrid);
			//	GWF2EVT7FM(&m_iGrid);
			//	if ( m_iunit[22-1] > 0 && NEVTOP == 3) GWF2LAK7ST(1,&m_iGrid);
			//	}
   //         
			//if ( m_iunit[7-1] > 0 ) GWF2GHB7FM(&m_iGrid);
            
			if ( m_iunit[8-1] > 0 )//************************************************************************ 
				{
				int locvar1 = 0;
				int locvar2 = 1;

				if ( m_iunit[22-1] > 0 && m_nrchop[0] == 3) GWF2LAK7ST(&locvar1,&m_iGrid);
				
				GWF2RCH7FM(&m_iGrid,m_rhs);

				if ( m_iunit[22-1] > 0 && m_nrchop[0] == 3) GWF2LAK7ST(&locvar2,&m_iGrid);
				}
            
//			if ( m_iunit[16-1] > 0 ) GWF2FHB7FM(&m_iGrid);
//            if ( m_iunit[17-1] > 0 ) GWF2RES7FM(&m_iGrid);
//            if ( m_iunit[18-1] > 0 ) GWF2STR7FM(&m_iGrid);
//            if ( m_iunit[19-1] > 0 ) GWF2IBS7FM(locKKPER,&m_iGrid);
//            if ( m_iunit[39-1] > 0 ) GWF2ETS7FM(&m_iGrid);
//            if ( m_iunit[40-1] > 0 ) GWF2DRT7FM(&m_iGrid);
//            if ( m_iunit[55-1] > 0 ) GWF2UZF1FM(locKKPER,locKKSTP,locKKITER,m_iunit[44-1],m_iunit[22-1],m_iunit[58-1],&m_iGrid);
//            if ( m_iunit[44-1] > 0 ) GWF2SFR7FM(locKKITER,locKKPER,locKKSTP,m_iunit[22-1],m_iunit[8-1],m_iunit[55-1],&m_iGrid);
//            if ( m_iunit[22-1] > 0 ) GWF2LAK7FM(locKKITER,locKKPER,locKKSTP,m_iunit[44-1],m_iunit[55-1],&m_iGrid);
            
			//if ( m_iunit[50-1] > 0 ) 
			//	{
			//	if (m_iunit[1-1] > 0 ) 
			//		{
			//		GWF2MNW27BCF(locKPER,&m_iGrid);
			//	else if ( m_iunit[23-1] > 0 )  GWF2MNW27LPF(locKPER,&m_iGrid);//************************************************************************
			//	else if ( m_iunit[37-1] > 0 ) GWF2MNW27HUF(locKPER,&m_iGrid);
			//		}
			//	GWF2MNW27FM(locKKITER,locKKSTP,locKKPER,&m_iGrid);
			//	}
            
//			if ( m_iunit[52-1] > 0 ) GWF2MNW17FM(locKKITER,m_iunit[1-1],m_iunit[23-1],m_iunit[37-1],&m_iGrid);
//            if ( m_iunit[54-1] > 0 ) GWF2SUB7FM(locKKPER,locKKITER,m_iunit[9-1],&m_iGrid);
//            if ( m_iunit[57-1] > 0 ) GWF2SWT7FM(locKKPER,&m_iGrid);

//---MAKE ONE CUT AT AN APPROXIMATE SOLUTION.
         
         int locIERR1 = 0;
         m_ierr = &locIERR1;
   //         if ( m_iunit[9-1] > 0 ) 
			//	{
   //             SIP7PNT(&m_iGrid);
   //             SIP7AP(HNEW,IBOUND,CR,CC,CV,HCOF,RHS,EL,FL,GL, V,
			//		W,HDCG,LRCH,NPARM,locKKITER,HCLOSE,ACCL,m_icnvg,locKKSTP,locKKPER,
			//		IPCALC,IPRSIP,MXITER,NSTP(locKKPER),NCOL,NROW,NLAY,NODES,IOUT,0,IERR);
			//	}

			//if ( m_iunit[10-1] > 0 ) 
			//	{
   //             DE47PNT(&m_iGrid);
   //             DE47AP(HNEW,IBOUND,AU,AL,IUPPNT,IEQPNT,D4B,MXUP,
			//		MXLOW,MXEQ,MXBW,CR,CC,CV,HCOF,RHS,ACCLDE4,locKITER,
			//		ITMX,MXITER,NITERDE4,HCLOSEDE4,IPRD4,m_icnvg,NCOL,
			//		NROW,NLAY,IOUT,LRCHDE4,HDCGDE4,IFREQ,locKKSTP,locKKPER,
			//		DELT,NSTP(locKKPER),ID4DIR,ID4DIM,MUTD4,
			//		DELTL,NBWL,NUPL,NLOWL,NLOW,NEQ,NUP,NBW,IERR);
			//	}
   //         
            
          if ( m_iunit[13-1] > 0 ) 
				{
                PCG7PNT(&m_iGrid,m_iter1,m_npcond,m_nbpol,
                        m_iprpcg,m_mutpcg,m_niter, m_hClose,m_rClose,
                        m_relax,m_dampSS,m_dampTR,m_ihcofadd);
                //env_HNEW = *(tmpHNEW+38306);
                
                for(int i=0;i < m_nodes[0];i++)
                  {
                  *(HNEW1d+i)=  *(tmpHNEW+i);
                  *(IBOUND1d+i)=*(tmpIBOUND+i);
                  *(CR1d+i)=    *(tmpCR+i);
                  *(CC1d+i)=    *(tmpCC+i);
                  *(CV1d+i)=    *(tmpCV+i);
                  *(HCOF1d+i)=  *(tmpHCOF+i);
                  *(RHS1d+i)=   *(tmpRHS+i);
                  *(V1d+i)=     *(tmpV+i);
                  *(SS1d+i)=    *(tmpSS+i);
                  *(P1d+i)=     *(tmpP+i);
                  *(CD1d+i)=    *(tmpCD+i);
                  *(HPCG1d+i)=  *(tmpHPCG+i);
                  *(RES1d+i)=   *(tmpRES+i);
                  *(HCSV1d+i)=  *(tmpHCSV+i);
                  }
                
                double env_HNEW1  = *(HNEW1d+N);
                int    env_ibound = *(IBOUND1d+N);
                float  env_cr     = *(CR1d+N);
                float  env_cc     = *(CC1d+N);
                float  env_cv     = *(CV1d+N);
                float  env_hcof   = *(HCOF1d+N);
                float  env_rhs    = *(RHS1d+N);
                double env_v      = *(V1d+N);
                double env_ss     = *(SS1d+N);
                double env_p      = *(P1d+N);
                float  env_cd     = *(CD1d+N);
                double env_hpcg   = *(HPCG1d+N);
                float  env_res    = *(RES1d+N);
                float  env_hcsv   = *(HCSV1d+N);

                               
                //double (&HNEW1d)[46332] = reinterpret_cast<double (&)[46332]>(tmpHNEW);
                PCG7AP(HNEW1d,IBOUND1d,CR1d,CC1d,
                      CV1d,HCOF1d,RHS1d,
                      V1d,SS1d,P1d,CD1d,m_hchg,
                      m_lhch,m_rchg,m_lrch,locKKITER, 
                      m_niter,m_hClose,m_rClose,m_icnvg,
                      locKKSTP,locKKPER,m_iprpcg,m_mxiter,m_iter1,
                      m_npcond,m_nbpol,m_nstp,m_col, 
                      m_row,m_lay,m_nodes,m_relax, 
                      m_iout,m_mutpcg,m_it1,m_dampSS,
                      RES1d,HCSV1d,m_ierr,HPCG1d, 
                      m_dampTR,m_iss,m_hdry,m_ihcofadd);
                
                env_HNEW1 = *(HNEW1d+N);
                env_HNEW1  = *(HNEW1d+N);
                env_ibound = *(IBOUND1d+N);
                env_cr     = *(CR1d+N);
                env_cc     = *(CC1d+N);
                env_cv     = *(CV1d+N);
                env_hcof   = *(HCOF1d+N);
                env_rhs    = *(RHS1d+N);
                env_v      = *(V1d+N);
                env_ss     = *(SS1d+N);
                env_p      = *(P1d+N);
                env_cd     = *(CD1d+N);
                env_hpcg   = *(HPCG1d+N);
                env_res    = *(RES1d+N);
                env_hcsv   = *(HCSV1d+N);       
                
                tmpHNEW = HNEW1d;
                tmpIBOUND = IBOUND1d;
                tmpCR = CR1d;
                tmpCC = CC1d;
                tmpCV = CV1d;
                tmpHCOF = HCOF1d;
                tmpRHS = RHS1d;
                tmpCD = CD1d;
                tmpRES = RES1d;
                tmpHCSV = HCSV1d;
                /*PCG7AP(m_hNew,m_iBound,m_cr,m_cc,
                        m_cv,m_hcof,m_rhs,
                        m_v,m_ss,m_p,m_cd,m_hchg,
                        m_lhch,m_rchg,m_lrch,locKKITER, 
                        m_niter,m_hClose,m_rClose,m_icnvg,
                        locKKSTP,locKKPER,m_iprpcg,m_mxiter,m_iter1,
                        m_npcond,m_nbpol,m_nstp,envcol, 
                        envrow,envlay,m_nodes,m_relax, 
                        m_iout,m_mutpcg,m_it1,m_dampSS,
                        m_res,m_hcsv,m_ierr,m_hpcg, 
                        m_dampTR,m_iss,m_hdry,m_ihcofadd);*/

                env_HNEW1 = *(HNEW1d+38306);
				}

//c            IF (m_iunit[14-1] > 0 ) THEN
//c              LMG7PNT(&m_iGrid)
//c              LMG7AP(HNEW,IBOUND,CR,CC,CV,HCOF,RHS,A,IA,JA,U1,
//c     1           FRHS,IG,ISIZ1,ISIZ2,ISIZ3,ISIZ4,locKKITER,BCLOSE,DAMPLMG,
//c     2           m_icnvg,locKKSTP,locKKPER,MXITER,MXCYC,NCOL,NROW,NLAY,NODES,
//c     3           HNOFLO,IOUT,IOUTAMG,ICG,IADAMPLMG,DUPLMG,DLOWLMG)
//c            END IF

           if ( m_iunit[42-1] > 0 )//************************************************************************ 
				{
                   GMG7PNT(&m_iGrid,m_iiter,m_iadampgmg,m_ioutgmg,
							  m_siter,m_tsiter,m_gmgid,m_rClose,m_hClose,
							  m_dampgmg,m_iunitmhc,m_dup,m_dlow,m_chglimit,
							  m_bigheadchg,m_hNewLast);

				       hnewtest=*(tmpHNEW+N);
                    holdtest  = *(tmpHOLD+N);
                   iboundtest   = *(tmpIBOUND+N);
                   crtest     = *(tmpCR+N);
                   cctest     = *(tmpCC+N);
                   cvtest     = *(tmpCV+N);
                   hcoftest   = *(tmpHCOF+N);
                   rhstest    = *(tmpRHS+N);
                   vtest     = *(tmpV+N);
                   sstest    = *(tmpSS+N);
                   ptest     = *(tmpP+N);
                   cdtest     = *(tmpCD+N);
                   hpcgtest  = *(tmpHPCG+N);
                   restest    = *(tmpRES+N);
                   hcsvtest   = *(tmpHCSV+N);
                   
                   GMG7AP(m_hNew,m_rhs,m_cr,m_cc,m_cv,m_hcof,m_hnoflo,m_iBound,
                              m_iiter,m_mxiter,m_rClose,m_hClose,
                              locKKITER,locKKSTP,locKKPER,&m_col[0],&m_row[0],&m_lay[0],m_icnvg,
                              m_siter,m_tsiter,m_dampgmg,m_iadampgmg,m_ioutgmg,
                              &m_iout[0],m_gmgid,
                              m_iunitmhc,m_dup,m_dlow,m_chglimit,
                              m_bigheadchg,m_hNewLast,m_hNew);
                   
                   hnewtest=*(tmpHNEW+N);
                    holdtest  = *(tmpHOLD+N);
                   iboundtest   = *(tmpIBOUND+N);
                   crtest     = *(tmpCR+N);
                   cctest     = *(tmpCC+N);
                   cvtest     = *(tmpCV+N);
                   hcoftest   = *(tmpHCOF+N);
                   rhstest    = *(tmpRHS+N);
                   vtest     = *(tmpV+N);
                   sstest    = *(tmpSS+N);
                   ptest     = *(tmpP+N);
                   cdtest     = *(tmpCD+N);
                   hpcgtest  = *(tmpHPCG+N);
                   restest    = *(tmpRES+N);
                   hcsvtest   = *(tmpHCSV+N);
                   
				}
            
//			if (m_iunit[59-1] > 0 ) 
//				{
//				PCGN2AP(HNEW,RHS,CR,CC,CV,HCOF,IBOUND,
//                   locKKITER,locKKSTP,locKKPER,m_icnvg,HNOFLO,&m_iGrid);
//				}      
//            
////			IF(IERR == 1) USTOP(' ')
//			IF(IERR == 1) return false;

//---IF CONVERGENCE CRITERION HAS BEEN MET STOP ITERATING.
			if ( m_icnvg[0] == 1 ) break;
//            IF (m_icnvg == 1) GOTO 33
//  30      CONTINUE
		  }
        hnewtest=*(tmpHNEW+N);
		  int locKITER = m_mxiter[0];

//  33      CONTINUE

//----DETERMINE WHICH OUTPUT IS NEEDED.
          GWF2BAS7OC(locKKSTP,locKKPER,m_icnvg,&m_iunit[12-1],&m_iGrid);//************************************************************************

//----CALCULATE BUDGET TERMS. SAVE CELL-BY-CELL FLOW TERMS.
          int locmsum = 1;
		    m_msumin = &locmsum;

//          if (m_iunit[1-1] > 0 );
//			{
//            GWF2BCF7BDS(locKKSTP,locKKPER,&m_iGrid);
//            GWF2BCF7BDCH(locKKSTP,locKKPER,&m_iGrid);
//            IBDRET=0;
//            IC1=1;
//            IC2=NCOL;
//            IR1=1;
//            IR2=NROW;
//            IL1=1;
//            IL2=NLAY;
//            //DO 37 IDIR = 1, 3
//            //  GWF2BCF7BDADJ(locKKSTP,locKKPER,IDIR,IBDRET,IC1,IC2,IR1,IR2,IL1,IL2,&m_iGrid)
////   37       CONTINUE

			//for ( IDIR=0; IDIR=3-1; IDIR++) GWF2BCF7BDADJ(locKKSTP,locKKPER,IDIR,IBDRET,IC1,IC2,IR1,IR2,IL1,IL2,&m_iGrid);

			//}

          if ( m_iunit[23-1] > 0 )//************************************************************************ 
			{
            //MODFLOW_GLOBAL_mp_HNEWFN myHNEW = m_HNEW;
			
			GWF2LPF7BDS(locKKSTP,locKKPER,&m_iGrid,m_msumin,m_hNew,m_hOld,m_iBound);
            GWF2LPF7BDCH(locKKSTP,locKKPER,&m_iGrid,m_hNew,m_cc,m_cv,m_cr);
            int IBDRET=0;
            int IC1=1;
            int IC2=m_col[0];
            int IR1=1;
            int IR2=m_row[0];
            int IL1=1;
            int IL2=m_lay[0];
            //DO 157 IDIR=1,3
            //  GWF2LPF7BDADJ(locKKSTP,locKKPER,IDIR,IBDRET,IC1,IC2,IR1,IR2,IL1,IL2,&m_iGrid)
//157         CONTINUE
			for ( int IDIR=1; IDIR <= 3; IDIR++) GWF2LPF7BDADJ(locKKSTP,locKKPER,&IDIR,&IBDRET,&IC1,&IC2,&IR1,&IR2,&IL1,&IL2,&m_iGrid,m_hNew,m_cc,m_cv,m_cr);
			
			}

//          if ( m_iunit[37-1] > 0 ) 
//			{
//            GWF2HUF7BDS(locKKSTP,locKKPER,&m_iGrid);
//            GWF2HUF7BDCH(locKKSTP,locKKPER,m_iunit[47-1],&m_iGrid);
//            IBDRET=0;
//            IC1=1;
//            IC2=NCOL;
//            IR1=1;
//            IR2=NROW;
//            IL1=1;
//            IL2=NLAY;
////            DO 159 IDIR=1,3
////              GWF2HUF7BDADJ(locKKSTP,locKKPER,IDIR,IBDRET,IC1,IC2,IR1,IR2,IL1,IL2,m_iunit[47-1],&m_iGrid)
////159         CONTINUE
//		  for ( IDIR=1; IDIR=3; IDIR++ ) GWF2HUF7BDADJ(locKKSTP,locKKPER,IDIR,IBDRET,IC1,IC2,IR1,IR2,IL1,IL2,m_iunit[47-1],&m_iGrid);
//
//			}

          if ( m_iunit[2-1] > 0 ) GWF2WEL7BD(locKKSTP,locKKPER,&m_iGrid);//************************************************************************
          if ( m_iunit[3-1] > 0 ) GWF2DRN7BD(locKKSTP,locKKPER,&m_iGrid,m_hNew);//************************************************************************
          if ( m_iunit[4-1] > 0 ) GWF2RIV7BD(locKKSTP,locKKPER,&m_iGrid,m_hNew);//************************************************************************f

   //       if ( m_iunit[5-1] > 0 ) 
			//{
   //          if ( m_iunit[22-1] > 0 && NEVTOP == 3) GWF2LAK7ST(0,&m_iGrid);
   //          GWF2EVT7BD(locKKSTP,locKKPER,&m_iGrid);
   //          if ( m_iunit[22-1] > 0 && NEVTOP == 3) GWF2LAK7ST(1,&m_iGrid);
			//}

//          if ( m_iunit[7-1] > 0 ) GWF2GHB7BD(locKKSTP,locKKPER,&m_iGrid);

          if ( m_iunit[8-1] > 0 )//************************************************************************ 
			{
			 int locvar3 = 0;
			 int locvar4 = 1;
             if ( m_iunit[22-1] > 0 && m_nrchop[0] == 3) GWF2LAK7ST(&locvar3,&m_iGrid);
             
			 GWF2RCH7BD(locKKSTP,locKKPER,&m_iGrid);
             
			 if ( m_iunit[22-1] > 0 && m_nrchop[0] == 3) GWF2LAK7ST(&locvar4,&m_iGrid);
			}

//          if ( m_iunit[16-1] > 0 ) GWF2FHB7BD(locKKSTP,locKKPER,&m_iGrid);
//          if ( m_iunit[17-1] > 0 ) GWF2RES7BD(locKKSTP,locKKPER,&m_iGrid);
//          if ( m_iunit[18-1] > 0 ) GWF2STR7BD(locKKSTP,locKKPER,&m_iGrid);
//          if ( m_iunit[19-1] > 0 ) GWF2IBS7BD(locKKSTP,locKKPER,&m_iGrid);
//          if ( m_iunit[39-1] > 0 ) GWF2ETS7BD(locKKSTP,locKKPER,&m_iGrid);
//          if ( m_iunit[40-1] > 0 ) GWF2DRT7BD(locKKSTP,locKKPER,&m_iGrid);
//          if ( m_iunit[44-1] > 0 ) GWF2SFR7BD(locKKSTP,locKKPER,m_iunit[15-1],m_iunit[22-1],m_iunit[46-1],m_iunit[55-1],NSOL,m_iunit[8-1],&m_iGrid);
//          if ( m_iunit[55-1] > 0 ) GWF2UZF1BD(locKKSTP,locKKPER,m_iunit[22-1],m_iunit[44-1],&m_iGrid);
//          if ( m_iunit[22-1] > 0 ) GWF2LAK7BD(locKKSTP,locKKPER,m_iunit[15-1],m_iunit[46-1],m_iunit[44-1],m_iunit[55-1],NSOL,&m_iGrid);
//          if ( m_iunit[50-1] > 0 ) GWF2MNW27BD(locKKSTP,locKKPER,&m_iGrid);
//          if ( m_iunit[52-1] > 0 ) GWF2MNW17BD(NSTP(locKPER-1],locKKSTP,locKKPER,&m_iGrid);
//          if ( m_iunit[54-1] > 0 ) GWF2SUB7BD(locKKSTP,locKKPER,&m_iGrid);
//          if ( m_iunit[57-1] > 0 ) GWF2SWT7BD(locKKSTP,locKKPER,&m_iGrid);

//----CALL LINK-MT3DMS SUBROUTINES TO SAVE FLOW-TRANSPORT LINK FILE
//----FOR USE BY MT3DMS FOR TRANSPORT SIMULATION
//
//          if ( m_iunit[49-1] > 0 ) LMT7BD(locKKSTP,locKKPER,&m_iGrid);
                              

//  Observation and hydrograph simulated equivalents
//          OBS2BAS7SE(m_iunit[28-1],&m_iGrid);
//          if ( m_iunit[33-1] > 0 ) OBS2DRN7SE(&m_iGrid);
//          if ( m_iunit[34-1] > 0 ) OBS2RIV7SE(&m_iGrid);
//         if ( m_iunit[35-1] > 0 ) OBS2GHB7SE(&m_iGrid);
//          if ( m_iunit[36-1] > 0 ) OBS2STR7SE(&m_iGrid);
//          if ( m_iunit[38-1] > 0 ) OBS2CHD7SE(locKKPER,&m_iGrid);
//          if ( m_iunit[43-1] > 0 ) GWF2HYD7BAS7SE(1,&m_iGrid);
//          if ( m_iunit[43-1] > 0 && m_iunit[19-1] > 0 ) GWF2HYD7IBS7SE(1,&m_iGrid);
//          if ( m_iunit[43-1] > 0 && m_iunit[54-1] > 0 ) GWF2HYD7SUB7SE(1,&m_iGrid);
//          if ( m_iunit[43-1] > 0 && m_iunit[18-1] > 0 ) GWF2HYD7STR7SE(1,&m_iGrid);
//          if ( m_iunit[43-1] > 0 && m_iunit[44-1] > 0 ) GWF2HYD7SFR7SE(1,&m_iGrid);

//---PRINT AND/OR SAVE DATA.
		  int locvar5 = 1;
        

       
          GWF2BAS7OT(locKKSTP,locKKPER,m_icnvg,&locvar5,&m_iGrid,locbudperc,m_budperc,m_hNew);
//          if ( m_iunit[19-1] > 0 ) GWF2IBS7OT(locKKSTP,locKKPER,m_iunit[19-1],&m_iGrid);

   //       if ( m_iunit[37-1] > 0 )
			//{
   //         if(IOHUFHDS != 0 || IOHUFFLWS != 0) GWF2HUF7OT(locKKSTP,locKKPER,m_icnvg,1,&m_iGrid);
			//}

   //       if ( m_iunit[51-1] != 0) GWF2MNW2I7OT(NSTP(locKKPER),locKKSTP,locKKPER,&m_iGrid);
   //       if ( m_iunit[54-1] > 0 ) GWF2SUB7OT(locKKSTP,locKKPER,m_iunit[54-1],&m_iGrid);
   //       if ( m_iunit[57-1] > 0 ) GWF2SWT7OT(locKKSTP,locKKPER,&m_iGrid);
   //       if ( m_iunit[43-1] > 0 ) GWF2HYD7BAS7OT(locKKSTP,locKKPER,&m_iGrid);

//---JUMP TO END OF PROGRAM IF CONVERGENCE WAS NOT ACHIEVED.

          if (m_icnvg == 0) 
			{
            m_ncverr=m_ncverr+1;
            //WRITE(IOUT,87) locbudperc
  // 87       FORMAT(1X,'FAILURE TO MEET SOLVER CONVERGENCE CRITERIA',/1X,'BUDGET PERCENT DISCREPANCY IS',F10.4)
			if(abs(m_budperc[0]) > m_stoper[0]) 
				{

				CString msg;
				msg.Format("ModflowAdapter: convergence not met for Envision timestep" );
				Report::ErrorMsg( msg );
				return FALSE;
				}
					
			}

//-----END OF TIME STEP (KSTP) AND STRESS PERIOD (locKPER) LOOPS
		}  //End time stop loop
//   90   CONTINUE
//  100 CONTINUE
	}  //End stress period loop
	// layer structures
    
	//POINT *pGridPtNdxs = NULL;

	//int totalcount   = 0;

	//float *GridPtProportions = NULL;

	//for (MapLayer::Iterator poly = m_pPolyMapLayer->Begin(); poly != m_pPolyMapLayer->End(); poly++)
	//	{
	//	     
	//	int 
 //        GridPtCnt    = 0, 
 //        MaxGridPtCnt = 0;
 //       
 //     // ensure arrays have sufficient space
	//	if((GridPtCnt = m_pPolyGridLkUp->GetGridPtCntForPoly(poly)) > MaxGridPtCnt) 
 //       {
 //        
	//	totalcount += GridPtCnt;
 //        
	//	MaxGridPtCnt = GridPtCnt;
	//		
	//	if(GridPtProportions)
	//			delete [] GridPtProportions;
	//	
	//	GridPtProportions = new float[MaxGridPtCnt];

	//		if(pGridPtNdxs)
	//			delete [] pGridPtNdxs;

	//	pGridPtNdxs = new POINT[MaxGridPtCnt];
	//	} 

	//	for( int i = 0; i < MaxGridPtCnt; i++ ) 
	//	{
	//		GridPtProportions[i] = 0;
	//		pGridPtNdxs[i].x = 0;
	//		pGridPtNdxs[i].y = 0;
	//	}

	//	m_pPolyGridLkUp->GetGridPtNdxsForPoly(poly,pGridPtNdxs);

	//	// get proportions for poly
	//	m_pPolyGridLkUp->GetGridPtProportionsForPoly(poly, GridPtProportions);

	//	}
	m_ncverr=0;
	msg.Format( "ModFlowAdapter::Run successfull" );
   
	Report::LogMsg( msg, RT_INFO );
	return TRUE;
////      if ( m_iunit[52-1] != 0) GWF2MNW17OT(&m_iGrid);
}
//------END OF SIMULATION
//-------SAVE RESTART RECORDS FOR SUB PACKAGE

BOOL ModflowAdapter::EndRun( FlowContext *pFlowContext )
	{
   
//	  if ( m_iunit[54-1] > 0 ) GWF2SUB7SV(&m_iGrid);

//  Observation output
//      if ( m_iunit[28-1] > 0 ) OBS2BAS7OT(m_iunit[28-1],&m_iGrid);
//      if ( m_iunit[33-1] > 0 ) OBS2DRN7OT(&m_iGrid);
//      if ( m_iunit[34-1] > 0 ) OBS2RIV7OT(&m_iGrid);
//      if ( m_iunit[35-1] > 0 ) OBS2GHB7OT(&m_iGrid);
//      if ( m_iunit[36-1] > 0 ) OBS2STR7OT(&m_iGrid);
//      if ( m_iunit[38-1] > 0 ) OBS2CHD7OT(&m_iGrid);
      int locvar6 = 1;
	  GLO1BAS6ET(m_iout,m_ibdt,&locvar6);

//------CLOSE FILES AND DEALLOCATE MEMORY.  GWF2BAS7DA MUST BE CALLED
//------LAST BECAUSE IT DEALLOCATES IUNIT.
      SGWF2BAS7PNT(&m_iGrid);
//      if ( m_iunit[1-1] > 0 )  GWF2BCF7DA(&m_iGrid);
      if ( m_iunit[2-1] > 0 )  GWF2WEL7DA(&m_iGrid);//************************************************************************
      if ( m_iunit[3-1] > 0 )  GWF2DRN7DA(&m_iGrid);//************************************************************************
      if ( m_iunit[4-1] > 0 )  GWF2RIV7DA(&m_iGrid);//************************************************************************
//      if ( m_iunit[5-1] > 0 )  GWF2EVT7DA(&m_iGrid);
//      if ( m_iunit[7-1] > 0 )  GWF2GHB7DA(&m_iGrid);
//      if ( m_iunit[8-1] > 0 )  GWF2RCH7DA(&m_iGrid);//************************************************************************
//      if ( m_iunit[9-1] > 0 )  SIP7DA(&m_iGrid);
//      if ( m_iunit[10-1] > 0 ) DE47DA(&m_iGrid);
//      if ( m_iunit[13-1] > 0 ) PCG7DA(&m_iGrid,m_iter1,m_npcond,m_nbpol,
//                        m_iprpcg,m_mutpcg,m_niter, m_hClose,m_rClose,
//                        m_relax,m_dampSS,m_dampTR,m_ihcofadd,m_v,m_ss,
//                        m_p,m_hpcg,m_cd,m_hcsv,m_lhch,m_hchg,
//                        m_lrch,m_rchg,m_it1);
//      if ( m_iunit[14-1] > 0 ) LMG7DA(&m_iGrid)
//      if ( m_iunit[16-1] > 0 ) GWF2FHB7DA(&m_iGrid);
//      if ( m_iunit[17-1] > 0 ) GWF2RES7DA(&m_iGrid);
//      if ( m_iunit[18-1] > 0 ) GWF2STR7DA(&m_iGrid);
//      if ( m_iunit[19-1] > 0 ) GWF2IBS7DA(&m_iGrid);
      if ( m_iunit[20-1] > 0 ) GWF2CHD7DA(&m_iGrid);
//      if ( m_iunit[21-1] > 0 ) GWF2HFB7DA(&m_iGrid);
//      if ( m_iunit[22-1] > 0 || m_iunit[44-1] > 0 ) CALL GWF2LAK7DA(m_iunit[22-1],&m_iGrid);
      if ( m_iunit[23-1] > 0 ) GWF2LPF7DA(&m_iGrid);//************************************************************************
      //if ( m_iunit[37-1] > 0 ) GWF2HUF7DA(&m_iGrid);
      //if ( m_iunit[39-1] > 0 ) GWF2ETS7DA(&m_iGrid);
      //if ( m_iunit[40-1] > 0 ) GWF2DRT7DA(&m_iGrid);
//      if ( m_iunit[42-1] > 0 ) GMG7DA(&m_iGrid);//************************************************************************
//      if ( m_iunit[59-1] > 0 ) PCGN2DA(&m_iGrid);
//      if ( m_iunit[44-1] > 0 ) GWF2SFR7DA(&m_iGrid);
//      if ( m_iunit[46-1] > 0 ) GWF2GAG7DA(&m_iGrid);
      //if ( m_iunit[50-1] > 0 ) GWF2MNW27DA(&m_iGrid);
      //if ( m_iunit[51-1] > 0 ) GWF2MNW2I7DA(&m_iGrid);
      //if ( m_iunit[52-1] > 0 ) GWF2MNW17DA(&m_iGrid);
      //if ( m_iunit[54-1] > 0 ) GWF2SUB7DA(&m_iGrid);
      //if ( m_iunit[55-1] > 0 ) GWF2UZF1DA(&m_iGrid);
      //if ( m_iunit[57-1] > 0 ) GWF2SWT7DA(&m_iGrid);
      OBS2BAS7DA(&m_iunit[28-1],&m_iGrid);
      //if ( m_iunit[33-1] > 0 ) OBS2DRN7DA(&m_iGrid);
      //if ( m_iunit[34-1] > 0 ) OBS2RIV7DA(&m_iGrid);
      //if ( m_iunit[35-1] > 0 ) OBS2GHB7DA(&m_iGrid);
      //if ( m_iunit[36-1] > 0 ) OBS2STR7DA(&m_iGrid);
      //if ( m_iunit[38-1] > 0 ) OBS2CHD7DA(&m_iGrid);
      //if ( m_iunit[43-1] > 0 ) GWF2HYD7DA(&m_iGrid);
      //if ( m_iunit[49-1] > 0 ) LMT7DA(&m_iGrid);
      GWF2BAS7DA(&m_iGrid);

//C10-----END OF PROGRAM.
      if (m_ncverr > 0 ) 
	  {
		  CString msg( "modflow: FAILED TO MEET SOLVER CONVERGENCE CRITERIA " );
     
          Report::ErrorMsg( msg );
      
		  
		  //WRITE(*,*) 'FAILED TO MEET SOLVER CONVERGENCE CRITERIA ',m_ncverr,' TIME(S)';
	  }
	  else 
	  {
		  msg.Format( "ModFlowAdapter::EndRun successfull" );
   
	     Report::LogMsg( msg, RT_INFO );
       
	     //WRITE(*,*) ' Normal termination of simulation'
	  }

   if(m_pPolyGridLkUp != NULL)
      {
      delete[] m_pPolyGridLkUp;
      }
 
//   delete [] IBDT            ; 
//   delete [] m_mxiter     ; 
//   delete [] m_iter1      ; 
//   delete [] m_iunit         ; 
//   delete [] mynper          ; 
//   delete [] m_nstp          ; 
//   delete [] m_iout          ; 
//   delete [] myncol          ; 
//   delete [] mynrow          ; 
//   delete [] mynlay          ; 
//   delete [] mynrchop        ; 
//   delete [] m_hnoflo     ; 
//   delete [] m_icnvg      ; 
//   delete [] m_icnvg           ; 
//   delete [] m_gmgid      ; 
//   delete [] m_bigheadchg ; 
//   delete [] m_iadampgmg  ; 
//   delete [] m_iunitmhc   ; 
//   delete [] m_rCloseGMG  ; 
//   delete [] m_iiter      ; 
//   delete [] m_hCloseGMG  ; 
//   delete [] m_dampgmg    ; 
//   delete [] m_ioutgmg    ; 
//   delete [] m_ism        ; 
//   delete [] m_isc        ; 
//   delete [] m_chglimit   ; 
//   delete [] m_dup        ; 
//   delete [] m_dlow       ; 
//   delete [] m_siter      ; 
//   delete [] m_tsiter     ; 
//   delete [] locbudperc         ; 
//   delete [] m_budperc    ; 
//   delete [] m_stoper     ; 
//   //delete [] m_hOld       ; 
//   //delete [] m_hNew       ; 
//   //delete [] m_hNew2      ; 
//   //delete [] m_rhs		  ;
//   //delete [] m_cr		  ;   
//   //delete [] m_cc		  ;   
//   //delete [] m_cv		  ;   
//   //delete [] m_hcof	     ;
//   //delete [] m_hNewLast  ; 
//   //delete [] m_iBound	  ;
//   delete [] JAMESNCOL       ; 
//   delete [] JAMESNROW       ; 
//   delete [] JAMESNLAY       ; 
//   delete [] m_nstp       ; 
//   delete [] m_nodes      ; 
////   delete [] m_res        ; 
//   delete [] m_issFlag         ; 
//   delete [] m_iter1      ; 
//   delete [] m_npcond     ; 
//   delete [] m_iprpcg     ; 
//   delete [] m_mutpcg     ; 
//   delete [] m_niter      ; 
//   delete [] m_hClose     ; 
//   delete [] m_rClose     ; 
//   delete [] m_relax      ; 
//   delete [] m_dampSS     ; 
//   delete [] m_dampTR     ; 
//   //delete [] m_v          ; 
//   //delete [] m_ss         ; 
//   //delete [] m_p          ; 
//   //delete [] m_hpcg       ; 
//   //delete [] m_cd         ; 
//   delete [] m_hcsv       ; 
//   delete [] m_lhch       ; 
//   delete [] m_hchg       ; 
//   delete [] m_rchg       ; 
//   delete [] m_lrch       ; 
//   delete [] m_icnvg      ; 
//   delete [] m_mxiter     ; 
//   delete [] m_nbpol      ; 
//   delete [] m_it1        ; 
//   delete [] m_ihcofadd   ; 
//   delete [] m_iout       ; 
//   delete [] m_iss        ; 

//      USTOP(' ')

//      END

   return TRUE;
	

	}

bool ModflowAdapter::LoadPolyGridLookup( MapLayer* , int *myncol, int *mynrow)
	{
	
	ASSERT(m_pPolyMapLayer != NULL);

   CString message;
   CString pglFile;
   CString pglPath;
   CString msg;
   int m_nYSize  = mynrow[0];
	int m_nXSize  = myncol[0];

	// need to create input file with these parameters.
	double UpperLeftUTMy = 5088076.0;
	double UpperLeftUTMx = 438791.0;
	double CellUTMyResolution = 2133.6;
	double CellUTMxResolution = 2133.6;
	
   CString m_polyGridFName = "ww2100_mckenzie_idu_polyGridLookup.pgl";
 	
   int XYZSize = mynrow[0] * myncol[0] * 1;
	
	double noDataValue = -9999.0;
 
   // status: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 
   int status = PathManager::FindPath( m_polyGridFName, pglPath );    // look along Envision paths

   // if .pgl file exists: read poly - grid - lookup table from file
   if ( status >= 0  )
      {      
      msg = _T("  ModFlowAdapter:: Loading PolyGrid from File: ");
      msg += pglPath;
      Report::LogMsg(msg);
         
      //Report::LogMsg(_T("  Creating PolyGrid - Starting"));
      m_pPolyGridLkUp = new PolyGridLookups( pglPath );
      //Report::LogMsg(_T("  Creating PolyGrid - Finished"));
      }
   else   // else (if .pgl file does NOT exists): create poly - grid - lookup table and write to .pgl file
      {
      
      Report::LogMsg(_T("  ModFlowAdapter:: Creating PolyGrid from Data - Starting"));
      
      m_pafScanbandbool =  new bool[ m_nXSize * m_nYSize ];

		BuildDataObj((double * )m_hNew,noDataValue, m_pafScanbandbool,XYZSize);

		m_pPolyGridLkUp  = new PolyGridLookups(
			UpperLeftUTMy,
			UpperLeftUTMx,
			CellUTMyResolution,
			CellUTMxResolution,
			m_pafScanbandbool,
			mynrow[0],
			myncol[0],
			m_pPolyMapLayer,
			m_numSubGridCells,
			m_nYSize * m_nXSize * 2, 
			0,
			-9999);
   
      Report::LogMsg(_T("  ModFlowAdapter:: Generating PolyGrid from Data - Finished"));

      pglPath = m_polyGridFName;

      // write to file.  If the name is full qualified, use it.  Otherwise, use the IDU directory
      if ( pglPath.Find( '\\' ) < 0 && pglPath.Find( '/' ) < 0 )     // no slashes specfied? 
         {
         pglPath = PathManager::GetPath( PM_IDU_DIR );
         pglPath += m_polyGridFName;
         }

      msg = _T(" ModFlowAdapter::  Writing PolyGrid to File: ");
      msg += pglPath;
      Report::LogMsg( msg );
      m_pPolyGridLkUp->WriteToFile( pglPath );

      Report::LogMsg(_T("  ModFlowAdapter:: Writing PolyGrid to File - Finished"));
      }

   msg.Format( "  Polygrid Rows=%i, Cols=%i", m_pPolyGridLkUp->GetNumGridRows(), m_pPolyGridLkUp->GetNumGridCols() ); 
   Report::LogMsg( msg );
 
   if(m_pafScanbandbool != NULL)
      {
      delete[] m_pafScanbandbool;
      }

	return true;
	}

bool ModflowAdapter::UpdateGrid(FlowContext *pFlowContext, PolyGridLookups *polyGridLkUp, float *gridVariable)
   {

   int GridPtCnt;
   int MaxGridPtCnt = 0;
   int Poly;
   int GridPt;
   int ptrIndex;
   float recharge;
   float  *pGridPtProportions = NULL;
   POINT  *pGridPtNdxs = NULL;
   
   //Map *pMap = pFlowContext->pEnvContext->pMapLayer->GetMapPtr();

   //MapLayer *pModFlowGrid = pMap->GetLayer( "ModFlowGrid" );
    
   // get HRU count
   int hruCount = pFlowContext->pFlowModel->GetHRUCount(); 

   // HRU loop
   for ( int h=0; h < hruCount; h++ )
      {
    
      HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);   

      HRULayer *pHRULayer = pHRU->GetLayer( FL_GROUNDWATER );

      int polyCount = int ( pHRU->m_polyIndexArray.GetCount() );

      // loop over polygons in each HRU
      for ( int poly=0; poly < polyCount; poly++ )
         {

         if( ( GridPtCnt = m_pPolyGridLkUp->GetGridPtCntForPoly(Poly) ) > MaxGridPtCnt ) 
            {
            MaxGridPtCnt = GridPtCnt;

            if( pGridPtProportions )
               delete [] pGridPtProportions;

            pGridPtProportions = new float[MaxGridPtCnt];

            if(pGridPtNdxs)
               delete [] pGridPtNdxs;

            pGridPtNdxs = new POINT[MaxGridPtCnt];
            } 

         // initialize pointers
         for(int i=0; i < MaxGridPtCnt; i++ )
            {
            pGridPtProportions[i] = 0;
            pGridPtNdxs[i].x = 0;
            pGridPtNdxs[i].y = 0;
            }

         // get the grid points for the polygon
         m_pPolyGridLkUp->GetGridPtNdxsForPoly(Poly, pGridPtNdxs);

         // get the proportions for the polygon
         m_pPolyGridLkUp->GetGridPtProportionsForPoly( Poly, pGridPtProportions );

         // loop over grid points for each polygon
         for( GridPt = 0; GridPt < GridPtCnt; GridPt++ ) 
            {                   
            recharge = 0.f;
               
            // get downward flux into groundwater layer.  
            recharge = pFlowContext->pHRU->m_waterFluxArray[FL_DOWNWARD_TOP];

            // index to ModFlow recharge grid/pointer  ( row * rowSize ) + column
            ptrIndex =  ( pGridPtNdxs[GridPt].y * m_row[0] ) + pGridPtNdxs[GridPt].x;
            
            // update modflow grid/pointer with recharge from HRU, groundwater layer, downward flux to top
            *(gridVariable +ptrIndex) = recharge * pGridPtProportions[GridPt];           
            }

         } // end Polygon loop

         if( pGridPtNdxs != NULL )
            {
            delete[] pGridPtNdxs;
            pGridPtNdxs = NULL;
            }

         if( pGridPtProportions != NULL ) 
            {
            delete[] pGridPtProportions;
            pGridPtProportions = NULL;
            }

      } // end HRU loop

   return true;
   }

bool ModflowAdapter::UpdateGrid(FlowContext *pFlowContext, float *gridVariable)
   {

   Vertex hruCentroid;
   int gridCol;
   int gridRow;
   float recharge;
   int ptrIndex;
      
   Map *pMap = pFlowContext->pEnvContext->pMapLayer->GetMapPtr();

   MapLayer *pModFlowGrid = pMap->GetLayer( "ModFlowGrid" );
    
   int hruCount = pFlowContext->pFlowModel->GetHRUCount(); 

   for ( int h=0; h < hruCount; h++ )
      {   
      HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);   
    
      HRULayer *pHRULayer = pHRU->GetLayer( FL_GROUNDWATER );
    
      hruCentroid = pHRU->m_centroid;

      pModFlowGrid->GetGridCellFromCoord(hruCentroid.x,hruCentroid.y,gridRow,gridCol);

      recharge = 0.f;
            
      // get downward flux into groundwater layer.  
      recharge = pFlowContext->pHRU->m_waterFluxArray[FL_DOWNWARD_TOP];

      // index to ModFlow recharge grid/pointer  ( row * rowSize ) + column
      ptrIndex =  ( gridRow * m_row[0] ) + gridCol;
      
      // update modflow grid/pointer with recharge from HRU, groundwater layer, downward flux to top
      *(gridVariable +ptrIndex) = recharge;        
      }

   return true;

   }

void ModflowAdapter::BuildDataObj(double *inputarr,double nodataval, bool *targetboolarray,int rasterxyzsize)
{
//boolean of true false that lets polygrid lookup know where there is a no data value
	
	for (int i=0;i<m_col[0]*m_row[0]*1;i++)
	{
		if (*(inputarr+i) == nodataval ) *(targetboolarray+i) = false; //where no data value make false at same location in bool for use in polygridlookups
	
	}
	
}
	
void ModflowAdapter::GetNamFil(char* envFileName)
	{
		m_GetNamFilFn(envFileName);
	}

void ModflowAdapter::OpenNamFil(int* envInUnit,char* envFileName,int* envIbdt)
	{
		m_OpenNamFilFn(envInUnit,envFileName,envIbdt);
	}

void ModflowAdapter::GWF2BAS7AR(int* envInUnit,char envCUnit[100][5],char* envVERSION,int* envIUDIS,int* envIUZON,int* envIUMLT,int* envMAXUNIT,
	                            int* enviGrid,int* envIUOC,char envHEADNG[2][81],int* envIUPVAL,char* envMFVNAM,int enviunit[100],int* envnper,
								int* envnstp,int* enviout,int* envcol,int* envrow,int* envlay,double* envstoper,void* envHnew,
								void *envRHS,void *envHCOF,void *envhNewLAST,void *envIBOUND, int *envnodes, int *envissFlag)
	{
		m_GWF2BAS7ARFn(envInUnit,envCUnit,envVERSION,envIUDIS,envIUZON,envIUMLT,envMAXUNIT,enviGrid,envIUOC,envHEADNG,envIUPVAL,envMFVNAM,enviunit,envnper,envnstp,
		enviout,envcol,envrow,envlay,envstoper,envHnew,envRHS,
	    envHCOF,envhNewLAST,envIBOUND,envnodes,envissFlag);
	}

void ModflowAdapter::GWF2LPF7AR    (int* jiunit,int* enviGrid,void* envCC,void* envCV,void* envCR){m_GWF2LPF7ARFn(jiunit,enviGrid,envCC,envCV,envCR);} 
void ModflowAdapter::GWF2WEL7AR    (int* jiunit,int* enviGrid){m_GWF2WEL7ARFn(jiunit,enviGrid);} 
void ModflowAdapter::GWF2DRN7AR    (int* jiunit,int* enviGrid){m_GWF2DRN7ARFn(jiunit,enviGrid);} 
void ModflowAdapter::GWF2RIV7AR    (int* jiunit,int* enviGrid){m_GWF2RIV7ARFn(jiunit,enviGrid);}
void ModflowAdapter::GWF2CHD7AR    (int* jiunit,int* enviGrid){m_GWF2CHD7ARFn(jiunit,enviGrid);}
void ModflowAdapter::GWF2RCH7AR    (int* jiunit,int* enviGrid,int* envNRCHOP){m_GWF2RCH7ARFn(jiunit,enviGrid,envNRCHOP);} 
void ModflowAdapter::PCG7AR        (int* jiunit,int* envmxiter, int* enviGrid, int* enviter1,float* envdampSS,float* envdampTR,
                                    int* envnpcond, int* envihcofadd, float* envhClose, float* envrClose,float* envrelax,
                                    int* envnbpol, int* enviprpcg, int* envmutpcg){m_PCG7ARFn(jiunit,envmxiter,enviGrid,enviter1,envdampSS,envdampTR,
                                    envnpcond,envihcofadd,envhClose,envrClose,envrelax,
                                    envnbpol,enviprpcg,envmutpcg);} 

void ModflowAdapter::PCG7PNT       (int* enviGrid,int* enviter1,int* envnpcond,int* envnbpol,int*  enviprpcg,int* envmutpcg,
                                   int* envniter,float* envhClose,float* envrClose,
                    float* envrelax,float* envdampSS,float* envdampTR,int* envihcofadd){m_PCG7PNTFn(enviGrid,enviter1,envnpcond,envnbpol,enviprpcg,envmutpcg,
                                   envniter,envhClose,envrClose, envrelax,envdampSS,envdampTR,envihcofadd);} 


void ModflowAdapter::GMG7AR        (int* jiunit,int* envmxiter,int* enviGrid,int* enviadampgmg,int* enviunitmhc,float* envrCloseGMG,int* enviiter,float* envhCloseGMG,
																									float* envdampgmg,  
																									int* envioutgmg,  
																									int* envism,      
																									int* envisc,      
																									float* envchglimit, 
																									float* envdup,      
																									float* envdlow,
																									int* envgmgid)
									{m_GMG7ARFn(jiunit,envmxiter,enviGrid,enviadampgmg,enviunitmhc,envrCloseGMG,
							           enviiter,    
	  						           envhCloseGMG,
	  						           envdampgmg,  
	  						           envioutgmg,  
	  						           envism,      
	  						           envisc,      
	  						           envchglimit, 
	  						           envdup,      
	  						           envdlow,
									   envgmgid);}     
void ModflowAdapter::OBS2BAS7AR    (int* jiunit,int* enviGrid){m_OBS2BAS7ARFn(jiunit,enviGrid);} 
void ModflowAdapter::GWF2BAS7ST    (int* envKKPER,int* enviGrid){m_GWF2BAS7STFn(envKKPER,enviGrid);} 
void ModflowAdapter::GWF2WEL7RP    (int* jiunit,int* enviGrid, int* envNwells){m_GWF2WEL7RPFn(jiunit,enviGrid,envNwells);} 
void ModflowAdapter::GWF2DRN7RP    (int* jiunit,int* enviGrid,int* envNdrains){m_GWF2DRN7RPFn(jiunit,enviGrid,envNdrains);} 
void ModflowAdapter::GWF2RIV7RP    (int* jiunit,int* enviGrid,int* envNrivers){m_GWF2RIV7RPFn(jiunit,enviGrid,envNrivers);}
void ModflowAdapter::GWF2CHD7RP    (int* jiunit,int* enviGrid,void* enviBound,int* envNCHDS){m_GWF2CHD7RPFn(jiunit,enviGrid,enviBound,envNCHDS);} 
void ModflowAdapter::GWF2RCH7RP    (int* jiunit,int* enviGrid,int* enviNrecharge,void* recharge){m_GWF2RCH7RPFn(jiunit,enviGrid,enviNrecharge,recharge);} 
void ModflowAdapter::GWF2BAS7AD    (int* envKKPER,int* envKKSTP,int* enviGrid,void* envhNew,void* envhOld){m_GWF2BAS7ADFn(envKKPER,envKKSTP,enviGrid,envhNew,envhOld);} 
void ModflowAdapter::GWF2LPF7AD    (int* envKKPER,int* enviGrid,void* envhOld){m_GWF2LPF7ADFn(envKKPER,enviGrid,envhOld);} 
void ModflowAdapter::GWF2CHD7AD    (int* envKKPER,int* enviGrid,void* envhNew,void* envhOld){m_GWF2CHD7ADFn(envKKPER,enviGrid,envhNew,envhOld);} 
void ModflowAdapter::UMESPR        (char* UMESPRtxt1,char* UMESPRtxt2,int* enviout){ m_UMESPRFn(UMESPRtxt1,UMESPRtxt2,enviout);} 
void ModflowAdapter::GWF2BAS7FM    (int* enviGrid,void* envhcof,void* envrhs){m_GWF2BAS7FMFn(enviGrid,envhcof,envrhs);} 
void ModflowAdapter::GWF2LPF7FM    (int* envKKITER,int* envKKSTP,int* envKKPER,int* enviGrid,void* envCC,void* envCV,void* envCR,void* envHCOF,void* envhOld,void* envrhs,void* envhNew,void* enviBound)
									{ m_GWF2LPF7FMFn(envKKITER,envKKSTP,envKKPER,enviGrid,envCC,envCV,envCR,envHCOF,envhOld,envrhs,envhNew,enviBound);} 
void ModflowAdapter::GWF2WEL7FM    (int* enviGrid,void* envrhs){m_GWF2WEL7FMFn(enviGrid,envrhs);} 
void ModflowAdapter::GWF2DRN7FM    (int* enviGrid,void* envrhs,void* envhcof,void* envhNew){m_GWF2DRN7FMFn(enviGrid,envrhs,envhcof,envhNew);} 
void ModflowAdapter::GWF2RIV7FM    (int* enviGrid,void* envhcof,void* envrhs,void* envhNew2){m_GWF2RIV7FMFn(enviGrid,envhcof,envrhs,envhNew2);} 
void ModflowAdapter::GWF2LAK7ST    (int* a,int* enviGrid){m_GWF2LAK7STFn(a,enviGrid);} 
void ModflowAdapter::GWF2RCH7FM    (int* enviGrid,void* envRHS){m_GWF2RCH7FMFn(enviGrid,envRHS);} 

void ModflowAdapter::GMG7PNT(int* enviGrid,int* enviiter,int* enviadampgmg,int* envioutgmg,
     int* envsiter,int* envtsiter,int* envgmgid,float* envrCloseGMG,float* envhCloseGMG,
     float* envdampgmg,int* enviunitmhc,float* envdup,float* envdlow,float* envchglimit,
     double* envbigheadchg,void* envhNewLast) {m_GMG7PNTFn(enviGrid,enviiter,enviadampgmg,envioutgmg,
     envsiter,envtsiter,envgmgid,envrCloseGMG,envhCloseGMG,
     envdampgmg,enviunitmhc,envdup,envdlow,envchglimit,
     envbigheadchg,envhNewLast);}   

void ModflowAdapter::GMG7AP        (void* envHNEW,void* envRHS,void* envCR,void* envCC,void* envCV,void* envHCOF,float* envHNOFLO,void* envIBOUND,
                                    int* envIITER,int* envmxiter,float* envRCLOSEGMG,float* envHCLOSEGMG,
                                    int* envKKITER,int* envKKSTP,int* envKKPER,int* envNCOL,int* envNROW,int* envNLAY,int* envicnvg,
                                    int* envITER,int* envtsiter,float* envDAMPGMG,int* envIADAMPGMG,int* envIOUTGMG,
                                    int* envIOUT,int* envGMGID,
                                    int* envIUNITMHC,float* envDUP,float* envDLOW,float* envCHGLIMIT,
                                    double* envBIGHEADCHG,void* envHNEWLAST,void* envhNew)
									{
										m_GMG7APFn(envHNEW,envRHS,envCR,envCC,envCV,envHCOF,envHNOFLO,envIBOUND,
                                    envIITER,envmxiter,envRCLOSEGMG,envHCLOSEGMG,
                                    envKKITER,envKKSTP,envKKPER,envNCOL,envNROW,envNLAY,envicnvg,
                                    envITER,envtsiter,envDAMPGMG,envIADAMPGMG,envIOUTGMG,
                                    envIOUT,envGMGID,
                                    envIUNITMHC,envDUP,envDLOW,envCHGLIMIT,
                                    envBIGHEADCHG,envHNEWLAST,envhNew);
									} 


void ModflowAdapter::PCG7AP  (void* envhNew,void* enviBound,void* envcr,void* envcc,
               void* envcv,void* envhcof,void* envrhs,
               void* envv,void* envss,void* envp,void* envcd,void* envhchg,
               void* envlhch,void* envrchg,void* envlrch,int* envKKITER, 
               int* envniter,float* envhClose,float* envrClose,int* envicnvg,
               int* envKKSTP, int* envKKPER,int* enviprpcg,int* envmxiter,int* enviter1,
               int* envnpcond, int* envnbpol,int* envnstp,int* envcol, 
               int* envrow,int* envlay,int* envnodes,float* envrelax, 
               int* enviout,int* envmutpcg,int* envit1,float* envdampSS,
               void* envres,void* envhcsv,int* envIERR,void* envhpcg, 
               float* envdampTR,int* enviss,float* envhdry,int* envihcofadd)
                       {
                       m_PCG7APFn(envhNew,enviBound,envcr,envcc,
               envcv,envhcof,envrhs,
               envv,envss,envp,envcd,envhchg,
               envlhch,envrchg,envlrch,envKKITER, 
               envniter,envhClose,envrClose,envicnvg,
               envKKSTP,envKKPER,enviprpcg,envmxiter,enviter1,
               envnpcond,envnbpol,envnstp,envcol, 
               envrow,envlay,envnodes,envrelax, 
               enviout,envmutpcg,envit1,envdampSS,
               envres,envhcsv,envIERR,envhpcg, 
               envdampTR,enviss,envhdry,envihcofadd);
                       }



void ModflowAdapter::GWF2BAS7OC     (int* envKKSTP,int* envKKPER,int* envicnvg,int* jiunit,int* enviGrid){m_GWF2BAS7OCFn(envKKSTP,envKKPER,envicnvg,jiunit,enviGrid);}    
void ModflowAdapter::GWF2LPF7BDS    (int* envKKSTP,int* envKKPER,int* enviGrid,int* envmsumin,void* envhNew,void* envhOld,void* enviBound){m_GWF2LPF7BDSFn(envKKSTP,envKKPER,enviGrid,envmsumin,envhNew,envhOld,enviBound);}  
void ModflowAdapter::GWF2LPF7BDCH   (int* envKKSTP,int* envKKPER,int* enviGrid,void* envhNew,void* envCC,void* envCV,void* envCR)
									{m_GWF2LPF7BDCHFn(envKKSTP,envKKPER,enviGrid,envhNew,envCC,envCV,envCR);} 
void ModflowAdapter::GWF2LPF7BDADJ  (int* envKKSTP,int* envKKPER,int* envIDIR,int* envIBDRET,int* envIC1,int* envIC2,int* envIR1,int* envIR2,int* envIL1,int* envIL2,int* enviGrid,void* envhNew,void* envcc,void* envcv,void* envcr)
									{m_GWF2LPF7BDADJFn(envKKSTP,envKKPER,envIDIR,envIBDRET,envIC1,envIC2,envIR1,envIR2,envIL1,envIL2,enviGrid,envhNew,envcc,envcv,envcr);} 
void ModflowAdapter::GWF2WEL7BD     (int* envKKSTP,int* envKKPER,int* enviGrid){m_GWF2WEL7BDFn(envKKSTP,envKKPER,enviGrid);}   
void ModflowAdapter::GWF2DRN7BD     (int* envKKSTP,int* envKKPER,int* enviGrid,void* envhNew){m_GWF2DRN7BDFn(envKKSTP,envKKPER,enviGrid,envhNew);}   
void ModflowAdapter::GWF2RIV7BD     (int* envKKSTP,int* envKKPER,int* enviGrid,void* envhNew){m_GWF2RIV7BDFn(envKKSTP,envKKPER,enviGrid,envhNew);}   
void ModflowAdapter::GWF2RCH7BD     (int* envKKSTP,int* envKKPER,int* enviGrid){m_GWF2RCH7BDFn(envKKSTP,envKKPER,enviGrid);}     
void ModflowAdapter::GWF2BAS7OT     (int* envKKSTP,int* envKKPER,int* envicnvg,int* aa,int* enviGrid,double* locbudperc,double* envbudperc,void* envhNew){m_GWF2BAS7OTFn(envKKSTP,envKKPER,envicnvg,aa,enviGrid,locbudperc,envbudperc,envhNew);}   
void ModflowAdapter::GLO1BAS6ET     (int* envIOUT,int* envIbdt,int* bb){m_GLO1BAS6ETFn(envIOUT,envIbdt,bb);}   
void ModflowAdapter::SGWF2BAS7PNT   (int* enviGrid){m_SGWF2BAS7PNTFn(enviGrid);} 
void ModflowAdapter::GWF2WEL7DA     (int* enviGrid){m_GWF2WEL7DAFn(enviGrid);}   
void ModflowAdapter::GWF2DRN7DA     (int* enviGrid){m_GWF2DRN7DAFn(enviGrid);}   
void ModflowAdapter::GWF2RIV7DA     (int* enviGrid){m_GWF2RIV7DAFn(enviGrid);}   
void ModflowAdapter::GWF2RCH7DA     (int* enviGrid){m_GWF2RCH7DAFn(enviGrid);}   
void ModflowAdapter::GWF2LPF7DA     (int* enviGrid){m_GWF2LPF7DAFn(enviGrid);}   
void ModflowAdapter::OBS2BAS7DA     (int* cc,int* enviGrid){m_OBS2BAS7DAFn(cc,enviGrid);}   
void ModflowAdapter::GWF2BAS7DA     (int* enviGrid){m_GWF2BAS7DAFn(enviGrid);}
//void ModflowAdapter::PCG7DA         (int* enviGrid,int* enviter1,int* envnpcond,int* envnbpol,int*  enviprpcg,int* envmutpcg,
//                                   int* envniter,float* envhClose,float* envrClose,
//                    float* envrelax,float* envdampSS,float* envdampTR,int* envihcofadd,void* envv,void* envss,
//    void* envp,void* envhpcg,void* envcd,void* envhcsv,void* envlhch,void* envhchg,
 //   void* envlrch,void* envrchg,void* envit1){m_PCG7DAFn(enviGrid,enviter1,envnpcond,envnbpol,enviprpcg,envmutpcg,
//                                   envniter,envhClose,envrClose, envrelax,envdampSS,envdampTR,envihcofadd,envv,envss,
 //                                  envp,envhpcg,envcd,envhcsv,envlhch,envhchg,
//                                   envlrch,envrchg,envit1);}
void ModflowAdapter::GWF2CHD7DA     (int* enviGrid){m_GWF2CHD7DAFn(enviGrid);}