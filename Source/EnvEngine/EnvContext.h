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
// EnvContext.h
//
///////////////////////////////////////////////////////

#if !defined (__ENVCONTEXT_H__INCLUDED_)
#define __ENVCONTEXT_H__INCLUDED_

#include <Vdata.h>
#include <string>

class EnvExtension;
class ActorManager;
class PolicyManager;
class DeltaArray;
class MapLayer;
class LulcTree;
class MapExprEngine;
class QueryEngine;
class EnvModel;
class EnvContext;
class FDataObj;
class Scenario;

struct MODEL_VAR;



struct ENV_EXT_INFO
{
public:
   ENV_EXT_INFO( void ) : types( 0 ), extra( 0 ) { }

   int     types;     // or'd combination of Envision extension types given above;
   CString description;

   long    extra;
};


//-----------------------------------------------------------------------------
//--------------- I N F O   I N T E R F A C E ---------------------------------
//-----------------------------------------------------------------------------
typedef void (PASCAL *GETEXTINFOFN) ( ENV_EXT_INFO* );


//-----------------------------------------------------------------------------
//------------------ E X T E N S I O N    I N T E R F A C E S -----------------
//-----------------------------------------------------------------------------
typedef EnvExtension* (PASCAL *FACTORYFN)(EnvContext*);


//-----------------------------------------------------------------------------
//--------- A N A L Y S I S   M O D U L E    I N T E R F A C E S --------------
//-----------------------------------------------------------------------------
typedef bool (PASCAL *ANALYSISMODULEFN)(EnvContext*, HWND, LPCTSTR initInfo );


//-----------------------------------------------------------------------------
//-------------- D A T A  M O D U L E    I N T E R F A C E S ------------------
//-----------------------------------------------------------------------------
typedef bool (PASCAL *DATAMODULEFN)(EnvContext*, HWND, LPCTSTR initInfo );


//-----------------------------------------------------------------------------
//--------------- V I S U A L I Z E R   I N T E R F A C E S -------------------
//-----------------------------------------------------------------------------        INPUT        RUNTIME      POSTRUN
typedef bool (PASCAL *INITWNDFN)  (EnvContext*, HWND );     //       required      optional     optional
typedef bool (PASCAL *UPDATEWNDFN)(EnvContext*, HWND );     // for init of run         optional      optional     optional


//-----------------------------------------------------------------------------
//--------- A C T O R   D E C I S I O N   I N T E R F A C E -------------------
//-----------------------------------------------------------------------------
typedef bool (PASCAL *UTILITYFN)(EnvContext* );


//-----------------------------------------------------------------------------
//--------------- I N T E R F A C E    S T R U C T U R E S  -------------------
//-----------------------------------------------------------------------------
/*

struct ENV_VISUALIZER : public EnvExtension
   {
   int  type;  // VT_INPUT, VT_RTVIEW, ...
   bool inRegistry;

   INITFN      initFn;
   INITRUNFN   initRunFn;
   RUNFN       runFn;
   ENDRUNFN    endRunFn;
   //VPOSTRUNFN    postRunFn; 
   INITWNDFN   initWndFn;
   UPDATEWNDFN updateWndFn;
 
   ENV_VISUALIZER( HINSTANCE _hDLL, 
               RUNFN       _runFn,
               INITFN      _initFn,
               INITRUNFN   _initRunFn, 
               ENDRUNFN    _endRunFn,
               INITWNDFN   _initWndFn,
               UPDATEWNDFN _updateWndFn,
               //VPOSTRUNFN  _postRunFn,
               SETUPFN _showPropertiesFn,
               GETCONFIGFN _getConfigFn,
               SETCONFIGFN _setConfigFn,
               int     _id,
               int     _type,
               LPCTSTR _initFnInfo,
               LPCTSTR _name, 
               LPCTSTR _path,
               LPCTSTR _description,
               LPCTSTR _imageURL,
               bool    _inRegistry )
          : EnvExtension( _hDLL, _showPropertiesFn, _getConfigFn, _setConfigFn, _id, _initFnInfo, _name, _path, _description, _imageURL )
          , type( _type )
          , inRegistry( _inRegistry )
          , runFn ( _runFn )
          , initFn( _initFn )
          , initRunFn( _initRunFn )
          //, postRunFn( _postRunFn )
          , endRunFn( _endRunFn )
          , initWndFn( _initWndFn )
          , updateWndFn( _updateWndFn )
         { }

   virtual ~ENV_VISUALIZER() { }
   };

*/
/*
class ENV_ANALYSISMODULE : public EnvExtension
   {
   int type;              // reserved
   bool inRegistry;

   ANALYSISMODULEFN runFn;

   ENV_ANALYSISMODULE( HINSTANCE _hDLL, 
               ANALYSISMODULEFN _runFn,
               int    _id,
               int    _type,
               LPCTSTR _name, 
               LPCTSTR _path,
               LPCTSTR _imageURL,
               LPCTSTR _description,
               bool   _inRegistry )
          : EnvExtension( _hDLL, NULL, NULL, NULL, _id, NULL, _name, _path, _description, _imageURL )
          , type( 0 )
          , inRegistry( _inRegistry )
          , runFn( _runFn )
          { }

   virtual ~ENV_ANALYSISMODULE() { }
   };



struct ENV_DATAMODULE : public EnvExtension
   {
   int  type;
   bool inRegistry;

   DATAMODULEFN runFn;

   ENV_DATAMODULE( HINSTANCE _hDLL, 
               DATAMODULEFN _runFn,
               int    _id,
               int    _type,
               LPCTSTR _name, 
               LPCTSTR _path,
               LPCTSTR _description,
               LPCTSTR _imageURL,
               bool   _inRegistry )
          : EnvExtension( _hDLL, NULL,NULL, NULL, _id, NULL, _name, _path, _description, _imageURL )
          , type( 0 )
          , inRegistry( _inRegistry )
          , runFn( _runFn )
          { }

   virtual ~ENV_DATAMODULE() { }
   };
*/

//-----------------------------------------------------------------------------
//------------ E N V C O N T E X T   D E F I N I T I O N ----------------------
//-----------------------------------------------------------------------------

class EnvContext
{
public:
   INT_PTR         (*ptrAddDelta)(EnvModel *pModel,  int cell, int col, int year, VData newValue, int handle );
   int             startYear;             // calendar year in which the simulation started (e.g. 2008)
   int             endYear;               // last calendar year of simulation (e.g. 2050)
   int             currentYear;           // current calendar year of run, incremented from startYear (e.g. 2010)
   int             yearOfRun;             // 0-based year of run  (e.g. 1)
   int             run;                   // 0-based, incremented after each run.
   int             scenarioIndex;         // 0-based scenario index 
   bool            showMessages;
   int             logMsgLevel;           // see flags in envmodel.h
   int			    exportMapInterval;     // maps to 
   const MapLayer *pMapLayer;             // pointer to the IDU layer.  This is const because extensions generally shouldn't write to this.
	ActorManager   *pActorManager;         // pointer to the actor manager
	PolicyManager  *pPolicyManager;        // pointer to the policy manager
   DeltaArray     *pDeltaArray;           // pointer to the current delta array
   EnvModel       *pEnvModel;             // pointer to the current model
   LulcTree       *pLulcTree;             // pointer to the lulc tree used in the simulation
   QueryEngine    *pQueryEngine;          // pointer to an query engine  (deprecated, now bundled with each maplayer)
   MapExprEngine  *pExprEngine;           // pointer to an expression engine (ditto)
   Scenario       *pScenario;             // pointer to current scenario 
   int             id;                    // id of the module being called
   INT_PTR         handle;                // unique handle of the module
   int             col;                   // database column to populate, -1 for no column
   LPCTSTR         initInfo;
   INT_PTR         firstUnseenDelta;      // models should start scanning the deltaArray here
   INT_PTR         lastUnseenDelta;       // models should stop scanning the delta array here.  This will be the index of the last delta
   bool           *targetPolyArray;       // array of polygon indices with true/false to process (NULL=process all)
   int             targetPolyCount;       // number of elements in targetPolyarray (0 to run everywhere
   EnvExtension   *pEnvExtension;         // opaque ptr to EnvEvaluator, EnvModelProcess, etc.
#ifndef NO_MFC
   CWnd           *pWnd;                  // relevant window, NULL or type depends on context
#endif

   // these values are set and returned by eval models, ignored by autonomous processes
   float           score;                 // overall score for this evaluation, -3 to +3 (unused by AP's)
   float           rawScore;              // model-specific raw score ( not normalized )

   FDataObj       *pDataObj;              // data object returned from the model at each time step (optional, NULL if not used)
   INT_PTR         extra;                 // extra data - depends on type

	EnvContext( MapLayer *_pMapLayer )
		: ptrAddDelta( NULL )
      , startYear( 0 )
      , endYear( 0 )
      , currentYear( 0 )
      , yearOfRun( 0 )
      , run( -1 )
      , scenarioIndex( -1 )
      , showMessages( true )
      , logMsgLevel( 0 )
      , pMapLayer(_pMapLayer)
      , pActorManager( NULL )
      , pPolicyManager( NULL )
      , pDeltaArray( NULL )
      , pEnvModel( NULL )
      , pLulcTree( NULL )
      , pQueryEngine( NULL )
      , pExprEngine( NULL )
      , pScenario(NULL)
      , col( -1 )
      , id( -1 )
      , handle( -1 )
      , firstUnseenDelta( -1 )
      , lastUnseenDelta( -1 )
      , pEnvExtension( NULL )
#ifndef NO_MFC
      , pWnd( NULL )
#endif
      , targetPolyArray( NULL )
      , targetPolyCount( 0 )
      , score( 0 )
      , rawScore( 0 )
      , pDataObj( NULL )
      , exportMapInterval(0)
      , extra(0)
      { }
};


//-----------------------------------------------------------------------------
//--------- E X P O S E D    M O D E L    V A R I A B L E S -------------------
//-----------------------------------------------------------------------------

enum MODEL_DISTR { MD_CONSTANT=0, MD_UNIFORM=1, MD_NORMAL=2, MD_LOGNORMAL=3, MD_WEIBULL=4 };

enum { MVF_LEVEL_0=0, MVF_LEVEL_1=1 };  // Model Variable Flags 


struct MODEL_VAR
{
public:
   CString name;              // name of the variable
   void   *pVar;              // address of the model variable. 
   TYPE    type;              // type of the address variable (see typedefs.h)
   MODEL_DISTR distType;      // destribution type (see enum above)
   VData   paramLocation;     // value of the location parameter
   VData   paramScale;        // value of the scale parameter   
   VData   paramShape;        // value of the shape parameter 
   CString description;       // description of the model variable
   int     flags;             // default=MVF_LEVEL_0, MVF_LEVEL_1 
   INT_PTR extra;             // if flags > 0, contains ptr to containing???
   int     useInSensitivity;
   VData   saValue;  
   // e.g. ~Normal(location, shape), 

   MODEL_VAR () : 
         name("uninitialized"),
         pVar(NULL),
         type(TYPE_NULL),
         distType(MD_CONSTANT),
         paramLocation(),//-DBL_MAX),
         paramScale(),
         paramShape(),
         description("uninitialized"),
         flags( 0 ),
         extra( 0 ),
         useInSensitivity( 0 ),
         saValue( 0 )
         {}

   MODEL_VAR(   
      const CString & name_,
      void *pVar_,
      TYPE type_,
      MODEL_DISTR distType_,
      VData paramLocation_,
      VData paramScale_,
      VData paramShape_,
      const CString & description_,
      int   flags_
      ) :
         name(name_),
         pVar(pVar_),
         type(type_),
         distType(distType_),
         paramLocation(paramLocation_),
         paramScale(paramScale_),
         paramShape(paramShape_),
         description(description_),
         flags( flags_ ),
         extra( 0 ),
         useInSensitivity( 0 ),
         saValue( 0 )
      {}

   MODEL_VAR ( const MODEL_VAR &mv ) : 
         name( mv.name ),
         pVar( mv.pVar ),
         type( mv.type ),
         distType( mv.distType ),
         paramLocation( mv.paramLocation ),
         paramScale( mv.paramScale ),
         paramShape( mv.paramShape ),
         description( mv.description ),
         flags( mv.flags ),
         extra( mv.extra ),
         useInSensitivity( 0 ),
         saValue( 0 )
         {} 
};



#endif
