//-------------------------------------------------------------------
//  StrArray.hpp
//  Purpose:    Provides general dynamic data storage classes
//-------------------------------------------------------------------

#if !defined _STRARRAY_HPP
#define      _STRARRAY_HPP 1

#include "libs.h"

#include "PtrArray.h"
#include "stdstring.h"


class  LIBSAPI StringArray
   {
   protected:
      CStringArray a;

   public:
      //-- constructors --//
      StringArray( void ){}// : length( 0 ), allocLength( 0 ), array( NULL ) { }
      StringArray( const StringArray& sa ) {a.RemoveAll(); a.Copy(sa.a);}
      StringArray & operator=(const StringArray& sa) {a.RemoveAll(); a.Copy(sa.a); return *this;}

      ~StringArray( void ) {;}

      INT_PTR Length( void ) { return a.GetSize(); }//length; }

      void AddString( const char *  string ) {a.Add((LPCTSTR)string);}
      BOOL SetString( UINT offset, const char *  string ) 
         {
         BOOL bRet = ( ((int) offset)  < a.GetSize());
         a.SetAtGrow(offset, (LPCTSTR)string);
         return bRet;
         }

      LPCTSTR GetString( UINT offset ) {return (LPCTSTR) a.GetAt(offset);}
      LPCTSTR operator [] ( UINT pos ) const { return (LPCTSTR) a[pos]; }  // jpb 1/18/07
      void Clear( void ) {a.RemoveAll();}

      void Delete( UINT pos) {a.RemoveAt(pos);}
   };




class  LIBSAPI StdStringArray
   {
   protected:
      PtrArray< CString > a;

   public:
      //-- constructors --//
      StdStringArray( void ){}// : length( 0 ), allocLength( 0 ), array( NULL ) { }
      StdStringArray( const StdStringArray& sa ) {a.RemoveAll(); a.Copy(sa.a);}
      StdStringArray & operator=(const StdStringArray& sa) {a.RemoveAll(); a.Copy(sa.a); return *this;}

      ~StdStringArray( void ) { }

      INT_PTR Length( void ) { return a.GetSize(); }//length; }

      void AddString( const char *  string ) {a.Add( new CString( string ) );}
      BOOL SetString( UINT offset, const char *  string ) 
         {
         BOOL bRet = ( ((int) offset)  < a.GetSize());
         a.SetAtGrow(offset, new CString( string ) );
         return bRet;
         }


      LPCSTR GetString( UINT offset ) {return (LPCSTR) a.GetAt(offset);}
      LPCSTR operator [] ( UINT pos ) const { return (LPCSTR) a[pos]; }  // jpb 1/18/07
      void Clear( void ) {a.RemoveAll();}

      void Delete( UINT pos) {a.RemoveAt(pos);}
   };



#endif
