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

#include "EnvWinLibs.h"

#include "Plot3DWnd.h"
#include <afxtempl.h>
#include <math.h>
#include <gl/glReal.h>

#include "plot3d/ScatterLegendWnd.h"

class WINLIBSAPI PointCollection : public CArray< glReal3*, glReal3* >
{
public:
	PointCollection(void); 
   PointCollection( PointCollection &collection ){ *this = collection; }  // copy constructor
   PointCollection( CString name, bool isPoints, bool isTrajectory  );
	~PointCollection(void);

   PointCollection & operator = ( PointCollection& );

	int AddPoint( glReal3 *pPoint ){ return (int) Add( pPoint ); } // pPoint must be newed
	int AddPoint( glReal x, glReal y, glReal z ){ return (int) Add( new glReal3(x,y,z) ); }

	void RemoveAllPoints( void );

   // Sets
   void SetName( CString name ){ m_name = name; }

   void SetPoints( bool isPoints ){ m_points = isPoints; }
   void SetPointColor( COLORREF color ){ m_pointColor = color; }
   void SetPointType( P3D_SHAPE_TYPE pointType ){ m_pointType = pointType; }
   void SetPointSize( int pointSize ){ m_pointSize = pointSize; }
   void SetPointProperties( COLORREF pointColor, int pointSize, P3D_SHAPE_TYPE pointType = P3D_ST_SPHERE ){ SetPointColor( pointColor ); SetPointSize( pointSize ); SetPointType( pointType ); }

   void SetTraj( bool isTrajectory ){ m_traj = isTrajectory; }
   void SetTrajColor( COLORREF color ){ m_trajColor = color; }
   void SetTrajType( P3D_LINE_TYPE trajType ){ m_trajType = trajType; }
   void SetTrajSize( int trajSize ){ m_trajSize = trajSize; }
   void SetTrajProperties( COLORREF trajColor, int trajSize, P3D_LINE_TYPE lineType = P3D_LT_PIXEL ){}

   // Gets
   CString GetName( void ){ return m_name; }

   bool           IsPoints( void ){ return m_points; }
   COLORREF       GetPointColor( void ){ return m_pointColor; }
   P3D_SHAPE_TYPE GetPointType( void ){ return m_pointType; }
   int            GetPointSize( void ){ return m_pointSize; }

   bool           IsTrajectory( void ){ return m_traj; }
   COLORREF       GetTrajColor( void ){ return m_trajColor; }
   P3D_LINE_TYPE  GetTrajType( void ){ return m_trajType; }
   int            GetTrajSize( void ){ return m_trajSize; }
   
protected:
	CString         m_name;

   // point properties
   bool            m_points;
	COLORREF        m_pointColor;
   int             m_pointSize;
   P3D_SHAPE_TYPE  m_pointType;

   // trajectory properties
   bool            m_traj;
   COLORREF        m_trajColor;
   int             m_trajSize;
   P3D_LINE_TYPE   m_trajType;
};

class PointCollectionArray : public CArray< PointCollection*, PointCollection* >
{
public:
   PointCollectionArray(void){}
   ~PointCollectionArray(void){ for ( int i=0; i< GetCount(); i++ ) delete GetAt(i); }
};


//////////////////////////////////////////////////////////////////////////
// ScatterPlot3DWnd
//////////////////////////////////////////////////////////////////////////

class WINLIBSAPI ScatterPlot3DWnd : public Plot3DWnd
{
public:
	ScatterPlot3DWnd();
	virtual ~ScatterPlot3DWnd();

protected:
   ScatterLegendWnd m_legend;
   bool m_showLegend;

   // Data
	PointCollectionArray m_dataArray;
   
   enum S3D_MENUS
      {
      DATA_SET_PROPS = 9000,
      SHOW_LEGEND,
      };

   virtual void BuildPopupMenu( CMenu* pMenu );
   void OnSelectOption( UINT nID );
   void SizeWindows( int cx, int cy );

//Methods
public:
   // Data
   void RemoveData( void ); // Clears all points and point collections
   void ClearData( void ); // Clears all points, saves point collections
   int  AddDataPoint( glReal x, glReal y, glReal z, int collection = 0 );
   PointCollection* AddDataPointCollection( void ){ PointCollection *pCol = new PointCollection(); m_dataArray.Add( pCol ); return pCol; }
   PointCollection* AddDataPointCollection( CString name, bool isPoints, bool isTrajectory  ){ PointCollection *pCol = new PointCollection( name, isPoints, isTrajectory ); m_dataArray.Add( pCol ); return pCol;}
   
   // Gets
   PointCollectionArray *GetPointCollectionArray( void ){ return &m_dataArray; }
   PointCollection      *GetPointCollection( int col ){ ASSERT( col < m_dataArray.GetCount() ); return m_dataArray.GetAt( col ); }
   COLORREF             GetBackgroundColor( void ){ return m_backgroundColor; }

   // Sets
//   void SetPointSize( int size ){ m_pointSize = size; }
      
protected:
   void DrawData();

protected:
	DECLARE_MESSAGE_MAP()
public:
   virtual void OnCreateGL(); 
   virtual void OnSizeGL(int cx, int cy);

   afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
protected:
   virtual BOOL PreCreateWindow(CREATESTRUCT& cs);   
};
