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
#include "stdafx.h"

#include "EnvPolicy.h"
#include "PolicyOutcomeParser.hpp"


// Globals
CString pogBuffer;
int pogIndex = -1;
EnvPolicy *poPolicy = NULL;
MultiOutcomeArray *pogpMultiOutcomeArray = NULL;
CStringArray pogStringArray;
CString poPolicyName;  // for reporting errors - should be set before calling poparse()

void PolicyOutcomeCompilerError( const char *str );
CStringArray* GetFunctionArgs( void );
LPCTSTR FindString( CString &str  );
int GetIdentifier( void );
int GetQuotation( void );
int GetNumber( void );
TCHAR SkipComment( void );
void PolicyOutcomeCompilerError( const char *str );


// Keywords (not case sensitive)

enum POGSTATE { POG_NORMAL, POG_FUNCTION };

POGSTATE postate = POG_NORMAL;

struct KEYWORD { const char *str; int ret; };
   
KEYWORD keywordArray[] = 
   {
   { "and"   , AND    },  
   { NULL,     -1     }
   };


struct poparms
   {
   CString pogBuffer;
   int pogIndex;
   MultiOutcomeArray *pogpMultiOutcomeArray;
   };

CList< poparms, poparms& > postack;


int polex ()
   {
   if ( postate == POG_FUNCTION )
      {
      polval.pArgArray = GetFunctionArgs();
      postate = POG_NORMAL;
      return ARGARRAY;
      }

   if ( pogIndex >= pogBuffer.GetLength() )
      return 0;

   char c = pogBuffer[ pogIndex++ ];
   
   // Skip white space and comments
   while ( c == ' ' || c == '\t' || c == '{' || c == '\n' || c == '\r' )
      {
      // Skip comments
      if ( c == '{' )
         SkipComment();   // positions pogIndex to char following closing brace
      
      if ( pogIndex < pogBuffer.GetLength() )
         c = pogBuffer[ pogIndex++ ]; 
      else
         return 0;
      }

   // Process numbers
   if ( isdigit(c) || c == '.' || c == '-' )
      {
      pogIndex--;
      return GetNumber();
      }

   // Process quotes
   if ( c == '\'' || c == '\"')
      {
      pogIndex--;
      return GetQuotation();
      }

   // Process identifiers keywords and functions
   if ( isalpha( c ) )
       {
       pogIndex--;
       return GetIdentifier();
       }

   //if ( c == ',' )  // comma's are alteratives to "and"
   //   return AND;
   
   // Return end-of-input
   if ( c == '\0' || c == EOF )
     return 0;
   
   // Return a single char
   return c;
   }



TCHAR SkipComment( void )
   {
   int openCount = 1;
   TCHAR c = pogBuffer[ pogIndex-1 ];

   if ( c != '{' )
      return 0;

   for (;;)
      {
      if ( pogIndex < pogBuffer.GetLength() )
         c = pogBuffer[ pogIndex++ ]; 
      else
         {
         PolicyOutcomeCompilerError( "Unterminated comment found, '}' Expected" );
         return 0;
         }

      if ( c == '{' )
          openCount++;
      else if ( c == '}' )
          openCount--;

      if ( openCount == 0 )
         return c;
      }

   return 0;
   }
   
void popush()
   {
   poparms p;
   p.pogBuffer = pogBuffer;
   p.pogIndex  = pogIndex;
   p.pogpMultiOutcomeArray = pogpMultiOutcomeArray;

   postack.AddHead( p );
   }

void popop()
   {
   poparms &p = postack.GetHead();

   pogBuffer = p.pogBuffer;
   pogIndex  = p.pogIndex;
   pogpMultiOutcomeArray = p.pogpMultiOutcomeArray;

   postack.RemoveHead();
   }


CStringArray* GetFunctionArgs()
   {   
   char c = pogBuffer[ pogIndex++ ];

   // Skip white space 
   while ( c == ' ' || c == '\t' || c == '\n' || c == '\r' ) //|| c == '{' )
      {
      //if ( c == '{' )
      //   SkipComment();

      if ( pogIndex < pogBuffer.GetLength() )
         c = pogBuffer[ pogIndex++ ]; 
      else
         return 0;
      }

   if ( c != '(' )
      {
      PolicyOutcomeCompilerError( "'(' Expected" );
      return 0;
      }

   CStringArray *pStringArray = new CStringArray();
   CString arg;
   int openCount = 1;

   bool inDirective = false;
   bool inComment = false;   // zero means not in comment, positive values indicate level of braces
   bool inFunction = false; 

   // start main parser loop for parsing outcome function arguments
   for (;;)
      {
      if ( pogIndex < pogBuffer.GetLength() )
         c = pogBuffer[ pogIndex++ ];           // get next char in buffer
      else     // end of buffer reached
         {
         PolicyOutcomeCompilerError( "Unterminated argument list, ')' Expected" );
         delete pStringArray;
         return 0;
         }

      switch( c )
         {
         case '[':   inDirective = true;     break;
         case ']':   inDirective = false;    break;
         case '(':   openCount++;            break;
         case ')':   openCount--;            break;

         case ',':   
            if ( ! inDirective && inComment == false && openCount == 1 )
               {
               pStringArray->Add( arg );
               arg = "";
               continue;
               }
            break;

         case '{':   // start  of a comment?
            inComment = true;
            break;

         case '}':
            inComment = false;
            break;           

         //   c = SkipComment();
         //   break;
         }

      if ( openCount == 0 )  // hit the end of the argument list?
         {
         pStringArray->Add( arg );     // then add the last argument
         break;                        // and exit
         }

      arg += c;
      }

   return pStringArray;
   }

LPCTSTR FindString( CString &str  )
   {
   int count = (int) pogStringArray.GetSize();

   for ( int i=0; i < count; i++ )
      {
      if ( pogStringArray[ i ].CompareNoCase( str ) == 0 )  // found?
         return pogStringArray[ i ];
      }

   int offset = (int) pogStringArray.Add( str );
   return pogStringArray[ offset ];
   }


int GetIdentifier()
   {
   while ( pogBuffer[ pogIndex ] == ' ' ) pogIndex++;  // skip whitespace

   char c = pogBuffer[ pogIndex++ ];
   CString string;
   ASSERT( isalpha( c ) );

   for (;;)
      {
      string += c;

      if ( pogIndex < pogBuffer.GetLength() )
         c = pogBuffer[ pogIndex++ ]; 
      else
         break;
          
      if ( ! isalnum( c ) && c != '_' )   // not a legal character?
         {
         pogIndex--;   // move back so pogIndex points to first char after identifier
         break;
         }
      }

   ///////////////////////////////////////////////////////////////////////
   // trim leading whitespace
   //while ( pogBuffer[ pogIndex ] == ' ' ) pogIndex++;
   
   // check if it an eval?
   if ( string.CompareNoCase( "eval" ) == 0 )
      {
      // move pogIndex ahead until '(' found
      while( pogBuffer[ pogIndex ] == ' ' ) pogIndex++;  // skip whitespace

      if ( pogBuffer[ pogIndex ] == '(' )
         {
         int start = ++pogIndex;   // move past '('
         while( pogBuffer[ pogIndex ] == ' ' ) { pogIndex++; start++; }  // skip whitespace

         CString expr;
         int paren = 1;
         while( paren > 0 && pogIndex < pogBuffer.GetLength() )
            {
            if ( pogBuffer[ pogIndex ] == '(')
               paren++;
            else if ( pogBuffer[ pogIndex ] == ')' )
               paren--;

            if ( paren > 0 )
               expr += pogBuffer[ pogIndex ];

            pogIndex++;
            }

         // pogIndex should now point to the first char past the closing ')'
         expr.Trim();

         polval.str = FindString( expr );
         return EVAL;
         }
      }  // end of: if ( string == "eval" )

   // check if its a function
   if ( c == '(' )
      {
      postate = POG_FUNCTION;
      polval.str = FindString( string );
      return FUNCTION;
      }

   // check if its a keyword
   for ( int i=0; keywordArray[ i ].str != NULL; i++ )
      {
      const char *keyword = keywordArray[ i ].str;
      int count = lstrlen( keyword ) > string.GetLength() ? lstrlen( keyword ) : string.GetLength();
      if ( _tcsnicmp( string, keyword, count ) == 0 )
         {
         return keywordArray[ i ].ret;
         }
      }

   polval.str = FindString( string );
   return STRING;
   }

int GetQuotation()
   {
   char c = pogBuffer[ pogIndex++ ];
   ASSERT( c == '\'' || c == '\"' );

   int index;
   if ( c == '\'' )
      index = pogBuffer.Find( '\'', pogIndex );
   else
      index = pogBuffer.Find( '\"', pogIndex );
   
   if ( index < 0 )
      {
      PolicyOutcomeCompilerError( "Unterminated quotation found" );
      return 0;
      }

   CString string = pogBuffer.Mid( pogIndex, index - pogIndex );
   polval.str = FindString( string );
   pogIndex = index + 1;
   
   return STRING;
   }

int GetNumber()
   {
   char c = pogBuffer[ pogIndex++ ];
   CString str;

   bool dot      = false;
   bool exponent = false;
   bool f        = false;

   for (;;)
      {
      if ( c == '.' )
         {
         if ( dot )
            {
            pogIndex--;
            break;
            }
         dot = true;
         }

      if ( c == 'e' || c == 'E' )
         {
         if ( exponent )
            {
            pogIndex--;
            break;
            }
         exponent = true;
         }
         
      if ( c == 'f' )
         {
         f = true;
         break;
         }

      str += c;

      if ( pogIndex < pogBuffer.GetLength() )
         c = pogBuffer[ pogIndex++ ]; 
      else
         break;
         
      if ( ! isdigit(c) && c != '.' && c != 'e' && c != 'E' && c != 'f' )
         {
         pogIndex--;
         break;
         }
      }
   
   if ( dot )
      {
      if ( f )
         {
         polval.fNum = (float)atof( str );
         return FNUM;
         }
      else
         {
         polval.dNum = atof( str );
         return DNUM;
         }
      }

   polval.iNum = atoi( str );
   return INUM;
   }
     

void PolicyOutcomeCompilerError( const char *str )
   {
#ifndef NO_MFC
   CString msg;
   CString ptrString; // TODO: make a MessageBox with fixed width font.
   for ( int i=0; i<pogIndex; i++ )
      {
      if ( i == pogIndex )
         ptrString += '^';
      else
         ptrString += ' ';
      }
   
   msg.Format( "%s\n\n%s", str, (PCTSTR) pogBuffer );

   CString hdr = "Policy Outcome Compiler Error: ";
   hdr += poPolicyName;
  
   MessageBox( GetActiveWindow(), msg, hdr, MB_OK ); 
#endif
   pogpMultiOutcomeArray->RemoveAll();
   }
