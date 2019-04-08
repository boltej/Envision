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

#include <EnvExtension.h>
#include <afxtempl.h>

#include "TransMatrix.h"

#define _EXPORT __declspec( dllexport )

// CALPSApp
// See ALPS.cpp for the implementation of this class
//

class _EXPORT SimpleSTM : public EnvModelProcess
{
public:
   SimpleSTM() 
      : EnvModelProcess()
      , m_ccPeriodScalar( 1.0f )
      , m_ccPCScalar( 1.0f )
      , m_transMatrix()
      , m_initialMatrix()
      { }

   float m_ccPeriodScalar;;    // dormancy period scalar
   float m_ccPCScalar;        // transition probability scalar

   TransMatrix m_transMatrix;
   TransMatrix m_initialMatrix;

   CList< TransMatrix, TransMatrix& > m_stack;

public:
   bool Init( EnvContext *pEnvContext, LPCTSTR initStr );
   bool InitRun( EnvContext *pEnvContext, bool );
   bool Run( EnvContext *pEnvContext );
   //BOOL APSetup( EnvContext*, HWND );
};

