#if !defined _MARKER_HPP
#define _MARKER_HPP

//-- marker style flag --/
enum MSTYLE
   {
   MS_NONE,
   MS_POINT,
   MS_SOLIDSQUARE,
   MS_HOLLOWSQUARE,
   MS_SOLIDCIRCLE,
   MS_HOLLOWCIRCLE,
   MS_SOLIDTRIANGLE,
   MS_HOLLOWTRIANGLE,
   MS_CROSS
   };

#define MS_START  MS_NONE
#define MS_END    MS_CROSS

void DrawMarker( HDC, int x, int y, MSTYLE, COLORREF, int size );

void InitComboMarker( HWND, int ctrlID );
void DrawComboMarker( HWND, DRAWITEMSTRUCT FAR *pItem );
void SetComboMarker ( HWND, int ctrlID, MSTYLE );
inline MSTYLE GetComboMarker( int index ) { return (MSTYLE) index; }

#endif

