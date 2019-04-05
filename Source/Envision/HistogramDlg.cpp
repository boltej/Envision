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
// HistogramDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "HistogramDlg.h"

#include <EnvModel.h>
#include <maplayer.h>
#include <dirplaceholder.h>

#include <histogram.h>
#include <queryEngine.h>
//#include <vector>

//extern QueryEngine *gpQueryEngine;

extern EnvModel *gpModel;

using std::vector;



// HistogramDlg dialog

IMPLEMENT_DYNAMIC(HistogramDlg, CDialog)

HistogramDlg::HistogramDlg(CWnd* pParent /*=NULL*/)
	: CDialog(HistogramDlg::IDD, pParent)
   , m_binCount(20)
   , m_mean( 0 )
   , m_stdDev( 0 )
   , m_areaWtMean( 0 )
   , m_constrain(FALSE)
   , m_query(_T(""))
   , m_expr(_T(""))
   { }


HistogramDlg::~HistogramDlg()
{
}

void HistogramDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_FIELDS, m_fields);
DDX_Control(pDX, IDC_CATEGORIES, m_categories);
DDX_Control(pDX, IDC_ATTR, m_attrs);
DDX_Text( pDX, IDC_BINCOUNT, m_binCount );
DDX_Check(pDX, IDC_USEQUERY, m_constrain);
DDX_Text(pDX, IDC_QUERY, m_query);
DDX_Text(pDX, IDC_EXPR, m_expr);
DDX_Control(pDX, IDC_NOTEXT, m_noText);
DDX_Control(pDX, IDC_EXPR, m_exprCtrl);
   }


BEGIN_MESSAGE_MAP(HistogramDlg, CDialog)
   //ON_BN_CLICKED(IDC_GENERATE, &HistogramDlg::OnBnClickedGenerate)
   ON_BN_CLICKED(IDC_SAVE, &HistogramDlg::OnBnClickedSave)
   ON_CBN_SELCHANGE(IDC_CATEGORIES, &HistogramDlg::OnCbnSelchangeCategories)
   ON_LBN_SELCHANGE(IDC_FIELDS, &HistogramDlg::OnLbnSelchangeFields)
   ON_CBN_SELCHANGE(IDC_ATTR, &HistogramDlg::OnCbnSelchangeAttr)
   ON_BN_CLICKED(IDC_BUTTON1, &HistogramDlg::OnBnClickedButton1)
   ON_BN_CLICKED(IDC_RADIO1, &HistogramDlg::OnBnClickedRadio1)
   ON_BN_CLICKED(IDC_RADIO2, &HistogramDlg::OnBnClickedRadio2)
   ON_LBN_DBLCLK(IDC_FIELDS, &HistogramDlg::OnLbnDblclkFields)
END_MESSAGE_MAP()


// HistogramDlg message handlers

BOOL HistogramDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   CheckDlgButton( IDC_RADIO2, 0 );
   CheckDlgButton( IDC_RADIO1, 1 );

   // add fields (only numeric for now)
   m_fields.ResetContent();
   for ( int i=0; i < m_pLayer->GetColCount(); i++ )
      {
      TYPE type = m_pLayer->GetFieldType( i );

      if ( ::IsNumeric( type ) )
         {
         LPCTSTR field = m_pLayer->GetFieldLabel( i );
         m_fields.AddString( field );
         }
      }
   m_fields.SetCurSel( 0 );

   // add categories
   m_categories.ResetContent();
   m_categories.AddString( _T("<All>") );

   for ( int i=0; i < m_pLayer->GetColCount(); i++ )
      {
      TYPE type = m_pLayer->GetFieldType( i );

      MAP_FIELD_INFO *pInfo = m_pLayer->FindFieldInfo( m_pLayer->GetFieldLabel( i ) );

      if ( pInfo != NULL || ( ::IsInteger( type ) || type == TYPE_STRING || type == TYPE_DSTRING ) )
         {
         LPCTSTR field = m_pLayer->GetFieldLabel( i );
         m_categories.AddString( field );
         }
      }
   m_categories.SetCurSel( 0 );

   RECT rect;
   CWnd *pCtrl = GetDlgItem( IDC_PLACEHOLDER );
   pCtrl->GetWindowRect( &rect );
   ScreenToClient( &rect );
   m_histoPlot.CreateEx( WS_EX_CLIENTEDGE, NULL, NULL, WS_VISIBLE | WS_CHILD, rect, this, 111 );
   m_plot.CreateEx( WS_EX_CLIENTEDGE, NULL, NULL, WS_CHILD, rect, this, 112 );

   GenerateHistogram( true );

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }

void HistogramDlg::OnCbnSelchangeCategories()
   {
   m_attrs.ResetContent();

   // get current category
   int index = m_categories.GetCurSel();

   if ( index < 1 )
      return;

   CString category;
   m_categories.GetLBText( index, category );

   int categoryIndex = m_pLayer->GetFieldCol( category );
   ASSERT( categoryIndex >= 0 );

   MAP_FIELD_INFO *pInfo = m_pLayer->FindFieldInfo( category );
   if ( pInfo )
      {
      for ( int i=0; i < pInfo->GetAttributeCount(); i++ )
         {
         FIELD_ATTR &attr = pInfo->GetAttribute( i );
         CString label;

         if ( attr.minVal == attr.maxVal )
            label.Format( "%s (%s)", (LPCTSTR) attr.label, attr.value.GetAsString() ); 
         else
            label.Format( "%s (%g-%g)", (LPCTSTR) attr.label, attr.minVal, attr.maxVal ); 

         m_attrs.AddString( label );

         if ( i == 0 )
            {
            m_curAttrVal = attr.value;
            m_curAttrMin = attr.minVal;
            m_curAttrMax = attr.maxVal;
            }
         }
      }
   else  // no field information defined
      {
      CStringArray values;
      m_pLayer->GetUniqueValues( categoryIndex, values );

      for ( int i=0; i < values.GetSize(); i++ )
         {
         m_attrs.AddString( values[ i ] );

         if ( i == 0 )
            {
            m_curAttrVal.Parse( values[ i ] );
            m_curAttrMin = -1;
            m_curAttrMax = -1;
            }            
         }
      }

   m_attrs.SetCurSel( 0 );
   GenerateHistogram( true );
   }


void HistogramDlg::GenerateHistogram( bool updatePlot )
   {
   CWaitCursor c;
   int selCat   = m_categories.GetCurSel();
   int categoryCol = -1;
   if ( selCat >= 1 )
      {
      CString category;
      m_categories.GetLBText( selCat, category );
      categoryCol = m_pLayer->GetFieldCol( category );
      ASSERT( categoryCol >= 0 );
      }

   int attrIndex = m_attrs.GetCurSel();

   UpdateAttrValues( attrIndex );  // score the current category attribute value(s)

   vector<float> vData;  // contains all the selected field's values (observations).  If a category is selected, this contains a subset of
                         // rows corresponding to the selected attribte value.  Otherwise, it contains all the rows.

   //vector<float> vAttr;   // contains the attribute values correspnding to the observations

   THistogram< float > histo( m_binCount );
   int observations = PopulateBins( histo, vData, categoryCol, updatePlot );

   if ( observations < 0 )
      return;

   // get a vector with the histogram
   vector< UINT > h = histo.GetHistogram();

   if ( observations == 0 )
      {
      m_histoPlot.ShowWindow( SW_HIDE );
      m_plot.ShowWindow( SW_HIDE );
      m_noText.ShowWindow( SW_SHOW );
      }
   else
      {
      m_histoPlot.ShowWindow( IsDlgButtonChecked( IDC_RADIO1 ) ? SW_SHOW : SW_HIDE );
      m_plot.ShowWindow( IsDlgButtonChecked( IDC_RADIO2 ) ? SW_SHOW : SW_HIDE );
      m_noText.ShowWindow( SW_HIDE );

      // set up plot data
      m_histoData.Clear();
      m_histoData.SetSize( 2, m_binCount );
      m_histoData.AddLabel( _T("Bin" ) );
      m_histoData.AddLabel( _T("Frequency" ) );

      for ( int i=0; i < m_binCount; i++ )
         {
         m_histoData.Set( 0, i, i );
         m_histoData.Set( 1, i, (int) h[i] );
         }

      m_histoPlot.ClearYCols();

      m_histoPlot.SetXCol( &m_histoData, 0 );
      m_histoPlot.AppendYCol( &m_histoData, 1 );
      m_histoPlot.yAxis.autoIncr = true;

      m_histoPlot.RedrawWindow();

      // regular plot
      int selCat = m_categories.GetCurSel();

      if ( updatePlot && selCat > 0 )
         {
         m_plot.ShowWindow( SW_SHOW );
         CString category;         
         m_categories.GetLBText( selCat, category );
         m_data.AddLabel( category );
         m_data.AddLabel( m_expr );

         m_plot.ClearYCols();

         m_plot.SetXCol( &m_data, 0 );
         m_plot.AppendYCol( &m_data, 1 );
         m_plot.yAxis.autoIncr = true;
         m_plot.xAxis.autoIncr = true;

         LINE *pLine = m_plot.GetLineInfo( 0 );
         pLine->markerIncr = 1;
         pLine->showLine = false;
         pLine->showMarker = true;
         pLine->markerStyle = MS_HOLLOWCIRCLE;

         m_plot.SetAutoscaleAll( true );

         m_plot.RedrawWindow();
         }
      }

   CString mean, areaWtMean, stdDev;
   mean.Format( "%g", m_mean );
   areaWtMean.Format( "%g", m_areaWtMean );
   stdDev.Format( "%g", m_stdDev );

   SetDlgItemText( IDC_MEAN, mean );
   SetDlgItemText( IDC_AREAWTMEAN, areaWtMean );
   SetDlgItemText( IDC_STDDEV, stdDev );
   }


int HistogramDlg::PopulateBins( THistogram<float> &histo, vector<float> &vData, int categoryCol, bool updatePlot )
   {
   UpdateData( 1 );
   // first pass - figure out number of field values needed to store.
   int observations = 0;
   int rows = m_pLayer->GetRowCount();
   int colArea = m_pLayer->GetFieldCol( _T("Area") );

   m_mean = 0;
   m_stdDev = 0;
   m_areaWtMean = 0;
   float totalArea = 0;

   Query *pQuery = NULL;

   if ( m_constrain  && ! m_query.IsEmpty() )
      {
      pQuery = gpModel->m_pQueryEngine->ParseQuery( m_query, 0, "Histogram Setup" );

      if ( pQuery != NULL )
         rows = gpModel->m_pQueryEngine->SelectQuery( pQuery, true ); //, false );
      }

   if ( categoryCol < 0 )   // no category selected?
      {
      observations = rows;
     }
   else  // category selected - figure out size of vector needed for the category
      {
      VData value;
      for ( int i=0; i < rows; i++ )
         {
         int idu = i;

         if ( pQuery )
            idu = m_pLayer->GetSelection( i );

         m_pLayer->GetData( idu, categoryCol, value );

         if ( m_curAttrMin == m_curAttrMax )
            {
            if ( value == m_curAttrVal )
               observations++;
            }
         else
            {
            float fValue;
            if ( value.GetAsFloat( fValue ) && fValue >= m_curAttrMin && fValue <= m_curAttrMax )
               observations++;
            }
         }
      }

   CString totalObs;
   totalObs.Format( _T("observations: %i of %i IDU's considered"), observations, rows );
   SetDlgItemText( IDC_OBSERVATIONS, totalObs );
   
   // make sure there are matching attributes   
   if ( observations == 0 )  // no attribute values for this attribute?
      {
      if ( pQuery )
         gpModel->m_pQueryEngine->RemoveQuery( pQuery );

      histo.Resize( 0 );
      return 0;
      }

   vData.resize( observations, 0.0f );

   if ( updatePlot )
      {
      m_data.Clear();
      m_data.SetSize( 2, rows );
      }

   // Pass 2: populate the field values vector
   int vIndex = 0;
   VData value;
   
   // set up parser
   MTParser parser;
   parser.enableAutoVarDefinition( true );
   parser.compile( m_expr );

   UINT varCount = parser.getNbUsedVars();
   PARSER_VAR *parserVars = NULL;
   
   if ( varCount > 0 )
      parserVars = new PARSER_VAR[ varCount ];

   for ( UINT i=0; i < varCount; i++ )
      {
      CString var = parser.getUsedVar( i ).c_str();

      int col = m_pLayer->GetFieldCol( var );
      if ( col < 0 )
         {
         AfxMessageBox( _T("Histogram: Unrecognized variable found - operation aborted" ) );
         return -1;
         }

      parserVars[ i ].col = col;
      parser.redefineVar( var, &(parserVars[i].value));
      }

   for ( int i=0; i < rows; i++ )
      {
      int idu = i;
      if ( pQuery )
         idu = m_pLayer->GetSelection( i );

      float fValue, area;
      m_pLayer->GetData( idu, colArea, area );
      bool used = false;

      //m_pLayer->GetData( idu, fieldCol, fValue ); // store the field value
      for ( UINT j=0; j < varCount; j++ )
         {
         float value;
         int col = parserVars[ j ].col;

         m_pLayer->GetData( idu, col, value );
         parserVars[ j ].value = value;
         }

      double dValue;
      if ( ! parser.evaluate( dValue ) )
         dValue = 0;
      fValue = (float) dValue;      
 
      if ( categoryCol >= 0 ) // category defined
         {
         m_pLayer->GetData( idu, categoryCol, value );     // get the category attribute

         if ( m_curAttrMin == m_curAttrMax )   // single value?
            {
            if ( value == m_curAttrVal )  // does it match the currently selected attribute value?
               {
               vData[ vIndex ] = fValue;
               vIndex++;               
               used = true;
               }
            
            if ( updatePlot )
               {
               float attrVal;
               value.GetAsFloat( attrVal );
               m_data.Set( 0, i, attrVal );
               m_data.Set( 1, i, fValue );
               }
            }
         else  // its a range
            {
            float attrValue;
            if ( value.GetAsFloat( attrValue ) && attrValue >= m_curAttrMin && attrValue <= m_curAttrMax ) // does is match the currently selected category attribute range?
               {
               vData[ vIndex ] = fValue;
               vIndex++;
               used = true;
               }
            
            if ( updatePlot )
               {
               m_data.Set( 0, i, attrValue );
               m_data.Set( 1, i, fValue );
               }
            }
         }  // end of: if ( categoryCol >= 0 ) 
      else  // no category
         {
         vData[ i ] = fValue;

         if ( updatePlot )
            {
            m_data.Set( 0, i, 0 );
            m_data.Set( 1, i, fValue );
            }

         used = true;
         }

      if ( used )
         {
         m_mean += fValue;
         m_areaWtMean += fValue * area;
         totalArea += area;
         }
      }  // end of: for ( i < rows )

   if ( varCount > 0 )
      delete [] parserVars;


   // vData vector fully populated.  Compute histograms
	// Creating histogram using float data and with binCount containers,
	//THistogram< float > histo( m_binCount );
	// computing the histogram
	histo.Compute( vData );

   ASSERT( vData.size() == observations );

   m_mean /= observations;
   m_areaWtMean /= totalArea;

   float stdErr = 0;

   for ( int i=0; i < observations; i++ )
      stdErr += (vData[ i ]-m_mean)*(vData[i]-m_mean );

   m_stdDev = sqrt( stdErr/observations );

   if ( pQuery )
      gpModel->m_pQueryEngine->RemoveQuery( pQuery );

   return observations;
   }


void HistogramDlg::OnBnClickedSave()
   {
   // basic idea - 
   //  if a category (e.g. LULC_C) is selected, iterate through the categories attributes, generating
   //      a histrogram for each category.
   //  if no category is selected, create a histogram for the specified field across the entire map
   UpdateData( 1 );

   int selCat = m_categories.GetCurSel();
   int categoryCol = -1;
   CString category;
   if ( selCat >= 1 )      // is a category selected?  then get the col for this category
      {
      m_categories.GetLBText( selCat, category );
      categoryCol = m_pLayer->GetFieldCol( category );
      ASSERT( categoryCol >= 0 );
      }
   else
      category = "";

   CWaitCursor c;
   DirPlaceholder d;

   CFileDialog dlg( FALSE, "xml", "", OFN_HIDEREADONLY, "XML Files|*.xml|All files|*.*||");

   if ( dlg.DoModal() != IDOK )
      return;

   CString filename( dlg.GetPathName() );
   
   // open the file and write contents
   FILE *fp;
   PCTSTR file = (PCTSTR)filename;
   fopen_s( &fp, file, "wt" );
   if ( fp == NULL )
      {
      LONG s_err = GetLastError();
      CString msg = "Failure to open " + CString(filename) + " for writing.  ";
      Report::SystemErrorMsg  ( s_err, msg );
      return;
      }

   fputs( _T("<?xml version='1.0' encoding='utf-8' ?>\n\n"), fp );

   CString query;
   if ( m_constrain && ! m_query.IsEmpty() )
      query = m_query;

   fprintf( fp, _T("<histograms name='%s' value='%s' category='%s' query='%s'>\n"), (LPCTSTR) m_expr, (LPCTSTR) m_expr, (LPCTSTR) category, (LPCTSTR) query );

   if ( categoryCol >= 0 )
      {
      for ( int j=0; j < m_attrs.GetCount(); j++ )
         {
         // set attributes
         int attrIndex = j;
         UpdateAttrValues( attrIndex );  // score the current category attribute value(s)

         vector<float> vData;  // upon return from PopulateBins(), contains all the selected field's values.
                               // If a category is selected, this contains a subset of rows (observations) corresponding to 
                               // the selected attribute value.  Otherwise, it contains all the rows.

         // create and populate histogram
         THistogram< float > histo( m_binCount );
         int observations = PopulateBins( histo, vData, categoryCol, false );  // computes statistics as well as populating bins

         if ( observations < 0 )
            continue;
         
         // get a vector with the histogram
         vector< UINT > h = histo.GetHistogram();
         vector<double>  &v = histo.GetLeftContainers();
         double step = histo.GetStep();

         if ( m_curAttrMin == m_curAttrMax )   /// category bin?  use value...
            fprintf( fp, _T("\n\t<histogram category='%s' value='%s' bins='%i' obs='%i' mean='%g' areaWtMean='%g' stdDev='%g'>"),  (LPCTSTR) category, m_curAttrVal.GetAsString(), m_binCount, observations, m_mean, m_areaWtMean, m_stdDev );
         else
            fprintf( fp, _T("\n\t<histogram category='%s' minValue='%g' maxValue='%g' bins='%i' obs='%i' mean='%g' areaWtMean='%g' stdDev='%g'>"), (LPCTSTR) category, m_curAttrMin, m_curAttrMax, m_binCount, observations, m_mean, m_areaWtMean, m_stdDev );

         if ( observations > 0 )
            {
            for ( int i=0; i < m_binCount; i++ )
               {
               // bins record range of field values and number of observations associated with the bin
               fprintf( fp, _T("\n\t\t<bin left='%g' right='%g' count='%i'/>"), (float) v[i], float( v[ i ]+step ), h[ i ] );
               }
            }

         fputs( _T("\n\t</histogram>\n"), fp );
         }
      fputs( _T("\n</histograms>\n"), fp );
      }

   fclose( fp );
   }


void HistogramDlg::UpdateAttrValues( int attrIndex )
   {
   int selCat = m_categories.GetCurSel();
   if ( selCat < 1 )
      return;

   CString category;
   m_categories.GetLBText( selCat, category );

   //int selField = m_fields.GetCurSel();
   //CString field;
   //m_fields.GetText( selField, field );
   //int fieldCol = m_pLayer->GetFieldCol( field );

   ASSERT( attrIndex >= 0 );

   MAP_FIELD_INFO *pInfo = m_pLayer->FindFieldInfo( category );
   if ( pInfo )
      {
      FIELD_ATTR &attr = pInfo->GetAttribute( attrIndex );

      m_curAttrVal = attr.value;
      m_curAttrMin = attr.minVal;
      m_curAttrMax = attr.maxVal;
      }
   else
      {
      CString attrValue;
      m_attrs.GetLBText( attrIndex, attrValue );

      m_curAttrVal.Parse( attrValue ); 
      m_curAttrMin = -1;
      m_curAttrMax = -1;
      }
   }

void HistogramDlg::OnLbnSelchangeFields()
   {
   //GenerateHistogram();
   }

void HistogramDlg::OnCbnSelchangeAttr()
   {
   GenerateHistogram( false );
   }

void HistogramDlg::OnBnClickedButton1()
   {
   GenerateHistogram( true );
   }

void HistogramDlg::OnBnClickedRadio1()
   {
   m_histoPlot.ShowWindow( IsDlgButtonChecked( IDC_RADIO1 ) ? SW_SHOW : SW_HIDE );
   m_plot.ShowWindow( IsDlgButtonChecked( IDC_RADIO2 ) ? SW_SHOW : SW_HIDE );
   }

void HistogramDlg::OnBnClickedRadio2()
   {
   m_histoPlot.ShowWindow( IsDlgButtonChecked( IDC_RADIO1 ) ? SW_SHOW : SW_HIDE );
   m_plot.ShowWindow( IsDlgButtonChecked( IDC_RADIO2 ) ? SW_SHOW : SW_HIDE );
   }

void HistogramDlg::OnLbnDblclkFields()
   {
   CString field;
   int index = m_fields.GetCurSel();
   m_fields.GetText( index, field );

   m_exprCtrl.ReplaceSel( field, TRUE );

   int end = m_exprCtrl.GetWindowTextLength();
   m_exprCtrl.SetSel( end, end, 1 ); 
   }
