#if !defined _STREAM_HPP
#define _STREAM_HPP

#include "libs.h"


#if !defined _TDLIST_HPP
#include "TDLIST.HPP"
#endif

#if !defined __STDIO_H
#include <stdio.h>
#endif


enum STREAM_TYPE { STREAM_READING=0, STREAM_WRITING=1 };


//---------------------------------------------------------------------------
//-- Serial I/O object, supporting binary reads/writes from disk.
//
// Basic idea - 
//  1) subclass from this object
//  2) implement virtual Serialize function
//  3) call the Serial::Serialize at the BEGINNING of the virtual function
//  4) do whatever else you need to to save the object to/from disk
//
// Notes:
//  1) the "id" is saved as the current address of the object.  When the
//     object is reloaded from disk, its "old" address is reloaded into 
//     the id method.  It stays there until the object is saved and reloaded.
//---------------------------------------------------------------------------

class Serial
   {
   protected:
      LONG_PTR    id;   // stores the objects address
		FILE       *fp;   // internal file handle
      STREAM_TYPE type;
      
      BOOL IsReading( void ) { return type & STREAM_READING; }
      BOOL IsWriting( void ) { return type & STREAM_WRITING; }

      void SetReading( void ) { type = STREAM_READING; }
      void SetWriting( void ) { type = STREAM_WRITING; }

		//-- constructor --//
		Serial( void ) : fp( NULL ), type( STREAM_READING ) { id = (LONG_PTR) this; }

	  ~Serial( void ) { Close(); }

	public:
		FILE *Open ( char *filename, STREAM_TYPE );
		void  Close( void ) { if ( fp ) fclose( fp ); fp = NULL; }

		virtual BOOL Serialize( void );

		LONG_PTR GetID( void ) { return id; }
	};


class SerialManager : public TDList< Serial* >
	{
	public:
		int Register( Serial *p ) { return Append( p ); }

		Serial *Find( long id );
	};


//------------------------ Inline methods ----------------------//


inline
FILE *Serial::Open ( char *filename, STREAM_TYPE _type )
	{
	type = _type;
	
	if ( type == STREAM_READING )
		fopen_s( &fp, filename, "r" );
	else
		fopen_s( &fp, filename, "w" );

	return fp;
	}



inline
BOOL Serial::Serialize( void )
	{
	if ( fp == NULL )
		return FALSE;

	if ( IsReading() )
		return fread( (void*) &id, sizeof( long ), 1, fp ) == 1;

	else  // writing
		return fwrite( (const void*) &id, sizeof( long ), 1, fp ) == 1;
	}


inline
Serial *SerialManager::Find( long id )
	{
	GoToHead();
   //
	for ( UINT i=0; i < Length(); i++ )
		{
		Serial *p = Get();

		if ( p->GetID() == id )
			return p;
		else
			GoToNext();
		}

	return NULL;
	}

#endif
