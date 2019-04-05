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

// USE CASE 1:  pAccum != NULL.
//              Timing begins when constructor is called.
//              Timing ends when destructor is called.
//              Elapsed time is added to *pAccum

// USE CASE 2:  pAccum == NULL.
//              Timing begins when Start() is called.
//              Timing ends when Stop() is called.
//              Elapsed time is stored in m_secsElapsed and returned by Stop()
//                Additional calls Start/Stop calls resets m_secsElapsed

class LIBSAPI HighResTime
   {
   public:
      HighResTime( double *pAccum = NULL );
      ~HighResTime();

      LARGE_INTEGER m_frequency;
      LARGE_INTEGER m_count;
      LARGE_INTEGER m_elapsed;
      double m_secsElapsed;  // seconds between most recent Start()/Stop() call

      // Starts the count.
      void Start(); 

      // Stops the count and returns elapsed secs.
      double Stop(); 

      double *m_pAccum;
   };
