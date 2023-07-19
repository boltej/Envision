
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
#define WITHINAREAFRAC 268
#define EXPAND 269
#define MOVAVG 270
#define CHANGED 271
#define TIME 272
#define DELTA 273
#define USERFN2 274
#define EQ 275
#define LT 276
#define LE 277
#define GT 278
#define GE 279
#define NE 280
#define CAND 281
#define COR 282
#define CONTAINS 283
#define AND 284
#define OR 285
#define NOT 286
#define MAPLAYER 287
#define CAN 288
