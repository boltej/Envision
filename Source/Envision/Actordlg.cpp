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
// ActorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "envision.h"

#include <MapLayer.h>
#include "envdoc.h"
#include <QueryBuilder.h>

#include ".\actordlg.h"

#include <DirPlaceholder.h>


extern CEnvDoc  *gpDoc;
extern EnvModel *gpModel;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ActorDlg dialog

ActorDlg::ActorDlg(CWnd* pParent /*=NULL*/)
	: CDialog(ActorDlg::IDD, pParent),
     m_isDirty( false ),
     m_isSetDirty( false ),
     m_pActorGroup( NULL )
   { }


void ActorDlg::DoDataExchange(CDataExchange* pDX)
   {
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ActorDlg)
   DDX_Radio(pDX, IDC_NOACTORS, m_actorInitMethod );
   
   DDX_Control(pDX, IDC_ACTORS, m_actorGroups);
   
	DDX_Text(pDX, IDC_NAME, m_groupName);
	DDX_Text(pDX, IDC_QUERY, m_query);
   DDX_Text(pDX, IDC_ID, m_id );
   DDX_Text(pDX, IDC_FREQ, m_freq);

   DDX_Control(pDX, IDC_WTSLIDER, m_wtSlider);
	DDX_Control(pDX, IDC_OBJECTIVES, m_objectives);
	//}}AFX_DATA_MAP
   }

BEGIN_MESSAGE_MAP(ActorDlg, CDialog)
	ON_BN_CLICKED(IDC_NOACTORS,   OnAim)
	ON_BN_CLICKED(IDC_IDU_WTS,    OnAim)
	ON_BN_CLICKED(IDC_IDU_GROUPS, OnAim)
	ON_BN_CLICKED(IDC_FROMQUERY,  OnAim)
	ON_BN_CLICKED(IDC_UNIFORM,    OnAim)

   ON_CBN_SELCHANGE(IDC_ACTORS, OnSelchangeActors)
   ON_CBN_EDITUPDATE(IDC_ACTORS, OnEditupdateActors)
	
   ON_LBN_SELCHANGE(IDC_OBJECTIVES, OnSelchangeObjectives)
	
   ON_BN_CLICKED(IDC_DELETE, OnDelete)
	ON_BN_CLICKED(IDC_COPY, OnCopy)
	ON_BN_CLICKED(IDC_APPLY, OnApply)
	ON_BN_CLICKED(IDC_NEXT, OnNext)
	ON_BN_CLICKED(IDC_PREV, OnPrev)

   ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_WTSLIDER, OnNMReleasedcaptureWtslider)
   ON_EN_CHANGE(IDC_NAME, &ActorDlg::OnEnChangeName)
   ON_EN_CHANGE(IDC_QUERY, &ActorDlg::OnEnChangeQuery)
   ON_BN_CLICKED(IDC_DEFINEQUERY, &ActorDlg::OnBnClickedDefinequery)
   ON_BN_CLICKED(IDC_ADD, &ActorDlg::OnAdd)
   ON_EN_CHANGE(IDC_ID, &ActorDlg::OnEnChangeId)
   ON_EN_CHANGE(IDC_FREQ, &ActorDlg::OnEnChangeFreq)
   ON_BN_CLICKED(IDC_EXPORT, &ActorDlg::OnBnClickedExport)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ActorDlg message handlers

BOOL ActorDlg::OnInitDialog() 
   {
	CDialog::OnInitDialog();

   m_actorInitMethod = (int) gpModel->m_pActorManager->m_actorInitMethod;

   int groups = gpModel->m_pActorManager->GetActorGroupCount();

   for ( int i=0; i < groups; i++ )
      {
      ActorGroup *pGroup = gpModel->m_pActorManager->GetActorGroup( i );
      m_actorGroupArray.Add( new ActorGroup( *pGroup ) );
      m_actorGroups.AddString( pGroup->m_name );
      }

   if ( groups > 0 )
      m_actorGroups.SetCurSel( 0 );

   m_wtSlider.SetRange( -30, 30 );
   m_wtSlider.SetTicFreq( 10 );
   //m_color.ShowWindow( SW_HIDE );

   LoadActorGroup();
   m_isDirty = false;
   SetApplyButton();

   UpdateData( 0 );

   EnableControls();
	
	return TRUE;
   }


void ActorDlg::LoadActorGroup()
   {
   // objectives
   m_objectives.ResetContent();
   int index = m_actorGroups.GetCurSel();

   if ( index >= 0 )
      m_pActorGroup = m_actorGroupArray[ index ];
   else
      {
      m_pActorGroup = NULL;
      return;
      }
   
   m_groupName = m_pActorGroup->m_name;
   m_query     = m_pActorGroup->m_query;
   m_id        = m_pActorGroup->m_id;
   m_freq      = m_pActorGroup->m_decisionFrequency;

   UpdateData( 0 );

   int valueCount = m_pActorGroup->GetValueCount();

   for ( int i=0; i < valueCount; i++ )
      {
      METAGOAL *pGoal = gpModel->GetActorValueMetagoalInfo( i );      
      m_objectives.AddString( pGoal->name );
      }
   
   if( valueCount > 0 )
      {
      m_objectives.SetCurSel( 0 );
      m_objective = 0;
      SetObjectiveWeight();
      }
   else
      {
      m_objectives.SetCurSel( -1 );
      m_objective = -1;
      }
   }


void ActorDlg::EnableControls()
   {
   BOOL enableActorGroups = TRUE;
   BOOL enableQuery = TRUE;
   BOOL enableAddCopyDelete = TRUE;

   switch( m_actorInitMethod )
      {
      case AIM_NONE:
         enableActorGroups = FALSE;
         enableQuery = FALSE;
         enableAddCopyDelete = FALSE;
         break;
         
      case AIM_IDU_WTS:
         enableActorGroups = TRUE;
         enableQuery = FALSE;
         enableAddCopyDelete = FALSE;
         break;

      case AIM_IDU_GROUPS:
         enableActorGroups = TRUE;
         enableQuery = FALSE;
         enableAddCopyDelete = TRUE;
         break;

      case AIM_QUERY:
         enableActorGroups = TRUE;
         enableQuery = TRUE;
         enableAddCopyDelete = TRUE;
         break;

      case AIM_UNIFORM:
         enableActorGroups = TRUE;
         enableQuery = FALSE;
         enableAddCopyDelete = FALSE;
         break;

      default:
         ASSERT( 0 );
      }

   GetDlgItem( IDC_ACTORS     )->EnableWindow( enableActorGroups );
   GetDlgItem( IDC_PREV       )->EnableWindow( enableAddCopyDelete );
   GetDlgItem( IDC_NEXT       )->EnableWindow( enableAddCopyDelete );

   GetDlgItem( IDC_ADD        )->EnableWindow( enableAddCopyDelete );
   GetDlgItem( IDC_COPY       )->EnableWindow( enableAddCopyDelete );
   GetDlgItem( IDC_DELETE     )->EnableWindow( enableAddCopyDelete );

   GetDlgItem( IDC_LABELBASIC )->EnableWindow( enableActorGroups );
   GetDlgItem( IDC_LABELNAME  )->EnableWindow( enableActorGroups );
   GetDlgItem( IDC_NAME       )->EnableWindow( enableActorGroups );

   GetDlgItem( IDC_LABELQUERY )->EnableWindow( enableQuery );
   GetDlgItem( IDC_QUERY      )->EnableWindow( enableQuery );
   GetDlgItem( IDC_DEFINEQUERY)->EnableWindow( enableQuery );

   GetDlgItem( IDC_LABELID    )->EnableWindow( enableActorGroups );
   GetDlgItem( IDC_ID         )->EnableWindow( enableActorGroups );

   GetDlgItem( IDC_LABELFREQ  )->EnableWindow( enableActorGroups );
   GetDlgItem( IDC_FREQ       )->EnableWindow( enableActorGroups );

   //GetDlgItem( IDC_LABELCOLOR )->EnableWindow( enableActorGroups );
   //GetDlgItem( IDC_COLOR      )->EnableWindow( enableActorGroups );
   //GetDlgItem( IDC_SETCOLOR   )->EnableWindow( enableActorGroups );

   GetDlgItem( IDC_LABELOBJECTIVES )->EnableWindow( enableActorGroups );
   GetDlgItem( IDC_OBJECTIVES      )->EnableWindow( enableActorGroups );
   GetDlgItem( IDC_WTSLIDER        )->EnableWindow( enableActorGroups );
   GetDlgItem( IDC_LABELSLIDER     )->EnableWindow( enableActorGroups );
   }

void ActorDlg::OnAim()
   {
   UpdateData( 1 );

   switch( m_actorInitMethod )
      {
      case AIM_NONE:
         break;

      case AIM_IDU_GROUPS:
         break;

      case AIM_QUERY:
         break;

      case AIM_IDU_WTS:
      case AIM_UNIFORM:
         if ( m_actorGroupArray.GetSize() == 0 )
            OnAdd();
         break;

      default:
         ASSERT( 0 );
      }

   MakeDirty();
   EnableControls();
   }


void ActorDlg::OnEditupdateActors() 
   {
   MakeDirty();

   CString name;
   GetDlgItemText( IDC_ACTORS, name );
   SetDlgItemText( IDC_NAME, name );
   }


void ActorDlg::OnSelchangeActors() 
   {
   if ( m_isDirty )
      StoreChanges();

   int index = m_actorGroups.GetCurSel();
   if ( index < 0 )
      return;
   
   // reset current actor
   m_pActorGroup = m_actorGroupArray[ index ];
	LoadActorGroup();
   }

void ActorDlg::OnPrev()
   {
   if ( m_actorGroupArray.GetSize() <= 1 )
      return;

   int index = m_actorGroups.GetCurSel();

   if ( m_isDirty )
      StoreChanges();

   if ( index == 0 )
      index = (int) m_actorGroupArray.GetSize()-1;
   else
      index--;

   m_actorGroups.SetCurSel( index );
   m_pActorGroup = m_actorGroupArray[ index ];
   
   LoadActorGroup();
   }

void ActorDlg::OnNext()
   {
   if ( m_actorGroupArray.GetSize() <= 1 )
      return;
   
   int index = m_actorGroups.GetCurSel();

   if ( m_isDirty )
      StoreChanges();

   if ( index == m_actorGroupArray.GetSize()-1 )
      index = 0;
   else
      index++;

   m_actorGroups.SetCurSel( index );
   m_pActorGroup = m_actorGroupArray[ index ];

   LoadActorGroup();
   }

void ActorDlg::OnAdd()
   {
   // create and initialize a new actor group
   ActorGroup *pActorGroup = new ActorGroup;

   pActorGroup->m_name += "new_actor";

   //pGroup->m_id = 0;                 // id of this actorGroup in database
   //pGroup->m_decisionFrequency = 2;  // how often this actor makes a decision on a particular parcel (years)
   pActorGroup->m_valueStdDev = 0;
   int valueCount =  gpModel->GetActorValueCount(); 
   pActorGroup->SetValueCount( valueCount );

   for ( int i=0; i < valueCount; i++ )
      pActorGroup->SetValue( i, 0 );

   // add it to the group
   m_actorGroupArray.Add( pActorGroup );

   m_actorGroups.ResetContent();   
   CString name;
   for ( int i=0; i < m_actorGroupArray.GetSize(); i++ )
      m_actorGroups.AddString( m_actorGroupArray[ i ]->m_name );

   if ( m_isDirty )
      StoreChanges();

   m_actorGroups.SetCurSel( int( m_actorGroupArray.GetSize()) - 1 );
   m_pActorGroup = pActorGroup;

   LoadActorGroup();
   }


void ActorDlg::OnDelete()
   {
   if ( MessageBox( "Do you want to delete the current selection?", "Remove Selection", MB_YESNO ) == IDYES )
      {
      // delete both copies of this actor
      delete m_pActorGroup;

      int index = m_actorGroups.GetCurSel();
      m_actorGroupArray.RemoveAt( index );

      m_isSetDirty = true;

      if ( m_actorGroupArray.GetSize() > 0 )
         {
         m_actorGroups.SetCurSel( 0 );
         m_pActorGroup = m_actorGroupArray[ 0 ];
         }
      else
         {
         m_pActorGroup = NULL;
         m_actorGroups.SetCurSel( -1 );
         }

      LoadActorGroup();
      }
   }


void ActorDlg::OnCopy()
   {
   ActorGroup *pActorGroup = new ActorGroup( *m_pActorGroup );   // make a copy of the current actor

   pActorGroup->m_name += "_copy";

   m_actorGroupArray.Add( pActorGroup );

   m_actorGroups.ResetContent();   
   CString name;
   for ( int i=0; i < m_actorGroupArray.GetSize(); i++ )
      m_actorGroups.AddString( m_actorGroupArray[ i ]->m_name );

   if ( m_isDirty )
      StoreChanges();

   m_actorGroups.SetCurSel( int( m_actorGroupArray.GetSize()) - 1 );
   m_pActorGroup = pActorGroup;

   m_isSetDirty = true;

   LoadActorGroup();
   }


void ActorDlg::OnEnChangeName()
   {
   UpdateData( 1 );
   m_actorGroups.SetWindowText( m_groupName );

   MakeDirty();
   }

void ActorDlg::OnEnChangeQuery()
   {
   MakeDirty();
   }

void ActorDlg::OnBnClickedDefinequery()
   {
   QueryBuilder dlg( gpModel->m_pQueryEngine, NULL, NULL, -1, this );

   if ( dlg.DoModal() == IDCANCEL )
      {
      m_query = dlg.m_queryString;
      UpdateData( 0 );

      MakeDirty();
      }
   }


void ActorDlg::OnEnChangeId()
   {
   MakeDirty();
   }

void ActorDlg::OnEnChangeFreq()
   {
   MakeDirty();
   }


void ActorDlg::OnSelchangeObjectives() 
   {
   m_objective = m_objectives.GetCurSel();
   SetObjectiveWeight();	
   }


void ActorDlg::SetObjectiveWeight()
   {   
   if ( m_objective >= 0 )
      {
      float wt = m_pActorGroup->GetValue( m_objective );
      m_wtSlider.SetPos( int( wt*10 ) );
      }
   }


void ActorDlg::SetApplyButton()
   {
   GetDlgItem( IDC_APPLY )->EnableWindow( m_isDirty );
   }

void ActorDlg::MakeDirty()
   {
   m_isSetDirty = true;
   m_isDirty = true;
   SetApplyButton();
   }

void ActorDlg::OnApply()
   {
   StoreChanges();
   SaveActorsToDoc();
   }

void ActorDlg::SaveActorsToDoc()
   {
   if ( m_isDirty )
      StoreChanges();

   // remove all actors and groups in the doc
   gpModel->m_pActorManager->RemoveAll();


   // copy actors from the dlg to the doc
   for ( int i=0; i < m_actorGroupArray.GetCount(); i++ )
      {
      ActorGroup *pActorGroup = m_actorGroupArray.GetAt(i);
      gpModel->m_pActorManager->AddActorGroup( new ActorGroup( *pActorGroup ) );
      }

   gpModel->m_pActorManager->m_actorInitMethod = (ACTOR_INIT_METHOD) m_actorInitMethod;
   gpModel->m_pActorManager->CreateActors();

   m_isDirty = false;
   m_isSetDirty = false;
   SetApplyButton();
   }

void ActorDlg::StoreChanges()
   {
   // store info for current actorGroup
   UpdateData( 1 );

   if ( m_pActorGroup == NULL )
      return;

   m_pActorGroup->m_name = m_groupName;
   m_pActorGroup->m_query = m_query;
   m_pActorGroup->m_id = m_id;
   m_pActorGroup->m_decisionFrequency = m_freq;

   // color, objective weights taken care of already
   m_isDirty = false;
   SetApplyButton();
   }


void ActorDlg::OnNMReleasedcaptureWtslider(NMHDR *pNMHDR, LRESULT *pResult)
   {
   float newWt = (float)m_wtSlider.GetPos()/10.0f;

   int valueIndex = m_objectives.GetCurSel();

   if ( valueIndex >= 0 )
      {
      m_pActorGroup->SetValue( valueIndex, newWt );
      MakeDirty();
      }
   
   *pResult = 0;
   }


void ActorDlg::OnOK()
   {
   if ( m_isSetDirty == TRUE )
      {         
      int answer = MessageBox( "Do you want to save the changes to the actors?", "Save Actor Changes", MB_YESNOCANCEL | MB_ICONWARNING );
      if ( answer == IDCANCEL )
         return;
      else if ( answer == IDYES )
         SaveActorsToDoc();
      }

   CDialog::OnOK();
   }

void ActorDlg::OnBnClickedExport()
   {
   DirPlaceholder d;

   CFileDialog dlg( FALSE, "xml", gpModel->m_pActorManager->m_path, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
         "XML Files|*.xml|All files|*.*||");

   int retVal = (int) dlg.DoModal();
   
   if ( retVal == IDOK )
      {
      SaveActorsToDoc();
      gpModel->m_pActorManager->SaveXml( dlg.GetPathName() );
      }
   }
