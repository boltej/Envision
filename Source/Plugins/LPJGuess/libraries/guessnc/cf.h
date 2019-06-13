#ifndef LPJGUESS_GUESSNC_CF_H
#define LPJGUESS_GUESSNC_CF_H

#ifdef HAVE_NETCDF

#include "guessnc.h"

namespace GuessNC {

namespace CF {

/// Thrown by code in the CF namespace
/** Typically thrown when encountering a file which doesn't
 *  conform to CF or our limited understanding of CF.
 */
class CFError : public GuessNCError {
public:
	CFError(const std::string& what) : GuessNCError(what) {}

	CFError(const std::string& variable, const std::string& what) : GuessNCError(variable + " : " + what) {}
};

}

}

#endif // HAVE_NETCDF

#endif // LPJGUESS_GUESSNC_CF_H
