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

//#include "Resource.h"

#include <MapFrame.h>
#include <MapWnd.h>
#include <Maplayer.h>

//#include <EasySize.h>

int SAMapProc( Map *pMap, NOTIFY_TYPE type, int a0, LONG_PTR a1, LONG_PTR extra );


// MapContainer dialog

class MapContainer : public CWnd
{
	DECLARE_DYNAMIC(MapContainer)

public:
	MapContainer(CWnd* pParent = NULL);   // standard constructor
	virtual ~MapContainer();

// Dialog Data
	//enum { IDD = IDD_MAPCONTAINER };

protected:
	//virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   afx_msg void OnCellproperties();
   afx_msg void OnSelectField(UINT nID);

   afx_msg void OnSize( UINT nType, int cx, int cy );
   afx_msg int OnCreate( LPCREATESTRUCT lpCreateStruct );
	DECLARE_MESSAGE_MAP()

public:    
   void SetMapLayer( MapLayer *pLayer );
   void CreateMapWnd();
   void SetStatusMsg(LPCTSTR msg) { if ( m_status.GetSafeHwnd() != 0 ) m_status.SetWindowText( msg ); }
   void RedrawMap() { m_mapFrame.RefreshList(); m_mapFrame.Update();  } //???

   // member data
   //CWnd     *m_pPlaceHolder;

   MapFrame  m_mapFrame;
   CStatic   m_status;
   MapLayer *m_pMapLayer;
  
   int m_currentCell;
   };
