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
#pragma hdrstop

#include "MovingWindow.h"

MovingWindow::MovingWindow( int windowLength ) 
: m_windowLength( windowLength )
, m_currentLength(0)
, m_values( NULL )
   {
   ASSERT( windowLength > 0 );

   m_values = new float[ windowLength ];

   for ( int i=0; i < windowLength; i++ )
      m_values[ i ] = 0.0f;
   }
   

MovingWindow::~MovingWindow( void )
   {
   if ( m_values != NULL ) 
      delete [] m_values;
   }

void MovingWindow::AddValue( float value )  // push on back, pop front
   {
   for ( int i=0; i < m_windowLength-1; i++ ) 
      m_values[ i ] = m_values[ i+1 ]; 
   
   if (m_currentLength < m_windowLength)
      m_currentLength++;

   m_values[ m_windowLength-1 ] = value;
   }


// Get the average value within a window
float MovingWindow::GetAvgValue( void )
   {
   float value = 0.0f; 
   for (int i=0; i < m_windowLength; i++ ) 
      value += m_values[ i ]; 
   
   return value / m_currentLength; 
   }

// Get the largest value within a window
float MovingWindow::GetMaxValue(void)
{
   float value = -FLT_MAX;
   for (int i = 0; i < m_windowLength; i++)
   { 
      if (m_values[ i ] > value)
         value = m_values[ i ];
   }

   return value;
}

// Get the minimum value within a window
float MovingWindow::GetMinValue(void)
{
   float value = FLT_MAX;
   for (int i = 0; i < m_windowLength; i++)
   {
      if (m_values[i] < value)
         value = m_values[i];
   }

   return value;
}

// Get the non-zero frequency of occurence within a window
float MovingWindow::GetFreqValue(void)
{
	int count = 0;
	for (int i = 0; i < m_windowLength; i++)
	{
		if (m_values[i] != 0) // || m_values[i] != nullDataValue))
			count++;
	}

	return float(count) / m_windowLength;
}

// Moving Average of within a smaller window length than max window length
// return false if interval attempts to be greater than the maximum window length 
bool MovingWindow::GetAvgValue( int interval, float &result )
{
   if (interval > m_windowLength)
      return false;

   float value = 0.0f;
   for (int i = m_windowLength; i > (m_windowLength - interval); i--)
      value += m_values[i];

   result = value / interval;

   return true;

}

// Reset Moving Window (zero out)
bool MovingWindow::Clear()
{
	for (int i = 0; i < m_windowLength; i++)
	{
		m_values[ i ] = 0.0f;
	}
	return true;
}
 