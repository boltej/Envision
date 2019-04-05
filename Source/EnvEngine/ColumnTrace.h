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

#include <Vdataobj.h>

class DeltaArray;
class MapLayer;
class EnvModel;

class ColumnTrace
   {
   public:
      ColumnTrace(EnvModel *pModel, int columnIndex, int run = -1, const DeltaArray *pDeltaArray = NULL );
      ~ColumnTrace();

   public:
      VData Get( int row );
      int   GetCurrentYear(){ return m_currentYear; }
      int   GetRowCount(){ return m_rows; }
      bool  SetCurrentYear( int year );

      bool Get( int row, float &value )         { return m_column.Get( 0, row, value); }
      bool Get( int row, double &value)         { return m_column.Get( 0, row, value); }
      bool Get( int row, COleVariant &value )   { return m_column.Get( 0, row, value); }
      bool Get( int row, VData &value )         { return m_column.Get( 0, row, value); }
      bool Get( int row, int &value )           { return m_column.Get( 0, row, value); }
      bool Get( int row, bool &value )          { return m_column.Get( 0, row, value); }

   private:
      int m_run;
      int m_columnIndex;
      int m_rows;
      int m_currentYear;  // interpret as beginning of year
      int m_maxYear;

      const DeltaArray *m_pDeltaArray;
      VDataObj m_column;
   };
