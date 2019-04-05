/*
Generate PolicyOutcomeParser.cpp and PolicyOutcomeParser.hpp from this file with this command

   >bison -d -p po -o PolicyOutcomeParser.cpp PolicyOutcomeGrammer.y

-d generate defines output
-p option changes the prefix from yy to whatever is specified, in this case "po"


*/

%{

#ifndef NO_MFC
#include "stdafx.h"

#include <afxcoll.h>
#include <afxdisp.h>
#endif
#include "Policy.h"

// Globals
extern CString pogBuffer;
extern int pogIndex;
extern Policy *poPolicy;
extern MultiOutcomeArray *pogpMultiOutcomeArray;
extern CStringArray pogStringArray;

int  powrap();
void poerror(const char *str);

extern int  polex();
extern void popush();
extern void popop();
extern void PolicyOutcomeCompilerError( const char *str );

%}

%token AND

%union 
   {
   const char*       str;
   double            dNum;
   float             fNum;
   int               iNum;
   CStringArray*     pArgArray;
   OutcomeInfo*      pOutcomeInfo;
   OutcomeDirective* pOutcomeDirective;
   MultiOutcomeInfo* pMultiOutcomeInfo;
   }

%token <str>  STRING
%token <dNum> DNUM
%token <fNum> FNUM
%token <iNum> INUM
%token <str>  EVAL
%token <str>  FUNCTION
%token <pArgArray> ARGARRAY

%left AND
%right ';'  /* this is needed in order to have a optional ; at the end */

%type <pOutcomeDirective> directive
%type <pOutcomeInfo> outcome
%type <pMultiOutcomeInfo> outcomeArray
%type <pMultiOutcomeInfo> multiOutcome

%start entry

%%

entry
   : /* empty */
   
   | entry multiOutcomeArray 
        { 
        //TRACE( "multiOutcomeArray\n" );  
        }
   
   | entry outcomeArray      
        { 
        //TRACE( "implied probablility\n" ); 
        MultiOutcomeInfo *pMOI = $2;
        pMOI->probability = 100;
        pogpMultiOutcomeArray->AddMultiOutcome( pMOI ); 
        }
   ;
   
multiOutcomeArray
   : multiOutcome							{ pogpMultiOutcomeArray->AddMultiOutcome( $1 ); }
   | multiOutcomeArray ';' multiOutcome		{ pogpMultiOutcomeArray->AddMultiOutcome( $3 ); }

multiOutcome
   : outcomeArray ':' INUM  { MultiOutcomeInfo *pMOI = $1; pMOI->probability = (float) $3; }
   | outcomeArray ':' FNUM  { MultiOutcomeInfo *pMOI = $1; pMOI->probability = (float) $3; }
   | outcomeArray ':' DNUM  { MultiOutcomeInfo *pMOI = $1; pMOI->probability = (float) $3; } /*{ PolicyOutcomeCompilerError( "Integer expected for the probability." ); YYABORT; } */
   | outcomeArray			{ MultiOutcomeInfo *pMOI = $1; pMOI->probability = 100; }
   ;

outcomeArray
   : outcome                  
        { 
        MultiOutcomeInfo *pMOI = new MultiOutcomeInfo;
        pMOI->AddOutcome( $1 );
        $$ = pMOI;
        }
      
   | outcomeArray AND outcome 
        { 
        MultiOutcomeInfo *pMOI = $1;
        pMOI->AddOutcome( $3 );
        $$ = pMOI;
        }
   ;

outcome
   : STRING '=' INUM   '[' directive ']'  { $$ = new OutcomeInfo( poPolicy, $1, $3, 0, $5 ); }
   | STRING '=' DNUM   '[' directive ']'  { $$ = new OutcomeInfo( poPolicy, $1, $3, 0, $5 ); }
   | STRING '=' FNUM   '[' directive ']'  { $$ = new OutcomeInfo( poPolicy, $1, $3, 0, $5 ); }
   | STRING '=' STRING '[' directive ']'  { $$ = new OutcomeInfo( poPolicy, $1, $3, 0, $5 ); }
   | STRING '=' EVAL   '[' directive ']'  { $$ = new OutcomeInfo( poPolicy, $1, $3, 0, $5 ); }
   | STRING '=' INUM   { $$ = new OutcomeInfo( poPolicy, $1, $3, 0 ); }   
   | STRING '=' DNUM   { $$ = new OutcomeInfo( poPolicy, $1, $3, 0 ); }
   | STRING '=' FNUM   { $$ = new OutcomeInfo( poPolicy, $1, $3, 0 ); }
   | STRING '=' STRING { $$ = new OutcomeInfo( poPolicy, $1, $3, 0 ); }
   | STRING '=' EVAL   { $$ = new OutcomeInfo( poPolicy, $1, $3, 0 ); }
   
   | FUNCTION ARGARRAY 
        { 
        //TRACE( "Function\n" ); 
                
        OutcomeFunction *pFunction = OutcomeFunction::MakeFunction( $1, poPolicy );
        
        if ( pFunction == NULL )
           {
           ASSERT(0);
           PolicyOutcomeCompilerError( "Unrecognized function." );
           YYABORT;
           }
           
        pFunction->SetArgs( $2 );
           
        $$ = new OutcomeInfo( poPolicy, pFunction );
        }
   ;
   
directive
	: STRING ':' INUM ',' INUM		{ $$ = new OutcomeDirective( $1, $3, $5 ); }
	| STRING ':' INUM ',' DNUM		{ $$ = new OutcomeDirective( $1, $3, $5 ); }	
	| STRING ':' INUM ',' FNUM 		{ $$ = new OutcomeDirective( $1, $3, $5 ); }
	| STRING ':' INUM ',' STRING 	{ $$ = new OutcomeDirective( $1, $3, $5 ); }		
	| STRING ':' INUM				{ $$ = new OutcomeDirective( $1, $3 ); }
	;   
   
%%

//============================================================================


void poerror(const char *str)
   {
   PolicyOutcomeCompilerError( str );
   }

int powrap()
   {
   return 1;
   }
   
