////////////////////////////////////////////////////////////////////////////////
/// \file guessstring.cpp
/// \brief Utility functions for working with strings (std::string and char*)
///
/// \author Joe Siltberg
/// $Date: 2014-02-13 10:31:34 +0100 (Thu, 13 Feb 2014) $
///
////////////////////////////////////////////////////////////////////////////////

#include "config.h"
#include "guessstring.h"
#include <cctype>
#include <stdarg.h>
#include <stdio.h>

std::string trim(const std::string& str) {
	size_t start_pos = 0;

	while (start_pos < str.size() && isspace(str[start_pos])) {
		++start_pos;
	}

	size_t end_pos = str.size();

	while (end_pos > 0 && isspace(str[end_pos-1])) {
		--end_pos;
	}

	if (start_pos < end_pos) {
		std::string result(str.begin()+start_pos, str.begin()+end_pos);
		return result;
	}
	else {
		return "";
	}
}

std::string to_upper(const std::string& str) {
	std::string result = str;

	for (size_t i = 0; i < result.size(); ++i) {
		result[i] = toupper(result[i]);
	}
	
	return result;
}

std::string to_lower(const std::string& str) {
	std::string result = str;

	for (size_t i = 0; i < result.size(); ++i) {
		result[i] = tolower(result[i]);
	}
	
	return result;
}

std::string format_string(const char* format, ...) {
	const size_t buffer_size = 4096;
	char buffer[buffer_size];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, buffer_size, format, args);
	return std::string(buffer);
}
