///////////////////////////////////////////////////////////////////////////////////////
/// \file outputchannel.h
/// \brief Classes for formatting and printing output from the model
///
/// \author Joe Siltberg
/// $Date: 2015-11-13 16:25:45 +0100 (Fri, 13 Nov 2015) $
///
///////////////////////////////////////////////////////////////////////////////////////

#ifndef LPJ_GUESS_OUTPUT_CHANNEL_H
#define LPJ_GUESS_OUTPUT_CHANNEL_H

#include <string>
#include <vector>

namespace GuessOutput {

/// Describes one column in an output table
class ColumnDescriptor {
public:
	 /// Creates a ColumnDescriptor
	 /** \param title The title for the column
	  *  \param width Width of column for output formats of fixed width
	  *  \param precision Number of digits after the decimal point
	  */
	 ColumnDescriptor(const char* title, int width, int precision);

	 /// Get the column title
	 const std::string& title() const;

	 /// Get the column width
	 int width() const;

	 /// Get the column precision
	 int precision() const;

private:
	 std::string t;
	 int w;
	 int p;
};

/// A list of column descriptors
class ColumnDescriptors {
public:
	 /// Creates a ColumnDescriptors with no columns
	 ColumnDescriptors() {}

	 /// Creates a ColumnDescriptors where all columns have the same format
	 ColumnDescriptors(const std::vector<std::string>& titles,
	                   int width,
	                   int precision);

	 /// Add a ColumnDescriptor to the end of the list
	 void operator+=(const ColumnDescriptor&  col);

	 /// Append a ColumnDescriptors to the end of this one
	 void operator+=(const ColumnDescriptors& cols);

	 /// Get the number of columns
	 size_t size() const;

	 /// Get one ColumnDescriptor
	 const ColumnDescriptor& operator[](size_t i) const;

private:
	 std::vector<ColumnDescriptor> columns;
};

/// Describes an output table
/** Each table is described by a name and a number of column descriptors.
 */
class TableDescriptor {
public:
	 /// Creates a TableDescriptor
	 TableDescriptor(const char* name,
	                 const ColumnDescriptors& columns);

	 /// Get the name of the table
	 const std::string& name() const;

	 /// Get the column descriptors
	 const ColumnDescriptors& columns() const;

private:
	 std::string n;
	 ColumnDescriptors cols;
};

/// A handle to an output table
/** Should be created by a call to create_table in an OutputChannel */
class Table {
public:
	 /// Creates an invalid table
	 Table();

	 /// Creates a table with a given id
	 Table(int id);

	 /// Get the id of the table
	 int id() const;

	 /// Is this an invalid table?
	 bool invalid() const;
private:
	 int identifier;
};

/// The interface to an output channel
/** All output should be sent via an output channel. The output channel
 *  takes care of formatting the values and writing it to the right files,
 *  database tables or whatever.
 *
 *  This class is an abstract base class, so a suitable sub-class should
 *  be used, for instance FileOutputChannel for regular file output.
 */
class OutputChannel {
public:
	 virtual ~OutputChannel() {}

	 /// Creates an output table and returns a handle to it
	 virtual Table create_table(const TableDescriptor& descriptor);

	 /// Adds a value to the next row of output for a given table
	 /** If the table is invalid, no action is taken. */
	 virtual void add_value(const Table& table, double d);

	 /// Finalizes the output of one row, annual output
	 virtual void finish_row(const Table& table, double lon, double lat,
	                         int year) = 0;

	 /// Finalizes the output of one row, daily output
	 virtual void finish_row(const Table& table, double lon, double lat,
	                         int year, int day) = 0;

	 virtual void close_table(Table& table) = 0;

protected:
	 /// Get the table descriptor for a table
	 const TableDescriptor& get_table_descriptor(const Table& table) const;

	 /// Get the added values for the current row
	 const std::vector<double> get_current_row(const Table& table) const;

	 /// Should be called at the end of finish_row
	 /** Removes all values stores in memory for the current row */
	 void clear_current_row(const Table& table);

private:
	 std::vector<TableDescriptor> table_descriptors;
	 std::vector<std::vector<double> > values;
};

/// An output channel for regular text files with fixed width columns
/** This output channel creates one text file for each output table.
 */
class FileOutputChannel : public OutputChannel {
public:
	 /// Creates a FileOutputChannel
	 /** \param out_dir All output files are placed in this directory.
	  *  \param coords_precision Precision to use when printing coordinates
	  */
	 FileOutputChannel(const char* out_dir, int coords_precision);

	 /// Destructor - closes all opened files
	 ~FileOutputChannel();

	 /// Creates an output file
	 /** \see OutputChannel::create_table */
	 Table create_table(const TableDescriptor& descriptor);

	 void close_table(Table& table);

	 /// Prints the values of the current row to the file
	 /** \see OutputChannel::finish_row */
	 void finish_row(const Table& table, double lon, double lat,
	                 int year);

	 /// Prints the values of the current row to the file
	 /** \see OutputChannel::finish_row */
	 void finish_row(const Table& table, double lon, double lat,
	                 int year, int day);

private:
	 /// Help function to the two variants of finish_row above
	 void finish_row(const Table& table, double lon, double lat,
	                 int year, int day, bool print_day);

	 /// Returns the printf style format string to be used for a column
	 const char* format(const Table& table, int column);

	 /// Formats a column title
	 const char* format_header(const Table& table, int column);

	 const std::string output_directory;

	 std::string coords_title_format;
	 std::string coords_format;

	 std::vector<FILE*> files;

	 /// Whether the header has been printed for each file
	 std::vector<bool> printed_header;
};

/// A convenience class for managing the output of one row to multiple tables.
/** At the end of the life time of an object of this class, finish_row is
 *  called for all tables that have gotten values.
 */
class OutputRows {
public:
	 /// OutputRows for annual output
	 OutputRows(OutputChannel* output_channel, double longitude, double latitude, int year);

	 /// OutputRows for daily output
	 OutputRows(OutputChannel* output_channel, double longitude, double latitude, int year, int day);

	 /// Calls finish_row for all involved tables
	 ~OutputRows();

	 /// Adds a value to one of the tables
	 /** \see OutputChannel::add_value */
	 void add_value(const Table& table, double d);

private:
	 OutputChannel* out;
	 double lon;
	 double lat;
	 int y;
	 int d;

	 std::vector<bool> used_tables;
};

/// Help function to prepare C:N values for output
/** Avoids division by zero and limits the results to a maximum
 *  value to avoid inf or values large enough to ruin the alignment
 *  in the output.
 *
 *  If both cmass and nmass is 0, the function returns 0.
 */
inline double limited_cton(double cmass, double nmass) {
	const double MAX_CTON = 1000;

	if (nmass > 0.0) {
		return min(MAX_CTON, cmass / nmass);
	}
	else if (cmass > 0.0) {
		return MAX_CTON;
	}
	else {
		return 0.0;
	}
}


}

#endif // LPJ_GUESS_OUTPUT_CHANNEL_H
