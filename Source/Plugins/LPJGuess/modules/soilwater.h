///////////////////////////////////////////////////////////////////////////////////////
/// \file soilwater.h
/// \brief Soil hydrology and snow
///
/// Version including evaporation from soil surface, based on work by Dieter Gerten,
/// Sibyll Schaphoff and Wolfgang Lucht, Potsdam
///
/// \author Ben Smith
/// $Date: 2015-11-13 16:25:45 +0100 (Fri, 13 Nov 2015) $
///
///////////////////////////////////////////////////////////////////////////////////////

// WHAT SHOULD THIS FILE CONTAIN?
// Module header files need normally contain only declarations of functions defined in
// the module that are to be accessible to the calling framework or to other modules.

#ifndef LPJ_GUESS_SOILWATER_H
#define LPJ_GUESS_SOILWATER_H

#include "guess.h"
void initial_infiltration(Patch& patch, Climate& climate);
void irrigation(Patch& patch);
void soilwater(Patch& patch, Climate& climate);

#endif // LPJ_GUESS_SOILWATER_H
