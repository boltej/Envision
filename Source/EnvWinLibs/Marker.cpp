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
//#if defined _MFC
#include "EnvWinLibs.h"
//#else
//#include <windowsb.h>
//#endif

#pragma hdrstop

#include <colors.hpp>
#include <marker.hpp>



//-- DrawMarker() ---------------------------------------------------
//
//-- Draws a marker on the screen (+) at the logical point (x,y)
//-- Note:  We assume that the points should NOT be distorted by the 
//   logical mapping.  
//-------------------------------------------------------------------

void DrawMarker ( HDC hDC, int x, int y, MSTYLE style, COLORREF color, // )
      int size )
   {
   int ySize = size;
   if ( GetMapMode( hDC ) == MM_ANISOTROPIC )
      {
      SIZE vsize;
      GetViewportExtEx( hDC, &vsize );
      float yScale = float( vsize.cy ) / vsize.cx;
      ySize = int( size/yScale );
      }

   switch( style )
      {
      case MS_POINT:
         SetPixel( hDC, x, y, color );
         return;

      case MS_SOLIDSQUARE:
         {
         HBRUSH hBrush = CreateSolidBrush( color );
         hBrush = (HBRUSH) SelectObject( hDC, hBrush );
         Rectangle( hDC, x-size, y-ySize, x+size, y+ySize );
         DeleteObject( SelectObject( hDC, hBrush ) );
         }
         return;

      case MS_HOLLOWSQUARE:
         {
         HPEN hPen = CreatePen( PS_SOLID, 1, color );
         HBRUSH hBrush = (HBRUSH) SelectObject( hDC,
                                       GetStockObject( NULL_BRUSH ) );
         hPen = (HPEN) SelectObject( hDC, hPen );
         Rectangle( hDC, x-size, y-ySize, x+size, y+ySize );
         DeleteObject( SelectObject( hDC, hPen ) );
         SelectObject( hDC, hBrush );
         }
         return;

      case MS_SOLIDCIRCLE:
         {
         HBRUSH hBrush = CreateSolidBrush( color );
         hBrush = (HBRUSH) SelectObject( hDC, hBrush );
         Ellipse( hDC, x-size, y-ySize, x+size, y+ySize );
         DeleteObject( SelectObject( hDC, hBrush ) );
         }
         return;

      case MS_HOLLOWCIRCLE:
         {
         HPEN hPen = CreatePen( PS_SOLID, 1, color );
         HBRUSH hBrush = (HBRUSH) \
                SelectObject( hDC, GetStockObject( NULL_BRUSH ) );
         hPen = (HPEN) SelectObject( hDC, hPen );
         Ellipse( hDC, x-size, y-ySize, x+size, y+ySize );
         DeleteObject( SelectObject( hDC, hPen ) );
         SelectObject( hDC, hBrush );
         }
         return;

      case MS_SOLIDTRIANGLE:
         {
         const float cos30 = 0.867f;
         const float sin30 = 0.5f;

         POINT pt[ 3 ];
         pt[ 0 ].x = (long)(x - size * cos30);          // lower left
         pt[ 0 ].y = (long)(y - ySize * sin30);

         pt[ 1 ].x = (long)(x + size * cos30);          // lower right
         pt[ 1 ].y = (long)(y - ySize * sin30);

         pt[ 2 ].x = (long) x;
         pt[ 2 ].y = (long)(y + ySize );

         HBRUSH hBrush = CreateSolidBrush( color );
         hBrush = (HBRUSH) SelectObject( hDC, hBrush );
         Polygon( hDC, (LPPOINT) &pt, 3 );
         DeleteObject( SelectObject( hDC, hBrush ) );
         }
         return;

      case MS_HOLLOWTRIANGLE:
         {
         const float cos30 = 0.866f;
         const float sin30 = 0.5f;

         POINT pt[ 3 ];
         pt[ 0 ].x = (long)(x - size * cos30);          // lower left
         pt[ 0 ].y = (long)(y - ySize * sin30);

         pt[ 1 ].x = (long)(x + size * cos30);          // lower right
         pt[ 1 ].y = (long)(y - ySize * sin30);

         pt[ 2 ].x = (long) x;
         pt[ 2 ].y = (long) y + ySize;

         HPEN hPen = CreatePen( PS_SOLID, 1, color );
         hPen      = (HPEN) SelectObject( hDC, hPen );
         HBRUSH hBrush = (HBRUSH) \
                 SelectObject( hDC, GetStockObject( NULL_BRUSH ) );
         Polygon( hDC, (LPPOINT) &pt, 3 );
         DeleteObject( SelectObject( hDC, hPen ) );
         SelectObject( hDC, hBrush );
         }
         return;

      case MS_CROSS:
         {
         SetPixel( hDC, x, y, color );

         for ( int i=1; i <= size; i++ )
            {
            SetPixel( hDC, x+i, y, color );
            SetPixel( hDC, x-i, y, color );
            }

         for ( int i=1; i <= ySize; i++ )
            {
            SetPixel( hDC, x, y+i, color );
            SetPixel( hDC, x, y-i, color );
            }

         return;
         }

      default:
         break;
      }

   return;
   }


void InitComboMarker( HWND hDlg, int ctrlID )
   {
   HWND hCombo = GetDlgItem( hDlg, ctrlID );

   for ( UINT i=MS_START; i <= MS_END; i++ )
      SendMessage( hCombo, CB_ADDSTRING, 0, (LPARAM)(LONG) i );
   }


void DrawComboMarker( HWND, DRAWITEMSTRUCT FAR *pItem )
   {
   UINT index = pItem->itemID;
   HDC  hDC   = pItem->hDC;

   if ( (int) index == -1 )
      return;

   //-- paint control action? (Draw control?) --//
   if ( pItem->itemAction & ODA_DRAWENTIRE )
      {
      //-- draw the combo item in its "normal" state --//
      HPEN   hPen   = (HPEN)   GetStockObject( BLACK_PEN );
      HBRUSH hBrush = (HBRUSH) GetStockObject( NULL_BRUSH );
      hPen   = (HPEN)   SelectObject( hDC, hPen   );
      hBrush = (HBRUSH) SelectObject( hDC, hBrush );

      Rectangle( hDC, pItem->rcItem.left-1, pItem->rcItem.top-1,
                      pItem->rcItem.right+1, pItem->rcItem.bottom );

      int xMid  = ( pItem->rcItem.left + pItem->rcItem.right ) / 2;
      int yMid  = ( pItem->rcItem.top + pItem->rcItem.bottom ) / 2;

      DrawMarker( hDC, xMid, yMid, (MSTYLE) index, BLACK, 6 );

      SelectObject( hDC, hPen   );
      SelectObject( hDC, hBrush );
      }  //-- end of ODA_DRAWENTIRE --//

   //-- selectionChange ? --//
   if ( pItem->itemAction & ODA_SELECT )
      {
      HPEN hPen;
      if ( pItem->itemState & ODS_SELECTED )
         hPen = (HPEN) SelectObject( hDC, GetStockObject( BLACK_PEN ) );
      else
         hPen = (HPEN) SelectObject( hDC, GetStockObject( WHITE_PEN ) );

      HBRUSH hBrush = (HBRUSH) SelectObject( hDC, GetStockObject( NULL_BRUSH ) );
      Rectangle( hDC, pItem->rcItem.left, pItem->rcItem.top,
                      pItem->rcItem.right, pItem->rcItem.bottom-1 );
      SelectObject( hDC, hBrush );
      SelectObject( hDC, hPen );
      }  //-- end of ODA_SELECT --//

   return;
   }



void SetComboMarker( HWND hDlg, int ctrlID, MSTYLE marker )
   {
   SendDlgItemMessage( hDlg, ctrlID, CB_SETCURSEL, (WPARAM) marker, 0L );
   }



