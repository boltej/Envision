//-------------------------------------------------------------------
//   DATE.HPP
//
//   Include file for date conversion routines.
//-------------------------------------------------------------------

#pragma once
#include "libs.h"

//-- Constants. -----------------------------------------------------
const int JD_INVALID_MONTH = -1;
const int JD_INVALID_DAY   = -2;
const int JD_INVALID_YEAR  = -3;

//-- Structures -----------------------------------------------------


enum MON { JAN=1, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEP, OCT, NOV, DEC };

struct SYSDATE
	{
	MON month;
	int day;
	int year;   // eg. 1994
	};


//-- Prototypes. ----------------------------------------------------

//extern "C"
//   {
   //-- Time methods. -----------------------------------------------
   int  LIBSAPI GetDaysPerMonth( int month, int year );

   int  LIBSAPI GetDayOfYear ( int month, int calDay, int year ); 

   BOOL LIBSAPI GetCalDate( int doy, int *pYear, int *pMonth, int *pCalDay, BOOL updateYear );

   int  LIBSAPI GetYearLength( int year );

   BOOL LIBSAPI IsLeapYear( int year );

   void LIBSAPI GetCurrentDate( int startDay, int startYear, int offset, int *pCurrDay, int *pCurrYear );

   int  LIBSAPI GetPerLnDayFromDates( int initDay, int initYear, int targDay, int targYear );

   int  LIBSAPI GetPerLnDayFromDatesNINC( int initDay, int initYear, int targDay, int targYear );

   void LIBSAPI GetTargDateFromInitDate( int initDay, int initYear, int targPerLnStep, int stepSize, int *pTargDay, int *pTargYear );

   void LIBSAPI GetInitDateFromTargDate( int targDay, int targYear, int targPerLnStep, int stepSize, int *pInitDay, int *pInitYear );

   void LIBSAPI GetMeanProfileDate( int initDay, int initYear, int targDay, int targYear, int *pPopEventDay, int *pPopEventYear );

   int  LIBSAPI GetDateSpan( SYSDATE &startDate, SYSDATE &endDate );

   bool IsDateValid( int month, int day, int year );     // note: month is 1-based
   inline bool IsDateValid( SYSDATE &d ) { return IsDateValid( d.month, d.day, d.year ); }

#ifndef NO_MFC
   void LIBSAPI LoadMonthCombo( HWND hCombo );
#endif

   LPCTSTR LIBSAPI GetMonthStr( int month ); // months are one-based
//   }
