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
// PPIniActors.cpp : implementation file
//

#include "stdafx.h"
#include "PPIniActors.h"
#include "EnvDoc.h"
#include <Actor.h>
#include "IniFileEditor.h"
#include "ActorDlg.h"
#include <colors.hpp>

extern CEnvDoc      *gpDoc;
extern ActorManager *gpActorManager;
extern EnvModel     *gpModel;

// PPIniActors dialog

IMPLEMENT_DYNAMIC(PPIniActors, CTabPageSSL)

PPIniActors::PPIniActors(IniFileEditor* pParent /*=NULL*/)
	: CTabPageSSL(PPIniActors::IDD, pParent)
   , m_pParent( pParent )
   , m_decisionFreq(0)
   , m_assocAttr(FALSE)
   , m_assocValues(FALSE)
   , m_assocNeighbors(FALSE)
   , m_useAltruism( FALSE )
   , m_useActorValues( FALSE )
   , m_usePolicyPreferences( FALSE )
   , m_useUtility( FALSE )
   , m_method(FALSE)
{ }

PPIniActors::~PPIniActors()
{ }


void PPIniActors::DoDataExchange(CDataExchange* pDX)
{
CTabPageSSL::DoDataExchange(pDX);
DDX_Text(pDX, IDC_DECISIONFREQ, m_decisionFreq);
DDV_MinMaxInt(pDX, m_decisionFreq, 0, 50);
DDX_Check(pDX, IDC_ASSOCATTR, m_assocAttr);
DDX_Check(pDX, IDC_ASSOCVALUES, m_assocValues);
DDX_Check(pDX, IDC_ASSOCNEIGHBORS, m_assocNeighbors);
DDX_Check(pDX, IDC_USEALTRUISM, m_useAltruism);
DDX_Check(pDX, IDC_USEACTORVALUES, m_useActorValues);
DDX_Check(pDX, IDC_USEPOLICYPREFS, m_usePolicyPreferences);
DDX_Check(pDX, IDC_USEUTILITY, m_useUtility);
DDX_Radio(pDX, IDC_ENVDB, m_method);
DDX_Control(pDX, IDC_PLACEHOLDER, m_placeHolder);
}


BEGIN_MESSAGE_MAP(PPIniActors, CTabPageSSL)
   ON_BN_CLICKED(IDC_EDITACTORDB2, &PPIniActors::OnBnClickedEditactordb2)
   ON_BN_CLICKED(IDC_EDITACTORDB, &PPIniActors::OnBnClickedEditactordb)
   ON_BN_CLICKED(IDC_ENVDB, &PPIniActors::MakeDirty)
   ON_BN_CLICKED(IDC_IDU, &PPIniActors::MakeDirty)
   ON_BN_CLICKED(IDC_SINGLE, &PPIniActors::MakeDirty)
   ON_BN_CLICKED(IDC_NONE, &PPIniActors::MakeDirty)
   ON_BN_CLICKED(IDC_ENVDB, &PPIniActors::MakeDirty)
   ON_BN_CLICKED(IDC_ASSOCATTR, &PPIniActors::MakeDirty)
   ON_BN_CLICKED(IDC_ASSOCNEIGHBORS, &PPIniActors::MakeDirty)
   ON_BN_CLICKED(IDC_ASSOCVALUES, &PPIniActors::MakeDirty)
   ON_BN_CLICKED(IDC_USEALTRUISM, &PPIniActors::MakeDirty)
   ON_BN_CLICKED(IDC_USEPOLICYPREFS, &PPIniActors::MakeDirty)
   ON_BN_CLICKED(IDC_USEACTORVALUES, &PPIniActors::MakeDirty)
   ON_BN_CLICKED(IDC_USEUTILITY, &PPIniActors::MakeDirty)
END_MESSAGE_MAP()


// PPIniActors message handlers

void PPIniActors::MakeDirty()
   {
   m_pParent->MakeDirty( INI_ACTORS );
   }

void PPIniActors::OnBnClickedEditactordb2()
   {
   MakeDirty();
   }

void PPIniActors::OnBnClickedEditactordb()
   {
   ActorDlg dlg;
   dlg.DoModal();
   MakeDirty();
   }

BOOL PPIniActors::OnInitDialog()
   {
   m_assocNone      = (( gpActorManager->m_associationTypes == AAT_NONE )) ? 1 : 0;
   m_assocAttr      = (( gpActorManager->m_associationTypes & AAT_COMMONATTRIBUTE ) != 0 ) ? 1 : 0;
   m_assocValues    = (( gpActorManager->m_associationTypes & AAT_VALUECLUSTER )    != 0 ) ? 1 : 0;
   m_assocNeighbors = (( gpActorManager->m_associationTypes & AAT_NEIGHBORS )       != 0 ) ? 1 : 0;
/*
   switch ( gpActorManager->m_actorInitMethod )
      {
      case AIM_LULC:             m_method = 0;  break;  
      case AIM_IDU:              m_method = 1;  break;  
      case AIM_UNIFORM:          m_method = 2;  break;  
      case AIM_RANDOM:
      case AIM_NONE:
      default:
         m_method = 3;
      }
*/
   // actor decision-making
   m_useAltruism          = ( gpModel->m_decisionElements & DE_ALTRUISM ) ? 1 : 0;
   m_useActorValues       = ( gpModel->m_decisionElements & DE_SELFINTEREST ) ? 1 : 0;
   m_usePolicyPreferences = ( gpModel->m_decisionElements & DE_GLOBALPOLICYPREF ) ? 1 : 0;
   m_useUtility           = ( gpModel->m_decisionElements & DE_UTILITY ) ? 1 : 0;
   
   if ( gpModel->m_decisionType == DT_PROBABILISTIC )
      {
      CheckDlgButton( IDC_USEPROBABILISTIC, 1 );
      CheckDlgButton( IDC_USEMAX, 0 );
      }
   else
      {
      CheckDlgButton( IDC_USEPROBABILISTIC, 0 );
      CheckDlgButton( IDC_USEMAX, 1 );
      }

   CWnd *pTempWnd = GetDlgItem( IDC_PLACEHOLDER );
   ASSERT( pTempWnd != NULL );
   
   RECT rect;
   pTempWnd->GetWindowRect( &rect );
   ScreenToClient( &rect );
   pTempWnd->ShowWindow( SW_HIDE );

   BOOL ok = m_grid.Create( rect, this, 100, WS_CHILD | WS_TABSTOP | WS_VISIBLE );
   m_grid.m_pParent = this;
   ASSERT( ok );

   LoadActorValueGrid();

   UpdateData( 0 );

   CTabPageSSL::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }

void PPIniActors::LoadActorValueGrid()
   {
   m_grid.DeleteAllItems();
   m_grid.SetColumnCount( 2 );
   m_grid.SetFixedRowCount( 1 );

   m_grid.SetItemText( 0, 0, "Goal" );
   m_grid.SetItemText( 0, 1, "Actor Value (-3 to 3)" );
   m_grid.SetColumnWidth( 0, 50 );
   m_grid.SetColumnWidth( 1, 80 );

   int goals = gpModel->GetActorValueCount();

   m_grid.SetRowCount( goals + 1 ); // the " + 1" is for the fixed row
   m_grid.ExpandColumnsToFit();
   m_grid.SetEditable();
   m_grid.EnableSelection( FALSE );

   for ( int i=0; i < goals; i++ )
      {
      METAGOAL *pInfo = gpModel->GetActorValueMetagoalInfo( i );
      // set up first column
      m_grid.SetItemBkColour( i+1, 0, LTGRAY );
      m_grid.SetItemText    ( i+1, 0, pInfo->name );
      m_grid.SetItemState   ( i+1, 0, m_grid.GetItemState(i+1, 0) | GVIS_READONLY );

      // set up second column
      //CString value;
      //value.Format( _T("%f"), pInfo->metagoal );
      //m_grid.SetItemText( i+1, 1, value );  ?????
      
      }
   }


bool PPIniActors::StoreChanges( void )
   {
   UpdateData( 1 );

   int decisionElement =0;
   if ( m_useAltruism )
      decisionElement += DE_ALTRUISM;
   
   if ( m_useActorValues )
      decisionElement += DE_SELFINTEREST;
   
   if ( m_usePolicyPreferences )
      decisionElement += DE_GLOBALPOLICYPREF;

   if ( m_useUtility )
      decisionElement += DE_UTILITY;

   gpModel->m_decisionElements += decisionElement;
   
   gpModel->m_decisionType = IsDlgButtonChecked( IDC_USEPROBABILISTIC ) ? DT_PROBABILISTIC : DT_MAX;

   gpActorManager->m_associationTypes = 0;
   if ( m_assocAttr )
      gpActorManager->m_associationTypes |= AAT_COMMONATTRIBUTE;

   if ( m_assocValues )
      gpActorManager->m_associationTypes |= AAT_VALUECLUSTER;
   
   if ( m_assocNeighbors )
      gpActorManager->m_associationTypes |= AAT_NEIGHBORS;

   if ( m_assocNone )
      gpActorManager->m_associationTypes = AAT_NONE;

/*   switch ( m_method )
      {
      case 0:  gpActorManager->m_actorInitMethod = AIM_LULC;            break;  
      case 1:  gpActorManager->m_actorInitMethod = AIM_IDU;             break;  
      case 2:  gpActorManager->m_actorInitMethod = AIM_UNIFORM;         break;
      case 3:  gpActorManager->m_actorInitMethod = AIM_NONE;            break;
      default:
         ASSERT( 0 );         
      }
*/
   gpActorManager->RemoveAll();

   return true;
   }

