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
// FieldCalculatorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "FieldCalculatorDlg.h"
#include "envdoc.h"
#include "MapPanel.h"
#include <map.h>
#include <maplayer.h>
#include <mtparser/mtparser.h>

extern MapPanel *gpMapPanel;
extern CEnvDoc  *gpDoc;

// FieldCalculatorDlg dialog

IMPLEMENT_DYNAMIC(FieldCalculatorDlg, CDialog)

FieldCalculatorDlg::FieldCalculatorDlg(CWnd* pParent /*=NULL*/)
	: CDialog(FieldCalculatorDlg::IDD, pParent)
   { }


FieldCalculatorDlg::~FieldCalculatorDlg()
{ }


void FieldCalculatorDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_LAYERS, m_layers);
DDX_Control(pDX, IDC_FIELDS, m_fields);
DDX_Control(pDX, IDC_FORMULA, m_formula);
DDX_Control(pDX, IDC_FIELDS1, m_fields1);
DDX_Control(pDX, IDC_FUNCTIONS, m_functions);
DDX_Control(pDX, IDC_OPERATORS, m_operators);
DDX_Control(pDX, IDC_QUERY, m_query);
}


BEGIN_MESSAGE_MAP(FieldCalculatorDlg, CDialog)
   ON_CBN_SELCHANGE(IDC_LAYERS, &FieldCalculatorDlg::OnCbnSelchangeLayers)
   ON_LBN_DBLCLK(IDC_FIELDS1, &FieldCalculatorDlg::OnLbnDblclkFields1)
   ON_LBN_DBLCLK(IDC_FUNCTIONS, &FieldCalculatorDlg::OnLbnDblclkFunctions)
   ON_LBN_DBLCLK(IDC_OPERATORS, &FieldCalculatorDlg::OnLbnDblclkOps)
END_MESSAGE_MAP()


struct OP { LPCTSTR op; LPCTSTR descr; };
OP opArray[ ] = {
   { "+", "Addition" },
   { "-",  "Subtraction and unary minus" },
   { "*",  "Multiplication" },
   { "/",  "Division" },
   { "^",  "Power" },
   { "%",  "Modulo" },
   { "&",  "Logical AND" },
   { "|",  "Logical OR" },
   { "!",  "Logical NOT" },
   { ">",  "Greater than" },
   { ">=",  "Greater or equal to" },
   { "<",  "Less than" },
   { "<=",  "Less than or equal to" },
   { "!=",  "Not equal to" },
   { "==",  "Equal to" },
   { NULL, NULL } };

struct FN { LPCTSTR fn; LPCTSTR descr; };
FN fnArray[ ] = {
   { "abs()", "" },
   { "acos()", "" },
   { "asin()", "" },
   { "atan()", "" },
   { "avg(x,y,z,…)", "" },
   { "bin()", "" },
   { "ceil()", "" },
   { "cos()", "" },
   { "floor()", "" },
   { "hex()", "" },
   { "if", "" },
   { "isNaN()", "" },
   { "log()", "" },
   { "log10()", "" },
   { "max(x,y,z,…)", "" },
   { "min(x,y,z,…)", "" },
   { "rand()", "" },
   { "rand(min, max)", "" },
   { "round()", "" },
   { "sin()", "" },
   { "sinh()", "" },
   { "sqrt()", "" },
   { "sum(x,y,z,…)", "" },
   { "tan()", "" },
   { "tanh()", "" },
   { NULL, NULL } };
 


// FieldCalculatorDlg message handlers

BOOL FieldCalculatorDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   // load layers combo
   Map *pMap = gpMapPanel->m_pMap;

   int layerCount = pMap->GetLayerCount();
   for ( int i=0; i < layerCount; i++ )
      {
      MapLayer *pLayer = pMap->GetLayer( i );
      m_layers.AddString( pLayer->m_name );
      }

   if ( layerCount > 0 )
      {
      m_layers.SetCurSel( 0 );
      m_pLayer = pMap->GetLayer( 0 );
      }
   else
      {
      m_layers.SetCurSel( -1 );
      m_pLayer = NULL;
      }

   LoadFieldInfo();

   int i = 0;
   while( opArray[ i ].op != NULL )
      m_operators.AddString( opArray[ i++ ].op );

   i = 0;
   while( fnArray[ i ].fn != NULL )
      m_functions.AddString( fnArray[ i++ ].fn );

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


void FieldCalculatorDlg::LoadFieldInfo()
   {
   int index = m_layers.GetCurSel();
   MapLayer *pLayer = gpMapPanel->m_pMap->GetLayer( index );

   m_fields.ResetContent();
   m_fields1.ResetContent();

   for ( int i=0; i < pLayer->GetFieldCount(); i++ )
      {
      TYPE type = pLayer->GetFieldType( i );

      m_fields.AddString( pLayer->GetFieldLabel( i ) );

      if ( ::IsNumeric( type ) )
         m_fields1.AddString( pLayer->GetFieldLabel( i ) );
      }
   }


void FieldCalculatorDlg::OnCbnSelchangeLayers()
   {
   LoadFieldInfo();
   }


void FieldCalculatorDlg::OnLbnDblclkFields1()
   {
   CString field;
   int index = m_fields1.GetCurSel();
   m_fields1.GetText( index, field );

   m_formula.ReplaceSel( field, TRUE );
   int end = m_formula.GetWindowTextLength();
   m_formula.SetSel( end, end, 1 ); 
   }

void FieldCalculatorDlg::OnLbnDblclkFunctions()
   {
   CString fn;
   int index = m_functions.GetCurSel();
   m_functions.GetText( index, fn );

   m_formula.ReplaceSel( fn, TRUE );
   int end = m_formula.GetWindowTextLength();
   m_formula.SetSel( end, end, 1 ); 
   }

void FieldCalculatorDlg::OnLbnDblclkOps()
   {
   CString op;
   int index = m_operators.GetCurSel();
   m_operators.GetText( index, op );

   m_formula.ReplaceSel( op, TRUE );
   int end = m_formula.GetWindowTextLength();
   m_formula.SetSel( end, end, 1 ); 
   }


void FieldCalculatorDlg::OnOK()     // Apply
   {
   int index = m_layers.GetCurSel();

   if ( index < 0 )
      {
      MessageBox( "No layer selected - Select a Map Layer to populate" );
      return;
      }

   MapLayer *pLayer = gpMapPanel->m_pMap->GetLayer( index );

   int colCount = pLayer->GetColCount();
   float fValue;
   VData value;
   
   CString formula;
   m_formula.GetWindowText( formula );

   CString field;
   m_fields.GetWindowText( field );

   int col = pLayer->GetFieldCol( field );

   if ( col < 0 )
      {
      int retVal = MessageBox( "The specified field does not exist - would you like to add it?", "Add Field?", MB_YESNO );

      if ( retVal == IDNO )
         return;

      CWaitCursor c;
      col = pLayer->m_pDbTable->AddField( field, TYPE_FLOAT, true );
      }

   CWaitCursor c;

   TYPE type = pLayer->GetFieldType( col );

   // create and initialize the parser
   MTParser parser;

   double *fieldVarsArray = new double[ colCount ];

   // define a variable for every numeric field
   for ( int i=0; i < colCount; i++ )
      parser.defineVar( pLayer->GetFieldLabel( i ), fieldVarsArray+i );

   // compile this formula
   parser.compile( formula );

   // iterate over ACTIVE records
   CString query;
   m_query.GetWindowText( query );
   query.Trim();

   MapLayer::ML_RECORD_SET flag = MapLayer::ALL;

   if ( query.GetLength() > 0 )
      {
      flag = MapLayer::SELECTION;
      QueryEngine qe( pLayer );
      Query *pQuery = qe.ParseQuery( query, 0, "Field Calculator" );

      if ( pQuery == NULL )
         {
         Report::ErrorMsg( "Unable to parse query" );
         return;
         }
         
      qe.SelectQuery( pQuery, true ); // , false );
      }

   for ( MapLayer::Iterator i = pLayer->Begin( flag ); i != pLayer->End( flag ); i++ )
      {
      // load current values into the m_fieldVarArray
      for ( int j=0; j < colCount; j++ )
         {
         bool okay = pLayer->GetData( i, j, value );
 
         if ( okay && value.GetAsFloat( fValue ) )
            fieldVarsArray[ j ] = fValue;
         }

      MTDOUBLE result;
      if( ! parser.evaluate( result ) )
         result = 0;

      VData _result;

      switch( type )
         {
         case TYPE_STRING:
         case TYPE_DSTRING:
            {
            TCHAR str[ 32 ];
            sprintf_s( str, 32, "%g", (float) result );
            _result.AllocateString( str );
            }
            break;

         case TYPE_INT:
         case TYPE_SHORT:
            _result = (int) result;
            break;

         case TYPE_UINT:
            _result = (UINT) result;
            break;

         case TYPE_LONG:
            _result = (LONG) result;
            break;

         case TYPE_ULONG:
            _result = (ULONG) result;
            break;

         case TYPE_FLOAT:
            _result = (float) result;
            break;

         case TYPE_DOUBLE:
            _result = (double) result;
            break;

         case TYPE_BOOL:
            _result = result > 0.5 ? true : false;
            break;

         default:
            ASSERT( 0 );
         }

      pLayer->SetData( i, col, _result );
      }  // end of: MapIterator

   pLayer->ClearSelection();

   delete fieldVarsArray;

   char *cwd = _getcwd( NULL, 0 );

   CFileDialog dlg( FALSE, "dbf", pLayer->m_tableName , OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
         "DBase Files|*.dbf|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      CWaitCursor c;
      CString filename( dlg.GetPathName() );
      pLayer->SaveDataDB( filename ); // uses DBase/CodeBase
      gpDoc->SetChanged( 0 );
      }
   else
      gpDoc->SetChanged( CHANGED_COVERAGE );

   _chdir( cwd );
   free( cwd );
   }
