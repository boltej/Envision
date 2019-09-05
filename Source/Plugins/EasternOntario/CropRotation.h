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

#include <Vdataobj.h>
#include <FDATAOBJ.H>
#include <PtrArray.h>
#include "EasternOntario.h"


class Rotation
{
public:
   CString m_name;
   int     m_rotationID;
   float   m_initPctArea;     // decimal percent (0-1)

   // output variables
   float   m_totalArea;       // ha
   float   m_totalAreaPct;    // percent (0-100)
   float   m_totalAreaPctAg;  // percent (0-100)

   CArray< int, int > m_sequenceArray;
   CMap< int, int, int, int > m_sequenceMap;   // key=Attr code, value=index in sequence array

   Rotation( void ) : m_rotationID( -1 ), m_initPctArea( 0 ),
      m_totalArea( 0 ), m_totalAreaPct( 0 ), m_totalAreaPctAg( 0 ) { }
   
   bool LookupLulc( int lulc, int &index );

};


class CropRotationProcess
{
public:
   CropRotationProcess( void );

   bool Init   ( EnvContext *pEnvContext, LPCTSTR initStr );
   bool InitRun( EnvContext *pEnvContext );
   bool Run    ( EnvContext *pContext, bool useAddDelta=true );

   int m_id;

   PtrArray< Rotation > m_rotationArray;

protected:
   CString m_lulcField;
   CString m_rotationField;

   CMap< int, int, Rotation*, Rotation* > m_rotationMap;   // key=rotation code, value=correspnding rotation

   CArray< int, int >     m_rotLulcArray;     // all lulc values used in a rotation
   CArray< float, float > m_rotLulcAreaArray; // all lulc areas used in a rotation

public:
   int m_colLulc;    // idu coverage
   int m_colLulcA;   
   int m_colRotation;
   int m_colRotIndex;   // 0-based index of current location in sequence, no data if not in sequence
   int m_colArea;    // "AREA"

protected:
   float m_totalIDUArea;
   float m_totalIDUAreaAg;

   bool LoadXml( EnvContext*, LPCTSTR filename );
   void AllocateCropRotations( MapLayer *pLayer );
   void UpdateRotationDeltas( EnvContext*, bool useAddDelta );

   bool IsCashCrop( int lulc );


};


//inline
//bool Rotation::FindLulcIndex( int lulc, int &index )
//   {
//   for ( int i=index; i < (int) m_sequenceArray.GetSize(); i++ )
//      if ( 
//
//   }

