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

class MapWindow;
class MapLayer;
class Bin;
class BinArray;

#include <afxtempl.h>

#include "EnvWinLibs.h"

/////////////////////////////////////////////////////////////////////////////
// MapListBox window

enum MID_TYPE { MID_NULL, MID_LAYER, MID_FIELDLABEL, MID_BIN, MID_BINGRAPH };


// structure for storing item information for each item in the list box

struct MAPLIST_ITEM_DATA
   {
   MapLayer *pLayer;     // pointer to corresponding layer
   Bin      *pBin;       // pointer to bin (NULL if this a a layer entry)
   BinArray *pBinArray;  // pointer to bin array (for legend map)
   CString   label;      // label to display (if any)
   MID_TYPE  type;       // the type of the list item

   MAPLIST_ITEM_DATA( MapLayer *_pLayer ) 
      : pLayer( _pLayer ), pBin( NULL ), pBinArray( NULL ), type( MID_NULL ) { }

   MAPLIST_ITEM_DATA( MapLayer *_pLayer, BinArray *_pBinArray ) 
      : pLayer( _pLayer ), pBin( NULL ), pBinArray( _pBinArray ), type( MID_BINGRAPH ) { }

   MAPLIST_ITEM_DATA( MapLayer *_pLayer, Bin *_pBin )
      : pLayer( _pLayer ), pBin( _pBin ), pBinArray( NULL ), type( MID_BIN ) { }
   };


class MLItemDataArray : public CArray< MAPLIST_ITEM_DATA*, MAPLIST_ITEM_DATA* >
   {
   public:
      ~MLItemDataArray() { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i );  RemoveAll(); }
   };



struct SHOW_LAYER
   {
   MapLayer *pLayer;
   bool showLayer;

   SHOW_LAYER() : pLayer( NULL ), showLayer( true ) { }
   SHOW_LAYER( MapLayer *_pLayer, bool _showLayer ) : pLayer( _pLayer ), showLayer( _showLayer ) { }
   };


class MapListBox : public CListBox
{
friend class MapListWnd;

// Construction
public:
	MapListBox();

// Attributes
protected:
   CFont m_font;
   CFont m_layerFont;

public:
   int m_layerHeight;         // for layer
   int m_fieldLabelHeight;    // for field labels
   int m_binHeight;           // for bins
   int m_binGraphHeight;

   int m_checkSize;
   int m_indent;
   int m_buttonSize;

// Operations
public:
   int GetIndexFromPoint( CPoint &point );
   int IsInLayerLine( CPoint &point, bool &isInCheckBox );
   int IsInBinColorBox( CPoint &point, int &binIndex );     // return layer and bin
   int IsInExpandButton( CPoint &point );
   MAPLIST_ITEM_DATA* IsInHistogram( CPoint &point, int &binIndex );      // return layer and bin


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(MapListBox)
   virtual void DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct );
	virtual void MeasureItem( LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~MapListBox();

protected:
   void DrawHistogram( CDC *pDC, RECT &rect, MAPLIST_ITEM_DATA *pData );
   bool IsExpanded( MapLayer *pLayer );
   CArray< SHOW_LAYER, SHOW_LAYER& > m_showLayerFlagArray;

	// Generated message map functions
protected:
	//{{AFX_MSG(MapListBox)
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
   afx_msg void OnMouseMove(UINT nFlags, CPoint point);
   afx_msg void OnMouseLeave();

protected:
   CToolTipCtrl m_toolTip;
   TCHAR    m_toolTipContent[ 512 ];
   CPoint   m_ttPosition;    // client coords
   bool     m_ttActivated;
   };

// MapListWnd

/////////////////////////////////////////////////////////////////////////////
// MapListWnd view

class WINLIBSAPI MapListWnd : public CWnd
{
public:
	MapListWnd();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(MapListWnd)

// Attributes
public:
   MapListBox m_list;

   MapWindow *m_pMapWnd;

protected:
   MLItemDataArray m_itemDataArray;

// Operations
public:
   void Refresh( void );
   void RedrawHistograms( void );

   int AddListItem( MAPLIST_ITEM_DATA *pData ) { return (int) m_itemDataArray.Add( pData ); }
   MAPLIST_ITEM_DATA *GetListItem( INT_PTR i ) { return m_itemDataArray[ i ]; }

// Implementation
public:
	virtual ~MapListWnd();

	// Generated message map functions
protected:
	//{{AFX_MSG(MapListWnd)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()
};


