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
// PPPolicyObjScores.cpp : implementation file
//

#include "stdafx.h"

#include "PolEditor.h"
#include "SandboxDlg.h"
#include ".\PPPolicyObjScores.h"

#include <Scenario.h>
#include <queryengine.h>

#include "PolQueryDlg.h"
#include "MapLayer.h"

extern PolicyManager   *gpPolicyManager;
extern ScenarioManager *gpScenarioManager;
extern MapLayer        *gpCellLayer;
//extern QueryEngine     *PolicyManager::m_pQueryEngine;
extern EnvModel *gpModel;


//#include <colors.hpp>
//==============================================================================================
//===================================  ScoreGrid ================================================
//==============================================================================================

BEGIN_MESSAGE_MAP(ScoreGrid,CUGCtrl)
	//{{AFX_MSG_MAP(ScoreGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Standard ScoreGrid construction/destruction
ScoreGrid::ScoreGrid()
: CUGCtrl()
, m_isEditable( true )
{ }


ScoreGrid::~ScoreGrid()
   {
   }

int ScoreGrid::AppendRow()
   {
   CUGCtrl::AppendRow();

   int row = this->GetNumberRows() - 1;

   // set up col 0 (goal)
   CUGCell cell0;
   GetCell( 0, row, &cell0); 
   cell0.SetCellType(UGCT_DROPLIST); 
   cell0.SetText( _T("Global") );

   int metagoalCount = gpModel->GetMetagoalCount();
   CString goals("Global\n");
   for ( int i=0; i < metagoalCount; i++ )
      {
      goals += gpModel->GetMetagoalInfo( i )->name;
      goals += "\n";
      }
   cell0.SetLabelText( goals ); 
   SetCell(0, row, &cell0); 

   CUGCell cell1;
   GetCell( 1, row, &cell1 );
   cell1.SetText( _T("0") );
   cell1.SetAlignment( UG_ALIGNCENTER | UG_ALIGNVCENTER );
   cell1.SetParam( CELLTYPE_IS_EDITABLE );
   this->SetCell( 1, row, &cell1 );

   CUGCell cell2;
   GetCell( 2, row, &cell2 );
   cell2.SetCellType( m_ellipsisIndex );
   cell2.SetText( _T("<Specify query or blank for everywhere>" ) );
   cell2.SetAlignment( UG_ALIGNLEFT | UG_ALIGNVCENTER ); 
	cell2.SetParam(CELLTYPE_IS_EDITABLE);
   SetCell( 2, row, &cell2 );
   
   this->GotoCell( 2, row ); 

   RedrawRow( row );

   m_pParent->MakeDirty();

   return row;
   }


void ScoreGrid::OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed)
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
//	OnSetup
//		This function is called just after the grid window 
//		is created or attached to a dialog item.
//		It can be used to initially setup the grid
void ScoreGrid::OnSetup()
   {
   //EnableMenu(TRUE);
   m_ellipsisIndex = AddCellType( &m_ellipsisCtrl );

   SetNumberCols( 3 );  // goal, scores, criteria

   CUGCell cell;
   cell.SetText( _T("Goal" ) );
   cell.SetBackColor( RGB( 220,220,220 ) );
   SetCell( 0, -1, &cell );

   cell.SetText( _T("Score" ) );
   cell.SetBackColor( RGB( 220,220,220 ) );
   SetCell( 1, -1, &cell );

   cell.SetText( _T("Criteria (Query)") );
   SetCell( 2, -1, &cell );

   SetSH_Width( 0 ) ;

   SetColWidth( 0, 100 );
   SetColWidth( 1, 60 );
   SetColWidth( 2, 400 );
   }


/////////////////////////////////////////////////////////////////////////////
//	OnColChange
//		Sent whenever the current column changes
//	Params:
//		oldcol - column that is loosing the focus
//		newcol - column that the user move into
//	Return:
//		<none>
void ScoreGrid::OnColChange(int oldcol,int newcol)
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
void ScoreGrid::OnRowChange(long oldrow,long newrow)
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
void ScoreGrid::OnCellChange(int oldcol,int newcol,long oldrow,long newrow)
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
void ScoreGrid::OnLClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed)
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
void ScoreGrid::OnRClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed)
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
int ScoreGrid::OnCellTypeNotify(long ID,int col,long row,long msg,LONG_PTR param)
   {
	UNREFERENCED_PARAMETER(param);

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

int ScoreGrid::OnEditStart(int col, long row,CWnd **edit)
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
int ScoreGrid::OnEditVerify(int col, long row,CWnd *edit,UINT *vcKey)
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
int ScoreGrid::OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag)
{
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(*edit);
	UNREFERENCED_PARAMETER(string);
	UNREFERENCED_PARAMETER(cancelFlag);
   m_pParent->MakeDirty();
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

void ScoreGrid::OnMenuCommand(int col,long row,int section,int item)
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
int ScoreGrid::OnHint(int col,long row,int section,CString *string)
{
	UNREFERENCED_PARAMETER(section);
	string->Format( _T("Col:%d Row:%ld") ,col,row);
	return TRUE;
}




//==============================================================================================
//===================================  ModifierGrid ================================================
//==============================================================================================

BEGIN_MESSAGE_MAP(ModifierGrid,CUGCtrl)
	//{{AFX_MSG_MAP(ModifierGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Standard ModifierGrid construction/destruction
ModifierGrid::ModifierGrid()
: CUGCtrl()
, m_isEditable( true )
{ }


ModifierGrid::~ModifierGrid()
   {
   }

int ModifierGrid::AppendRow()
   {
   CUGCtrl::AppendRow();

   int row = this->GetNumberRows() - 1;

   // set up col 0 (goal)
   CUGCell cell0;
   GetCell( 0, row, &cell0); 
   cell0.SetCellType(UGCT_DROPLIST); 
   cell0.SetText( _T("Global") );

   int metagoalCount = gpModel->GetMetagoalCount();
   CString goals("Global\n");
   for ( int i=0; i < metagoalCount; i++ )
      {
      goals += gpModel->GetMetagoalInfo( i )->name;
      goals += "\n";
      }
   cell0.SetLabelText( goals ); 
   SetCell(0, row, &cell0); 

   // set up col 1 (type)
   CUGCell cell1;
   GetCell( 1, row, &cell1); 
   cell1.SetCellType(UGCT_DROPLIST); 
   cell1.SetText( _T("Increment") );
   cell1.SetLabelText("Scenario\nIncrement\nDecrement\n"); 
   SetCell(1, row, &cell1); 

   // col 2 (modifier)
   CUGCell cell2;
   GetCell( 2, row, &cell2 );
   cell2.SetText( _T("0") );
   cell2.SetAlignment( UG_ALIGNCENTER | UG_ALIGNVCENTER );
   cell2.SetParam( CELLTYPE_IS_EDITABLE );
   this->SetCell( 2, row, &cell2 );

   // col 3 (query)
   CUGCell cell3;
   GetCell( 3, row, &cell3 );
   cell3.SetCellType( m_ellipsisIndex );
   cell3.SetText( _T("<Specify query or blank for everywhere>" ) );
   cell3.SetAlignment( UG_ALIGNLEFT | UG_ALIGNVCENTER ); 
 	cell3.SetParam(CELLTYPE_IS_EDITABLE);
   SetCell( 3, row, &cell3 );

   this->GotoCell( 2,row ); 
   RedrawRow( row );

   m_pParent->MakeDirty();
   return row;
   }



/////////////////////////////////////////////////////////////////////////////
//	OnSetup
//		This function is called just after the grid window 
//		is created or attached to a dialog item.
//		It can be used to initially setup the grid
void ModifierGrid::OnSetup()
   {
   //EnableMenu(TRUE);
   m_ellipsisIndex = AddCellType( &m_ellipsisCtrl ); 
   
   SetNumberCols( 4 );  // goal (dropdown), type (dropdown), value (float), criteria (scenario or query)
  
   // set headers
   CUGCell cell;
   cell.SetText( _T("Goal" ) );
   cell.SetBackColor( RGB( 220,220,220 ) );
   SetCell( 0, -1, &cell );

   cell.SetText( _T("Type" ) );
   cell.SetBackColor( RGB( 220,220,220 ) );
   SetCell( 1, -1, &cell );

   cell.SetText( _T("Score Modifier") );
   SetCell( 2, -1, &cell );

   cell.SetText( _T("Criteria (Query)") );  // or scenario
   SetCell( 3, -1, &cell );

   SetSH_Width(0) ;

   SetColWidth( 0, 100 );
   SetColWidth( 1, 100 );
   SetColWidth( 2, 100 );
   SetColWidth( 3, 260 );
   }


void ModifierGrid::OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed)
   {
	CUGCell cell;
	GetCell( col, row, &cell );
	
	int nParam = cell.GetParam();
	
   CString string;
	if( cell.GetLabelText() != NULL )
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
void ModifierGrid::OnColChange(int oldcol,int newcol)
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
void ModifierGrid::OnRowChange(long oldrow,long newrow)
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
void ModifierGrid::OnCellChange(int oldcol,int newcol,long oldrow,long newrow)
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
void ModifierGrid::OnLClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed)
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
void ModifierGrid::OnRClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed)
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
int ModifierGrid::OnCellTypeNotify(long ID,int col,long row,long msg,LONG_PTR param)
   {
	UNREFERENCED_PARAMETER(param);

   if ( ID == UGCT_DROPLIST && msg == UGCT_DROPLISTSELECT )
      {
      if ( col == 1 )  
         {
         CString *selStr = (CString*) param;

         switch( selStr->GetAt( 0 ) )
            {
            case 'S': 
               {
               CUGCell cell;
               GetCell( 3, row, &cell );  // conditions
               cell.SetCellType( UGCT_DROPLIST );

               // construct scenario list 
               CString scenarios;
               for ( int i=0; i < gpScenarioManager->GetCount(); i++ )
                  {
                  scenarios += gpScenarioManager->GetScenario( i )->m_name;
                  scenarios += "\n";
                  }
               cell.SetLabelText( scenarios );
               cell.SetText( gpScenarioManager->GetScenario( 0 )->m_name );
               SetCell( 3, row, &cell );
               RedrawRow( row );
               m_pParent->MakeDirty();
               }
               break;

            case 'I':
            case 'D':  // Increment or Decrement selected
               {
               CUGCell cell;
               GetCell( 3, row, &cell );
               cell.SetCellType( m_ellipsisIndex );
               //cell.SetText( _T("0") );
               cell.SetAlignment( UG_ALIGNLEFT | UG_ALIGNVCENTER );
               cell.SetParam( CELLTYPE_IS_EDITABLE );
               SetCell( 3, row, &cell );
               RedrawRow( row );
               m_pParent->MakeDirty();
               }
               break;
            }   // end of: switch selStr[ 0 ];
         }  // end of: if col == 0 

      else if ( col == 3 )  // scenario selection
         {
         /*CString *selStr = (CString*) param;

         int scenarioIndex = atoi( *selStr );
         CString scenarioIndexStr;
         scenarioIndexStr.Format( "%i", scenarioIndex );

         this->QuickSetText( 1, row, scenarioIndexStr );
         this->QuickSetText( 2, row, gpScenarioManager->GetScenario( scenarioIndex )->m_name ); */
         return TRUE;
         }
      }

   else if ( ID == m_ellipsisIndex )
      {
      ASSERT( col == 3 );
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
            RedrawRow( row );
            m_pParent->MakeDirty();
            }
		   }
	   }

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

int ModifierGrid::OnEditStart(int col, long row,CWnd **edit)
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
int ModifierGrid::OnEditVerify(int col, long row,CWnd *edit,UINT *vcKey)
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
int ModifierGrid::OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag)
{
	UNREFERENCED_PARAMETER(col);
	UNREFERENCED_PARAMETER(row);
	UNREFERENCED_PARAMETER(*edit);
	UNREFERENCED_PARAMETER(string);
	UNREFERENCED_PARAMETER(cancelFlag);
   m_pParent->MakeDirty();
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

void ModifierGrid::OnMenuCommand(int col,long row,int section,int item)
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
int ModifierGrid::OnHint(int col,long row,int section,CString *string)
{
	UNREFERENCED_PARAMETER(section);
	string->Format( _T("Col:%d Row:%ld") ,col,row);
	return TRUE;
}




// PPPolicyObjScores dialog

IMPLEMENT_DYNAMIC(PPPolicyObjScores, CTabPageSSL)

PPPolicyObjScores::PPPolicyObjScores( PolEditor *pParent, Policy *&pPolicy)
	: CTabPageSSL()
   , m_pPolicy( pPolicy )
   , m_pParent( pParent )
{ }

PPPolicyObjScores::~PPPolicyObjScores()
{ }

void PPPolicyObjScores::DoDataExchange(CDataExchange* pDX)
{
CTabPageSSL::DoDataExchange(pDX);
//DDX_Control(pDX, IDC_OBJECTIIVES, m_objectives);
   }

BEGIN_MESSAGE_MAP(PPPolicyObjScores, CTabPageSSL)
   ON_WM_DESTROY()
  
   //ON_CBN_SELCHANGE(IDC_OBJECTIIVES, &PPPolicyObjScores::OnCbnSelchangeObjectiives)
   ON_BN_CLICKED(IDC_ADD, &PPPolicyObjScores::OnBnClickedAdd)
   ON_BN_CLICKED(IDC_REMOVE, &PPPolicyObjScores::OnBnClickedRemove)
   ON_BN_CLICKED(IDC_MODADD, &PPPolicyObjScores::OnBnClickedModadd)
   ON_BN_CLICKED(IDC_MODREMOVE, &PPPolicyObjScores::OnBnClickedModremove)
END_MESSAGE_MAP()


// PPPolicyObjScores message handlers

void PPPolicyObjScores::OnDestroy()
   {
   CTabPageSSL::OnDestroy();
   }


BOOL PPPolicyObjScores::OnInitDialog() 
   {
   CTabPageSSL::OnInitDialog();

   m_scoreGrid.AttachGrid( this, IDC_SCOREFRAME );
   m_modifierGrid.AttachGrid( this, IDC_MODFRAME );

   m_scoreGrid.m_pParent = this;
   m_modifierGrid.m_pParent = this;

   m_scoreGrid.ShowWindow( SW_SHOW );
   m_modifierGrid.ShowWindow( SW_SHOW );

   if ( m_pPolicy )
      LoadPolicy();

	return TRUE;
   }


void PPPolicyObjScores::LoadPolicy()
   {
   BOOL enabled = FALSE;
   if ( m_pParent->IsEditable() )
      enabled = TRUE;

   m_scoreGrid.m_isEditable = m_pParent->IsEditable();
   m_modifierGrid.m_isEditable = m_pParent->IsEditable();

   ClearObjectives();

   int objectiveCount = (int) m_pPolicy->m_objectiveArray.GetSize();
   for ( int i=0; i < objectiveCount; i++ ) 
      LoadObjective( i );
   }

void PPPolicyObjScores::ClearObjectives()
   {
  int rows = m_scoreGrid.GetNumberRows();
   for ( int i=rows-1; i >= 0; i-- )
      m_scoreGrid.DeleteRow( i );
   
   rows = m_modifierGrid.GetNumberRows();
   for ( int i=rows-1; i >= 0; i-- )
      m_modifierGrid.DeleteRow( i );
   }


void PPPolicyObjScores::LoadObjective( int objective )
   {
   ObjectiveScores *pScores = m_pPolicy->m_objectiveArray.GetAt( objective );

   CString goal = pScores->m_objectiveName;

   // first, scores
   GoalScoreArray &gsa = pScores->m_goalScoreArray;

   for ( int j=0; j < gsa.GetSize(); j++ )
      {
      GOAL_SCORE &gs = gsa.GetAt( j );
      int row = m_scoreGrid.AppendRow();

      CString score;
      score.Format( "%g", gs.score );

      m_scoreGrid.QuickSetText( 0, row, goal );
      m_scoreGrid.QuickSetText( 1, row, score );
      m_scoreGrid.QuickSetText( 2, row, gs.queryStr );
      }

   // next, modifiers
   ModifierArray &ma = pScores->m_modifierArray;
   for ( int j=0; j < ma.GetSize(); j++ )
      {
      SCORE_MODIFIER &sm = ma.GetAt( j );
      int row = m_modifierGrid.AppendRow();

      CString value;
      value.Format( "%g", sm.value );
      
      CString type;
      CString condition;
      switch( sm.type )
         {
         case SM_SCENARIO:
            {
            if ( sm.scenario < 0 )
               sm.scenario = 0;

            // add 4/15/12 (jpb) - on OpnDocXML(), scenarios haven't been loaded yet
            if ( sm.scenario >= gpScenarioManager->GetCount() )
               break;               

            type = "Scenario";
            Scenario *pScenario =  gpScenarioManager->GetScenario( sm.scenario );

            if ( pScenario == NULL )
               {
               AfxMessageBox( "Scenario not found, setting to default scenario" );
               sm.scenario = gpScenarioManager->GetDefaultScenario();
               pScenario = gpScenarioManager->GetScenario( sm.scenario );
               }

            condition = pScenario->m_name;
            }
            break;

         case SM_INCR:
            type = "Increment";
            condition = sm.condition;
            break;

         case SM_DECR:
            type = "Decrement";
            condition = sm.condition;
            break;

         default:
            ASSERT( 0 );
         }

      m_modifierGrid.QuickSetText( 0, row, goal );
      m_modifierGrid.QuickSetText( 1, row, type );
      m_modifierGrid.QuickSetText( 2, row, value );
      m_modifierGrid.QuickSetText( 3, row, condition );
      }  // end of modifiers

   m_modifierGrid.RedrawAll();
   m_scoreGrid.RedrawAll();
   }
   

bool PPPolicyObjScores::StoreObjChanges()
   {
   UpdateData( TRUE );  // update local variables

   int objectiveCount = (int) m_pPolicy->m_objectiveArray.GetSize();

   for ( int i=0; i < objectiveCount; i++ )
      {
      ObjectiveScores *pScores = m_pPolicy->m_objectiveArray.GetAt( i );
      pScores->Clear();
      }

   // first, update scores
   //pScores->Clear();

   for ( int i=0; i < m_scoreGrid.GetNumberRows(); i++ )
      {
      CString goal, score, query;

      m_scoreGrid.QuickGetText( 0, i, &goal );
      m_scoreGrid.QuickGetText( 1, i, &score );
      m_scoreGrid.QuickGetText( 2, i, &query );

      if ( score.IsEmpty() && query.IsEmpty() )
         continue;

      if ( score.IsEmpty() )
         {
         Report::ErrorMsg( _T("Score missing - set to zero" ) );
         score = _T( "0" );
         }

      float _score = (float) atof( score );

      // make sure query is legit
      Query *pQuery = gpPolicyManager->m_pQueryEngine->ParseQuery( query, 0, "Policy Objectives Scores Setup" );
      if ( pQuery == NULL )
         Report::ErrorMsg( _T("Invalid query encountered - the query will be ignored" ) );

      gpPolicyManager->m_pQueryEngine->RemoveQuery( pQuery );

      // find goal
      ObjectiveScores *pScores = m_pPolicy->FindObjective( goal );
      ASSERT ( pScores != NULL );
      pScores->AddScore( _score, query );
      }

   // get modifiers next
   for ( int i=0; i < m_modifierGrid.GetNumberRows(); i++ )
      {
      CString goal, type, value, condition;
      m_modifierGrid.QuickGetText( 0, i, &goal );
      m_modifierGrid.QuickGetText( 1, i, &type );
      m_modifierGrid.QuickGetText( 2, i, &value );
      m_modifierGrid.QuickGetText( 3, i, &condition );

      if ( type.IsEmpty() && value.IsEmpty() && condition.IsEmpty() )
         continue;

      if ( value.IsEmpty() )
         {
         Report::ErrorMsg( _T("Value missing - set to zero" ) );
         value = _T( "0" );
         }

      SM_TYPE _type = SM_SCENARIO;
      if ( type[0] == 'I' )
         _type = SM_INCR;
      else if ( type[ 0 ] == 'D' )
         _type = SM_DECR;

      float _value = (float) atof( value );

      int scenario = -1;
      if ( _type == SM_SCENARIO )
         {
         for ( int j=0; j < gpScenarioManager->GetCount(); j++ )
            {
            if ( condition == gpScenarioManager->GetScenario( j )->m_name )
               {
               scenario = j;
               break;
               }
            }

         if ( scenario == -1 )
            {
            CString msg( _T("Unable to find scenario: " ));
            msg += condition;
            AfxMessageBox( msg );
            return false;
            }
         }

      // make sure query is legit
      if ( _type != SM_SCENARIO )
         {
         Query *pQuery = gpPolicyManager->m_pQueryEngine->ParseQuery( condition, 0, "Policy Modifier Setup" );
         if ( pQuery == NULL )
            {
            CString msg( _T("Invalid condition encountered [" ));
            msg += condition;
            msg += _T("] - the condition will be ignored" );
            Report::ErrorMsg( msg );
            }

         gpPolicyManager->m_pQueryEngine->RemoveQuery( pQuery );
         }
      
      // find goal
      ObjectiveScores *pScores = m_pPolicy->FindObjective( goal );
      ASSERT ( pScores != NULL );
      pScores->AddModifier( _type, _value, condition, scenario );
      }

   return true;
   }


bool PPPolicyObjScores::StoreChanges()
   {
   if( StoreObjChanges() == false )
      return false;

   //StoreChanges();
   /*
   if ( m_pParent->IsDirty( DIRTY_SCORES ) )
      {
      StoreObjChanges();
      m_pParent->MakeClean( DIRTY_SCORES );
      }
   */
   m_pParent->MakeClean( DIRTY_SCORES );
   return true;
   }


void PPPolicyObjScores::MakeDirty( void )
   {
   m_pParent->MakeDirty( DIRTY_SCORES );
   }


void PPPolicyObjScores::OnBnClickedAdd()
   {
   m_scoreGrid.AppendRow();
   MakeDirty();
   }


void PPPolicyObjScores::OnBnClickedRemove()
   {
   long row = m_scoreGrid.GetCurrentRow();
   if ( row >= 0 )
      {
      int retVal = MessageBox( _T("Delete current row?" ), _T("Delete"), MB_YESNO );

      if ( retVal == IDYES )
         {
         m_scoreGrid.DeleteRow( row );
         m_scoreGrid.RedrawRow( m_scoreGrid.GetCurrentRow() );
         MakeDirty();
         }
      }
   else
      {
      MessageBeep( 0 );
      }
   }

void PPPolicyObjScores::OnBnClickedModadd()
   {
   m_modifierGrid.AppendRow();
   MakeDirty();
   }

void PPPolicyObjScores::OnBnClickedModremove()
   {
   long row = m_modifierGrid.GetCurrentRow();
   if ( row >= 0 )
      {
      int retVal = MessageBox( _T("Delete current row?" ), _T("Delete"), MB_YESNO );

      if ( retVal == IDYES )
         {
         m_modifierGrid.DeleteRow( row );
         m_modifierGrid.RedrawRow( m_modifierGrid.GetCurrentRow() );
         MakeDirty();
         }
      }
   else
      {
      MessageBeep( 0 );
      } 
   }
