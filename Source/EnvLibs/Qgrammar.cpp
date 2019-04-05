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
QExternal *QIsExternal(char **p);
char *QParseString( char **p );
void  QCompilerError( LPCTSTR errMsg, LPSTR buffer );
void  yyerror( char* );
int   QCountNewlines( char *start, char *end );
void  QInitGlobals( int lineNo, LPCTSTR queryStr, LPCTSTR source );
int   yylex( void );


//------ constants ---------


//----- keywords ------
typedef union  {
   int           ivalue;
   double        dvalue;
   char         *pStr;
   QExternal    *pExternal;
   QNode        *pNode;
   QFNARG       qFnArg;    // a structure with members: int functionID; QArgList *pArgList;
   //QValueSet    *pValueSet;
   MapLayer     *pMapLayer;	// only used for field references
} YYSTYPE;
#define INTEGER 257
#define BOOLEAN 258
#define DOUBLE 259
#define STRING 260
#define EXTERNAL 261
#define FIELD 262
#define INDEX 263
#define NEXTTO 264
#define NEXTTOAREA 265
#define WITHIN 266
#define WITHINAREA 267
#define EXPAND 268
#define MOVAVG 269
#define CHANGED 270
#define TIME 271
#define DELTA 272
#define EQ 273
#define LT 274
#define LE 275
#define GT 276
#define GE 277
#define NE 278
#define CAND 279
#define COR 280
#define CONTAINS 281
#define AND 282
#define OR 283
#define MAPLAYER 284
#define CAN 285
YYSTYPE yylval, yyval;
#define YYERRCODE 256


struct KEYWORD { int type; const char *keyword; int ivalue; };

const int OPERATOR     = 0;
const int INT_CONSTANT = 1;
const int BUILTIN_FN   = 2;


KEYWORD fKeywordArray[] = 
{ { OPERATOR,      "and ",       AND  },
  { OPERATOR,      "or ",        OR   },
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
   p = QStripWhitespace(p);

   //end of line found?
   if (p == NULL || *p == NULL)
      return 0;

   // are we starting a comment?  (of the form /* ... */ )
   if (*p == '/' && *(p + 1) == '*')
      {
      p += 2;
   cycle:
      char *end = strchr(p, '*');
      if (end == '\0')
         {
         QCompilerError("Unterminated comment found", p);
         //QCurrentLineNo = 0;
         return 0;
         }

      if (*(end + 1) != '/')   // did we find a '*/' (as opposed to just a '*')?
         {
         QCurrentLineNo += QCountNewlines(p, end);
         p = end + 1;
         goto cycle;
         }
      else     // a valid comment delimiter was found, start again...
         {
         QCurrentLineNo += QCountNewlines(p, end);
         p = end + 2;
         }

      goto top;
      }

   // are we starting a comment?  (of the form { })
   if (*p == '{')
      {
      p++;
      char *end = strchr(p, '}');
      if (end == NULL)
         {
         QCompilerError("Unterminated comment found", p);
         return 0;
         }
      else
         {
         QCurrentLineNo += QCountNewlines(p, end);
         p = end + 1;
         }
      goto top;
      }

   // are we starting a comment?  (of the form //  \n )
   if (*p == '/' && *(p + 1) == '/')
      {
      char *end = strchr(p + 2, '\n');
      if (end == NULL)
         {
         QCurrentLineNo = 0;
         return 0;
         }

      QCurrentLineNo++;
      p = end + 1;  // move to start of next line
      goto top;
      }

   // end of input?
   if (*p == '\0')
      {
      QCurrentLineNo = 0;
      return 0;
      }

   // what's here?
   switch (*p)
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
         if (*p == '=')
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
         if (*p == '=')
            {
            gQueryBuffer = ++p;
            yylval.ivalue = LE;
            }
         else if (*p == '>')
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
         if (*(p + 1) == '=')
            {
            gQueryBuffer = p + 2;
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
      if (pLayer == NULL)
         pLayer = _gpQueryEngine->GetMapLayer();

      char *end = p;
      while (isalnum(*end) || *end == '_') end++;
      char _end = *end;
      *end = NULL;
      int col = pLayer->GetFieldCol(p);
      yylval.pExternal = _gpQueryEngine->AddExternal(p, col);
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
         return yylval.ivalue = *(p - 1);

      case '\'':	// single quote
                  // see if we have a legal char
         if (isalnum(*(p + 1)) && *(p + 2) == '\'')
            {
            gQueryBuffer += 3;
            yylval.ivalue = (int)(*(p + 1));
            return INTEGER;
            }
         break;

      }  // end of: switch( *p )

         // not an operator, is it a reserved word?
   int reservedIndex = QIsReserved(&p);
   if (reservedIndex >= 0)
      {
      if (p == NULL)
         {
         QCurrentLineNo = 0;
         return 0;
         }

      gQueryBuffer = p;      // note:  IsReserved increments p if match found

      switch (fKeywordArray[reservedIndex].type)
         {
         case OPERATOR:         // a reserved word - operator
            return yylval.ivalue = fKeywordArray[reservedIndex].ivalue;

         case INT_CONSTANT:     // an integer constant
            yylval.ivalue = fKeywordArray[reservedIndex].ivalue;
            return INTEGER;

         default:
            ASSERT(0);
         }
      }   // end of:  if ( IsReserved() )

          // Is it a function?
   int fnID = QIsFunction(&p);
   if (fnID >= 0)
      {
      gQueryBuffer = p;
      return yylval.ivalue = fnID;   // this is the #defined ID of the function
      }

   // or a layer name?
   MapLayer *pLayer = QIsMapLayer(&p);
   if (pLayer != NULL)
      {
      gQueryBuffer = p;
      gFieldLayer = pLayer;
      yylval.pMapLayer = pLayer;
      return MAPLAYER;
      }

   // or a field column name?
   int col = QIsFieldCol(&p);
   if (col >= 0)
      {
      gQueryBuffer = p;
      gFieldLayer = NULL;
      yylval.ivalue = col;
      return FIELD;
      }

   // or a appvar name?
   QExternal *pExt = QIsExternal(&p);
   if (pExt != NULL)
      {
      gQueryBuffer = p;
      yylval.pExternal = pExt;
      return EXTERNAL;
      }

   // is it a number?
   bool negative = false;
   if (*p == '-')
      {
      negative = true;
      p++;
      }

   if (isdigit(*p))
      {
      // find terminating character
      char *end = p;
      int   type = INTEGER;
      while (isdigit(*end) || *end == '.')
         {
         if (*end == '.')
            type = DOUBLE;
         end++;
         }

      if (type == DOUBLE)
         {
         yylval.dvalue = atof(p);
         if (negative)
            yylval.dvalue = -yylval.dvalue;
         }
      else   // INTEGER
         {
         yylval.ivalue = atoi(p);
         if (negative)
            yylval.ivalue = -yylval.ivalue;
         }

      // is there a unit associated with this constant?
      while (*end == ' ')   // skip white space
         end++;

      gQueryBuffer = end;
      return type;
      }   // end of: isdigit( *p )

   if (negative)    // back pointer up
      --p;

   // or a string?
   char *str;
   if (*p == '"')   // start of a string delimiter?
      {
      if ((str = QParseString(&p)) != NULL)
         {
         yylval.pStr = str;
         gQueryBuffer = p;
         return STRING;
         }
      }

   QCompilerError("Unrecognized token found", p);
   return *p;
   //QCurrentLineNo = 0;
   //return 0;
   }


   char *QStripWhitespace(char *p)// strip white space
      {
      while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
         {
         if (*p == '\n' || *p == '\r')
            QCurrentLineNo++;
         p++;
         }

      return p;
      }


   // check to see if the string is a fieldname in the current map.  If so, increment cursor to
   // following character and return keyword id.  Othewise, do nothing and return -1

   int QIsFieldCol(char **p)
      {
      if (!isalpha((int)(**p)) && **p != '_')	// has to start with alpha or underscore
         return -1;

      MapLayer *pLayer = gFieldLayer;
      if (pLayer == NULL)
         pLayer = _gpQueryEngine->GetMapLayer();

      // find termination of field name
      char *end = *p;
      while (isalnum((int)(*end)) || *end == '_')
         end++;

      char endChar = *end;
      *end = '\0';

      int col = pLayer->GetFieldCol(*p);

      *end = endChar;

      if (col >= 0)   // col found?
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


   MapLayer *QIsMapLayer(char **p)
      {
      if (!isalpha((int)(**p)) && **p != '_')	// has to start with alpha or underscore
         return NULL;

      MapLayer *pLayer = _gpQueryEngine->GetMapLayer();
      Map      *pMap = pLayer->m_pMap;

      // find termination of field name
      char *end = *p;
      while (isalnum((int)(*end)) || *end == '_')
         end++;

      if (*end != '.')    // layer names must terminate with '.'
         return NULL;

      char endChar = *end;
      *end = '\0';

      int col = -1;
      pLayer = pMap->GetLayer(*p);

      *end = endChar;

      if (pLayer != NULL)   // layer found?
         *p = end;

      return pLayer;
      }


   int QIsFunction(char **p)
      {
      if (_strnicmp("Index", *p, 5) == 0)
         {
         *p += 5;
         return INDEX;
         }
      if (_strnicmp("NextToArea", *p, 10) == 0)
         {
         *p += 10;
         return NEXTTOAREA;
         }

      if (_strnicmp("NextTo", *p, 6) == 0)
         {
         *p += 6;
         return NEXTTO;
         }

      if (_strnicmp("WithinArea", *p, 10) == 0)
         {
         *p += 10;
         return WITHINAREA;
         }


      if (_strnicmp("Within", *p, 6) == 0)
         {
         *p += 6;
         return WITHIN;
         }

      if (_strnicmp("MOVAVG", *p, 6) == 0)
         {
         *p += 6;
         return MOVAVG;
         }


      if (_strnicmp("CHANGED", *p, 7) == 0)
         {
         *p += 7;
         return CHANGED;
         }

      if (_strnicmp("DELTA", *p, 5) == 0)
         {
         *p += 5;
         return DELTA;
         }

      if (_strnicmp("TIME", *p, 4) == 0)
         {
         *p += 4;
         return TIME;
         }


      return -1;
      }


   // check to see if the string is a reserved word.  If so, increment cursor to
   // following character and return keyword id.  Othewise, do nothing and return -1

   int QIsReserved(char **p)
      {
      int i = 0;
      while (fKeywordArray[i].keyword != NULL)
         {
         const char *keyword = fKeywordArray[i].keyword;
         if (_strnicmp(*p, keyword, lstrlen(keyword)) == 0)
            {
            *p += lstrlen(keyword);
            return i;
            }
         ++i;
         }

      return -1;
      }



   char *QParseString(char **p)
      {
      ASSERT(**p == '"');
      (*p)++;

      char *end = strchr(*p, '"');

      if (end == NULL)
         {
         QCompilerError("Unterminated string found", *p);
         QCurrentLineNo = 0;
         return 0;
         }

      // actual string found, so see if it is already in the string table
      *end = '\0';
      LPCTSTR str = _gpQueryEngine->FindString(*p, true);
      *end = '"';
      *p = end + 1;
      return (char*)str;
      }

   void QCompilerError(LPCTSTR errorStr, LPSTR buffer)
      {
      char _buffer[512];
      strncpy_s(_buffer, 512, buffer, 512);
      _buffer[511] = '\0';

      CString msg;
      msg.Format("Query Compiler Error: %s while reading '%s' at line %i: %s.  The complete parse string is '%s'", errorStr, QSource, QCurrentLineNo + 1, _buffer, QQueryStr);

      if (QShowCompilerErrors)
         Report::ErrorMsg(msg, "Compiler Error", MB_OK);

      _gpQueryEngine->AddError(QCurrentLineNo, errorStr);
      }



   void yyerror(const char *msg)
      {
      QCompilerError(msg, gQueryBuffer);
      }


   int QCountNewlines(char *start, char *end)
      {
      int count = 0;
      while (start != end)
         {
         if (*start == '\n')
            count++;

         start++;
         }

      return count;
      }


   void  QInitGlobals(int lineNo, LPCTSTR queryStr, LPCTSTR source)
      {
      if (lineNo >= 0)
         QCurrentLineNo = lineNo;

      if (queryStr != NULL)
         lstrcpyn(QQueryStr, queryStr, 511);

      if (source != NULL)
         lstrcpyn(QSource, source, 255);
      }







const int yyexca[] = {
  -1, 1,
  0, -1,
  -2, 0,
  0,
};

#define YYNPROD 53
#define YYLAST 296

const int yyact[] = {
      61,      89,      93,      88,      26,      27,      68,      13,
      77,      69,      96,      13,      97,      95,     112,      94,
     111,     110,      98,      99,     102,     101,      82,      81,
      83,      45,      35,       6,       4,      14,      59,      33,
      31,      14,      32,       5,      34,      35,      46,     107,
      29,     106,      33,      31,     115,      32,      87,      34,
      86,      85,       2,      84,      35,      58,      35,      28,
     109,      33,      31,      33,      32,     114,      34,     105,
      34,     113,      60,      62,      63,      64,      65,      66,
     108,      67,      35,     104,      57,     103,      59,      33,
      31,      35,      32,      90,      34,      56,      33,      31,
      91,      32,      78,      34,      79,      70,      55,      54,
      53,      80,      52,      71,      72,      73,      74,      75,
      76,      51,      50,      35,      49,      48,       1,      92,
      33,      31,      47,      32,      -1,      34,      -1,      12,
      30,       3,      25,       0,       0,       0,       0,     100,
       0,       0,       0,       0,       0,       0,       0,       0,
       0,       0,       0,       0,       0,       0,       0,       0,
       0,       0,       0,       0,       0,       0,       0,       0,
       0,       0,       0,       0,       0,       0,       0,       0,
       0,       0,       0,       0,       0,       0,       0,       0,
       0,       0,       0,       0,       0,       0,       0,       0,
       0,       0,       0,       0,       0,       0,       0,       0,
       0,       0,       0,       0,       0,       0,       0,       0,
       0,       0,       0,       0,       0,       0,       0,       0,
       0,       0,       0,       0,       0,       0,       0,       0,
       0,       0,       0,       0,       0,       0,       0,       0,
       0,       8,      11,       9,      10,       7,      13,      15,
      16,      17,      18,      19,      20,      21,      22,      23,
      24,       0,       0,       0,       0,       0,       0,      26,
      27,      26,      27,       0,      14,       8,      11,       9,
      10,       7,      13,      15,      16,      17,      18,      19,
      20,      21,      22,      23,      24,       0,      36,      37,
      38,      39,      40,      41,      42,      43,      44,       0,
      14,      36,      37,      38,      39,      40,      41,      42,
      43,      44,       0,       0,      26,      27,      26,      27,
       0,       0,      26,      27,      26,      27,      26,      27,
};

const int yypact[] = {
   -4096,     -12,    -278,   -4096,     -12,       0,   -4096,   -4096,
   -4096,   -4096,   -4096,   -4096,   -4096,     -66,      -8,      74,
      69,      68,      66,      65,      58,      56,      55,      54,
      45,     -12,   -4096,   -4096,      12,     -11,     -40,     -40,
     -40,     -40,     -40,     -40,   -4096,   -4096,   -4096,   -4096,
   -4096,   -4096,   -4096,   -4096,   -4096,    -251,    -253,      52,
     -12,     -12,     -12,     -12,     -12,     -12,    -254,      49,
     -40,    -278,   -4096,   -4096,      70,     -40,      17,      17,
   -4096,   -4096,   -4096,     -70,     -71,     -67,   -4096,      10,
       8,       4,       2,     -41,     -43,      42,   -4096,      44,
      37,   -4096,   -4096,    -255,   -4096,   -4096,    -244,    -247,
    -241,    -238,   -4096,     -40,     -72,     -73,      36,      34,
      19,      -3,      -5,      31,      15,   -4096,   -4096,   -4096,
   -4096,    -242,    -243,    -245,   -4096,   -4096,      24,      20,
       3,   -4096,   -4096,   -4096,
};

const int yypgo[] = {
       0,      50,     122,     121,     120,      35,     119,      27,
     118,     116,     110,
};

const int yyr1[] = {
       0,      10,      10,       1,       1,       1,       2,       2,
       3,       3,       4,       4,       4,       4,       4,       4,
       4,       4,       4,       5,       5,       5,       5,       5,
       5,       5,       5,       5,       5,       5,       5,       5,
       7,       7,       7,       7,       7,       8,       8,       9,
       9,       6,       6,       6,       6,       6,       6,       6,
       6,       6,       6,       6,       6,
};

const int yyr2[] = {
       0,       2,       0,       3,       1,       3,       1,       1,
       3,       1,       1,       1,       1,       1,       1,       1,
       1,       1,       1,       3,       3,       3,       3,       3,
       1,       1,       1,       1,       1,       1,       1,       3,
       1,       4,       4,       6,       6,       3,       1,       1,
       1,       3,       4,       4,       6,       6,       8,       8,
       8,       6,       4,       3,       6,
};

const int yychk[] = {
   -4096,     -10,      -1,      -3,      40,      -5,      -7,     261,
     257,     259,     260,     258,      -6,     262,     284,     263,
     264,     265,     266,     267,     268,     269,     270,     271,
     272,      -2,     282,     283,      -1,      -5,      -4,      43,
      45,      42,      47,      37,     273,     274,     275,     276,
     277,     278,     279,     280,     281,      91,      46,      40,
      40,      40,      40,      40,      40,      40,      40,      40,
      40,      -1,      41,      41,      -5,      40,      -5,      -5,
      -5,      -5,      -5,      -7,     257,     262,      41,      -1,
      -1,      -1,      -1,      -1,      -1,     262,      41,      -5,
      -5,      93,      93,      91,      41,      41,      44,      44,
      44,      44,      41,      44,      -7,     257,     259,     257,
     257,     259,     259,     257,      -5,      93,      93,      41,
      41,      44,      44,      44,      41,      41,     259,     259,
     259,      41,      41,      41,
};

const int yydef[] = {
       2,      -2,       1,       4,       0,       9,      24,      25,
      26,      27,      28,      29,      30,      32,       0,       0,
       0,       0,       0,       0,       0,       0,       0,       0,
       0,       0,       6,       7,       0,       9,       0,       0,
       0,       0,       0,       0,      10,      11,      12,      13,
      14,      15,      16,      17,      18,       0,       0,       0,
       0,       0,       0,       0,       0,       0,       0,       0,
       0,       3,       5,      31,       8,       0,      19,      20,
      21,      22,      23,       0,       0,       0,      41,       0,
       0,       0,       0,       0,       0,       0,      51,       0,
       0,      33,      34,       0,      42,      43,       0,       0,
       0,       0,      50,       0,       0,       0,       0,       0,
       0,       0,       0,       0,       0,      35,      36,      44,
      45,       0,       0,       0,      49,      52,       0,       0,
       0,      46,      47,      48,
};

/*****************************************************************/
/* PCYACC LALR parser driver routine -- a table driven procedure */
/* for recognizing sentences of a language defined by the        */
/* grammar that PCYACC analyzes. An LALR parsing table is then   */
/* constructed for the grammar and the skeletal parser uses the  */
/* table when performing syntactical analysis on input source    */
/* programs. The actions associated with grammar rules are       */
/* inserted into a switch statement for execution.               */
/*****************************************************************/


#ifndef YYMAXDEPTH
#define YYMAXDEPTH 200
#endif
#ifndef YYREDMAX
#define YYREDMAX 1000
#endif
#define PCYYFLAG -4096
#define WAS0ERR 0
#define WAS1ERR 1
#define WAS2ERR 2
#define WAS3ERR 3
#define yyclearin pcyytoken = -1
#define yyerrok   pcyyerrfl = 0
YYSTYPE yyv[YYMAXDEPTH];     /* value stack */
int pcyyerrct = 0;           /* error count */
int pcyyerrfl = 0;           /* error flag */
int redseq[YYREDMAX];
int redcnt = 0;
int pcyytoken = -1;          /* input token */


int yyparse()
{
#ifdef YYSHORT
  const short *yyxi;
#else
  const int *yyxi;
#endif
#ifdef YYASTFLAG
    int ti; int tj;
#endif

#ifdef YYDEBUG
int tmptoken;
#endif
  int statestack[YYMAXDEPTH]; /* state stack */
  int      j, m;              /* working index */
  YYSTYPE *yypvt;
  int      tmpstate, *yyps, n;
  YYSTYPE *yypv;


  tmpstate = 0;
  pcyytoken = -1;


#ifdef YYDEBUG
  tmptoken = -1;
#endif


  pcyyerrct = 0;
  pcyyerrfl = 0;
  yyps = &statestack[-1];
  yypv = &yyv[-1];


  enstack:    /* push stack */
#ifdef YYDEBUG
    printf("at state %d, next token %d\n", tmpstate, tmptoken);
#endif
    if (++yyps - &statestack[YYMAXDEPTH-1] > 0) {
      yyerror("pcyacc internal stack overflow");
      return(1);
    }
    *yyps = tmpstate;
    ++yypv;
    *yypv = yyval;


  newstate:
    n = yypact[tmpstate];
    if (n <= PCYYFLAG) goto defaultact; /*  a simple state */


    if (pcyytoken < 0) if ((pcyytoken=yylex()) < 0) pcyytoken = 0;
    if ((n += pcyytoken) < 0 || n >= YYLAST) goto defaultact;


    if (yychk[n=yyact[n]] == pcyytoken) { /* a shift */
#ifdef YYDEBUG
      tmptoken  = pcyytoken;
#endif
      pcyytoken = -1;
      yyval = yylval;
      tmpstate = n;
      if (pcyyerrfl > 0) --pcyyerrfl;
      goto enstack;
    }


  defaultact:


    if ((n=yydef[tmpstate]) == -2) {
      if (pcyytoken < 0) if ((pcyytoken=yylex())<0) pcyytoken = 0;
      for (yyxi=yyexca; (*yyxi!= (-1)) || (yyxi[1]!=tmpstate); yyxi += 2);
      while (*(yyxi+=2) >= 0) if (*yyxi == pcyytoken) break;
      if ((n=yyxi[1]) < 0) { /* an accept action */
#ifdef YYASTFLAG
          yytfilep = fopen(yytfilen, "w");
          if (yytfilep == NULL) {
            fprintf(stderr, "Can't open t file: %s\n", yytfilen);
            return(0);          }
          for (ti=redcnt-1; ti>=0; ti--) {
            tj = svdprd[redseq[ti]];
            while (strcmp(svdnams[tj], "$EOP"))
              fprintf(yytfilep, "%s ", svdnams[tj++]);
            fprintf(yytfilep, "\n");
          }
          fclose(yytfilep);
#endif

        return (0);
      }
    }


    if (n == 0) {        /* error situation */
      switch (pcyyerrfl) {
        case WAS0ERR:          /* an error just occurred */
          yyerror("syntax error");
            ++pcyyerrct;
        case WAS1ERR:
        case WAS2ERR:           /* try again */
          pcyyerrfl = 3;
	   /* find a state for a legal shift action */
          while (yyps >= statestack) {
	     n = yypact[*yyps] + YYERRCODE;
	     if (n >= 0 && n < YYLAST && yychk[yyact[n]] == YYERRCODE) {
	       tmpstate = yyact[n];  /* simulate a shift of "error" */
	       goto enstack;
            }
	     n = yypact[*yyps];


	     /* the current yyps has no shift on "error", pop stack */
#ifdef YYDEBUG
            printf("error: pop state %d, recover state %d\n", *yyps, yyps[-1]);
#endif
	     --yyps;
	     --yypv;
	   }


	   yyabort:

#ifdef YYASTFLAG
              yytfilep = fopen(yytfilen, "w");
              if (yytfilep == NULL) {
                fprintf(stderr, "Can't open t file: %s\n", yytfilen);
                return(1);              }
              for (ti=1; ti<redcnt; ti++) {
                tj = svdprd[redseq[ti]];
                while (strcmp(svdnams[tj], "$EOP"))
                  fprintf(yytfilep, "%s ", svdnams[tj++]);
                fprintf(yytfilep, "\n");
              }
              fclose(yytfilep);
#endif

	     return(1);


	 case WAS3ERR:  /* clobber input char */
#ifdef YYDEBUG
          printf("error: discard token %d\n", pcyytoken);
#endif
          if (pcyytoken == 0) goto yyabort; /* quit */
	   pcyytoken = -1;
	   goto newstate;      } /* switch */
    } /* if */


    /* reduction, given a production n */
#ifdef YYDEBUG
    printf("reduce with rule %d\n", n);
#endif
#ifdef YYASTFLAG
    if ( redcnt<YYREDMAX ) redseq[redcnt++] = n;
#endif
    yyps -= yyr2[n];
    yypvt = yypv;
    yypv -= yyr2[n];
    yyval = yypv[1];
    m = n;
    /* find next state from goto table */
    n = yyr1[n];
    j = yypgo[n] + *yyps + 1;
    if (j>=YYLAST || yychk[ tmpstate = yyact[j] ] != -n) tmpstate = yyact[yypgo[n]];
    switch (m) { /* actions associated with grammar rules */
      
      case 1:{ _gpQueryEngine->AddQuery( yypvt[-0].pNode ); } break;
      case 3:{ yyval.pNode = new QNode( yypvt[-2].pNode, yypvt[-1].ivalue, yypvt[-0].pNode ); } break;
      case 4:{ yyval.pNode = yypvt[-0].pNode; } break;
      case 5:{ yyval.pNode = yypvt[-1].pNode; } break;
      case 8:{ yyval.pNode = new QNode( yypvt[-2].pNode, yypvt[-1].ivalue, yypvt[-0].pNode ); } break;
      case 9:{ yyval.pNode = yypvt[-0].pNode; } break;
      case 19:{ yyval.pNode = new QNode( yypvt[-2].pNode, '+', yypvt[-0].pNode ); } break;
      case 20:{ yyval.pNode = new QNode( yypvt[-2].pNode, '-', yypvt[-0].pNode ); } break;
      case 21:{ yyval.pNode = new QNode( yypvt[-2].pNode, '*', yypvt[-0].pNode ); } break;
      case 22:{ yyval.pNode = new QNode( yypvt[-2].pNode, '/', yypvt[-0].pNode ); } break;
      case 23:{ yyval.pNode = new QNode( yypvt[-2].pNode, '%', yypvt[-0].pNode ); } break;
      case 24:{ yyval.pNode = yypvt[-0].pNode; } break;
      case 25:{ yyval.pNode = new QNode( yypvt[-0].pExternal ); } break;
      case 26:{ yyval.pNode = new QNode( yypvt[-0].ivalue ); } break;
      case 27:{ yyval.pNode = new QNode( yypvt[-0].dvalue ); } break;
      case 28:{ yyval.pNode = new QNode( yypvt[-0].pStr ); } break;
      case 29:{ yyval.pNode = new QNode( yypvt[-0].ivalue ); } break;
      case 30:{ yyval.pNode = new QNode( yypvt[-0].qFnArg ); } break;
      case 31:{ yyval.pNode = yypvt[-1].pNode; } break;
      case 32:{ yyval.pNode = new QNode( _gpQueryEngine->GetMapLayer(), yypvt[-0].ivalue ); } break;
      case 33:{ yyval.pNode = new QNode( _gpQueryEngine->GetMapLayer(), yypvt[-3].ivalue, yypvt[-1].pNode ); } break;
      case 34:{ yyval.pNode = new QNode( _gpQueryEngine->GetMapLayer(), yypvt[-3].ivalue, yypvt[-1].ivalue ); } break;
      case 35:{ yyval.pNode = new QNode( yypvt[-5].pMapLayer, yypvt[-3].ivalue, yypvt[-1].pNode ); } break;
      case 36:{ yyval.pNode = new QNode( yypvt[-5].pMapLayer, yypvt[-3].ivalue, yypvt[-1].ivalue ); } break;
      case 37:{ yyval.ivalue = 1; } break;
      case 38:{ yyval.ivalue = 1; } break;
      case 39:{ yyval.ivalue = (int) _gpQueryEngine->AddValue( yypvt[-0].ivalue ); } break;
      case 40:{ yyval.ivalue = (int) _gpQueryEngine->AddValue( yypvt[-0].dvalue ); } break;
      case 41:{ yyval.qFnArg = _gpQueryEngine->AddFunctionArgs( yypvt[-2].ivalue ); } break;
      case 42:{ yyval.qFnArg = _gpQueryEngine->AddFunctionArgs( yypvt[-3].ivalue, yypvt[-1].pNode ); } break;
      case 43:{ yyval.qFnArg = _gpQueryEngine->AddFunctionArgs( yypvt[-3].ivalue, yypvt[-1].pNode ); } break;
      case 44:{ yyval.qFnArg = _gpQueryEngine->AddFunctionArgs( yypvt[-5].ivalue, yypvt[-3].pNode, yypvt[-1].dvalue ); } break;
      case 45:{ yyval.qFnArg = _gpQueryEngine->AddFunctionArgs( yypvt[-5].ivalue, yypvt[-3].pNode, (double) yypvt[-1].ivalue ); } break;
      case 46:{ yyval.qFnArg = _gpQueryEngine->AddFunctionArgs( yypvt[-7].ivalue, yypvt[-5].pNode, (double) yypvt[-3].ivalue, yypvt[-1].dvalue ); } break;
      case 47:{ yyval.qFnArg = _gpQueryEngine->AddFunctionArgs( yypvt[-7].ivalue, yypvt[-5].pNode, yypvt[-3].dvalue, yypvt[-1].dvalue ); } break;
      case 48:{ yyval.qFnArg = _gpQueryEngine->AddFunctionArgs( yypvt[-7].ivalue, yypvt[-5].pNode, yypvt[-3].dvalue, yypvt[-1].dvalue ); } break;
      case 49:{ yyval.qFnArg = _gpQueryEngine->AddFunctionArgs( yypvt[-5].ivalue, yypvt[-3].pNode, (double) yypvt[-1].ivalue ); } break;
      case 50:{ yyval.qFnArg = _gpQueryEngine->AddFunctionArgs( yypvt[-3].ivalue, yypvt[-1].ivalue ); } break;
      case 51:{ yyval.qFnArg = _gpQueryEngine->AddFunctionArgs( yypvt[-2].ivalue ); } break;
      case 52:{ yyval.qFnArg = _gpQueryEngine->AddFunctionArgs( yypvt[-5].ivalue, yypvt[-3].pNode, yypvt[-1].pNode ); } break;    }
    goto enstack;
}
