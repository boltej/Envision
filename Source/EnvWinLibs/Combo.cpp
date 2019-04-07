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
#include "EnvWinLibs.h"
#pragma hdrstop

#include <combo.hpp>

#include <typedefs.h>
#include <colors.hpp>

//#include <scatter.hpp>


extern COLORREF gColorRef[];


//-------------------------- Hatch Combo --------------------------//

//-- note: indexes are: 
// 0 = BS_NULL
// 1 = BS_SOLID 
// 2 = HS_HORIZONTAL
// 3 = HS_VERTICAL
// 4 = HS_FDIAGONAL
// 5 = HS_BDIAGONAL
// 6 = HS_CROSS
// 7 = HS_DIAGCROSS

int hatchMap[ 8 ] = { BS_NULL, BS_SOLID, HS_HORIZONTAL, HS_VERTICAL, \
                      HS_FDIAGONAL, HS_BDIAGONAL, HS_CROSS, HS_DIAGCROSS };


void InitComboHatch( HWND hDlg, int ctrlID  )
   {
   HWND hCombo = GetDlgItem( hDlg, ctrlID );

   for ( int i=0; i < 8; i++ )
      SendMessage( hCombo, CB_ADDSTRING, 0, (LPARAM)(LONG) i );

   SendDlgItemMessage( hDlg, ctrlID, CB_SETCURSEL, 0, 0L );
   }


void DrawComboHatch( HWND, DRAWITEMSTRUCT FAR *pItem )
   {
   UINT index = pItem->itemID;
   HDC  hDC   = pItem->hDC;

   if ( (int) index == -1 )
      return;

   //-- paint control action? (Draw control?) --//
   if ( pItem->itemAction & ODA_DRAWENTIRE )
      {
      LOGBRUSH logBrush;
      logBrush.lbColor = WHITE;

      //-- draw the combo item in its "normal" state --//
      if ( index == 0 )       // BS_NULL
         logBrush.lbStyle = BS_NULL;

      else if ( index == 1 )  // BS_SOLID
         {
         logBrush.lbStyle = BS_SOLID;
         logBrush.lbColor = BLACK;
         }
      else
         {   
         logBrush.lbStyle = BS_HATCHED;
         logBrush.lbHatch= hatchMap[ index ];
         logBrush.lbColor = BLACK;
         }

      HBRUSH hBrush = CreateBrushIndirect( &logBrush );
      hBrush        = (HBRUSH) SelectObject( hDC, hBrush );
      HPEN hPen     = (HPEN) GetStockObject( BLACK_PEN );
      hPen          = (HPEN) SelectObject( hDC, hPen );
      Rectangle( hDC, pItem->rcItem.left-1, pItem->rcItem.top-1,
                      pItem->rcItem.right+1, pItem->rcItem.bottom );
      DeleteObject( SelectObject( hDC, hBrush ) );
      SelectObject( hDC, hPen );
      }  //-- end of ODA_DRAWENTIRE --//

   /*-----------------------------
   //-- gaining focus? --//
   if ( ( pItem->itemAction & ODA_FOCUS )&&( pItem->itemState & ODS_FOCUS ) )
      {
      }  //-- end of ODA_FOCUS --//
   ---------------*/

   //-- selectionChange ? --//
   if ( pItem->itemAction & ODA_SELECT )
      {
      //-- draw the item in
      int    r2     = SetROP2( hDC, R2_NOT );
      HPEN   hPen   = (HPEN) SelectObject( hDC, GetStockObject( BLACK_PEN ) );
      HBRUSH hBrush = (HBRUSH) GetStockObject( NULL_BRUSH );
      hBrush        = (HBRUSH) SelectObject( hDC, hBrush );
      Rectangle( hDC, pItem->rcItem.left, pItem->rcItem.top,
                      pItem->rcItem.right, pItem->rcItem.bottom-1 );
      SetROP2( hDC, r2 );
      SelectObject( hDC, hBrush );
      SelectObject( hDC, hPen );
      }  //-- end of ODA_SELECT --//

   return;
   }


void SetComboHatch( HWND hDlg, int ctrlID, LOGBRUSH *pLogBrush )
   {
   UINT index;

   if ( pLogBrush->lbStyle == BS_NULL )
      index = 0;

   else if ( pLogBrush->lbStyle == BS_HATCHED )
      index = 1;

   else
      {
      index = 2;
      while ( ( hatchMap[ index ] != (int) pLogBrush->lbStyle ) && ( index < 7 ) )
         index++;
      }

   SendDlgItemMessage( hDlg, ctrlID, CB_SETCURSEL, index, 0L );
   }



BOOL GetComboHatchBrush( int index, LOGBRUSH *pLogBrush )
   {
   if ( index == 0 )
      pLogBrush->lbStyle = BS_NULL;

   else if ( index == 1 )
      pLogBrush->lbStyle = BS_SOLID;

   else
      {
      pLogBrush->lbStyle = BS_HATCHED;
      pLogBrush->lbHatch = hatchMap[ index ];
      }

   return TRUE;
   }


//-------------------------- Color Combo --------------------------//

void InitComboColor( HWND hDlg, int ctrlID  )
   {
   HWND hCombo = GetDlgItem( hDlg, ctrlID );

   for ( UINT i=0; i < 16; i++ )
      SendMessage( hCombo, CB_ADDSTRING, 0, (LPARAM) gColorRef[ i ] );

   SendDlgItemMessage( hDlg, ctrlID, CB_SETCURSEL, 0, 0L );
   }


void DrawComboColor( HWND, DRAWITEMSTRUCT FAR *pItem )
   {
   UINT index = pItem->itemID;
   HDC  hDC   = pItem->hDC;

   if ( (int) index == -1 )
      return;

   //-- paint control action? (Draw control?) --//
   if ( pItem->itemAction & ODA_DRAWENTIRE )
      {
      //-- draw the combo item in its "normal" state --//
      HBRUSH hBrush = CreateSolidBrush( (COLORREF) pItem->itemData );
      hBrush        = (HBRUSH) SelectObject( hDC, hBrush );
      HPEN hPen     = (HPEN) GetStockObject( BLACK_PEN );
      hPen          = (HPEN) SelectObject( hDC, hPen );
      Rectangle( hDC, pItem->rcItem.left-1, pItem->rcItem.top-1,
                      pItem->rcItem.right+1, pItem->rcItem.bottom );
      DeleteObject( SelectObject( hDC, hBrush ) );
      SelectObject( hDC, hPen );
      }  //-- end of ODA_DRAWENTIRE --//

   /*-----------------------------
   //-- gaining focus? --//
   if ( ( pItem->itemAction & ODA_FOCUS )&&( pItem->itemState & ODS_FOCUS ) )
      {
      }  //-- end of ODA_FOCUS --//
   ---------------*/

   //-- selectionChange ? --//
   if ( pItem->itemAction & ODA_SELECT )
      {
      if ( pItem->itemState & ODS_SELECTED )
         {
         //-- draw the item in
         int    r2     = SetROP2( hDC, R2_NOT );
         HPEN   hPen   = (HPEN) SelectObject( hDC, GetStockObject( BLACK_PEN ) );
         HBRUSH hBrush = (HBRUSH) GetStockObject( NULL_BRUSH );
         hBrush        = (HBRUSH) SelectObject( hDC, hBrush );
         Rectangle( hDC, pItem->rcItem.left, pItem->rcItem.top,
                         pItem->rcItem.right, pItem->rcItem.bottom-1 );
         SetROP2( hDC, r2 );
         SelectObject( hDC, hBrush );
         SelectObject( hDC, hPen );
         }
      else
         {
         HPEN   hPen   = (HPEN) SelectObject( hDC, GetStockObject( BLACK_PEN ) );
         HBRUSH hBrush = CreateSolidBrush( (COLORREF) pItem->itemData );
         hBrush        = (HBRUSH) SelectObject( hDC, hBrush );

         Rectangle( hDC, pItem->rcItem.left-1, pItem->rcItem.top-1,
                      pItem->rcItem.right+1, pItem->rcItem.bottom );

         DeleteObject( SelectObject( hDC, hBrush ) );
         SelectObject( hDC, hPen );
         }
      }  //-- end of ODA_SELECT --//

   return;
   }

void SetComboColor( HWND hDlg, int ctrlID, COLORREF color )
   {
   int index = 0;

   while ( gColorRef[ index ] != color && index < 15 )
      index++;

   SendDlgItemMessage( hDlg, ctrlID, CB_SETCURSEL, index, 0L );
   }


LONG GetComboColor( int index )
   {
   return gColorRef[ index ];
   }


//---------------------- LineStyle Combo --------------------------//
//
// PS_SOLID 1
// PS_SOLID 2:
// PS_SOLID 3:
// PS_SOLID 4:
// PS_SOLID 5:
//   ...
// PS_SOLID + 1
//   ...
// PS_NULL

void InitComboLineStyle( HWND hDlg, int ctrlID )
   {
   HWND hCombo = GetDlgItem( hDlg, ctrlID );
   int i = 0;

   //-- line widths (store negative width) --//
   for ( i=1; i < 6; i++ )
      ::SendMessage( hCombo, CB_ADDSTRING, 0, (LPARAM)(LONG) -i );

   //-- various line styles --//
   for ( i=PS_SOLID+1; i < PS_NULL; i++ )
      ::SendMessage( hCombo, CB_ADDSTRING, 0, (LPARAM)(LONG) i );

   ::SendDlgItemMessage( hDlg, ctrlID, CB_SETCURSEL, 0, 0L );
   }


void DrawComboLineStyle( HWND , DRAWITEMSTRUCT FAR *pItem )
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

      //-- set up pen style --//
      int style = (int) pItem->itemData;

      if ( style < 0 )
         hPen = CreatePen( PS_SOLID, -style, BLACK );
      else
         hPen = CreatePen( style, 0, BLACK );

      hPen = (HPEN) SelectObject( hDC, hPen );

      int yMid = ( pItem->rcItem.top + pItem->rcItem.bottom ) / 2;

      MoveToEx( hDC, pItem->rcItem.left  + 4, yMid, NULL );
      LineTo( hDC, pItem->rcItem.right - 4, yMid );

      DeleteObject( SelectObject( hDC, hPen ) );
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


void SetComboLineStyle( HWND hDlg, int ctrlID, LOGPEN *pLogPen )
   {
   if ( pLogPen->lopnStyle == PS_SOLID )
      {
      int width = ( ( pLogPen->lopnWidth.x == 0 ) ? 1 : pLogPen->lopnWidth.x );
      SendDlgItemMessage( hDlg, ctrlID, CB_SETCURSEL, width-1, 0L );
      }
   else
      SendDlgItemMessage( hDlg, ctrlID, CB_SETCURSEL, pLogPen->lopnStyle+5, 0L );
   }


BOOL GetComboLineStyle( int index, LOGPEN *pLogPen )
   {
   if ( index < 5 )
      {
      pLogPen->lopnStyle = PS_SOLID;
      pLogPen->lopnWidth.x = index+1;
      }

   else
      {
      pLogPen->lopnStyle   = index-4;
      pLogPen->lopnWidth.x = 0;
      }

   return TRUE;
   }


//----------------------- LineWidth Combo -------------------------//

void InitComboLineWidth( HWND hDlg, int ctrlID )
   {
   HWND hCombo = GetDlgItem( hDlg, ctrlID );

   for ( UINT i=1; i < 7; i++ )
      SendMessage( hCombo, CB_ADDSTRING, 0, (LPARAM)(LONG) i );

   SendDlgItemMessage( hDlg, ctrlID, CB_SETCURSEL, 0, 0L );
   }


void DrawComboLineWidth( HWND, DRAWITEMSTRUCT FAR *pItem )
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

      hPen = CreatePen( PS_SOLID, index+1, BLACK );
      hPen = (HPEN) SelectObject( hDC, hPen );
      int yMid  = ( pItem->rcItem.top + pItem->rcItem.bottom ) / 2;

      MoveToEx( hDC, pItem->rcItem.left  + 4, yMid, NULL );
      LineTo( hDC, pItem->rcItem.right - 4, yMid );

      DeleteObject( SelectObject( hDC, hPen ) );
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



void SetComboLineWidth( HWND hDlg, int ctrlID, int penWidth )
   {
   SendDlgItemMessage( hDlg, ctrlID, CB_SETCURSEL, penWidth, 0L );
   }

