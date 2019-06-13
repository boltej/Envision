///////////////////////////////////////////////////////////////////////////////////////
/// \file plib.h
/// \brief PLIB Version 2.1 (Fully portable version)
///
/// PLIB is a utility library for reading input from a profile script. This is a
/// text file consisting of commands and data, in a format defined in part by PLIB,
/// and in part by the calling program. Language elements supported are:
///
/// -  COMMANDS: statements beginning with an identifier (a string of non-white
///    space characters commencing with an alphabetic character and not including
///    any of the following: - + . " ' ( ) !. The identifier may be followed by
///    a string (enclosed in single or double quotation marks) with a specified
///    maximum significant length, a specified number of integers or real numbers
///    in a specified range, a specified number of bool values (numerals
///    interpreted such that 0=false, non-zero=true) or no parameters at all.
///    Identifiers are non-case-sensitive.
///    e.g.
///    \code
///         title "Kiruna"
///         mtemp -14.2 -13.5 -9.5 -3.5 2.9 9.3 12.6 10.3 4.8 -1.5 -8.1 -12.1
///         include 1
///         exit
///    \endcode
///
/// -  SETS: named blocks of script enclosed in parentheses. Sets may be nested.
///    e.g.
///    \code
///         taxon "picea" ( ... )
///    \endcode
///
/// -  GROUPS: named blocks of script enclosed in parentheses which are interpreted
///    as "macros" when the script is interpreted. The contents of the group are
///    implicitly inserted in the script wherever the name of the group appears
///    following the group definition. Groups may be nested, but always have
///    'global' scope. Groups are identified by the keyword group followed by the
///    group name in single or double quotation marks.
///    e.g.
///    \code
///         group 'woody' ( tree 1 wooddens 250 leaftoroot 1.0 )
///         taxon "picea" ( woody )
///    \endcode
///
/// -  COMMENTS: strings of characters beginning with an exclamation mark (!). They
///    are ignored to the end of the line they appear on.
///    e.g.
///    \code
///         !a comment example
///    \endcode
///
/// \section using_plib Using PLIB
///
/// The recommended way of building an executable incorporating the functionality
/// of PLIB is to include plib.h and plib.cpp into your project as regular source
/// code files. If you place plib.h somewhere in your include path, include PLIB
/// with:
///
/// \code
/// #include <plib.h>
/// \endcode
///
/// Otherwise, you may have to specify a relative or full path name, such as:
///
/// \code
/// #include "include/plib.h"
/// \endcode
///
/// Character strings are represented in PLIB as xtring objects. String arguments
/// to PLIB library functions are also of type xtring. The xtring class is included
/// in the GUTIL library.
///
///
/// \section reading Reading information from a PLIB script
///
/// To process a PLIB script, call function plib, specifying the full pathname of
/// the script file to process:
///
/// bool plib(xtring filename)
///
/// Function plib returns TRUE if there were no errors, otherwise FALSE (zero).
/// In the event of an error, an explanatory message is sent to function
/// plib_receivemessage in the calling program.
///
///
/// \section communication Communication between PLIB and the calling program
///
/// Communication is via PLIB library functions, accessible to the calling program.
/// The calling program must also implement two, and in some cases, three, special
/// functions accessible to PLIB:
///
/// -  plib_declarations
/// -  plib_receivemessage
/// -  plib_callback
///
/// The calling program must supply functions plib_declarations and
/// plib_receivemessage and, if the callback feature is implemented for any command
/// function plib_callback.
///
///
/// Enquiries to: Joe Siltberg, Lund University: joe.siltberg@nateko.lu.se
/// All rights reserved, copyright retained by the author.
///
/// \author Ben Smith, University of Lund
/// $Date: 2013-07-30 14:52:32 +0200 (Tue, 30 Jul 2013) $
///
///////////////////////////////////////////////////////////////////////////////////////

#ifndef PLIB_H
#define PLIB_H

#include <gutil.h>
#include <string>

// PLIB LIBRARY FUNCTIONS

/// Starts the processing of a PLIB script
/** This function will call the function plib_declarations to declare
 *  all items.
 *
 *  This function may also call plib_callback if the callback feature
 *  is used.
 *
 *  In the event of an error, an explanatory message is sent to function
 *  plib_receivemessage in the calling program.
 *
 *  It is the responsibility of the calling program to implement these
 *  functions.
 *
 *  \returns TRUE if there were no errors, otherwise FALSE (zero).
 */
bool plib(xtring filename);

/// Declares a string (xtring) parameter
/** When the string specified by identifier is encountered, expect the next item
 *  to be a string and write it to 'param'. 'maxlen' specifies the maximum
 *  allowable length of the string, not including the terminating null character;
 *  the string is truncated, if necessary, before being written to 'param'. If
 *  'callback' is non-zero, function plib_callback is called with the
 *  integer value of 'callback' as a parameter, AFTER the assignment to param.
 *  e.g.
 *  \code
 *  title "kiruna"
 *  \endcode
 */
bool declareitem(xtring identifier,xtring* param,int maxlen,int callback,
	xtring help = "");

/// Declares a string (std::string) parameter
/** Works like the corresponding function for xtring
 */
bool declareitem(xtring identifier, std::string* param, int maxlen, int callback,
	xtring help = "");

/// Declares an integer parameter
/** Here the number of integers specified in 'nparam' are expected after the
 *  identifier and assigned to the array (or simple variable) pointed to by
 *  'param'. Floating-point numbers are rounded to the nearest integer. The
 *  parameters should be in the range 'min'-'max', otherwise a PLIB error
 *  results. e.g.
 *   \code
 *   npat 1000
 *   \endcode
 */
bool declareitem(xtring identifier,int* param,int min,int max,int nparam,
	int callback, xtring help = "");

/// Declares a floating-point parameter
/** Here the number of floating-point numbers specified in 'nparam' are expected
 *  after the identifier and assigned to the array (or simple variable) pointed
 *  to by 'param'. The parameters should be in the range 'min'-'max', otherwise
 *  a PLIB error results. e.g.
 *  \code
 *  mtemp -14.2 -13.5 -9.5 -3.5 2.9 9.3 12.6 10.3 4.8 -1.5 -8.1 -12.1
 *  \endcode
 */
bool declareitem(xtring identifier,double* param,double min,double max,int nparam,
	int callback,xtring help = "");

/// Declares a boolean parameter
/** Here the number of numbers (nominally integers 0 or 1) specified in 'nparam'
 *  are expected after the identifier and are reinterpreted as bools (0=false,
 *  non-zero=true) and assigned to the array (or simple variable) pointed to by
 *  'param'. e.g.
 *  \code
 *  include 1
 *  \endcode
 */
bool declareitem(xtring identifier,bool* param,int nparam,int callback,
	xtring help = "");

/// Declares a boolean parameter
/** If this identifier is encountered, the variable pointed to by 'param' is
 *  assigned the value true. e.g.
 *  \code
 *  exit
 *  \endcode
 */
bool declareitem(xtring identifier,bool* param,int callback,xtring help = "");

/// Declares a set header
/** Specifies the identifier and associated id-code for a set header. If the
 *  specified identifier is encountered, it is interpreted as a set header and
 *  should be followed by the set name as a string and an open parenthesis.
 *  Function plib_declarations is then called, passing 'id' from this call to
 *  declareitem and the set name encountered in the script. e.g.
 *  \code
 *  taxon "picea" (
 *  \endcode
 */
bool declareitem(xtring identifier,int id,int callback,xtring help = "");

/// Call this if you want to be notified when the PLIB parsing is done
/** Function callwhendone declares a callback code which, if non-zero, is passed in
 *  a call to function plib_callback following input of the last recognised PLIB
 *  item in the current set. This can provide the calling program with an
 *  opportunity to query, via function itemparsed, which of the items declared for
 *  this set were actually encountered in the script.
 */
void callwhendone(int callback);

/// Checks whether a certain item has been read in from the script
/** Function itemparsed should be called from function plib_callback following
 *  input of data for a particular set (see function callwhendone above). It
 *  returns true if the string 'identifier' corresponds to one of the PLIB
 *  commands declared by calls to function declareitem for this set, AND this
 *  command was encountered in processing this set in the script. If 'identifier'
 *  is not recognised, or was not encountered in the script, the return value is
 *  false.
 */
bool itemparsed(xtring identifier);

/// Send a message to the user during parsing
/** Function sendmessage may be called by the calling program to output warning or
 *  informational messages to the user during processing of a PLIB script.
 *  Typically sendmessage would be called from within function plib_callback.
 *  Parameter 'heading' should consist of a keyword identifying the type of message
 *  sent (e.g. "Warning"). More detailed information may be given within parameter
 *  'message'.
 */
void sendmessage(xtring heading,xtring message);

/// Gives the user documentation on all declared keywords
/** Documentation of a calling module's native keywords (as defined in function
 *  plib_declarations) is possible using the 'xtring help' parameter of function
 *  declareitem. A list of all keywords and their associated help text (if
 *  provided) is sent as sequential calls (one per line of output text) to function
 *  plib_receivemessage, when function plibhelp is called from the calling program.
 */
void plibhelp();

/// Aborts the processing of a PLIB script
/** This function is provided to allow processing of a PLIB script to be forcibly
 *  aborted. Typically, plibabort would be called from within function
 *  plib_callback.
 */
void plibabort();




// FUNCTIONS TO BE IMPLEMENTED BY THE CALLING PROGRAM

/// This is where the calling program configures PLIB
/** This function is called by PLIB just before interpretation of a script
 *  begins, and subsequently whenever a set header is encountered. It should
 *  include calls to the functions declareitem which define the identifiers and
 *  formats for commands and sets in the script (or this set), and what action
 *  should be taken when a particular command or set identifier is encountered.
 */
void plib_declarations(int id,xtring setname);

/// This function can be called by PLIB when certain commands are read
/** Implement this function in the calling program to handle callbacks from
 *  PLIB. To get a call when a command is read in, specify a non-zero
 *  integer value as the callback parameter when declaring the item
 *  with declareitem. This integer value is then sent into plib_callback
 *  when the parameter is read.
 */
void plib_callback(int callback);

/// Called by PLIB whenever output should be conveyed to the user.
/** Plib sends output in the form of an xtring (character string) object. It
 *  is the responsibility of the calling program to send this string to an
 *  output stream or device (typically the screen, or a text file). Output
 *  generated by PLIB includes error messages, help text and messages
 *  originating from the calling program and sent to PLIB via function
 *  sendmessage.
 */
void plib_receivemessage(xtring text);

#endif // PLIB_H
