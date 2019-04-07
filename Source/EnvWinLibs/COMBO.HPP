#if !defined _COMBO_HPP
#define _COMBO_HPP  1

extern "C"
{
void InitComboHatch    ( HWND hDlg, int ctrlID  );
void InitComboColor    ( HWND hDlg, int ctrlID  );
void InitComboLineStyle( HWND hDlg, int ctrlID );
void InitComboLineWidth( HWND hDlg, int ctrlID );

void DrawComboHatch    ( HWND hDlg, DRAWITEMSTRUCT FAR *pItem );
void DrawComboColor    ( HWND hDlg, DRAWITEMSTRUCT FAR *pItem );
void DrawComboLineStyle( HWND hDlg, DRAWITEMSTRUCT FAR *pItem );
void DrawComboLineWidth( HWND hDlg, DRAWITEMSTRUCT FAR *pItem );

void SetComboHatch    ( HWND hDlg, int ctrlID, LOGBRUSH *pLogBrush );
void SetComboColor    ( HWND hDlg, int ctrlID, COLORREF color );
void SetComboLineStyle( HWND hDlg, int ctrlID, LOGPEN *pLogPen );
void SetComboLineWidth( HWND hDlg, int ctrlID, int penWidth );

BOOL GetComboHatchBrush( int index, LOGBRUSH *pLogBrush );
LONG GetComboColor     ( int index );
BOOL GetComboLineStyle ( int index, LOGPEN *pLogPen );
inline int  GetComboLineWidth ( int index )  { return index+1; }
}

#endif
