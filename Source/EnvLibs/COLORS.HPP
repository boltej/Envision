//===================================================================
//  COLOR.HPP
//
//-- Define the colors used in term of RGB( RED, GREEN, BLUE )
//-- The 16 pure colors available on the VGA under Windows 3.0
//===================================================================

#if !defined ( _COLORS_HPP )
#define _COLORS_HPP  1

//-- RGB triplets for "standard" colors --//

const COLORREF BLACK     = RGB(    0,    0,    0 );
const COLORREF BLUE      = RGB(    0,    0, 0x80 );
const COLORREF GREEN     = RGB(    0, 0x80,    0 );
const COLORREF CYAN      = RGB(    0, 0x80, 0x80 );
const COLORREF RED       = RGB( 0x80,    0,    0 );
const COLORREF MAGENTA   = RGB( 0x80,    0, 0x80 );
const COLORREF BROWN     = RGB( 0x80, 0x80,    0 );
const COLORREF LTGRAY    = RGB( 0xC0, 0xC0, 0xC0 );
const COLORREF GRAY      = RGB( 0x80, 0x80, 0x80 );
const COLORREF LTBLUE    = RGB(    0,    0, 0xFF );
const COLORREF LTGREEN   = RGB(    0, 0xFF,    0 );
const COLORREF LTCYAN    = RGB(    0, 0xFF, 0xFF );
const COLORREF LTRED     = RGB( 0xFF,    0,    0 );
const COLORREF LTMAGENTA = RGB( 0xFF,    0, 0xFF );
const COLORREF YELLOW    = RGB( 0xFF, 0xFF,    0 );
const COLORREF WHITE     = RGB( 0xFF, 0xFF, 0xFF );

#endif
