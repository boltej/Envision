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
// ScenarioEditor.cpp : implementation file
//

#include "stdafx.h"
#include "ScenarioEditor.h"
#include <Scenario.h>
#include <EnvPolicy.h>
#include <EnvModel.h>
#include "EnvDoc.h"

#include "ScenarioGridDlg.h"
#include "SaveToDlg.h"

#include <Path.h>
#include <DirPlaceholder.h>


extern ScenarioManager *gpScenarioManager;
extern PolicyManager   *gpPolicyManager;
extern CEnvDoc         *gpDoc;
extern EnvModel        *gpModel;

// ScenarioEditor dialog

IMPLEMENT_DYNAMIC(ScenarioEditor, CDialog)

ScenarioEditor::ScenarioEditor(CWnd* pParent /*=NULL*/)
	: CDialog(ScenarioEditor::IDD, pParent)
   , m_name(_T(""))
   , m_originator(_T(""))
   , m_scnDescription( _T(""))
//   , m_altruism(0)
//   , m_self(0)
   , m_policyPref(0)
   , m_location(_T(""))
   , m_scale(_T(""))
   , m_evalFreq(1)
   { }


ScenarioEditor::~ScenarioEditor()
{
}

void ScenarioEditor::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_SCENARIOS, m_scenarios);
DDX_Text(pDX, IDC_NAME, m_name);
DDX_Text(pDX, IDC_ORIGINATOR, m_originator);
DDX_Text(pDX, IDC_SCNDESCRIPTION, m_scnDescription);
//DDX_Slider(pDX, IDC_SLIDERALTRUISM, m_altruism);
//DDX_Slider(pDX, IDC_SLIDERSELF, m_self);
DDX_Slider(pDX, IDC_SLIDERPOLICYPREF, m_policyPref);
DDX_Control(pDX, IDC_MODELVARS, m_inputVars);
DDX_Control(pDX, IDC_DISTRIBUTION, m_distribution);
DDX_Text(pDX, IDC_LOCATION, m_location);
DDX_Text(pDX, IDC_SCALE, m_scale);
DDX_Control(pDX, IDC_DESCRIPTION, m_description);
DDX_Control(pDX, IDC_POLICIES, m_policies);
//DDX_Control(pDX, IDC_SLIDERALTRUISM, m_altruismSlider);
//DDX_Control(pDX, IDC_SLIDERSELF, m_selfSlider);
DDX_Control(pDX, IDC_SLIDERPOLICYPREF, m_policyPrefSlider);
DDX_Text(pDX, IDC_EVALFREQ, m_evalFreq);
   }


BEGIN_MESSAGE_MAP(ScenarioEditor, CDialog)
   ON_NOTIFY(TVN_SELCHANGED, IDC_MODELVARS, &ScenarioEditor::OnTvnSelchangedModelvars)
   ON_EN_CHANGE(IDC_NAME, &ScenarioEditor::OnEnChangeName)
   ON_EN_CHANGE(IDC_ORIGINATOR, &ScenarioEditor::OnEnChangeOriginator)
   ON_EN_CHANGE(IDC_SCNDESCRIPTION, &ScenarioEditor::OnEnChangeScnDescription)
   ON_CBN_SELCHANGE(IDC_DISTRIBUTION, &ScenarioEditor::OnCbnSelchangeDistribution)
   ON_EN_CHANGE(IDC_LOCATION, &ScenarioEditor::OnEnChangeLocation)
   ON_EN_CHANGE(IDC_SCALE, &ScenarioEditor::OnEnChangeScale)
   ON_CLBN_CHKCHANGE(IDC_POLICIES, &ScenarioEditor::OnLbnChkchangePolicies)
   ON_CBN_SELCHANGE(IDC_SCENARIOS, &ScenarioEditor::OnCbnSelchangeScenarios)
   ON_BN_CLICKED(IDC_ADD, &ScenarioEditor::OnBnClickedAdd)
   ON_BN_CLICKED(IDC_DELETE, &ScenarioEditor::OnBnClickedDelete)
   ON_BN_CLICKED(IDC_SAVE, &ScenarioEditor::OnBnClickedSave)
   ON_BN_CLICKED(IDOK, &ScenarioEditor::OnBnClickedOk)
   ON_WM_HSCROLL()
   ON_EN_CHANGE(IDC_EVALFREQ, &ScenarioEditor::OnEnChangeEvalfreq)
   ON_BN_CLICKED(IDC_USEDEFAULTVALUE, &ScenarioEditor::OnBnClickedUsedfaultvalues)
   ON_BN_CLICKED(IDC_USESPECIFIEDVALUE, &ScenarioEditor::OnBnClickedUsespecifiedvalues)
   ON_BN_CLICKED(IDC_GRIDVIEW, &ScenarioEditor::OnBnClickedGridview)
   ON_NOTIFY(TVN_ITEMEXPANDED, IDC_MODELVARS, &ScenarioEditor::OnTvnItemexpandedModelvars)
END_MESSAGE_MAP()


// ScenarioEditor message handlers

BOOL ScenarioEditor::OnInitDialog()
   {
   CDialog::OnInitDialog();

   // make local copy of scenarios
   ScenarioManager::CopyScenarioArray( m_scenarioArray, gpScenarioManager->m_scenarioArray );

   // load scenario combo
   int count = (int) m_scenarioArray.GetSize();

   for ( int i=0; i < count; i++ )
      m_scenarios.AddString( m_scenarioArray[ i ]->m_name );

   if ( count > 0 )
      m_currentIndex = 0;
   else
      m_currentIndex = -1;

   m_scenarios.SetCurSel( m_currentIndex );

   // load all existing policies from policy manager
   for ( int i=0; i < gpPolicyManager->GetPolicyCount(); i++ )
      m_policies.AddString( gpPolicyManager->GetPolicy( i )->m_name );
   
   m_policies.SetCurSel( -1 );

   // load variables from Envmodel
   //m_altruismSlider.SetRange( 0, 100 );
   //m_altruismSlider.SetTicFreq( 10 );
   //m_altruismSlider.SetLineSize( 2 );
   //m_altruismSlider.SetPageSize( 10 );   

   //m_selfSlider.SetRange( 0, 100 );
   //m_selfSlider.SetTicFreq( 10 );
   //m_selfSlider.SetLineSize( 2 );
   //m_selfSlider.SetPageSize( 10 );   

   m_policyPrefSlider.SetRange( 0, 100 );
   m_policyPrefSlider.SetTicFreq( 10 );
   m_policyPrefSlider.SetLineSize( 2 );
   m_policyPrefSlider.SetPageSize( 10 );   

   // load distribution combo box
   m_distribution.AddString( "Constant" );
   m_distribution.AddString( "Uniform" );
   m_distribution.AddString( "Normal" );
   m_distribution.AddString( "LogNormal" );
   m_distribution.AddString( "Weibull" );
   m_distribution.SetCurSel( -1 );

   LoadVarTree();

   // load current scenario info
   LoadScenario();
   
   return TRUE;  // return TRUE unless you set the focus to a control
   }


void ScenarioEditor::LoadScenario()
   {
   m_pScenario = NULL;
   m_pInfo     = NULL;
   m_currentIndex = m_scenarios.GetCurSel();

   if ( m_currentIndex < 0 )
      return;

   m_pScenario = m_scenarioArray[ m_currentIndex ];

   m_name = m_pScenario->m_name;
   m_originator = m_pScenario->m_originator;
   m_scnDescription = m_pScenario->m_description;

   //m_altruism = int( m_pScenario->m_actorAltruismWt * 100 );
   //m_self = int( m_pScenario->m_actorSelfInterestWt * 100 );
   m_policyPref = int( m_pScenario->m_policyPrefWt * 100 );
   m_evalFreq = m_pScenario->m_evalModelFreq;

   char sliderValue[ 16 ];
   //sprintf_s( sliderValue, 16, "%3.1g", m_pScenario->m_actorAltruismWt );
   //GetDlgItem( IDC_ALTRUISM )->SetWindowText( sliderValue );

   //sprintf_s( sliderValue, 16, "%3.1g", m_pScenario->m_actorSelfInterestWt );
   //GetDlgItem( IDC_SELF )->SetWindowText( sliderValue );
   
   sprintf_s( sliderValue, 16, "%3.1g", m_pScenario->m_policyPrefWt );
   GetDlgItem( IDC_POLICYPREF )->SetWindowText( sliderValue );

   // set policy uses
   for ( int i=0; i < gpPolicyManager->GetPolicyCount(); i++ )
      {
      EnvPolicy *pPolicy = gpPolicyManager->GetPolicy( i );
      
      POLICY_INFO *pInfo = m_pScenario->GetPolicyInfo( pPolicy->m_name );
      if ( pInfo == NULL )
         continue;

      int index = -1;
      gpPolicyManager->FindPolicy( pPolicy->m_name, &index ); 

      if ( index >= 0 )
         m_policies.SetCheck( index, pInfo->inUse ? 1 : 0 );
      }
   // Load model variables
   LoadModelVar();

   CheckDlgButton( IDC_DEFAULT, m_pScenario->IsDefault() ? 1 : 0 );
   }


void ScenarioEditor::LoadVarTree()
   {   
   int varIndex = 1;
   HTREEITEM hItem;
   m_hMetagoals = 0;
   m_hModels = 0;
   m_hProcesses = 0;
   m_hAppVars = 0;
   m_hModelUsage= 0;
   m_hProcessUsage = 0;

   //========================== METAGOAL SETTINGS ============================
   int metagoalCount = gpModel->GetMetagoalCount();

   if ( metagoalCount > 0 )
      {
      m_hMetagoals = m_inputVars.InsertItem( "Metagoal Settings" );
      m_inputVars.SetItemData( m_hMetagoals, 20000 );

      for ( int i=0; i < metagoalCount; i++ )
         {
         //int index = EnvModel::GetModelIndexFromMetagoalIndex( i );
         //EnvEvaluator *pInfo = EnvModel::GetEvaluatorInfo( index );
         METAGOAL *pInfo = gpModel->GetMetagoalInfo( i );
         hItem = m_inputVars.InsertItem( pInfo->name, m_hMetagoals );
         m_inputVars.SetItemData( hItem, varIndex );   // index of the associated VAR_INFO structure
         varIndex++;
         }
      }

   //====================== MODEL/PROCESS USAGE SETTINGS =====================
   int modelCount = gpModel->GetEvaluatorCount();
   //
   //if ( modelCount > 0 )
   //   {
   //   m_hModelUsage = m_inputVars.InsertItem( "Model Usage" );
   //   m_inputVars.SetItemData( m_hModelUsage, 21000 );
   //
   //   for ( int i=0; i < modelCount; i++ )
   //      {
   //      EnvEvaluator *pInfo = EnvModel::GetEvaluatorInfo( i );
   //      hItem = m_inputVars.InsertItem( pInfo->m_name, m_hModelUsage );
   //      m_inputVars.SetItemData( hItem, varIndex );   // index of the associated EnvEvaluator structure
   //      varIndex++;
   //      }
   //   }
   //
   //int processCount = EnvModel::GetModelProcessCount();
   //
   //if ( processCount > 0 )
   //   {
   //   m_hProcessUsage = m_inputVars.InsertItem( "Process Usage" );
   //   m_inputVars.SetItemData( m_hProcessUsage, 22000 );
   //
   //   for ( int i=0; i < processCount; i++ )
   //      {
   //      EnvModelProcess *pInfo = EnvModel::GetModelProcessInfo( i );
   //      hItem = m_inputVars.InsertItem( pInfo->m_name, m_hProcessUsage );
   //      m_inputVars.SetItemData( hItem, varIndex );   // index of the associated EnvModelProcess structure
   //      varIndex++;
   //      }
   //   }

   //================================ M O D E L   V A R I A B L E S ==========================
   if ( modelCount > 0 )
      {
      m_hModels = m_inputVars.InsertItem( "Model Variables" );
      m_inputVars.SetItemData( m_hModels, 23000 );
   
      for( int i=0; i < modelCount; i++ )
	      {
         EnvEvaluator *pModel = gpModel->GetEvaluatorInfo( i );
         //if ( pModel->inputVarFn != NULL )
            {
            MODEL_VAR *modelVarArray = NULL;
            int varCount = pModel->InputVar( pModel->m_id, &modelVarArray );

            if ( varCount > 0 )
               {
               HTREEITEM hModelItem = m_inputVars.InsertItem( pModel->m_name, m_hModels ); 
               
               // for each variable exposed by the model, add it to the possible scenario variables
               for ( int j=0; j < varCount; j++ )
                  {
                  MODEL_VAR &mv = modelVarArray[ j ];
                  if ( mv.pVar != NULL )
                     {
                     hItem = m_inputVars.InsertItem( mv.name, hModelItem );
                     m_inputVars.SetItemData( hItem, varIndex );   // index of the associated VAR_INFO structure
                     varIndex++;
                     }
                  }
               }
            }
         }
      }

   //================= A U T O N O U S  P R O C E S S   V A R I A B L E S ==================
   int apCount = gpModel->GetModelProcessCount();

   if ( apCount > 0 )
      {
      m_hProcesses = m_inputVars.InsertItem( "Process Variables" );
      m_inputVars.SetItemData( m_hProcesses, 24000 );

      for( int i=0; i < apCount; i++ )
   	   {
         EnvModelProcess *pAP = gpModel->GetModelProcessInfo( i );
//         if ( pAP->inputVarFn != NULL )
            {
            MODEL_VAR *modelVarArray = NULL;
            int varCount = pAP->InputVar( pAP->m_id, &modelVarArray );
   
            if ( varCount > 0 )
               {
               HTREEITEM hProcessItem = m_inputVars.InsertItem( pAP->m_name, m_hProcesses ); 
   
               // for each variable exposed by the model, add it to the possible scenario variables
               for ( int j=0; j < varCount; j++ )
                  {
                  MODEL_VAR &mv = modelVarArray[ j ];
                  if (mv.pVar != NULL )
                     {
                     hItem = m_inputVars.InsertItem( mv.name, hProcessItem );
                     m_inputVars.SetItemData( hItem, varIndex );   // index of the associated VAR_INFO structure
                     varIndex++;
                     }
                  }
               }
            }
         }
      }

   //================= A P P L I C A T I O N   V A R I A B L E S ==================
   int apVarCount = gpModel->GetAppVarCount( (int) AVT_APP );
   int usedAppVarCount = 0;
   for( int i=0; i < apVarCount; i++ )
	   {
      AppVar *pVar = gpModel->GetAppVar( i );

      if ( pVar->m_avType == AVT_APP && pVar->m_timing > 0 )
         usedAppVarCount++;
      }

   if ( usedAppVarCount > 0 )
      {
      m_hAppVars = m_inputVars.InsertItem( "Application Variables" );

      for( int i=0; i < apVarCount; i++ )
	      {
         AppVar *pVar = gpModel->GetAppVar( i );

         if ( pVar->m_avType == AVT_APP && pVar->m_timing > 0 )
            {
            HTREEITEM hAppVarItem = m_inputVars.InsertItem( pVar->m_name, m_hAppVars ); 
            m_inputVars.SetItemData( hAppVarItem, -(i+1) );   // index of the associated APP_VAR structure negative value signals it's an APP_VAR
            }
         }
      }
   else
      m_hAppVars = 0;

   //=============== S C E N A R I O   S E T T I N G S  ==========================
   hItem = m_inputVars.InsertItem( "Decision Elements" );
   m_inputVars.SetItemData( hItem, 10000 );   

   ///  finish up...
   if ( m_hMetagoals )  m_inputVars.Expand( m_hMetagoals, TVE_EXPAND );
   if ( m_hModels    )  m_inputVars.Expand( m_hModels, TVE_EXPAND );
   if ( m_hProcesses )  m_inputVars.Expand( m_hProcesses, TVE_EXPAND );
   if ( m_hAppVars   )  m_inputVars.Expand( m_hAppVars, TVE_EXPAND );

   if ( m_hProcessUsage )  m_inputVars.Expand( m_hProcessUsage, TVE_COLLAPSE );
   if ( m_hModelUsage   )  m_inputVars.Expand( m_hModelUsage, TVE_COLLAPSE );
   }


void ScenarioEditor::LoadModelVar()
   {
   // is there an active model var selected?
   HTREEITEM hItem = m_inputVars.GetSelectedItem();

   m_distribution.EnableWindow( 0 );

   GetDlgItem( IDC_LABELLOCATION )->ShowWindow( SW_HIDE );
   GetDlgItem( IDC_LOCATION )->ShowWindow( SW_HIDE );
   GetDlgItem( IDC_LABELSCALE )->ShowWindow( SW_HIDE );
   GetDlgItem( IDC_SCALE )->ShowWindow( SW_HIDE );

   m_description.SetWindowText( "" );

   m_pInfo = NULL;
   m_pAppVar = NULL;

   if ( hItem != NULL )
      {
      int itemData = (int) m_inputVars.GetItemData( hItem );

      //HTREEITEM hParentItem = m_inputVars.GetParentItem( hItem );
      //int parentData = m_inputVars.GetItemData( hParentItem );

      // is it decisionElements?
      if ( itemData == 10000 )
         {
         SCENARIO_VAR *pVar = m_pScenario->FindScenarioVar( &(m_pScenario->m_decisionElements ) );
         ASSERT( pVar != NULL );
         
         CheckDlgButton( IDC_USEDEFAULTVALUE, pVar->inUse ? 0 : 1 );
         CheckDlgButton( IDC_USESPECIFIEDVALUE, pVar->inUse ? 1 : 0 );
 
          if ( pVar->inUse )
             pVar->paramLocation.GetAsString( m_location );  // show current value
          else
             pVar->defaultValue.GetAsString( m_location );
         
          m_pInfo = pVar;
          }
          
      //else if ( parentData == 21000 )  // hModelUsage
      //   {
      //   VAR_INFO *pVar = m_pScenario->FindVar( &(m_pScenario->m_decisionElements ) );
      //   ASSERT( pVar != NULL );
      //   
      //   }
      //
      //else if ( parentData == 22000 )  // hProcessUsage
      //   {
      //   }

      // or is it a model/process input variable
      else if ( itemData > 0 && itemData < 1000 )
         {      
         int varInfoIndex = int( itemData ) - 1;
         SCENARIO_VAR &var = m_pScenario->GetScenarioVar( varInfoIndex );
         m_pInfo = &var;

         CheckDlgButton( IDC_USEDEFAULTVALUE,   var.inUse ? 0 : 1 );
         CheckDlgButton( IDC_USESPECIFIEDVALUE, var.inUse ? 1 : 0 );

         m_distribution.SetCurSel( var.distType );

         switch( var.distType )
            {
            case MD_CONSTANT:
               if ( var.inUse )
                  var.paramLocation.GetAsString( m_location );
               else
                  var.defaultValue.GetAsString( m_location );
               break;

            case MD_UNIFORM:
               var.paramLocation.GetAsString( m_location );
               var.paramScale.GetAsString( m_scale );
               break;
                  
            case MD_NORMAL:
               var.paramLocation.GetAsString( m_location );
               var.paramScale.GetAsString( m_scale );
               break;
       
            case MD_WEIBULL:
               var.paramLocation.GetAsString( m_location );
               var.paramScale.GetAsString( m_scale );
               break;

            case MD_LOGNORMAL:
               var.paramLocation.GetAsString( m_location );
               var.paramScale.GetAsString( m_scale );
               break;
            }

         if ( m_location.Find( '.' ) >= 0  )
            m_location.TrimRight( '0' );
         
         if ( m_scale.Find( '.' ) >= 0  )
            m_scale.TrimRight( '0' );

         if ( ! var.description.IsEmpty() )
            m_description.SetWindowText( var.description );
         }  // end of: if ( itemData > 0 );

      else if ( itemData < 0 )  // it's an app variable
         {      
         int appVarIndex = int( -itemData ) - 1;
         m_pAppVar = gpModel->GetAppVar( appVarIndex );
         }
      }

   UpdateData( 0 );        // refresh dialog field with variable values
   EnableControls();
   }


void ScenarioEditor::EnableControls()
   {
   GetDlgItem( IDC_USEDEFAULTVALUE   )->EnableWindow( 0 );
   GetDlgItem( IDC_USESPECIFIEDVALUE )->EnableWindow( 0 );

   GetDlgItem( IDC_LABELDISTRIBUTION )->ShowWindow( SW_HIDE );
   GetDlgItem( IDC_DISTRIBUTION      )->ShowWindow( SW_HIDE );
   GetDlgItem( IDC_LABELLOCATION     )->ShowWindow( SW_HIDE );
   GetDlgItem( IDC_LOCATION          )->ShowWindow( SW_HIDE );
   GetDlgItem( IDC_LABELSCALE        )->ShowWindow( SW_HIDE );
   GetDlgItem( IDC_SCALE             )->ShowWindow( SW_HIDE );


   if ( m_pInfo == NULL )     // not a model variable
      return;
   
   GetDlgItem( IDC_USEDEFAULTVALUE   )->EnableWindow( 1 );
   GetDlgItem( IDC_USESPECIFIEDVALUE )->EnableWindow( 1 );

   // its a variable so turn on appropriate stuff
   GetDlgItem( IDC_LABELDISTRIBUTION )->ShowWindow( SW_SHOW );
   GetDlgItem( IDC_DISTRIBUTION      )->ShowWindow( SW_SHOW );
   GetDlgItem( IDC_LABELLOCATION     )->ShowWindow( SW_SHOW );
   GetDlgItem( IDC_LOCATION          )->ShowWindow( SW_SHOW );

   GetDlgItem( IDC_LABELDISTRIBUTION )->EnableWindow( m_pInfo->inUse ? 1 : 0 );
   GetDlgItem( IDC_DISTRIBUTION      )->EnableWindow( m_pInfo->inUse ? 1 : 0 );
   GetDlgItem( IDC_LABELLOCATION     )->EnableWindow( m_pInfo->inUse ? 1 : 0 );
   GetDlgItem( IDC_LOCATION          )->EnableWindow( m_pInfo->inUse ? 1 : 0 );
   GetDlgItem( IDC_LABELSCALE        )->EnableWindow( m_pInfo->inUse ? 1 : 0 );
   GetDlgItem( IDC_SCALE             )->EnableWindow( m_pInfo->inUse ? 1 : 0 );

   switch( m_pInfo->distType )
      {
      case MD_CONSTANT:
         GetDlgItem( IDC_LABELLOCATION )->SetWindowText( "Value:" );
         break;

      case MD_UNIFORM:
         GetDlgItem( IDC_LABELLOCATION )->SetWindowText( "Min:" );
         GetDlgItem( IDC_LABELSCALE )->SetWindowText( "Max:" );
         GetDlgItem( IDC_LABELSCALE )->ShowWindow( SW_SHOW );
         GetDlgItem( IDC_SCALE )->ShowWindow( SW_SHOW );
         break;
                  
      case MD_NORMAL:
         GetDlgItem( IDC_LABELLOCATION )->SetWindowText( "Mean:" );
         GetDlgItem( IDC_LABELSCALE )->SetWindowText( "StdDev:" );
         GetDlgItem( IDC_LABELSCALE )->ShowWindow( SW_SHOW );
         GetDlgItem( IDC_SCALE )->ShowWindow( SW_SHOW );
         break;
       
      case MD_WEIBULL:
         GetDlgItem( IDC_LABELLOCATION )->SetWindowText( "Alpha:" );
         GetDlgItem( IDC_LABELSCALE )->SetWindowText( "Beta:" );
         GetDlgItem( IDC_LABELSCALE )->ShowWindow( SW_SHOW );
         GetDlgItem( IDC_SCALE )->ShowWindow( SW_SHOW );
         break;

      case MD_LOGNORMAL:
         GetDlgItem( IDC_LABELLOCATION )->SetWindowText( "Mean:" );
         GetDlgItem( IDC_LABELSCALE )->SetWindowText( "Variance:" );
         GetDlgItem( IDC_LABELSCALE )->ShowWindow( SW_SHOW );
         GetDlgItem( IDC_SCALE )->ShowWindow( SW_SHOW );
         break;
      }
   }


void ScenarioEditor::OnTvnSelchangedModelvars(NMHDR *pNMHDR, LRESULT *pResult)
   {
   LoadModelVar();
   *pResult = 0;
   }


void ScenarioEditor::OnEnChangeName()
   {
   GetDlgItem( IDC_NAME )->GetWindowText( m_pScenario->m_name );

   m_scenarios.InsertString( m_currentIndex, m_pScenario->m_name );
   m_scenarios.DeleteString( m_currentIndex + 1 );
   m_scenarios.SetCurSel( m_currentIndex );
   MakeDirty();
   }


void ScenarioEditor::OnEnChangeOriginator()
   {
   GetDlgItem( IDC_ORIGINATOR )->GetWindowText( m_pScenario->m_originator );
   MakeDirty();
   }

void ScenarioEditor::OnEnChangeScnDescription()
   {
   GetDlgItem( IDC_SCNDESCRIPTION )->GetWindowText( m_pScenario->m_description );
   MakeDirty();
   }

void ScenarioEditor::OnCbnSelchangeDistribution()
   {
   if ( m_pInfo == NULL )
      return;

   m_pInfo->distType = (MODEL_DISTR) m_distribution.GetCurSel();

   EnableControls();
   MakeDirty();
   }


void ScenarioEditor::OnEnChangeLocation()
   {
   if ( m_pInfo == NULL )
      return;

   UpdateData( 1 );    // save dlg values to class members

   switch( m_pInfo->type )
      {
      case TYPE_INT:
      case TYPE_LONG:
      case TYPE_UINT:
      case TYPE_ULONG:
         m_pInfo->paramLocation = atoi( m_location );
         break;

      case TYPE_FLOAT:
      case TYPE_DOUBLE:
         m_pInfo->paramLocation = atof( m_location );
         break;

      case TYPE_STRING:
      case TYPE_DSTRING:
         m_pInfo->paramLocation = m_location;
         break;

      case TYPE_BOOL:
         if ( m_location[ 0 ] == '1' || m_location[ 0 ] == 't' || m_location[ 0 ] == 'T' 
            || m_location[ 0 ] == 'y' || m_location[ 0 ] == 'Y' )
            m_pInfo->paramLocation = true;
         else
            m_pInfo->paramLocation = false;
         break;
      }
      
   MakeDirty();
   }


void ScenarioEditor::OnEnChangeScale()
   {
   if ( m_pInfo == NULL )
      return;

   UpdateData( 1 );

   m_pInfo->paramScale = m_scale;
   MakeDirty();
   }


void ScenarioEditor::OnLbnChkchangePolicies()
   {
   if ( m_pScenario == NULL )
      return;

   int index = m_policies.GetCurSel();

   if ( index < 0 )
      return;

   POLICY_INFO &pi = m_pScenario->GetPolicyInfo( index );

   pi.inUse = ( m_policies.GetCheck( index ) == 0 ) ? false : true;
   MakeDirty();
   }


void ScenarioEditor::OnCbnSelchangeScenarios()
   {
   LoadScenario();
   }


bool inUpdate = false;
/*
void ScenarioEditor::OnEnChangeSlidervalue()
   {
   if ( inUpdate )
      return;

   if ( m_pScenario == NULL )
      return;

   char buffer[ 32 ];
   GetDlgItem( IDC_SLIDERVALUE )->GetWindowText( buffer, 31 );

   float value = 0;
   value = (float) atof( buffer );

   m_pScenario->m_actorAltruismWt = value;
   MakeDirty();

   int pos = int( value * 100 );

   inUpdate = true;
   this->m_altruismSlider.SetPos( pos );
   inUpdate = false;
   }
*/

void ScenarioEditor::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
   {
   CDialog::OnHScroll(nSBCode, nPos, pScrollBar);

   if ( inUpdate )
      return;

   CSliderCtrl *pCtrl = (CSliderCtrl*) pScrollBar;

   switch( nSBCode )
      {
      case SB_LEFT:  //   Scroll to far left.
         nPos = 0;
         break;

      case SB_ENDSCROLL: //   End scroll.
         return;

      case SB_LINELEFT:  //   Scroll left.
         nPos = pCtrl->GetPos();
         nPos -= 2;
         if ( nPos < 0 ) nPos = 0;
         break;

      case SB_LINERIGHT: //   Scroll right.
         nPos = pCtrl->GetPos();
         nPos += 2;
         if ( nPos > 100 ) nPos = 100;
         break;

      case SB_PAGELEFT:  //   Scroll one page left.
         nPos = pCtrl->GetPos();
         nPos -= 10;
         if ( nPos < 0 ) nPos = 0;
         break;

      case SB_PAGERIGHT:  //   Scroll one page right.
         nPos = pCtrl->GetPos();
         nPos += 10;
         if ( nPos > 100 ) nPos = 100;
         break;

      case SB_RIGHT: //   Scroll to far right.
         nPos = 100;
         break;

      case SB_THUMBPOSITION: //   Scroll to absolute position. The current position is specified by the nPos parameter.
      case SB_THUMBTRACK:
         break;

      default:
         ASSERT( 0 );
      }

   // which scrollbar?
   int id = pScrollBar->GetDlgCtrlID();
   float wt = nPos/100.0f;
/*
   switch( id )
      {
      case IDC_SLIDERALTRUISM:
         {
         float delta = wt - m_pScenario->m_actorAltruismWt;
         if ( delta == 0 )
            break;

         m_pScenario->m_actorAltruismWt = wt;

         if ( m_pScenario->m_actorSelfInterestWt > 0 || m_pScenario->m_policyPrefWt > 0 )
            {
            float ratio = m_pScenario->m_actorSelfInterestWt / ( m_pScenario->m_actorSelfInterestWt + m_pScenario->m_policyPrefWt );
            m_pScenario->m_actorSelfInterestWt -= ( delta * ratio );
            m_pScenario->m_policyPrefWt = 1 - wt - m_pScenario->m_actorSelfInterestWt;
            }
         else  // both equal to zero
            {
            m_pScenario->m_actorSelfInterestWt = -delta/2;
            m_pScenario->m_policyPrefWt = -delta/2;
            }
         }
         break;

      case IDC_SLIDERSELF:
         {
         float delta = wt - m_pScenario->m_actorSelfInterestWt;
         if ( delta == 0 )
            break;

         m_pScenario->m_actorSelfInterestWt = wt;
         if ( m_pScenario->m_actorAltruismWt > 0 || m_pScenario->m_policyPrefWt > 0 )
            {
            float ratio = m_pScenario->m_actorAltruismWt / ( m_pScenario->m_actorAltruismWt + m_pScenario->m_policyPrefWt );
            m_pScenario->m_actorAltruismWt -= ( delta * ratio );
            m_pScenario->m_policyPrefWt = 1 - wt - m_pScenario->m_actorAltruismWt;       
            }
         else
            {
            m_pScenario->m_actorAltruismWt = -delta/2;
            m_pScenario->m_policyPrefWt = -delta/2;
            }
         }
         break;

      case IDC_SLIDERPOLICYPREF:
         {         
         float delta = wt - m_pScenario->m_policyPrefWt;
         if ( delta == 0 )
            break;

         m_pScenario->m_policyPrefWt = wt;
         if ( m_pScenario->m_actorAltruismWt > 0 || m_pScenario->m_actorSelfInterestWt > 0 )
            {
            float ratio = m_pScenario->m_actorSelfInterestWt / ( m_pScenario->m_actorSelfInterestWt + m_pScenario->m_actorAltruismWt );
            m_pScenario->m_actorSelfInterestWt -= ( delta * ratio );
            m_pScenario->m_actorAltruismWt = 1 - wt - m_pScenario->m_actorSelfInterestWt;       
            }
         else
            {
            m_pScenario->m_actorAltruismWt = -delta/2;
            m_pScenario->m_actorSelfInterestWt = -delta/2;
            }
         }
         break;

      default:
         ASSERT( 0 );
      }
*/
/*
   if ( m_pScenario->m_actorAltruismWt < 0 ) m_pScenario->m_actorAltruismWt = 0;
   if ( m_pScenario->m_actorAltruismWt > 1 ) m_pScenario->m_actorAltruismWt = 1;

   if ( m_pScenario->m_actorSelfInterestWt < 0 ) m_pScenario->m_actorSelfInterestWt = 0;
   if ( m_pScenario->m_actorSelfInterestWt > 1 ) m_pScenario->m_actorSelfInterestWt = 1;
*/      
   if ( m_pScenario->m_policyPrefWt < 0 ) m_pScenario->m_policyPrefWt = 0;
   if ( m_pScenario->m_policyPrefWt > 1 ) m_pScenario->m_policyPrefWt = 1;

   //NormalizeWeights( m_pScenario->m_actorAltruismWt, m_pScenario->m_actorSelfInterestWt, m_pScenario->m_policyPrefWt );

   //if ( m_pScenario->m_actorAltruismWt < 0 ) m_pScenario->m_actorAltruismWt = 0;
   //if ( m_pScenario->m_actorAltruismWt > 1 ) m_pScenario->m_actorAltruismWt = 1;

   //if ( m_pScenario->m_actorSelfInterestWt < 0 ) m_pScenario->m_actorSelfInterestWt = 0;
   //if ( m_pScenario->m_actorSelfInterestWt > 1 ) m_pScenario->m_actorSelfInterestWt = 1;
      
   //if ( m_pScenario->m_policyPrefWt < 0 ) m_pScenario->m_policyPrefWt = 0;
   //if ( m_pScenario->m_policyPrefWt > 1 ) m_pScenario->m_policyPrefWt = 1;

   inUpdate = true;

   char buffer[ 32 ];
   //sprintf_s( buffer, 32, "%4.2g", m_pScenario->m_actorAltruismWt );
   //GetDlgItem( IDC_ALTRUISM )->SetWindowText( buffer ); 

   //sprintf_s( buffer, 32, "%4.2g", m_pScenario->m_actorSelfInterestWt );
   //GetDlgItem( IDC_SELF)->SetWindowText( buffer ); 

   sprintf_s( buffer, 32, "%4.2g", m_pScenario->m_policyPrefWt );
   GetDlgItem( IDC_POLICYPREF )->SetWindowText( buffer ); 

   //m_altruismSlider.SetPos( int( m_pScenario->m_actorAltruismWt * 100 ) );
   //m_selfSlider.SetPos( int( m_pScenario->m_actorSelfInterestWt * 100 ) );
   //m_policyPrefSlider.SetPos( int( m_pScenario->m_policyPrefWt * 100 ) );

   inUpdate = false;

   MakeDirty();
   }


void ScenarioEditor::OnBnClickedAdd()
   {
   m_pScenario = new Scenario( gpScenarioManager, "New Scenario" );
   m_scenarioArray.Add( m_pScenario );

   int index = m_scenarios.AddString( m_pScenario->m_name );

   m_scenarios.SetCurSel( index );

   LoadScenario();
   MakeDirty();
   }

void ScenarioEditor::OnBnClickedDelete()
   {
   if ( m_pScenario == NULL )
      return;

   int retVal = MessageBox( "Do you want to delete this scenario?", "Confirm Deletion", MB_YESNO );

   if ( retVal != IDYES )
      return;

   delete m_pScenario;
   m_scenarioArray.RemoveAt( m_currentIndex );
   
   if ( m_scenarioArray.GetSize() == 0 )
      m_scenarios.SetCurSel( -1 );
   else
      m_scenarios.SetCurSel( 0 );

   LoadScenario();
   MakeDirty();
   }


void ScenarioEditor::OnBnClickedSave()
   {
   SaveScenarios();
   }

int ScenarioEditor::SaveScenarios()
   {
   DirPlaceholder d;
   CString startPath = gpScenarioManager->m_path;
   if ( startPath.IsEmpty() )
      startPath = gpDoc->m_iniFile;

   SaveToDlg dlg( "Scenarios", startPath );

   int retVal = (int) dlg.DoModal();

   if ( retVal == IDOK )
      {
      if ( dlg.m_saveThisSession )
         {
         ScenarioManager::CopyScenarioArray( gpScenarioManager->m_scenarioArray, m_scenarioArray );
         gpDoc->UnSetChanged( CHANGED_SCENARIOS );
         }

      if ( dlg.m_saveToDisk )
         {
         m_path = dlg.m_path;

         nsPath::CPath path( m_path );
         CString ext = path.GetExtension();

         if (ext.CompareNoCase( _T("envx") ) == 0 )
            gpDoc->OnSaveDocument( m_path );
         else
            gpScenarioManager->SaveXml( m_path );

         gpDoc->UnSetChanged( CHANGED_SCENARIOS );
         }

      m_isDirty = false;      
      }

   return retVal;
   }


void ScenarioEditor::OnBnClickedOk()
   {
   if ( m_isDirty )
      {
      int retVal = MessageBox( "Scenarios have been modified. Do you want to save the changes?", "Save Changes", MB_YESNOCANCEL );

      switch( retVal )
         {
         case IDYES:
            if ( SaveScenarios() != IDOK )
               return;
            break;

         case IDNO:
            break;

         case IDCANCEL:
            return;
         }
      }

   OnOK();
   }


void ScenarioEditor::OnEnChangeEvalfreq()
   {
   if ( m_pInfo == NULL )
      return;

   UpdateData( 1 );

   m_pScenario->m_evalModelFreq = m_evalFreq;

   MakeDirty();
   }

void ScenarioEditor::OnBnClickedUsedfaultvalues()
   {
   if ( m_pInfo == NULL )
      return;

   m_pInfo->inUse = false;
   MakeDirty();
   EnableControls();
   }

void ScenarioEditor::OnBnClickedUsespecifiedvalues()
   {
   if ( m_pInfo == NULL )
      return;

   m_pInfo->inUse = true;
   MakeDirty();
   EnableControls();
   }


void ScenarioEditor::OnBnClickedGridview()
   {
   ScenarioGridDlg dlg;
   dlg.DoModal();
   }


void ScenarioEditor::OnTvnItemexpandedModelvars(NMHDR *pNMHDR, LRESULT *pResult)
   {
   LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
   // TODO: Add your control notification handler code here
   *pResult = 0;
   }
