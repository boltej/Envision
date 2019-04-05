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

class PointSet;
class ShaderMgr;
class MapLayer;
class DeltaArray;


//#include "camera.h"
//using namespace glm;

//#include "PointSet.h"
//#include "ShaderMgr.h"

enum AVAXIS { AV_XAXIS, AV_YAXIS, AV_ZAXIS };


// AnalyticsViewer

class AnalyticsViewer : public CWnd
{
   DECLARE_DYNAMIC(AnalyticsViewer)

public:
   AnalyticsViewer();
   virtual ~AnalyticsViewer();

   int Init( MapLayer *pLayer, DeltaArray *pDA );

protected:
   // camera support
   //Camera m_camera;
   //glm::vec3 m_cameraPos;
   //glm::vec3 m_lookAtPos;

   //void RotateCamera( AVAXIS axis, float radians, bool redraw );
   //void ZoomCamera( float fraction, bool redraw );


   // OpenGL rendering context
  // HGLRC m_hRC;
  // bool m_inDrag;
  // int m_startXPos;
  // int m_startYPos;
  //
  // float m_currXRot;
  // float m_currYRot;
  //
  // int m_mwDelta;    // cumulative mouse wheel deltas, used only at runtime
  //
  // PointSet  *m_pPointSet;        // =PointSet(4); - points to draw
  // ShaderMgr *m_pShdrMgr;         // =ShaderMgr();
  //
  // MapLayer *m_pMapLayer;
  // DeltaArray *m_pDeltaArray;
  //
  // void InitOpenGL();      // attach GL window to this one and initialize the Open GL stuff
  //
  // void PopulateFullGridPointSet(MapLayer *pLayer, DeltaArray *pDA);
  // void PopulateDeltaGridPointSet(MapLayer *pLayer, DeltaArray *pDA);
  // void PopulateTestPointSet();  // temporary
  // void PopulateSinglePointSet();
  //
  // void DrawPointSet();
   
//   void Rotate(float rotYAxis, float rotXAxis );

protected:
   DECLARE_MESSAGE_MAP()

public:
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg void OnPaint();
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
   afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
   afx_msg void OnMouseMove(UINT nFlags, CPoint point);
   afx_msg BOOL OnMouseWheel(UINT fFlags,short zDelta, CPoint point);
   afx_msg void OnDestroy();
   afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
   };


