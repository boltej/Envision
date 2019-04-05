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
// SelectActor.cpp : implementation file
//

#include "stdafx.h"
#include "envision.h"
#include "SelectActor.h"

#include <Actor.h>

extern ActorManager *gpActorManager;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// SelectActor dialog


SelectActor::SelectActor(CWnd* pParent /*=NULL*/)
	: CDialog(SelectActor::IDD, pParent)
{
	//{{AFX_DATA_INIT(SelectActor)
	m_index = -1;
	//}}AFX_DATA_INIT
}


void SelectActor::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(SelectActor)
	DDX_Control(pDX, IDC_ACTORS, m_actors);
	DDX_CBIndex(pDX, IDC_ACTORS, m_index);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(SelectActor, CDialog)
	//{{AFX_MSG_MAP(SelectActor)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// SelectActor message handlers

BOOL SelectActor::OnInitDialog() 
   {
	CDialog::OnInitDialog();

   CString name;
   for ( int i=0; i < gpActorManager->GetActorCount(); i++ )
      {
      name.Format( "%s_%i", (LPCTSTR) gpActorManager->GetActor(i)->m_pGroup->m_name, i );
      m_actors.AddString( name );
      }

   m_actors.SetCurSel( 0 );

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
   }
