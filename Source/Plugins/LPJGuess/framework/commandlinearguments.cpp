///////////////////////////////////////////////////////////////////////////////////////
/// \file commandlinearguments.cpp
/// \brief Takes care of the command line arguments to LPJ-GUESS
///
/// $Date: 2016-12-08 18:24:04 +0100 (Thu, 08 Dec 2016) $
///
///////////////////////////////////////////////////////////////////////////////////////

#include "config.h"
#include "commandlinearguments.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <algorithm>

namespace {

std::string tolower(const char* str) {
	std::string result(str);
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}

}

CommandLineArguments::CommandLineArguments(int argc, char** argv) 
: help(false),
  parallel(false),
  input_module("cru_ncep") {

	driver_file = "";

	if (!parse_arguments(argc, argv)) {
		print_usage(argv[0]);
	}
}

bool CommandLineArguments::parse_arguments(int argc, char** argv) {
	if (argc < 2) {
		return false;
	}

	// For now, just consider anything starting with '-' to be an option,
	// and anything else to be the ins file
	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '-') {
			std::string option = tolower(argv[i]);
			if (option == "-help") {
				help = true;
			}
			else if (option == "-parallel") {
				parallel = true;
			}
			else if (option == "-input") {
				if (i+1 < argc) {
					std::string module = tolower(argv[i + 1]);
					if (module == "getclim") {
						// If GetClim input module requested, next argument should be path to driver file
						if (i + 2 < argc){
							input_module = argv[i + 1];
							driver_file = argv[i + 2];
							i += 2; // skip two arguments
						}
						else {
							fprintf(stderr, "Missing pathname after -input getclim");
							return false;
						}
					}
					else {
						input_module = argv[i + 1];
						++i; // skip the next argument
					}
				}
				else {
					fprintf(stderr, "Missing argument after -input\n");
					return false;
				}
			}
			else {
				fprintf(stderr, "Unknown option: \"%s\"\n", argv[i]);
				return false;
			}
		}
		else {
			if (insfile.empty()) {
				insfile = argv[i];
			}
			else {
				fprintf(stderr, "Two arguments parsed as ins file: %s and %s\n",
						  insfile.c_str(), argv[i]);
				return false;
			}
		}
	}

	// The only time it's ok not to specify an insfile is if -help is used
	if (insfile.empty() && !help) {
		fprintf(stderr, "No instruction file specified\n");
		return false;
	}

	return true;
}

void CommandLineArguments::print_usage(const char* command_name) const {
	fprintf(stderr, "\nUsage: %s [-parallel] [-input <module_name> [<GetClim-driver-file-path>] ] <instruction-script-filename> | -help\n", 
			  command_name);
	exit(EXIT_FAILURE);
}

bool CommandLineArguments::get_help() const {
	return help;
}

bool CommandLineArguments::get_parallel() const {
	return parallel;
}

const char* CommandLineArguments::get_instruction_file() const {
	return insfile.c_str();
}

const char* CommandLineArguments::get_input_module() const {
	return input_module.c_str();
}

const char* CommandLineArguments::get_driver_file() const {
	return driver_file.c_str();
}
