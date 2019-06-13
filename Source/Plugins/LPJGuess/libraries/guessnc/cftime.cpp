#ifdef HAVE_NETCDF

#include "cftime.h"
#include "cf.h"
#include <string>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

namespace GuessNC {

namespace CF {

const int HOURS_PER_DAY        = 24;
const int SECONDS_PER_MINUTE   = 60;
const int MINUTES_PER_HOUR     = 60;
const int SECONDS_PER_HOUR     = SECONDS_PER_MINUTE*MINUTES_PER_HOUR;
const int SECONDS_PER_DAY      = SECONDS_PER_HOUR*HOURS_PER_DAY;
const int MINUTES_PER_DAY      = MINUTES_PER_HOUR*HOURS_PER_DAY;

const int FIRST_GREGORIAN_YEAR = 1583;
const char* PRE_GREGORIAN_ERROR_MESSAGE = "standard/gregorian calendar not supported for dates before 1583";

void check_gregorian_year(int year) {
	if (year < FIRST_GREGORIAN_YEAR) {
		throw CFError(PRE_GREGORIAN_ERROR_MESSAGE);
	}
}

CalendarType parse_calendar(const char* calendar) {
	std::string str(calendar);

	if (str == "standard" || str == "gregorian") {
		return STANDARD;
	}
	else if (str == "proleptic_gregorian") {
		return PROLEPTIC_GREGORIAN;
	}
	else if (str == "noleap" || str == "no_leap" || str == "365_day") {
		return NO_LEAP;
	}
	else {
		throw CFError(std::string("Unsupported calendar type: ") + calendar);
	}
}


TimeUnit parse_time_unit(const char* time_unit) {
	std::string str(time_unit);

	// zero-terminated arrays of synonyms for each time unit

	static const char* s_strings[] =
		{ "seconds", "second", "secs", "sec", "s", 0 };

	static const char* m_strings[] =
		{ "minutes", "minute", "mins", "min", 0};

	static const char* h_strings[] =
		{ "hours", "hour", "hrs", "hr", "h", 0};

	static const char* d_strings[] =
		{ "days", "day", "d", 0 };

	// array of the above arrays, ordered as the TimeUnit enum

	static const char** strings[] = {
		s_strings, m_strings, h_strings, d_strings 
	};

	// search among the synonyms

	for (int i = 0; i < NBR_TIMEUNITS; ++i) {
		for (int j = 0; strings[i][j] != 0; ++j) {
			if (str == strings[i][j]) {
				return static_cast<TimeUnit>(i);
			}
		}
	}

	// nothing found...

	throw CFError(std::string("Unsupported time unit: ") + time_unit);
}


bool is_leap(int year) {
	return (!(year % 4) && (year % 100 | !(year % 400)));
}


int days_in_month(int year, int month, CalendarType calendar_type) {
	
	static const int days_of_month[] = 
		{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	if (calendar_type == NO_LEAP) {
		return days_of_month[month-1];
	}
	else if (calendar_type == STANDARD || 
	         calendar_type == PROLEPTIC_GREGORIAN) {

		if (month != 2 || !is_leap(year)) {
			return days_of_month[month-1];
		}
		else {
			return 29;
		}
	}
	else {
		// Shouldn't happen...
		throw CFError("Unsupported calendar type");
	}
}


int days_in_year(int year, CalendarType calendar_type) {

	if (calendar_type == NO_LEAP) {
		return 365;
	}
	else if (calendar_type == STANDARD || calendar_type == PROLEPTIC_GREGORIAN) {
		if (is_leap(year)) {
			return 366;
		}
		else {
			return 365;
		}
	}
	else {
		// Shouldn't happen...
		throw CFError("Unsupported calendar type");
	}
}


bool valid_date(int year, int month, int day, CalendarType calendar_type) {
	if (month < 1 || month > 12) {
		return false;
	}

	if (day < 1 || day > days_in_month(year, month, calendar_type)) {
		return false;
	}

	if (calendar_type == STANDARD && year < FIRST_GREGORIAN_YEAR) {
		return false;
	}

	return true;
}


DateTime::DateTime()
	:year(0), month(1), day(1), seconds(0) {
}


DateTime::DateTime(int y, int mon, int d,
                   int h, int min, double s) 
	: year(y), month(mon), day(d),
	  seconds(h*SECONDS_PER_HOUR+min*SECONDS_PER_MINUTE+s) {
}


DateTime::DateTime(const char* specification) {

	int hour, minute;
	float second;

	int count = sscanf(specification, "%d-%d-%d %d:%d:%f",
	                   &year, &month, &day, &hour, &minute, &second);

	if (count == 3) { // date only
		hour = 0;
		minute = 0;
		seconds = 0;
	}
	else if (count == 6) { // date and time
		seconds = hour*3600 + minute*60 + second;
	}
	else { // unknown format
		throw CFError(std::string("Unknown date time format: ") + specification);
	}
}

bool DateTime::operator==(const DateTime& other) const {
	return year == other.year &&
		month == other.month &&
		day == other.day &&
		seconds == other.seconds;
}

int DateTime::get_hour() const {
	return static_cast<int>(seconds)/SECONDS_PER_HOUR;
}


int DateTime::get_minute() const {
	return (static_cast<int>(seconds)%SECONDS_PER_HOUR)/SECONDS_PER_MINUTE;
}


double DateTime::get_second() const {
	return seconds - (get_hour()*SECONDS_PER_HOUR+get_minute()*SECONDS_PER_MINUTE);
}


double DateTime::get_seconds_after_midnight() const {
	return seconds;
}

void DateTime::add_time(double time, TimeUnit time_unit, CalendarType calendar_type,
                        double resolution /* = 1.0 */) {

	if (time < 0) {
		throw CFError("Negative time offsets not supported");
	}

	if (resolution < 0) {
		throw CFError("Negative resolution sent to add_time");
	}

	if (calendar_type == STANDARD) {
		check_gregorian_year(year);
	}

	// convert time, which is in the unit time_unit, into two
	// parts: days_increment (unit days) and seconds_increment (unit seconds)
	// (so time is equal to days_increment days plus seconds_increment seconds)
	double days_increment;
	double seconds_increment;

	switch (time_unit) {
	case SECONDS:
		days_increment = time/SECONDS_PER_DAY;
		break;
	case MINUTES:
		days_increment = time/MINUTES_PER_DAY;
		break;
	case HOURS:
		days_increment = time/HOURS_PER_DAY;
		break;
	case DAYS:
		days_increment = time;
		break;
	default:
		// should never happen
		throw CFError("Unsupported time unit in DateTime::add_time");
	}

	// keep only integer part in days_increment, and multiply fractional
	// part by SECONDS_PER_DAY to get seconds_increment
	seconds_increment = modf(days_increment, &days_increment) * SECONDS_PER_DAY;

	add_days(static_cast<int>(days_increment), calendar_type);

	seconds += seconds_increment;

	// overflow?
	if (seconds >= SECONDS_PER_DAY) {
		seconds -= SECONDS_PER_DAY;
		add_days(1, calendar_type);
	}

	// rounding
	if (resolution != 0) {
		seconds = floor(seconds/resolution+0.5)*resolution;
	
		if (seconds >= SECONDS_PER_DAY) {
			seconds -= SECONDS_PER_DAY;
			add_days(1, calendar_type);
		}
	}
}

int DateTime::get_day_index(CalendarType calendar_type) const {
	// todo: optimize
	int result = 0;
	
	for (int i = 1; i < month; ++i) {
		result += days_in_month(year, i, calendar_type);
	}
	
	result += day - 1;
	return result;
}

void DateTime::skip_ahead_whole_years(int& day_increment, CalendarType calendar_type) {
	// todo: optimize
	int day_index = get_day_index(calendar_type);

	while (day_index + day_increment >= days_in_year(year, calendar_type)) {
		int days_added = days_in_year(year, calendar_type)-day_index;
		day_increment -= days_added;
		day = 1;
		month = 1;
		++year;
		day_index = 0;
	}
}

void DateTime::skip_ahead_whole_months(int& day_increment, CalendarType calendar_type) {

	// assumes we're called just after skip_ahead_whole_years, so
	// day_increments isn't large enough to cause year to increase

	while (day + day_increment > days_in_month(year, month, calendar_type)) {
		int days_added = days_in_month(year, month, calendar_type) - day + 1;
		day_increment -= days_added;
		day = 1;
		++month;
	}
}

void DateTime::add_days(int day_increment, CalendarType calendar_type) {

	// make sure we start with a valid date
	assert(valid_date(year, month, day, calendar_type));

	skip_ahead_whole_years(day_increment, calendar_type);

	skip_ahead_whole_months(day_increment, calendar_type);

	// day_increment should now be small enough not to cause month increase
	day += day_increment;

	// make sure we end with a valid date
	assert(valid_date(year, month, day, calendar_type));
}

std::string DateTime::to_string() const {
	char buff[256];
	sprintf(buff, "%d-%02d-%02d %02d:%02d:%05.2f", 
	        year, month, day, get_hour(), get_minute(), get_second());
	return std::string(buff);
}

TimeUnitSpecification::TimeUnitSpecification()
	: time_unit(SECONDS),
	  reference_time(1970, 01, 01, 0, 0, 0) {
}

TimeUnitSpecification::TimeUnitSpecification(const char* specification) {
	char buff[256];
	
	sscanf(specification, "%s", buff);

	time_unit = parse_time_unit(buff);

	const char* psince = strstr(specification, "since");
	
	if (!psince) {
		throw CFError(std::string("Bad time unit string: ") + specification);
	}

	reference_time = DateTime(psince+strlen("since"));
}

DateTime TimeUnitSpecification::get_date_time(double time, CalendarType calendar,
                                              double resolution /* = 1.0 */) const {
	DateTime result = reference_time;
	result.add_time(time, time_unit, calendar, resolution);
	return result;
}

} // namespace CF

} // namespace GuessNC

#endif // HAVE_NETCDF
