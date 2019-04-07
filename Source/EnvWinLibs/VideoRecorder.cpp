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
#include "EnvWinLibs.h"

#pragma hdrstop

#include "VideoRecorder.h"
#include "Report.h"


VideoRecorder::VideoRecorder(void)
   : m_pCaptureWnd( NULL )
   , m_type( 0 )
   , m_extra( 0 )
   , m_enabled( true )
   , m_timerID( 0 )
   , m_frameRate( 30 )
   , m_quality( 75 )
   , m_isInitialized( false )
   , m_lpbi( NULL )
   , m_bitAddress( NULL ) 
   , m_path( "untitled.avi" )
   , m_timerInterval( 1000 )
   , m_useTimer( false )
   , m_useMessages( false )
   , m_useApplication( false )
   , m_pAVIFile( NULL )
   , m_pStream( NULL )
   , m_pStreamCompressed( NULL )
   {
   memset( &m_bih, 0, sizeof(BITMAPINFOHEADER) );
   }


VideoRecorder::VideoRecorder( LPCTSTR path, CWnd* pWnd, int frameRate )
   : m_pCaptureWnd( pWnd )
   , m_type( 0 )
   , m_extra( 0 )
   , m_timerID( 0 )
   , m_frameRate( frameRate )
   , m_quality( 75 )
   , m_isInitialized( false )
   , m_lpbi( NULL )
   , m_bitAddress( NULL ) 
   , m_path( path )
   , m_timerInterval( 1000 )
   , m_useTimer( false )
   , m_useMessages( false )
   , m_useApplication( false )
   , m_pAVIFile( NULL )
   , m_pStream( NULL )
   , m_pStreamCompressed( NULL )
   {
   memset( &m_bih, 0, sizeof(BITMAPINFOHEADER) );
   MakeExtAvi();     // make sure the file has an AVI extension
   SetBitmapHeader(pWnd);
   }


VideoRecorder::~VideoRecorder(void)
   {
   // make sure engine resources are released
   ReleaseEngine();

   if ( m_bitAddress != NULL ) 
      delete [] m_bitAddress;
   }


bool VideoRecorder::StartCapture( void )
   {
   if ( m_pCaptureWnd == NULL )
      return false;

   if ( m_isInitialized )
      return false;

   if ( m_enabled == false )
      return false;

   ReleaseEngine();     // clean up any previous capture info

   CDC *pDC = m_pCaptureWnd->GetDC();
   
   // get the window's dimensions
   CRect rect;
   m_pCaptureWnd->GetWindowRect(rect);

   // get output filename from user
   // CString movieFileName;
   // CFileDialog movieSaveDlg(FALSE, "avi", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Movie Files (*.avi)|*.avi");
   //
   //    if (movieSaveDlg.DoModal() == IDOK)
   //       {
   //       movieFileName = movieSaveDlg.GetPathName();
   //       }
   //    // need to handle canceling out of the file dialog box somehow


   if ( InitEngine() < 0 )
      {
      Report::ErrorMsg( "VideoRecorder: Failure Initializing engine" );
      return false;
      }
   
   m_bitmapDC.CreateCompatibleDC( pDC );
   m_bitmap.CreateCompatibleBitmap(pDC, rect.Width(), rect.Height());
   m_bitmapDC.SelectObject( &m_bitmap );
   m_bi.bmiHeader = m_bih;
   m_lpbi         = &m_bi;
   m_bitAddress = new BYTE[3*rect.Width()*rect.Height()];
   
   m_pCaptureWnd->ReleaseDC( pDC );
   m_isInitialized = true;
   return true;
   }


int VideoRecorder::CaptureFrame( void )
   {
   if ( m_pCaptureWnd == NULL )
      return -1;

   if ( m_isInitialized == false )
      return -2;

   if ( m_enabled == false )
      return -3;

   // get a device context to the entire application window (view)
   //CDC *pDC = m_pCaptureWnd->GetWindowDC();
   CRect rect;
   m_pCaptureWnd->GetWindowRect(rect);
   
   CDC *pDC = m_pCaptureWnd->GetDC();

   // copy from the application window to the new device context (and thus the bitmap)
   BOOL blitSuc = m_bitmapDC.BitBlt( 0, 0, rect.Width(), rect.Height(), pDC, 0, 0, SRCCOPY );

   if ( blitSuc == FALSE )
      {
      Report::ErrorMsg( "VideoRecorder:  Failure copying window bitmap to frame" );
      }
   else
      {
      int success = ::GetDIBits( HDC( m_bitmapDC ), 
                HBITMAP( m_bitmap ),
                0, rect.Height(), 
                m_bitAddress,
                m_lpbi,
                DIB_RGB_COLORS );

      if ( success == 0 )
         Report::ErrorMsg( "VideoRecorder: Failure copying bitmap to buffer" );
      else
         {
         //CImage img;
         //img.Attach( pTrack->m_pAuxMovieInfo->bitmap );
         //img.Save( "\\Envision\\StudyAreas\\WW2100\\Outputs\\AVIs\\temp.bmp" );
         //img.Detach();

         AddFrame((BYTE*) m_bitAddress );
         }
      }

   m_pCaptureWnd->ReleaseDC( pDC );

   return 1;
   }


void VideoRecorder::EndCapture( void )
   {
   ReleaseEngine();

   if ( m_enabled )
      {
      m_bitmapDC.DeleteDC();
      m_bitmap.DeleteObject();
      }

   m_isInitialized = false;
   }


bool VideoRecorder::StartTimer( int interval )
   {
   if ( m_isInitialized == false )
      return false;

   if ( m_pCaptureWnd == NULL )
      return false;

   m_timerID = ::SetTimer( NULL, (INT_PTR) this, interval, VideoTimerProc );
   return true;
   }


void VideoRecorder::StopTimer( void )
   {
   if ( m_timerID > 0 )
      ::KillTimer( NULL, m_timerID );
   
   EndCapture();
   }


void CALLBACK VideoTimerProc(HWND, UINT eventID, UINT_PTR timerID, DWORD timeSoFar )
   {
   //ASSERT( m_timerID == timerID );
   }


///////////////////////////////////////////////////////////////////////
// Supporting structure
///////////////////////////////////////////////////////////////////////

void VideoRecorder::SetBitmapHeader(LPBITMAPINFOHEADER lpbih)
   {
   // checking that bitmap size are multiple of 4
   ASSERT( lpbih->biWidth  % 4 == 0);
   ASSERT( lpbih->biHeight % 4 == 0);

   // copying bitmap info structure.
   memcpy(&m_bih,lpbih, sizeof(BITMAPINFOHEADER));
   }


void VideoRecorder::SetBitmapHeader(CWnd *pWnd)
   {
   ASSERT_VALID(pWnd);

   ////////////////////////////////////////////////
   // Getting screen dimensions
   // Get client geometry 
   CRect rect; 
   pWnd->GetClientRect(&rect); 
   CSize size(rect.Width(),rect.Height()); 

   /////////////////////////////////////////////////
   // changing size of image so dimension are multiple of 4
   size.cx=(size.cx/4)*4;
   size.cy=(size.cy/4)*4;

   // initialize m_bih
   memset(&m_bih,0, sizeof(BITMAPINFOHEADER));

   // filling bitmap info structure.
   m_bih.biSize=sizeof(BITMAPINFOHEADER);
   m_bih.biWidth=size.cx;
   m_bih.biHeight=size.cy;
   m_bih.biPlanes=1;
   m_bih.biBitCount=24;
   m_bih.biSizeImage=((m_bih.biWidth*m_bih.biBitCount+31)/32 * 4)*m_bih.biHeight; 
   m_bih.biCompression=BI_RGB;      //BI_RGB means BRG in reality
   }


int VideoRecorder::InitEngine()
   {
   AVISTREAMINFO strHdr; // information for a single stream 
   AVICOMPRESSOPTIONS opts;
   AVICOMPRESSOPTIONS FAR * aopts[1] = {&opts};

   TCHAR szBuffer[1024];
   HRESULT hr;

   m_sError=_T("Ok");

   // Step 0 : Let's make sure we are running on 1.1 
   DWORD wVer = HIWORD(VideoForWindowsVersion());
   if (wVer < 0x010a)
      {
       // oops, we are too old, blow out of here 
      m_sError=_T("Version of Video for Windows too old - please update!");
      return -1;
      }

   // Step 1 : initialize AVI engine
   AVIFileInit();

   // Step 2 : Open the movie file for writing....
   hr = AVIFileOpen(&m_pAVIFile,         // Address to contain the new file interface pointer
             (LPCTSTR)m_path,             // Null-terminated string containing the name of the file to open
             OF_WRITE | OF_CREATE,       // Access mode to use when opening the file. 
             NULL);                      // use handler determined from file extension.
                                         // Name your file .avi -> very important

   if ( hr != AVIERR_OK )
      {
      _tprintf( szBuffer, _T("AVI Engine failed to initialize. Check filename %s."), m_path );
      m_sError = szBuffer;

      // Check it succeded.
      switch(hr)
         {
         case AVIERR_BADFORMAT: 
            m_sError += _T("The file couldn't be read, indicating a corrupt file or an unrecognized format.");
            break;
         case AVIERR_MEMORY:      
            m_sError += _T("The file could not be opened because of insufficient memory."); 
            break;
         case AVIERR_FILEREAD:
            m_sError += _T("A disk error occurred while reading the file."); 
            break;
         case AVIERR_FILEOPEN:      
            m_sError += _T("A disk error occurred while opening the file.");
            break;
         case REGDB_E_CLASSNOTREG:      
            m_sError += _T("According to the registry, the type of file specified in AVIFileOpen does not have a handler to process it");
            break;
         }

      return -2;
      }

   // Fill in the header for the video stream....
   memset(&strHdr, 0, sizeof(strHdr));
   strHdr.fccType                = streamtypeVIDEO;   // video stream type
   strHdr.fccHandler             = 0;
   strHdr.dwScale                = 1;                 // should be one for video
   strHdr.dwRate                 = m_frameRate;       // fps
   strHdr.dwSuggestedBufferSize  = m_bih.biSizeImage;   // Recommended buffer size, in bytes, for the stream.
   SetRect(&strHdr.rcFrame, 0, 0,          // rectangle for stream
       (int) m_bih.biWidth,
       (int) m_bih.biHeight);

   // Step 3 : Create the stream;
   hr = AVIFileCreateStream(m_pAVIFile,          // file pointer
                  &m_pStream,          // returned stream pointer
                  &strHdr);       // stream header

   // Check it succeded.
   if (hr != AVIERR_OK)
      {
      m_sError=_T("AVI Stream creation failed. Check Bitmap info.");

      if ( hr == AVIERR_READONLY )
         m_sError+=_T(" Read only file.");

      return -3;
      }

   // Step 4: Get codec and infos about codec
   memset(&opts, 0, sizeof(opts));

   // Poping codec dialog
   CWnd *pParent = AfxGetMainWnd();
   if (!AVISaveOptions(pParent->GetSafeHwnd(), ICMF_CHOOSE_PREVIEW, 1, &m_pStream, (LPAVICOMPRESSOPTIONS FAR *) &aopts))
      {
      AVISaveOptionsFree(1,(LPAVICOMPRESSOPTIONS FAR *) &aopts);
      return -4;
      }

   // Step 5:  Create a compressed stream using codec options.
   hr = AVIMakeCompressedStream(&m_pStreamCompressed, 
            m_pStream, 
            &opts, 
            NULL);

   if (hr != AVIERR_OK)
      {
      m_sError=_T("AVI Compressed Stream creation failed.");
      
      switch(hr)
         {
         case AVIERR_NOCOMPRESSOR:
            m_sError+=_T(" A suitable compressor cannot be found.");
               break;
         case AVIERR_MEMORY:
            m_sError+=_T(" There is not enough memory to complete the operation.");
               break; 
         case AVIERR_UNSUPPORTED:
            m_sError+=_T("Compression is not supported for this type of data. This error might be returned if you try to compress data that is not audio or video.");
            break;
         }

      return -5;
      }

   // releasing memory allocated by AVISaveOptionFree
   hr = AVISaveOptionsFree(1,(LPAVICOMPRESSOPTIONS FAR *) &aopts);
   if ( hr != AVIERR_OK)
      {
      m_sError=_T("Error releasing memory");
      return -6;   
      }

   // Step 6 : sets the format of a stream at the specified position
   hr = AVIStreamSetFormat(m_pStreamCompressed, 
               0,         // position
               &m_bih,       // stream format
               m_bih.biSize +   // format size
               m_bih.biClrUsed * sizeof(RGBQUAD));

   if (hr != AVIERR_OK)
      {
      m_sError=_T("AVI Compressed Stream format setting failed.");
      return -7;
      }

   // Step 6 : Initialize step counter
   m_lFrame=0;

   return 1;
   }


void VideoRecorder::ReleaseEngine()
   {
   if (m_pStream)
      {
      AVIStreamRelease(m_pStream);
      m_pStream=NULL;
      }

   if (m_pStreamCompressed)
      {
      AVIStreamRelease(m_pStreamCompressed);
      m_pStreamCompressed=NULL;
      }

   if (m_pAVIFile)
      {
      AVIFileRelease(m_pAVIFile);
      m_pAVIFile=NULL;
      }

   // Close engine
   AVIFileExit();
   }

HRESULT VideoRecorder::AddFrame(BYTE *bmBits)
   {
   HRESULT hr;

   // compress bitmap
   hr = AVIStreamWrite(m_pStreamCompressed,   // stream pointer
      m_lFrame,                  // time of this frame
      1,                  // number to write
      bmBits,               // image buffer
      m_bih.biSizeImage,      // size of this frame
      AVIIF_KEYFRAME,         // flags....
      NULL,
      NULL);

   // updating frame counter
   m_lFrame++;

   return hr;
   }



void VideoRecorder::MakeExtAvi()
   {
   // finding avi
   if ( m_path.Find( _T( "avi" ) ) == NULL )
   //if( _tcsstr((char*) m_path, _T("avi"))==NULL )
      {
      m_path += _T(".avi");
      }
   }
