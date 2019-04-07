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
// MapListWnd.cpp : implementation file
//

#include "EnvWinLibs.h"

#include "MapListWnd.h"
#include "MapWnd.h"
#include "maplayerdlg.h"

#include <maplayer.h>
#include <colors.hpp>
#include <vdatatable.h>
#include <GEOMETRY.HPP>

#include <direct.h>

extern MapLayer *gpCellLayer;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// MapListWnd

IMPLEMENT_DYNCREATE(MapListWnd, CWnd)

MapListWnd::MapListWnd()
: m_pMapWnd( NULL )
{ }

MapListWnd::~MapListWnd()
{ }

BEGIN_MESSAGE_MAP(MapListWnd, CWnd)
	//{{AFX_MSG_MAP(MapListWnd)
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// MapListWnd message handlers

void MapListWnd::OnSize(UINT nType, int cx, int cy) 
   {
	CWnd::OnSize(nType, cx, cy);

   if ( IsWindow( m_list.GetSafeHwnd() ) )
      m_list.MoveWindow( 0, 0, cx, cy, TRUE );
   }


int MapListWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) 
   {
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   RECT rect;
   GetClientRect( &rect );
	
   m_list.Create( WS_CHILD /*| WS_BORDER*/ | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LBS_OWNERDRAWVARIABLE,  rect, this, 100 );

   m_list.m_font.CreateStockObject( DEFAULT_GUI_FONT );
   //m_font.CreatePointFont( 200, "Arial" );

   LOGFONT logFont;
   m_list.m_font.GetLogFont( &logFont );

   logFont.lfWeight = FW_BOLD;
   m_list.m_layerFont.CreateFontIndirect( &logFont );

   m_list.SetFont( &m_list.m_font );

   
   // set up tool tip
   m_list.m_ttPosition.x = 200;
   m_list.m_ttPosition.y = 200;
   m_list.m_toolTip.Create( this, TTS_ALWAYSTIP|TTF_TRACK|TTF_ABSOLUTE|TTF_IDISHWND );

   m_list.m_toolTip.SetDelayTime(TTDT_AUTOPOP, 3);  // -1,0,0
   m_list.m_toolTip.SetDelayTime(TTDT_INITIAL, 3); 
   m_list.m_toolTip.SetDelayTime(TTDT_RESHOW, 3);

   TOOLINFO ti;
   ti.cbSize = TTTOOLINFOA_V2_SIZE;  //sizeof( TOOLINFO );
   ti.uFlags = TTF_ABSOLUTE | TTF_IDISHWND | TTF_TRACK;
   ti.hwnd   = this->m_hWnd;
   ti.uId    = (UINT_PTR) this->m_hWnd;
   ti.lpszText = LPSTR_TEXTCALLBACK;
   ti.lParam = 10001;  // ap defined value
   ti.lpReserved = NULL;
   GetClientRect( &ti.rect);
   m_list.m_toolTip.SendMessage(TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti); 
   //m_list.m_toolTip.Activate(TRUE);

   m_list.m_toolTip.SendMessage( TTM_SETMAXTIPWIDTH, 0, (LPARAM) SHRT_MAX );
   m_list.m_toolTip.SendMessage( TTM_SETDELAYTIME, TTDT_AUTOPOP, SHRT_MAX ); // turn of autopop
   m_list.m_toolTip.SendMessage( TTM_SETDELAYTIME, TTDT_INITIAL, 1200 );  // 200 200
   m_list.m_toolTip.SendMessage( TTM_SETDELAYTIME, TTDT_RESHOW, 1200 );

   m_list.m_toolTip.EnableTrackingToolTips(TRUE);
   m_list.m_toolTip.EnableToolTips(TRUE);
   
	return 0;
   }


void MapListWnd::Refresh( void )
   {
   // reset everything
   for ( int i=0; i < m_itemDataArray.GetSize(); i++ )
      delete m_itemDataArray[ i ];

   m_itemDataArray.RemoveAll();
   m_list.ResetContent();

   if ( m_pMapWnd == NULL )
      return;

   int layerCount = m_pMapWnd->m_pMap->GetLayerCount();
   //int itemCount = 0;

   for ( int i=0; i < layerCount; i++ )
      {
      MapLayer *pLayer = m_pMapWnd->m_pMap->GetLayerFromDrawOrder( i );

      // see if layer is in show layer list
      bool found = false;
      for ( int j=0; j < m_list.m_showLayerFlagArray.GetSize(); j++ )
         if ( m_list.m_showLayerFlagArray[ j ].pLayer == pLayer )
            {
            found = true;
            break;
            }

      if ( ! found )
         m_list.m_showLayerFlagArray.Add( SHOW_LAYER( pLayer, pLayer->m_expandLegend ? true : false ) );

      // layer ptr
      MAPLIST_ITEM_DATA *pData = new MAPLIST_ITEM_DATA( pLayer );
      pData->type = MID_LAYER;
      pData->label = pLayer->m_name;
      m_itemDataArray.Add( pData );

      // add to list box
      int index = m_list.AddString( pData->label );
      m_list.SetItemData( index, (DWORD_PTR) pData );
      m_list.SetItemHeight( index, m_list.m_layerHeight );

      // field label
      int activeField = pLayer->GetActiveField();
      if ( activeField >= 0 )
         {
         pData = new MAPLIST_ITEM_DATA( pLayer );
         pData->type = MID_FIELDLABEL;

         MAP_FIELD_INFO *pInfo = NULL;
         if ( ( pInfo = pLayer->FindFieldInfo( pLayer->GetFieldLabel() ) ) != NULL )
            {
            pData->label = pInfo->label;
            pData->label += " (";
            pData->label += pLayer->GetFieldLabel();
            pData->label += ")";
            }
         else
            pData->label = pLayer->GetFieldLabel();
         
         m_itemDataArray.Add( pData );

         // add to list
         index = m_list.AddString( pData->label );
         m_list.SetItemData( index /*itemCount++*/, (DWORD_PTR) pData );
         m_list.SetItemHeight( index, m_list.m_fieldLabelHeight );
 
         // is expanded, add bins to list
         if ( m_list.IsExpanded( pLayer ) )
            {
            // next, add the bins
            int binCount = pLayer->GetBinCount( activeField );

            for ( int j=0; j < binCount; j++ )
               {
               Bin *pBin = &pLayer->GetBin( activeField, j );
               pData = new MAPLIST_ITEM_DATA( pLayer, pBin );  // sets type = MID_BIN;
               CString label;
               if ( pBin->m_pFieldAttr != NULL )
                  {
                  FIELD_ATTR *pAttr = pBin->m_pFieldAttr;
                  
                  if ( ::IsInteger( pAttr->value.type ) )
                     {
                     int value = 0;
                     if ( pAttr->value.GetAsInt( value ) )
                        label.Format( _T("%s (%i)"), (LPCTSTR) pAttr->label, value );
                     else
                        label = pAttr->label;
                     }
                  else
                     label = pAttr->label;
                  }
               else
                  label = pBin->m_label;

               pData->label = label;
               m_itemDataArray.Add( pData );

               index = m_list.AddString( label );
               m_list.SetItemData( index /*itemCount++*/, (DWORD_PTR) pData );
               m_list.SetItemHeight( index, m_list.m_binHeight );
               }

            if ( binCount > 0 )
               {
               // add one more for the binArray graph
               BinArray *pBinArray = pLayer->GetBinArray( activeField, false );

               if ( pBinArray != NULL )
                  {
                  pData = new MAPLIST_ITEM_DATA( pLayer, pBinArray );  // set type = MID_BINGRAPH;
                  pData->label = pLayer->m_name;
                  }
               else
                  pData = NULL;

               m_itemDataArray.Add( pData );

               index = m_list.AddString( pData->label );
               m_list.SetItemData( index /*itemCount++*/, (DWORD_PTR) pData );
               m_list.SetItemHeight( index, m_list.m_binGraphHeight );
               }
            }
         }

      // see if layer is in show layer list
      //bool found = false;
      //for ( int j=0; j < m_list.m_showLayerFlagArray.GetSize(); j++ )
      //   if ( m_list.m_showLayerFlagArray[ j ].pLayer == pLayer )
      //      {
      //      found = true;
      //      break;
      //      }
      //
      //if ( ! found )
      //   m_list.m_showLayerFlagArray.Add( SHOW_LAYER( pLayer, pLayer->m_expandLegend ? true : false ) );
      }  // end of:  for ( i < layerCount )
   }


void MapListWnd::RedrawHistograms()
   {
   CDC *pDC = GetDC();

   CRect rect;
   m_list.GetClientRect( &rect );

   int scroll = m_list.GetScrollPos(SB_VERT);  // note: this is the index of the list box item at the top of the list box

   CBrush bkgrBrush;
   DWORD windowColor = GetSysColor( COLOR_WINDOW );
   bkgrBrush.CreateSolidBrush( windowColor );

   for ( int i=scroll; i < m_itemDataArray.GetSize(); i++ )
      {
      MAPLIST_ITEM_DATA *pData = GetListItem( i );

      switch( pData->type )
         {
         case MID_LAYER:
            rect.bottom = rect.top + m_list.m_layerHeight;
            break;

         case MID_FIELDLABEL:
            rect.bottom = rect.top + m_list.m_fieldLabelHeight;
            break;

         case MID_BIN:
            rect.bottom = rect.top + m_list.m_binHeight;
            break;

         case MID_BINGRAPH:
            rect.bottom = rect.top + m_list.m_binGraphHeight;
            pDC->FillRect( &rect, &bkgrBrush );
            m_list.DrawHistogram( pDC, rect, pData );
            break;
         }

      rect.top = rect.bottom;
      }

   ReleaseDC( pDC );
   }





/////////////////////////////////////////////////////////////////////////////
// MapListBox

MapListBox::MapListBox()
:  m_layerHeight( -1 ),
   m_fieldLabelHeight(-1 ),
   m_binHeight( -1 ),
   m_binGraphHeight( -1 ),
   m_checkSize( -1 ),
   m_indent( -1 ),
   m_buttonSize( -1 )
{
}

MapListBox::~MapListBox()
{
}



BEGIN_MESSAGE_MAP(MapListBox, CListBox)
	//{{AFX_MSG_MAP(MapListBox)
	ON_WM_LBUTTONUP()
	ON_WM_MEASUREITEM()
	ON_WM_DRAWITEM()
	ON_WM_RBUTTONDOWN()
	//}}AFX_MSG_MAP
   ON_WM_LBUTTONDBLCLK()
   ON_WM_MOUSEMOVE()
   ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// MapListBox message handlers

const int leftMargin = 3;

void MapListBox::MeasureItem( LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
   {
	// TODO: Add your message handler code here and/or call default
   CDC *pDC = GetDC();

   TEXTMETRIC tm;
   //CFont font;
   CGdiObject *pOldFont = pDC->SelectStockObject( DEFAULT_GUI_FONT );   
   pDC->GetTextMetrics( &tm );
   pDC->SelectObject( pOldFont );
   ReleaseDC( pDC );

   m_layerHeight      = 7*tm.tmHeight/4;    // for a layer
   m_fieldLabelHeight = 5*tm.tmHeight/4;    // for a field label
   m_binHeight        = 5*tm.tmHeight/4;    // for a bin
   m_checkSize        = 2*m_layerHeight/5;
   m_indent           = leftMargin*2 + m_checkSize;   
   m_buttonSize       = m_checkSize*3/4;
   m_binGraphHeight   = 4 * tm.tmHeight;    // for a bin graph
   if ( m_binGraphHeight > 255 )
      m_binGraphHeight = 255;

   int itemID = lpMeasureItemStruct->itemID;
   MAPLIST_ITEM_DATA *pData = ((MapListWnd*)this->GetParent())->GetListItem( itemID );

   switch( pData->type )
      {
      case MID_LAYER:
         lpMeasureItemStruct->itemHeight = m_layerHeight;
         break;

      case MID_FIELDLABEL:
         lpMeasureItemStruct->itemHeight = m_fieldLabelHeight;
         break;

      case MID_BIN:
         lpMeasureItemStruct->itemHeight = m_binHeight;
         break;

      case MID_BINGRAPH:
         lpMeasureItemStruct->itemHeight = m_binGraphHeight;
         break;

      case MID_NULL:
         lpMeasureItemStruct->itemHeight = 1;
         break;

      default:
         ASSERT( 0 );
      }

   lpMeasureItemStruct->itemWidth  = GetSystemMetrics( SM_CXSCREEN );
   }


void MapListBox::DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct) 
   {
   int item  = lpDrawItemStruct->itemID;

   if ( item < 0 )
      return;

   // get the corresponding layer on the map
   MAPLIST_ITEM_DATA *pData = (MAPLIST_ITEM_DATA*) GetItemData( item );

   MapWindow *pMapWnd = ((MapListWnd*) GetParent())->m_pMapWnd;
   Map *pMap = pMapWnd->m_pMap;
   MapLayer *pLayer = pData->pLayer;

   CDC *pDC = CDC::FromHandle( lpDrawItemStruct->hDC );

   int saveDC = pDC->SaveDC();

   CRect rect( lpDrawItemStruct->rcItem );
   CBrush bkgrBrush;

   DWORD windowColor = GetSysColor( COLOR_WINDOW );
   //DWORD windowColor = GetSysColor( COLOR_3DFACE );

   bkgrBrush.CreateSolidBrush( windowColor );
  
   pDC->FillRect( &rect, &bkgrBrush );
   pDC->SetBkMode( TRANSPARENT );

   switch( pData->type )
      {
      case MID_LAYER:
         {
         ASSERT( pData->pLayer != NULL );

         // draw visible check box
         int x = leftMargin;
         int y = rect.top + (m_layerHeight - m_checkSize)/2 + 2;

         pDC->Rectangle( x, y, x+m_checkSize+1, y+m_checkSize+1 );

         if ( pLayer->IsVisible() )
            {
            pDC->MoveTo( x, y );
            pDC->LineTo( x+m_checkSize, y+m_checkSize );
            pDC->MoveTo( x+m_checkSize, y );
            pDC->LineTo( x, y+m_checkSize );
            }

         // write out layer name
         if ( pMap->GetActiveLayer() == pLayer )
            {
            CFont *pOldFont = pDC->SelectObject( &m_layerFont );
            pDC->TextOut( m_indent, y-2, pData->label );
            pDC->SelectObject( pOldFont );
            }
         else
            {
            CFont *pOldFont = pDC->SelectObject( &m_font );
            pDC->TextOut( m_indent, y-2, pData->label );
            pDC->SelectObject( pOldFont );
            }
/*
         if ( pLayer->m_pMapWnd->GetActiveLayer() == pLayer )
            {
            // edges
            int flag = BF_LEFT | BF_RIGHT | BF_TOP;
            // no bins?
            int binCount = pLayer->GetBinCount();
            if ( binCount == 0 )
               flag |= BF_BOTTOM;
            pDC->DrawEdge( rect, EDGE_RAISED, flag );
            } */
         }
         break;

      case MID_FIELDLABEL:
         {
         int x = m_indent;
         int y = rect.top + (m_fieldLabelHeight - m_buttonSize)/2 + 2;

         // draw button
         pDC->Rectangle( x, y, x+m_buttonSize+1, y+m_buttonSize+1 );

         // show whether expanded (+) or collapsed (-)
         pDC->MoveTo( x+2, y+(m_buttonSize+1)/2 );
         pDC->LineTo( x+m_buttonSize-1, y+(m_buttonSize+1)/2 ); 

         if ( IsExpanded( pLayer ) )
            {
            pDC->MoveTo( x+(m_buttonSize+1)/2, y+2 );
            pDC->LineTo( x+(m_buttonSize+1)/2, y+m_buttonSize-1 ); 
            }

         pDC->TextOut( m_indent+m_buttonSize+leftMargin, y-4, pData->label );
         }
         break;

      case MID_BIN:
         {
         ASSERT( pData->pBin != NULL );
         Bin *pBin = pData->pBin;

         CBrush binBrush;
         if ( pBin->m_color != -1 )
            binBrush.CreateSolidBrush( pBin->m_color );
         else
            binBrush.CreateStockObject( NULL_BRUSH );

         CBrush *pOldBrush = pDC->SelectObject( &binBrush );

         int x = m_indent + m_buttonSize + leftMargin; 
         int y = rect.top + m_binHeight / 5;
         int box = 2 * m_binHeight / 3;

         pDC->Rectangle( x, y, x+box, y+box );
         pDC->SelectObject( pOldBrush );

         x += m_binHeight;
     
         pDC->TextOut( x, y, pData->label );

         if ( pMap->GetActiveLayer() == pLayer )
            {
            // edges
            int flag = BF_LEFT | BF_RIGHT;
            // last bin?
            int binCount = pLayer->GetBinCount( USE_ACTIVE_COL );
            if ( binCount > 0 )
               {
               if ( pBin == &pLayer->GetBin( USE_ACTIVE_COL, binCount-1 ) )
                  flag |= BF_BOTTOM;
               }

            //pDC->DrawEdge( rect, EDGE_RAISED, flag );
            }
         }
         break;

      case MID_BINGRAPH:
         DrawHistogram( pDC, rect, pData );
         break;

      default:
         ASSERT( 0 );
      }

   pDC->RestoreDC( saveDC );
   }



void MapListBox::DrawHistogram( CDC *pDC, RECT &rect, MAPLIST_ITEM_DATA *pData )
   {
   ASSERT( pData->pLayer != NULL );
   ASSERT( pData->pBinArray != NULL );
   ASSERT( pData->type = MID_BINGRAPH );

   MapLayer *pLayer = pData->pLayer;

   int x = leftMargin;
   int width = rect.right - 3*leftMargin;
   int y = rect.top;
   int height = rect.bottom - rect.top - 6;

   // first, draw axes
   pDC->MoveTo( x, y+2 );
   pDC->LineTo( x, y+height );
   pDC->LineTo( x+width, y+height );

   BinArray *pBinArray = pData->pBinArray;

   int binCount = (int) pBinArray->GetSize();
   int binWidth = width / binCount;

   CPen *pOldPen = (CPen*) pDC->SelectStockObject( NULL_PEN );

   float maxArea = 0;
   int   maxPopSize = 0;
   for ( int i=0; i < binCount; i++ )
      {
      Bin &bin = pBinArray->ElementAt( i );

      if ( bin.m_area > maxArea )
         maxArea = bin.m_area;

      if ( bin.m_popSize > maxPopSize )
         maxPopSize = bin.m_popSize;
      }

   x += leftMargin;

   float totalArea = pLayer->GetTotalArea();
   for ( int i=0; i < binCount; i++ )
      {
      // draw each bin
      Bin &bin = pBinArray->ElementAt( i );

      CBrush brush;
      if ( bin.m_color == -1 )
         brush.CreateStockObject( NULL_BRUSH );
      else
         brush.CreateSolidBrush( bin.m_color );

      CBrush *pOldBrush = (CBrush*) pDC->SelectObject( &brush );

      int binHeight = 0;
            
      if ( totalArea < 1 && maxPopSize != 0 )
         binHeight = int( height * bin.m_popSize / maxPopSize );
      else if ( totalArea > 1 && maxArea > 0.0001f )
         binHeight = int( height * bin.m_area / maxArea);

      pDC->Rectangle( x, y+height+1, x+binWidth, y+height-binHeight );

      pDC->SelectObject( pOldBrush );

      x += binWidth;
      }

   pDC->SelectObject( pOldPen );
   }
   

bool MapListBox::IsExpanded( MapLayer *pLayer )
   {
   for ( int i=0; i < m_showLayerFlagArray.GetSize(); i++ )
      if ( m_showLayerFlagArray[ i ].pLayer == pLayer )
         return m_showLayerFlagArray[ i ].showLayer;

   return true;   // default, if layer not found
   }


void MapListBox::OnLButtonUp(UINT nFlags, CPoint point) 
   {
   bool isInCheckBox = false;
   int layerIndex = IsInLayerLine( point, isInCheckBox );  // note: this returns layer index, not draw index
   int binIndex;

   if ( layerIndex >= 0 )      
      {
      // get the corresponding layer from the map
      MapWindow *pMapWnd = ((MapListWnd*) GetParent())->m_pMapWnd;
      Map *pMap = pMapWnd->m_pMap;
      MapLayer *pLayer = pMap->GetLayer( layerIndex );

      if ( pLayer->IsVisible() )
         pLayer->Hide();
      else
         pLayer->Show();

      pMapWnd->RedrawWindow();
      RedrawWindow();
      }

   else if ( ( layerIndex = IsInBinColorBox( point, binIndex ) ) >= 0 && binIndex >= 0 )      // return layer and bin
      {
      MapWindow *pMapWnd = ((MapListWnd*) GetParent())->m_pMapWnd;
      Map *pMap = pMapWnd->m_pMap;
      MapLayer *pLayer = pMap->GetLayer( layerIndex );

      Bin &bin = pLayer->GetBin( USE_ACTIVE_COL, binIndex );

      CColorDialog dlg;
      dlg.m_cc.rgbResult = bin.m_color;
      dlg.m_cc.Flags |= CC_ANYCOLOR | CC_RGBINIT;

      if ( dlg.DoModal() == IDOK )
         {
         bin.m_color = dlg.GetColor();

         ((MapListWnd*) GetParent())->Refresh();
         pMapWnd->RedrawWindow();
         }
      }

   else if ( ( layerIndex = IsInExpandButton( point ) ) >= 0 )
      {
      // find layer in show array
      MapWindow *pMapWnd = ((MapListWnd*) GetParent())->m_pMapWnd;
      Map *pMap = pMapWnd->m_pMap;

      MapLayer *pLayer = pMap->GetLayer( layerIndex );

      for ( int i=0; i < m_showLayerFlagArray.GetSize(); i++ )
         {
         if ( m_showLayerFlagArray[ i ].pLayer == pLayer )
            {
            if ( m_showLayerFlagArray[ i ].showLayer == true )
               m_showLayerFlagArray[ i ].showLayer = false;
            else
               m_showLayerFlagArray[ i ].showLayer = true;

            break;
            }
         }

      ((MapListWnd*) GetParent())->Refresh();
      }
            
   CListBox::OnLButtonUp(nFlags, point);
   }


void MapListBox::OnLButtonDblClk(UINT nFlags, CPoint point)
   {
   bool inCheckBox = false;
   int layerIndex = IsInLayerLine( point, inCheckBox );
   int binIndex;

   if ( layerIndex >= 0 && inCheckBox == false )      
      {
      // get the corresponding layer from the map
      MapWindow *pMapWnd = ((MapListWnd*) GetParent())->m_pMapWnd;
      Map *pMap = pMapWnd->m_pMap;
      MapLayer *pLayer = pMap->GetLayer( layerIndex );

      pMap->SetActiveLayer( pLayer );

      RedrawWindow();
      }
   
   else if ( ( layerIndex = IsInBinColorBox( point, binIndex ) ) >= 0 && binIndex >= 0 )      // return layer and bin
      {
      MapWindow *pMapWnd = ((MapListWnd*) GetParent())->m_pMapWnd;
      Map *pMap = pMapWnd->m_pMap;
      MapLayer *pLayer = pMap->GetLayer( layerIndex );
   
      Bin &bin = pLayer->GetBin( USE_ACTIVE_COL, binIndex );

      CColorDialog dlg;
      dlg.m_cc.rgbResult = bin.m_color;
      dlg.m_cc.Flags |= CC_ANYCOLOR | CC_RGBINIT;

      if ( dlg.DoModal() == IDOK )
         {
         bin.m_color = dlg.GetColor();

         ((MapListWnd*) GetParent())->Refresh();

         pMapWnd->RedrawWindow();
         }
      }

   CListBox::OnLButtonDblClk(nFlags, point);
   }



//-- MapListBox::IsInExpandButton() --------------------------
//
// returns the layer index if point is in the checkbox of a 
//  layer, -1 otherwise
//------------------------------------------------------------

int MapListBox::IsInExpandButton( CPoint &point )
   {
   // are we within the left and right bounds?
   if ( point.x < m_indent || point.x > (m_indent+m_buttonSize+1 ) )
      return -1;

   int count = GetCount();

   RECT rect;
   for ( int i=0; i < count; i++ )
      {      
      int retVal = GetItemRect( i, &rect );

      ASSERT( retVal != LB_ERR );

      // is the point in the rectangle for this item?
      if ( point.y >= rect.top && point.y <= rect.bottom )
         {
         // found the right item, see what it is...
         MAPLIST_ITEM_DATA *pData = (MAPLIST_ITEM_DATA*) GetItemData( i );

         if ( pData->type != MID_FIELDLABEL )
            return -1;

         MapWindow *pMapWnd = ((MapListWnd*) GetParent())->m_pMapWnd;
         Map *pMap = pMapWnd->m_pMap;

         for ( int j=0; j < pMap->GetLayerCount(); j++ )
            {
            if ( pData->pLayer == pMap->GetLayer( j ) )
               return j;
            }

         return -1;
         }
      }

   return -1;
   }


//-- MapListBox::IsInLayerCheckBox() -----------------------------
//
// returns the layer index if point is in the checkbox of a 
//  layer, -1 otherwise
//------------------------------------------------------------

int MapListBox::IsInLayerLine( CPoint &point, bool &isInCheckBox )
   {
   if ( point.x >= leftMargin && point.x <= leftMargin+m_checkSize+1 )
      isInCheckBox = true;
   else
      isInCheckBox = false;

   int count = GetCount();

   RECT rect;
   // iterate through list box items until right list entry found
   for ( int i=0; i < count; i++ )
      {      
      int retVal = GetItemRect( i, &rect );

      ASSERT( retVal != LB_ERR );

      // is the point in the rectangle for this item?
      if ( point.y >= rect.top && point.y <= rect.bottom )
         {
         // found the right item, see what it is...
         MAPLIST_ITEM_DATA *pData = (MAPLIST_ITEM_DATA*) GetItemData( i );

         if ( pData->type != MID_LAYER )
            {
            isInCheckBox = false;
            return -1;
            }

         MapWindow *pMapWnd = ((MapListWnd*) GetParent())->m_pMapWnd;
         Map *pMap = pMapWnd->m_pMap;

         for ( int j=0; j < pMap->GetLayerCount(); j++ )
            {
            if ( pData->pLayer == pMap->GetLayer( j ) )
               return j;
            }

         isInCheckBox = false;
         return -1;
         }
      }

   isInCheckBox = false;
   return -1;
   }



// returns layer index if found, -1 otherwise
int MapListBox::IsInBinColorBox( CPoint &point, int &binIndex )     // return layer and bin
   {
   // are we within the left and right bounds?
   int box = 2 * m_binHeight / 3;
   int left = m_indent+m_buttonSize+leftMargin;

   if ( point.x < left || point.x > left+box )
      return -1;

   int count = GetCount();

   RECT rect;
   for ( int i=0; i < count; i++ )
      {      
      int retVal = GetItemRect( i, &rect );

      ASSERT( retVal != LB_ERR );

      // is the point in the rectangle for this item?
      if ( point.y >= rect.top && point.y <= rect.bottom )
         {
         // found the right item, see what it is...
         MAPLIST_ITEM_DATA *pData = (MAPLIST_ITEM_DATA*) GetItemData( i );

         if ( pData->type != MID_BIN )
            return -1;

         MapWindow *pMapWnd = ((MapListWnd*) GetParent())->m_pMapWnd;
         Map *pMap = pMapWnd->m_pMap;

         // get bin index
         int binCount = pData->pLayer->GetBinCount( USE_ACTIVE_COL );
         binIndex = -1;

         for ( int j=0; j < binCount; j++ )
            {
            if ( &pData->pLayer->GetBin( USE_ACTIVE_COL, j ) == pData->pBin )
               {
               binIndex = j;
               break;
               }
            }

         for ( int j=0; j < pMap->GetLayerCount(); j++ )
            {
            if ( pData->pLayer == pMap->GetLayer( j ) )
               return j;
            }

         return -1;
         }
      }

   return -1;
   }


// returns layer index if found, -1 otherwise
MAPLIST_ITEM_DATA *MapListBox::IsInHistogram( CPoint &point, int &binIndex )     // return layer and bin
   {
   int count = GetCount();

   RECT rect;
   for ( int i=0; i < count; i++ )
      {      
      int retVal = GetItemRect( i, &rect );

      ASSERT( retVal != LB_ERR );

      // is the point in the rectangle for this item?
      if ( point.y >= rect.top && point.y <= rect.bottom )
         {
         // found the right item, see what it is...
         MAPLIST_ITEM_DATA *pData = (MAPLIST_ITEM_DATA*) GetItemData( i );

         if ( pData->type != MID_BINGRAPH )
            return NULL;

         // okay, we know it is a bin histogram, figure out which bar (if any) we are on.
         MapLayer *pLayer = pData->pLayer;
         Map *pMap = pLayer->m_pMap;

         int x = leftMargin;
         int width = rect.right - 3*leftMargin;
         int y = rect.top;
         int height = rect.bottom - rect.top - 6;

         BinArray *pBinArray = pData->pBinArray;

         int binCount = (int) pBinArray->GetSize();
         int binWidth = width / binCount;

         float maxArea = 0;
         int   maxPopSize = 0;

         for ( int i=0; i < binCount; i++ )
            {
            Bin &bin = pBinArray->ElementAt( i );

            if ( bin.m_area > maxArea )
               maxArea = bin.m_area;

           if ( bin.m_popSize > maxPopSize )
              maxPopSize = bin.m_popSize;
            }

         x += leftMargin;

         float totalArea = pLayer->GetTotalArea();
         for ( int i=0; i < binCount; i++ )
            {
            // draw each bin
            Bin &bin = pBinArray->ElementAt( i );

            int binHeight = 0;
                  
            if ( totalArea < 1 && maxPopSize != 0 )
               binHeight = int( height * bin.m_popSize / maxPopSize );
            else if ( totalArea > 1 && maxArea > 0.0001f )
               binHeight = int( height * bin.m_area / maxArea);

            if ( ::IsPointInRect( CRect( x, y+height+1, x+binWidth, y+height-binHeight ), point, 0 ) )
               {
               binIndex = i;
               return pData;
               }

            x += binWidth;
            }
         }
      }

   binIndex = -1;
   return NULL;
   }


 
int MapListBox::GetIndexFromPoint( CPoint &point )
   {
   int count = GetCount();

   RECT rect;
   for ( int i=0; i < count; i++ )
      {      
      int retVal = GetItemRect( i, &rect );

      ASSERT( retVal != LB_ERR );

      if ( point.y >= rect.top && point.y <= rect.bottom )
         return i;
      }

   return -1;
   }
    

BOOL MapListBox::OnCommand(WPARAM wParam, LPARAM lParam) 
   {
   int index = GetCurSel();

   if ( index < 0 )
      return false;

   // get the corresponding layer from the map
   MapListWnd *pView = (MapListWnd*) GetParent();
   MapWindow *pMapWnd = pView->m_pMapWnd;
   Map *pMap = pMapWnd->m_pMap;
   MapLayer *pLayer = pView->GetListItem( index )->pLayer;

   switch( LOWORD( wParam ) )
      {
      case 100:  // map properties
         if ( RunMapLayerDlg( pMapWnd, pLayer ) == IDOK )
            pMapWnd->RedrawWindow();
         break;

      case 101:  // classify
         {
         pLayer->ClassifyData();
         pView->Refresh();
         pMapWnd->RedrawWindow();
         }
         break;

      case 102:  // hide/show
         {
         if ( pLayer->IsVisible() )
            pLayer->Hide();
         else
            pLayer->Show();

         for ( int i=0; i < this->m_showLayerFlagArray.GetSize(); i++ )
            {
            if ( m_showLayerFlagArray[ i ].pLayer == pLayer )
               {
               m_showLayerFlagArray[ i ].showLayer = pLayer->IsVisible() ? true : false;
               break;
               }
            }

         pView->Refresh();
         pMapWnd->RedrawWindow();
         }
         break;
         
      case 103:   // up
         pMap->ChangeDrawOrder( pLayer, DO_UP );
         pView->Refresh();
         pMapWnd->RedrawWindow();
         break;

      case 104:   // down
         pMap->ChangeDrawOrder( pLayer, DO_DOWN );
         pView->Refresh();
         pMapWnd->RedrawWindow();
         break;

      case 105:   // top
         pMap->ChangeDrawOrder( pLayer, DO_TOP );
         pView->Refresh();
         pMapWnd->RedrawWindow();
         break;

      case 106:   // bottom
         pMap->ChangeDrawOrder( pLayer, DO_BOTTOM );
         pView->Refresh();
         pMapWnd->RedrawWindow();
         break;

      //case 110: // Save Selected Columns
      //   {
      //   MapLayer *pLayer = (MapLayer*) pItemData->pData;
      //   SaveColsDlg dlg( pLayer );
      //   dlg.DoModal();
      //   }
      //   break;

      case 107:      // save
         {
         char *cwd = _getcwd( NULL, 0 );

         CFileDialog dlg( FALSE, "shp",pLayer->m_name, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
         "Shape Files|*.shp|All files|*.*||");

         if ( dlg.DoModal() == IDOK )
            {
            CString filename( dlg.GetPathName() );

            CWaitCursor c;
            pLayer->SaveShapeFile(filename, false);
            }
         
         _chdir( cwd );
         free( cwd );
         }
         break;

      case 109:      // save data
         {
         char *cwd = _getcwd( NULL, 0 );

         CFileDialog dlg( FALSE, "dbf",pLayer->m_tableName , OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
               "DBase Files|*.dbf|All files|*.*||");

         if ( dlg.DoModal() == IDOK )
            {
            CString filename( dlg.GetPathName() );
            bool useWide = VData::SetUseWideChar( true );

            CWaitCursor c;
            pLayer->SaveDataDB( filename ); // uses DBase/CodeBase

            VData::SetUseWideChar( useWide );
            //if ( pLayer == gpCellLayer )
            //   gpDoc->UnSetChanged( CHANGED_COVERAGE );
            pLayer->SetChangedFlag( CF_DATA );
            }

         _chdir( cwd );
         free( cwd );
         }
         break;

      case 110:       // Make Active
         pMap->SetActiveLayer( pLayer );
         pView->Refresh();
         pMapWnd->RedrawWindow();
         break;

      case 111:       // polygon edges
         {
         long edge = pLayer->GetOutlineColor();

         if ( edge == NO_OUTLINE )
            pLayer->SetOutlineColor( RGB( 127, 127, 127 ) );
         else
            pLayer->SetNoOutline();

         pMapWnd->RedrawWindow();
         }
         break;
      
      case 112:       // Show/Hide basemap
         pMapWnd->ShowBkgrImage( pMapWnd->IsBkgrImageVisible() ? false : true );
         pMapWnd->RedrawWindow();
         break;

      case 115:      // refresh
         pView->Refresh();
         pMapWnd->RedrawWindow();
         break;

      case 116:      // Add Overlay
         {
         CWaitCursor c;
         MapLayer *pOverlay = pMap->CreateOverlay( pLayer, OT_SHARED_ALL, pLayer->m_activeField );
         pView->Refresh();  // updates showLayerFlagArray
         pMapWnd->RedrawWindow();
         }
         break;

      case 120:      // Delete
         {
         CWaitCursor c;
         pMap->RemoveLayer( pLayer, true );
         pView->Refresh();
         pMapWnd->RedrawWindow();
         }
         break;

      case 130:
         {
         Bin *pBin = pView->GetListItem( index )->pBin;

         if ( pBin == NULL )
            break;

         CColorDialog dlg;
         dlg.m_cc.rgbResult = pBin->m_color;
         dlg.m_cc.Flags |= CC_ANYCOLOR | CC_RGBINIT;

         if ( dlg.DoModal() == IDOK )
            {
            pBin->m_color = dlg.GetColor();

            pView->Refresh();
            pMapWnd->RedrawWindow();
            }
         }

      case 160:      // transparency
      case 161:
      case 162:
      case 163:
      case 164:
      case 165:
      case 166:
      case 167:
      case 168:
      case 169:
         {
         int pctTrans = ( LOWORD( wParam ) - 160 ) * 10;
         pLayer->SetTransparency( pctTrans );
         pMapWnd->RedrawWindow();
         }
         break;
      }


	return CListBox::OnCommand(wParam, lParam);
   }


/*
int MapListBox::RunMapLayerDlg( MapLayer *pLayer )
   {
   return RunMapLayerDlg( pLayer );
   }
*/

void MapListBox::OnRButtonDown(UINT nFlags, CPoint point) 
   {
   int index = GetIndexFromPoint( point );

   if ( index < 0 )
      return;

   SetCurSel( index );

   // get the corresponding layer from the map
   MapWindow *pMapWnd = ((MapListWnd*) GetParent())->m_pMapWnd;
   MapListWnd *pView = (MapListWnd*) GetParent();
   Map       *pMap = pMapWnd->m_pMap;
   MapLayer *pLayer = pView->GetListItem( index )->pLayer;
   Bin      *pBin   = pView->GetListItem( index )->pBin;

   CMenu menu;
   menu.CreatePopupMenu();

   // is it a bin?
   if ( pBin != NULL )
      menu.AppendMenu( MF_STRING, 130, "Set Bin Color..." );

   else if ( pLayer != NULL )  // its a layer
      {      
      menu.AppendMenu( MF_STRING, 100, "Properties..." );
      menu.AppendMenu( MF_SEPARATOR );

      if( pLayer->m_pData != NULL )   // does the layer have data?
         {
         menu.AppendMenu( MF_STRING , 108, "Show Table..." );
         menu.AppendMenu( MF_STRING , 101, "Classify..." );
         }
      else
         {
         menu.AppendMenu( MF_STRING | MF_GRAYED , 108, "Show Table..." );
         menu.AppendMenu( MF_STRING | MF_GRAYED, 101, "Classify..." );
         }

      // show polygons
      int flag = MF_STRING;

      if ( pLayer->GetOutlineColor() != NO_OUTLINE )
         flag |= MF_CHECKED;

      menu.AppendMenu( flag, 111, "Show Polygon Edges" );

      if ( pMapWnd->GetBkgrImage() != NULL )
         {
         if ( pMapWnd->IsBkgrImageVisible() )
            menu.AppendMenu( MF_STRING | MF_CHECKED, 112, "Show Basemap" );
         else
            menu.AppendMenu( MF_STRING, 112, "Show Basemap" );
         }
      else
         menu.AppendMenu( MF_STRING | MF_GRAYED, 112, "Show Basemap" );

      CMenu transparencyMenu;
      transparencyMenu.CreatePopupMenu();
      transparencyMenu.AppendMenu( MF_STRING, 160, "0%" );
      transparencyMenu.AppendMenu( MF_STRING, 161, "10%" );
      transparencyMenu.AppendMenu( MF_STRING, 162, "20%" );
      transparencyMenu.AppendMenu( MF_STRING, 163, "30%" );
      transparencyMenu.AppendMenu( MF_STRING, 164, "40%" );
      transparencyMenu.AppendMenu( MF_STRING, 165, "50%" );
      transparencyMenu.AppendMenu( MF_STRING, 166, "60%" );
      transparencyMenu.AppendMenu( MF_STRING, 167, "70%" );
      transparencyMenu.AppendMenu( MF_STRING, 168, "80%" );
      transparencyMenu.AppendMenu( MF_STRING, 169, "90%" );
      menu.AppendMenu( MF_POPUP | MF_STRING, (UINT_PTR) transparencyMenu.m_hMenu, "Transparency" );
      
      if ( pLayer->IsVisible() )
         menu.AppendMenu( MF_STRING, 102, "Hide" );
      else
         menu.AppendMenu( MF_STRING, 102, "Show" );

      menu.AppendMenu( MF_SEPARATOR );
      menu.AppendMenu( MF_STRING, 103, "Move Up" );
      menu.AppendMenu( MF_STRING, 104, "Move Down" );
      menu.AppendMenu( MF_STRING, 105, "Move to Top" );
      menu.AppendMenu( MF_STRING, 106, "Move to Bottom" );

      if ( pMap->GetActiveLayer() != pLayer )
         {
         menu.AppendMenu( MF_SEPARATOR );
         menu.AppendMenu( MF_STRING, 110, "Make Active" );
         }
         
      menu.AppendMenu( MF_SEPARATOR );
      menu.AppendMenu( MF_STRING, 116, "Add Overlay" );

      menu.AppendMenu( MF_SEPARATOR );
      menu.AppendMenu( MF_STRING, 120, "Remove" );

      if ( ( pLayer->m_layerType == LT_POLYGON || pLayer->m_layerType == LT_POINT || pLayer->m_layerType == LT_LINE )
            && pLayer->m_pData != NULL )
         {
         menu.AppendMenu( MF_SEPARATOR );
         menu.AppendMenu( MF_STRING, 107, "Save Shapefile" );
         menu.AppendMenu( MF_STRING, 109, "Save Data..." );
         }
      }
   else
      {
      // it a bin array
      return;
      }

   menu.AppendMenu( MF_STRING, 115, "Refresh Legend..." );

   //RECT rect;
   //BOOL ok = tree.GetItemRect( hItem, &rect, TRUE );   // from ul corner of this window
   //tree.ClientToScreen( &rect );
   POINT pt;
   GetCursorPos( &pt );
   menu.TrackPopupMenu( TPM_LEFTALIGN, pt.x, pt.y, this );
   }


void MapListBox::OnMouseMove(UINT nFlags, CPoint point)
   {
   int binIndex = -1;
   MAPLIST_ITEM_DATA *pData = IsInHistogram( point, binIndex );      // return layer and bin

   if ( pData != NULL && binIndex >= 0 )
      {
      BinArray *pBinArray = pData->pBinArray;
      Bin &bin = pBinArray->ElementAt( binIndex );

      //if ( bin.m_area > maxArea )
      //   maxArea = bin.m_area;
      //
      //if ( bin.m_popSize > maxPopSize )
      //   maxPopSize = bin.m_popSize;

      m_ttPosition = point;

      CString tipText;
      tipText = bin.m_label;
      m_toolTip.ShowWindow( SW_SHOW );
      lstrcpyn( m_toolTipContent, tipText, 511 );//,, tipText, 511 );
      bool activate = true;
      if ( activate && ! m_ttActivated  )
         {
         CToolInfo ti;
         m_toolTip.GetToolInfo( ti, this, (UINT_PTR) m_hWnd );
         m_toolTip.SendMessage(TTM_TRACKACTIVATE,(WPARAM)TRUE, (LPARAM) &ti );
   //    m_toolTip.Activate( TRUE );
         m_ttActivated = true;
         }
      else if ( ! activate && m_ttActivated )
         {
         CToolInfo ti;
         m_toolTip.GetToolInfo( ti, this, (UINT_PTR) m_hWnd );
         m_toolTip.SendMessage(TTM_TRACKACTIVATE,(WPARAM)FALSE, (LPARAM) &ti );
   //    m_toolTip.Activate( FALSE );
         m_ttActivated = false;
         }
      
      ClientToScreen( &point );
      point.Offset( 10, -10 );
      m_toolTip.SendMessage(TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(point.x, point.y) ); // Set pos
      }
   else
      {
      lstrcpy( m_toolTipContent, "" );
      m_toolTip.ShowWindow( SW_HIDE );
      }

   CListBox::OnMouseMove(nFlags, point);
   }


void MapListBox::OnMouseLeave()
   {
   m_toolTip.ShowWindow( SW_HIDE );
   CListBox::OnMouseLeave();
   }
