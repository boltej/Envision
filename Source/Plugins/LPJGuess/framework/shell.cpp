///////////////////////////////////////////////////////////////////////////////////////
/// \file shell.cpp
/// \brief The "shell" is the model's interface to the world
///
/// \author Joe Siltberg
/// $Date: 2016-12-08 18:24:04 +0100 (Thu, 08 Dec 2016) $
///
///////////////////////////////////////////////////////////////////////////////////////

#include "config.h"
#include "shell.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory>

namespace {

/// The global Shell object
std::auto_ptr<Shell> current_shell;

}

void dprintf(xtring format,...) {
	// printf-style function accessible throughout the model code.
	// Sends text to currently chosen Shell object, which in turn
	// sends it to the screen and/or log file.

	va_list v;
	va_start(v,format);

	xtring output;
	formatf(output,format,v);

	current_shell->log_message(output);
}


void fail(xtring format,...) {
	// printf-style function accessible throughout the model code.
	// Sends text to currently chosen Shell, which prints it and
	// terminates the program.

	va_list v;
	va_start(v,format);

	xtring output;
	formatf(output,format,v);

	current_shell->fail(output);
}


void plot(xtring window_name,xtring series_name,double x,double y) {
	current_shell->plot(window_name, series_name, x, y);
}


void resetwindow(xtring window_name) {
	current_shell->resetwindow(window_name);
}


void clear_all_graphs() {
	current_shell->clear_all_graphs();
}

void open3d() {
	current_shell->open3d();
}

void plot3d(const char* filename) {
	current_shell->plot3d(filename);
}

bool abort_request_received() {
	return current_shell->abort_request_received();
}

void set_shell(Shell* s) {
	current_shell = std::auto_ptr<Shell>(s);
}


////////////////////////////////////////////////////////////////////////////////
//// Implementation of CommandLineShell
////////////////////////////////////////////////////////////////////////////////

CommandLineShell::CommandLineShell(const char* logfile_path) {
	logfile = fopen(logfile_path, "wt");

	if (!logfile) {
		printf("Could not open log file %s for output\n", logfile_path);
		exit(99);
	}
}

CommandLineShell::~CommandLineShell() {
	if (logfile) {
		fclose(logfile);
	}
}

void CommandLineShell::fail(const char* message) {
	log_message(xtring(message)+"\n");
	exit(99);
}

void CommandLineShell::log_message(const char* message) {
	fprintf(stdout,"%s", message);
	fprintf(logfile,"%s", message);
	fflush(logfile);
}

void CommandLineShell::plot(const char* window_name, 
                            const char* series_name, 
                            double x, 
                            double y) {
	// Can't do anything here	 
}

void CommandLineShell::open3d() {
	// Can't do anything here	 
}

void CommandLineShell::plot3d(const char* filename) {
	// Can't do anything here	 
}

void CommandLineShell::resetwindow(const char* window_name) {
	// Can't do anything here
}

void CommandLineShell::clear_all_graphs() {
	// Can't do anything here
}

bool CommandLineShell::abort_request_received() {
	// Can't do anything here

	return false;
}
