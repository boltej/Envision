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
// AreaSummaryDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "AreaSummaryDlg.h"

#include <queryEngine.h>
#include <DirPlaceholder.h>
#include <VDataObj.h>



// AreaSummaryDlg dialog

IMPLEMENT_DYNAMIC(AreaSummaryDlg, CDialog)

AreaSummaryDlg::AreaSummaryDlg(MapLayer *pLayer, CWnd* pParent /*=NULL*/)
	: CDialog(AreaSummaryDlg::IDD, pParent)
   , m_pLayer( pLayer )
   , m_query(_T(""))
   {

}

AreaSummaryDlg::~AreaSummaryDlg()
{
}

void AreaSummaryDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_FIELDS, m_fields);
DDX_Text(pDX, IDC_QUERY, m_query);
   }


BEGIN_MESSAGE_MAP(AreaSummaryDlg, CDialog)
   ON_BN_CLICKED(IDC_SAVEAS, &AreaSummaryDlg::OnBnClickedSaveas)
END_MESSAGE_MAP()


// AreaSummaryDlg message handlers

void AreaSummaryDlg::OnBnClickedSaveas()
   {
   VDataObj table( 4, m_grid.GetNumberRows(), U_UNDEFINED );

   table.AddLabel( _T("Attribute") ); 
   table.AddLabel( _T("Area") ); 
   table.AddLabel( _T("% Query Area") ); 
   table.AddLabel( _T("% Total Area") ); 

   for ( int i=0; i < m_grid.GetNumberRows(); i++ )
      {
      for ( int j=0; j < 4; j++ )
         table.Set( j, i, m_grid.QuickGetText( j, i ) );
      }

   DirPlaceholder d;
   CFileDialog dlg( TRUE, "csv", "", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "CSV files|*.csv|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      table.WriteAscii( dlg.GetPathName(), ',' );
   }


BOOL AreaSummaryDlg::OnInitDialog()
   {
   ASSERT( m_pLayer != NULL );

   CDialog::OnInitDialog();

   m_grid.AttachGrid( this, IDC_PLACEHOLDER );

   m_grid.SetNumberCols( 4 );
   m_grid.SetSH_Width( 0 );
   m_grid.SetColWidth( 0, 250 );
   m_grid.SetColWidth( 1, 100 );
   m_grid.SetColWidth( 2, 100 );
   m_grid.SetColWidth( 3, 100 );

   m_grid.QuickSetText( 0, -1, _T("Attribute") );
   m_grid.QuickSetText( 1, -1, _T("Area") );
   m_grid.QuickSetText( 2, -1, _T("% Query Area") );
   m_grid.QuickSetText( 3, -1, _T("% Total Area") );

   for ( int i=0; i < m_pLayer->GetColCount(); i++ )
      {
      TYPE type = m_pLayer->GetFieldType( i );

      MAP_FIELD_INFO *pInfo = m_pLayer->FindFieldInfo( m_pLayer->GetFieldLabel( i ) );

      if ( pInfo != NULL || ( ::IsInteger( type ) || type == TYPE_STRING || type == TYPE_DSTRING ) )
         {
         LPCTSTR field = m_pLayer->GetFieldLabel( i );
         m_fields.AddString( field );
         }
      }

   m_fields.SetCurSel( 0 );

   UpdateAreas();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


void AreaSummaryDlg::UpdateAreas()
   {
   UpdateData( 1 );

   int gridRowCount = m_grid.GetNumberRows();

   for ( int i=gridRowCount-1; i >= 0; i-- )
      m_grid.DeleteRow( i );

   int colArea = m_pLayer->GetFieldCol( _T("Area") );
   ASSERT( colArea >= 0 );

   int index = m_fields.GetCurSel();

   CString field;
   m_fields.GetLBText( index, field );

   int colField = m_pLayer->GetFieldCol( field );
   ASSERT( colField >= 0 );

   QueryEngine *pQE = NULL;

   if ( ! m_query.IsEmpty() )
      {
      pQE = new QueryEngine( m_pLayer );
      Query *pQuery = pQE->ParseQuery( m_query, 0, "Area Summary" );
      if ( pQuery == NULL )
         {
         delete pQE;
         pQE = NULL;
         return;
         }

      pQE->SelectQuery( pQuery, true ); // redraw=false );
      }

   int gridrow = 0;

   float totalArea = m_pLayer->GetTotalArea();
   float sumArea = 0;
   float sumPctQueryArea = 0;
   float sumPctTotalArea = 0;
   
   // get attribute info
   MAP_FIELD_INFO *pInfo = m_pLayer->FindFieldInfo( field );
   float totalQueryArea = 0;

   if ( pInfo )
      {
      for ( int i=0; i < pInfo->GetAttributeCount(); i++ )
         {
         totalQueryArea = 0;
         float area = 0;
         bool range = false;

         FIELD_ATTR &attr = pInfo->GetAttribute( i );
         CString label;

         if ( attr.minVal == attr.maxVal )
            label.Format( "%s (%s)", (LPCTSTR) attr.label, attr.value.GetAsString() ); 
         else
            {
            label.Format( "%s (%g-%g)", (LPCTSTR) attr.label, attr.minVal, attr.maxVal ); 
            range = true;
            }

         // get area
         int row = 0;
         int rowCount = m_pLayer->GetRowCount();
         if ( pQE )
            rowCount = m_pLayer->GetSelectionCount();

         for ( int j=0; j < rowCount; j++ )
            {
            int idu = j;
            if ( pQE )
               idu = m_pLayer->GetSelection( j );

            float iduArea;
            m_pLayer->GetData( idu, colArea, iduArea );

            if ( range )   // assume float
               {
               float value;
               m_pLayer->GetData( idu, colField, value );

               if ( attr.minVal <= value && value <= attr.maxVal )
                  area += iduArea;
               }
            else  // a unique value
               {
               VData value;
               m_pLayer->GetData( idu, colField, value );

               if ( value == attr.value )
                  area += iduArea;
               }

            totalQueryArea += iduArea;
            }  // end of idu rows

         m_grid.AppendRow();
         m_grid.QuickSetText( 0, gridrow, label );

         CString sarea, pctQueryArea, pctTotalArea;
         sarea.Format( _T("%g"), area );
         pctQueryArea.Format( _T("%g"), 100*area/totalQueryArea );
         pctTotalArea.Format( _T("%g"), 100*area/totalArea );

         m_grid.QuickSetText( 1, gridrow, sarea );
         m_grid.QuickSetText( 2, gridrow, pctQueryArea );
         m_grid.QuickSetText( 3, gridrow, pctTotalArea );

         sumArea += area;
         sumPctQueryArea += area/totalQueryArea;
         sumPctTotalArea += area/totalArea;

         gridrow++;
         }  // end of field Infos
      }
   else  // no field information defined
      {
      CStringArray values;
      m_pLayer->GetUniqueValues( colField, values );

      for ( int i=0; i < values.GetSize(); i++ )
         {
         // get area
         float area = 0;
         totalQueryArea = 0;

         int row = 0;
         int rowCount = m_pLayer->GetRowCount();
         if ( pQE )
            rowCount = m_pLayer->GetSelectionCount();

         for ( int j=0; j < rowCount; j++ )
            {
            int idu = j;
            if ( pQE )
               idu = m_pLayer->GetSelection( j );

            float iduArea;
            m_pLayer->GetData( idu, colArea, iduArea );

            VData value;
            m_pLayer->GetData( idu, colField, value );

            if ( value == VData( values[ i ] ) )
               area += iduArea;
            
            totalQueryArea += iduArea;
            }  // end of idu rows

         m_grid.AppendRow();
         m_grid.QuickSetText( 0, gridrow, values[ i ] );

         CString sarea, pctQueryArea, pctTotalArea;
         sarea.Format( _T("%g"), area );
         pctQueryArea.Format( _T("%g"), 100*area/totalQueryArea );
         pctTotalArea.Format( _T("%g"), 100*area/totalArea );

         m_grid.QuickSetText( 1, gridrow, sarea );
         m_grid.QuickSetText( 2, gridrow, pctQueryArea );
         m_grid.QuickSetText( 3, gridrow, pctTotalArea );
 
         sumArea += area;
         sumPctQueryArea += ( area/totalQueryArea );
         sumPctTotalArea += ( area/totalArea );

         gridrow++;
         }
      }

   m_grid.AppendRow();

   m_grid.QuickSetText( 0, gridrow, _T("Totals") );

   CString sarea, pctQueryArea, pctTotalArea;
   sarea.Format( _T("%g"), sumArea);
   pctQueryArea.Format( _T("%g"), 100*sumPctQueryArea );
   pctTotalArea.Format( _T("%g"), 100*sumPctTotalArea );

   m_grid.QuickSetText( 1, gridrow, sarea );
   m_grid.QuickSetText( 2, gridrow, pctQueryArea );
   m_grid.QuickSetText( 3, gridrow, pctTotalArea );
 
   m_grid.RedrawAll();

   if( pQE )
      delete pQE;
   }


void AreaSummaryDlg::OnOK()
   {
   UpdateAreas();
   }
