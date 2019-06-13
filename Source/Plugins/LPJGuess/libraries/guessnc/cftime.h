#ifndef LPJGUESS_GUESSNC_CFTIME_H
#define LPJGUESS_GUESSNC_CFTIME_H

#ifdef HAVE_NETCDF

#include <string>

namespace GuessNC {

namespace CF {

/// CF calendar types supported by this library
enum CalendarType {
	STANDARD,            // also known as gregorian
	PROLEPTIC_GREGORIAN, // a Gregorian calendar extended to dates before the Julian/Gregorian switch
	NO_LEAP
};


/// Parses a calendar string specification (such as "gregorian")
CalendarType parse_calendar(const char* calendar);


/// CF time units supported by this library
enum TimeUnit {
	SECONDS = 0,
	MINUTES,
	HOURS,
	DAYS,
	NBR_TIMEUNITS
};


/// Parses a time unit string specification (such as "days")
TimeUnit parse_time_unit(const char* time_unit);


/// Represents a date and time of day
/** Does not support time zones
 */
class DateTime {
public:
	/// Constructs a default DateTime (0000-01-01 00:00:00.0)
	DateTime();

	/// Constructs a DateTime
	/**
	 *  \param year   Calendar year
	 *  \param month  Month (1-12)
	 *  \param day    Day (1-31)
	 *  \param hour   Hour (0-23)
	 *  \param minute Minute (0-59)
	 *  \param second Second (0 up to, but not including 60)
	 */
	DateTime(int year, int month, int day,
	         int hour, int minute, double second);

	/// Constructs a DateTime from a CF date time specification
	/** The time specification is a string such as "1973-01-01 04:20:10"
	 *  If the string contains time zone information, that part will be 
	 *  ignored.
	 */
	DateTime(const char* specification);

	/// Comparison operator
	bool operator==(const DateTime& other) const;

	/// \returns year
	int get_year() const { return year; }

	/// \returns month number (1-12)
	int get_month() const { return month; }

	/// \returns day number (1-31)
	int get_day() const { return day; }

	/// \returns hour (0-23)
	int get_hour() const;

	/// \returns minute (0-59)
	int get_minute() const;

	/// \returns second (0 up to, but not including 60)
	/** Note that this might return fractional seconds */
	double get_second() const;

	/// \returns seconds since midnight this date
	double get_seconds_after_midnight() const;

	/// Adds an amount of time of a given time unit, given a certain calendar
	/** The original date is assumed to be a valid date in the given calendar.
	 *
	 *  The result is by default rounded to closest second. The reason for this
	 *  is that doubles can't represent all numbers perfectly, so you may e.g.
	 *  end up a nanosecond before midnight even though you shouldn't, which
	 *  can be very bad if you care about dates but not about nanoseconds.
	 *  To disable this, set resolution to 0. 
	 */
	void add_time(double time, TimeUnit time_unit, CalendarType calendar_type,
	              double resolution = 1.0);

	/// Creates a string representation of the object
	std::string to_string() const;

private:

	/// Adds a given number of days to the date, given a certain calendar
	void add_days(int days, CalendarType calendar_type);

	/// Returns days since Jan 1 this year
	int get_day_index(CalendarType calendar_type) const;
	
	/// Help function for add_days, takes year long leaps
	void skip_ahead_whole_years(int& days, CalendarType calendar_type);

	/// Help function for add_days, takes month long leaps
	void skip_ahead_whole_months(int& days, CalendarType calendar_type);

	/// calendar year
	int year;

	/// calendar month (1-12)
	int month;

	/// calendar day (1-31)
	int day;

	/// seconds after midnight (0 up to, but not including 86400)
	double seconds;
};


/// A CF time unit specification consisting of a starting point and a time unit
class TimeUnitSpecification {
public:

	/// Default constructor
	/** Creates a time unit specification representing seconds since
	 *  the standard Unix epoch (1970-01-01 00:00:00). */
	TimeUnitSpecification();

	/// Constructor
	/** Parses a CF time unit specification string,
	 *  such as "days since 1970-01-01 00:00:00" */
	TimeUnitSpecification(const char* specification);

	/// Returns the DateTime corresponding to a time offset
	/** Calculates a DateTime in a certain calendar by adding
	 *  the time parameter to the reference_time according
	 *  to the chosen time_unit.
	 *
	 *  Rounds to nearest second by default, see resolution parameter 
	 *  to DateTime::add_time
	 */
	DateTime get_date_time(double time, CalendarType calendar,
	                       double resolution = 1.0) const;

private:
	/// Start time
	DateTime reference_time;

	/// How to interpret time offsets
	TimeUnit time_unit;
};

} // namespace CF

} // namespace GuessNC

#endif // HAVE_NETCDF

#endif // LPJGUESS_GUESSNC_CFTIME_H
