//-------------------------------------------------------------------
// INIFILE.HPP
//
//-- provides a buffered ini file reader/writer
//-------------------------------------------------------------------

#if !defined _INIFILE_HPP
#define _INIFILE_HPP

#if !defined __STDIO_H
#include <stdio.h>
#endif

//------------------------ IniFile --------------------------------//

class IniFile 
   {
   protected:
      char *rbuffer;         // read buffer
      char *rpos;            // current position in the read buffer
      char  wbuffer[ 128 ];  // write buffer
      FILE *fp;              // output only (buffered on input)
      BOOL trapErrors;

      long GetSize       ( void );
      int  SetNextReadPos( void );
      int  TrapErrors    ( LPCSTR label );

   public:
      //-- constructor --//
      IniFile( void ) 
         : rbuffer( NULL ), rpos( NULL ), fp( NULL ), trapErrors( 1 ) { }

      //-- destructor --//
      ~IniFile( void ) { if ( rbuffer ) delete [] rbuffer; Close(); }

      //-- open/close file --//
      int  OpenRead( LPCSTR filename );    // no close necessary
      int  OpenWrite( LPCSTR filename );
      void Close( void );  // only needed for writing

      void SetErrorTrapping( int flag ) { trapErrors = flag; }

      int WriteHeader( LPCSTR );
      int WriteInt   ( LPCSTR, int );
      int WriteLong  ( LPCSTR, long );
      int WriteUINT  ( LPCSTR, UINT  );
      int WriteFloat ( LPCSTR, float );
      int WriteString( LPCSTR, LPCSTR );
      int WriteMatrix( float**, int rows, int cols, int width );
      int NL( void );

      int ReadFloat ( LPCSTR, float* );
      int ReadInt   ( LPCSTR, int*   );
      int ReadLong  ( LPCSTR, long*   );
      int ReadUINT  ( LPCSTR, UINT*   );
      int ReadString( LPCSTR, char*, int );
      int ReadMatrix( float**, int, int );
   };




#endif




