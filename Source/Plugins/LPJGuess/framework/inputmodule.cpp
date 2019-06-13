///////////////////////////////////////////////////////////////////////////////////////
/// \file inputmodule.cpp
/// \brief Implemenation file for inputmodule.h
///
/// \author Joe Siltberg
/// $Date: 2016-12-08 18:24:04 +0100 (Thu, 08 Dec 2016) $
///
///////////////////////////////////////////////////////////////////////////////////////

#include "config.h"
#include "inputmodule.h"
#include "guess.h"

///////////////////////////////////////////////////////////////////////////////////////
/// InputModuleRegistry
///

InputModuleRegistry& InputModuleRegistry::get_instance() {
	static InputModuleRegistry instance;
	return instance;
}

void InputModuleRegistry::register_input_module(const char* name, 
                                                InputModuleCreator imc) {
	modules.insert(make_pair(std::string(name), imc));
}
            
InputModule* InputModuleRegistry::create_input_module(const char* name) const {
	std::map<std::string, InputModuleCreator>::const_iterator itr = modules.find(name);
	
	if (itr != modules.end()) {
		return (itr->second)();
	}
	else {
		fail("Couldn't find input module %s\n", name);
		return 0;
	}
}

void InputModuleRegistry::get_input_module_list(std::string& list) {

	list = "";
	std::map<std::string, InputModuleCreator>::iterator it = modules.begin();
	while (it!=modules.end()) {
		list += it->first + ";";
		it++;
	}

}

