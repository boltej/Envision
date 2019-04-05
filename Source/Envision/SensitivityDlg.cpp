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
// SensitivityDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "EnvExtension.h"
#include "SensitivityDlg.h"
#include "afxdialogex.h"

#include <PathManager.h>
#include <EnvConstants.h>
#include <EnvModel.h>
#include <Policy.h>
#include <Scenario.h>


extern PolicyManager   *gpPolicyManager;
extern ScenarioManager *gpScenarioManager;
extern EnvModel        *gpModel;

// SensitivityDlg dialog

IMPLEMENT_DYNAMIC(SensitivityDlg, CDialogEx)

SensitivityDlg::SensitivityDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(SensitivityDlg::IDD, pParent)
   , m_perturbation(1)
   , m_interactions(FALSE)
   , m_outputFile(_T(""))
   {

}

SensitivityDlg::~SensitivityDlg()
{
}

void SensitivityDlg::DoDataExchange(CDataExchange* pDX)
{
CDialogEx::DoDataExchange(pDX);
DDX_Text(pDX, IDC_PERTURBATION, m_perturbation);
DDX_Check(pDX, IDC_INTERACTIONS, m_interactions);
DDX_Text(pDX, IDC_OUTPUT, m_outputFile);
DDX_Control(pDX, IDC_SCENARIOS, m_scenarios);
//DDX_Control(pDX, IDC_VARS, m_modelVars);
DDX_Control(pDX, IDC_POLICIES, m_policies);
   }


BEGIN_MESSAGE_MAP(SensitivityDlg, CDialogEx)
   ON_CBN_SELCHANGE(IDC_SCENARIOS, &SensitivityDlg::OnCbnSelchangeScenarios)
END_MESSAGE_MAP()


// SensitivityDlg message handlers


BOOL SensitivityDlg::OnInitDialog()
   {
   CDialogEx::OnInitDialog();

   m_outputFile = PathManager::GetPath( PM_PROJECT_DIR  );  // includes '/', this is the envx file directory

   // load scenario combo
   int count = gpScenarioManager->GetCount();

   for ( int i=0; i < count; i++ )   
      m_scenarios.AddString( gpScenarioManager->GetScenario( i )->m_name );

   m_scenarios.SetCurSel( gpScenarioManager->GetDefaultScenario() );

   m_modelVarGrid.AttachGrid( this, IDC_VARS );
   m_modelVarGrid.m_pParent = this;
   m_modelVarGrid.ShowWindow( SW_SHOW );

   
   // add model vars to modelVarGrid

   //========================== METAGOAL SETTINGS ============================
   int metagoalCount = gpModel->GetMetagoalCount();
   for ( int i=0; i < metagoalCount; i++ )
      {
      METAGOAL *pInfo = gpModel->GetMetagoalInfo( i );

      CString name( "META:" );
      name += pInfo->name;

      CString value;
      value.Format( "%f", pInfo->saValue );

      m_modelVarGrid.AppendRow( pInfo->useInSensitivity != 0, name, value, 10000+i );

      //int index = m_modelVars.AddString( name );
      //m_modelVars.SetItemData( index, 10000+i );
      }

   //====================== MODEL/PROCESS USAGE SETTINGS =====================
   int modelCount = gpModel->GetEvaluatorCount();
   for ( int i=0; i < modelCount; i++ )
	   {
      EnvEvaluator *pModel = gpModel->GetEvaluatorInfo( i );
      MODEL_VAR *modelVarArray = NULL;
      int varCount = pModel->InputVar( pModel->m_id, &modelVarArray );

      // for each variable exposed by the model, add it to the possible scenario variables
      for ( int j=0; j < varCount; j++ )
         {
         MODEL_VAR &mv = modelVarArray[ j ];

         if ( mv.pVar != NULL )
            {
            CString name;
            name.Format( "%s.%s", (LPCTSTR) pModel->m_name, (LPCTSTR) mv.name );
   
            CString value;
            mv.saValue.GetAsString( value );
            m_modelVarGrid.AppendRow( mv.useInSensitivity != 0, name, value, 20000 + i*100 + j );
            //int index = m_modelVars.AddString( name ); 
            //m_modelVars.SetItemData( index, 20000 + i*100 + j );
            }
         }
      }
   
   //================= A U T O N O U S  P R O C E S S   V A R I A B L E S ==================
   int apCount = gpModel->GetModelProcessCount();

   for ( int i=0; i < apCount; i++ )
	   {
      EnvModelProcess *pModel = gpModel->GetModelProcessInfo( i );
      
      MODEL_VAR *modelVarArray = NULL;
      int varCount = pModel->InputVar( pModel->m_id, &modelVarArray );

      // for each variable exposed by the model, add it to the possible scenario variables
      for ( int j=0; j < varCount; j++ )
         {
         MODEL_VAR &mv = modelVarArray[ j ];

         if ( mv.pVar != NULL )
            {
            CString name;
            name.Format( "%s.%s", (LPCTSTR) pModel->m_name, (LPCTSTR) mv.name );

            CString value;
            mv.saValue.GetAsString( value );
            m_modelVarGrid.AppendRow( mv.useInSensitivity != 0, name, value, 30000 + i*100 + j );
            //int index = m_modelVars.AddString( name ); 
            //m_modelVars.SetItemData( index, 30000 + i*100 + j );
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
      for( int i=0; i < apVarCount; i++ )
	      {
         AppVar *pVar = gpModel->GetAppVar( i );

         if ( pVar->m_avType == AVT_APP )
            {
            CString name( "APPVAR." );
            name += pVar->m_name;

            CString value;
            pVar->m_saValue.GetAsString( value );
            m_modelVarGrid.AppendRow( pVar->m_useInSensitivity != 0, name, value, 40000 + i );
            //int index = m_modelVars.AddString( name );
            //m_modelVars.SetItemData( index, 40000 + i );
            }
         }
      }
   
   // load all existing policies from policy manager
   for ( int i=0; i < gpPolicyManager->GetPolicyCount(); i++ )
      m_policies.AddString( gpPolicyManager->GetPolicy( i )->m_name );
   
   m_policies.SetCurSel( -1 );

   OnCbnSelchangeScenarios();

   
   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


void SensitivityDlg::OnOK()
   {
   // TODO: Add your specialized code here and/or call the base class

   CDialogEx::OnOK();
   }


void SensitivityDlg::OnCbnSelchangeScenarios()
   {

   // get current scenario and load policy checkbox settings
   }





//==============================================================================================
//===================================  ModelVarGrid ================================================
//==============================================================================================

BEGIN_MESSAGE_MAP(ModelVarGrid,CUGCtrl)
	//{{AFX_MSG_MAP(ModelVarGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Standard ModelVarGrid construction/destruction
ModelVarGrid::ModelVarGrid()
: CUGCtrl()
, m_isEditable( true )
{ }


ModelVarGrid::~ModelVarGrid()
   {
   }



/////////////////////////////////////////////////////////////////////////////
//	OnSetup
//		This function is called just after the grid window 
//		is created or attached to a dialog item.
//		It can be used to initially setup the grid
void ModelVarGrid::OnSetup()
   {
   //EnableMenu(TRUE);
   //m_ellipsisIndex = AddCellType( &m_ellipsisCtrl );

   SetNumberCols( 3 );  // use, name, value

   CUGCell cell;
   cell.SetText( _T("Use" ) );
   cell.SetAlignment( UG_ALIGNCENTER | UG_ALIGNVCENTER );
   cell.SetBackColor( RGB( 220,220,220 ) );
   SetCell( 0, -1, &cell );

   cell.SetText( _T("Variable" ) );
   cell.SetAlignment( UG_ALIGNLEFT | UG_ALIGNVCENTER );
   SetCell( 1, -1, &cell );

   cell.SetText( _T("Perturbed Value") );
   cell.SetAlignment( UG_ALIGNCENTER | UG_ALIGNVCENTER );
   SetCell( 2, -1, &cell );

   SetSH_Width( 0 ) ;

   RECT rect;
   this->GetClientRect( &rect );

   int tableWidth = rect.right - rect.left;

   SetColWidth( 0, 30 );
   SetColWidth( 1, 400 );
   SetColWidth( 2, tableWidth-400-40 );
   }


int ModelVarGrid::AppendRow( bool check, LPCTSTR varName, LPCTSTR value, int extra )
   {
   CUGCtrl::AppendRow();

   int row = this->GetNumberRows() - 1;

   // set up col 0 (checkbox)
   CUGCell cell0;
   GetCell( 0, row, &cell0); 
   cell0.SetCellType(UGCT_CHECKBOX); 
   cell0.SetBool( check ? TRUE : FALSE );
   cell0.SetLabelText( "" ); 
   cell0.SetAlignment( UG_ALIGNCENTER | UG_ALIGNVCENTER );
   cell0.SetParam( CELLTYPE_IS_EDITABLE );
   SetCell(0, row, &cell0); 
   
   // col1 - variable name
   CUGCell cell1;
   GetCell( 1, row, &cell1 );
   cell1.SetText( varName );
   cell1.SetAlignment( UG_ALIGNLEFT | UG_ALIGNVCENTER );
   cell1.SetParam( extra );
   SetCell( 1, row, &cell1 );

   // col2 - variable value
   CUGCell cell2;
   GetCell( 2, row, &cell2 );
   cell2.SetText( value );
   cell2.SetAlignment( UG_ALIGNCENTER | UG_ALIGNVCENTER );
   cell2.SetParam( CELLTYPE_IS_EDITABLE );
   SetCell( 2, row, &cell2 );
   
   this->GotoCell( 2, row ); 

   RedrawRow( row );

   //m_pParent->MakeDirty();

   return row;
   }


void ModelVarGrid::OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed)
   {
	CUGCell cell;
	GetCell( col, row, &cell );
	
	int nParam = cell.GetParam();
	
   CString string;
	if(cell.GetLabelText() != NULL)
		string = cell.GetLabelText();

   //if(processed)
   //   {
   //	if(cell.GetCellType() == m_nSpinIndex)
   //		return ;
   //   }
	if( nParam == CELLTYPE_IS_EDITABLE || string == "CELLTYPE_IS_EDITABLE")
      {
		StartEdit();
   	}
   }


/////////////////////////////////////////////////////////////////////////////
//	OnColChange
//		Sent whenever the current column changes
//	Params:
//		oldcol - column that is loosing the focus
//		newcol - column that the user move into
//	Return:
//		<none>
void ModelVarGrid::OnColChange(int oldcol,int newcol)
{
	UNREFERENCED_PARAMETER(oldcol);
	UNREFERENCED_PARAMETER(newcol);
}

/////////////////////////////////////////////////////////////////////////////
//	OnRowChange
//		Sent whenever the current row changes
//	Params:
//		oldrow - row that is loosing the locus
//		newrow - row that user moved into
//	Return:
//		<none>
void ModelVarGrid::OnRowChange(long oldrow,long newrow)
{
	UNREFERENCED_PARAMETER(oldrow);
	UNREFERENCED_PARAMETER(newrow);
}

/////////////////////////////////////////////////////////////////////////////
//	OnCellChange
//		Sent whenever the current cell changes
//	Params:
//		oldcol, oldrow - coordinates of cell that is loosing the focus
//		newcol, newrow - coordinates of cell that is gaining the focus
//	Return:
//		<none>
void ModelVarGrid::OnCellChange(int oldcol,int newcol,long oldrow,long newrow)
{
	UNREFERENCED_PARAMETER(oldcol);
	UNREFERENCED_PARAMETER(newcol);
	UNREFERENCED_PARAMETER(oldrow);
	UNREFERENCED_PARAMETER(newrow);
}


/////////////////////////////////////////////////////////////////////////////
//	OnLClicked
//		Sent whenever the user clicks the left mouse button within the grid
//		this message is sent when the button goes down then again when the button goes up
//	Params:
//		col, row	- coordinates of a cell that received mouse click event
//		updn		- is TRUE if mouse button is 'down' and FALSE if button is 'up'
//		processed	- indicates if current event was processed by other control in the grid.
//		rect		- represents the CDC rectangle of cell in question
//		point		- represents the screen point where the mouse event was detected
//	Return:
//		<none>
void ModelVarGrid::OnLClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed)
{
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(updn);
	UNREFERENCED_PARAMETER(*rect);
	UNREFERENCED_PARAMETER(*point);
	UNREFERENCED_PARAMETER(processed);
}
		
/////////////////////////////////////////////////////////////////////////////
//	OnRClicked
//		Sent whenever the user clicks the right mouse button within the grid
//		this message is sent when the button goes down then again when the button goes up
//	Params:
//		col, row	- coordinates of a cell that received mouse click event
//		updn		- is TRUE if mouse button is 'down' and FALSE if button is 'up'
//		processed	- indicates if current event was processed by other control in the grid.
//		rect		- represents the CDC rectangle of cell in question
//		point		- represents the screen point where the mouse event was detected
//	Return:
//		<none>
void ModelVarGrid::OnRClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed)
{
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(updn);
	UNREFERENCED_PARAMETER(*rect);
	UNREFERENCED_PARAMETER(*point);
	UNREFERENCED_PARAMETER(processed);
}


/////////////////////////////////////////////////////////////////////////////
//	OnCellTypeNotify
//		This notification is sent by the celltype and it is different from cell-type
//		to celltype and even from notification to notification.  It is usually used to
//		provide the developer with some feed back on the cell events and sometimes to
//		ask the developer if given event is to be accepted or not
//	Params:
//		ID			- celltype ID
//		col, row	- co-ordinates cell that is processing the message
//		msg			- message ID to identify current process
//		param		- additional information or object that might be needed
//	Return:
//		TRUE - to allow celltype event
//		FALSE - to disallow the celltype event
int ModelVarGrid::OnCellTypeNotify(long ID,int col,long row,long msg,LONG_PTR param)
   {
	UNREFERENCED_PARAMETER(param);

   //if ( ID == m_ellipsisIndex )
   //   {
	//   CUGCell cell;
	//   GetCell(col,row,&cell);
	//   int nParam = cell.GetParam();
   //
	//   if( msg == UGCT_ELLIPSISBUTTONCLICK )
   //      {
   //      PolQueryDlg dlg( gpCellLayer );
   //      dlg.m_queryString = cell.GetText();
   //
   //      if ( dlg.DoModal() == IDOK )
   //         {
   //         this->QuickSetText( col, row, dlg.m_queryString );
   //         m_pParent->MakeDirty();
   //         }
	//	   }
	//   }

	return TRUE;
   }


/////////////////////////////////////////////////////////////////////////////
//	OnEditStart
//		This message is sent whenever the grid is ready to start editing a cell
//	Params:
//		col, row - location of the cell that edit was requested over
//		edit -	pointer to a pointer to the edit control, allows for swap of edit control
//				if edit control is swapped permanently (for the whole grid) is it better
//				to use 'SetNewEditClass' function.
//	Return:
//		TRUE - to allow the edit to start
//		FALSE - to prevent the edit from starting

int ModelVarGrid::OnEditStart(int col, long row,CWnd **edit)
   {
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(**edit);

   if ( ! m_isEditable )
      return FALSE;

   // set any needed masks

   // mask edit control will be set once a cell has mask string set
   // we always want the initials to be in uppercase, modify the style of the edit control itself  

   //if( col == 1 )
   //   (*edit)->ModifyStyle( 0, ES_UPPERCASE ); 
   //else 
   //   (*edit)->ModifyStyle( ES_UPPERCASE, 0 );

   return TRUE; 
   } 


/////////////////////////////////////////////////////////////////////////////
//	OnEditVerify
//		This notification is sent every time the user hits a key while in edit mode.
//		It is mostly used to create custom behavior of the edit control, because it is
//		so easy to allow or disallow keys hit.
//	Params:
//		col, row	- location of the edit cell
//		edit		-	pointer to the edit control
//		vcKey		- virtual key code of the pressed key
//	Return:
//		TRUE - to accept pressed key
//		FALSE - to do not accept the key
int ModelVarGrid::OnEditVerify(int col, long row,CWnd *edit,UINT *vcKey)
{
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(*edit);
	UNREFERENCED_PARAMETER(*vcKey);

   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//	OnEditFinish
//		This notification is sent when the edit is being finished
//	Params:
//		col, row	- coordinates of the edit cell
//		edit		- pointer to the edit control
//		string		- actual string that user typed in
//		cancelFlag	- indicates if the edit is being canceled
//	Return:
//		TRUE - to allow the edit to proceed
//		FALSE - to force the user back to editing of that same cell
int ModelVarGrid::OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag)
{
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(*edit);
	UNREFERENCED_PARAMETER(string);
	UNREFERENCED_PARAMETER(cancelFlag);
   //m_pParent->MakeDirty();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//	OnMenuCommand
//		This notification is called when the user has selected a menu item
//		in the pop-up menu.
//	Params:
//		col, row - the cell coordinates of where the menu originated from
//		section - identify for which portion of the gird the menu is for.
//				  possible sections:
//						UG_TOPHEADING, UG_SIDEHEADING,UG_GRID
//						UG_HSCROLL  UG_VSCROLL  UG_CORNERBUTTON
//		item - ID of the menu item selected
//	Return:
//		<none>

void ModelVarGrid::OnMenuCommand(int col,long row,int section,int item)
   {
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);

   if ( section == UG_GRID )
      {
      switch( item )
         {
         case 1000:
            CutSelected();
            break;

         case 2000:
            CopySelected();
            break;

         case 3000:     // paste
            Paste(); 
            break;
         }
      }
   } 

/////////////////////////////////////////////////////////////////////////////
//	OnHint
//		Is called whent the hint is about to be displayed on the main grid area,
//		here you have a chance to set the text that will be shown
//	Params:
//		col, row	- the cell coordinates of where the menu originated from
//		string		- pointer to the string that will be shown
//	Return:
//		TRUE - to show the hint
//		FALSE - to prevent the hint from showing
int ModelVarGrid::OnHint(int col,long row,int section,CString *string)
{
	UNREFERENCED_PARAMETER(section);
	string->Format( _T("Col:%d Row:%ld") ,col,row);
	return FALSE;
}


