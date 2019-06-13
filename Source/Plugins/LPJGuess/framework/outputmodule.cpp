///////////////////////////////////////////////////////////////////////////////////////
/// \file outputmodule.cpp
/// \brief Implementation of OutputModule and its container class
///
/// \author Joe Siltberg
/// $Date: 2015-12-14 16:08:55 +0100 (Mon, 14 Dec 2015) $
///
///////////////////////////////////////////////////////////////////////////////////////

#include "config.h"
#include "outputmodule.h"
#include "parameters.h"
#include "guess.h"

namespace GuessOutput {

OutputChannel* output_channel;

///////////////////////////////////////////////////////////////////////////////////////
/// OutputModule
///

void OutputModule::create_output_table(Table& table, const char* file, const ColumnDescriptors& columns) {
	 table = output_channel->create_table(TableDescriptor(file, columns));
}

void OutputModule::close_output_table(Table& table) {
	 output_channel->close_table(table);
	 table = Table();
}

///////////////////////////////////////////////////////////////////////////////////////
/// OutputModuleContainer
///

OutputModuleContainer::OutputModuleContainer()
	: coordinates_precision(2) {
	declare_parameter("outputdirectory", &outputdirectory, 300, "Directory for the output files");
	declare_parameter("coordinates_precision", &coordinates_precision, 0, 10, "Digits after decimal point in coordinates in output");
}

OutputModuleContainer::~OutputModuleContainer() {
	for (size_t i = 0; i < modules.size(); ++i) {
		delete modules[i];
	}

	delete output_channel;
}

void OutputModuleContainer::add(OutputModule* output_module) {
	modules.push_back(output_module);
}

void OutputModuleContainer::init() {
	// We MUST have an output directory
	if (outputdirectory=="") {
		fail("No output directory given in the .ins file!");
	}

	// Create the output channel
	output_channel = new FileOutputChannel(outputdirectory.c_str(),
	                                       coordinates_precision);

	for (size_t i = 0; i < modules.size(); ++i) {
		modules[i]->init();
	}
}

void OutputModuleContainer::outannual(Gridcell& gridcell) {
	for (size_t i = 0; i < modules.size(); ++i) {
		modules[i]->outannual(gridcell);
	}
}

void OutputModuleContainer::outdaily(Gridcell& gridcell) {
	for (size_t i = 0; i < modules.size(); ++i) {
		modules[i]->outdaily(gridcell);
	}
}

///////////////////////////////////////////////////////////////////////////////////////
/// OutputModuleRegistry
///

OutputModuleRegistry& OutputModuleRegistry::get_instance() {
	static OutputModuleRegistry instance;
	return instance;
}

void OutputModuleRegistry::register_output_module(const char* name,
                                                  OutputModuleCreator omc) {
	modules.insert(make_pair(std::string(name), omc));
}

void OutputModuleRegistry::create_all_modules(OutputModuleContainer& container) const {
	for (std::map<std::string, OutputModuleCreator>::const_iterator itr = modules.begin();
	     itr != modules.end(); ++itr) {
		container.add((itr->second)());
	}
}

} // namespace
