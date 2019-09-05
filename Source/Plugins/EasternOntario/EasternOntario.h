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
#include <PtrArray.h>


class WildlifeModel;
class PModel;
class NModel;
class ClimateIndicators;
class TiXmlElement;
class NAESI;
class CropRotationProcess;
class LivestockProcess;

enum { WILDLIFE=0, PHOSPHORUS=1, NITROGEN=2 , CLIMATE=3, NAESI_HABITAT=4, CROP_ROTATION=5, LIVESTOCK=6, REPORT=100 };


// info about an SLC - this is the model reporting unit
class SLC
{
public:
   SLC( void ) : m_score( 0 ), m_area( 0 ),m_slcNumber(0) { }

   float m_score;
   float m_area;
   int m_slcNumber;
   CUIntArray m_iduArray;     // list of idus assocaited with this array
};

class Animal
{
public:
   int m_id;
   CString m_field;
   int m_col;
   CString m_name;
   float m_wt;
   float m_manure;      // kg manure/animal/year
   float m_pmanure;
   float m_nmanure;
   
   Animal( int id, LPCTSTR name ) : m_id( id ), m_name( name ), m_col( -1 ), m_wt( 0 ), m_manure( 0 ), m_nmanure( 0 ), m_pmanure( 0 )
      { }

};

class EasternOntarioModel : public EnvEvaluator
{
public:
   EasternOntarioModel();
   ~EasternOntarioModel( void );

   bool Init   ( EnvContext *pEnvContext, LPCTSTR initStr );
   bool InitRun( EnvContext *pEnvContext, bool useInitialSeed );
   bool Run    ( EnvContext *pContext );
   //BOOL Setup( EnvContext *pContext, HWND hWnd )      { return FALSE; }
   int  InputVar ( int id, MODEL_VAR** modelVar );
   int  OutputVar( int id, MODEL_VAR** modelVar );
   
   SLC *GetSLC( int i ) { return m_slcArray[ i ]; }
   int  GetSLCCount( void ) { return (int) m_slcArray.GetSize(); }
   int  m_colSLC;    // SLC column - used by NAHARP model as a reporting unit
   int m_colArea;
   int m_colSlcNum;
      
protected:
   // SLC mapping
   CMap< int, int, int, int > m_slcIDtoIndexMap;  // slcID to index in SLC array
   PtrArray< SLC > m_slcArray;

   // models
   WildlifeModel *m_pWildlife;
   PModel        *m_pPhosphorus;     // IROWC-P Phosphorus model
   NModel        *m_pNitrogen;       // IROWC-N Nitrogen model
   ClimateIndicators *m_pClimateIndicators;
   NAESI *m_pNAESI;
   //CropRotationModel  *m_pCropRotation;

public:
   //int m_outVarIndexCropRotation;
   //int m_outVarCountCropRotation;

   int m_outVarIndexWildlife;
   int m_outVarCountWildlife;

   int m_outVarIndexPModel;
   int m_outVarCountPModel;

   int m_outVarIndexNModel;
   int m_outVarCountNModel;

   int m_outVarIndexClimate;
   int m_outVarCountClimate;

   int m_outVarIndexNaesi;
   int m_outVarCountNaesi;

protected:
   // helper methods
   bool LoadXml( EnvContext *pContext, LPCTSTR filename, MapLayer *pLayer );
   bool BuildSLCs( MapLayer *pLayer );
   bool BuildPopDens( MapLayer *pLayer );

   bool PopulateSLCAreas( MapLayer *pLayer, int colLulc );  // populate SLC lulc percentages

 
   };



class EasternOntarioProcess : public EnvModelProcess
{
public:
   EasternOntarioProcess();
   ~EasternOntarioProcess( void );

   bool Init   ( EnvContext *pEnvContext, LPCTSTR initStr );
   bool InitRun( EnvContext *pEnvContext, bool useInitialSeed );
   bool Run    ( EnvContext *pContext );
   //BOOL Setup( EnvContext *pContext, HWND hWnd )      { return FALSE; }
   int  InputVar ( int id, MODEL_VAR** modelVar );
   int  OutputVar( int id, MODEL_VAR** modelVar );
   
   //SLC *GetSLC( int i ) { return m_slcArray[ i ]; }
   //int  GetSLCCount( void ) { return (int) m_slcArray.GetSize(); }
   //int  m_colSLC;    // SLC column - used by NAHARP model as a reporting unit
   //int m_colArea;
   //int m_colSlcNum;

public:
   // SLC mapping
   //CMap< int, int, int, int > m_slcIDtoIndexMap;  // slcID to index in SLC array
   //PtrArray< SLC > m_slcArray;

   // models
   CropRotationProcess  *m_pCropRotation;
   LivestockProcess     *m_pLivestock;

public:
   int m_outVarIndexCropRotation;
   int m_outVarCountCropRotation;

   int m_outVarIndexLivestock;
   int m_outVarCountLivestock;
protected:
   // helper methods
   bool LoadXml( EnvContext *pContext, LPCTSTR filename, MapLayer *pLayer );
   //bool BuildSLCs( MapLayer *pLayer );
   //
   //bool PopulateSLCAreas( MapLayer *pLayer, int colLulc );  // populate SLC lulc percentages
 
   };




class EasternOntarioReport : public EnvModelProcess
{
public:
   EasternOntarioReport();
   //~EasternOntarioReport( void );

   bool Init   ( EnvContext *pEnvContext, LPCTSTR initStr );
   bool InitRun( EnvContext *pEnvContext, bool useInitialSeed );
   bool Run    ( EnvContext *pContext );

   int m_colArea;
   int m_colLulc;

   int m_colDairyAvg;
   int m_colBeefAvg;
   int m_colPigsAvg;
   int m_colPoultryAvg;

   float m_totalIDUArea;
   float m_totalIDUAreaAg;
   
   float m_pctCashCrop;
   float m_pctHayPasture;

   float m_cashCropRatio;
   float m_hayPastureRatio;

   float m_hayPastureAreaHa;
   float m_bioenergyAreaHa;
   float m_cropRotationAreaHa;
   float m_fruitAreaHa;
   float m_vegAreaHa;

   float m_beefAreaHa;
   float m_dairyAreaHa;
   float m_pigAreaHa;
   float m_poultryAreaHa;

   float m_dairyCount;
   float m_beefCount;
   float m_poultryCount;
   float m_pigCount;
   
   bool IsCashCrop( int lulcC );
   
};


