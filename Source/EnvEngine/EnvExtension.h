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
#include "stdafx.h"
#include "EnvAPI.h"
#include "EnvContext.h"
#include <maplayer.h>

enum { ADD_DELTA=1, SET_DATA=2, FORCE_UPDATE=4 };

// Envision extension types
enum EXT_TYPE
   {
   EET_UNDEFINED = 0,
   EET_MODELPROCESS = 1,
   EET_EVALUATOR = 2,
   EET_ANALYSISMODULE = 4,
   EET_DATAMODULE = 8,
   EET_INPUT_VISUALIZER = 16,
   EET_RT_VISUALIZER = 32,
   EET_POSTRUN_VISUALIZER = 64
   };

// visualizer types.  These populate ENV_VISUALIZER::m_vizType
enum VIZ_TYPE { VZT_UNDEFINED=0, VZT_INPUT = 1, VZT_RUNTIME = 2, VZT_POSTRUN = 4, VZT_POSTRUN_GRAPH = 8, VZT_POSTRUN_MAP = 16, VZT_POSTRUN_VIEW = 32 };

#ifndef NO_MFC
//BOOL CALLBACK MoveChildrenProc( HWND hwnd, LPARAM lParam );
#endif

class ENVAPI EnvExtension
   {
   public:
      EnvExtension();
      EnvExtension( int inputCount, int outputCount );
      EnvExtension(EnvExtension &);
      virtual ~EnvExtension();

   public:
      int       m_id;           // model id as specified by the project file
      CString   m_name;         // name of the model
      CString   m_path;         // path to the dll
      CString   m_initInfo;     // string passed to the model, specified in the project file
      CString   m_description;  // description, from ENVX file
      CString   m_imageURL;     // not currently used
      bool      m_use;          // use this model during the run?
      HINSTANCE m_hDLL;
      EXT_TYPE  m_type;         // Envision Extension type - see EnvContext.h for EET_xxx values; can be multiple types, or'd together
      int       m_handle;       // unique handle for this model/process, passed in the context

   public:
      // Exposed entry points - override these if so desired
      virtual bool Init     ( EnvContext *pEnvContext, LPCTSTR initStr )    { m_initInfo = initStr; return true; }
      virtual bool InitRun  ( EnvContext *pEnvContext, bool useInitialSeed ){ return true; }
      virtual bool Run      ( EnvContext *pContext )                        { return true; }
      virtual bool EndRun   ( EnvContext *pContext )                        { return true; }
      virtual bool Setup    ( EnvContext *pContext, HWND hWnd )      { return false; }
      virtual bool GetConfig(std::string &configStr) { return false; }
      virtual bool SetConfig(EnvContext*, LPCTSTR configStr) { return false; }
      virtual int  InputVar ( int id, MODEL_VAR** modelVar );
      virtual int  OutputVar( int id, MODEL_VAR** modelVar );

      int GetInputVarCount()  { return (int) m_inputVars.GetSize(); }
      int GetOutputVarCount() { return (int) m_outputVars.GetSize(); }

      // call this if you HAVEN'T allocated variables in the constructor
      int AddVar( bool input, LPCTSTR name, void *pVar, TYPE type, MODEL_DISTR distType, VData paramLocation, VData paramScale, VData paramShape, LPCTSTR desc, int flags );
      // overrides
      int AddVar( bool input, LPCTSTR name, float  &var, LPCTSTR description, int flags ) { return AddVar( input, name, &var, TYPE_FLOAT,  MD_CONSTANT, VData( var ), VData(), VData(), description, flags ); }
      int AddVar( bool input, LPCTSTR name, int    &var, LPCTSTR description, int flags ) { return AddVar( input, name, &var, TYPE_INT,    MD_CONSTANT, VData( var ), VData(), VData(), description, flags ); }
      int AddVar( bool input, LPCTSTR name, double &var, LPCTSTR description, int flags ) { return AddVar( input, name, &var, TYPE_DOUBLE, MD_CONSTANT, VData( var ), VData(), VData(), description, flags ); }
      int AddVar( bool input, LPCTSTR name, bool   &var, LPCTSTR description, int flags ) { return AddVar( input, name, &var, TYPE_BOOL,   MD_CONSTANT, VData( var ), VData(), VData(), description, flags ); }

      // more specialized versions
      int AddInputVar( LPCTSTR name, float  &var, LPCTSTR description, int flags=0 ) { return AddVar( true, name, &var, TYPE_FLOAT,  MD_CONSTANT, VData( var ), VData(), VData(), description, flags ); }
      int AddInputVar( LPCTSTR name, int    &var, LPCTSTR description, int flags=0 ) { return AddVar( true, name, &var, TYPE_INT,    MD_CONSTANT, VData( var ), VData(), VData(), description, flags ); }
      int AddInputVar( LPCTSTR name, double &var, LPCTSTR description, int flags=0 ) { return AddVar( true, name, &var, TYPE_DOUBLE, MD_CONSTANT, VData( var ), VData(), VData(), description, flags ); }
      int AddInputVar( LPCTSTR name, bool   &var, LPCTSTR description, int flags=0 ) { return AddVar( true, name, &var, TYPE_BOOL,   MD_CONSTANT, VData( var ), VData(), VData(), description, flags ); }

	   int AddOutputVar( LPCTSTR name, float  &var, LPCTSTR description, int flags=0 ) { return AddVar( false, name, &var, TYPE_FLOAT,   MD_CONSTANT, VData( var ), VData(), VData(),  description, flags ); }
      int AddOutputVar( LPCTSTR name, int    &var, LPCTSTR description, int flags=0 ) { return AddVar( false, name, &var, TYPE_INT,     MD_CONSTANT, VData( var ), VData(), VData(),  description, flags ); }
      int AddOutputVar( LPCTSTR name, double &var, LPCTSTR description, int flags=0 ) { return AddVar( false, name, &var, TYPE_DOUBLE,  MD_CONSTANT, VData( var ), VData(), VData(),  description, flags ); }
      int AddOutputVar( LPCTSTR name, bool   &var, LPCTSTR description, int flags=0 ) { return AddVar( false, name, &var, TYPE_BOOL,    MD_CONSTANT, VData( var ), VData(), VData(),  description, flags ); }
      int AddOutputVar( LPCTSTR name, DataObj*var, LPCTSTR description, int flags=0 ) { return AddVar( false, name, var,  TYPE_PDATAOBJ, MD_CONSTANT, VData( var ), VData(), VData(), description, flags ); }

      MODEL_VAR *FindOutputVar(LPCTSTR name);


      // call this if you HAVE allocated variables in the constructor
      bool DefineVar( bool input, int index, LPCTSTR name, void *pVar, TYPE type, MODEL_DISTR distType, VData paramLocation, VData paramScale, VData paramShape, LPCTSTR desc );
      // overrides
      bool DefineVar( bool input, int index, LPCTSTR name, float  &var, LPCTSTR description ) { return DefineVar( input, index, name, &var, TYPE_FLOAT,  MD_CONSTANT, VData( var ), VData(), VData(), description ); }
      bool DefineVar( bool input, int index, LPCTSTR name, int    &var, LPCTSTR description ) { return DefineVar( input, index, name, &var, TYPE_INT,    MD_CONSTANT, VData( var ), VData(), VData(), description ); }
      bool DefineVar( bool input, int index, LPCTSTR name, double &var, LPCTSTR description ) { return DefineVar( input, index, name, &var, TYPE_DOUBLE, MD_CONSTANT, VData( var ), VData(), VData(), description ); }
      bool DefineVar( bool input, int index, LPCTSTR name, bool   &var, LPCTSTR description ) { return DefineVar( input, index, name, &var, TYPE_BOOL,   MD_CONSTANT, VData( var ), VData(), VData(), description ); }

   public:
      INT_PTR AddDelta( EnvContext *pContext, int idu, int col, VData  newValue );
      // overrides
      INT_PTR AddDelta( EnvContext *pContext, int idu, int col, double newValue ) { return AddDelta(pContext, idu, col, VData(newValue)); }
      INT_PTR AddDelta( EnvContext *pContext, int idu, int col, float  newValue ) { return AddDelta( pContext, idu, col, VData( newValue ) ); }
      INT_PTR AddDelta( EnvContext *pContext, int idu, int col, int    newValue ) { return AddDelta( pContext, idu, col, VData( newValue ) ); }
      INT_PTR AddDelta( EnvContext *pContext, int idu, int col, LPCTSTR newValue ) { return AddDelta( pContext, idu, col, VData( newValue, true ) ); }
      INT_PTR AddDelta( EnvContext *pContext, int idu, int col, bool   newValue ) { return AddDelta( pContext, idu, col, VData( newValue ) ); }

   public:
      // CheckCol checks to see if a columns exists in a maplayer.  It can optionally add it.
      // For flags, see CC_xxx enum above
      static bool CheckCol( const MapLayer *pLayer, int &col, LPCTSTR label, TYPE type, int flags );
  
      bool UpdateIDU( EnvContext *pContext, int idu, int col, VData &value, int flags=ADD_DELTA );
      bool UpdateIDU( EnvContext *pContext, int idu, int col, float value,  int flags=ADD_DELTA ) { return _UpdateIDU<float> ( pContext, idu, col, value, flags ); }
      bool UpdateIDU( EnvContext *pContext, int idu, int col, int value,    int flags=ADD_DELTA ) { return _UpdateIDU<int>   ( pContext, idu, col, value, flags ); }
      bool UpdateIDU( EnvContext *pContext, int idu, int col, double value, int flags=ADD_DELTA ) { return _UpdateIDU<double>( pContext, idu, col, value, flags ); }
      bool UpdateIDU( EnvContext *pContext, int idu, int col, short value,  int flags=ADD_DELTA ) { return _UpdateIDU<short> ( pContext, idu, col, value, flags ); }
      bool UpdateIDU( EnvContext *pContext, int idu, int col, bool value,   int flags=ADD_DELTA ) { return _UpdateIDU<bool>  ( pContext, idu, col, value, flags ); }
      template <class T> bool _UpdateIDU(EnvContext *pContext, int idu, int col, T value, int flags = ADD_DELTA);
   protected:
      CArray< MODEL_VAR, MODEL_VAR& > m_inputVars;
      CArray< MODEL_VAR, MODEL_VAR& > m_outputVars;

   };

template <class T>
inline bool EnvExtension::_UpdateIDU(EnvContext *pContext, int idu, int col, T value, int flags)
   {
   if ((flags & (ADD_DELTA | SET_DATA)) == 0)
      {
      Report::LogWarning(_T("UpdateIDU() - invalid flag encountered - must be ADD_DELTA and/or SET_DATA"));
      return false;
      }

   bool retVal = true;
   if (flags & ADD_DELTA)
      {
      if (flags & FORCE_UPDATE)
         this->AddDelta(pContext, idu, col, value);

      else
         {
         T oldValue;
         pContext->pMapLayer->GetData(idu, col, oldValue);

         if (oldValue != value)
            this->AddDelta(pContext, idu, col, value);
         else
            retVal = false;
         }
      }

   if (flags & SET_DATA)
      {
      if (pContext->pMapLayer->m_readOnly)
         {
         if (flags & FORCE_UPDATE)
            {
            ((MapLayer*)pContext->pMapLayer)->m_readOnly = false;
            ((MapLayer*)pContext->pMapLayer)->SetData(idu, col, value);
            ((MapLayer*)pContext->pMapLayer)->m_readOnly = true;
            }
         else
            retVal = false;
         }
      else
         ((MapLayer*)pContext->pMapLayer)->SetData(idu, col, value);
      }

   return retVal;
   }


class ENVAPI EnvProcess : public EnvExtension
   {
   public:
      EnvProcess(int inputCount, int outputCount) 
         : EnvExtension(inputCount, outputCount), 
         m_timing(0), m_frequency(0), m_runTime(0), m_dependencyCount(0), m_dependencyArray(nullptr) {}

      EnvProcess() : EnvExtension(), m_timing(0), m_frequency(0), m_runTime(0), m_dependencyCount(0), m_dependencyArray(nullptr) {}

      int   m_timing;       // 0=pre,1=post
      int   m_frequency;    // how often this process is executed
      float m_runTime;      // amount of time this process has executed
      CString m_dependencyNames;
      int m_dependencyCount;
      EnvProcess **m_dependencyArray;
   };


class ENVAPI EnvModelProcess : public EnvProcess
   {
   public:
      EnvModelProcess( int inputCount, int outputCount )
         : EnvProcess(inputCount, outputCount)
         {
         m_handle = m_nextHandle++;
         m_type = EET_MODELPROCESS;
         }

      EnvModelProcess() 
         : EnvProcess()
         {
         m_handle = m_nextHandle++;
         m_type = EET_MODELPROCESS;
         }

      virtual ~EnvModelProcess( void ) { }

      static int m_nextHandle;
   };


class ENVAPI EnvEvaluator : public EnvProcess
   {
   public:
      EnvEvaluator(int inputCount, int outputCount)
         : EnvProcess(inputCount, outputCount)
         , m_showInResults()
         , m_score(0)
         , m_rawScore(0)
         , m_pDataObj(nullptr)
         , m_rawScoreMin(0)
         , m_rawScoreMax(0)
         {
         m_handle = m_nextHandle++;
         m_type = EET_EVALUATOR;
         }

      EnvEvaluator()
         : EnvProcess()
         , m_showInResults(0)
         , m_score(0)
         , m_rawScore(0)
         , m_pDataObj(nullptr)
         , m_rawScoreMin(0)
         , m_rawScoreMax(0)
         {
         m_handle = m_nextHandle++;
         m_type = EET_EVALUATOR;
         }

      virtual ~EnvEvaluator(void) {}

      int ScaleScore(float & arg) const;


      int        m_showInResults;        // determines whether this model is used in the presentation of results
                                       // 0=don't show anywhere, 1=show everywhere, 2=show where unscaled (beyond [-3..3]) can be shown, 4=show as pie chart (default is scatter).
      float      m_score;                // current value of the abundance metric (-3 to +3)
      float      m_rawScore;             // current value of abundance metric (nonscaled)
      FDataObj  *m_pDataObj;             // current (optional) data object returned by the model (NULL if not used)
      float      m_rawScoreMin;
      float      m_rawScoreMax;
      CString    m_rawScoreUnits;        // string decribing units of raw score

      static int m_nextHandle;
   };



// A visualizer is an object that manages the visual display of data.  The basic idea is that
// the visualizer plugin provides for the display of (potentionally multiple) windows of data.
// Envision manages the windows associated with a visualizer; the visualizer draws in/manages this window
// as needed.

class ENVAPI EnvVisualizer : public EnvExtension
   {
   public:
      EnvVisualizer() : EnvExtension(), m_inRegistry(false), m_vizType(VZT_UNDEFINED) { }
      virtual ~EnvVisualizer( void ) { }
      
      //---------------------------------------------------------------------                    INPUT        RUNTIME      POSTRUN
      virtual bool Init        ( EnvContext*, LPCTSTR ) { return true; }                   // this should alway be called by children if they override
      virtual bool InitRun     ( EnvContext*, bool )    { return true; }      // for init of run         optional      optional     optional
      virtual bool Run         ( EnvContext* )          { return true; }      // for runtime views       optional      required     optional

      virtual bool InitWindow  ( EnvContext*, HWND )    { return true; }
      virtual bool UpdateWindow( EnvContext*, HWND )    { return true; }

      bool m_inRegistry;
      VIZ_TYPE m_vizType;   // see VZT_XXXX
   };


class ENVAPI EnvRtVisualizer : public EnvVisualizer
   {
   public:
      EnvRtVisualizer( void ) : EnvVisualizer( )
         {
         m_type = EET_RT_VISUALIZER;
         m_vizType = VZT_RUNTIME;
         }
      virtual ~EnvRtVisualizer( void ) { }
   };


class ENVAPI EnvInputVisualizer : public EnvVisualizer
   {
   public:
      EnvInputVisualizer( void ) : EnvVisualizer()
         {
         m_type = EET_INPUT_VISUALIZER;
         m_vizType = VZT_INPUT;
         }
      virtual ~EnvInputVisualizer( void ) { }
   };


class ENVAPI EnvPostRunVisualizer : public EnvVisualizer
   {
   public:
      EnvPostRunVisualizer( void ) : EnvVisualizer()
         {
         m_type = EET_POSTRUN_VISUALIZER;
         m_vizType = VZT_POSTRUN;
         }
      virtual ~EnvPostRunVisualizer( void ) { }
   };


// A visualizer is an object that manages the visual display of data.  The basic idea is that
// the visualizer plugin provides for the display of (potentionally multiple) windows of data.
// Envision manages the windows associated with a visualizer; the visualizer draws in/manages this window
// as needed.

class ENVAPI EnvAnalysisModule : public EnvExtension
   {
   public:
      EnvAnalysisModule() 
         : EnvExtension()
         , m_inRegistry(false)
         { m_type = EET_ANALYSISMODULE; }

      virtual ~EnvAnalysisModule(void) {}

      bool m_inRegistry;
   };

class ENVAPI EnvDataModule : public EnvExtension
   {
   public:
      EnvDataModule() 
         : EnvExtension()
         , m_inRegistry(false)
         { m_type = EET_DATAMODULE; }

      virtual ~EnvDataModule(void) {}

      bool m_inRegistry;
   };
