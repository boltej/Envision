///////////////////////////////////////////////////////////////////////////////////////
/// \file outputmodule.h
/// \brief Base class for output modules and a container class for output modules
///
/// \author Joe Siltberg
/// $Date: 2015-11-13 16:25:45 +0100 (Fri, 13 Nov 2015) $
///
///////////////////////////////////////////////////////////////////////////////////////

#ifndef LPJ_GUESS_OUTPUT_MODULE_H
#define LPJ_GUESS_OUTPUT_MODULE_H

#include <vector>
#include <string>
#include <map>
#include "outputchannel.h"

class Gridcell;

namespace GuessOutput {

/// Base class for output modules
/**
 *  An output module should inherit from this class and implement
 *  the pure virtual functions.
 *
 *  In order for the output module to be used by the framework,
 *  it must be registered with the REGISTER_OUTPUT_MODULE macro
 *  (defined and documented below).
 */
class OutputModule {
public:
	virtual ~OutputModule() {};

	/// Called after the instruction file has been read
	/**
	 *  If an output module needs to declare its own instruction
	 *  file parameters, it should declare them in its constructor
	 *  (which will be called before the instrction file is read).
	 *
	 *  Typically, some of the output module's initialization
	 *  can't be done until the instruction file parameters are
	 *  available, and is therefore done in this init function.
	 */
	virtual void init() = 0;

	/// Called by the framework at the end of the last day of each simulation year
	/** The function should generate simulation results for the year, typically
	 *  by getting values from the gridcell, doing various calculations, and
	 *  sending the results along to the output channel (\see OutputChannel) */
	virtual void outannual(Gridcell& gridcell) = 0;

	/// Called by the framework at the end of each simulation day
	/** Similar to outannual but called every day */
	virtual void outdaily(Gridcell& gridcell) = 0;

protected:

	/// Help function to define_output_tables, creates one output table
	void create_output_table(Table& table,
	                         const char* file,
	                         const ColumnDescriptors& columns);

	void close_output_table(Table& table);
};


/// Manages a list of output modules
/**
 *  Apart from simply storing the output modules,
 *  this class is also a place for things used by all output
 *  modules (for instance creating the OutputChannel or
 *  declaring the 'outputdirectory' parameter).
 */
class OutputModuleContainer {
public:
	OutputModuleContainer();

	~OutputModuleContainer();

	/// Adds an output module to the container
	/** The container will deallocate the module when
	 *  the container is destructed.
	 */
	void add(OutputModule* output_module);

	/// Calls init on all output modules
	/** Should be called after the instruction file has been read */
	void init();

	/// Calls outannual on all output modules
	void outannual(Gridcell& gridcell);

	/// Calls outdaily on all output modules
	void outdaily(Gridcell& gridcell);

private:

	/// The output modules
	std::vector<OutputModule*> modules;

	/// Instruction file parameter deciding where to create output files
	std::string outputdirectory;

	/// Instruction file parameter deciding precision of coordinates in output
	/** The parameter controls the number of digits after the decimal point */
	int coordinates_precision;
};


/// Keeps track of registered output modules
/** Output modules are registered with the REGISTER_OUTPUT_MODULE
 *  macro defined below. The framework can then use the registry
 *  to create output modules.
 *
 *  The OutputModuleRegistry is a singleton, meaning there's only
 *  one instance of this class, which is retrieved with the
 *  get_instance() member function.
 */
class OutputModuleRegistry {
public:

	/// Function pointer type
	/** For each registered output module, we have a function which
	 *  creates an instance of that output module. The function is
	 *  created by the REGISTER_OUTPUT_MODULE macro below.
	 */
	typedef OutputModule* (*OutputModuleCreator)();

	/// Returns the one and only output module registry
	static OutputModuleRegistry& get_instance();

	/// Registers an output module
	/** This function shouldn't be called directly, use the REGISTER_OUTPUT_MODULE
	 *  macro below instead.
	 */
	void register_output_module(const char* name, OutputModuleCreator omc);

	/// Creates one instance of each registered module and adds it to a container
	void create_all_modules(OutputModuleContainer& container) const;

private:

	/// Private constructor to make sure we only have one instance
	OutputModuleRegistry() {}

	/// Also private to prevent copying
	OutputModuleRegistry(const OutputModuleRegistry&);

	/// The modules and their names
	std::map<std::string, OutputModuleCreator> modules;
};


/// A macro used to register output modules
/** Each output module should use this macro somewhere in their
 *  cpp file. For instance:
 *
 *  REGISTER_OUTPUT_MODULE("euroflux", EurofluxOutput)
 *
 *  where "euroflux" is the name of the module, and EurofluxOutput is the
 *  class to associate with that name. The names are currently not used,
 *  all registered output modules are always used by the framework.
 */
#define REGISTER_OUTPUT_MODULE(name, class_name) \
namespace class_name##_registration { \
\
GuessOutput::OutputModule* class_name##_creator() {	  \
	return new class_name();\
}\
\
int dummy() {\
	GuessOutput::OutputModuleRegistry::get_instance().register_output_module(name, class_name##_creator); \
	return 0;\
}\
\
int x = dummy();\
}

/// The output channel through which all output is sent
/** Currently a global for legacy reasons (in order to not break
 *  existing outannual functions). Should be a member of
 *  OutputModuleContainer, which is already responsible for
 *  creating and destroying the output_channel.
 */
extern OutputChannel* output_channel;

}

#endif // LPJ_GUESS_OUTPUT_MODULE_H
