#include <FDataObj.h>
#include <map.h>
#include <maplayer.h>
#include <PolyGridLookups.h>
class FlowContext;
class Reach;
class HRU;

using namespace std;
typedef double (*MODFLOW_GLOBAL_mp_HNEWFN);
typedef void (*MODFLOW_GETNAMFILFN)  (char*);
typedef void (*MODFLOW_OPENNAMFILFN) (int*,char*,int*);
typedef void (*MODFLOW_GWF2BAS7ARFN) (int*,char[100][5],char*,int*,int*,int*,int*,int*,
	                                  int*,char[2][81],int*,char*,int[100],int*,int[24],
									  int[1],int[1],int[1],int[1],double*,void*,void*,
									  void*,void*,void*,int*,int*);
typedef void (*MODFLOW_GWF2LPF7ARFN) (int*,int*,void*,void*,void*);
typedef void (*MODFLOW_GWF2WEL7ARFN) (int*,int*);
typedef void (*MODFLOW_GWF2DRN7ARFN) (int*,int*);
typedef void (*MODFLOW_GWF2RIV7ARFN) (int*,int*);
typedef void (*MODFLOW_GWF2CHD7ARFN) (int*,int*);
typedef void (*MODFLOW_GWF2RCH7ARFN) (int*,int*,int*);
typedef void (*MODFLOW_GMG7ARFN)     (int*,int*,int*,int*,int*,float*,int*,float*,float*,int*,int*,int*,float*,float*,float*,int*);
typedef void (*MODFLOW_OBS2BAS7ARFN) (int*,int*);
typedef void (*MODFLOW_GWF2BAS7STFN) (int*,int*);
typedef void (*MODFLOW_GWF2WEL7RPFN) (int*,int*,int*);
typedef void (*MODFLOW_GWF2DRN7RPFN) (int*,int*,int*);
typedef void (*MODFLOW_GWF2RIV7RPFN) (int*,int*,int*);
typedef void (*MODFLOW_GWF2CHD7RPFN) (int*,int*,void*,int*);
typedef void (*MODFLOW_GWF2RCH7RPFN) (int*,int*,int*,void*);
typedef void (*MODFLOW_GWF2BAS7ADFN) (int*,int*,int*,void*,void*);
typedef void (*MODFLOW_GWF2CHD7ADFN) (int*,int*,void*,void*);
typedef void (*MODFLOW_GWF2LPF7ADFN) (int*,int*,void*);
typedef void (*MODFLOW_UMESPRFN)     (char*,char*,int*);//('SOLVING FOR HEAD',' ',int*)
typedef void (*MODFLOW_GWF2BAS7FMFN) (int*,void*,void*);
typedef void (*MODFLOW_GWF2LPF7FMFN) (int*,int*,int*,int*,void*,void*,void*,void*,void*,void*,void*,void*);
typedef void (*MODFLOW_GWF2WEL7FMFN) (int*,void*);
typedef void (*MODFLOW_GWF2DRN7FMFN) (int*,void*,void*,void*);
typedef void (*MODFLOW_GWF2RIV7FMFN) (int*,void*,void*,void*);
typedef void (*MODFLOW_GWF2RCH7FMFN) (int*,void*);
typedef void (*MODFLOW_GMG7PNTFN)    (int*,int*,int*,int*,int*,int*,int*,float*,float*,
                                      float*,int*,float*,float*,float*,double*,void*);
typedef void (*MODFLOW_GMG7APFN)(void*,void*,void*,void*,void*,void*,float*,void*,
                              int*,int*,float*,float*,
                              int*,int*,int*,int*,int*,int*,int*,
                              int*,int*,float*,int*,int*,
                              int*,int*,
                              int*,float*,float*,float*,
                              double*,void*,void*);

typedef void (*MODFLOW_PCG7APFN)(void*,void*,void* ,void* ,
               void* ,void* ,void* ,
               void* ,void* ,void*,void* ,void* ,
               void* ,void* ,void* ,int* , 
               int* ,float* ,float* ,int* ,
               int* , int* ,int* ,int* ,int* ,
               int* , int* ,int* ,int* , 
               int* ,int* ,int* ,float* , 
               int* ,int* ,int* ,float* ,
               void* ,void* ,int* ,void* , 
               float* ,int* ,float* ,int* );


typedef void (*MODFLOW_GWF2BAS7OCFN)   (int*,int*,int*,int*,int*);
typedef void (*MODFLOW_GWF2LPF7BDSFN)  (int*,int*,int*,int*,void*,void*,void*);
typedef void (*MODFLOW_GWF2LPF7BDCHFN) (int*,int*,int*,void*,void*,void*,void*);
typedef void (*MODFLOW_GWF2LPF7BDADJFN)(int*,int*,int*,int*,int*,int*,int*,int*,int*,int*,int*,void*,void*,void*,void*);
typedef void (*MODFLOW_GWF2WEL7BDFN)   (int*,int*,int*);
typedef void (*MODFLOW_GWF2DRN7BDFN)   (int*,int*,int*,void*);
typedef void (*MODFLOW_GWF2RIV7BDFN)   (int*,int*,int*,void*);
typedef void (*MODFLOW_GWF2LAK7STFN)   (int*,int*);
typedef void (*MODFLOW_GWF2RCH7BDFN)   (int*,int*,int*);
typedef void (*MODFLOW_GWF2BAS7OTFN)   (int*,int*,int*,int*,int*,double*,double*,void*);
typedef void (*MODFLOW_GLO1BAS6ETFN)   (int*,int*,int*);
typedef void (*MODFLOW_SGWF2BAS7PNTFN) (int*);
typedef void (*MODFLOW_GWF2WEL7DAFN)   (int*);
typedef void (*MODFLOW_GWF2DRN7DAFN)   (int*);
typedef void (*MODFLOW_GWF2RIV7DAFN)   (int*);
typedef void (*MODFLOW_GWF2RCH7DAFN)   (int*);
typedef void (*MODFLOW_GWF2LPF7DAFN)   (int*);
typedef void (*MODFLOW_GWF2CHD7DAFN)   (int*);
typedef void (*MODFLOW_OBS2BAS7DAFN)   (int*,int*);
typedef void (*MODFLOW_GWF2BAS7DAFN)   (int*);
typedef void (*MODFLOW_PCG7ARFN)       (int*,int*,int*,int*,float*,float*,int*,int*,float*,float*,float*,int*,int*,int*);
typedef void (*MODFLOW_PCG7PNTFN)     (int* ,int* ,int* ,int* ,int*  ,int* , int* ,float* ,float* ,float* ,float* ,float* ,int*);
//typedef void (*MODFLOW_PCG7DAFN)      (int*,int*,int*,int*,int*,int*, int*,float*,float*,float*,float*,float*,int*,void*,void*,
//                                       void*,void*,void*,void*,void*,void*,void*,void*,void*);
class ModflowAdapter
{
public:
   ModflowAdapter ( void );
   ~ModflowAdapter( void );

   BOOL Init   ( FlowContext *pFlowContext,LPCTSTR initStr );
   BOOL InitRun( FlowContext *pFlowContext );
   BOOL Run    ( FlowContext *pFlowContext );
   BOOL EndRun ( FlowContext *pFlowContext );
   bool LoadPolyGridLookup( MapLayer *pLayer, int *col, int *row );
   bool UpdateGrid( FlowContext *pFlowContext, PolyGridLookups *polyGridLkUp, float *gridVariable ); //use polygrid lookups
   bool UpdateGrid( FlowContext *pFlowContext, float *gridVariable ); //use HRU centroid
   void BuildDataObj(double *inputarr,double nodataval, bool *boolarray,int rasterxysize);	

   void GetNamFil     (char*);
   void OpenNamFil    (int*,char*,int*);
   void GWF2BAS7AR    (int*,char[100][5],char*,int*,int*,int*,int*,int*,int*,
	                   char[2][81],int*,char*,int[100],int*,int[24],int[1],
					       int[1],int[1],int[1],double*,void*,void*,
					       void*,void*,void*,int*,int*);  
   void GWF2LPF7AR    (int*,int*,void*,void*,void*);
   void GWF2WEL7AR    (int*,int*);
   void GWF2DRN7AR    (int*,int*);
   void GWF2RIV7AR    (int*,int*);
   void GWF2RCH7AR    (int*,int*,int*);
   void GWF2CHD7AR    (int*,int*);
   void GMG7AR        (int*,int*,int*,int*,int*,float*,int*,float*,float*,int*,int*,int*,float*,float*,float*,int*);
   void OBS2BAS7AR    (int*,int*);
   void GWF2BAS7ST    (int*,int*);
   void GWF2WEL7RP    (int*,int*,int*);
   void GWF2DRN7RP    (int*,int*,int*);
   void GWF2RIV7RP    (int*,int*,int*);
   void GWF2CHD7RP    (int*,int*,void*,int*);
   void GWF2RCH7RP    (int*,int*,int*,void*);
   void GWF2BAS7AD    (int*,int*,int*,void*,void*);
   void GWF2LPF7AD    (int*,int*,void*);
   void GWF2CHD7AD    (int*,int*,void*,void*);
   void UMESPR        (char*,char*,int*);  //('SOLVING FOR HEAD',' ',int*)
   void GWF2BAS7FM    (int*,void*,void*);
   void GWF2LPF7FM    (int*,int*,int*,int*,void*,void*,void*,void*,void*,void*,void*,void*);
   void GWF2WEL7FM    (int*,void*);
   void GWF2DRN7FM    (int*,void*,void*,void*);
   void GWF2RIV7FM    (int*,void*,void*,void*);
   void GWF2LAK7ST    (int*,int*);
   void GWF2RCH7FM    (int*,void*);
   void GMG7PNT       (int*,int*,int*,int*,int*,int*,int*,float*,float*,
                       float*,int*,float*,float*,float*,double*,void*); 
   void GMG7AP        (void*,void*,void*,void*,void*,void*,float*,void*,
                       int*,int*,float*,float*,
                       int*,int*,int*,int*,int*,int*,int*,
                       int*,int*,float*,int*,int*,
                       int*,int*,
                       int*,float*,float*,float*,
                       double*,void*,void*);

   void PCG7AP      (void*,void*,void* ,void* ,
               void* ,void* ,void* ,
               void* ,void* ,void*,void* ,void* ,
               void* ,void* ,void* ,int* , 
               int* ,float* ,float* ,int* ,
               int* , int* ,int* ,int* ,int* ,
               int* , int* ,int* ,int* , 
               int* ,int* ,int* ,float* , 
               int* ,int* ,int* ,float* ,
               void* ,void* ,int* ,void* , 
               float* ,int* ,float* ,int*);

   void GWF2BAS7OC     (int*,int*,int*,int*,int*);
   void GWF2LPF7BDS    (int*,int*,int*,int*,void*,void*,void*);
   void GWF2LPF7BDCH   (int*,int*,int*,void*,void*,void*,void*);
   void GWF2LPF7BDADJ  (int*,int*,int*,int*,int*,int*,int*,int*,int*,int*,int*,void*,void*,void*,void*);
   void GWF2WEL7BD     (int*,int*,int*);
   void GWF2DRN7BD     (int*,int*,int*,void*);
   void GWF2RIV7BD     (int*,int*,int*,void*);
   void GWF2RCH7BD     (int*,int*,int*);
   void GWF2BAS7OT     (int*,int*,int*,int*,int*,double*,double*,void*);
   void GLO1BAS6ET     (int*,int*,int*);
   void SGWF2BAS7PNT   (int*);
   void GWF2WEL7DA     (int*);
   void GWF2DRN7DA     (int*);
   void GWF2RIV7DA     (int*);
   void GWF2RCH7DA     (int*);
   void GWF2LPF7DA     (int*);
   void GWF2CHD7DA     (int*);
   void OBS2BAS7DA     (int*,int*);
   void GWF2BAS7DA     (int*);
   void PCG7AR         (int*,int*,int*,int*,float*,float*,int*,int*,float*,float*,float*,int*,int*,int*);
   void PCG7PNT        (int* ,int* ,int* ,int* ,int*  ,int* , int* ,float* ,float* ,float* ,float* ,float* ,int* );
   //void PCG7DA         (int*,int*,int*,int*,int*,int*, int*,float*,float*,float*,float*,float*,int*,void*,void*,
    //                                   void*,void*,void*,void*,void*,void*,void*,void*,void*);
          
protected:

   int    *m_col;
   int    *m_row;
   int    *m_lay;
   int     m_inUnit;
   int    *m_ibdt;
   int     m_iudis;
   int     m_iuzon;
   int     m_iumlt;
   int     m_maxunit;
   int     m_iGrid;
   int     m_iuoc;
   int     m_iupval;
   int    *m_iunit;
   int    *m_nper;
   int    *m_nstp;
   int    *m_iout;
   double *m_stoper;
   void   *m_hNew;
   void   *m_rhs;
   void   *m_hcof;
   void   *m_hNewLast;
   void   *m_iBound;
   int    *m_nodes;
   int    *m_issFlag;
   void   *m_cc;
   void   *m_cv;
   void   *m_cr;
   int    *m_nrchop;
   void   *m_recharge;
   int    *m_mxiter;
   int    *m_iter1;
   float  *m_dampSS;
   float  *m_dampTR;
   int    *m_npcond;
   int    *m_ihcofadd;
   float  *m_hClose;
   float  *m_rClose;
   float  *m_relax;
   int    *m_nbpol;
   int    *m_iprpcg;
   int    *m_mutpcg;
   int    *m_iadampgmg;
   int    *m_iunitmhc;
   int    *m_iiter;
   float  *m_dampgmg;
   int    *m_ioutgmg;
   int    *m_ism; 
   int    *m_isc;
   float  *m_chglimit;
   float  *m_dup;
   float  *m_dlow; 
   int    *m_gmgid;
   void   *m_hOld;
   int    *m_ierr;
   int    *m_niter;
   void   *m_v;
   void   *m_ss;
   void   *m_p;
   void   *m_cd;
   void   *m_hpcg;
   void   *m_res;
   void   *m_hcsv;
   void   *m_hchg;
   void   *m_lhch;
   void   *m_rchg;
   void   *m_lrch;
   int    *m_icnvg;
   int    *m_it1;
   int    *m_iss;
   float  *m_hdry;
   int    *m_siter;
   int    *m_tsiter;
   double *m_bigheadchg;
   float  *m_hnoflo;
   int    *m_msumin;
   double *m_budperc;
   int     m_ncverr;
   int     m_cellDim;
   int     m_numSubGridCells;
   bool    *m_pafScanbandbool;
   double  *HNEW1d;
   int     *IBOUND1d;
   float   *CR1d;    
   float   *CC1d;    
   float   *CV1d;    
   float   *HCOF1d;  
   float   *RHS1d;   
   double  *V1d;     
   double  *SS1d;    
   double  *P1d;     
   float   *CD1d;    
   double  *HPCG1d;  
   float   *RES1d;   
   float   *HCSV1d;
   double  *locbudperc;
   int    m_nStressPeriods;
   int     *m_nWells;
   int     *m_nDrains;
   int     *m_nRivers;
   int     *m_iNrecharge;
   int     *m_nCHDS;
   float   *m_wells;

   PolyGridLookups *m_pPolyGridLkUp;
   MapLayer *m_pPolyMapLayer;
   MapLayer *m_pModFlowGrid;
   Map *m_pMap;
   
   CString msg, failureID;

   HINSTANCE  m_hInst;
   
   MODFLOW_GLOBAL_mp_HNEWFN         m_HNEW;
   MODFLOW_GETNAMFILFN              m_GetNamFilFn;
   MODFLOW_OPENNAMFILFN             m_OpenNamFilFn;
   MODFLOW_GWF2BAS7ARFN             m_GWF2BAS7ARFn;  
   MODFLOW_GWF2LPF7ARFN             m_GWF2LPF7ARFn;  
   MODFLOW_GWF2WEL7ARFN  			m_GWF2WEL7ARFn;  
   MODFLOW_GWF2DRN7ARFN  			m_GWF2DRN7ARFn;  
   MODFLOW_GWF2RIV7ARFN  			m_GWF2RIV7ARFn;
   MODFLOW_GWF2CHD7ARFN  			m_GWF2CHD7ARFn;
   MODFLOW_GWF2RCH7ARFN  			m_GWF2RCH7ARFn;  
   MODFLOW_GMG7ARFN      			m_GMG7ARFn;      
   MODFLOW_OBS2BAS7ARFN  			m_OBS2BAS7ARFn;  
   MODFLOW_GWF2BAS7STFN  			m_GWF2BAS7STFn;  
   MODFLOW_GWF2WEL7RPFN  			m_GWF2WEL7RPFn;  
   MODFLOW_GWF2DRN7RPFN  			m_GWF2DRN7RPFn;  
   MODFLOW_GWF2RIV7RPFN  			m_GWF2RIV7RPFn;
   MODFLOW_GWF2CHD7RPFN  			m_GWF2CHD7RPFn;  
   MODFLOW_GWF2RCH7RPFN  			m_GWF2RCH7RPFn;  
   MODFLOW_GWF2BAS7ADFN  			m_GWF2BAS7ADFn;  
   MODFLOW_GWF2LPF7ADFN  			m_GWF2LPF7ADFn;
   MODFLOW_GWF2CHD7ADFN  			m_GWF2CHD7ADFn;
   MODFLOW_UMESPRFN      			m_UMESPRFn;      
   MODFLOW_GWF2BAS7FMFN  			m_GWF2BAS7FMFn;  
   MODFLOW_GWF2LPF7FMFN  			m_GWF2LPF7FMFn;  
   MODFLOW_GWF2WEL7FMFN  			m_GWF2WEL7FMFn;  
   MODFLOW_GWF2DRN7FMFN  			m_GWF2DRN7FMFn;  
   MODFLOW_GWF2RIV7FMFN  			m_GWF2RIV7FMFn;  
   MODFLOW_GWF2RCH7FMFN  			m_GWF2RCH7FMFn;  
   MODFLOW_GMG7PNTFN      			m_GMG7PNTFn;      
   MODFLOW_GMG7APFN 				   m_GMG7APFn; 
   MODFLOW_GWF2BAS7OCFN    		m_GWF2BAS7OCFn;   
   MODFLOW_GWF2LPF7BDSFN   		m_GWF2LPF7BDSFn;  
   MODFLOW_GWF2LPF7BDCHFN  		m_GWF2LPF7BDCHFn; 
   MODFLOW_GWF2LPF7BDADJFN 		m_GWF2LPF7BDADJFn;
   MODFLOW_GWF2WEL7BDFN    		m_GWF2WEL7BDFn;   
   MODFLOW_GWF2DRN7BDFN    		m_GWF2DRN7BDFn;   
   MODFLOW_GWF2RIV7BDFN    		m_GWF2RIV7BDFn;   
   MODFLOW_GWF2LAK7STFN    		m_GWF2LAK7STFn;   
   MODFLOW_GWF2RCH7BDFN    		m_GWF2RCH7BDFn;   
   MODFLOW_GWF2BAS7OTFN    		m_GWF2BAS7OTFn;   
   MODFLOW_GLO1BAS6ETFN    		m_GLO1BAS6ETFn;   
   MODFLOW_SGWF2BAS7PNTFN  		m_SGWF2BAS7PNTFn; 
   MODFLOW_GWF2WEL7DAFN    		m_GWF2WEL7DAFn;   
   MODFLOW_GWF2DRN7DAFN    		m_GWF2DRN7DAFn;   
   MODFLOW_GWF2RIV7DAFN    		m_GWF2RIV7DAFn;   
   MODFLOW_GWF2RCH7DAFN    		m_GWF2RCH7DAFn;   
   MODFLOW_GWF2LPF7DAFN    		m_GWF2LPF7DAFn; 
   MODFLOW_GWF2CHD7DAFN    		m_GWF2CHD7DAFn; 
   MODFLOW_OBS2BAS7DAFN    		m_OBS2BAS7DAFn;   
   MODFLOW_GWF2BAS7DAFN    		m_GWF2BAS7DAFn;
   MODFLOW_PCG7ARFN    		      m_PCG7ARFn;
   MODFLOW_PCG7PNTFN    		   m_PCG7PNTFn;
   MODFLOW_PCG7APFN              m_PCG7APFn;
   //MODFLOW_PCG7DAFN              m_PCG7DAFn;
};