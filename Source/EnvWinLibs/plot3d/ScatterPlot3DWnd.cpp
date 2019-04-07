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
// ScatterPlot3DWnd.cpp : implementation file
//

#include "EnvWinLibs.h"
#include "plot3d/ScatterPlot3DWnd.h"
#include "plot3d/ScatterPropertiesDlg.h"

static const glReal pi = 3.14159265359;

////////////////////////////////////////////////////////////////////////
// PointCollection
////////////////////////////////////////////////////////////////////////
PointCollection::PointCollection(void)
:  m_name( "Untitled" ), 
   m_pointColor( RGB(0,0,0) ), 
   m_pointSize( 1 ), 
   m_pointType( P3D_ST_SPHERE ), 
   m_points( TRUE ),
   m_traj( FALSE ),
   m_trajColor( RGB(0,0,0) ),
   m_trajSize( 1 ),
   m_trajType( P3D_LT_PIXEL )
   {
   }

PointCollection::PointCollection( CString name, bool isPoints, bool isTrajectory )
:  m_name( name ), 
   m_pointColor( RGB(0,0,0) ), 
   m_pointSize( 1 ), 
   m_pointType( P3D_ST_SPHERE ), 
   m_points( isPoints ),
   m_traj( isTrajectory ),
   m_trajColor( RGB(0,0,0) ),
   m_trajSize( 1 ),
   m_trajType( P3D_LT_PIXEL )
   {
   }

PointCollection& PointCollection::operator = ( PointCollection &collection )
   {
   m_name       = collection.m_name;

   m_points     = collection.m_points;
   m_pointColor = collection.m_pointColor;
   m_pointSize  = collection.m_pointSize;
   m_pointType  = collection.m_pointType;
   m_traj       = collection.m_traj;

   m_trajColor  = collection.m_trajColor;
   m_trajSize   = collection.m_trajSize;
   m_trajType   = collection.m_trajType;

   return *this;
   }

PointCollection::~PointCollection(void)
   {
   RemoveAllPoints();
   }

void PointCollection::RemoveAllPoints()
   {
   for ( int i=0; i< GetCount(); i++ )
      delete GetAt(i);

   RemoveAll();
   }


////////////////////////////////////////////////////////////////////////
// ScatterPlot3DWnd
////////////////////////////////////////////////////////////////////////
ScatterPlot3DWnd::ScatterPlot3DWnd()
:  m_showLegend( false )
   {
   SetProjectionType( P3D_PT_PERSPECTIVE );
   m_dataArray.Add( new PointCollection() ); // There must always be a collection

   SetAxisBounds( 0, 10, 0, 10, 0, 100 );
   SetAxisTicks(2,2,20);

   SetAxisBounds2( 0, 10, 0, 10, 0, 100 );
   SetAxisTicks2(2,2,20);

   SetAxesLabels( "x axis", "y axis", "z axis" );
   //GetAxis( P3D_XAXIS )->StandardFixed();
   //GetAxis( P3D_YAXIS )->StandardFixed();
   //GetAxis( P3D_ZAXIS )->StandardFixed();

   SetAxesVisible( true );
   SetAxesVisible2( false );
   }

ScatterPlot3DWnd::~ScatterPlot3DWnd()
   {
   RemoveData();
   }

void ScatterPlot3DWnd::RemoveData( void )
   {
   // delete all PointCollections except for one
   // delete all points in remaining collection

   int count = (int) m_dataArray.GetCount();
   ASSERT( count > 0 );

   m_dataArray.GetAt(0)->RemoveAllPoints();

   while ( count > 1 )
      {
      delete m_dataArray.GetAt( 1 );
      m_dataArray.RemoveAt( 1 );
      count = (int) m_dataArray.GetCount();
      }
   }

void ScatterPlot3DWnd::ClearData( void )
   {
   int count = (int) m_dataArray.GetCount();
   ASSERT( count > 0 );

   for ( int i=0; i<count; i++ )
      m_dataArray.GetAt(i)->RemoveAllPoints();

   }

int ScatterPlot3DWnd::AddDataPoint( glReal x, glReal y, glReal z, int collection )
   { 
   ASSERT( collection >= 0 );
   ASSERT( collection < m_dataArray.GetCount() );

   return m_dataArray.GetAt( collection )->AddPoint(x,y,z); 
   }

BEGIN_MESSAGE_MAP(ScatterPlot3DWnd, Plot3DWnd)
   ON_WM_RBUTTONDOWN()
   ON_WM_LBUTTONDOWN()
   ON_COMMAND_RANGE(9000, 10000, OnSelectOption)
END_MESSAGE_MAP()

BOOL ScatterPlot3DWnd::PreCreateWindow(CREATESTRUCT& cs)
   {
   cs.style |= WS_CLIPCHILDREN;

   return Plot3DWnd::PreCreateWindow(cs);
   }

void ScatterPlot3DWnd::OnCreateGL()
   {
   Plot3DWnd::OnCreateGL();

   m_legend.SetPointCollectionArray( &m_dataArray );
   m_legend.Create( NULL, "Legend", WS_BORDER, CRect(0,0,0,0), this, 666 );

   if ( m_showLegend )
      m_legend.ShowWindow( SW_SHOW );

   //glCullFace( GL_BACK );
   //glEnable( GL_CULL_FACE );

   // make font used for axis labels
   //MakeFont();
   MakeFont3D();
   }

void ScatterPlot3DWnd::DrawData()
   {
   for ( int i=0; i<m_dataArray.GetCount(); i++ )
      {
      PointCollection *pCol = m_dataArray.GetAt(i);

      //glReal3 *pPoint;

      glReal3 point;
      glReal3 prevPoint;

      int count = (int) pCol->GetCount();

      int start = 0;
      if ( m_startIndex > 0 )
         start = m_startIndex;

      int end = count-1;
      if ( m_endIndex >= 0 )
         {
         if ( m_endIndex > count-1 )
            end = count-1;
         else
            end = m_endIndex;
         }

      if ( pCol->IsTrajectory() )
         {
         COLORREF color = pCol->GetTrajColor();
         SetColorEmission( color );
         EnableClippingPlanes( true );
         glLineWidth( GLfloat( pCol->GetTrajSize()) );
         //GLfloat red   = GetRValue( color ) / 255.0f;
         //GLfloat green = GetGValue( color ) / 255.0f;
         //GLfloat blue  = GetBValue( color ) / 255.0f;
         //glColor3f( red, green, blue );

         for ( int j=start; j <= end; j++ )
            {
            point = *pCol->GetAt(j);

            if ( j>0 )
               DrawLine( prevPoint, point );

            prevPoint = point;
            }
         EnableClippingPlanes( false );
         }

      glLineWidth( 1 );
      for ( int j=start; j <= end; j++ )
         {
         point = *pCol->GetAt(j);
         SetColorObject( pCol->GetPointColor() );
         if ( IsPointInPlot( point ) )
            DrawShape( pCol->GetPointType(), point, pCol->GetPointSize() );

         }
      }
   }

void ScatterPlot3DWnd::OnSizeGL(int cx, int cy)
   {
   static int gutter = 10;

   CSize legendSize(0,0);
   if ( m_showLegend && IsWindow( m_legend.m_hWnd ) )
      {
      legendSize = m_legend.GetIdealSize();

      if ( legendSize.cx > 200 )
         legendSize.cx = 200;
      if ( legendSize.cy > cy )
         legendSize.cy = cy;

      int legendNudge = ( cy - legendSize.cy )/2;

      cx = cx - legendSize.cx - 2*gutter;

      //MoveWindow( 0, 0, cx - legendSize.cx - 2*gutter, cy );
      m_legend.MoveWindow( cx + gutter, legendNudge, legendSize.cx, legendSize.cy );
      }
   //else
   //   MoveWindow( 0, 0, cx, cy );
   /////////////////////////////////////////////

   if ( cy > 0 )
      m_aspect = (double)cx/(double)cy;
   else
      m_aspect = 1;

   SetProjection( m_projType );

   // set correspondence between window and OGL viewport
   glViewport( 0, 0, cx, cy );
   Invalidate();
   }

void ScatterPlot3DWnd::SizeWindows(int cx, int cy)
   {
   BeginGLCommands();
   EndGLCommands();
   }

void ScatterPlot3DWnd::OnLButtonDown(UINT nFlags, CPoint point)
   {
   SetFocus();

   Plot3DWnd::OnLButtonDown(nFlags, point);
   }

void ScatterPlot3DWnd::BuildPopupMenu( CMenu* pMenu )
   {
   Plot3DWnd::BuildPopupMenu( pMenu );

   UINT flag;

   // --- Show Legend ---
   flag = MF_STRING;
   if ( m_showLegend == true )
      flag |= MF_CHECKED;
   pMenu->AppendMenu( flag, SHOW_LEGEND, "Show Legend" );

   // --- Data Set Props ---
   flag = MF_STRING;
   pMenu->AppendMenu( flag, DATA_SET_PROPS, "Data Set Properties" );
   }

void ScatterPlot3DWnd::OnSelectOption( UINT nID )
   {
   switch (nID)
      {
      case DATA_SET_PROPS:
         {
         ScatterPropertiesDlg dlg( this );
         dlg.DoModal();
         m_legend.Invalidate();//Update();
         Invalidate();
         }
      break;

      case SHOW_LEGEND:
         {
         if ( m_showLegend )
            m_legend.ShowWindow( SW_HIDE );
         else
            m_legend.ShowWindow( SW_SHOW );
         m_showLegend = !m_showLegend;
         CRect rect;
         GetWindowRect( &rect );

         //BeginGLCommands();
         //OnSizeGL( rect.Width(), rect.Height() );
         //EndGLCommands(); 

         // The above method doesnt work.  This is an ugly brute force method.
         SendMessage( WM_SIZE, SIZE_RESTORED, MAKELPARAM( rect.Width(), rect.Height() ) );
         }
      break;

      default:
         Plot3DWnd::OnSelectOption( nID );
      }

   }
