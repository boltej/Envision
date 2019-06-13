///////////////////////////////////////////////////////////////////////////////////////
/// \file shell.h
/// \brief The "shell" is the model's interface to the world
///
/// \author Joe Siltberg
/// $Date: 2016-12-08 18:24:04 +0100 (Thu, 08 Dec 2016) $
///
///////////////////////////////////////////////////////////////////////////////////////

#ifndef LPJ_GUESS_SHELL_H
#define LPJ_GUESS_SHELL_H

#include "gutil.h"

/// A printf-style function for sending messages from LPJ-GUESS to the user.
/**
 *  The text is typically sent to the screen and/or a log file. To maintain 
 *  portability of the modular code, please use this function for general
 *  output instead of the standard C++ printf function.
 */
void dprintf(xtring format,...);


/// A printf-style function that sends the user a message and then terminates.
void fail(xtring format,...);


/// Adds data point (x,y) to series 'series_name' of line graph 'window_name'.
/**
 * If the series and/or line graph do not yet exist, they are created. 
 * Functional only when the framework is built as a DLL and linked to the 
 * LPJ-GUESS Windows Shell (the function may still be called in other 
 * implementations, but will have no effect).
 */
void plot(xtring window_name,xtring series_name,double x,double y);


/// 'Forgets' series and data for line graph 'window_name'.
/**
 * Functional only when the framework is built as a DLL and linked to the 
 * LPJ-GUESS Windows Shell.
 */
void resetwindow(xtring window_name);


/// 'Forgets' series and data for all currently-defined line graphs.
/** 
 * Functional only when the framework is built as a DLL and linked to the 
 * LPJ-GUESS Windows Shell.
 */
void clear_all_graphs();


/// Initiates a 3D view of stand vegetation in the Windows shell
/**
* Functional only when the framework is built as a DLL and linked to the
* LPJ-GUESS Windows Shell.
*/
void open3d();

/// Sends data on current stand structure to 3D vegetation plot in the Windows shell
/**
* Functional only when the framework is built as a DLL and linked to the
* LPJ-GUESS Windows Shell.
*/
void plot3d(const char* filename);

/// May be called by framework to respond to abort request from the user.
/**
 * \returns true if shell has sent an abort request, otherwise false.
 */
bool abort_request_received();


/// The interface LPJ-GUESS uses to communicate with the world
/**
 *  This is an abstract base class, which is sub-classed by
 *  concrete implementations for different ways of running LPJ-GUESS.
 *
 *  See CommandLineShell which implements this interface for when
 *  LPJ-GUESS runs as a regular command line client.
 */
class Shell {
public:
	virtual ~Shell() {}

	/// Sends a message to the user somehow and terminates the program
	virtual void fail(const char* message) = 0;

	/// Sends a message to the user somehow
	virtual void log_message(const char* message) = 0;

	/// Adds data point (x,y) to series 'series_name' of line graph 'window_name'.
	virtual void plot(const char* window_name, 
	                  const char* series_name, 
	                  double x, 
	                  double y) = 0;

	/// 'Forgets' series and data for line graph 'window_name'.
	virtual void resetwindow(const char* window_name) = 0;

	/// 'Forgets' series and data for all currently-defined line graphs.
	virtual void clear_all_graphs() = 0;

	/// May be called by framework to respond to abort request from the user.
	virtual bool abort_request_received() = 0;

	/// Initiates a 3D view of stand vegetation in the Windows shell
	virtual void open3d() = 0;

	/// Sends data on current stand structure to 3D vegetation plot in the Windows shell
	virtual void plot3d(const char* filename) = 0;
};



/// A Shell which sends messages to the terminal and log file
/**
 *  This class ignores the plotting related functions.
 */
class CommandLineShell : public Shell {
public:
	CommandLineShell(const char* logfile_path);

	~CommandLineShell();

	void fail(const char* message);

	void log_message(const char* message);

	void plot(const char* window_name, 
	          const char* series_name, 
	          double x, 
	          double y);

	void resetwindow(const char* window_name);

	void open3d();

	void plot3d(const char* filename);

	void clear_all_graphs();

	bool abort_request_received();

private:
	FILE* logfile;
};



/// Chooses a global shell object
/**
 *  Should be called at program start up, before the model starts running.
 *
 *  If LPJ-GUESS is running as a stand-alone program, this is called from 
 *  main().
 */
void set_shell(Shell* s);


#endif // LPJ_GUESS_SHELL_H
