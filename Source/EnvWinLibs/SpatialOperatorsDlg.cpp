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
// SpatialOperatorsDlg.cpp : implementation file
//

#include "EnvWinLibs.h"
#pragma hdrstop

#include ".\spatialoperatorsdlg.h"


// SpatialOperatorsDlg dialog

IMPLEMENT_DYNAMIC(SpatialOperatorsDlg, CDialog)
SpatialOperatorsDlg::SpatialOperatorsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(SpatialOperatorsDlg::IDD, pParent),
     m_distance( 100 ),
     m_areaThreshold( 0.50 )
{ }

SpatialOperatorsDlg::~SpatialOperatorsDlg()
{ }

void SpatialOperatorsDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_LIB_COMMENT, m_comment);
}


BEGIN_MESSAGE_MAP(SpatialOperatorsDlg, CDialog)
   ON_BN_CLICKED(IDC_LIB_NEXTTO, OnBnClickedNextto)
   ON_BN_CLICKED(IDC_LIB_NEXTTOAREA, OnBnClickedNexttoarea)
   ON_BN_CLICKED(IDC_LIB_WITHIN, OnBnClickedWithin)
   ON_BN_CLICKED(IDC_LIB_WITHINAREA, OnBnClickedWithinarea)
END_MESSAGE_MAP()


// SpatialOperatorsDlg message handlers

BOOL SpatialOperatorsDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   this->CheckDlgButton( IDC_LIB_NEXTTO, TRUE );
   OnBnClickedNextto();

   SetDlgItemText( IDC_LIB_DISTANCE, "100" );
   SetDlgItemText( IDC_LIB_AREA, "0.30" );

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }

void SpatialOperatorsDlg::OnOK()
   {
   char buffer[ 64 ];
   GetDlgItemText( IDC_LIB_DISTANCE, buffer, 63 );
   m_distance = (float) atof( buffer );

   GetDlgItemText( IDC_LIB_AREA, buffer, 63 );
   m_areaThreshold = (float) atof( buffer );

   CDialog::OnOK();
   }

void SpatialOperatorsDlg::OnBnClickedNextto()
   {
   m_group = 0;
   SetDlgItemText( IDC_LIB_COMMENT, "NextTo( <query> ) is TRUE if any adjacent polygon satisfies the query, FALSE otherwise.  EXAMPLE:  NextTo( \"LULC_A=2\" ) is true if the specified polygon is adjacent to any polygon of type LULC_A=2" );

   GetDlgItem( IDC_LIB_DISTANCE )->EnableWindow( FALSE );
   GetDlgItem( IDC_LIB_DISTANCELABEL )->EnableWindow( FALSE );
   GetDlgItem( IDC_LIB_AREA )->EnableWindow( FALSE );
   GetDlgItem( IDC_LIB_AREALABEL )->EnableWindow( FALSE );
   }

void SpatialOperatorsDlg::OnBnClickedNexttoarea()
   {
   m_group = 0;
   SetDlgItemText( IDC_LIB_COMMENT, "NextToArea( <query> ) returns the area of contiguous polygons next to the target polygon that satisfy the query  EXAMPLE:  NextToArea( \"LULC_A=2\" ) > 500 is true if the area of contiguous polygons with LULC_A=2 is greater than 500" );

   GetDlgItem( IDC_LIB_DISTANCE )->EnableWindow( FALSE );
   GetDlgItem( IDC_LIB_DISTANCELABEL )->EnableWindow( FALSE );
   GetDlgItem( IDC_LIB_AREA )->EnableWindow( FALSE );
   GetDlgItem( IDC_LIB_AREALABEL )->EnableWindow( FALSE );
   }

void SpatialOperatorsDlg::OnBnClickedWithin()
   {
   m_group = 1;
   SetDlgItemText( IDC_LIB_COMMENT, "Within( <query>, <distance> ) is TRUE if any polygon within the specified distance satisfies the query, FALSE otherwise.  EXAMPLE:  Within( \"LULC_A=2\", 100 ) is true if any polygon of type LULC_A=2 is within 100 distance units of the specified polygon" );

   GetDlgItem( IDC_LIB_DISTANCE )->EnableWindow( TRUE );
   GetDlgItem( IDC_LIB_DISTANCELABEL )->EnableWindow( TRUE );
   GetDlgItem( IDC_LIB_AREA )->EnableWindow( FALSE );
   GetDlgItem( IDC_LIB_AREALABEL )->EnableWindow( FALSE );
   }

void SpatialOperatorsDlg::OnBnClickedWithinarea()
   {
   m_group = 2;
   SetDlgItemText( IDC_LIB_COMMENT, 
      "WithinArea( <query>, <distance>, <areaThreshold> ) is TRUE if the TOTAL AREA of polygons within the specified distance (radius) is GREATER THAN the specified area threshold.  NOTE:  The area threshold is expressed as a decimal percent.  EXAMPLE:  WithinArea( \"LULC_A=2\", 100, 0.30 ) is true if more than 30% of the area within 100 distance units from the specified polygon are of type LULC=2" );
   
   GetDlgItem( IDC_LIB_DISTANCE )->EnableWindow( TRUE );
   GetDlgItem( IDC_LIB_DISTANCELABEL )->EnableWindow( TRUE );
   GetDlgItem( IDC_LIB_AREA )->EnableWindow( TRUE );
   GetDlgItem( IDC_LIB_AREALABEL )->EnableWindow( TRUE );
   }
