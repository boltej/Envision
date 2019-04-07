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
// BarPlot3DWnd.cpp : implementation file
//

#include "EnvWinLibs.h"

#include "plot3d/BarPlot3DWnd.h"


// BarPlot3DWnd

BarPlot3DWnd::BarPlot3DWnd()
   :
   m_pDataObj( NULL ),
   m_whiteBackground( true ),
   m_delOnDel( false ),
   m_showLegend( false ),
   m_showLabels( true ),
   m_showOutline( true )
   {

   m_useLighting = false;

   SetProjectionType( P3D_PT_PERSPECTIVE );

   m_eye = normalize( glReal3( -1.0, -1.0, 0.7 ) );
   m_up  = normalize( cross( cross( m_eye, K ), m_eye ) );
   m_eyeDist = 4.5;

   m_colorScaleWnd.SetBaseColor( RGB( 0,255,0 ) );

   //GetAxis( P3D_XAXIS )->StandardFixed( true );
   //GetAxis( P3D_YAXIS )->StandardFixed( true );

   //GetAxis( P3D_XAXIS )->SetPlane( K );
   //GetAxis( P3D_YAXIS )->SetPlane( K );

   //GetAxis( P3D_XAXIS2 )->SetPlane( K );
   //GetAxis( P3D_YAXIS2 )->SetPlane( K );

   GetAxis( P3D_XAXIS )->ShowTicks( true );
   GetAxis( P3D_YAXIS )->ShowTicks( true );

   GetAxis( P3D_ZAXIS )->ShowTicks( true );
   GetAxis( P3D_ZAXIS )->ShowTickLabels( true );
   }

BarPlot3DWnd::~BarPlot3DWnd()
   {
   if ( m_pDataObj != NULL && m_delOnDel )
      delete m_pDataObj;
   }
   
void BarPlot3DWnd::SetDataObj( FDataObj *pDataObj, bool delOnDel, bool onlyIncreaseScale /*= false*/ )
   { 
   m_pDataObj = pDataObj; 
   m_delOnDel = delOnDel; 

   glReal xMax = (glReal)m_pDataObj->GetRowCount();
   glReal yMax = (glReal)m_pDataObj->GetColCount();

   float m = 0.0f;
   float test = 0.0f;
   for ( int i=0; i<m_pDataObj->GetRowCount(); i++ )
      for ( int j=0; j<m_pDataObj->GetColCount(); j++ )
         {
         m_pDataObj->Get(j,i,test);
         if (test>m)
            m = test;
         }

   if ( m <= 0.0 )
      m = 1.0;

   //SetAxisBounds( 0.0, xMax, 0.0, yMax, 0.0, m );
   //SetAxisBounds2( 0.0, xMax, 0.0, yMax, 0.0, m );

   GetAxis( P3D_XAXIS )->SetExtents( 0.0, xMax );
   GetAxis( P3D_YAXIS )->SetExtents( 0.0, yMax );

   if ( !onlyIncreaseScale || ( onlyIncreaseScale && m > GetAxis( P3D_ZAXIS )->GetMax() ) )
      {
      GetAxis( P3D_ZAXIS )->AutoScale( 0.0, m );
      }

   m_colorScaleWnd.SetMinMax( 0.0f, (float)GetAxis( P3D_ZAXIS )->GetMax() );

   if ( GetSafeHwnd() )
      {
      Invalidate();
      m_colorScaleWnd.Invalidate();
      }

   }

BEGIN_MESSAGE_MAP(BarPlot3DWnd, Plot3DWnd)
   ON_WM_RBUTTONDOWN()
   ON_WM_LBUTTONDOWN()
   ON_COMMAND_RANGE(9000, 10000, OnSelectOption)
END_MESSAGE_MAP()

void BarPlot3DWnd::OnCreateGL()
   {
   Plot3DWnd::OnCreateGL();

   m_colorScaleWnd.Create( NULL, NULL, WS_BORDER, CRect( 0,0,0,0 ), this, 2314 );
   if ( m_showLegend )
      m_colorScaleWnd.ShowWindow( SW_SHOW );
   
   // make font used for axis labels
   MakeFont3D();

   }

void BarPlot3DWnd::DrawData( void )
   {
   SetColorObject( RGB( 255,0,0) );

   if ( m_pDataObj != NULL )
      {
      int rows = m_pDataObj->GetRowCount();
      ASSERT( rows == m_pDataObj->GetColCount() );

      glReal3 org( 0,0,0 );
      glReal3 length( 1.0/(double)rows, 0.0, 0.0 );
      glReal3 width( 0.0, 1.0/(double)rows, 0.0 );

      glReal maxZ = GetAxis( P3D_ZAXIS )->GetMax();

      if ( m_showOutline )
         {
         glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
         //glColor3f( 0.0, 0.0, 0.0 );
         SetColorEmission( RGB( 0.0, 0.0, 0.0 ) );

         for ( int i=0; i<rows; i++ )  // rows increase along the x-axis
            for ( int j=0; j<rows; j++ ) // columns increase along the y-axis
               {
               glReal3 anchor = org + double(i)*length + double(j)*width;
               glPushMatrix();
                  glTranslated( anchor.x, anchor.y, anchor.z );
                  float h;
                  m_pDataObj->Get( j, i, h );
                  //SetColorObject( m_colorScaleWnd.GetColor( h ) );
                  glReal3 height( 0.0, 0.0, (double)h/maxZ );
                  DrawPrism( length, width, height );
               glPopMatrix();
               }

         glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
         glEnable( GL_POLYGON_OFFSET_FILL );
         glPolygonOffset( 1.0, 1.0 );
         }

      //SetColorObject( RGB( 255, 255, 255 ) );

      for ( int i=0; i<rows; i++ )  // rows increase along the x-axis
         for ( int j=0; j<rows; j++ ) // columns increase along the y-axis
            {
            glReal3 anchor = org + double(i)*length + double(j)*width;
            glPushMatrix();
               glTranslated( anchor.x, anchor.y, anchor.z );
               float h;
               m_pDataObj->Get( j, i, h );
               SetColorObject( m_colorScaleWnd.GetColor( h ) );
               glReal3 height( 0.0, 0.0, (double)h/maxZ );
               DrawPrism( length, width, height );
            glPopMatrix();
            }

      if ( m_showOutline )
         glDisable( GL_POLYGON_OFFSET_FILL );

      }
   
   }

void BarPlot3DWnd::DrawAxes()
   {
   static glReal tickSize  = 0.03;
   //static glReal tickLabelSize  = 0.07;
   static glReal labelSize = 0.07; //0.1;

   // Call base class
   Plot3DWnd::DrawAxes();

   if ( m_showLabels && m_pDataObj != NULL )
      {
      int rows = m_pDataObj->GetRowCount();
      ASSERT( rows == m_pDataObj->GetColCount() );

//      glReal3 org = GetAxis( YAXIS ).GetLocation();;
      glReal3 length( 1.0/(double)rows, 0.0, 0.0 );
      glReal3 width( 0.0, 1.0/(double)rows, 0.0 );

      Axis *pX = GetAxis( P3D_XAXIS );
      Axis *pY = GetAxis( P3D_YAXIS );

      glReal signX = -1.0;
      glReal signY = -1.0;
      int styleX   = 0; 
      int styleY   = 0;

      glReal size = min( labelSize, 1.0/(double)rows );
      
      for ( int i=0; i<rows; i++ )
         {
         LPCTSTR str = m_pDataObj->GetLabel( i );
         glReal3 ext = GetText3DExtent( str );
         styleX = P3D_TXT_VCENTER;
         styleY = P3D_TXT_VCENTER;

         if ( dot( pX->GetDirection(), m_up ) > 0 )
            signX = -1.0;
         else
            signX = 1.0;

         if ( dot( pY->GetDirection(), m_up ) > 0 )
            signY = -1.0;
         else
            signY = 1.0;

         glReal3 v = cross( signX*pX->GetDirection(), pX->GetTickDir() );
         v = normalize( v );
         if ( dot( v, pX->GetNormal() ) > 0 )
            styleX |= P3D_TXT_LEFT;
         else
            styleX |= P3D_TXT_RIGHT;

         v = cross( signY*pY->GetDirection(), pY->GetTickDir() );
         v = normalize( v );
         if ( dot( v, pY->GetNormal() ) > 0 )
            styleY |= P3D_TXT_LEFT;
         else
            styleY |= P3D_TXT_RIGHT;

         //SetColorEmission( RGB(200,200,200) );
         //DrawLine( glReal3( double(i), 0.0, 0.0 ), glReal3( double(i), double(rows), 0.0 ) );
         //DrawLine( glReal3( 0.0, double(i), 0.0 ), glReal3( double(rows), double(i), 0.0 ) );

         SetColorEmission( pX->GetLabelColor() );
         DrawText3D( str, pX->GetLocation() + 0.03*pX->GetTickDir() + (double(i)+0.5)*1.0/(double)rows*pX->GetDirection(), signX*size*pX->GetDirection(), pX->GetNormal(), styleX ); //P3D_TXT_BOTTOMLEFT );         
         SetColorEmission( pY->GetLabelColor() );
         DrawText3D( str, pY->GetLocation() + 0.03*pY->GetTickDir() + (double(i)+0.5)*1.0/(double)rows*pY->GetDirection(), signY*size*pY->GetDirection(), pY->GetNormal(), styleY ); //P3D_TXT_BOTTOM | P3D_TXT_RIGHT /*P3D_TXT_BOTTOMLEFT*/ 
         }

      }

   }

void BarPlot3DWnd::OnSizeGL(int cx, int cy)
{
   Plot3DWnd::OnSizeGL( cx, cy );

   m_colorScaleWnd.MoveWindow( cx - 60, 0, 60, cy );
}

void BarPlot3DWnd::OnLButtonDown(UINT nFlags, CPoint point)
   {
   SetFocus();

   Plot3DWnd::OnLButtonDown(nFlags, point);
   }

void BarPlot3DWnd::BuildPopupMenu( CMenu* pMenu )
   {
   Plot3DWnd::BuildPopupMenu( pMenu );

   UINT flag;

   // --- Show Legend ---
   flag = MF_STRING;
   if ( m_showLegend == true )
      flag |= MF_CHECKED;
   pMenu->AppendMenu( flag, SHOW_LEGEND, "Show Legend" );

   // --- Show Outline ---
   //flag = MF_STRING;
   //if ( m_showOutline == true )
   //   flag |= MF_CHECKED;
   //pMenu->AppendMenu( flag, SHOW_OUTLINE, "Show Outline" );
   }

void BarPlot3DWnd::OnSelectOption( UINT nID )
   {
   switch (nID)
      {
      case SHOW_LEGEND:
         {
         if ( m_showLegend )
            m_colorScaleWnd.ShowWindow( SW_HIDE );
         else
            m_colorScaleWnd.ShowWindow( SW_SHOW );
         m_showLegend = !m_showLegend;
         }
         break;
      case SHOW_OUTLINE:
         {
         m_showOutline = !m_showOutline;
         Invalidate();
         }
         break;

      case DATA_SET_PROPS:
         break;

      default:
         Plot3DWnd::OnSelectOption( nID );
      }
   }