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
// PolEditor.cpp : implementation file
//

#include "stdafx.h"
#include "PolEditor.h"
#include <Scenario.h>
#include "Envision.h"
#include "EnvDoc.h"
#include "SiteAttrDlg.h"
#include "NewPolicyDlg.h"
#include "FieldInfoDlg.h"
#include "SaveToDlg.h"
#include "MapPanel.h"
#include <map.h>
#include <maplayer.h>
#include <queryengine.h>
#include <dirplaceholder.h>
#include <path.h>

extern CEnvDoc       *gpDoc;
extern MapLayer      *gpCellLayer;
//extern QueryEngine   *gpQueryEngine;
extern PolicyManager *gpPolicyManager;
extern ScenarioManager *gpScenarioManager;
extern MapPanel        *gpMapPanel;
extern EnvModel *gpModel;

// PolEditor dialog

IMPLEMENT_DYNAMIC(PolEditor, CDialog)

PolEditor::PolEditor(CWnd* pParent /*=NULL*/)
	: CDialog( PolEditor::IDD, pParent)
   , m_pPolicy( NULL )
   , m_tabCtrl()
   , m_basic( this, m_pPolicy )
   , m_siteAttr( this, m_pPolicy )
   , m_constraints( this, m_pPolicy )
   , m_outcomes( this, m_pPolicy )
   , m_scores( this, m_pPolicy )
   , m_scenarios( this, m_pPolicy )
   , m_firstLoad( true )
   , m_currentIndex( 0 )
   , m_showingMore( true )
   , m_isEditable( false )
   , m_isDirty( false )
   , m_hasBeenDirty( false )
   , m_fullHeight( 0 )
   , m_shrunkHeight( 0 )
{ }


PolEditor::~PolEditor()
   {
   m_policyArray.RemoveAll();
   }

void PolEditor::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_TAB, m_tabCtrl);
DDX_Control(pDX, IDC_POLICIES, m_policies);
}


BEGIN_MESSAGE_MAP(PolEditor, CDialog)
 	ON_CBN_EDITUPDATE(IDC_POLICIES, OnMakeDirty)
	ON_CBN_SELCHANGE(IDC_POLICIES, OnSelchangePolicies)
   ON_BN_CLICKED(IDC_DELETE, OnDelete)
	ON_BN_CLICKED(IDC_COPY, OnCopy)
	ON_BN_CLICKED(IDC_NEXT, OnNext)
	ON_BN_CLICKED(IDC_PREV, OnPrev)
   ON_BN_CLICKED(IDC_ADDNEW, OnAddnew)
   ON_BN_CLICKED(IDC_SHOWHIDE, OnBnClickedShowhide)
   ON_BN_CLICKED(IDC_EDITFIELDINFO, &PolEditor::OnBnClickedEditfieldinfo)
   ON_BN_CLICKED(IDC_SAVE, &PolEditor::OnBnClickedSave)
   ON_CBN_EDITCHANGE(IDC_POLICIES, &PolEditor::OnCbnEditchangePolicies)
   ON_BN_CLICKED(IDC_IMPORT, &PolEditor::OnBnClickedImport)
   ON_WM_TIMER()
END_MESSAGE_MAP()


// PolEditor message handlers

BOOL PolEditor::OnInitDialog()
   {
   CDialog::OnInitDialog();

   CenterWindow();

   RECT rect;
   GetWindowRect( &rect );
   m_fullHeight = rect.bottom - rect.top;

   RECT rectButton;
   GetDlgItem( IDC_NEXT )->GetWindowRect( &rectButton );

   m_shrunkHeight = rectButton.bottom - rect.top + 8;
   
   m_firstLoad = true;

   // Setup the tab control
   int nPageID = 0;
   m_basic.Create( PPPolicyBasic::IDD, this );
   m_tabCtrl.AddSSLPage(_T("Basic Properties"), nPageID++, &m_basic );

   m_siteAttr.Create( PPPolicySiteAttr::IDD, this );
   m_tabCtrl.AddSSLPage(_T("Site Constraints"), nPageID++, &m_siteAttr );

   m_constraints.Create( PPPolicyConstraints::IDD, this );
   m_tabCtrl.AddSSLPage(_T("Resource Constraints"), nPageID++, &m_constraints );

   m_outcomes.Create( PPPolicyOutcomes::IDD, this );
   m_tabCtrl.AddSSLPage(_T("Outcomes"), nPageID++, &m_outcomes );

   m_scores.Create( PPPolicyObjScores::IDD, this );
   m_tabCtrl.AddSSLPage(_T("Effectiveness Scores"), nPageID++, &m_scores );

   m_scenarios.Create( PPPolicyScenarios::IDD, this );
   m_tabCtrl.AddSSLPage(_T("Scenarios"), nPageID++, &m_scenarios );
   
   CString title( "Policy Editor " );
   //title += m_pPolicy->m_name;

   SetWindowText( title ); 
   return TRUE;
   }


int PolEditor::ReloadPolicies( bool promptForNewPolicy )
   {
   m_policies.ResetContent();

   MakeLocalCopyOfPolicies(); 
   
   // load Policys in Policy dialog
   int count = (int) m_policyArray.GetSize();

   if ( m_firstLoad == true )
      {
//      m_pPolicy = NULL;
      m_firstLoad = false;
//      return 0;
      }

   if ( count == 0 ) // no policies yet?
      {
      if ( promptForNewPolicy )
         {
         if ( AddNew() == false )
            {
            m_pPolicy = NULL;
            return 0;
            }
         }
      else
         {
         m_pPolicy = NULL;
         return 0;
         }
      }
   else
      {
      ASSERT( count > 0 );
      m_pPolicy = m_policyArray[ 0 ];

      // load policies combo
      for ( int i=0; i < count; i++ )
         {
         Policy *pPolicy = m_policyArray[ i ];
         m_policies.AddString( m_policyArray[ i ]->m_name );
         }

      m_policies.SetCurSel( 0 );
      
      m_isDirty = false;
      m_hasBeenDirty = false;
      //SetModified( 1 );   // why???
      }

   // make sure policy compiled
   if ( m_pPolicy->m_pSiteAttrQuery == NULL )
      {
      gpPolicyManager->CompilePolicy( m_pPolicy, gpCellLayer );

      // bad compilation?
      if ( m_pPolicy->m_pSiteAttrQuery == NULL )
         {
         //CString msg( "Error compiling site attribute query: " );
         //msg += m_pPolicy->m_siteAttrStr;
         //Report::ErrorMsg( msg );
         }
      }

   LoadPolicy();     // this loads all children
   m_firstLoad = false;

   return (int) m_policyArray.GetSize();
   }



bool PolEditor::IsEditable()
   {
   return ( m_pPolicy->m_originator.CompareNoCase( gpDoc->m_userName ) != 0 && m_pPolicy->m_isEditable == false ) ? false : true;
   }


void PolEditor::OnMakeDirty()
   {
   MakeDirty( DIRTY_PARENT );
   }


void PolEditor::LoadPolicy( void )
   {
   if ( m_pPolicy == NULL )
      {
      MessageBox( _T("Unable to load policy..." ) );
      return;
      }
   m_basic.LoadPolicy();
   m_siteAttr.LoadPolicy();
   m_constraints.LoadPolicy();
   m_outcomes.LoadPolicy();
   m_scores.LoadPolicy();
   m_scenarios.LoadPolicy();
   }


void PolEditor::OnSelchangePolicies() 
   {
   bool goAhead = true;
   if ( m_isDirty )
      goAhead = StoreChanges();

   if ( goAhead )
      {
      m_currentIndex = m_policies.GetCurSel();
      if ( m_currentIndex < 0 )
         return;

      CString policyName;
      m_policies.GetLBText( m_currentIndex, policyName );

      m_pPolicy = FindPolicy( policyName );
      ASSERT( m_pPolicy != NULL );
      
      // reset current Policy
      //m_pPolicy = m_policyArray[ m_currentIndex ];
	   LoadPolicy();
      CString title( "Policy Editor: " );
      title += m_pPolicy->m_name;
      SetWindowText( title ); 
      }
   }

Policy *PolEditor::FindPolicy( LPCTSTR name )
   {
   int len = lstrlen( name );

   for ( INT_PTR i=0; i < m_policyArray.GetSize(); i++ )
      {
      Policy *pPolicy = m_policyArray[ i ];

      if ( pPolicy->m_name.Compare( name ) == 0 && pPolicy->m_name.GetLength() == len )
         return pPolicy;
      }

   return NULL;
   }


void PolEditor::MakeDirty( int flag ) 
   {
   m_isDirty |= flag;
   m_hasBeenDirty = true;
   }


void PolEditor::MakeLocalCopyOfPolicies()
   {   
   m_policyArray.RemoveAll();
   m_policyArray = gpPolicyManager->GetPolicyArray();  // make a copy
   m_constraintArray.RemoveAll();

   for ( int i=0; i < gpPolicyManager->GetGlobalConstraintCount(); i++ )
      {
      GlobalConstraint *pConstraint = gpPolicyManager->GetGlobalConstraint( i );
      m_constraintArray.Add( new GlobalConstraint( *pConstraint ) );
      }
   
   if ( GetSafeHwnd() && IsWindowVisible() )
      {
      if ( m_policyArray.GetSize() == 0 )
         m_pPolicy = NULL;
      else
         {
         // reload current Policy
         ASSERT( 0 <= m_currentIndex && m_currentIndex < m_policyArray.GetCount() );

         CString policyName;
         m_policies.GetLBText( m_currentIndex, policyName );

         m_pPolicy = FindPolicy( policyName );

         if ( m_pPolicy != NULL )
            LoadPolicy();
         }
      }
   }


// Store all changes to the current m_pPolicy pointer
bool PolEditor::StoreChanges()
   {
   UpdateData( TRUE );  // update local variables

   gpPolicyManager->m_pQueryEngine->ShowCompilerErrors( false );

   // Validate entered data
   CString problems;
   CString name;
   m_policies.GetWindowText( name );
   if ( name.GetLength() >= 256 )
      {
      Report::ErrorMsg( "The Policy name must be less than 256 characters in length" ); 
      return false;
      }

   // only member variable for the main page
   m_pPolicy->m_name = name;

   bool cont;
   
   if ( IsDirty( DIRTY_BASIC ) )
      {
      cont = m_basic.StoreChanges();
      if ( ! cont )
         return false;
      }

   if ( IsDirty( DIRTY_SITEATTR ) )
      {
      cont = m_siteAttr.StoreChanges();
      if ( ! cont )
         return false;
      }

/*
if ( IsDirty( DIRTY_GLOBAL_CONSTRAINTS) )
      {
      cont = m_constraints.StoreGlobalChanges();
      if ( ! cont )
         return false;
      }
*/
   if ( IsDirty( DIRTY_POLICY_CONSTRAINTS) )
      {
      cont = m_constraints.StorePolicyChanges();
      if ( ! cont )
         return false;
      }
   
   if ( IsDirty( DIRTY_OUTCOMES ) )
      {
      cont = m_outcomes.StoreChanges();
      if ( ! cont )
         return false;
      }

   if ( IsDirty( DIRTY_SCORES ) )
      {
      cont = m_scores.StoreChanges();
      if ( ! cont )
         return false;
      }

   if ( IsDirty( DIRTY_SCENARIOS ) )
      {
      cont = m_scenarios.StoreChanges();
      if ( ! cont )
         return false;
      }
   m_isDirty = 0;
   return true;
   }


void PolEditor::OnPrev()
   {
   m_currentIndex = m_policies.GetCurSel();

   bool goAhead = true;
   if ( m_isDirty )
      goAhead = StoreChanges();

   if ( goAhead )
      {
      if ( m_currentIndex == 0 )
         m_currentIndex = int( m_policyArray.GetSize()) - 1 ;
      else
         m_currentIndex--;

      m_policies.SetCurSel( m_currentIndex );
      
      CString policyName;
      m_policies.GetLBText( m_currentIndex, policyName );

      m_pPolicy = FindPolicy( policyName );
      ASSERT( m_pPolicy != NULL );
      //m_pPolicy = m_policyArray[ m_currentIndex ];
      
      LoadPolicy();
      }
   }


void PolEditor::OnDelete()
   {
   if ( MessageBox( "Do you want to delete the current selection?", "Remove Selection", MB_YESNO ) == IDYES )
      {
      // delete both copies of this policy
      int index;
      Policy *pPolicy = gpPolicyManager->FindPolicy( m_pPolicy->m_name, &index );
      if ( pPolicy != NULL )
         gpPolicyManager->DeletePolicy( index );  // this also adds the policy to the deleted policy pool

      gpScenarioManager->RemovePolicyFromScenarios( pPolicy );

      // remove it from the list box
      //gpPolicyManager->FindPolicy( m_pPolicy->m_name, index );  // index = m_policies.FindString( -1, m_pPolicy->m_name ); << FindString bug...
      m_policies.DeleteString( m_currentIndex );

      // delete the temporary policy currently being pointed at
      delete m_pPolicy;

      // find index where this policy was in array copy
      bool found = false;
      for ( int i=0; i < m_policyArray.GetSize(); i++)
         {
         if ( m_policyArray[ i ] == m_pPolicy )
            {
            m_policyArray.RemoveAt( i );
            found = true;
            }
         }

      ASSERT( found == true );

      m_currentIndex = 0;
      m_policies.SetCurSel( 0 );
          
      CString policyName;
      m_policies.GetLBText( m_currentIndex, policyName );

      m_pPolicy = FindPolicy( policyName );
      ASSERT( m_pPolicy != NULL );
      //m_pPolicy = m_policyArray[ 0 ];

      LoadPolicy();

      m_hasBeenDirty = true;
      }
   }


void PolEditor::OnCopy()
   {
   bool goAhead = true;

   if ( m_isDirty )
      goAhead = StoreChanges();

   if ( goAhead )
      {
      Policy *pPolicy = NULL;

      pPolicy = new Policy( *m_pPolicy );   // make a copy of the current Policy

      pPolicy->m_name += "_copy";

      // Duplicate Name not allowed; check case sensitive as does PolicyManager::FindPolicy()
      int dupCount = 0;
      for ( int i=0; i < this->m_policyArray.GetCount(); i++ )
         {
         if ( strncmp( m_policyArray[ i ]->m_name, pPolicy->m_name, pPolicy->m_name.GetLength() ) == 0 )
            {
            dupCount++;
            }
         }

      if ( dupCount > 0 )
         {
         pPolicy->m_name.AppendFormat( "%02d", dupCount+1 ); 
         }

      pPolicy->m_originator = gpDoc->m_userName;
      pPolicy->m_isAdded = true;
      pPolicy->m_id = gpPolicyManager->GetNextPolicyID();

      m_policyArray.Add( pPolicy );

      // need to add to all the scenarios POLICY_INFO's as well...
      gpScenarioManager->AddPolicyToScenarios( pPolicy, false );

      // reload dialog box
      m_policies.ResetContent();   

      m_currentIndex = -1;

      for ( int i=0; i < m_policyArray.GetSize(); i++ )
         m_policies.AddString( m_policyArray[ i ]->m_name );

      m_currentIndex = m_policies.FindStringExact( 0, pPolicy->m_name );
      ASSERT( m_currentIndex >= 0 );
      m_policies.SetCurSel( m_currentIndex );
              
      //CString policyName;
      //m_policies.GetLBText( m_currentIndex, policyName );
      //m_pPolicy = FindPolicy( policyName );
      m_pPolicy = pPolicy;
      ASSERT( m_pPolicy != NULL );
      //m_pPolicy = pPolicy;

      LoadPolicy();

      m_hasBeenDirty = true;      
      }
   }


void PolEditor::OnNext()
   {
   m_currentIndex = m_policies.GetCurSel();

   bool goAhead = true;
   if ( m_isDirty )
      goAhead = StoreChanges();

   if ( goAhead )
      {
      if ( m_currentIndex == m_policyArray.GetSize()-1 )
         m_currentIndex = 0;
      else
         m_currentIndex++;

      m_policies.SetCurSel( m_currentIndex );
              
      CString policyName;
      m_policies.GetLBText( m_currentIndex, policyName );

      m_pPolicy = FindPolicy( policyName );
      ASSERT( m_pPolicy != NULL );
      //m_pPolicy = m_policyArray[ m_currentIndex ];

      LoadPolicy();
      }
   }

/*
void PolEditor::OnBnClickedEditsiteconstraints()
   {
   SiteAttrDlg dlg( m_pPolicy );
   if ( dlg.DoModal() == IDOK )
      {
      LoadPolicy();
      SetModified( 1 );
      }
   }
*/

void PolEditor::OnAddnew()
   {
   AddNew();
   }

bool PolEditor::AddNew()
   {
   NewPolicyDlg dlg;
   Policy *pPolicy = NULL;

   if ( dlg.DoModal() == IDOK )
      {
      // Duplicate Name not allowed; check case sensitive as does PolicyManager::FindPolicy()
      for ( int i=0; i < this->m_policyArray.GetCount(); i++ )
         {
         if ( lstrcmp(dlg.m_description , m_policyArray[ i ]->m_name ) == 0 )
            {
            CString msg;
            msg.Format("Add New Policy failed because policy of name, %s, already exists.", dlg.m_description);
            Report::WarningMsg(msg);
            return false;
            }
         }

      pPolicy = new Policy( gpPolicyManager );
         
      int metagoalCount = gpModel->GetMetagoalCount();
      for ( int j=0; j < metagoalCount; j++ )  
         {
         METAGOAL *pInfo = gpModel->GetMetagoalInfo( j );
         //EnvEvaluator *pInfo = gpModel->GetMetagoalModelInfo( j );
         pPolicy->AddObjective( pInfo->name, j ); //pInfo->id, j );
         }

      pPolicy->m_id = gpPolicyManager->GetNextPolicyID();
      
      pPolicy->m_name = dlg.m_description;
      pPolicy->m_originator = gpDoc->m_userName;
      pPolicy->m_isAdded    = true;

      // add to the local policy array set
      m_policyArray.Add( pPolicy );

      gpScenarioManager->AddPolicyToScenarios( pPolicy, false );

      m_currentIndex = m_policies.AddString( dlg.m_description );
      m_policies.SetCurSel( m_currentIndex );
      m_pPolicy = pPolicy; //m_policyArray[ m_currentIndex ];
      m_pPolicy->SetSiteAttrQuery( "" );

      LoadPolicy();

      m_hasBeenDirty = true;
      return true;
      }

   return false;
   }


void PolEditor::ShowSites()
   {
   CString query;
   this->m_siteAttr.GetDlgItem( IDC_QUERY )->GetWindowText( query );

   if ( query.IsEmpty() )
      {
      MessageBox( "No query specified", "", MB_OK );
      return;
      }

   Query *pQuery = gpPolicyManager->m_pQueryEngine->ParseQuery( query, 0, "Policy Editor" );
   if ( pQuery == NULL )
      {
      MessageBox( "Unable to parse query - please fix the query before proceeding", "Error Parsing Query", MB_OK );
      return;
      }

   int count = gpPolicyManager->m_pQueryEngine->SelectQuery( pQuery, true ); //, true );
   gpMapPanel->m_mapFrame.RedrawWindow();
   gpPolicyManager->m_pQueryEngine->RemoveQuery( pQuery );

   CString msg;
   msg.Format( "Applies to %i sites", count );
   m_siteAttr.SetDlgItemText( IDC_SITES, msg );
   //m_basic.SetDlgItemText( IDC_SITES, msg );
   
   if ( m_showingMore == false )
      return;
   else
      ShowLess();
   
   //UINT_PTR timerVal = SetTimer(IDT_TIMER, 10000 /*millisecs*/, NULL);
   }


void PolEditor::OnTimer(UINT_PTR timerVal)
   {
   KillTimer( timerVal );
   ShowMore();

   gpCellLayer->ClearSelection();
   gpMapPanel->m_mapFrame.m_pMapWnd->RedrawWindow();
   }


void PolEditor::OnBnClickedShowhide()
   {
   if ( m_showingMore )
      ShowLess();
   else
      ShowMore();
   }


void PolEditor::ShowMore()
   {
   RECT rect;
   GetWindowRect( &rect );   // get rect (screen coords) of containing dialog box for the height

   POINT pt;
   pt.x = rect.left;
   pt.y = rect.top;
   
   MoveWindow( pt.x, pt.y, rect.right-rect.left, m_fullHeight, TRUE );
   SetDlgItemText( IDC_SHOWHIDE, "Show Less" );
   m_showingMore = true;
   }


void PolEditor::ShowLess()
   {
   RECT rect;
   GetWindowRect( &rect );   // get rect (screen coords) of containing dialog box for the height

   POINT pt;
   pt.x = rect.left;
   pt.y = rect.top;
   
   MoveWindow( pt.x, pt.y, rect.right-rect.left, m_shrunkHeight, TRUE );
   SetDlgItemText( IDC_SHOWHIDE, "Show More" );
   m_showingMore = false;
   }


void PolEditor::OnOK()
   {
   SavePolicies();
   return CDialog::OnOK();
   }


bool PolEditor::SavePolicies()
   {
   bool goAhead = true;
   if ( m_isDirty )
      goAhead = StoreChanges();

   if ( goAhead )
      {
      gpPolicyManager->RemoveAllPolicies();

      for ( int i=0; i < (int) m_policyArray.GetSize(); i++ )
         {
         Policy *pPolicy = m_policyArray[ i ];
         gpPolicyManager->AddPolicy( new Policy( *pPolicy ) );
         }

      // add any global constraints
      bool saveGlobalConstraints = m_isDirty & DIRTY_GLOBAL_CONSTRAINTS ? true : false;

      if ( saveGlobalConstraints )
         gpPolicyManager->ReplaceGlobalConstraints( m_constraintArray );
         
      m_isDirty = 0;
      m_hasBeenDirty = false;

      gpDoc->SetChanged( CHANGED_POLICIES );
      }

   return goAhead;
   }


void PolEditor::OnCancel()
   {
   MakeLocalCopyOfPolicies();    // refresh local copy of policies

   m_isDirty = 0;
   m_hasBeenDirty = false;

   CDialog::OnCancel();
   }

void PolEditor::OnBnClickedEditfieldinfo()
   {
   FieldInfoDlg dlg;

   if ( dlg.DoModal() == IDOK )
      m_siteAttr.LoadFieldInfo();
   }

void PolEditor::OnBnClickedSave()
   { 
   DirPlaceholder d;
   CString startPath = gpPolicyManager->m_path;

   if ( gpPolicyManager->m_loadStatus == 1 ) //0=embedded in envx,1=external xml
      startPath = gpPolicyManager->m_importPath; 

   if ( startPath.IsEmpty() )
      startPath = gpDoc->m_iniFile;

   SaveToDlg dlg( "Policies", startPath );

   int retVal = (int) dlg.DoModal();

   if ( retVal == IDOK )
      {
      if ( dlg.m_saveThisSession )
         SavePolicies();    // save the policies internally to the policy manager
   
      if ( dlg.m_saveToDisk )
         {
         m_path = dlg.m_path;

         nsPath::CPath path( m_path );
         CString ext = path.GetExtension();

         if (ext.CompareNoCase( _T("envx") ) == 0 )
             gpDoc->OnSaveDocument( m_path );
         else
            gpPolicyManager->SaveXml( m_path );

         gpDoc->UnSetChanged( CHANGED_POLICIES );
         }

      m_isDirty = 0;      
      }

   return;
   }


void PolEditor::OnCbnEditchangePolicies()
   {
   CString name;
   m_policies.GetWindowText( name );
   m_pPolicy->m_name = name;

   DWORD sel = m_policies.GetEditSel();

   //A 32-bit value that contains the starting position in the low-order word
   //   and the position of the first nonselected character after the
   //   end of the selection in the high-order word.

   m_policies.InsertString( m_currentIndex, name );
   m_policies.DeleteString( m_currentIndex+1 );
   m_policies.SetCurSel( m_currentIndex );
   m_policies.SetEditSel( HIWORD( sel ), HIWORD(sel ) );
   }


void PolEditor::OnBnClickedImport()
   {
   gpDoc->OnImportPolicies();
   /*
   DirPlaceholder d;
   CString startPath = gpPolicyManager->m_path;
   CString ext = _T("xml");
   CString extensions = _T("XML Files|*.xml|ENVX (Project) Files|*.envx|All files|*.*||");


   CFileDialog dlg( TRUE, ext, startPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, extensions);

   int retVal = (int) dlg.DoModal();
   
   if ( retVal == IDOK )
      {
      m_path = dlg.GetPathName();
      gpPolicyManager->LoadXml( m_path, false, true );
      }*/
   }