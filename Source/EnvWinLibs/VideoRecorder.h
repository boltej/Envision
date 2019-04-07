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
#pragma once
#include "EnvWinLibs.h"

//#include <avigenerator.h>
#include <PtrArray.h>
#include <vfw.h>

enum VRMETHOD { VRM_CALLDIRECT=0, VRM_USETIMER=1, VRM_USEMESSAGES=2, VRM_APPLICATION=3 };

void CALLBACK VideoTimerProc(HWND, UINT, UINT_PTR, DWORD); 


//------------------------------------------------------------------------
// VideoRecorder class
//
// This class provide a video recorder that can be attached the a window to record
// what is being displayed on the window to an AVI file.
//
// Usage:
//    1) Create a video recorder using one of the constructors.  This sets up a basic video recorder.
//       if you use the basic constructor, you have to set the path, framerate, and capture window
//    2) Initialize the recorder fr recording by calling StartCapture().  This setup up some
//       internal data structures to prepare for capturing windows.
//    3) Call CaptureFrame() for each frame you want to store in the video
//    4) Call EndCapture when you are done recording.  This will store the video to disk and
//       release recorder resources.
//
//  If you want to reuse the recorder for another recording, reset the path, attached window, or
//  framerate, then call StartCapture(), CaptureFrame(), and EndCapture()
//------------------------------------------------------------------------

class WINLIBSAPI VideoRecorder
{
public:
   VideoRecorder(void);
   VideoRecorder( LPCTSTR path, CWnd* pWnd, int frameRate );

   ~VideoRecorder(void);

   CString   m_name;           // name associated with this recorder
   CWnd     *m_pCaptureWnd;    // window to capture
   int       m_type;           // app-defined type
   UINT_PTR  m_extra;          // app-defined "extra" data associated with this recorder
   CString   m_path;           // path to write AVI file to.
   int       m_timerInterval;  // 0 to disable, otherwise, milliseconds
   bool      m_useTimer;       // record every clock tick at timer interval
   bool      m_useMessages;    // record when attached window recieves an update message (not currently supported)
   bool      m_useApplication; // application determines when to record frames
   int       m_frameRate;      // for avi file 
   bool      m_enabled;        // is this recorder active?
   int       m_quality;        // 0-100

public:
   //AVI production
   bool StartCapture( void );   // allocates generators for each track

   // to do frame by frame, use these
   int  CaptureFrame( );
   void EndCapture();                        // releases generators for each track, write avi file
   
   // to do use a timed capture, use these
   bool StartTimer( int interval = 100 );    // interval is in milliseconds
   void StopTimer();

   void SetType( int type ) { m_type = type; }
   void SetName( LPCTSTR name ) { m_name = name; } 
   void SetPath( LPCTSTR path ) { m_path = path; }
   void SetFrameRate( int frameRate ) { m_frameRate = frameRate; }
   void SetCaptureMethod( int method ) { m_useTimer = ( method & VRM_USETIMER ) ? true : false;  m_useMessages = ( method & VRM_USEMESSAGES ) ? true : false; }
   void SetExtra( UINT_PTR extra ) { m_extra = extra; }

protected:
   // this is all just for precalculating and storing things to avoid doing it per-frame
   CDC          m_bitmapDC;
   CBitmap      m_bitmap;
   BITMAPINFO   m_bi;
   LPBITMAPINFO m_lpbi;
   BYTE        *m_bitAddress;
   BITMAPINFOHEADER m_bih;   
   UINT_PTR     m_timerID;
   bool         m_isInitialized;

private:
   int InitEngine();

   HRESULT AddFrame(BYTE* bmBits);
 
   void ReleaseEngine();  // Release ressources allocated for movie and close file.

   void SetBitmapHeader(CWnd* pWnd);
   void SetBitmapHeader(LPBITMAPINFOHEADER lpbih);
   LPBITMAPINFOHEADER GetBitmapHeader()          {   return &m_bih;};
   //void SetFileName(LPCTSTR _sFileName)        {   m_sFile=_sFileName; MakeExtAvi();};
   //void SetRate(DWORD dwRate)                  {   m_dwRate=dwRate;};
   
   LPCTSTR GetLastErrorMessage() const { return m_sError;};

protected:   
   CString m_sError;

private:
   void MakeExtAvi();
   long m_lFrame;    // frame counter
   PAVIFILE m_pAVIFile;    // file interface ptr

   PAVISTREAM m_pStream;      // stream interface
   PAVISTREAM m_pStreamCompressed;    // compressed video stream
};

