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
//-------------------------------------------------------------------
//   Program: DATE.CPP
//
//   Date conversion methods.
//-------------------------------------------------------------------


#include "EnvLibs.h"
#pragma hdrstop

#if !defined( _DATE_HPP )
#include "DATE.HPP"
#endif


//-------------------------------------------------------------------
//-- If calendar year divided by 4 gives no remainder, then year
//-- is a leap year.
//-- Short version: return ( year%4 == 0 ) ? 366 : 365;

int GetYearLength( int year )
   {
   int rem = year%4;

   int length = 365;

   if ( rem == 0 )
      length = 366;

   return length;
   }


//-------------------------------------------------------------------
//-- If calendar year divided by 4 gives no remainder, then year
//-- is a leap year.

BOOL IsLeapYear( int year )
   {
   int rem = year%4;

   if ( rem == 0 ) return TRUE;
   else return FALSE;
   }


//-------------------------------------------------------------------
int GetDaysPerMonth( int month, int year )
   {
   int dpm[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

   if ( GetYearLength( year ) == 366 )
      dpm[1] = 29;

   return dpm[ month - 1 ];
   }


//-- GetDayOfYear() -------------------------------------------------
//
//   Determines day of year as a function of calendar month, day and
//   year. (1-based)
//-------------------------------------------------------------------

int GetDayOfYear( int month, int calDay, int year )
   {
   //-- Error, return 0.
   if (  month  < 1 || month  > 12
      || calDay < 1 || calDay > GetDaysPerMonth( month, year )
      || year   < 0 || year   > 10000  )
      return 0;

   int doy = 0;
   if ( month == 1 )
      doy = calDay;

   else
      {
      for ( int i = 1; i < month; i++ )
		   doy += GetDaysPerMonth( i, year );

      doy += calDay;
      }

   return doy;
   }


//-----------------------------------------------------------------
//@ Method: GetCalDate()
//
//@ Description: Converts day of year to a calendar date.  Day of year is
//@   one-based, may be < 0 or > 365 and adjusts year if necessary.
//
//@ Arguments:
//@ - int doy - julian day of date to convert.  Can be positive or negative
//@ - int* pYear - reference year for calculations, contains year the
//@        julian date occurs in on return.
//@ - int* pMonth - returned calendar month (1-based)
//@ - int* pDay   - returned calendar day (1-based)
//@ - BOOL statAdjYear - flag - TRUE to calculate the new year, FALSE
//@        to assume the specified julian date is in the current year
//-------------------------------------------------------------------

BOOL GetCalDate( int doy, int *pYear, int *pMonth, int *pCalDay,
   BOOL statAdjYear )
   {
   //-- Error.
   if ( *pYear < 0 || *pYear > 10000 )
      {
      *pMonth  = 1;
      *pCalDay = 1;
      return FALSE;
      }

   if ( GetYearLength( *pYear ) == doy || statAdjYear == FALSE )
      ;

   else if ( doy <= 0 )
      {
      while ( doy <= 0 )
         {
         *pYear -= 1;
         doy += GetYearLength( *pYear );
         }
      }

   else if ( doy > GetYearLength( *pYear ) )
      {
      while ( doy > GetYearLength( *pYear ) )
         {
         doy -= GetYearLength( *pYear );
         *pYear += 1;
         }
      }

   //-- Initialize month and day, Jan. 31
   *pMonth = 1;
   int day = 31;

   while ( day < doy )
      {
      *pMonth = *pMonth + 1;
      day += GetDaysPerMonth( *pMonth, *pYear );
      }

   *pCalDay = doy - day + GetDaysPerMonth( *pMonth, *pYear );

   return TRUE;
   }

#ifndef NO_MFC
//-------------------------------------------------------------------
void LoadMonthCombo( HWND hCombo )
	{
   SendMessage( hCombo, CB_RESETCONTENT, 0, 0L );

   //-- load record combo box --//
   SendMessage( hCombo, CB_ADDSTRING, 0, (LPARAM)(LPSTR) "Jan" );
   SendMessage( hCombo, CB_ADDSTRING, 0, (LPARAM)(LPSTR) "Feb" );
   SendMessage( hCombo, CB_ADDSTRING, 0, (LPARAM)(LPSTR) "Mar" );
   SendMessage( hCombo, CB_ADDSTRING, 0, (LPARAM)(LPSTR) "Apr" );
   SendMessage( hCombo, CB_ADDSTRING, 0, (LPARAM)(LPSTR) "May" );
   SendMessage( hCombo, CB_ADDSTRING, 0, (LPARAM)(LPSTR) "Jun" );
   SendMessage( hCombo, CB_ADDSTRING, 0, (LPARAM)(LPSTR) "Jul" );
   SendMessage( hCombo, CB_ADDSTRING, 0, (LPARAM)(LPSTR) "Aug" );
   SendMessage( hCombo, CB_ADDSTRING, 0, (LPARAM)(LPSTR) "Sep" );
   SendMessage( hCombo, CB_ADDSTRING, 0, (LPARAM)(LPSTR) "Oct" );
   SendMessage( hCombo, CB_ADDSTRING, 0, (LPARAM)(LPSTR) "Nov" );
   SendMessage( hCombo, CB_ADDSTRING, 0, (LPARAM)(LPSTR) "Dec" );

   SendMessage( hCombo, CB_SETCURSEL, 0, 0L );
   }
#endif

//-------------------------------------------------------------------
//@ Method: GetPerLnDayFromDates()
//
//@ Description: Returns period length (days). Method is inclusive.
//@ Returns:  0 for invalid dates, positive value for forward spans,
//@     negative values of backward spans, 1 for same date
//-------------------------------------------------------------------

int GetPerLnDayFromDates( int initDay, int initYear, int targDay, int targYear )
   {
   //-- Check for incorrect dates.
   if (  initDay  <= 0
      || initYear <= 0
      || targDay  <= 0
      || targYear <= 0  )
      return 0;

   else if ( initYear == targYear && initDay == targDay )
      return 1;

   //-- Going backwards.
   else if (  targYear < initYear
      || ( targYear == initYear && targDay < initDay )  )
      return -GetPerLnDayFromDates( targDay, targYear, initDay, initYear );

   //-- Going forwards
   else
      {
      int perLnDay = 0;

	   //-- Add year periods.
   	while( initYear < targYear )
      	{
      	perLnDay += GetYearLength( initYear ) - initDay + 1;
      	initDay = 1;
      	initYear++;

      	if ( perLnDay > 30000 )
         	break;
      	}

   	//-- Add remaining days.
   	perLnDay += ( targDay - initDay ) + 1;

      return perLnDay;
      }
   }


//-------------------------------------------------------------------
//-- Not inclusive.

int GetPerLnDayFromDatesNINC( int initDay, int initYear, int targDay,
   int targYear )
   {
   int length = GetPerLnDayFromDates( initDay, initYear, targDay, targYear );

   if ( length == 0 )
      ;

   else if ( length < 0 )
      length += 1;

   else
      length -= 1;

   return length;
   }

//-------------------------------------------------------------------
//@ Method: GetCurrentDate()
//
//@ Description:  Calculates a future calendar date based on a starting
//@ julian date and an offset duration. Passes current day and year. Inclusive.
//
//@ Arguments:
//@ - int  startDay  - start julian day
//@ - int  startYear - start year
//@ - int  offset    - total days: offset 0 or 1 = same day, 2 = next day
//@ - int *pCurrDay  - returned julian day for offset (1-365)
//@ - int *pCurrYear - returned year for offset
//
//@ Comment:  The offset must be <= 0 - "reverse" dates are not calculated.
//@  Also note that the offset is inclusive!
//-------------------------------------------------------------------

void GetCurrentDate(
   int  startDay,    // start day
   int  startYear,   // start year
   int  offset,      // total days: offset 0 or 1 = same day, 2 = next day
   int *pCurrDay,    // pass out
   int *pCurrYear )  // pass out
   {
   *pCurrDay  = startDay;
   *pCurrYear = startYear;

   if ( offset <= 1 ) return;

   for ( int i = 1; i < offset; i++ )
      {
      (*pCurrDay)++;

      if ( *pCurrDay > GetYearLength( *pCurrYear ) )
         {
         *pCurrDay -= GetYearLength( *pCurrYear );
         (*pCurrYear)++;
         }
      }
   }


//-- GetTargDateFromInitDate() -----------------------------------------
//
//   Determines target date from initial date, period length, and
//   step size.  Method is inclusive.
//
//   Passes target date variables.
//-------------------------------------------------------------------

void GetTargDateFromInitDate( int initDay, int initYear,
   int targPerLnStep, int stepSize, int *pTargDay, int *pTargYear )
   {
   *pTargDay  = initDay;
   *pTargYear = initYear;

   int days = targPerLnStep * stepSize;
   if ( days <= 1 ) return;

   GetCurrentDate( initDay, initYear, days, pTargDay, pTargYear );
   }


//-- GetInitDateFromTargDate() -----------------------------------------
//
//   Determines initial date from final date, period length, and step
//   size. Method is inclusive.
//
//   Passes initial date variables.
//-------------------------------------------------------------------

void GetInitDateFromTargDate( int targDay, int targYear,
   int targPerLnStep, int stepSize, int *pInitDay, int *pInitYear )
   {
   *pInitDay  = targDay;
   *pInitYear = targYear;

   int days = targPerLnStep * stepSize;
   if ( days <= 1 ) return;

   for ( int i = 1; i < days; i++ )
      {
      (*pInitDay)--;

      if ( *pInitDay <= 0 )
         {
         (*pInitYear)--;
         *pInitDay += GetYearLength( *pInitYear );
         }
      }
   }


//-- GetMeanProfileDate() -------------------------------------------
//
//   Determines mean date of a lot profile.
//
//   Passes date variables.
//-------------------------------------------------------------------

void GetMeanProfileDate(
   int   initDay,         // mean day of initial lot
   int   initYear,        // year of initial lot
   int   targDay,         // mean day of final lot
   int   targYear,        // year of final lot
   int  *pPopEventDay,    // pass - mean day of event
   int  *pPopEventYear )  // pass - year of mean day of event
   {
   int yearLength = GetYearLength( initYear );

   int eventPerLnDay = ( yearLength * ( targYear - initYear ) ) +
      ( targDay - initDay );

   *pPopEventDay  = initDay + ( eventPerLnDay / 2 );
   *pPopEventYear = initYear;

   if( *pPopEventDay > yearLength )
      {
      *pPopEventDay  -= yearLength;
      (*pPopEventYear)++;
      }
   }



int GetDateSpan( SYSDATE &startDate, SYSDATE &endDate )
   {
   int julDate0 = GetDayOfYear( startDate.month, startDate.day, startDate.year );
   int julDate1 = GetDayOfYear( endDate.month, endDate.day, endDate.year );

   // same date? --//
   if ( startDate.year == endDate.year && julDate0 == julDate1 )
      return 0;

   int span = GetPerLnDayFromDates( julDate0, startDate.year, julDate1, endDate.year );  // inclusive

   if ( span > 0 )            // make it an actual span, rather than inclusive...
      span--;
   else if ( span < 0 )
      span++;

   return span;
   }


bool IsDateValid( int month, int day, int year )     // note: month is 1-based
   {
   //-- Error, return 0.
   if (  month < 1 || month  > 12
      || day   < 1 || day > GetDaysPerMonth( month, year )
      || year  < 0 || year   > 10000  )
      return false;
   else
      return true;
   }


LPCTSTR GetMonthStr( int month )   // month is one-based
   {
   switch( month )
      {
      case 1: return _T("Jan");
      case 2: return _T("Feb");
      case 3: return _T("Mar");
      case 4: return _T("Apr");
      case 5: return _T("May");
      case 6: return _T("Jun");
      case 7: return _T("Jul");
      case 8: return _T("Aug");
      case 9: return _T("Sep");
      case 10: return _T("Oct");
      case 11: return _T("Nov");
      case 12: return _T("Dec");
      }

   return _T( "" );
   }
