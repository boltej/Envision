//-------------------------------------------------------------------
// BFILE.HPP
//
//-- provides a binary file i/o class
//-------------------------------------------------------------------

#pragma once
#include "libs.h"

#if !defined _FILE_HPP
#include <file.hpp>
#endif

#include <fcntl.h>
#include <io.h>


//--------------------- BinaryFile --------------------------------//

class BinaryFile : public File
   {
   protected:

   public:
      BinaryFile( const char *name ) : File( name ) { }

      //--------------- BINARY HEADER ---------------------//
      // writes a NULL terminated message to the current pos
      UINT WriteBinaryHeader( const char *string );

      //-- skips past this null terminated message, the file position
      //-- is left right after the NULL terminator
      LONG  SkipBinaryHeader( void );
      int   GetBinaryHeader( char *buffer, int maxSize );

      //-- write string WITH null terminator --//
      virtual UINT WriteString( const char *data );
      virtual UINT ReadString ( char *data, UINT maxLength );

      BinaryFile &operator << (const char *data)          
                     { WriteString( data ); return *this; }
      BinaryFile &operator << ( const BYTE &data )          
                     { Write( &data,sizeof( data ) ); return *this; }
      BinaryFile &operator << ( const short &data )         
                     { Write( &data,sizeof( data ) ); return *this; }
      BinaryFile &operator << ( const unsigned short &data )
                     { Write( &data,sizeof( data ) ); return *this; }
      BinaryFile &operator << ( const int &data )           
                     { Write( &data,sizeof( data ) ); return *this; }
      BinaryFile &operator << ( const unsigned int &data )  
                     { Write( &data,sizeof( data ) ); return *this; }
      BinaryFile &operator << ( const long &data )          
                     { Write( &data,sizeof( data ) ); return *this; }
      BinaryFile &operator << ( const unsigned long &data ) 
                     { Write( &data,sizeof( data ) ); return *this; }
      BinaryFile &operator << ( const float &data )         
                     { Write( &data,sizeof( data ) ); return *this; }
      BinaryFile &operator << ( const double &data )        
                     { Write( &data,sizeof( data ) ); return *this; }
      BinaryFile &operator << ( const long double &data )   
                     { Write( &data,sizeof( data ) ); return *this; }
      BinaryFile &operator << ( const RECT &data )          
                     { Write( &data,sizeof( data ) ); return *this; }
      BinaryFile &operator << ( const POINT &data )         
                     { Write( &data,sizeof( data ) ); return *this; }

      void operator >> (BYTE &data)           { Read( &data,sizeof(data)); }
      void operator >> (short &data)          { Read( &data,sizeof(data)); }
      void operator >> (unsigned short &data) { Read( &data,sizeof(data)); }
      void operator >> (int &data)            { Read( &data,sizeof(data)); }
      void operator >> (unsigned int &data)   { Read( &data,sizeof(data)); }
      void operator >> (long &data)           { Read( &data,sizeof(data)); }
      void operator >> (unsigned long &data)  { Read( &data,sizeof(data)); }
      void operator >> (float &data)          { Read( &data,sizeof(data)); }
      void operator >> (double &data)         { Read( &data,sizeof(data)); }
      void operator >> (long double &data)    { Read( &data,sizeof(data)); }
      void operator >> (RECT &data)           { Read( &data,sizeof(data)); }
      void operator >> (POINT &data)          { Read( &data,sizeof(data)); }
   };

