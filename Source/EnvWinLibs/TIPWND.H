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
#if !defined(AFX_TIPWND_H__D0683D79_5181_400B_AB40_EDB11AE20363__INCLUDED_)
#define AFX_TIPWND_H__D0683D79_5181_400B_AB40_EDB11AE20363__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TipWnd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// TipWnd window

//class TipWnd : public CWnd
//{
//// Construction
//public:
//   TipWnd();
//
//// Attributes
//public:
//   CString m_text;
//   int m_border;
//   int m_fontSize;
//
//   CWnd *m_pAttachedWnd;
//
//   static LPCTSTR m_wndClass;
//
//// Operations
//public:
//
//// Overrides
//   // ClassWizard generated virtual function overrides
//   //{{AFX_VIRTUAL(TipWnd)
//   public:
//   virtual BOOL Create(int x, int y, CWnd* pParentWnd, UINT nID );
//   //}}AFX_VIRTUAL
//
//// Implementation
//public:
//   void Move( int x, int y );
//   void UpdateText( LPCTSTR text );
//   virtual ~TipWnd();
//
//   // Generated message map functions
//protected:
//   //{{AFX_MSG(TipWnd)
//   afx_msg void OnPaint();
//   //}}AFX_MSG
//   DECLARE_MESSAGE_MAP()
//};


// Get NONCLIENTMETRICS info: ctor calls SystemParametersInfo.
//
class CNonClientMetrics : public NONCLIENTMETRICS {
public:
   CNonClientMetrics() {
      cbSize = sizeof(NONCLIENTMETRICS);
      SystemParametersInfo(SPI_GETNONCLIENTMETRICS,0,this,0);
   }
};

// Popup text window, like tooltip.
// Can be right or left justified relative to creation point.
//
class WINLIBSAPI TipWnd : public CWnd 
   {
   public:
      CSize m_szMargins;      // extra space around text: change if you like
      enum {JUSTIFYLEFT=0, JUSTIFYRIGHT};
      
      TipWnd();
      virtual ~TipWnd();

      BOOL Create(CPoint pt, CWnd* pParentWnd, UINT nStyle=0, UINT nID=0);
      void ShowDelayed(UINT msec);
      void Cancel();
   
   protected:
      CFont m_font;           // font to use (same as tooltips)
      UINT  m_nStyle;         // style (see enum below)
   
      virtual void PostNcDestroy();
      virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
   
      afx_msg void OnPaint();
      afx_msg void OnTimer(UINT_PTR nIDEvent);
      afx_msg LRESULT OnSetText(WPARAM wp, LPARAM lp);

      DECLARE_DYNAMIC(TipWnd);
      DECLARE_MESSAGE_MAP();
   };











/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TIPWND_H__D0683D79_5181_400B_AB40_EDB11AE20363__INCLUDED_)
