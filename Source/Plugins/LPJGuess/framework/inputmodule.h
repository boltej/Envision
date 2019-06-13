///////////////////////////////////////////////////////////////////////////////////////
/// \file inputmodule.h
/// \brief Base class for input modules
///
/// \author Joe Siltberg
/// $Date: 2016-12-08 18:24:04 +0100 (Thu, 08 Dec 2016) $
///
///////////////////////////////////////////////////////////////////////////////////////

#ifndef LPJ_GUESS_INPUT_MODULE_H
#define LPJ_GUESS_INPUT_MODULE_H

#include <map>
#include <string>

class Gridcell;

/// Base class from which any input module must inherit
/** An input module supplies LPJ-GUESS with the forcing data it needs. The
 *  InputModule base class is an abstract class, meaning it only defines
 *  the interface which subclasses must implement.
 *
 *  To create a new input module, create a new class which inherits from
 *  this one, implement all the member functions below, and register it
 *  with the REGISTER_INPUT_MODULE macro (defined and documented below).
 */
class InputModule {
public:
	virtual ~InputModule() {}

	/// Called after the instruction file has been read
	/** Initialises the input module (e.g. opening files). Typically
	 *  reads in a gridlist.
	 */
	virtual void init() = 0;

	/// Obtains coordinates and soil static parameters for the next grid cell to simulate 
	/** The function should return false if no grid cells remain to be simulated,
	 *  otherwise true. Currently the following member variables of gridcell should be
	 *  initialised: longitude, latitude and climate.instype; the following members of
	 *  member soiltype: awc[0], awc[1], perc_base, perc_exp, thermdiff_0, thermdiff_15,
	 *  thermdiff_100. The soil parameters can be set indirectly based on an lpj soil
	 *  code (Sitch et al 2000) by a call to function soilparameters in the driver
	 *  module (driver.cpp):
	 *
	 *  soilparameters(gridcell.soiltype,soilcode);
	 *
	 *  If the model is to be driven by quasi-daily values of the climate variables
	 *  derived from monthly means, this function may be the appropriate place to
	 *  perform the required interpolations. The utility functions interp_monthly_means
	 *  and interp_monthly_totals in driver.cpp may be called for this purpose.
	 */
	virtual bool getgridcell(Gridcell& gridcell) = 0;

	/// Obtains climate data (including atmospheric CO2 and insolation) for this day
	/** The function should return false if the simulation is complete for this grid cell,
	 *  otherwise true. This will normally require querying the year and day member
	 *  variables of the global class object date:
	 *
	 *  if (date.day==0 && date.year==nyear_spinup) return false;
	 *  // else
	 *  return true;
	 *
	 *  Currently the following member variables of the climate member of gridcell must be
	 *  initialised: co2, temp, prec, insol. If the model is to be driven by quasi-daily
	 *  values of the climate variables derived from monthly means, this day's values
	 *  will presumably be extracted from arrays containing the interpolated daily
	 *  values (see function getgridcell):
	 *
	 *  gridcell.climate.temp=dtemp[date.day];
	 *  gridcell.climate.prec=dprec[date.day];
	 *  gridcell.climate.insol=dsun[date.day];
	 *
	 *  Diurnal temperature range (dtr) added for calculation of leaf temperatures in 
	 *  BVOC:
	 *  gridcell.climate.dtr=ddtr[date.day]; 
	 *
	 *  If model is run in diurnal mode, which requires appropriate climate forcing data, 
	 *  additional members of the climate must be initialised: temps, insols. Both of the
	 *  variables must be of type std::vector. The length of these vectors should be equal
	 *  to value of date.subdaily which also needs to be set either in getclimate or 
	 *  getgridcell functions. date.subdaily is a number of sub-daily period in a single 
	 *  day. Irrespective of the BVOC settings, climate.dtr variable is not required in 
	 *  diurnal mode.
	 */
	virtual bool getclimate(Gridcell& gridcell) = 0;

	/// Obtains land transitions for one year
	virtual void getlandcover(Gridcell& gridcell) = 0;

	/// Obtains land management data for one day
	virtual void getmanagement(Gridcell& gridcell) = 0;
};


/// Keeps track of registered input modules
/** Input modules are registered with the REGISTER_INPUT_MODULE
 *  macro defined below. The framework can then ask the registry
 *  to create an input module by specifying its name.
 *
 *  The InputModuleRegistry is a singleton, meaning there's only
 *  one instance of this class, which is retrieved with the
 *  get_instance() member function.
 */
class InputModuleRegistry {
public:

	/// Function pointer type
	/** For each registered input module, we have a function which
	 *  creates an instance of that input module. The function is
	 *  created by the REGISTER_INPUT_MODULE macro below.
	 */
	typedef InputModule* (*InputModuleCreator)();

	/// Returns the one and only input module registry
	static InputModuleRegistry& get_instance();

	/// Registers an input module
	/** This function shouldn't be called directly, use the REGISTER_INPUT_MODULE
	 *  macro below instead.
	 */
	void register_input_module(const char* name, InputModuleCreator imc);

	/// Creates an input module given its name
	/** Used by the framework to instantiate the chosen input module. */
	InputModule* create_input_module(const char* name) const;

	/// Retrieves a semi-colon-separated list of available input modules
	/** Used by LPJ-GUESS Windows Shell  */
	void get_input_module_list(std::string& list);

private:

	/// Private constructor to make sure we only have one instance
	InputModuleRegistry() {}

	/// Also private to prevent copying
	InputModuleRegistry(const InputModuleRegistry&);

	/// The modules and their names
	std::map<std::string, InputModuleCreator> modules;
};

/// A macro used to register input modules
/** Each input module should use this macro somewhere in their
 *  cpp file. For instance:
 *
 *  REGISTER_INPUT_MODULE("cru_ncep", CRUInputModule)
 *
 *  where "cru_ncep" is the name of the module (used when chosing which
 *  input module to use), and CRUInputModule is the class to associate
 *  with that name.
 */
#define REGISTER_INPUT_MODULE(name, class_name) \
namespace class_name##_registration { \
\
InputModule* class_name##_creator() {\
	return new class_name();\
}\
\
int dummy() {\
	InputModuleRegistry::get_instance().register_input_module(name, class_name##_creator);\
	return 0;\
}\
\
int x = dummy();\
}


#endif // LPJ_GUESS_INPUT_MODULE_H
