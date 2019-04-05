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
// PPMetaGoals.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "Envdoc.h"
#include ".\ppmetagoals.h"

extern CEnvDoc *gpDoc;
extern EnvModel *gpModel;


// PPMetaGoals dialog

IMPLEMENT_DYNAMIC(PPMetaGoals, CPropertyPage)

PPMetaGoals::PPMetaGoals()
	: CPropertyPage(PPMetaGoals::IDD)
{
}

PPMetaGoals::~PPMetaGoals()
{
}

void PPMetaGoals::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_SLIDERALTRUISM, m_altruismSlider);
   DDX_Control(pDX, IDC_SLIDERSELF, m_selfInterestSlider);
   DDX_Control(pDX, IDC_SLIDERPOLICYPREF, m_policyPrefSlider);
}


BEGIN_MESSAGE_MAP(PPMetaGoals, CPropertyPage)
   ON_WM_HSCROLL()
   ON_WM_DESTROY()
   ON_BN_CLICKED(IDC_BALANCE, OnBnClickedBalance)
END_MESSAGE_MAP()


// PPMetaGoals message handlers

BOOL PPMetaGoals::OnInitDialog() 
   {
	CPropertyPage::OnInitDialog();

   CFont font;
   font.CreateStockObject( DEFAULT_GUI_FONT );

   CDC *pDC = GetDC();
   pDC->SelectObject( &font );

   int width=0;
   int height = 0;
   int cumHeight = 0;

   CSize sz = pDC->GetTextExtent( "Z", 1 );
   height = sz.cy;
   int metagoalCount = gpModel->GetMetagoalCount();
     
   for ( int i=0; i < metagoalCount; i++ )
      {
      int index = gpModel->GetEvaluatorIndexFromMetagoalIndex( i );

      if ( index >= 0 )
         {
         EnvEvaluator *pModel = gpModel->GetEvaluatorInfo( index );
         sz = pDC->GetTextExtent( pModel->m_name );
         if ( sz.cx > width ) 
            width = sz.cx;
         }
      }

   ReleaseDC( pDC );

   CWnd *pScroll = GetDlgItem( IDC_SCROLLBAR );
   RECT scrollRect;
   pScroll->GetWindowRect( &scrollRect );
   ScreenToClient( &scrollRect );
   
   int vSpacing = 1;
   
   if ( metagoalCount > 0 )
      vSpacing = ( scrollRect.bottom - scrollRect.top ) / metagoalCount;
   
   CWnd *pLabel = GetDlgItem( IDC_LABEL );
   pLabel->ShowWindow( SW_HIDE );
   RECT labelRect;
   pLabel->GetWindowRect( &labelRect );  // screen coords
   ScreenToClient( &labelRect );         // convert to client coords

   labelRect.right  = labelRect.left + width + 2;
   labelRect.bottom = labelRect.top + height;

   RECT sliderRect;
   sliderRect.left   = labelRect.right + 3;
   sliderRect.top    = scrollRect.top;
   sliderRect.right  = scrollRect.left - 3;
   sliderRect.bottom = sliderRect.top + height*3;
   
   for ( int i=0; i < metagoalCount; i++ )
      {
      METAGOAL *pGoal = gpModel->GetMetagoalInfo( i );
      CStatic *pStatic = new CStatic;
      CString label( pGoal->name );
      pStatic->Create( label, WS_CHILD | WS_VISIBLE | SS_RIGHT, labelRect, this );
      pStatic->SetFont( &font, TRUE );

      CSliderCtrl	*pCtrl = new CSliderCtrl;
      pCtrl->Create( WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ | TBS_TOP, 
         sliderRect, this, 500+i );

      labelRect.top += vSpacing;
      labelRect.bottom += vSpacing;
     
      sliderRect.top += vSpacing;
      sliderRect.bottom += vSpacing;

      m_sliderArray.Add( pCtrl );
      m_staticArray.Add( pStatic );

      pCtrl->SetRange( -3000, 3000, TRUE );
      pCtrl->SetTicFreq( 600 );
      pCtrl->SetPos( int( pGoal->weight * 1000 ) );
      }

   m_altruismSlider.SetRange( 0, 100, TRUE );
   m_altruismSlider.SetTicFreq( 10 );
   m_altruismSlider.SetPos( int(m_altruismWt*100) );
   m_altruismSlider.SetLineSize( 2 );
   m_altruismSlider.SetPageSize( 10 );   

   m_selfInterestSlider.SetRange( 0, 100, TRUE );
   m_selfInterestSlider.SetTicFreq( 10 );
   m_selfInterestSlider.SetPos( int(m_selfInterestWt*100) );
   m_selfInterestSlider.SetLineSize( 2 );
   m_selfInterestSlider.SetPageSize( 10 );   

   m_policyPrefSlider.SetRange( 0, 100, TRUE );
   m_policyPrefSlider.SetTicFreq( 10 );
   m_policyPrefSlider.SetPos( int(m_policyPrefWt*100) );
   m_policyPrefSlider.SetLineSize( 2 );
   m_policyPrefSlider.SetPageSize( 10 );   


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
   }


void PPMetaGoals::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
   {
   // figure out which scrollbar it is
   int thisID = pScrollBar->GetDlgCtrlID();

   if ( thisID == IDC_SLIDERALTRUISM || thisID == IDC_SLIDERSELF || thisID == IDC_SLIDERPOLICYPREF )
      {
      CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
      OnWtSlider( nSBCode, nPos, (CSliderCtrl*) pScrollBar );
      return;
      }

   if ( nSBCode == TB_THUMBTRACK && gpDoc->m_coupleSliders ) //|| nSBCode = TB_THUMBPOS )
      {
      int sliderCount =  (int) m_sliderArray.GetSize();
      int *positionArray = new int[ sliderCount ];

      for ( int i=0; i < sliderCount; i++ )
         positionArray[ i ] = m_sliderArray[ i ]->GetPos();

      positionArray[ thisID-500 ] = nPos;

      int total = 0;
      for ( int i=0; i < sliderCount; i++ )
         total += positionArray[ i ];

      float scalar = total / 6000.0f;

      for ( int i=0; i < sliderCount; i++ )
         m_sliderArray[ i ]->SetPos( int( positionArray[ i ]/scalar ) );

      delete [] positionArray;
      }

   CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
   }


void PPMetaGoals::OnDestroy()
   {
   int i;
   for ( i=0; i < m_sliderArray.GetSize(); i++ )
      delete m_sliderArray[ i ];

   for ( i=0; i < m_staticArray.GetSize(); i++ )
      delete m_staticArray[ i ];

   CPropertyPage::OnDestroy();
   }


void PPMetaGoals::OnBnClickedBalance()
   {
   int goalCount = (int) m_sliderArray.GetSize();

   for ( int i=0; i < goalCount; i++ )
      m_sliderArray[ i ]->SetPos( 0 );
   }


float PPMetaGoals::GetGoalWeight( int i )
   {
   ASSERT( i < m_sliderArray.GetSize() );
   int pos = m_sliderArray[ i ]->GetPos();

   return (pos/1000.0f);
   }

bool ppmInUpdate = false;

void PPMetaGoals::OnWtSlider(UINT nSBCode, UINT nPos, CSliderCtrl* pCtrl)
   {
   if ( ppmInUpdate )
      return;

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
   int id = pCtrl->GetDlgCtrlID();
   float wt = nPos/100.0f;

   switch( id )
      {
      case IDC_SLIDERALTRUISM:
         {
         float delta = wt - m_altruismWt;
         if ( delta == 0 )
            break;

         m_altruismWt = wt;

         if ( m_selfInterestWt > 0 || m_policyPrefWt > 0 )
            {
            float ratio = m_selfInterestWt / ( m_selfInterestWt + m_policyPrefWt );
            m_selfInterestWt -= ( delta * ratio );
            m_policyPrefWt = 1 - wt - m_selfInterestWt;
            }
         else  // both equal to zero
            {
            m_selfInterestWt = -delta/2;
            m_policyPrefWt = -delta/2;
            }
         }
         break;

      case IDC_SLIDERSELF:
         {
         float delta = wt - m_selfInterestWt;
         if ( delta == 0 )
            break;

         m_selfInterestWt = wt;
         if ( m_altruismWt > 0 || m_policyPrefWt > 0 )
            {
            float ratio = m_altruismWt / ( m_altruismWt + m_policyPrefWt );
            m_altruismWt -= ( delta * ratio );
            m_policyPrefWt = 1 - wt - m_altruismWt;       
            }
         else
            {
            m_altruismWt = -delta/2;
            m_policyPrefWt = -delta/2;
            }
         }
         break;

      case IDC_SLIDERPOLICYPREF:
         {
         float delta = wt - m_policyPrefWt;
         if ( delta == 0 )
            break;

         m_policyPrefWt = wt;
         if ( m_altruismWt > 0 || m_selfInterestWt > 0 )
            {
            float ratio = m_selfInterestWt / ( m_selfInterestWt + m_altruismWt );
            m_selfInterestWt -= ( delta * ratio );
            m_altruismWt = 1 - wt - m_selfInterestWt;       
            }
         else
            {
            m_altruismWt = -delta/2;
            m_selfInterestWt = -delta/2;
            }
         }
         break;

      default:
         ASSERT( 0 );
      }

   if ( m_altruismWt < 0 ) m_altruismWt = 0;
   if ( m_altruismWt > 1 ) m_altruismWt = 1;

   if ( m_selfInterestWt < 0 ) m_selfInterestWt = 0;
   if ( m_selfInterestWt > 1 ) m_selfInterestWt = 1;
      
   if ( m_policyPrefWt < 0 ) m_policyPrefWt = 0;
   if ( m_policyPrefWt > 1 ) m_policyPrefWt = 1;

   NormalizeWeights( m_altruismWt, m_selfInterestWt, m_policyPrefWt );

   if ( m_altruismWt < 0 ) m_altruismWt = 0;
   if ( m_altruismWt > 1 ) m_altruismWt = 1;

   if ( m_selfInterestWt < 0 ) m_selfInterestWt = 0;
   if ( m_selfInterestWt > 1 ) m_selfInterestWt = 1;
      
   if ( m_policyPrefWt < 0 ) m_policyPrefWt = 0;
   if ( m_policyPrefWt > 1 ) m_policyPrefWt = 1;

   ppmInUpdate = true;

   /*char buffer[ 32 ];
   sprintf_s( buffer, 32, "%4.2g", m_altruismWt );
   GetDlgItem( IDC_ALTRUISM )->SetWindowText( buffer ); 

   sprintf_s( buffer, 32, "%4.2g", m_selfInterestWt );
   GetDlgItem( IDC_SELF)->SetWindowText( buffer ); 

   sprintf_s( buffer, 32, "%4.2g", m_policyPrefWt );
   GetDlgItem( IDC_POLICYPREF )->SetWindowText( buffer ); 
*/
   m_altruismSlider.SetPos( int( m_altruismWt * 100 ) );
   m_selfInterestSlider.SetPos( int( m_selfInterestWt * 100 ) );
   m_policyPrefSlider.SetPos( int( m_policyPrefWt * 100 ) );

   ppmInUpdate = false;
   }
