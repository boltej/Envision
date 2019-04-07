//========================================================================
// BAR.HPP
//
//  -- Class declaration for the bar plot
//========================================================================

#if !defined  _BAR_HPP
#define       _BAR_HPP 1

#if !defined ( _SCATTER_HPP )
#include "scatter.hpp"
#endif

class Bar : public Scatter
   {
   protected:
      //--- protected functions ---//
      virtual LRESULT Dispatch( UINT wMsg, WPARAM wParam, LPARAM lParam );

   public:
      //--- constructors ---//
      Bar( HWND hParent, LPSTR wClass,
           LPSTR title  = "", \
           DWORD wStyle = WS_OVERLAPPEDWINDOW,
           int   x      = CW_USEDEFAULT,
           int   y      = CW_USEDEFAULT,
           int   cx     = CW_USEDEFAULT,
           int   cy     = CW_USEDEFAULT );

      Bar( void );   // for use by mdi 

      virtual BOOL DrawLine( HDC, LINE*, POINT&, POINT& );

      static void DrawBar( HDC hDC, int x, int y, MSTYLE style, \
                           COLORREF color, int size );

   };


class MDIBar : public Bar
   {
   public:
      //--- constructors ---//
      MDIBar( HWND hClient, LPSTR wClass,
              LPSTR title  = "",
              DWORD wStyle = WS_CHILD,
              int   x      = CW_USEDEFAULT,
              int   y      = CW_USEDEFAULT,
              int   cx     = CW_USEDEFAULT,
              int   cy     = CW_USEDEFAULT )
         : Bar()
         {
         CreateMDIChild( hClient, wClass, title, wStyle, x, y, cx, cy );
         }

   };

#endif
