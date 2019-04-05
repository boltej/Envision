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

#include <Maplayer.h>
#include "EnvAPI.h"

class Map;
class EnvModel;
class PolicyManager;
class ActorManager;
class ScenarioManager;
class QueryEngine;
class TiXmlElement;



enum AML_TYPE { AMLT_INFER=-1, AMLT_SHAPE=0, AMLT_INT_GRID=1, AMLT_FLOAT_GRID=2 };
class ENVAPI EnvLoader
   {
   public:
      EnvLoader( void );
      ~EnvLoader();

      int LoadProject(LPCTSTR filename, Map *pMap, EnvModel *pEnvModel);
      int LoadLayer( Map *pMap, LPCTSTR name, LPCTSTR path, AML_TYPE type, int red, int green, int blue, int extraCols, int records, LPCTSTR initField, LPCTSTR overlayFields, bool loadFieldInfo, bool expandLegend );
      
      //int LoadFieldInfoXml( MapLayer *pLayer, LPCTSTR _filename );

      bool LoadEvaluator(EnvModel *pEnvModel, LPCTSTR name, LPCTSTR path, LPCTSTR description, LPCTSTR imageURL, int id, int use, int showInResults, LPCTSTR initInfo, LPCTSTR dependencyNames);
      bool LoadModel(EnvModel *pEnvModel, LPCTSTR name, LPCTSTR path, LPCTSTR description, LPCTSTR imageURL, int id, int use, int timing, LPCTSTR initInfo, LPCTSTR dependencyNames);

   protected:
      Map             *m_pMap;      
      MapLayer        *m_pIDULayer;          // set during loading of project file
      EnvModel        *m_pEnvModel;
      QueryEngine     *m_pQueryEngine;       // set during load of project file

      //bool m_areModelsInitialized;
      
   public:

   protected:
      //int LoadFieldInfoXml( TiXmlElement *pXmlElement, MAP_FIELD_INFO *pParent, MapLayer *pLayer, LPCTSTR filename );
      bool LoadExtension( LPCTSTR name, LPCTSTR path, LPCTSTR description, LPCTSTR imageURL, int modelID, int use, int showInResults, int decisionUse, int useCol, LPCTSTR fieldname, LPCTSTR initInfo, float gain, float offset );
      bool LoadVisualizer(LPCTSTR name, LPCTSTR path, LPCTSTR description, LPCTSTR imageURL, int use, int type, int vizID, LPCTSTR initInfo);

      //int  ReconcileDataTypes( void );
      void SetDependencies( void );

      void InitVisualizers( void );

      // Note: valid 'types' are -1=detect from extension, 0=shape, 1=integer grid, 2=float grid
      MapLayer *AddMapLayer( LPCTSTR _path, AML_TYPE type, int extraCols, int records, bool loadFieldInfo, bool expandLegend );

      int AddGDALPath();
   };

