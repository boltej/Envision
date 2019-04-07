#if !defined _SYSINIT_HPP
#define _SYSINIT_HPP  1

extern   WINLIBSAPI  COLORREF gColorRef[ ];

extern "C"
{
BOOL InitSysLibApp( void );
BOOL InitSysLibInst( HINSTANCE );
BOOL CloseSysLib( void );
}
#endif

