///////////////////////////////////////////////////////////////////////////////////////
/// \file framework.h
/// \brief The 'mission control' of the model
///
/// $Date: 2012-09-20 15:22:28 +0200 (Thu, 20 Sep 2012) $
///
///////////////////////////////////////////////////////////////////////////////////////

#ifndef LPJ_GUESS_FRAMEWORK_H
#define LPJ_GUESS_FRAMEWORK_H

class CommandLineArguments;

/// The 'mission control' of the model
/** 
 *  Responsible for maintaining the primary model data structures and containing
 *  all explicit loops through space (grid cells/stands) and time (days and 
 *  years).
 *
 *  \param args The command line arguments, sent in from main
 *
 */
int framework(const CommandLineArguments& args);

#endif // LPJ_GUESS_FRAMEWORK_H
