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

#include <aviadapter.h>

AVIAdapter::AVIAdapter(void)
{
}

AVIAdapter::AVIAdapter( CWnd* w, unsigned int fr, CString &filename )
:  wnd(w),
   frameRate(fr),
   fileName(filename)
{
   Init();
}


AVIAdapter::~AVIAdapter(void)
{
   delete aviGen;
   delete [] bitAddress;
}

void AVIAdapter::Init()
{
   CRect rect;
   wnd->GetWindowRect(rect);

   // need to crop window dimensions so they're compatable (multiples of 4)

   BITMAPINFOHEADER bih;
   bih.biSize = sizeof(BITMAPINFOHEADER);
   bih.biWidth = (rect.Width()/4)*4;
   bih.biHeight = (rect.Height()/4)*4;
   bih.biPlanes = 1;
   bih.biBitCount = 24;
   bih.biSizeImage = ((bih.biWidth*bih.biBitCount+31)/32 * 4)*bih.biHeight;
   bih.biCompression = BI_RGB;
   bih.biClrUsed = 0;

   aviGen = new CAVIGenerator( fileName, &bih, frameRate );

   if( aviGen->InitEngine() != 0 )
   {
      AfxMessageBox(_T("Couldn't initialize AVI generator."));
   }

   CDC* dc = wnd->GetWindowDC();

   bitmap.CreateCompatibleBitmap(dc, rect.Width(), rect.Height());
   bitmapDC.CreateCompatibleDC(dc);
   bitmapDC.SelectObject(&bitmap);
   bi.bmiHeader = *aviGen->GetBitmapHeader();

   wnd->ReleaseDC(dc);

   bitAddress = new BYTE[3*rect.Width()*rect.Height()];
}

void AVIAdapter::AddFrame()
{
   CDC* dc = wnd->GetWindowDC();

   CRect rect;
   wnd->GetWindowRect(rect);

   BOOL bltSuccess = bitmapDC.BitBlt( 0, 0, rect.Width(), rect.Height(), dc, 0, 0, SRCCOPY );

   GetDIBits( bitmapDC, HBITMAP(bitmap), 0, rect.Height(), bitAddress, &bi, DIB_RGB_COLORS );

   aviGen->AddFrame(bitAddress);

   wnd->ReleaseDC(dc);
}

void AVIAdapter::End()
{
   aviGen->ReleaseEngine();
}