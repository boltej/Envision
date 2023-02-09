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
// ResultsWnd.cpp : implementation file
//

#include "stdafx.h"
#include ".\resultswnd.h"
#include "resource.h"
#include <Policy.h>
#include <EnvModel.h> 
#include "ActiveWnd.h"
#include "EnvView.h"
#include <vdataobj.h>
#include <Map.h>
#include <maplayer.h>
#include "resultsMapWnd.h"

//#include <avigenerator.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern MapLayer      *gpCellLayer;
extern PolicyManager *gpPolicyManager;
extern EnvModel      *gpModel;
extern CEnvView      *gpView;


/////////////////////////////////////////////////////////////////////////////
// ResultsChildWnd
//
// this constructor takes a child window and makes a ResultsChildWnd wrapper
// for it.  The resulting ResultsChildWnd becomes the parent of the
// passed in window. 

ResultsChildWnd::ResultsChildWnd( CWnd *pWnd, RCW_TYPE type, REPLAYFN pfnReplay, SETYEARFN pfnSetYear )
 : m_pWnd( pWnd )
 , m_type( type )
 , m_pfnReplay( pfnReplay )
 , m_pfnSetYear( pfnSetYear )
   {
   ASSERT( m_pWnd != NULL );

   CString name;
   m_pWnd->GetWindowText( name );

   DWORD style = m_pWnd->GetStyle();
   m_pWnd->ModifyStyle( style, WS_VISIBLE | WS_CHILD, SWP_FRAMECHANGED );

   CWnd *pParentWnd = m_pWnd->GetParent();
   ASSERT( pParentWnd );

   CRect rect;
   m_pWnd->GetWindowRect( rect );
   pParentWnd->ScreenToClient( rect );

   CWnd::Create( NULL, name, style, rect, pParentWnd, 42 );

   m_pWnd->SetParent( this );

   if( type == RCW_MAP )
      {
      ResultsMapWnd *pResultsMapWnd = (ResultsMapWnd*) pWnd;
      pResultsMapWnd->m_pMap->InstallNotifyHandler( ResultsMapProc, (LONG_PTR) pResultsMapWnd );
      }
   }

ResultsChildWnd::~ResultsChildWnd()
   {
   if ( m_pWnd != NULL )
      delete m_pWnd;
   }


BEGIN_MESSAGE_MAP(ResultsChildWnd, CWnd)
   ON_WM_SIZE()
END_MESSAGE_MAP()


void ResultsChildWnd::OnSize(UINT nType, int cx, int cy)
   {
   CWnd::OnSize(nType, cx, cy);

   ASSERT( m_pWnd != NULL );
   m_pWnd->MoveWindow( 0, 0, cx, cy );
   }

/////////////////////////////////////////////////////////////////////////////
// ResultsWnd

IMPLEMENT_DYNCREATE(ResultsWnd, CWnd)


BEGIN_MESSAGE_MAP(ResultsWnd, CWnd)
	ON_WM_ERASEBKGND()
   ON_WM_PARENTNOTIFY()
   ON_WM_PAINT()
   ON_WM_TIMER()
END_MESSAGE_MAP()


ResultsWnd::ResultsWnd()
: m_resultCount( 0 )
, m_pActiveWnd( NULL )
, m_syncMapZoom( true )
//, m_createMovie( false )
//, m_aviGenerators(NULL)
, m_year( 0 )
   { }


ResultsWnd::~ResultsWnd()
   {
   RemoveAll();
   //if(m_aviGenerators != NULL)
   //   delete [] m_aviGenerators;
   }


void ResultsWnd::RemoveAll()
   {
   for ( int i=0; i < m_wndArray.GetSize(); i++ )
      {
      ResultsChildWnd *pWnd = m_wndArray[ i ];
      delete pWnd;
      }

   m_wndArray.RemoveAll();
   }


/////////////////////////////////////////////////////////////////////////////
// ResultsWnd message handlers

BOOL ResultsWnd::OnEraseBkgnd(CDC* pDC) 
   {
   RECT rect;
   GetClientRect( &rect );

   pDC->FillSolidRect( &rect, RGB( 0xFF, 0xFF, 0xFF ) );
   return TRUE; // 	return CWnd::OnEraseBkgnd(pDC);
   }


void ResultsWnd::Tile() 
   {
   int count = (int) m_wndArray.GetSize();

   if ( count == 0 )
      return;

   int cols = 1;
   
   while( count >= cols * (cols+1) )
      cols++;

   // have rows, now get cols
   int *rows = new int[ cols ];

   for ( int i=0; i < cols; i++ )
      rows[ i ] = cols;  // start with this

   int changeCols = count - ( cols*cols );  // 0 for regular grid, positive if cols needs to increase, negative if cols needs to decrease

   for ( int i=0; i < abs( changeCols ); i++ )
      {
      if ( changeCols < 0 )
         rows[ i ] -= 1;
      else
         rows[ i ] += 1;
      }

   // have rows and cols, ready to tile
   RECT rect;
   GetClientRect( &rect );









   //int height = rect.bottom / rows;
   int width = rect.right / cols;


   int currentRow = 0;
   int currentCol = 0;
   int x = 0;
   int y = 0;

   for ( int i=0; i < count; i++ )
      {
      CWnd *pWnd = m_wndArray[ i ];

      //int width = rect.right / cols[ currentRow ];
      int height = rect.bottom / rows[ currentCol ];
      
      pWnd->MoveWindow( x, y, width, height );

      //x += width+1;
      //currentCol++;
      y += height+1;
      currentRow++;

      if ( currentRow == rows[ currentCol ] )
         {
         currentRow = 0;
         currentCol++;

         //x = 0;
         //y += height+1;
         y = 0;
         x += width + 1;
         }
      }
   
   // clean up
   ASSERT( rows != NULL );
   delete [] rows;
   }


void ResultsWnd::Cascade() 
   {
   int count = (int) m_wndArray.GetSize();

   if ( count == 0 )
      return;
   
   RECT rect;
   GetClientRect( &rect );

   int borderHeight = GetSystemMetrics( SM_CYSIZEFRAME );
   int captionHeight = GetSystemMetrics( SM_CYCAPTION );
   int totalHeight = borderHeight + captionHeight + 1;

   int height = rect.bottom - totalHeight*(count-1);
   int width  = rect.right - totalHeight*(count-1);

   int x = 0;
   int y = 0;

   for ( int i=0; i < count; i++ )
      {
      CWnd *pWnd = m_wndArray[ i ];

      if ( !IsWindow( pWnd->m_hWnd ) )
         ASSERT( IsWindow( pWnd->m_hWnd ) );
      
      pWnd->MoveWindow( x, y, width, height );

      x += totalHeight;
      y += totalHeight;

      pWnd->BringWindowToTop();
      }
   }

void ResultsWnd::OnParentNotify(UINT message, LPARAM lParam)
   {
   CWnd::OnParentNotify(message, lParam);

   int msg = LOWORD( message );
   switch ( msg )
      {
      case WM_CREATE:
         break;
      case WM_DESTROY:
         {
         CWnd *pWnd = FromHandlePermanent( (HWND) lParam );
         if (pWnd!=NULL)
            {
            //remove this window from m_wndArray and delete
            for ( int i=0; i < m_wndArray.GetCount(); i++ )
               {
               if ( m_wndArray.GetAt(i) == pWnd ) 
                  {
                  m_wndArray.RemoveAt(i);
                  delete pWnd;
                  m_pActiveWnd = NULL;
                  ActiveWnd::SetActiveWnd( NULL );
                  }
               }
            }
         }
         break;

      case WM_LBUTTONDOWN:    // 
      case WM_MBUTTONDOWN:   // Simulates a MDI
      case WM_RBUTTONDOWN:  // 
         {
         // Get the child window that was clicked
         int x = LOWORD( lParam );
         int y = HIWORD( lParam );
         CWnd *pWnd = ChildWindowFromPoint( CPoint(x,y), CWP_ALL );

         if ( pWnd != NULL )
            MakeActive( pWnd );
         
         }
         break;
      }

   }


void ResultsWnd::MakeActive( CWnd *pWnd )
   {
   // This function is used to simulate a MDI
   // pWnd is activated and all other windows in m_wndArray are deactivated 
   if ( pWnd != NULL )
      ASSERT( IsWindow( pWnd->m_hWnd ) );

   // deactivate all windows
   for ( int i=0; i<m_wndArray.GetCount(); i++ )
      {
      CWnd *_pWnd = m_wndArray.GetAt(i);
      _pWnd->SendMessage( WM_NCACTIVATE, false );
      }

   if ( pWnd != NULL )
      {
      pWnd->BringWindowToTop();
      pWnd->SendMessage( WM_NCACTIVATE, true );

      ResultsChildWnd *pResultsChildWnd = (ResultsChildWnd*) pWnd;
      ActiveWnd::SetActiveWnd( pResultsChildWnd );
      }
   else
      ActiveWnd::SetActiveWnd( NULL );

   m_pActiveWnd = pWnd;
   }


void ResultsWnd::OnPaint()
   {
   CPaintDC dc(this); // device context for painting
   CFont font;
   font.CreatePointFont( 90, "Arial" );
   CFont *pFont = dc.SelectObject( &font );
   CString text( "Click on a tree item to display it in this view" );
   dc.TextOut( 15, 15, text );
   dc.SelectObject( pFont );
   }


void ResultsWnd::ReplayReset()
   {
   m_year = 0;
   int count = (int) m_wndArray.GetSize();
   for ( int i=0; i<count; i++ )
      {
      ResultsChildWnd *pWnd = m_wndArray[ i ];

      if ( pWnd->m_pfnReplay != NULL )
         pWnd->m_pfnReplay( pWnd->m_pWnd, -1 );  
      }
   }

void ResultsWnd::ReplayStop()
   {
   KillTimer( 4242 );
   //if (m_createMovie)
   //   CleanupMovieCapture();
   }

void ResultsWnd::ReplayPlay()
   {
   //if (m_createMovie)
   //   InitMovieCapture();

   SetTimer( 4242, 100, NULL );
   }


void ResultsWnd::SetYear( int oldYear, int newYear )
   {   
   m_year = newYear;
   int count = (int) m_wndArray.GetSize();
   for ( int i=0; i<count; i++ )
      {
      ResultsChildWnd *pWnd = m_wndArray[ i ];

      if ( pWnd->m_pfnSetYear != NULL )
         pWnd->m_pfnSetYear( pWnd->m_pWnd, oldYear, newYear );  
      }
   }


void ResultsWnd::OnTimer(UINT_PTR nIDEvent)
   {
   bool dontStop = false;
   bool ret = false;

   int count = (int) m_wndArray.GetSize();
   for ( int i=0; i < count; i++ )
      {
      ResultsChildWnd *pWnd = m_wndArray[ i ];

      if ( pWnd->m_pfnReplay != NULL )
         ret = pWnd->m_pfnReplay( pWnd->m_pWnd, m_year );  // returns false if its at the end of the replay

      if ( ret )  // if not at the end of the replay yet
         dontStop = true;
      }

   m_year++;

   //if (m_createMovie)
   //   WriteFrame();

   if ( ! dontStop )  // if all windows are are the end of the replay
      ReplayStop();

   CWnd::OnTimer(nIDEvent);
   }

/*
void ResultsWnd::WriteFrame()
   {
   // get a device context to the entire application window (view)
   CDC* dc = GetWindowDC();
  
   CRect rect;
   GetWindowRect(rect);
   int numMovies=1;
   for(int i = 0; i < numMovies; i++)
      {
      // copy from the application window to the new device context (and thus the bitmap)
      BOOL blitSuc = m_auxMovieStuff[i].bitmapDC.BitBlt(0, 0,
                                                      rect.Width(),
                                                      rect.Height(),
                                                      dc, 0, 0, SRCCOPY);

      GetDIBits(m_auxMovieStuff[i].bitmapDC, HBITMAP(m_auxMovieStuff[i].bitmap),
                0, rect.Height(), m_auxMovieStuff[i].bitAddress,
                m_auxMovieStuff[i].lpbi, DIB_RGB_COLORS);

      m_aviGenerators[i]->AddFrame((BYTE*)m_auxMovieStuff[i].bitAddress);
      }
      ReleaseDC(dc);
   }


void ResultsWnd::InitMovieCapture()
     {
     // init movie capturing       
     int numMovies = 1;
     int movieFrameRate = 5 ;

     m_aviGenerators = new CAVIGenerator*[numMovies];  // array of pointers to AVI generators
     m_auxMovieStuff = new AuxMovieStuff[numMovies];

     CDC* dc = GetWindowDC();
     // get the window's dimensions
     CRect rect;
     GetWindowRect(rect);

     for(int i = 0; i < numMovies; i++)
        {
        // get output filename from user
        CString movieFileName;
        CFileDialog movieSaveDlg(FALSE, "avi", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Movie Files (*.avi)|*.avi");

        if (movieSaveDlg.DoModal() == IDOK)
           {
           movieFileName = movieSaveDlg.GetPathName();
           }
        // need to handle canceling out of the file dialog box somehow

        m_aviGenerators[i] = new CAVIGenerator(movieFileName, (CView*)this, movieFrameRate);
        m_aviGenerators[i]->InitEngine();
        m_auxMovieStuff[i].bitmapDC.CreateCompatibleDC(dc);
        m_auxMovieStuff[i].bitmap.CreateCompatibleBitmap(dc, rect.Width(), rect.Height());
        m_auxMovieStuff[i].bitmapDC.SelectObject(&m_auxMovieStuff[i].bitmap);
        m_auxMovieStuff[i].bi.bmiHeader = *m_aviGenerators[i]->GetBitmapHeader();
        m_auxMovieStuff[i].lpbi = &m_auxMovieStuff[i].bi;
        m_auxMovieStuff[i].bitAddress = new BYTE[3*rect.Width()*rect.Height()];
        }
     ReleaseDC(dc);
     }

void ResultsWnd::CleanupMovieCapture()
      {
      int numMovies = 1;
      for(int i = 0; i < numMovies; i++)
         {
         m_aviGenerators[i]->ReleaseEngine();
         delete m_aviGenerators[i];
         m_aviGenerators[i] = 0;
         //ReleaseDC(NULL,m_auxMovieStuff[i].bitmapDC);
         delete [] m_auxMovieStuff[i].bitAddress;
         }
      delete [] m_aviGenerators;
      delete [] m_auxMovieStuff;
      m_aviGenerators = NULL;
      }
      */

/*
void ResultsWnd::SyncMapZoom( Map *pSourceMap, ResultsMapWnd *pResultsMapWnd )
   {
   float vxMin, vxMax, vyMin, vyMax;
   pSourceMap->GetViewExtents( vxMin, vxMax, vyMin, vyMax );

   for ( int i=0; i < m_wndArray.GetSize(); i++ )
      {
      ResultsChildWnd *pChild = m_wndArray[ i ];
      if ( pChild->m_type == RCW_MAP )
         {
         ResultsMapWnd *_pResultsMapWnd = (ResultsMapWnd*) pChild->m_pWnd;

         if ( _pResultsMapWnd != pResultsMapWnd )
            {
            _pResultsMapWnd->m_pMap->SetViewExtents( vxMin, vxMax, vyMin, vyMax );  // doesn't raise NT_EXTENTSCHANGED event
            _pResultsMapWnd->m_pMap->RedrawWindow();
            }
         }
      }  // end of: if ( syncMapZoom && type == NT_EXTENTSCHANGED
   }
   */
