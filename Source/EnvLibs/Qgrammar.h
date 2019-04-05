
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
