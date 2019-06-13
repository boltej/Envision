///////////////////////////////////////////////////////////////////////////////////////
/// \file recursivefilereader.cpp
/// \brief Implementation of the RecursiveFileReader class
///
/// $Date: 2011-05-04 14:13:45 +0200 (Wed, 04 May 2011) $
///////////////////////////////////////////////////////////////////////////////////////

#include "recursivefilereader.h"
#include <stdexcept>

#ifdef WIN32
#include <windows.h>
#else
#include <stdlib.h>
#endif

// Some internal functions for handling paths
namespace {

	// Internal exception class
	// Isn't allowed to propagate outside of RecursiveFileReader
	struct PathError : public std::runtime_error {
		PathError() : std::runtime_error("path error") {}
	};

	// Checks if ch is a letter in the english alphabet
	bool valid_windows_drive(char ch) {
		return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
	}

	// Checks if ch is a path separator
	inline bool is_path_separator(char ch) {
#ifdef WIN32
		return ch == '/' || ch == '\\';
#else
		return ch == '/';
#endif
	}

	// Checks if a path is absolute
	bool is_absolute(xtring path) {
#ifdef WIN32
		// could use PathIsRelative() here, but that requires linking with shlwapi

		// Consider the path to be absolute if it starts with \ or a drive
		// specifier (like C:\)
		return path.left(1) == "\\" ||
			(path.len() >= 3 && valid_windows_drive(path[0]) && path[1] == ':' &&
				is_path_separator(path[2]));
#else
		// On non-Windows systems, consider the path to be absolute if it
		// starts with /
		return path.left(1) == "/";
#endif
	}

	// Given a path to a file, this function returns the containing directory
	// Trailing directory separator is kept, so "/a/b/c.txt" returns "/a/b/"
	xtring get_directory(xtring path) {
		// find the last separator
		int separator_pos = -1;
		for (int i = path.len()-1; i >= 0; i--) {
			if (is_path_separator(path[i])) {
				separator_pos = i;
				break;
			}
		}

		if (separator_pos == -1) {
			// no separator found, no directory to be found
			throw PathError();
		}

		// return everything up to and including the separator
		return path.left(separator_pos+1);
	}

	// Convert a path, relative or absolute, to absolute
	xtring to_absolute_path(xtring path) {
		const int BUFSIZE = 8192;
		char buffer[BUFSIZE];
#ifdef WIN32
		if (GetFullPathNameA((char*)path, BUFSIZE, buffer, NULL) == 0) {
#else
		if (realpath((char*)path, buffer) == NULL) {
#endif
			// failed to convert to absolute path
			throw PathError();
		}
		else {
			return xtring(buffer);
		}
	}

	// Convert a path to absolute
	// If the path is relative, it will be considered to be relative
	// to the directory in which the file pointed to by reference is in.
	xtring to_absolute_path(xtring path, xtring reference) {
		if (is_absolute(path)) {
			return path;
		}
		else {
			return to_absolute_path(get_directory(reference) + path);
		}
	}
}

RecursiveFileReader::RecursiveFileReader() {
}

RecursiveFileReader::~RecursiveFileReader() {
	while (!files.empty()) {
		closeandpop();
	}
}

int RecursiveFileReader::Feof() {
	if (files.empty()) {
		return 1;
	}

	int result = feof(currentstream());
	if (result != 0) {
		// get rid of the current file and try again recursively
		closeandpop();
		return Feof();
	}
	else {
		return 0;
	}
}

int RecursiveFileReader::Fgetc() {
	if (files.empty()) {
		return EOF;
	}
	int result = fgetc(currentstream());

	if (result == EOF) {
		// EOF is returned both on eof and on read error
		if (feof(currentstream())) {
			// eof, so continue with previous file
			closeandpop();
			return Fgetc();
		}
		else {
			// read error, don't try to continue
			return EOF;
		}
	}
	else {
		if (result == '\n') {
			files.top().lineno++;
		}
		return result;
	}
}

bool RecursiveFileReader::addfile(const char* path) {
	xtring absolute_path;

	// Get the absolute path. If the path is relative and we're currently
	// reading from a file, consider it to be relative to that file.
	try {
		if (files.empty()) {
			absolute_path = to_absolute_path(path);
		}
		else {
			absolute_path = to_absolute_path(path, currentfilename());
		}
	} catch (const PathError&) {
		return false;
	}

	FILE* stream = fopen(absolute_path, "rt");
	if (!stream) {
		return false;
	}
	else {
		files.push(File(stream, absolute_path));
		return true;
	}
}

xtring RecursiveFileReader::currentfilename() const {
	return files.top().filename;
}

int RecursiveFileReader::currentlineno() const {
	return files.top().lineno;
}

void RecursiveFileReader::closeandpop() {
	fclose(currentstream());
	files.pop();
}

FILE* RecursiveFileReader::currentstream() const {
	return files.top().stream;
}
