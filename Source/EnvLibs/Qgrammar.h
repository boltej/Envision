
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
extern YYSTYPE yylval;
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
#define USERFN2 273
#define EQ 274
#define LT 275
#define LE 276
#define GT 277
#define GE 278
#define NE 279
#define CAND 280
#define COR 281
#define CONTAINS 282
#define AND 283
#define OR 284
#define NOT 285
#define MAPLAYER 286
#define CAN 287
