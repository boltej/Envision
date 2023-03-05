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
// ScenarioGridDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "ScenarioGridDlg.h"
#include "afxdialogex.h"

#include <Scenario.h>
#include <EnvPolicy.h>

extern ScenarioManager *gpScenarioManager;
extern PolicyManager   *gpPolicyManager;


//==============================================================================================
//===================================  ScenarioGrid ================================================
//==============================================================================================

BEGIN_MESSAGE_MAP(ScenarioGrid,CUGCtrl)
	//{{AFX_MSG_MAP(ScenarioGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Standard ScenarioGrid construction/destruction
ScenarioGrid::ScenarioGrid()
: CUGCtrl()
, m_isEditable( true )
{ }


ScenarioGrid::~ScenarioGrid()
   {
   }


int ScenarioGrid::AddPolicy( EnvPolicy *pPolicy )
   {
   CUGCtrl::AppendRow();

   int row = this->GetNumberRows() - 1;

   this->QuickSetText( 0, row, pPolicy->m_name );
   
   for ( int i=0; i < gpScenarioManager->GetCount(); i++ )
      {
      Scenario *pScenario = gpScenarioManager->GetScenario( i );

      BOOL use = pScenario->GetPolicyInfo( pPolicy->m_name )->inUse;

      // set up col 0 (goal)
      CUGCell cell;
      GetCell( i+1, row, &cell); 
      cell.SetCellType(UGCT_CHECKBOXCHECKMARK ); 

/*      UGCT_CHECKBOXFLAT
UGCT_CHECKBOXCROSS
UGCT_CHECKBOX3DRECESS
UGCT_CHECKBOX3DRAISED
UGCT_CHECKBOXCHECKMARK
<code>UGCT_CHECKBOXROUND
UGCT_CHECKBOXUSEALIGN
UGCT_CHECKBOX3DSTATE
UGCT_CHECKBOXDISABLED

Notification #defines:
UGCT_CHECKBOXSET
*/
      cell.SetAlignment( UG_ALIGNCENTER | UG_ALIGNVCENTER );
      cell.SetParam( CELLTYPE_IS_EDITABLE );
      cell.SetBool( use );
      SetCell(i+1, row, &cell);
      }
   
   //this->GotoCell( 2, row ); 
   RedrawRow( row );

   m_pParent->m_isDirty = true;
   
   return row;
   }


void ScenarioGrid::OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed)
   {
/*	CUGCell cell;
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
   	} */
   }


/////////////////////////////////////////////////////////////////////////////
//	OnSetup
//		This function is called just after the grid window 
//		is created or attached to a dialog item.
//		It can be used to initially setup the grid
void ScenarioGrid::OnSetup()
   {
   //m_ellipsisIndex = AddCellType( &m_ellipsisCtrl );

   SetNumberCols( gpScenarioManager->GetCount() + 1 ); 

   CUGCell cell;
   cell.SetText( _T("Policy" ) );
   cell.SetBackColor( RGB( 220,220,220 ) );
   SetCell( 0, -1, &cell );
   SetColWidth( 0, 200 );

   for ( int i=0; i < gpScenarioManager->GetCount(); i++ )
      {
      cell.SetText( gpScenarioManager->GetScenario(i)->m_name );
      cell.SetBackColor( RGB( 220,220,220 ) );
      cell.SetAlignment( UG_ALIGNCENTER | UG_ALIGNVCENTER );
      SetCell( i+1, -1, &cell );
      SetColWidth( i+1, 100 );
      }

   SetSH_Width( 0 ) ;
   }


/////////////////////////////////////////////////////////////////////////////
//	OnColChange
//		Sent whenever the current column changes
//	Params:
//		oldcol - column that is loosing the focus
//		newcol - column that the user move into
//	Return:
//		<none>
void ScenarioGrid::OnColChange(int oldcol,int newcol)
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
void ScenarioGrid::OnRowChange(long oldrow,long newrow)
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
void ScenarioGrid::OnCellChange(int oldcol,int newcol,long oldrow,long newrow)
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
void ScenarioGrid::OnTH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed)
   {
	UNREFERENCED_PARAMETER(*rect);
	UNREFERENCED_PARAMETER(*point);
	UNREFERENCED_PARAMETER(processed);
   if ( row == -1 && updn == FALSE )
      {
      SortBy(col ); //, int flag = UG_SORT_ASCENDING);
	//int SortBy(int *cols,int num,int flag = UG_SORT_ASCENDING);

      }
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
void ScenarioGrid::OnLClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed)
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
void ScenarioGrid::OnRClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed)
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
int ScenarioGrid::OnCellTypeNotify(long ID,int col,long row,long msg,LONG_PTR param)
   {
	UNREFERENCED_PARAMETER(param);
   /*
   if ( ID == m_ellipsisIndex )
      {
	   CUGCell cell;
	   GetCell(col,row,&cell);
	   int nParam = cell.GetParam();

	   if( msg == UGCT_ELLIPSISBUTTONCLICK )
         {
         PolQueryDlg dlg( gpCellLayer );
         dlg.m_queryString = cell.GetText();

         if ( dlg.DoModal() == IDOK )
            {
            this->QuickSetText( col, row, dlg.m_queryString );
            m_pParent->MakeDirty();
            }
		   }
	   }
      */
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

int ScenarioGrid::OnEditStart(int col, long row,CWnd **edit)
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
int ScenarioGrid::OnEditVerify(int col, long row,CWnd *edit,UINT *vcKey)
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
int ScenarioGrid::OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag)
{
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(*edit);
	UNREFERENCED_PARAMETER(string);
	UNREFERENCED_PARAMETER(cancelFlag);
   m_pParent->m_isDirty = true;
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

void ScenarioGrid::OnMenuCommand(int col,long row,int section,int item)
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
int ScenarioGrid::OnHint(int col,long row,int section,CString *string)
{
	UNREFERENCED_PARAMETER(section);
	string->Format( _T("Col:%d Row:%ld") ,col,row);
	return TRUE;
}






// ScenarioGridDlg dialog

IMPLEMENT_DYNAMIC(ScenarioGridDlg, CDialog)

ScenarioGridDlg::ScenarioGridDlg(CWnd* pParent /*=NULL*/)
	: CDialog(ScenarioGridDlg::IDD, pParent)
, m_isDirty( false )
{

}

ScenarioGridDlg::~ScenarioGridDlg()
{
}

void ScenarioGridDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(ScenarioGridDlg, CDialog)
   ON_WM_SIZE()
END_MESSAGE_MAP()


// ScenarioGridDlg message handlers


BOOL ScenarioGridDlg::OnInitDialog() 
   {
   m_scenarioGrid.AttachGrid( this, IDC_PLACEHOLDER );
   m_scenarioGrid.m_pParent = this;

   for ( int i=0; i < gpPolicyManager->GetPolicyCount(); i++ )
      {
      EnvPolicy *pPolicy = gpPolicyManager->GetPolicy( i );
      m_scenarioGrid.AddPolicy( pPolicy );
      }

   m_scenarioGrid.ShowWindow( SW_SHOW );
	return TRUE;
   }



void ScenarioGridDlg::OnSize(UINT nType, int cx, int cy)
   {
   CDialog::OnSize(nType, cx, cy);
   if ( m_scenarioGrid.m_hWnd != 0 )
      m_scenarioGrid.MoveWindow( 5,5, cx-10, cy-10, TRUE );
   }
