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
// MapLayerDlg.cpp : implementation file
//

#include "EnvWinLibs.h"
#include "EnvWinLib_resource.h"
#include "maplayerdlg.h"
#include "MapLabelDlg.h"
#include "FieldSorterDlg.h"
#include "MapWnd.h"
#include <UNITCONV.H>

#include <maplayer.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// MapLayerDlg dialog


int RunMapLayerDlg( MapWindow *pMapWnd, MapLayer *pLayer, CWnd *pParent )
   {
   MapLayerDlg dlg( pMapWnd, pLayer, pParent );
   return (int) dlg.DoModal();   
   }


MapLayerDlg::MapLayerDlg(MapWindow *pMapWnd, MapLayer *pLayer, CWnd* pParent /*=NULL*/)
: CDialog(MapLayerDlg::IDD, pParent)
, m_pMapWnd( pMapWnd ) 
, m_pLayer( pLayer )
, m_outlineColor( 0 )
, m_width(0)
, m_decimals(0)
, m_actualExtents(_T(""))
   {
	m_features = _T("");
   m_fields = _T("");
	m_file = _T("");
	m_layerName = _T("");
	m_type = _T("");
	m_visible = FALSE;
	m_showOutlines = FALSE;

   float minArea = 10000000, maxArea = 0;
   int colArea = pLayer->GetFieldCol( "Area" );

   if ( colArea >= 0 )
      {
      for ( int idu=0; idu < pLayer->GetRecordCount(); idu++ )
         {
         float area = 0;
         pLayer->GetData( idu, colArea, area );

         if ( area < minArea ) minArea = area;
         if ( area > maxArea ) maxArea = area;
         }
      }
   
   switch( pLayer->m_layerType )
      {
      case LT_POLYGON:
         m_features.Format( "%d polygons", pLayer->GetPolygonCount() );
         break;

      case LT_LINE:
         m_features.Format( "%d vectors", pLayer->GetPolygonCount() );
         break;

      case LT_POINT:
         m_features.Format( "%d points", pLayer->GetRecordCount() );
         break;
         
      case LT_GRID:
         m_features.Format( "%d rows x %d columns", pLayer->m_pData->GetRowCount(), pLayer->m_pData->GetColCount() );
         break;
      }

   float totalArea = pLayer->GetTotalArea();

   if ( pLayer->GetType() == LT_POLYGON )
      {
      switch( pLayer->m_pMap->GetMapUnits() )
         {
         case MU_FEET:     // convert to square miles to total area, acres for average area
            m_totalArea.Format( "%.1f sq. miles", totalArea / ( 5280*5280) );
            m_avgArea.Format( "%.1f acres", totalArea / ( pLayer->GetPolygonCount() * FT2_PER_ACRE ) );
            m_areaRange.Format( "%.1f - %.1f acres", minArea * FT2_PER_ACRE, maxArea* FT2_PER_ACRE );
            break;

         case MU_METERS:
            m_totalArea.Format( "%.1f ha", totalArea / M2_PER_HA );
            m_avgArea.Format( "%.1f ha", totalArea / ( pLayer->GetPolygonCount() * M2_PER_HA  ) );
            m_areaRange.Format( "%.1f - %.1f ha", minArea / M2_PER_HA, maxArea /M2_PER_HA );
            break;

         default:
            m_totalArea.Format( "%.1f", totalArea );
            m_avgArea.Format( "%.1f", totalArea / pLayer->GetPolygonCount() );
            m_areaRange.Format( "%.1f - %.1f", minArea, maxArea );
            break;       
         }
      }

   if( pLayer->m_pData == NULL )
      m_fields = "No data loaded";
   else
      m_fields.Format( "%d fields", pLayer->GetFieldCount() );

	m_file = pLayer->m_path;
   m_layerName = pLayer->m_name;

   switch( pLayer->GetType() )
      {
      case LT_POLYGON:  m_type = "Polygon";  break;
      case LT_LINE:     m_type = "Line";     break;
      case LT_GRID:     m_type = "Grid";     break;
      case LT_POINT:    m_type = "Point";    break;
      default: m_type = "Unknown";
      }

   if ( pLayer->IsOverlay() )
      m_type += " (Overlay)";

   m_visible = pLayer->m_isVisible ? TRUE : FALSE;
   m_showOutlines = pLayer->GetOutlineColor() == NO_OUTLINE ? FALSE : TRUE;

   REAL xMin, xMax, yMin, yMax;
   m_pLayer->GetExtents( xMin, xMax, yMin, yMax );
   m_actualExtents.Format( "Extents: (%.0f, %.0f)-(%.0f, %.0f)", xMin, yMin, xMax, yMax );
   }


void MapLayerDlg::DoDataExchange(CDataExchange* pDX)
   {
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_LIB_OUTLINECOLOR, m_outlineColorBtn);
   DDX_Control(pDX, IDC_LIB_LWIDTH, m_outlineWidth );
   DDX_Text(pDX, IDC_LIB_FEATURES, m_features);
   DDX_Text(pDX, IDC_LIB_TOTALAREA, m_totalArea);
   DDX_Text(pDX, IDC_LIB_AVERAGEAREA, m_avgArea);
   DDX_Text(pDX, IDC_LIB_AREARANGE, m_areaRange);
   DDX_Text(pDX, IDC_LIB_FIELDS, m_fields);
   DDX_Text(pDX, IDC_LIB_LFILE, m_file);
   DDX_Text(pDX, IDC_LIB_LAYER, m_layerName);
   DDX_Text(pDX, IDC_LIB_LTYPE, m_type);
   DDX_Check(pDX, IDC_LIB_VISIBLE, m_visible);
   DDX_Check(pDX, IDC_LIB_SHOWOUTLINES, m_showOutlines);
   DDX_Check(pDX, IDC_LIB_VARWIDTH, m_varWidth);
   DDX_Control(pDX, IDC_LIB_FIELDCOMBO, m_fieldsCombo);
   DDX_Control(pDX, IDC_LIB_TYPE, m_typeCombo);
   DDX_Text(pDX, IDC_LIB_WIDTH, m_width);
   DDX_Text(pDX, IDC_LIB_DECIMALS, m_decimals);
   DDX_Text(pDX, IDC_LIB_EXTENTS2, m_actualExtents);
   DDX_Control(pDX, IDC_LIB_TRANSPARENCY, m_transparencySlider);
   DDX_Control(pDX, IDC_LIB_TRANSPARENCY_VALUE, m_percentTransparency);
   }


BEGIN_MESSAGE_MAP(MapLayerDlg, CDialog)
	ON_BN_CLICKED(IDC_LIB_SHOWOUTLINES, OnShowoutlines)
   ON_BN_CLICKED(IDC_LIB_LEDITFIELDINFO, &MapLayerDlg::OnBnClickedEditfieldinfo)
   ON_BN_CLICKED(IDC_LIB_EDITLABELINFO, &MapLayerDlg::OnBnClickedEditlabelinfo)
   ON_BN_CLICKED(IDC_LIB_ADDFIELD, &MapLayerDlg::OnBnClickedAddfield)
   ON_BN_CLICKED(IDC_LIB_REMOVEFIELD, &MapLayerDlg::OnBnClickedRemovefield)
   ON_CBN_SELCHANGE(IDC_LIB_TYPE, &MapLayerDlg::OnCbnSelchangeType)
   ON_CBN_SELCHANGE(IDC_LIB_FIELDCOMBO, &MapLayerDlg::OnCbnSelchangeFieldcombo)
   ON_BN_CLICKED(IDC_LIB_REORDERCOLS, &MapLayerDlg::OnBnClickedLibReorderCols)
   ON_WM_HSCROLL()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// MapLayerDlg message handlers


BOOL MapLayerDlg::OnInitDialog() 
   {
	CDialog::OnInitDialog();

   int colArea = m_pLayer->GetFieldCol( "Area" );
   if ( colArea < 0 )
      GetDlgItem( IDC_LIB_AREARANGE )->ShowWindow( SW_HIDE );


   long outlineColor = m_pLayer->GetOutlineColor();

   if ( outlineColor != NO_OUTLINE )
      m_outlineColorBtn.SetColor( outlineColor );

   m_outlineWidth.AddString( "0" );
   TCHAR str[ 12 ];
   for( int i=1; i < 10; i++ )
      {
      _itoa_s( i, str, 12, 10 );
      m_outlineWidth.AddString( str );
      }

   m_outlineWidth.SetCurSel( m_pLayer->GetLineWidth() );

   m_typeCombo.AddString( "Boolean" );
   m_typeCombo.AddString( "Character" );
   m_typeCombo.AddString( "Short" );
   m_typeCombo.AddString( "Integer" );
   m_typeCombo.AddString( "Unsigned Integer" );
   m_typeCombo.AddString( "Long" );
   m_typeCombo.AddString( "Float" );
   m_typeCombo.AddString( "Double" );
   m_typeCombo.AddString( "String" );
   m_typeCombo.SetCurSel( 0 );

   m_transparencySlider.SetRange( 0, 100 );
   m_transparencySlider.SetTicFreq( 10 );
   m_transparencySlider.SetPos( m_pLayer->GetTransparency() );
   LoadFields();

	UpdateInterface();	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
   }


void MapLayerDlg::LoadFields()
   {   
   // fields
   for ( int i=0; i < m_pLayer->GetFieldCount(); i++ )
      m_fieldsCombo.AddString( m_pLayer->GetFieldLabel( i ) );

   m_fieldsCombo.SetCurSel( 0 );
   UpdateFieldInfo();
   }


void MapLayerDlg::UpdateFieldInfo()
   {
   int index = m_fieldsCombo.GetCurSel();
   ASSERT( index >= 0 );

   CString field;
   m_fieldsCombo.GetLBText( index, field );

   int col = m_pLayer->GetFieldCol( field );
   ASSERT( col >= 0 );

   // type
   TYPE type = m_pLayer->GetFieldType( col );
   switch ( type )
      {
      case TYPE_CHAR:   m_typeCombo.SetCurSel( 1 );   break;
      case TYPE_BOOL:   m_typeCombo.SetCurSel( 0 );   break;
      case TYPE_SHORT:  m_typeCombo.SetCurSel( 2 );   break;
      case TYPE_UINT:   m_typeCombo.SetCurSel( 4 );   break;
      case TYPE_INT:    m_typeCombo.SetCurSel( 3 );   break;
      case TYPE_ULONG:  m_typeCombo.SetCurSel( 5 );   break;
      case TYPE_LONG:   m_typeCombo.SetCurSel( 5 );   break;
      case TYPE_FLOAT:  m_typeCombo.SetCurSel( 6 );   break;
      case TYPE_DOUBLE: m_typeCombo.SetCurSel( 7 );   break;
      case TYPE_STRING: m_typeCombo.SetCurSel( 8 );   break;
      case TYPE_DSTRING:m_typeCombo.SetCurSel( 8 );   break;
      default: ASSERT( 0 );
      }

   // ignore width/decimals for now
   }


void MapLayerDlg::OnShowoutlines() 
   {
   UpdateInterface();
   }


void MapLayerDlg::UpdateInterface( void )
   {
   UpdateData( 1 );

   m_pLayer->m_name = m_layerName;
   m_pLayer->m_isVisible =  m_visible != 0 ? true : false;

   m_outlineColor = m_outlineColorBtn.GetColor();

   if ( m_showOutlines )
      {
      m_pLayer->SetOutlineColor( m_outlineColor );
      m_pLayer->SetLineWidth( m_outlineWidth.GetCurSel() );
      }
   else
      m_pLayer->SetNoOutline();


   int transparency = m_transparencySlider.GetPos();
   m_pLayer->SetTransparency( transparency );

   m_pMapWnd->RedrawWindow();
   }


void MapLayerDlg::OnBnClickedEditfieldinfo()
   {
   // TODO: Add your control notification handler code here
   }

void MapLayerDlg::OnOK()
   {
   UpdateInterface();

   CDialog::OnOK();
   }


void MapLayerDlg::OnBnClickedEditlabelinfo()
   {
   MapLabelDlg dlg( m_pLayer->GetMapPtr(), this );

   dlg.DoModal();

   // TODO: Add your control notification handler code here
   }


void MapLayerDlg::OnBnClickedAddfield()
   {
   }


void MapLayerDlg::OnBnClickedRemovefield()
   {
   int index = m_fieldsCombo.GetCurSel();

   if ( index < 0 )
      return;

   CString field;
   m_fieldsCombo.GetLBText( index, field );
 
   int col = m_pLayer->GetFieldCol( field );
   if ( col < 0 )
      return;

   m_pLayer->m_pDbTable->RemoveField( col );

   LoadFields();
   }


void MapLayerDlg::OnCbnSelchangeType()
   {
   int index = m_typeCombo.GetCurSel();

   if ( index < 0 )
      return;

   int fIndex = m_fieldsCombo.GetCurSel();

   if ( fIndex < 0 )
      return;

   CString field;
   m_fieldsCombo.GetLBText( fIndex, field );

   int col = m_pLayer->GetFieldCol( field );
   if ( col < 0 )
      return;

   TYPE type = TYPE_NULL;
   switch ( index )
      {
      case 1:  type = TYPE_CHAR;    break;
      case 0:  type = TYPE_BOOL;    break;
      case 4:  type = TYPE_UINT;    break;
      case 3:  type = TYPE_INT;     break;
      case 2:  type = TYPE_SHORT;   break;
      //case 5:  type = TYPE_ULONG;   break;
      case 5:  type = TYPE_LONG;    break;
      case 6:  type = TYPE_FLOAT;   break;
      case 7:  type = TYPE_DOUBLE;  break;
      case 8:  type = TYPE_DSTRING; break;
      default: ASSERT( 0 );
      }
   
   // type
   TYPE oldType = m_pLayer->GetFieldType( col );

   if ( oldType != type )
      {
      CString msg;
      LPCTSTR strOldType = ::GetTypeLabel( oldType );
      LPCTSTR strType = ::GetTypeLabel( type );
      msg.Format( "Change field type from %s to %s?", strOldType, strType );
      int retVal = MessageBox( msg, "Change Type?", MB_YESNOCANCEL );

      if ( retVal == IDYES )
         {
         int width, decimals;
         GetTypeParams( type, width, decimals );
         m_pLayer->SetFieldType( col, type, width, decimals, true );
         }
      }
   }


void MapLayerDlg::OnCbnSelchangeFieldcombo()
   {
   UpdateFieldInfo();
   }


void MapLayerDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
   {
   CDialog::OnHScroll(nSBCode, nPos, pScrollBar);   
   
   // which scrollbar?
   int id = pScrollBar->GetDlgCtrlID();

   if ( id == IDC_LIB_TRANSPARENCY )
      {
      switch( nSBCode )
         {
         case SB_LEFT:  //   Scroll to far left.
         case SB_ENDSCROLL: //   End scroll.
         case SB_LINELEFT:  //   Scroll left.
         case SB_LINERIGHT: //   Scroll right.
         case SB_PAGELEFT:  //   Scroll one page left.
         case SB_PAGERIGHT:  //   Scroll one page right.
         case SB_RIGHT: //   Scroll to far right.
         case SB_THUMBPOSITION: //   Scroll to absolute position. The current position is specified by the nPos parameter.
         case SB_THUMBTRACK:
            {
            int pos = m_transparencySlider.GetPos();
            TCHAR buffer[ 32 ];
            sprintf_s( buffer, 32, "%i", pos );
            GetDlgItem( IDC_LIB_TRANSPARENCY_VALUE )->SetWindowText( buffer ); 

            UpdateInterface();
            }
         }
      }
   }


void MapLayerDlg::OnBnClickedLibReorderCols()
   {
   FieldSorterDlg dlg( m_pLayer );
   dlg.DoModal();
   // TODO: Add your control notification handler code here
   }


