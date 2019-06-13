////////////////////////////////////////////////////////////////////////////////
/// \file guessstring.h
/// \brief Utility functions for working with strings (std::string and char*)
///
/// \author Joe Siltberg
/// $Date: 2014-02-13 10:31:34 +0100 (Thu, 13 Feb 2014) $
///
////////////////////////////////////////////////////////////////////////////////

#ifndef LPJ_GUESS_GUESSSTRING_H
#define LPJ_GUESS_GUESSSTRING_H

#include <string>

/// Removes leading and trailing whitespace from a string
std::string trim(const std::string& str);

/// Converts a string to upper case
std::string to_upper(const std::string& str);

/// Converts a string to lower case
std::string to_lower(const std::string& str);

/// Creates a string with printf style formatting
std::string format_string(const char* format, ...);

#endif // LPJ_GUESS_GUESSSTRING_H
