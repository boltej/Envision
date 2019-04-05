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

#include "ppinibasic.h"
//#include "ppiniActors.h"
#include "ppiniLayers.h"
#include "ppiniModels.h"
#include "ppiniSettings.h"
#include "ppiniProcesses.h"
#include "PPIniMetagoals.h"

#include <tabctrlssl.h>


// m_isDirty flags

const int INI_BASIC     = 1;
const int INI_SETTINGS  = 2;
const int INI_ACTORS    = 4;
const int INI_LAYERS    = 8;
const int INI_METAGOALS = 16;
const int INI_MODELS    = 32;
const int INI_PROCESSES = 64;


// IniFileEditor dialog

class IniFileEditor : public CDialog
{
	DECLARE_DYNAMIC(IniFileEditor)

public:
	IniFileEditor(CWnd* pParent = NULL);   // standard constructor
	virtual ~IniFileEditor();

// Dialog Data
	enum { IDD = IDD_INIEDITOR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   CTabCtrlSSL m_tabCtrl;

public:
   PPIniBasic      m_basic;
   PPIniSettings   m_settings;
   //PPIniActors     m_actors;
   PPIniLayers     m_layers;
   PPIniMetagoals  m_metagoals;
   PPIniModels     m_models;
   PPIniProcesses  m_processes;

protected:
   int m_isDirty;

public:
   void MakeClean( int flag ) { m_isDirty &= ( ~flag ); }
   void MakeDirty( int flag ) { m_isDirty |= flag; }
   bool IsDirty( int flag ) { return m_isDirty | flag ? true : false; }

protected:
   bool StoreChanges( void );
   bool SaveAs( void );

protected:
   virtual BOOL OnInitDialog();
   virtual void OnOK();
   virtual void OnCancel();
	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnBnClickedSaveas();
   afx_msg void OnBnClickedSave();
   };
