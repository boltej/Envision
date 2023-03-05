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
#include <EnvModel.h>

 class EvaluatorLearningStats;

class Sandbox : protected EnvModel
   {
   public:
      Sandbox( EnvModel *pModel, int years = 10, float percentCoverage = 1.0f, bool callInitRun = false );
      ~Sandbox(void);

   private:
      CArray< float, float > m_initialMetrics;  // state of the landscape at creation of sandbox
      
      float m_percentCoverage;
   
   public:
      void  SetYearsToRun( int years ){ m_yearsToRun = years; }
      void  SetPercentCoverage( float percentCoverage ){ m_percentCoverage = percentCoverage; }
      int   GetYearsToRun(){ return m_yearsToRun; }
      float GetPercentCoverage(){ return m_percentCoverage; }

      void CalculateGoalScores( EnvPolicy *pPolicy, bool *useMetagoalsArray ); // NULL says update all policy metagoals; else just where true
      void CalculateGoalScores( CArray<EnvPolicy *>  & schedPolicy, EnvPolicy *pPolicy, EvaluatorLearningStats * emStats, bool *useMetagoalsArray ); // NULL says update all policy metagoals; else just where true

   private:
      // Used Base Methods
      UINT_PTR AddDelta( int cell, int col, int year, VData newValue, int type ){ return EnvModel::AddDelta( cell, col, year, newValue, type ); }
      UINT_PTR ApplyDeltaArray( MapLayer *pLayer ){ return EnvModel::ApplyDeltaArray( pLayer ); }
      UINT_PTR UnApplyDeltaArray( MapLayer *pLayer ){ return EnvModel::UnApplyDeltaArray(pLayer); }
      int   SelectPolicyOutcome( MultiOutcomeArray &m ){ return EnvModel::SelectPolicyOutcome( m ); }
      int   DoesPolicyApply( EnvPolicy *pPolicy, int cell ){ return EnvModel::DoesPolicyApply( pPolicy, cell ); }
           
      float GetLandscapeMetric( int i ){ return EnvModel::GetLandscapeMetric( i ); }

      // Overridden Base Methods
      bool Reset( void );

      // Sandbox Methods
      void InitRunModels();
      void PushModels();
      void PopModels();
   };
