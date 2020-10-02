//%prefix qq

%{
#include "stdafx.h"
#include "EnvLibs.h"
#pragma hdrstop

#include <QueryEngine.h>
#include <map.h>
#include <maplayer.h>
#include <Report.h>

extern QueryEngine *_gpQueryEngine;
extern char *gQueryBuffer;
extern MapLayer *gFieldLayer;
char functionBuffer[ 128 ];
bool QShowCompilerErrors = true;
TCHAR QSource[ 256 ];
TCHAR QQueryStr[ 512 ];
int   QCurrentLineNo = 0;


//#define YYDEBUG


//---- prototypes -----

char *QStripWhitespace( char *p );
int   QIsReserved ( char **p );
int   QIsFunction( char **p );
int   QIsFieldCol( char **p );
MapLayer *QIsMapLayer( char **p );
QExternal *QIsExternal( char **p );
char *QParseString( char **p );
void  QCompilerError( LPCSTR errMsg, LPSTR buffer );
void  yyerror( const char* );
int   QCountNewlines( char *start, char *end );
void  QInitGlobals( int lineNo, LPCTSTR queryStr, LPCTSTR source );
int   yylex( void );


//------ constants ---------


//----- keywords ------
%}

%union {
   int           ivalue;
   double        dvalue;
   char         *pStr;
   QExternal    *pExternal;
   QNode        *pNode;
   QFNARG       qFnArg;    // a structure with members: int functionID; QArgList *pArgList;
   //QValueSet    *pValueSet;
   MapLayer     *pMapLayer;	// only used for field references
};

%token <ivalue> INTEGER 
%token <ivalue> BOOLEAN
%token <dvalue> DOUBLE
%token <pStr>   STRING
%token <pExternal> EXTERNAL 
%token <ivalue> FIELD
%token <ivalue> INDEX NEXTTO NEXTTOAREA WITHIN WITHINAREA EXPAND MOVAVG CHANGED TIME DELTA
%token <ivalue> EQ LT LE GT GE NE CAND COR CONTAINS 
%token <ivalue> AND OR NOT
%token <pMapLayer> MAPLAYER

%right AND
%right OR
%right NOT
%left  GE LE EQ NE LT GT CAN COR CONTAINS
%left FIELD 
%left  '+' '-'
%left  '*' '/' '%'

%type <pNode>  queryExpr
%type <ivalue> logicalOp
%type <pNode>  conditional
%type <ivalue> conditionalOp
%type <pNode>  fieldExpr
%type <qFnArg> function
%type <pNode>  fieldRef
%type <ivalue> valueSet
%type <ivalue> value
 
%%
querySet:
    querySet  queryExpr       { _gpQueryEngine->AddQuery( $2 ); } // pulls args off temp 
|   /* nothing */
;

/*-------------------------------------------------------
* Note:  query expression are of the form: 
* 
* [(] Condition1 [)] [[AND|OR] Condition2..  ]
* 
* where each condition is a conditional statement of the form:
* Field [<|>|=|!| ] fieldExpression OR alternatively is a builtin function
* 
* FieldExpresions are either constant values or builtin functions that return values that can be compared
* to the field value 
* 
* e.g. ( LULC_A = 16 OR ( LULC_B = 22 AND INFLOOD = 1 ) OR DIST_STREAM < 100 )
* 
* for builtin functions...
* e.g. 
* Within( LULC=16, 10 ) OR NextTo( LULC=21 ) OR INFLOOD OR NextToArea( LULC=16 )
* 
* fields can be referenced explicitly as follows:
* INFLOOD[ 22 ] or SzuLine.FieldXXX[ SZUINDEX ]
---------------------------------------------------------*/ 

queryExpr:
   queryExpr logicalOp queryExpr   { $$ = new QNode( $1, $2, $3 ); }
|  NOT queryExpr                 { $$ = new QNode( (QNode*)NULL, NOT, $2 ); }
|  conditional                     { $$ = $1; }
|  '(' queryExpr ')'               { $$ = $2; }
;

logicalOp:
   AND
|  OR
;

conditional:
   fieldExpr conditionalOp fieldExpr     { $$ = new QNode( $1, $2, $3 ); }
|  fieldExpr							 { $$ = $1; }
/*
|  function conditionalOp fieldExpr      { $$ = new QNode( new QNode( $1 ), $2, $3 ); }
|  function								 { $$ = new QNode( $1 ); }
*/
;
/*
|  fieldExpr '=' '[' valueSet ']'        { $$ = new QNode( $1, new QNode( _gpQueryEngine->CreateValueSet() ); } 
;
*/

conditionalOp:
   EQ
|  LT
|  LE
|  GT
|  GE
|  NE
|  CAND
|  COR
|  CONTAINS
;

/* fieldExpr are statements that evaluate to a value that can be compared using conditionals.
*  They can be constant values, numeric expressions, or builtin functions that return values 
*  that can be compared to other field expression values using conditional operators (e.g. <, >)
*  Examples:
*     AREA          <-- simple fieldRef expression
*     AREA * 10     <-- numeric expression
*     13            <-- numeric value
*     $TotalValue   <-- external value set at runtime by some other process
*     [1,2,5,6]     <-- a set of values
*/

fieldExpr:
   fieldExpr '+' fieldExpr          { $$ = new QNode( $1, '+', $3 ); }
|  fieldExpr '-' fieldExpr          { $$ = new QNode( $1, '-', $3 ); }
|  fieldExpr '*' fieldExpr          { $$ = new QNode( $1, '*', $3 ); }
|  fieldExpr '/' fieldExpr          { $$ = new QNode( $1, '/', $3 ); }
|  fieldExpr '%' fieldExpr          { $$ = new QNode( $1, '%', $3 ); }
|  fieldRef							{ $$ = $1; }
|  EXTERNAL                         { $$ = new QNode( $1 ); }
|  INTEGER                          { $$ = new QNode( $1 ); }
|  DOUBLE                           { $$ = new QNode( $1 ); }
|  STRING                           { $$ = new QNode( $1 ); }
|  BOOLEAN                          { $$ = new QNode( $1 ); }
|  function							{ $$ = new QNode( $1 ); }
|  '(' fieldExpr ')'                { $$ = $2; }
;


fieldRef:
	FIELD								{ $$ = new QNode( _gpQueryEngine->GetMapLayer(), $1 ); }
|   FIELD '[' fieldRef ']'              { $$ = new QNode( _gpQueryEngine->GetMapLayer(), $1, $3 ); } 
|   FIELD '[' INTEGER  ']'              { $$ = new QNode( _gpQueryEngine->GetMapLayer(), $1, $3 ); } 
|	MAPLAYER '.' FIELD '[' fieldRef ']'	{ $$ = new QNode( $1, $3, $5 ); }
|	MAPLAYER '.' FIELD '[' INTEGER  ']'	{ $$ = new QNode( $1, $3, $5 ); }
;

	
valueSet:
    valueSet '|' value             { $$ = 1; }
|   value					       { $$ = 1; }
;

value:
	INTEGER							{ $$ = (int) _gpQueryEngine->AddValue( $1 ); } 
|	DOUBLE							{ $$ = (int) _gpQueryEngine->AddValue( $1 ); }
;



function:
   INDEX      '(' ')'                                  { $$ = _gpQueryEngine->AddFunctionArgs( $1 ); }
|  NEXTTO     '(' queryExpr ')'                        { $$ = _gpQueryEngine->AddFunctionArgs( $1, $3 ); }
|  NEXTTOAREA '(' queryExpr ')'                        { $$ = _gpQueryEngine->AddFunctionArgs( $1, $3 ); }
|  WITHIN     '(' queryExpr ',' DOUBLE ')'             { $$ = _gpQueryEngine->AddFunctionArgs( $1, $3, $5 ); }
|  WITHIN     '(' queryExpr ',' INTEGER ')'            { $$ = _gpQueryEngine->AddFunctionArgs( $1, $3, (double) $5 ); }
|  WITHINAREA '(' queryExpr ',' INTEGER ',' DOUBLE ')' { $$ = _gpQueryEngine->AddFunctionArgs( $1, $3, (double) $5, $7 ); }
|  WITHINAREA '(' queryExpr ',' DOUBLE  ',' DOUBLE ')' { $$ = _gpQueryEngine->AddFunctionArgs( $1, $3, $5, $7 ); }
|  EXPAND     '(' queryExpr ',' DOUBLE  ',' DOUBLE ')' { $$ = _gpQueryEngine->AddFunctionArgs( $1, $3, $5, $7 ); }
|  MOVAVG     '(' queryExpr ',' INTEGER ')'            { $$ = _gpQueryEngine->AddFunctionArgs( $1, $3, (double) $5 ); }
|  CHANGED    '(' FIELD ')'                            { $$ = _gpQueryEngine->AddFunctionArgs( $1, $3 ); }
|  TIME       '(' ')'                                  { $$ = _gpQueryEngine->AddFunctionArgs( $1 ); }
|  DELTA      '(' fieldExpr ',' fieldExpr ')'          { $$ = _gpQueryEngine->AddFunctionArgs( $1, $3, $5 ); }
;


%%

struct KEYWORD { int type; const char *keyword; int ivalue; };

const int OPERATOR     = 0;
const int INT_CONSTANT = 1;
const int BUILTIN_FN   = 2;


KEYWORD fKeywordArray[] = 
{ { OPERATOR,      "and ",       AND  },
  { OPERATOR,      "or ",        OR   },
  { OPERATOR,      "not ",       NOT   },
  { OPERATOR,      "contains ",  CONTAINS   },

  //-- integer constants --//
  { INT_CONSTANT,  "false",     0   },
  { INT_CONSTANT,  "true",      1   },

  { -1,  NULL,     0   } };


//extern int lineNo;

// yylex() should return the id of the token found.  he yylval structure should be set 
// to whatever information should be associated with the particular token found, if any.

int yylex()
   {
   char *p = gQueryBuffer;
top:
   p = QStripWhitespace( p );
   
   //end of line found?
   if (p == NULL || *p == NULL )
      return 0;

   // are we starting a comment?  (of the form /* ... */ )
   if ( *p == '/' && *(p+1) == '*' )
      {
      p +=2;
cycle:
      char *end = strchr( p, '*' );
      if ( end == '\0' )
        {
        QCompilerError( "Unterminated comment found", p );
        //QCurrentLineNo = 0;
        return 0;
        }

      if ( *(end+1) != '/' )   // did we find a '*/' (as opposed to just a '*')?
        {
        QCurrentLineNo += QCountNewlines( p, end );
        p = end+1;
        goto cycle;
        }
      else     // a valid comment delimiter was found, start again...
        {
        QCurrentLineNo += QCountNewlines( p, end );
        p = end+2;
        }

       goto top;
       }

   // are we starting a comment?  (of the form { })
   if ( *p == '{' )
      {
      p++;
      char *end = strchr( p, '}' );
      if ( end == NULL )
        {
        QCompilerError( "Unterminated comment found", p );
        return 0;
        }
       else
        {
        QCurrentLineNo += QCountNewlines( p, end );
        p = end+1;
        }
       goto top;
       }

   // are we starting a comment?  (of the form //  \n )
   if ( *p == '/' && *(p+1) == '/' )
      {
      char *end = strchr( p+2, '\n' );
      if ( end == NULL )
         {
         QCurrentLineNo = 0;
         return 0;
         }

      QCurrentLineNo++;
      p = end+1;  // move to start of next line
      goto top;
      }

   // end of input?
   if ( *p == '\0' )
      {
      QCurrentLineNo = 0;
      return 0;
      }

   // what's here?
   switch ( *p )
      {
      // operators
	  case '%':
         gQueryBuffer = ++p;
         return yylval.ivalue = '%';

      case '=':
         gQueryBuffer = ++p;
         return yylval.ivalue = EQ;

      case '>':
         p++;
         if ( *p == '=' )
            {
           gQueryBuffer = ++p;
              yylval.ivalue = GE;
           }
         else
           {
           gQueryBuffer = p;
           yylval.ivalue = GT;
           }
         return yylval.ivalue;

      case '<':
         p++;
         if ( *p == '=' )
            {
            gQueryBuffer = ++p;
            yylval.ivalue = LE;
            }
         else if ( *p == '>' )
           {
           gQueryBuffer = ++p;
           yylval.ivalue = NE;         
           }
         else
           {
           gQueryBuffer = p;
           yylval.ivalue = LT;
           }
         return yylval.ivalue;

      case '|':
           gQueryBuffer = ++p;
           return yylval.ivalue = COR;

      case '&':
           gQueryBuffer = ++p;
           return yylval.ivalue = CAND;
      
      case '!':
         if ( *(p+1) == '=' ) 
            {
           gQueryBuffer = p+2;
           return yylval.ivalue = NE;           
           }
         break;
         
      case '$':
         gQueryBuffer = ++p;
         return yylval.ivalue = CONTAINS;   

      case '@':		// external? (this should be removed!)
         {
         gQueryBuffer = ++p;

         MapLayer *pLayer = gFieldLayer;
         if ( pLayer == NULL )
	         pLayer =_gpQueryEngine->GetMapLayer();

		 char *end = p;
		 while( isalnum( *end ) || *end == '_' ) end++;
		 char _end = *end;
		 *end = NULL;
		 int col = pLayer->GetFieldCol( p );
         yylval.pExternal = _gpQueryEngine->AddExternal( p, col );   
		 *end = _end;
		 gQueryBuffer = end;
		 return EXTERNAL;  // yylval.pExternal;
		 }

      case '(':
      case ')':
      case ',':
      case '[':
      case ']':
      case '.':      
         gQueryBuffer = ++p;
         return yylval.ivalue = *(p-1);
         
      case '\'':	// single quote
		// see if we have a legal char
		if ( isalnum( *(p+1) ) && *(p+2) == '\'' )
		   {
		   gQueryBuffer += 3;
		   yylval.ivalue = (int) (*(p+1));
		   return INTEGER;
		   }
		break;

      }  // end of: switch( *p )

   // not an operator, is it a reserved word?
   int reservedIndex = QIsReserved( &p );
   if ( reservedIndex >= 0 )
       {
      if ( p == NULL )
         {
         QCurrentLineNo = 0;
         return 0;
         }

      gQueryBuffer = p;      // note:  IsReserved increments p if match found

      switch( fKeywordArray[ reservedIndex ].type  )
         {
         case OPERATOR:         // a reserved word - operator
            return yylval.ivalue = fKeywordArray[ reservedIndex ].ivalue;

         case INT_CONSTANT:     // an integer constant
            yylval.ivalue = fKeywordArray[ reservedIndex ].ivalue;
            return INTEGER;
            
         default:
            ASSERT( 0 );
         }
      }   // end of:  if ( IsReserved() )

   // Is it a function?
   int fnID = QIsFunction( &p );
   if ( fnID >= 0 )
	  {
      gQueryBuffer = p;
      return yylval.ivalue = fnID;   // this is the #defined ID of the function
      }
      
   // or a layer name?
   MapLayer *pLayer = QIsMapLayer( &p );
   if ( pLayer != NULL )
      {
      gQueryBuffer = p;
      gFieldLayer = pLayer;
      yylval.pMapLayer = pLayer;      
      return MAPLAYER;
      }
              
   // or a field column name?
   int col = QIsFieldCol( &p );
   if ( col >=0 )
      {
      gQueryBuffer = p;
      gFieldLayer = NULL;
      yylval.ivalue = col;
      return FIELD;
      }

	// or a appvar name?
	QExternal *pExt = QIsExternal( &p );
	if ( pExt != NULL )
	   {
       gQueryBuffer = p;
       yylval.ivalue = col;
       return EXTERNAL;
       }

   // is it a number?
   bool negative = false;
   if ( *p == '-' )
      {
      negative = true;
      p++;
      }

   if ( isdigit( *p ) )
      {
      // find terminating character
      char *end = p;
      int   type = INTEGER;
      while ( isdigit( *end ) || *end == '.' )
         {
         if ( *end == '.' )
            type = DOUBLE;
         end++;
         }

      if ( type == DOUBLE )
         {
         yylval.dvalue = atof( p );
         if ( negative )
            yylval.dvalue = -yylval.dvalue;
         }
      else   // INTEGER
         {
         yylval.ivalue = atoi( p );
         if ( negative )
            yylval.ivalue = -yylval.ivalue;
         }

      // is there a unit associated with this constant?
      while ( *end == ' ' )   // skip white space
         end++;
   
      gQueryBuffer = end;
      return type;
      }   // end of: isdigit( *p )

   if ( negative )    // back pointer up
      --p;

   // or a string?
   char *str;
   if ( *p == '"' )   // start of a string delimiter?
      {
      if ( ( str = QParseString( &p ) ) != NULL )
         {
         yylval.pStr = str;
         gQueryBuffer = p;
         return STRING;
         }
      }

   QCompilerError( "Unrecognized token found", p );
   return *p;
   //QCurrentLineNo = 0;
   //return 0;
   }


char *QStripWhitespace( char *p )// strip white space
   {
   while( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'  )
      {
      if ( *p == '\n' || *p == '\r' )
         QCurrentLineNo++;
      p++;
      }

   return p;
   }


// check to see if the string is a fieldname in the current map.  If so, increment cursor to
// following character and return keyword id.  Othewise, do nothing and return -1

int QIsFieldCol( char **p )
   {
   if ( ! isalpha( (int) (**p ) ) && **p != '_' )	// has to start with alpha or underscore
      return -1;
      
   MapLayer *pLayer = gFieldLayer;
   if ( pLayer == NULL )
	  pLayer =_gpQueryEngine->GetMapLayer();
	  
   // find termination of field name
   char *end = *p;
   while ( isalnum( (int) (*end ) ) || *end == '_' )
      end++;

   char endChar = *end;
   *end = '\0';

   int col = pLayer->GetFieldCol( *p );
   
   *end = endChar;
   
   if ( col >= 0 )   // col found?
      *p = end;
  
   return col;
   }

// check to see if the string is a External Variable (AppVar).  If so, increment cursor to
// following character and return variable name (id).  Othewise, do nothing and return -1

   QExternal *QIsExternal(char **p)
      {
      if (!isalpha((int)(**p)) && **p != '_')	// has to start with alpha or underscore
         return NULL;

      TCHAR name[64];
      memset(name, 0, 64);
      TCHAR *ptr = *p;
      int index = 0;
      while ((isalnum((int)(*ptr))) || *ptr == '_' || *ptr == '.')	// has to start with alpha or underscore
         {
         name[index] = *ptr;
         index++;
         ptr++;
         }

      // ptr pts to the first position after the name.  See if the name matches an extern id
      QExternal *pExt = _gpQueryEngine->FindExternal(name);
      if (pExt == NULL)
         return NULL;

      // found, so return found external
      *p = ptr;
      return pExt;
      }


MapLayer *QIsMapLayer( char **p )
   {
   if ( ! isalpha( (int) (**p ) ) && **p != '_' )	// has to start with alpha or underscore
      return NULL;

   MapLayer *pLayer = _gpQueryEngine->GetMapLayer();
   Map      *pMap   = pLayer->m_pMap;
	  
   // find termination of field name
   char *end = *p;
   while ( isalnum( (int) (*end ) ) || *end == '_' )
      end++;
      
   if ( *end != '.' )    // layer names must terminate with '.'
      return NULL;      

   char endChar = *end;
   *end = '\0';

   int col = -1;
   pLayer = pMap->GetLayer( *p );
   
   *end = endChar;
   
   if ( pLayer != NULL )   // layer found?
      *p = end;

   return pLayer;
   }


int QIsFunction( char **p )
   {
   if ( _strnicmp( "Index", *p, 5 ) == 0 )
      {
      *p += 5;
      return INDEX;
      } 
   if ( _strnicmp( "NextToArea", *p, 10 ) == 0 )
      {
      *p += 10;
      return NEXTTOAREA;
      } 
      
   if ( _strnicmp( "NextTo", *p, 6 ) == 0 )
      {
      *p += 6;
      return NEXTTO;
      }
      
   if ( _strnicmp( "WithinArea", *p, 10 ) == 0 )
      {
      *p += 10;
      return WITHINAREA;
      }
      
      
   if ( _strnicmp( "Within", *p, 6 ) == 0 )
      {
      *p += 6;
      return WITHIN;
      }
      
   if ( _strnicmp( "MOVAVG", *p, 6 ) == 0 )
      {
      *p += 6;
      return MOVAVG;
      }

      
   if ( _strnicmp( "CHANGED", *p, 7 ) == 0 )
      {
      *p += 7;
      return CHANGED;
      }

   if ( _strnicmp( "DELTA", *p, 5 ) == 0 )
      {
      *p += 5;
      return DELTA;
      }

   if ( _strnicmp( "TIME", *p, 4 ) == 0 )
      {
      *p += 4;
      return TIME;
      }


   return -1;
   }


// check to see if the string is a reserved word.  If so, increment cursor to
// following character and return keyword id.  Othewise, do nothing and return -1

int QIsReserved( char **p )
   {
   int i=0;
   while ( fKeywordArray[ i ].keyword != NULL )
      {
      const char *keyword = fKeywordArray[ i ].keyword;
      if ( _strnicmp( *p, keyword, lstrlen( keyword ) ) == 0 )
         {
         *p += lstrlen( keyword );
         return i;
          }
      ++i;
      }

   return -1;
   }



char *QParseString( char **p )
   {
   ASSERT( **p == '"' );
   (*p)++;

   char *end = strchr( *p, '"' );

   if ( end == NULL )
      {
      QCompilerError( "Unterminated string found", *p );
      QCurrentLineNo = 0;
      return 0;
      }

   // actual string found, so see if it is already in the string table
   *end = '\0';
   LPCSTR str = _gpQueryEngine->FindString( *p, true );
   *end = '"';
   *p = end+1;
   return (char*) str;
   }

void QCompilerError( LPCSTR errorStr, LPSTR buffer )
   {
   char _buffer[ 512 ];
   strncpy_s( _buffer, 512, buffer, 512);
   _buffer[511] = '\0';

   CString msg;
   msg.Format( "Query Compiler Error: %s while reading '%s' at line %i: %s.  The complete parse string is '%s'", errorStr, QSource, QCurrentLineNo+1, _buffer, QQueryStr );
   
   if ( QShowCompilerErrors )
	  Report::ErrorMsg( msg, "Compiler Error", MB_OK );

   _gpQueryEngine->AddError( QCurrentLineNo, errorStr );
   }



void yyerror( const char *msg )
   {
   QCompilerError( msg, gQueryBuffer );
   }


int QCountNewlines( char *start, char *end )
   {
   int count = 0;
   while ( start != end )
      {
      if ( *start == '\n' )
         count++;

      start++;
      }

   return count;
   }
   

void  QInitGlobals( int lineNo, LPCTSTR queryStr, LPCTSTR source )
	{
	if ( lineNo >= 0 )
		QCurrentLineNo = lineNo;

	if ( queryStr != NULL )
		lstrcpyn( QQueryStr, queryStr, 511 );

	if ( source != NULL )
		lstrcpyn( QSource, source, 255 );
	}




