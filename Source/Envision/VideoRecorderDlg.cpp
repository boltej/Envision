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
// VideoRecorderDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "VideoRecorderDlg.h"
#include "afxdialogex.h"

#include "EnvView.h"


extern CEnvView *gpView;


// VideoRecorderDlg dialog

IMPLEMENT_DYNAMIC(VideoRecorderDlg, CDialogEx)

VideoRecorderDlg::VideoRecorderDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(VideoRecorderDlg::IDD, pParent)
   , m_frameRate(0)
   , m_quality(0)
   , m_path(_T(""))
   , m_timerInterval(0)
   , m_useTimer(FALSE)
   , m_useMessage(FALSE)
   , m_useApplication(FALSE)
   { }


VideoRecorderDlg::~VideoRecorderDlg()
{ }


void VideoRecorderDlg::DoDataExchange(CDataExchange* pDX)
{
CDialogEx::DoDataExchange(pDX);
DDX_Control(pDX, IDC_WINDOWS, m_recorders);
DDX_Text(pDX, IDC_FRAMERATE, m_frameRate);
DDX_Text(pDX, IDC_QUALITY, m_quality);
DDV_MinMaxInt(pDX, m_quality, 0, 100);
DDX_Text(pDX, IDC_PATH, m_path);
DDX_Text(pDX, IDC_INTERVAL, m_timerInterval);
DDV_MinMaxInt(pDX, m_timerInterval, 0, 10000);
DDX_Check(pDX, IDC_CAPTUREINTERVAL, m_useTimer);
DDX_Check(pDX, IDC_ONUPDATE, m_useMessage);
DDX_Check(pDX, IDC_APPLICATION, m_useApplication);
}


BEGIN_MESSAGE_MAP(VideoRecorderDlg, CDialogEx)
   ON_LBN_SELCHANGE(IDC_WINDOWS, &VideoRecorderDlg::OnLbnSelchangeWindows)
   ON_EN_CHANGE(IDC_FRAMERATE, &VideoRecorderDlg::OnEnChangeFramerate)
   ON_EN_CHANGE(IDC_QUALITY, &VideoRecorderDlg::OnEnChangeQuality)
   ON_BN_CLICKED(IDC_CAPTUREINTERVAL, &VideoRecorderDlg::OnBnClickedCaptureinterval)
   ON_EN_CHANGE(IDC_INTERVAL, &VideoRecorderDlg::OnEnChangeInterval)
   ON_BN_CLICKED(IDC_ONUPDATE, &VideoRecorderDlg::OnBnClickedMessage)
   ON_BN_CLICKED(IDC_APPLICATION, &VideoRecorderDlg::OnBnClickedApplication)
   ON_EN_CHANGE(IDC_PATH, &VideoRecorderDlg::OnEnChangePath)
   ON_CLBN_CHKCHANGE(IDC_WINDOWS, OnCheckRecorders)
END_MESSAGE_MAP()


// VideoRecorderDlg message handlers
BOOL VideoRecorderDlg::OnInitDialog()
   {
   CDialogEx::OnInitDialog();

   // add existing recorders
   for ( int i=0; i < gpView->GetVideoRecorderCount(); i++ )
      {
      VideoRecorder *pVR = gpView->GetVideoRecorder( i );
      m_recorders.AddString( pVR->m_name );
      m_recorders.SetCheck( i, pVR->m_enabled ? 1 : 0 );
      }

   m_recorders.SetCurSel( 0 );

   LoadVR();

   return TRUE;
   }


void VideoRecorderDlg::LoadVR( void )
   {
   m_frameRate = 30;
   m_quality = 75;
   m_path = "";
   m_timerInterval = 1000;
   m_useTimer = FALSE;
   m_useMessage = FALSE;

   int index = m_recorders.GetCurSel();

   if ( index >= 0 )
      {
      VideoRecorder *pVR = gpView->GetVideoRecorder( index );

      m_frameRate  = pVR->m_frameRate;
      m_quality    = pVR->m_quality;
      m_path       = pVR->m_path;
      m_timerInterval = pVR->m_timerInterval;
      m_useTimer   = pVR->m_useTimer ? 1 : 0;
      m_useMessage = pVR->m_useMessages ? 1 : 0;
      m_useApplication = pVR->m_useApplication ? 1 : 0;
      }

   UpdateData( 0 );
   EnableControls();
   m_dirty = false;
   }



void VideoRecorderDlg::OnLbnSelchangeWindows()
   {
   LoadVR();
   }


void VideoRecorderDlg::OnCheckRecorders()
   {
   int index = m_recorders.GetCurSel();

   if ( index >= 0 )
      {
      VideoRecorder *pVR = gpView->GetVideoRecorder( index );
      pVR->m_enabled = m_recorders.GetCheck( index ) ? true : false;
      }

   m_dirty = true;
   }


void VideoRecorderDlg::OnEnChangeFramerate()
   {
   if ( m_recorders.m_hWnd == 0 )
      return;

   UpdateData( 1 );

   int index = m_recorders.GetCurSel();

   if ( index >= 0 )
      {
      VideoRecorder *pVR = gpView->GetVideoRecorder( index );
      pVR->m_frameRate = m_frameRate;
      }

   m_dirty = true;
   }


void VideoRecorderDlg::OnEnChangeQuality()
   {
   if ( GetDlgItem( IDC_QUALITY ) == NULL )
      return;

   if ( m_recorders.m_hWnd == 0 )
      return;

   UpdateData( 1 );

   int index = m_recorders.GetCurSel();

   if ( index >= 0 )
      {
      VideoRecorder *pVR = gpView->GetVideoRecorder( index );
      pVR->m_quality = m_quality;
      }

   m_dirty = true;
   }


void VideoRecorderDlg::OnBnClickedCaptureinterval()
   {
   BOOL checked = IsDlgButtonChecked( IDC_CAPTUREINTERVAL );

   int index = m_recorders.GetCurSel();

   if ( index >= 0 )
      {
      VideoRecorder *pVR = gpView->GetVideoRecorder( index );
      pVR->m_useTimer = checked ? true : false;
      }

   EnableControls();

   m_dirty = true;
   }


void VideoRecorderDlg::OnEnChangeInterval()
   {
   if ( GetDlgItem( IDC_INTERVAL ) == NULL )
      return;

   UpdateData( 1 );

   int index = m_recorders.GetCurSel();

   if ( index >= 0 )
      {
      VideoRecorder *pVR = gpView->GetVideoRecorder( index );
      pVR->m_timerInterval = m_timerInterval;
      }

   m_dirty = true;
   }


void VideoRecorderDlg::OnBnClickedMessage()
   {
   BOOL checked = IsDlgButtonChecked( IDC_ONUPDATE );

   int index = m_recorders.GetCurSel();

   if ( index >= 0 )
      {
      VideoRecorder *pVR = gpView->GetVideoRecorder( index );
      pVR->m_useMessages = checked ? true : false;
      }

   EnableControls();

   m_dirty = true;
   }


void VideoRecorderDlg::OnBnClickedApplication()
   {
   BOOL checked = IsDlgButtonChecked( IDC_APPLICATION );

   int index = m_recorders.GetCurSel();

   if ( index >= 0 )
      {
      VideoRecorder *pVR = gpView->GetVideoRecorder( index );
      pVR->m_useApplication = checked ? true : false;
      }

   EnableControls();

   m_dirty = true;
   }



void VideoRecorderDlg::OnEnChangePath()
   {
   if ( GetDlgItem( IDC_PATH ) == NULL )
      return;

   UpdateData( 1 );

   int index = m_recorders.GetCurSel();

   if ( index >= 0 )
      {
      VideoRecorder *pVR = gpView->GetVideoRecorder( index );
      pVR->SetPath( m_path );
      }

   m_dirty = true;
   }



void VideoRecorderDlg::EnableControls()
   {
   int index = m_recorders.GetCurSel();
   bool enabled = true;
   bool intervalEnabled = false;

   if ( index >= 0 )
      {
      VideoRecorder *pVR = gpView->GetVideoRecorder( index );
      enabled = pVR->m_enabled;
      intervalEnabled = pVR->m_useTimer;
      }

   GetDlgItem( IDC_FRAMERATE       )->EnableWindow( enabled ? 1 : 0 );
   GetDlgItem( IDC_FRAMERATELABEL  )->EnableWindow( enabled ? 1 : 0 );
   GetDlgItem( IDC_SECONDSLABEL    )->EnableWindow( enabled ? 1 : 0 );
   
   GetDlgItem( IDC_CAPTUREQUALITYLABEL )->EnableWindow( enabled ? 1 : 0 );
   GetDlgItem( IDC_QUALITY         )->EnableWindow( enabled ? 1 : 0 );
   GetDlgItem( IDC_PERCENTLABEL    )->EnableWindow( enabled ? 1 : 0 );

   GetDlgItem( IDC_CAPTUREINTERVAL   )->EnableWindow( enabled ? 1 : 0 );
   GetDlgItem( IDC_INTERVAL          )->EnableWindow( intervalEnabled ? 1 : 0 );
   GetDlgItem( IDC_MILLISECONDSLABEL )->EnableWindow( intervalEnabled ? 1 : 0 );
   
   GetDlgItem( IDC_ONUPDATE  )->EnableWindow( enabled ? 1 : 0 );

   GetDlgItem( IDC_PATHLABEL )->EnableWindow( enabled ? 1 : 0 );
   GetDlgItem( IDC_PATH      )->EnableWindow( enabled ? 1 : 0 );
   }
