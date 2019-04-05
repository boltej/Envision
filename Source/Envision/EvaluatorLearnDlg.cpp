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
// EvaluatorLearnDlg.cpp : implementation file
#include "stdafx.h"
#define NOMINMAX
#include <utility>
using std::pair;

#include "EvaluatorLearnDlg.h"

#ifndef __INI_TEST__
#include "Sandbox.h"
#include ".\evaluatorlearndlg.h"
extern PolicyManager   *gpPolicyManager ;
extern ScenarioManager *gpScenarioManager;
extern EnvModel        *gpModel;
const int bHeight = 50;
const int bWidth  = 80;
const int margin  = 3;
#endif
#undef NOMINMAX
//===============================
#ifdef __INI_TEST__             //
CArray<Policy> g_TestPolicies;  //
EnvModel model;                 //
EnvModel * gpModel = &model;               //
#endif                          //
//=======================================================================================
IMPLEMENT_DYNAMIC(EvaluatorLearnDlg, CDialog)
EvaluatorLearnDlg::EvaluatorLearnDlg(CWnd* pParent /*=NULL*/)
	: CDialog(EvaluatorLearnDlg::IDD, pParent)
{
}

EvaluatorLearnDlg::~EvaluatorLearnDlg()
{
}

BOOL EvaluatorLearnDlg::OnInitDialog() 
{
   CDialog::OnInitDialog();

   Policy * pPolicy = NULL;
   int polID = -1;
   int lbi;
   int i;
   pair< map<int,Policy*>::iterator, bool > polPr;

#ifndef __INI_TEST__
   
   // TODO: Add extra initialization here
   ASSERT(gpScenarioManager);
   ASSERT(gpPolicyManager);
   ASSERT(gpModel);

   // Models list box
   DWORD modelCount = gpModel->GetEvaluatorCount();
   EnvEvaluator * mi;
   int idx;

   for( DWORD m = 0; m < modelCount; ++m) 
      {
      mi = gpModel->GetEvaluatorInfo(m);
      idx = m_modelListBox.AddString(mi->m_name);
      if (idx < 0)
         {
         ASSERT (idx != LB_ERR && idx != LB_ERRSPACE);
         return true;
         }
      VERIFY ( LB_ERR != m_modelListBox.SetItemData(idx, m));
      VERIFY ( LB_ERR != m_modelListBox.SetItemDataPtr(idx, mi));
      VERIFY ( LB_ERR != m_modelListBox.SetSel(idx, 1));
      }

   // scenarios and spanning set of policies controls.
   int  scenCount = gpScenarioManager->GetCount();
   int  scenVarCount = -1;

   Scenario * pScenario = NULL; // can't const because Scenario has no const members

   ////LVITEM lvitem {                                       
   ////   mask,                                         
   ////      0, // iItem
   ////      0, // iSubItem
   ////      state,
   ////      LVIS_SELECTED, // stateMask
   ////      NULL, // pszText
   ////      32, //int cchTextMax; 
   ////      0, // iIMage
   ////      0, // lParam,
   ////      0, // iIndent
   ////      0, // iGroupId;
   ////      1, // UINT cColumns; // tile view columns
   ////      NULL //PUINT puColumns;
   ////   };
  for( i = 0; i < scenCount; ++i) 
      {
      pScenario = gpScenarioManager->GetScenario(i);
      scenVarCount = pScenario->GetScenarioVarCount();
      CString scenName = pScenario->m_name;
      lbi = this->m_scenarioListBox.AddString(scenName);
      if (lbi < 0)
         {
         ASSERT (lbi != LB_ERR && lbi != LB_ERRSPACE);
         return true;
         }

      this->m_scenarioListBox.SetItemDataPtr(lbi, pScenario);

      // UNSELECT Default Scenario
      scenName.MakeUpper();
      if (-1 == scenName.Find("DEFAULT"))
         {
         VERIFY (LB_ERR != m_scenarioListBox.SetSel(lbi, 1));
         } 
      else 
         {
         VERIFY ( LB_ERR != m_scenarioListBox.SetSel(lbi, 0));
         continue;                         
         }

      // GET THE SPANNING (over Scenario) SET OF POLICIES
      int polCount = pScenario->GetPolicyCount();
      for (int j = 0; j < polCount; ++j)
         {
         polID = -1;
         pPolicy = NULL;
         POLICY_INFO &info = pScenario->GetPolicyInfo(j);

         if ( info.inUse == false ) 
            continue;

         pPolicy = info.pPolicy;
         polID  = pPolicy->m_id;

         polPr = m_policySet.insert(pair<int,Policy*>(polID, pPolicy)); 
         }
      }
#endif
   //-------------------------------------------------------//
      #ifdef __INI_TEST__                                   //
      lbi = this->m_scenarioListBox.AddString("LB00");      //
      VERIFY (LB_ERR != m_scenarioListBox.SetSel(lbi, 1));  //
      lbi = this->m_scenarioListBox.AddString("LB01");      //
      VERIFY (LB_ERR != m_scenarioListBox.SetSel(lbi, 1));  //
      lbi = this->m_scenarioListBox.AddString("LB02");      //
      VERIFY (LB_ERR != m_scenarioListBox.SetSel(lbi, 1));  //
      //                                                    //
      g_TestPolicies.Add(Policy("P00", 0));                 //
      g_TestPolicies.Add(Policy("P01", 1));                 //
      g_TestPolicies.Add(Policy("P02", 2));                 //
      for( i=0; i< g_TestPolicies.GetCount(); ++i)          //////
         polPr = m_policySet.insert(pair<int,Policy*>(g_TestPolicies.GetAt(i).m_id, &g_TestPolicies.GetAt(i))); //
      #endif                                                /////
   //-------------------------------------------------------

   UpdatePolicyListControl();

   //UpdateData( false );

   return TRUE;   // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
//=======================================================================================
void EvaluatorLearnDlg::UpdatePolicyListControl()
   {
   int j;
   map< int, Policy * >::iterator polIt;
   UINT mask =  LVIF_STATE | LVIF_TEXT | LVIF_PARAM; // 
   UINT state = LVIS_SELECTED ;
   UINT stateMask = LVIS_SELECTED ;
   ASSERT(sizeof (LPARAM) == sizeof(polIt->second));

   m_policyListCtrl.DeleteAllItems();

   for (j = 0, polIt = m_policySet.begin(); polIt != m_policySet.end();  ++polIt )
      {
      int nItem = j++;
      m_policyListCtrl.InsertItem(nItem, polIt->second->m_name);
      if (0 == this->m_policyListCtrl.SetItem(
         nItem,                  //int nItem,
         -1,                     //0, //int nSubItem,
         mask,                   // UINT nMask,
         polIt->second->m_name,  // LPCTSTR lpszItem,
         0,                      // int nImage,
         state,                  //UINT nState,
         stateMask,              //UINT nStateMask,
         reinterpret_cast<LPARAM>(polIt->second), //LPARAM lParam,
         0                       //int nIndent 
         ))
         {
         ASSERT(0);// error
         }
      }
   }
//=======================================================================================
void EvaluatorLearnDlg::OnOK() 
{
   // TODO: Add extra validation here
   
   // Ensure that your UI got the necessary input 
   // from the user before closing the dialog. The 
   // default OnOK will close this.

   //UpdateData(TRUE);

   for (int k = 0; k < m_policyListCtrl.GetItemCount(); ++k)      
      {
      if ( ! (LVIS_SELECTED & m_policyListCtrl.GetItemState(k, LVIS_SELECTED)) )
         {
         Policy * pPolicy = reinterpret_cast<Policy*>(m_policyListCtrl.GetItemData(k));
         int polID = pPolicy->m_id;
         m_policySet.erase(polID);
         }
      }

   DWORD_PTR dx;
   int midx;
   EnvEvaluator * mi;
   for (int m = 0; m < m_modelListBox.GetCount(); ++ m)
      {
      if ( 0 < m_modelListBox.GetSel(m) )
         {
         mi = static_cast<EnvEvaluator*> (m_modelListBox.GetItemDataPtr(m));
         dx = m_modelListBox.GetItemData(m); // strangely wrong
         midx = gpModel->FindEvaluatorIndex(mi->m_name);
         m_idxModelInfo.insert(midx);
         }
      }

   m_scenarioSet.clear();  
   CArray<int,int> aryListBoxSel;
   int nCount = m_scenarioListBox.GetCount();
   aryListBoxSel.SetSize(nCount);
   int nSel = m_scenarioListBox.GetSelItems(nCount, aryListBoxSel.GetData());
   for (int i=0; i<nSel; ++i)
      m_scenarioSet.insert(static_cast<Scenario*>(m_scenarioListBox.GetItemDataPtr(aryListBoxSel[i])));

   CDialog::OnOK(); // This will close the dialog and DoModal will return.
}

void EvaluatorLearnDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_LB_IDD_EVAL_MODEL_LEARN_DLG, m_scenarioListBox);
DDX_Control(pDX, IDC_LC_IDD_EVAL_MODEL_LEARN_DLG, m_policyListCtrl);
DDX_Control(pDX, IDC_LB_MODELS_IDD_EVAL_MODEL_LEARN_DLG, m_modelListBox);
}


BEGIN_MESSAGE_MAP(EvaluatorLearnDlg, CDialog)
   //ON_WM_CREATE()
   ON_LBN_SELCHANGE(IDC_LB_IDD_EVAL_MODEL_LEARN_DLG, OnLbnSelchangeLbIddEvaluatorLearnDlg)
END_MESSAGE_MAP()

void EvaluatorLearnDlg::OnLbnSelchangeLbIddEvaluatorLearnDlg()
   {
   // TODO: Add your control notification handler code here
   // Get the indexes of all the selected items.
   // Redo the spanning set of policies.
   pair< map<int,Policy*>::iterator, bool > polPr;
   CArray<int,int> aryListBoxSel;
   int nCount = m_scenarioListBox.GetCount();
   aryListBoxSel.SetSize(nCount);
   int nSel = m_scenarioListBox.GetSelItems(nCount, aryListBoxSel.GetData());
   
   m_policySet.clear();

   for (int i=0; i<nSel; ++i)
      {    
      #ifndef __INI_TEST__
      Scenario * pScenario = static_cast<Scenario*>(m_scenarioListBox.GetItemDataPtr(aryListBoxSel[i]));
      int polCount = pScenario->GetPolicyCount();

      for (int j = 0; j < polCount; ++j)
         {
         int polID = -1;
         Policy *pPolicy = NULL;
         POLICY_INFO &info = pScenario->GetPolicyInfo(j);

         if ( info.inUse == false ) 
            continue;

         pPolicy = info.pPolicy;
         polID  = pPolicy->m_id;

         polPr = m_policySet.insert(pair<int,Policy*>(polID, pPolicy)); 
         }
      #endif
      #ifdef __INI_TEST__
         polPr = m_policySet.insert(pair<int,Policy*>(g_TestPolicies[aryListBoxSel[i]].m_id, &g_TestPolicies[aryListBoxSel[i]]));          
      #endif
      }
   
   UpdatePolicyListControl();

   }
