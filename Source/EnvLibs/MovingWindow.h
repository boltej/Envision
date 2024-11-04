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
#include "EnvLibs.h"

#include <float.h>



class LIBSAPI MovingWindow
{
public:   
   MovingWindow( int windowLength );
   ~MovingWindow( void );

   void AddValue( float value );
   
   float GetAvgValue( void );
   float GetMaxValue(void);
   float GetMinValue(void);
   float GetFreqValue(void);
   bool  GetAvgValue(int interval, float &result);

   bool Clear();
      
protected:
   int m_windowLength = 5;
   int m_currentLength = 0;
   float *m_values = nullptr;
};

