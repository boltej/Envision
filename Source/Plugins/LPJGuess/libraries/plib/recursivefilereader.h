///////////////////////////////////////////////////////////////////////////////////////
/// \file recursivefilereader.h
/// \brief The RecursiveFileReader class
///
/// $Date: 2011-05-04 14:13:45 +0200 (Wed, 04 May 2011) $
///////////////////////////////////////////////////////////////////////////////////////

#ifndef RECURSIVEFILEREADER_H
#define RECURSIVEFILEREADER_H

// Disable warning in old VC 6.0 due to the limited support for STL
#if defined(_MSC_VER) && (_MSC_VER<1300)
#pragma warning (disable : 4786)
#endif

#include <gutil.h>
#include <stack>

/// Class for reading from files which include other files
/** This class gives plib the illusion that its reading from one simple long
 *  flat file, even if in reality there are several files involved due to the
 *  fact that plib scripts can include other plib scripts.
 *
 * This class behaves like a regular C FILE* stream. At the moment only
 * feof and fgetc are implemented since that is all plib needs, however it
 * should be easy to add other standard functions if needed.
 *
 * To use the class, create an object and call addfile with the path to the
 * initial file. Then start reading from the file with the member functions of
 * RecursiveFileReader. When a new file should be included, call addfile again
 * and the RecursiveFileReader object will start reading from that file until
 * it is empty (or it includes another file). When reaching the end of a file
 * RecursiveFileReader will automatically go back and start reading from the
 * previous file.
 */
class RecursiveFileReader {
public:
	/// Creates a RecursiveFileReader object without any file opened
	RecursiveFileReader();

	/// Closes all remaining open files
	~RecursiveFileReader();

	/// Checks if we have reached the end of the file(s)
	/** Works like the standard feof function but takes into account
	 *  that we might be inside of an included file and thus may have
	 *  more to read by going back to the including file.
	 *
	 *  Odd capitalization due to the fact that some compilers may
	 *  have feof defined as a macro.
	 */
	int Feof();

	/// Gets the next character from the stream
	/** Works like the standard fgetc function but maintains the illusion
	 *  that we're reading from a single flat file.
	 *
	 *  Odd capitalization due to the fact that some compilers may
	 *  have fgetc defined as a macro.
	 */
	int Fgetc();

	/// Start reading from a new file
	/** The file will be opened and will be the new current file from
	 *  which reading is done. When we reach the end of the new file
	 *  we will go back to the file which was the current file when
	 *  addfile was called.
	 *
	 *  @returns whether the file could be opened or not.
	 */
	bool addfile(const char* path);

	/// The file we're currently reading from
	/** Even though we want plib to have the illusion of one flat file,
	 *  it needs to be able to get the current file name in order to print
	 *  out good error messages.
	 */
	xtring currentfilename() const;

	/// The line number we're at in the current file.
	int currentlineno() const;

private:
	/// Closes the current file and pops it from the stack
	void closeandpop();

	/// The FILE* we're currently reading from
	FILE* currentstream() const;

	/// All we need to know about each opened file
	struct File {
		File(FILE* in, const xtring& file)
			: stream(in), filename(file), lineno(1) {}

		FILE* stream;
		xtring filename;
		int lineno;
	};

	/// The currently opened files
	std::stack<File> files;
};

#endif // RECURSIVEFILEREADER_H
