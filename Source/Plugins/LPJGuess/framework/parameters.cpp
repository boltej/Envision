/////////////////////////////////////////////////////////////////////////////////
/// \file parameters.cpp
/// \brief Implementation of the parameters module
///
/// New instructions (PLIB keywords) may be added (this would require addition of a
/// declareitem call in function plib_declarations, and possibly some additional code in
/// function plib_callback).
///
/// \author Joe Siltberg
/// $Date: 2016-12-08 18:24:04 +0100 (Thu, 08 Dec 2016) $
///
///////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "config.h"
#include "parameters.h"
#include "guess.h"
#include "plib.h"
#include <map>

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

// Definitions of parameters defined globally in parameters.h,
// for documentation, see parameters.h

xtring title;
vegmodetype vegmode;
int npatch;
int npatch_secondarystand;
bool reduce_all_stands;
int age_limit_reduce;
double patcharea;
bool ifbgestab;
bool ifsme;
bool ifstochestab;
bool ifstochmort;
bool iffire;
bool ifdisturb;
bool ifcalcsla;
bool ifcalccton;
int estinterval;
double distinterval;
bool ifcdebt;

bool ifcentury;
bool ifnlim;
int freenyears;
double nrelocfrac;
double nfix_a;
double nfix_b;

bool ifsmoothgreffmort;
bool ifdroughtlimitedestab;
bool ifrainonwetdaysonly;

bool ifbvoc;

wateruptaketype wateruptake;

bool run_landcover;
bool run[NLANDCOVERTYPES];
bool frac_fixed[NLANDCOVERTYPES];
bool lcfrac_fixed;
bool all_fracs_const;
bool ifslowharvestpool;
bool ifintercropgrass;
bool ifcalcdynamic_phu;
int gross_land_transfer;
bool ifprimary_lc_transfer;
bool ifprimary_to_secondary_transfer;
int transfer_level;
bool ifdyn_phu_limit;
bool iftransfer_to_new_stand;
int nyear_dyn_phu;
int nyear_spinup;
bool textured_soil;
bool disturb_pasture;
bool grassforcrop;

xtring state_path;
bool restart;
bool save_state;
int state_year;
	
bool readsowingdates = false;
bool readharvestdates = false;
bool readNfert = false;
bool readNfert_st = false;
bool printseparatestands = false;
bool iftillage = false;

///////////////////////////////////////////////////////////////////////////////////////
// Implementation of the Paramlist class

Paramlist param;

void Paramlist::addparam(xtring name,xtring value) {
	Paramtype* p = find(name);
	if (p == 0) {
		p = &createobj();
	}
	p->name=name.lower();
	p->str=value;
}

void Paramlist::addparam(xtring name,double value) {
	Paramtype* p = find(name);
	if (p == 0) {
		p = &createobj();
	}
	p->name=name.lower();
	p->num=value;
}

Paramtype& Paramlist::operator[](xtring name) {
	Paramtype* param = find(name);

	if (param == 0) {
		fail("Paramlist::operator[]: parameter \"%s\" not found",(char*)name);
	}

	return *param;
}

Paramtype* Paramlist::find(xtring name) {
	name = name.lower();
	firstobj();
	while (isobj) {
		Paramtype& p=getobj();
		if (p.name==name) return &p;
		nextobj();
	}
	// nothing found
	return 0;
}

bool Paramlist::isparam(xtring name) {
	if (!find(name))
		return false;
	else
		return true;
}

///////////////////////////////////////////////////////////////////////////////////////
// ENUM DECLARATIONS OF INTEGER CONSTANTS FOR PLIB INTERFACE

enum {BLOCK_GLOBAL,BLOCK_PFT,BLOCK_PARAM,BLOCK_ST,BLOCK_MT};
enum {CB_NONE,CB_VEGMODE,CB_CHECKGLOBAL,CB_LIFEFORM,CB_LANDCOVER,CB_PHENOLOGY,CB_LEAFPHYSIOGNOMY,CB_SELECTION,	
	CB_STLANDCOVER, CB_STINTERCROP, CB_STNATURALVEG, CB_CHECKST, CB_CHECKMT,
	CB_MTPLANTINGSYSTEM, CB_MTHARVESTSYSTEM, CB_MTPFT, CB_STREESTAB, CB_MTSELECTION, CB_MTHYDROLOGY,
	CB_PLANTINGSYSTEM, CB_HARVESTSYSTEM, CB_PFT, CB_STSELECTION, CB_STHYDROLOGY, CB_MANAGEMENT1, CB_MANAGEMENT2, CB_MANAGEMENT3,
	CB_PATHWAY,CB_ROOTDIST,CB_EST,CB_CHECKPFT,CB_STRPARAM,CB_NUMPARAM,CB_WATERUPTAKE};

// File local variables
namespace {

Pft* ppft; // pointer to Pft object currently being assigned to
StandType* pst;
ManagementType* pmt;

xtring paramname;
xtring strparam;
double numparam;
bool ifhelp=false;

// 'include' parameter for currently scanned PFT
bool includepft;
bool includest;
bool includemt;

// 'include' parameter per PFT
std::map<xtring, bool> includepft_map;
// 'include' parameter per ST
std::map<xtring, bool> includest_map;
// 'include' parameter per MT
std::map<xtring, bool> includemt_map;

// Whether each PFT has had their parameters checked.
// We only check a PFT:s parameters (in plib_callback) the first time the PFT is
// parsed. If the same PFT occurs again (probably in a different file with a few
// minor modifications) we don't check again (because plib's itemparsed() function
// doesn't remember the old parsed parameters).
std::map<xtring, bool> checked_pft;
std::map<xtring, bool> checked_st;
std::map<xtring, bool> checked_mt;
}

void initsettings() {

	// Initialises global settings
	// Parameters not initialised here must be set in instruction script

	iffire=true;
	ifcalcsla=true;
	ifdisturb=false;
	ifcalcsla=false;
	ifcalccton=true;
	ifcdebt=false;
	distinterval=1.0e10;
	npatch=1;
	vegmode=COHORT;
	run_landcover = false;
	printseparatestands = false;
	save_state = false;
	restart = false;
	lcfrac_fixed = true;
	for(int lc=0; lc<NLANDCOVERTYPES; lc++)
		frac_fixed[lc] = true;
	textured_soil = true;
	disturb_pasture = false;
	grassforcrop = false;
}

void initpft(Pft& pft,xtring& setname) {

	// Initialises a PFT object
	// Parameters not initialised here must be set in instruction script

	pft.name=setname;
	pft.lifeform=NOLIFEFORM;
	pft.phenology=NOPHENOLOGY;

	// Set bioclimatic limits so that PFT can establish and survive under all
	// conditions (may be overridden by settings in instruction script)

	pft.tcmin_surv=-1000.0;
	pft.tcmin_est=-1000.0;
	pft.tcmax_est=1000.0;
	pft.twmin_est=-1000.0;
	pft.gdd5min_est=1000.0;
	pft.twminusc=0.0;

	// Set chilling parameters so that no chilling period required for budburst

	pft.k_chilla=0.0;
	pft.k_chillb=0.0;
	pft.k_chillk=0.0;
}

void initst(StandType& st,xtring& setname) {

	// Initialises a PFT object
	// Parameters not initialised here must be set in instruction script

	st.name=setname;
	st.landcover=NATURAL;
	st.intercrop=NOINTERCROP;

}

void initmt(ManagementType& mt,xtring& setname) {

	// Initialises a PFT object
	// Parameters not initialised here must be set in instruction script

	mt.name=setname;
}

///////////////////////////////////////////////////////////////////////////////////////
// Data structures for storing parameters declared from other modules
// (see the declare_parameter functions)
//

namespace {

struct xtringParam {

	xtringParam(const char* name, xtring* param, int maxlen, const char* help)
		: name(name),
		  param(param),
		  maxlen(maxlen),
		  help(help) {}

	const char* name;
	xtring* param;
	int maxlen;
	const char* help;
};

std::vector<xtringParam> xtringParams;

struct stringParam {

	stringParam(const char* name, std::string* param, int maxlen, const char* help)
		: name(name),
		  param(param),
		  maxlen(maxlen),
		  help(help) {}

	const char* name;
	std::string* param;
	int maxlen;
	const char* help;
};

std::vector<stringParam> stringParams;

struct intParam {

	intParam(const char* name, int* param, int min, int max, const char* help)
		: name(name),
		  param(param),
		  min(min),
		  max(max),
		  help(help) {}

	const char* name;
	int* param;
	int min;
	int max;
	const char* help;
};

std::vector<intParam> intParams;

struct doubleParam {

	doubleParam(const char* name, double* param, double min, double max, const char* help)
		: name(name),
		  param(param),
		  min(min),
		  max(max),
		  help(help) {}

	const char* name;
	double* param;
	double min;
	double max;
	const char* help;
};

std::vector<doubleParam> doubleParams;

struct boolParam {

	boolParam(const char* name, bool* param, const char* help)
		: name(name),
		  param(param),
		  help(help) {}

	const char* name;
	bool* param;
	const char* help;
};

std::vector<boolParam> boolParams;

} // namespace

///////////////////////////////////////////////////////////////////////////////////////
// The following declare_parameter allow other modules to declare instruction file
// parameters. The information is simply stored and then sent to plib in
// plib_declarations with calls to declareitem().

void declare_parameter(const char* name, xtring* param, int maxlen, const char* help) {
	xtringParams.push_back(xtringParam(name, param, maxlen, help));
}

void declare_parameter(const char* name, std::string* param, int maxlen, const char* help) {
	stringParams.push_back(stringParam(name, param, maxlen, help));
}

void declare_parameter(const char* name, int* param, int min, int max, const char* help) {
	intParams.push_back(intParam(name, param, min, max, help));
}

void declare_parameter(const char* name, double* param, double min, double max, const char* help) {
	doubleParams.push_back(doubleParam(name, param, min, max, help));
}

void declare_parameter(const char* name, bool* param, const char* help) {
	boolParams.push_back(boolParam(name, param, help));
}


///////////////////////////////////////////////////////////////////////////////////////
// The following code uses functionality from the PLIB library to process an
// instruction script (ins) file containing simulation settings and PFT parameters.
// Function read_instruction_file() is called by the framework to initiate parsing of the script.
// Function printhelp() is called if GUESS is run with '-help' instead of an ins file
// name as a command line argument. Functions plib_declarations, plib_callback and
// plib_receivemessage comprise part of the interface to PLIB.

void plib_declarations(int id,xtring setname) {

	switch (id) {

	case BLOCK_GLOBAL:

		declareitem("title",&title,80,CB_NONE,"Title for run");
		declareitem("nyear_spinup",&nyear_spinup,-1,10000,1,CB_NONE,"Number of simulation years to spinup for");
		declareitem("vegmode",&strparam,16,CB_VEGMODE,
			"Vegetation mode (\"INDIVIDUAL\", \"COHORT\", \"POPULATION\")");
		declareitem("ifbgestab",&ifbgestab,1,CB_NONE,
			"Whether background establishment enabled (0,1)");
		declareitem("ifsme",&ifsme,1,CB_NONE,
			"Whether spatial mass effect enabled for establishment (0,1)");
		declareitem("ifstochmort",&ifstochmort,1,CB_NONE,
			"Whether mortality stochastic (0,1)");
		declareitem("ifstochestab",&ifstochestab,1,CB_NONE,
			"Whether establishment stochastic (0,1)");
		declareitem("estinterval",&estinterval,1,10,1,CB_NONE,
			"Interval for establishment of new cohorts (years)");
		declareitem("distinterval",&distinterval,1.0,1.0e10,1,CB_NONE,
			"Generic patch-destroying disturbance interval (years)");
		declareitem("iffire",&iffire,1,CB_NONE,
			"Whether fire enabled (0,1)");
		declareitem("ifdisturb",&ifdisturb,1,CB_NONE,
			"Whether generic patch-destroying disturbance enabled (0,1)");
		declareitem("ifcalcsla",&ifcalcsla,1,CB_NONE,
			"Whether SLA calculated from leaf longevity");
		declareitem("ifcalccton",&ifcalccton,1,CB_NONE,
			"Whether leaf C:N min calculated from leaf longevity");
		declareitem("ifcdebt",&ifcdebt,1,CB_NONE,
			"Whether to allow C storage");
		declareitem("npatch",&npatch,1,1000,1,CB_NONE,
			"Number of patches simulated");
		declareitem("npatch_secondarystand",&npatch_secondarystand,1,1000,1,CB_NONE,
			"Number of patches simulated in secondary stands");
		declareitem("reduce_all_stands",&reduce_all_stands,1,CB_NONE,
			"Whether to reduce equal percentage of all stands of a stand type at land cover change");
		declareitem("age_limit_reduce",&age_limit_reduce,0,1000,1,CB_NONE,
			"Minimum age of stands to reduce at land cover change");
		declareitem("patcharea",&patcharea,1.0,1.0e4,1,CB_NONE,
			"Patch area (m2)");
		declareitem("wateruptake", &strparam, 20, CB_WATERUPTAKE,
			"Water uptake mode (\"WCONT\", \"ROOTDIST\", \"SMART\", \"SPECIESSPECIFIC\")");

		declareitem("nrelocfrac",&nrelocfrac,0.0,0.99,1,CB_NONE,
			"Fractional nitrogen relocation from shed leaves & roots");
		declareitem("nfix_a",&nfix_a,0.0,0.4,1,CB_NONE,
			"first term in nitrogen fixation eqn");
		declareitem("nfix_b",&nfix_b,-10.0,10.,1,CB_NONE,
			"second term in nitrogen fixation eqn");

		declareitem("ifcentury",&ifcentury,1,CB_NONE,
			"Whether to use CENTURY SOM dynamics (default standard LPJ)");
		declareitem("ifnlim",&ifnlim,1,CB_NONE,
			"Whether plant growth limited by available nitrogen");
		declareitem("freenyears",&freenyears,-1,1000,1,CB_NONE,
			"Number of years to spinup without nitrogen limitation");

		declareitem("ifsmoothgreffmort",&ifsmoothgreffmort,1,CB_NONE,
			"Whether to vary mort_greff smoothly with growth efficiency (0,1)");
		declareitem("ifdroughtlimitedestab",&ifdroughtlimitedestab,1,CB_NONE,
			"Whether establishment drought limited (0,1)");
		declareitem("ifrainonwetdaysonly",&ifrainonwetdaysonly,1,CB_NONE,
			"Whether it rains on wet days only (1), or a little every day (0);");

		// bvoc
		declareitem("ifbvoc",&ifbvoc,1,CB_NONE,
			"Whether or not BVOC calculations are performed (0,1)");
		declareitem("run_landcover",&run_landcover,1,CB_NONE,"Landcover version");
		declareitem("run_urban",&run[URBAN],1,CB_NONE,"Whether urban land is to be simulated");
		declareitem("run_crop",&run[CROPLAND],1,CB_NONE,"Whether crop-land is to be simulated");
		declareitem("run_pasture",&run[PASTURE],1,CB_NONE,"Whether pasture is to be simulated");
		declareitem("run_forest",&run[FOREST],1,CB_NONE,"Whether managed forest is to be simulated");
		declareitem("run_natural",&run[NATURAL],1,CB_NONE,"Whether natural vegetation is to be simulated");
		declareitem("run_peatland",&run[PEATLAND],1,CB_NONE,"Whether peatland is to be simulated");
		declareitem("run_barren",&run[BARREN],1,CB_NONE,"Whether barren land is to be simulated");
		declareitem("ifslowharvestpool",&ifslowharvestpool,1,CB_NONE,"If a slow harvested product pool is included in patchpft.");
		declareitem("ifintercropgrass",&ifintercropgrass,1,CB_NONE,"Whether intercrop growth is allowed");
		declareitem("ifcalcdynamic_phu",&ifcalcdynamic_phu,1,CB_NONE,"Whether to calculate dynamic potential heat units");
		declareitem("gross_land_transfer",&gross_land_transfer,0,3,1,CB_NONE,
			"Whether to use gross land transfer: simulate gross lcc (1); read landcover transfer matrix input file (2); read stand type transfer matrix input file (3), or not (0)");
		declareitem("ifprimary_lc_transfer",&ifprimary_lc_transfer,1,CB_NONE,
			"Whether to use primary/secondary land transition info in landcover transfer input file (1). or not (0)");
		declareitem("ifprimary_to_secondary_transfer",&ifprimary_to_secondary_transfer,1,CB_NONE,
			"Whether to use primary-to-secondary land transition info (within land cover type) in landcover transfer input file (1). or not (0)");
		declareitem("transfer_level",&transfer_level,0,3,1,CB_NONE,"Pooling level of land cover transitions; 0: one big pool; 1: land cover-level; 2: stand type-level");
		declareitem("ifdyn_phu_limit",&ifdyn_phu_limit,1,CB_NONE,"Whether to limit dynamic phu calculation to a time period");
		declareitem("iftransfer_to_new_stand",&iftransfer_to_new_stand,1,CB_NONE,"Whether to create new stands in transfer_to_new_stand()");
		declareitem("nyear_dyn_phu",&nyear_dyn_phu,0,1000,1,CB_NONE, "Number of years to calculate dynamic phu");
		declareitem("printseparatestands",&printseparatestands,1,CB_NONE,"Whether to print multiple stands within a land cover type (except cropland) separately");
		declareitem("iftillage",&iftillage,1,CB_NONE,"Whether to simulate tillage by increasing soil respiration");
		declareitem("textured_soil",&textured_soil,1,CB_NONE,"Use silt/sand fractions specific to soiltype");
		declareitem("disturb_pasture",&disturb_pasture,1,CB_NONE,"Whether fire and disturbances enabled on pastures (0,1)");
		declareitem("grassforcrop",&grassforcrop,1,CB_NONE,"grassforcrop");

		declareitem("state_path", &state_path, 300, CB_NONE, "State files directory (for restarting from, or saving state files)");
		declareitem("restart", &restart, 1, CB_NONE, "Whether to restart from state files");
		declareitem("save_state", &save_state, 1, CB_NONE, "Whether to save new state files");
		declareitem("state_year", &state_year, 1, 20000, 1, CB_NONE, "Save/restart year. Unspecified means just after spinup");

		declareitem("pft",BLOCK_PFT,CB_NONE,"Header for block defining PFT");
		declareitem("param",BLOCK_PARAM,CB_NONE,"Header for custom parameter block");
		declareitem("st",BLOCK_ST,CB_NONE,"Header for block defining StandType");
		declareitem("mt",BLOCK_MT,CB_NONE,"Header for block defining Management");

		for (size_t i = 0; i < xtringParams.size(); ++i) {
			const xtringParam& p = xtringParams[i];
			declareitem(p.name, p.param, p.maxlen, 0, p.help);
		}

		for (size_t i = 0; i < stringParams.size(); ++i) {
			const stringParam& p = stringParams[i];
			declareitem(p.name, p.param, p.maxlen, 0, p.help);
		}

		for (size_t i = 0; i < intParams.size(); ++i) {
			const intParam& p = intParams[i];
			declareitem(p.name, p.param, p.min, p.max, 1, 0, p.help);
		}

		for (size_t i = 0; i < doubleParams.size(); ++i) {
			const doubleParam& p = doubleParams[i];
			declareitem(p.name, p.param, p.min, p.max, 1, 0, p.help);
		}

		for (size_t i = 0; i < boolParams.size(); ++i) {
			const boolParam& p = boolParams[i];
			declareitem(p.name, p.param, 1, 0, p.help);
		}

		callwhendone(CB_CHECKGLOBAL);


		break;

	case BLOCK_PFT:

		if (!ifhelp) {

			ppft = 0;

			// Was this pft already created?
			for (size_t p = 0; p < pftlist.nobj; ++p) {
				if (pftlist[(unsigned int)p].name == setname) {
					ppft = &pftlist[(unsigned int)p];
				}
			}

			if (ppft == 0) {
				// Create and initialise a new Pft object and obtain a reference to it

				ppft=&pftlist.createobj();
				initpft(*ppft,setname);
				includepft_map[setname] = true;
			}
		}

		declareitem("include",&includepft,1,CB_NONE,"Include PFT in analysis");
		declareitem("lifeform",&strparam,16,CB_LIFEFORM,
			"Lifeform (\"TREE\" or \"GRASS\")");
		declareitem("landcover",&strparam,16,CB_LANDCOVER,
			"Landcovertype (\"URBAN\", \"CROP\", \"PASTURE\", \"FOREST\", \"NATURAL\", \"PEATLAND\" or \"BARREN\")");
		declareitem("selection",&strparam,16,CB_SELECTION	,"Name of pft selection");
		declareitem("phenology",&strparam,16,CB_PHENOLOGY,
			"Phenology (\"EVERGREEN\", \"SUMMERGREEN\", \"RAINGREEN\", \"CROPGREEN\" or \"ANY\")");
		declareitem("leafphysiognomy",&strparam,16,CB_LEAFPHYSIOGNOMY,
			"Leaf physiognomy (\"NEEDLELEAF\" or \"BROADLEAF\")");
		declareitem("phengdd5ramp",&ppft->phengdd5ramp,0.0,1000.0,1,CB_NONE,
			"GDD on 5 deg C base to attain full leaf cover");
		declareitem("wscal_min",&ppft->wscal_min,0.0,1.0,1,CB_NONE,
			"Water stress threshold for leaf abscission (raingreen PFTs)");
		declareitem("pathway",&strparam,16,CB_PATHWAY,
			"Biochemical pathway (\"C3\" or \"C4\")");
		declareitem("pstemp_min",&ppft->pstemp_min,-50.0,50.0,1,CB_NONE,
			"Approximate low temp limit for photosynthesis (deg C)");
		declareitem("pstemp_low",&ppft->pstemp_low,-50.0,50.0,1,CB_NONE,
			"Approx lower range of temp optimum for photosynthesis (deg C)");
		declareitem("pstemp_high",&ppft->pstemp_high,0.0,60.0,1,CB_NONE,
			"Approx higher range of temp optimum for photosynthesis (deg C)");
		declareitem("pstemp_max",&ppft->pstemp_max,0.0,60.0,1,CB_NONE,
			"Maximum temperature limit for photosynthesis (deg C)");
		declareitem("lambda_max",&ppft->lambda_max,0.1,0.99,1,CB_NONE,
			"Non-water-stressed ratio of intercellular to ambient CO2 pp");
		declareitem("rootdist",ppft->rootdist,0.0,1.0,NSOILLAYER,CB_ROOTDIST,
			"Fraction of roots in each soil layer (first value=upper layer)");
		declareitem("gmin",&ppft->gmin,0.0,1.0,1,CB_NONE,
			"Canopy conductance not assoc with photosynthesis (mm/s)");
		declareitem("emax",&ppft->emax,0.0,50.0,1,CB_NONE,
			"Maximum evapotranspiration rate (mm/day)");
		// guess2008 - increased the upper limit to possible respcoeff values (was 1.2)
		declareitem("respcoeff",&ppft->respcoeff,0.0,3,1,CB_NONE,
			"Respiration coefficient (0-1)");

		declareitem("cton_root",&ppft->cton_root,1.0,1.0e4,1,CB_NONE,
			"Reference Fine root C:N mass ratio");
		declareitem("cton_sap",&ppft->cton_sap,1.0,1.0e4,1,CB_NONE,
			"Reference Sapwood C:N mass ratio");
		declareitem("nuptoroot",&ppft->nuptoroot,0.0,1.0,1,CB_NONE,
			"Maximum nitrogen uptake per fine root");
		declareitem("km_volume",&ppft->km_volume,0.0,10.0,1,CB_NONE,
			"Michaelis-Menten kinetic parameters for nitrogen uptake");
		declareitem("fnstorage",&ppft->fnstorage,0.0,10.0,1,CB_NONE,
			"fraction of sapwood (root for herbaceous pfts) that can be used as a nitrogen storage scalar");

		declareitem("reprfrac",&ppft->reprfrac,0.0,1.0,1,CB_NONE,
			"Fraction of NPP allocated to reproduction");
		declareitem("turnover_leaf",&ppft->turnover_leaf,0.0,1.0,1,CB_NONE,
			"Leaf turnover (fraction/year)");
		declareitem("turnover_root",&ppft->turnover_root,0.0,1.0,1,CB_NONE,
			"Fine root turnover (fraction/year)");
		declareitem("turnover_sap",&ppft->turnover_sap,0.0,1.0,1,CB_NONE,
			"Sapwood turnover (fraction/year)");
		declareitem("wooddens",&ppft->wooddens,10.0,1000.0,1,CB_NONE,
			"Sapwood and heartwood density (kgC/m3)");
		declareitem("crownarea_max",&ppft->crownarea_max,1.0,1000.0,1,CB_NONE,
			"Maximum tree crown area (m2)");
		declareitem("k_allom1",&ppft->k_allom1,10.0,1000.0,1,CB_NONE,
			"Constant in allometry equations");
		// guess2008 - changed lower limit for k_allom2 to 1 from 10. This is needed
		// for the shrub allometries.
		declareitem("k_allom2",&ppft->k_allom2,1.0,1.0e4,1,CB_NONE,
			"Constant in allometry equations");
		declareitem("k_allom3",&ppft->k_allom3,0.1,1.0,1,CB_NONE,
			"Constant in allometry equations");
		declareitem("k_rp",&ppft->k_rp,1.0,2.0,1,CB_NONE,
			"Constant in allometry equations");
		declareitem("k_latosa",&ppft->k_latosa,100.0,1.0e5,1,CB_NONE,
			"Tree leaf to sapwood xs area ratio");
		declareitem("sla",&ppft->sla,1.0,1000.0,1,CB_NONE,
			"Specific leaf area (m2/kgC)");
		declareitem("cton_leaf_min",&ppft->cton_leaf_min,1.0,1.0e4,1,CB_NONE,
			"Minimum leaf C:N mass ratio");
		declareitem("ltor_max",&ppft->ltor_max,0.1,10.0,1,CB_NONE,
			"Non-water-stressed leaf:fine root mass ratio");
		declareitem("litterme",&ppft->litterme,0.0,1.0,1,CB_NONE,
			"Litter moisture flammability threshold (fraction of AWC)");
		declareitem("fireresist",&ppft->fireresist,0.0,1.0,1,CB_NONE,
			"Fire resistance (0-1)");
		declareitem("tcmin_surv",&ppft->tcmin_surv,-1000.0,50.0,1,CB_NONE,
			"Min 20-year coldest month mean temp for survival (deg C)");
		declareitem("tcmin_est",&ppft->tcmin_est,-1000.0,50.0,1,CB_NONE,
			"Min 20-year coldest month mean temp for establishment (deg C)");
		declareitem("tcmax_est",&ppft->tcmax_est,-50.0,1000.0,1,CB_NONE,
			"Max 20-year coldest month mean temp for establishment (deg C)");
		declareitem("twmin_est",&ppft->twmin_est,-1000.0,50.0,1,CB_NONE,
			"Min warmest month mean temp for establishment (deg C)");
		declareitem("twminusc",&ppft->twminusc,0,100,1,CB_NONE,
			"Stupid larch parameter");
		declareitem("gdd5min_est",&ppft->gdd5min_est,0.0,5000.0,1,CB_NONE,
			"Min GDD on 5 deg C base for establishment");
		declareitem("k_chilla",&ppft->k_chilla,0.0,5000.0,1,CB_NONE,
			"Constant in equation for budburst chilling time requirement");
		declareitem("k_chillb",&ppft->k_chillb,0.0,5000.0,1,CB_NONE,
			"Coefficient in equation for budburst chilling time requirement");
		declareitem("k_chillk",&ppft->k_chillk,0.0,1.0,1,CB_NONE,
			"Exponent in equation for budburst chilling time requirement");
		declareitem("parff_min",&ppft->parff_min,0.0,1.0e7,1,CB_NONE,
			"Min forest floor PAR for grass growth/tree estab (J/m2/day)");
		declareitem("alphar",&ppft->alphar,0.01,100.0,1,CB_NONE,
			"Shape parameter for recruitment-juv growth rate relationship");
		declareitem("est_max",&ppft->est_max,1.0e-4,1.0,1,CB_NONE,
			"Max sapling establishment rate (indiv/m2/year)");
		declareitem("kest_repr",&ppft->kest_repr,1.0,1000.0,1,CB_NONE,
			"Constant in equation for tree estab rate");
		declareitem("kest_bg",&ppft->kest_bg,0.0,1.0,1,CB_NONE,
			"Constant in equation for tree estab rate");
		declareitem("kest_pres",&ppft->kest_pres,0.0,1.0,1,CB_NONE,
			"Constant in equation for tree estab rate");
		declareitem("longevity",&ppft->longevity,0.0,3000.0,1,CB_NONE,
			"Expected longevity under lifetime non-stressed conditions (yr)");
		declareitem("greff_min",&ppft->greff_min,0.0,1.0,1,CB_NONE,
			"Threshold for growth suppression mortality (kgC/m2 leaf/yr)");
		declareitem("leaflong",&ppft->leaflong,0.1,100.0,1,CB_NONE,
			"Leaf longevity (years)");
		declareitem("intc",&ppft->intc,0.0,1.0,1,CB_NONE,"Interception coefficient");

		// guess2008 - DLE
		declareitem("drought_tolerance",&ppft->drought_tolerance,0.0,1.0,1,CB_NONE,
			"Drought tolerance level (0 = very -> 1 = not at all) (unitless)");

		// bvoc
		declareitem("ga",&ppft->ga,0.0,1.0,1,CB_NONE,
			"aerodynamic conductance (m/s)");
		declareitem("eps_iso",&ppft->eps_iso,0.,100.,1,CB_NONE,
			"isoprene emission capacity (ug C g-1 h-1)");
		declareitem("seas_iso",&ppft->seas_iso,1,CB_NONE,
			"whether (1) or not (0) isoprene emissions show seasonality");
		declareitem("eps_mon",&ppft->eps_mon,0.,100.,1,CB_NONE,
			"monoterpene emission capacity (ug C g-1 h-1)");
		declareitem("storfrac_mon",&ppft->storfrac_mon,0.,1.,1,CB_NONE,
			"fraction of monoterpene production that goes into storage pool (-)");

		declareitem("harv_eff",&ppft->harv_eff,0.0,1.0,1,CB_NONE,"Harvest efficiency");
		declareitem("harvest_slow_frac",&ppft->harvest_slow_frac,0.0,1.0,1,CB_NONE,
			"Fraction of harvested products that goes into carbon depository for long-lived products like wood");
		declareitem("turnover_harv_prod",&ppft->turnover_harv_prod,0.0,1.0,1,CB_NONE,"Harvested products turnover (fraction/year)");
		declareitem("res_outtake",&ppft->res_outtake,0.0,1.0,1,CB_NONE,"Fraction of residue outtake at harvest");

		declareitem("sdatenh",&ppft->sdatenh,1,365,1,CB_NONE,"sowing day northern hemisphere");
		declareitem("sdatesh",&ppft->sdatesh,1,365,1,CB_NONE,"sowing day southern hemisphere");
		declareitem("lgp_def",&ppft->lgp_def,1,365,1,CB_NONE,"default lgp");
		declareitem("sd_adjust",&ppft->sd_adjust,1,CB_NONE,"whether sowing date adjusting equation is used");
		declareitem("sd_adjust_par1",&ppft->sd_adjust_par1,1,365,1,CB_NONE,"parameter 1 in sowing date adjusting equation");
		declareitem("sd_adjust_par2",&ppft->sd_adjust_par2,1,365,1,CB_NONE,"parameter 2 in sowing date adjusting equation");
		declareitem("sd_adjust_par3",&ppft->sd_adjust_par3,1,365,1,CB_NONE,"parameter 3 in sowing date adjusting equation");
		declareitem("hlimitdatenh",&ppft->hlimitdatenh,1,365,1,CB_NONE,"last harvest date in the northern hemisphere");
		declareitem("hlimitdatesh",&ppft->hlimitdatesh,1,365,1,CB_NONE,"last harvest date in the southern hemisphere");
		declareitem("tb",&ppft->tb,0.0,25.0,1,CB_NONE,"base temperature for heat unit calculation");
		declareitem("trg",&ppft->trg,0.0,20.0,1,CB_NONE,"upper temperature limit for vernalisation effect");
		declareitem("pvd",&ppft->pvd,0,100,1,CB_NONE,"number of vernalising days required");
		declareitem("vern_lag",&ppft->vern_lag,0,100,1,CB_NONE,"lag in days after sowing before vernalization starts");
		declareitem("isintercropgrass",&ppft->isintercropgrass,1,CB_NONE,"Whether this pft is allowed to grow in intercrop period");
		declareitem("psens",&ppft->psens,0.0,1.0,1,CB_NONE,"sensitivity to the photoperiod effect [0-1]");
		declareitem("pb",&ppft->pb,0.0,24.0,1,CB_NONE,"basal photoperiod (h)");
		declareitem("ps",&ppft->ps,0.0,24.0,1,CB_NONE,"saturating photoperiod (h)");
		declareitem("phu",&ppft->phu,0.0,4000.0,1,CB_NONE,"default potential heat units for crop maturity");
		declareitem("phu_calc_quad",&ppft->phu_calc_quad,1,CB_NONE,"whether linear equation used for calculating potential heat units (Bondeau method)");
		declareitem("phu_calc_lin",&ppft->phu_calc_lin,1,CB_NONE,"minimum potential heat units required for crop maturity (Bondeau method) (degree-days)");
		declareitem("phu_min",&ppft->phu_min,0.0,4000.0,1,CB_NONE,"minimum potential heat units for crop maturity (Bondeau method)");
		declareitem("phu_max",&ppft->phu_max,0.0,4000.0,1,CB_NONE,"maximum potential heat units for crop maturity (Bondeau method)");
		declareitem("phu_red_spring_sow",&ppft->phu_red_spring_sow,0.0,1.0,1,CB_NONE,"reduction factor of potential heat units in spring crops (Bondeau method)");
		declareitem("phu_interc",&ppft->phu_interc,0.0,4000.0,1,CB_NONE,"intercept for the linear phu equation (Bondeau method)");
		declareitem("ndays_ramp_phu",&ppft->ndays_ramp_phu,0.0,365.0,1,CB_NONE,"number of days of phu decrease in the linear phu equation (Bondeau method)");
		declareitem("fphusen",&ppft->fphusen,0.0,1.0,1,CB_NONE,"growing season fract. when lai starts decreasing");
		declareitem("shapesenescencenorm",&ppft->shapesenescencenorm,1,CB_NONE,"Type of senescence curve");
		declareitem("flaimaxharvest",&ppft->flaimaxharvest,0.0,1.0,1,CB_NONE,"Fraction of maximum lai when harvest prescribed");
		declareitem("aboveground_ho",&ppft->aboveground_ho,1,CB_NONE,"Whether aboveground structures are harvested");
		declareitem("harv_eff_ic",&ppft->harv_eff_ic,0.0,1.0,1,CB_NONE,"Harvest efficiency of covercrop grass");
		declareitem("ifsdautumn",&ppft->ifsdautumn,1,CB_NONE,"Whether sowing date in autumn is to be calculated");
		declareitem("tempautumn",&ppft->tempautumn,0.0,25.0,1,CB_NONE,"Upper temperature limit for winter sowing");
		declareitem("tempspring",&ppft->tempspring,0.0,25.0,1,CB_NONE,"Lower temperature limt for spring sowing");
		declareitem("maxtemp_sowing",&ppft->maxtemp_sowing,0.0,60.0,1,CB_NONE,"Upper minimum temperature limit for crop sowing");	
		declareitem("hiopt",&ppft->hiopt,0.0,2.0,1,CB_NONE,"Optimal harvest index");
		declareitem("himin",&ppft->himin,0.0,2.0,1,CB_NONE,"Minimal harvest index");
		declareitem("frootstart",&ppft->frootstart,0.0,1.0,1,CB_NONE,"Initial root mass fraction of total plant");
		declareitem("frootend",&ppft->frootend,0.0,1.0,1,CB_NONE,"Root mass fraction of total plant at harvest");
		declareitem("laimax",&ppft->laimax,0.0,10.0,1,CB_NONE,"Maximum lai (crop grass only)");
		declareitem("forceautumnsowing",&ppft->forceautumnsowing,0,2,1,CB_NONE,"Whether autumn sowing is forced independent of climate");

		declareitem("fertdates",ppft->fertdates,0,365,2,CB_NONE,
			"Fertilisation dates, relative to sowing");
		declareitem("fertrate",ppft->fertrate,0.0,1.0,2,CB_NONE,
			"Fraction of total fertilisation at fertilisation event");
		declareitem("N_appfert",&ppft->N_appfert,0.0,300.0,1,CB_NONE,
			"Fertilisation rate");

		declareitem("T_vn_min",&ppft->T_vn_min,-1000.0,1000.0,1,CB_NONE,
			"Min temperature for vernalization");
		declareitem("T_vn_opt",&ppft->T_vn_opt,-1000.0,1000.0,1,CB_NONE,
			"Opt temperature for vernalization");
		declareitem("T_vn_max",&ppft->T_vn_max,-1000.0,1000.0,1,CB_NONE,
			"Max temperature for vernalization");
		declareitem("T_veg_min",&ppft->T_veg_min,-1000.0,1000.0,1,CB_NONE,
			"Min temperature for the vegetative phase");
		declareitem("T_veg_opt",&ppft->T_veg_opt,-1000.0,1000.0,1,CB_NONE,
			"Opt temperature for the vegetative phase");
		declareitem("T_veg_max",&ppft->T_veg_max,-1000.0,1000.0,1,CB_NONE,
			"Max temperature for the vegetative phase");
		declareitem("T_rep_min",&ppft->T_rep_min,-1000.0,1000.0,1,CB_NONE,
			"Min temperature for the reproductive phase");
		declareitem("T_rep_opt",&ppft->T_rep_opt,-1000.0,1000.0,1,CB_NONE,
			"Opt temperature for the reproductive phase");
		declareitem("T_rep_max",&ppft->T_rep_max,-1000.0,1000.0,1,CB_NONE,
			"Max temperature for the reproductive phase");
		declareitem("photo",ppft->photo,-1000.0,1000.0,3,CB_NONE,
			"Parameters for photoperiod");
		declareitem("dev_rate_veg",&ppft->dev_rate_veg,-1000.0,1000.0,1,CB_NONE,
			"Maximal vegetative develoment rate");
		declareitem("dev_rate_rep",&ppft->dev_rate_rep,-1000.0,1000.0,1,CB_NONE,
			"Maximal reproductive develoment rate");
		declareitem("a1",&ppft->a1,-1000.0,1000.0,1,CB_NONE,
			"a1 parameter for allocation with N stress");
		declareitem("b1",&ppft->b1,-1000.0,1000.0,1,CB_NONE,
			"b1 parameter for allocation with N stress");
		declareitem("c1",&ppft->c1,-1000.0,1000.0,1,CB_NONE,
			"c1 parameter for allocation with N stress");
		declareitem("d1",&ppft->d1,-1000.0,1000.0,1,CB_NONE,
			"d1 parameter for allocation with N stress");
		declareitem("a2",&ppft->a2,-1000.0,1000.0,1,CB_NONE,
			"a2 parameter for allocation with N stress");
		declareitem("b2",&ppft->b2,-1000.0,1000.0,1,CB_NONE,
			"b2 parameter for allocation with N stress");
		declareitem("c2",&ppft->c2,-1000.0,1000.0,1,CB_NONE,
			"c2 parameter for allocation with N stress");
		declareitem("d2",&ppft->d2,-1000.0,1000.0,1,CB_NONE,
			"d2 parameter for allocation with N stress");
		declareitem("a3",&ppft->a3,-1000.0,1000.0,1,CB_NONE,
			"a3 parameter for allocation with N stress");
		declareitem("b3",&ppft->b3,-1000.0,1000.0,1,CB_NONE,
			"b3 parameter for allocation with N stress");
		declareitem("c3",&ppft->c3,-1000.0,1000.0,1,CB_NONE,
			"c3 parameter for allocation with N stress");
		declareitem("d3",&ppft->d3,-1000.0,1000.0,1,CB_NONE,
			"d3 parameter for allocation with N stress");

		callwhendone(CB_CHECKPFT);

		break;

	case BLOCK_MT:

		if (!ifhelp) {

			pmt = 0;

			// Was this mt already created?
			for (size_t p = 0; p < mtlist.nobj; ++p) {
				if (mtlist[(unsigned int)p].name == setname) {
					pmt = &mtlist[(unsigned int)p];
				}
			}

			if (pmt == 0) {
				// Create and initialise a new st object and obtain a reference to it
			
				pmt=&mtlist.createobj();
				initmt(*pmt,setname);
				includemt_map[setname] = true;
			}
		}

		declareitem("mtinclude",&includemt,1,CB_NONE,"Include ManagementType in analysis");
		declareitem("planting_system",&strparam,32,CB_MTPLANTINGSYSTEM,"Planting system");
		declareitem("harvest_system",&strparam,32,CB_MTHARVESTSYSTEM,"Harvest system");
		declareitem("pft",&strparam,16,CB_MTPFT,"PFT name");
		declareitem("selection",&strparam,200,CB_MTSELECTION	,"String of pft names");
		declareitem("rottime",&pmt->nyears,0.0,100.0,1,CB_NONE,"Rotation time (years)");
		declareitem("hydrology",&strparam,16,CB_MTHYDROLOGY, "Hydrology of crop (\"RAINFED\" or \"IRRIGATED\")");
//		declareitem("irrigation",&pmt->firr,0.0,1.0,1,CB_NONE,"Irrigation of crop");
		declareitem("sdate",&pmt->sdate,0,364,1,CB_NONE,"Sowing date of crop");
		declareitem("hdate",&pmt->hdate,0,364,1,CB_NONE,"Harvest date of crop");
		declareitem("nfert",&pmt->nfert,0.0,1000.0,1,CB_NONE,"Fertilization application of crop");
		declareitem("fallow",&pmt->fallow,1,CB_NONE,"Fallow in place of crop");
		declareitem("multicrop",&pmt->multicrop,1,CB_NONE,"Whether to grow several crops in a year");

		callwhendone(CB_CHECKMT);

		break;

	case BLOCK_ST:

		if (!ifhelp) {

			pst = 0;

			// Was this st already created?
			for (size_t p = 0; p < stlist.nobj; ++p) {
				if (stlist[(unsigned int)p].name == setname) {
					pst = &stlist[(unsigned int)p];
				}
			}

			if (pst == 0) {
				// Create and initialise a new st object and obtain a reference to it

				pst=&stlist.createobj();
				initst(*pst,setname);
				includest_map[setname] = true;
			}
		}

		declareitem("stinclude",&includest,1,CB_NONE,"Include StandType in analysis");
		declareitem("landcover",&strparam,16,CB_STLANDCOVER,
			"Landcovertype (\"URBAN\", \"CROP\", \"PASTURE\", \"FOREST\", \"NATURAL\", \"PEATLAND\" or \"BARREN\")");
		declareitem("intercrop",&strparam,16,CB_STINTERCROP,
			"Cover crop (\"NOINTERCROP\" or \"NATURALGRASS\")");
		declareitem("naturalveg",&strparam,16,CB_STNATURALVEG,
			"Natural pfts (\"NONE\", \"GRASSONLY\" or \"ALL\")");
		declareitem("reestab",&strparam,16,CB_STREESTAB,
			"Re-establishment (\"NONE\", \"RESTRICTED\" or \"ALL\")");

		declareitem("firstrotyear",&pst->rotation.firstrotyear,0,3000,1,CB_NONE,"First calender year of rotation");
		declareitem("restrictpfts",&pst->restrictpfts,1,CB_NONE,"Whether to only allow pft:s specified in stand type");
		declareitem("firstmanageyear",&pst->firstmanageyear,0,3000,1,CB_NONE,"First calender year of management");

		for(int i = 0; i < NROTATIONPERIODS_MAX; ++i) {
			if(i == 0) {
				declareitem("management1",&strparam,16,CB_MANAGEMENT1,"");
				declareitem("planting_system",&strparam,32,CB_PLANTINGSYSTEM,"Planting system of management 1");
				declareitem("harvest_system",&strparam,32,CB_HARVESTSYSTEM,"Harvest system of management 1");
				declareitem("pft",&strparam,16,CB_PFT,"PFT name");
				declareitem("selection",&strparam,200,CB_STSELECTION,"String of pft names");
				declareitem("rottime",&pst->management.nyears,0.0,100.0,1,CB_NONE,"Rotation time (years)");
				declareitem("hydrology",&strparam,16,CB_STHYDROLOGY, "Hydrology of crop 1 (\"RAINFED\" or \"IRRIGATED\")");
//				declareitem("irrigation",&pst->management.firr,0.0,1.0,1,CB_NONE,"Irrigation of crop 1");
				declareitem("sdate",&pst->management.sdate,0,364,1,CB_NONE,"Sowing date of crop 1");
				declareitem("hdate",&pst->management.hdate,0,364,1,CB_NONE,"Harvest date of crop 1");
				declareitem("nfert",&pst->management.nfert,0.0,1000.0,1,CB_NONE,"Fertilization application of crop 1");
				declareitem("fallow",&pst->management.fallow,1,CB_NONE,"Fallow in place of crop 1");
				declareitem("multicrop",&pst->management.multicrop,1,CB_NONE,"Whether to grow several crops in a year in management 1");
			}
			else if(i == 1) {
				declareitem("management2",&strparam,16,CB_MANAGEMENT2,"");
			}
			else if(i == 2) {
				declareitem("management3",&strparam,16,CB_MANAGEMENT3,"");
			}
		}
		callwhendone(CB_CHECKST);

		break;

	case BLOCK_PARAM:

		paramname=setname;
		declareitem("str",&strparam,300,CB_STRPARAM,
			"String value for custom parameter");
		declareitem("num",&numparam,-1.0e38,1.0e38,1,CB_NUMPARAM,
			"Numerical value for custom parameter");

		break;
	}
}

void badins(xtring missing) {

	xtring message=(xtring)"Missing mandatory setting: "+missing;
	sendmessage("Error",message);
	plibabort();
}

void plib_callback(int callback) {

	xtring message;
	int i;
	double numval;

	switch (callback) {

	case CB_VEGMODE:
		if (strparam.upper()=="INDIVIDUAL") vegmode=INDIVIDUAL;
		else if (strparam.upper()=="COHORT") vegmode=COHORT;
		else if (strparam.upper()=="POPULATION") vegmode=POPULATION;
		else {
			sendmessage("Error",
				"Unknown vegetation mode (valid types: \"INDIVIDUAL\",\"COHORT\", \"POPULATION\")");
			plibabort();
		}
		break;
	case CB_WATERUPTAKE:
		if (strparam.upper() == "WCONT") wateruptake = WR_WCONT;
		else if (strparam.upper() == "ROOTDIST") wateruptake = WR_ROOTDIST;
		else if (strparam.upper() == "SMART") wateruptake = WR_SMART;
		else if (strparam.upper() == "SPECIESSPECIFIC") wateruptake = WR_SPECIESSPECIFIC;
		else {
			sendmessage("Error",
				"Unknown water uptake mode (valid types: \"WCONT\", \"ROOTDIST\", \"SMART\", \"SPECIESSPECIFIC\")");
		}
		break;
	case CB_LIFEFORM:
		if (strparam.upper()=="TREE") ppft->lifeform=TREE;
		else if (strparam.upper()=="GRASS") ppft->lifeform=GRASS;
		else {
			sendmessage("Error",
				"Unknown lifeform type (valid types: \"TREE\", \"GRASS\")");
			plibabort();
		}
		break;
	case CB_LANDCOVER:
		if (strparam.upper()=="NATURAL") ppft->landcover=NATURAL;
		else if (strparam.upper()=="URBAN") ppft->landcover=URBAN;
		else if (strparam.upper()=="CROPLAND") ppft->landcover=CROPLAND;
		else if (strparam.upper()=="PASTURE") ppft->landcover=PASTURE;
		else if (strparam.upper()=="FOREST") ppft->landcover=FOREST;
		else if (strparam.upper()=="PEATLAND") ppft->landcover=PEATLAND;
		else if (strparam.upper()=="BARREN") ppft->landcover=BARREN;
		else {
			sendmessage("Error",
				"Unknown landcover type (valid types: \"URBAN\", \"CROPLAND\", \"PASTURE\", \"FOREST\", \"NATURAL\", \"PEATLAND\" or \"BARREN\")");
			plibabort();
		}
		break;
	case CB_SELECTION:
		ppft->selection = strparam;
		break;
	case CB_STLANDCOVER:
		if (strparam.upper()=="NATURAL") pst->landcover=NATURAL;
		else if (strparam.upper()=="URBAN") pst->landcover=URBAN;
		else if (strparam.upper()=="CROPLAND") pst->landcover=CROPLAND;
		else if (strparam.upper()=="PASTURE") pst->landcover=PASTURE;
		else if (strparam.upper()=="FOREST") pst->landcover=FOREST;
		else if (strparam.upper()=="PEATLAND") pst->landcover=PEATLAND;
		else if (strparam.upper()=="BARREN") pst->landcover=BARREN;
		else {
			sendmessage("Error",
				"Unknown landcover type (valid types: \"URBAN\", \"CROPLAND\", \"PASTURE\", \"FOREST\", \"NATURAL\", \"PEATLAND\" or \"BARREN\")");
			plibabort();
		}
		break;
	case CB_STINTERCROP:
		if (strparam.upper()=="NOINTERCROP") pst->intercrop=NOINTERCROP;
		else if (strparam.upper()=="NATURALGRASS") pst->intercrop=NATURALGRASS;
		else {
			sendmessage("Error",
				"Unknown intercrop type (valid types: \"NOINTERCROP\", \"NATURALGRASS\")");
			plibabort();
		}
		break;
	case CB_STNATURALVEG:
		pst->naturalveg = strparam.upper();
		break;
	case CB_STREESTAB:
		pst->reestab = strparam.upper();
		break;
	case CB_MTPLANTINGSYSTEM:
		pmt->planting_system = strparam.upper();
		break;
	case CB_MTHARVESTSYSTEM:
		pmt->harvest_system = strparam.upper();
		break;
	case CB_MTPFT:
		pmt->pftname = strparam;
		break;
	case CB_MTSELECTION:
		pmt->selection = strparam;
		break;
	case CB_MTHYDROLOGY:
		if (strparam.upper()=="RAINFED") pmt->hydrology = RAINFED;
		else if (strparam.upper()=="IRRIGATED") pmt->hydrology = IRRIGATED;
		else 
		{
			sendmessage("Error",
				"Unknown hydrology type (valid types: \"RAINFED\", \"IRRIGATED\")");
			plibabort();
		}
		break;
	case CB_MANAGEMENT1:
		pst->mtnames[0] = strparam;
		break;
	case CB_MANAGEMENT2:
		pst->mtnames[1] = strparam;
		break;
	case CB_MANAGEMENT3:
		pst->mtnames[2] = strparam;
		break;
	case CB_PLANTINGSYSTEM:
		pst->management.planting_system = strparam.upper();
		break;
	case CB_HARVESTSYSTEM:
		pst->management.harvest_system = strparam.upper();
		break;
	case CB_PFT:
		pst->management.pftname = strparam;
		break;
	case CB_STSELECTION:
		pst->management.selection = strparam;
		break;
	case CB_STHYDROLOGY:
		if (strparam.upper()=="RAINFED") pst->management.hydrology = RAINFED;
		else if (strparam.upper()=="IRRIGATED") pst->management.hydrology = IRRIGATED;
		else 
		{
			sendmessage("Error",
				"Unknown hydrology type (valid types: \"RAINFED\", \"IRRIGATED\")");
			plibabort();
		}
		break;
	case CB_PHENOLOGY:
		if (strparam.upper()=="SUMMERGREEN") ppft->phenology=SUMMERGREEN;
		else if (strparam.upper()=="RAINGREEN") ppft->phenology=RAINGREEN;
		else if (strparam.upper()=="EVERGREEN") ppft->phenology=EVERGREEN;
		else if (strparam.upper()=="CROPGREEN") ppft->phenology=CROPGREEN;
		else if (strparam.upper()=="ANY") ppft->phenology=ANY;
		else {
			sendmessage("Error",
				"Unknown phenology type\n  (valid types: \"EVERGREEN\", \"SUMMERGREEN\", \"RAINGREEN\", \"CROPGREEN\" or \"ANY\")");
			plibabort();
		}
		break;
	case CB_LEAFPHYSIOGNOMY:
		if (strparam.upper()=="NEEDLELEAF") ppft->leafphysiognomy=NEEDLELEAF;
		else if (strparam.upper()=="BROADLEAF") ppft->leafphysiognomy=BROADLEAF;
		else {
			sendmessage("Error",
				"Unknown leaf physiognomy (valid types: \"NEEDLELEAF\", \"BROADLEAF\")");
			plibabort();
		}
		break;
	case CB_PATHWAY:
		if (strparam.upper()=="C3") ppft->pathway=C3;
		else if (strparam.upper()=="C4") ppft->pathway=C4;
		else {
			sendmessage("Error",
				"Unknown pathway type\n  (valid types: \"C3\" or \"C4\")");
			plibabort();
		}
		break;
	case CB_ROOTDIST:
		numval=0.0;
		for (i=0;i<NSOILLAYER;i++) numval+=ppft->rootdist[i];
		if (numval<0.99 || numval>1.01) {
			sendmessage("Error","Specified root fractions do not sum to 1.0");
			plibabort();
		}
		ppft->rootdist[NSOILLAYER-1]+=1.0-numval;
		break;
	case CB_STRPARAM:
		param.addparam(paramname,strparam);
		break;
	case CB_NUMPARAM:
		param.addparam(paramname,numparam);
		break;
	case CB_CHECKGLOBAL:
		if (!itemparsed("title")) badins("title");
		if (!itemparsed("nyear_spinup")) badins("nyear_spinup");
		if (!itemparsed("vegmode")) badins("vegmode");
		if (!itemparsed("iffire")) badins("iffire");
		if (!itemparsed("ifcalcsla")) badins("ifcalcsla");
		if (!itemparsed("ifcalccton")) badins("ifcalccton");
		if (!itemparsed("ifcdebt")) badins("ifcdebt");
		if (!itemparsed("wateruptake")) badins("wateruptake");

		if (!itemparsed("nrelocfrac")) badins("nrelocfrac");
		if (!itemparsed("nfix_a")) badins("nfix_a");
		if (!itemparsed("nfix_b")) badins("nfix_b");

		if (!itemparsed("ifcentury")) badins("ifcentury");
		if (!itemparsed("ifnlim")) badins("ifnlim");
		if (!itemparsed("freenyears")) badins("freenyears");

		if (nyear_spinup < freenyears) {
			sendmessage("Error", "freenyears must be smaller than nyear_spinup");
			plibabort();
		}

		if (!itemparsed("outputdirectory")) badins("outputdirectory");
		if (!itemparsed("ifsmoothgreffmort")) badins("ifsmoothgreffmort");
		if (!itemparsed("ifdroughtlimitedestab")) badins("ifdroughtlimitedestab");
		if (!itemparsed("ifrainonwetdaysonly")) badins("ifrainonwetdaysonly");
		// bvoc
		if (!itemparsed("ifbvoc")) badins("ifbvoc");

		if (!itemparsed("run_landcover")) badins("run_landcover");
		if (run_landcover) {
			if (!itemparsed("npatch_secondarystand")) badins("npatch_secondarystand");
			if (!itemparsed("reduce_all_stands")) badins("reduce_all_stands");
			if (!itemparsed("age_limit_reduce")) badins("age_limit_reduce");
			if (!itemparsed("minimizecftlist")) badins("minimizecftlist");
			if (!itemparsed("run_natural")) badins("run_natural");
			if (!itemparsed("run_crop")) badins("run_crop");
			if (!itemparsed("run_forest")) badins("run_forest");
			if (!itemparsed("run_urban")) badins("run_urban");
			if (!itemparsed("run_pasture")) badins("run_pasture");
			if (!itemparsed("run_barren")) badins("run_barren");
			if (!itemparsed("ifslowharvestpool")) badins("ifslowharvestpool");
			if (!itemparsed("ifintercropgrass")) badins("ifintercropgrass");
			if (!itemparsed("ifcalcdynamic_phu")) badins("ifcalcdynamic_phu");
			if (!itemparsed("gross_land_transfer")) badins("gross_land_transfer");
			if (!itemparsed("ifprimary_lc_transfer")) badins("ifprimary_lc_transfer");
			if (!itemparsed("ifprimary_to_secondary_transfer")) badins("ifprimary_to_secondary_transfer");
			if (!itemparsed("transfer_level")) badins("transfer_level");
			if (!itemparsed("ifdyn_phu_limit")) badins("ifdyn_phu_limit");
			if (!itemparsed("iftransfer_to_new_stand")) badins("iftransfer_to_new_stand");
			if (!itemparsed("nyear_dyn_phu")) badins("nyear_dyn_phu");
			if (!itemparsed("printseparatestands")) badins("printseparatestands");
			if (!itemparsed("iftillage")) badins("iftillage");
		}

		if (!itemparsed("pft")) badins("pft");
		if (vegmode==COHORT || vegmode==INDIVIDUAL) {
			if (!itemparsed("ifbgestab")) badins("ifbgestab");
			if (!itemparsed("ifsme")) badins("ifsme");
			if (!itemparsed("ifstochmort")) badins("ifstochmort");
			if (!itemparsed("ifstochestab")) badins("ifstochestab");
			if (itemparsed("ifdisturb") && !itemparsed("distinterval"))
				badins("distinterval");
			if (!itemparsed("npatch")) badins("npatch");
			if (!itemparsed("patcharea")) badins("patcharea");
			if (!itemparsed("estinterval")) badins("estinterval");
		}
		else if (vegmode==POPULATION && npatch!=1) {
			sendmessage("Information",
				"Value specified for npatch ignored in population mode");
			npatch=1;
		}

		if (save_state && restart) {
			sendmessage("Error",
			            "Can't save state and restart at the same time");
			plibabort();
		}

		if (!itemparsed("state_year")) {
			state_year = nyear_spinup;
		}

		if (state_path == "" && (save_state || restart)) {
			badins("state_path");
		}

		if (grassforcrop) {
			run[CROPLAND] = 0;
			run[PASTURE] = 1;
		}

		if (!run_landcover)
			printseparatestands = false;

		//	delete unused management types from mtlist

		mtlist.firstobj();
		while (mtlist.isobj) {
			ManagementType& mt = mtlist.getobj();
			bool include = includemt_map[mt.name];

			if (!include) {
				// Remove this management type from list
				mtlist.killobj();
			}
			else {
				mtlist.nextobj();
			}
		}

		//	delete unused stand types from stlist

		stlist.firstobj();
		while (stlist.isobj) {
			StandType& st = stlist.getobj();
			bool include = includest_map[st.name];

			if (st.landcover != NATURAL) {
				if (!run_landcover || !run[st.landcover])
					include = false;
			}
			else if (run_landcover && !run[NATURAL]) {
				if (st.landcover == NATURAL)
					include = false;
			}

			if (!include) {
				// Remove this stand type from list
				stlist.killobj();
			}
			else {
				stlist.nextobj();
			}
		}

		//	delete unused pft:s from pftlist

		// first check if natural pft:s are included in other land cover stand types
		bool include_natural_pfts;
		bool include_natural_grass_pfts;

		if(run_landcover && !run[NATURAL]) {

			include_natural_pfts = false;
			include_natural_grass_pfts = false;

			stlist.firstobj();
			while(stlist.isobj) {
				StandType& st = stlist.getobj();

				if(st.naturalveg == "ALL") {
					include_natural_pfts = true;
					include_natural_grass_pfts = true;
					break;
				}
				if(st.naturalveg == "GRASSONLY")
					include_natural_grass_pfts = true;
				stlist.nextobj();
			}
		}
		else {
			include_natural_pfts = true;
			include_natural_grass_pfts = true;
		}

		pftlist.firstobj();
		while (pftlist.isobj) {
			Pft& pft = pftlist.getobj();
			bool include = includepft_map[pft.name];

			if(pft.landcover == NATURAL) {
				if(!include_natural_grass_pfts && pft.lifeform == GRASS)
					include = false;
				else if(!include_natural_pfts && pft.lifeform != GRASS)
					include = false;
			}
			else {
				if (!run_landcover || !run[pft.landcover])
					include = false;
			}

			if (!include) {
				// Remove this PFT from list
				pftlist.killobj();
			}
			else {
				pftlist.nextobj();
			}
		}

		/// Set ncrops and verify that crop rotations have defined pftnames
		stlist.firstobj();
		while (stlist.isobj) {
			StandType& st = stlist.getobj();

			// management types in ins-file overrides management settings in stand type
			if(st.mtnames[0] != "") {

				for(int rot=0; rot<NROTATIONPERIODS_MAX; rot++) {

					if(st.mtnames[rot] != "") {
						st.rotation.ncrops++;
						if(rot == 0) {
							int mtid = mtlist.getmtid(st.mtnames[rot]);
							if(mtid > -1) {
								ManagementType& mt = mtlist[mtid];
								// Copy management from mtlist to stand type management, used only if ncrops=1
								st.management = mt;
							}
						}
					}
					else {
						break;
					}
				}
			}
			if(!st.rotation.ncrops) {
				// Check if there are management settings in the stand type definition
				if(st.management.is_managed())
					st.rotation.ncrops = 1;
			}
			if(st.landcover == CROPLAND && 
				(st.rotation.ncrops == 0 ||
				st.rotation.ncrops >= 1 && st.get_management(0).pftname == "" && !st.get_management(0).fallow ||
				st.rotation.ncrops >= 2 && st.get_management(1).pftname == "" && !st.get_management(1).fallow ||
				st.rotation.ncrops >= 3 && st.get_management(2).pftname == "" && !st.get_management(2).fallow))
				fail("Check stand type rotation parameter setting, pftname missing\n");

			stlist.nextobj();
		}

		// Remove crop st:s with pft:s that are not found in the pftlist or with mt:s that are not in the mtlist
		dprintf("\n");
		stlist.firstobj();
		while (stlist.isobj) {
			StandType& st = stlist.getobj();

			bool include = true;

			if(st.landcover == CROPLAND) {	// Should check this for other land covers too, forest monocultures can have a pftname

				for(int i=0; i<st.rotation.ncrops; i++) {

					if(st.mtnames[i] != "" && mtlist.getmtid(st.mtnames[i]) < 0) {
						include = false;
						dprintf("Stand type %s not used; mt %s not in mtlist !\n", (char*)st.name, (char*)st.mtnames[i]);
					}
					if(st.get_management(i).pftname != "" && pftlist.getpftid(st.get_management(i).pftname) < 0) {
						include = false;
						dprintf("Stand type %s not used; pft %s not in pftlist !\n", (char*)st.name, (char*)st.get_management(i).pftname);
					}
				}
			}

			if (!include) {
				// Remove this stand type from list
				stlist.killobj();
			}
			else {
				stlist.nextobj();
			}
		}
		dprintf("\n");

		// Set ids and npft variable after removing unused pfts	; NB: minimizecftlist may remove more pfts
		npft = 0;
		pftlist.firstobj();
		while (pftlist.isobj) {
			Pft& pft = pftlist.getobj();
			pft.id = npft++;
			pftlist.nextobj();
		}

		// Set ids and nmt variable after removing unused mts
		nmt = 0;
		mtlist.firstobj();
		while (mtlist.isobj) {
			ManagementType& mt = mtlist.getobj();
			mt.id = nmt++;
			mtlist.nextobj();
		}

		// Set ids and nst variable after removing unused sts
		nst = 0;
		for(int i=0;i<NLANDCOVERTYPES;i++)
			nst_lc[i] = 0;
		stlist.firstobj();
		while (stlist.isobj) {
			StandType& st = stlist.getobj();
			st.id = nst++;
			nst_lc[st.landcover]++;
			stlist.nextobj();
		}

		stlist.firstobj();
		while (stlist.isobj) {
			StandType& st = stlist.getobj();

			if(st.intercrop == NATURALGRASS && pftlist[pftlist.getpftid(st.get_management(0).pftname)].phenology != CROPGREEN)
				dprintf("Warning: covercrop grass should not be activated in stand types without true crops\n");
				stlist.nextobj();
		}

		// Check that stand type exists for all active land covers when run_landcover==true.
		if(run_landcover) {
			for(int lc=0; lc<NLANDCOVERTYPES;lc++) {

				if(run[lc] && nst_lc[lc] == 0)
					fail("All active land covers must have at least one stand type in instruction file when run_landcover is true.");
			}
		}

		// Call various init functions on each PFT now that all settings have been read
		pftlist.firstobj();
		while (pftlist.isobj) {
			ppft = &pftlist.getobj();

			if (ifcalcsla) {
				if(!(ppft->phenology == CROPGREEN && run_landcover && ifnlim))
					// Calculate SLA
					ppft->initsla();
			}

			if (ifcalccton) {
				if(!(ppft->phenology == CROPGREEN && run_landcover && ifnlim))
					// Calculate leaf C:N ratio minimum
					ppft->init_cton_min();
			}

			// Calculate C:N ratio limits
			ppft->init_cton_limits();

			// Calculate nitrogen uptake strength dependency on root distribution
			ppft->init_nupscoeff();

			// Calculate regeneration characteristics for population mode
			ppft->initregen();

			pftlist.nextobj();
		}


		break;
	case CB_CHECKPFT:
		if (!checked_pft[ppft->name]) {
			checked_pft[ppft->name] = true;

			if (!itemparsed("lifeform")) badins("lifeform");
			if (!itemparsed("phenology")) badins("phenology");
			if (ppft->phenology==SUMMERGREEN || ppft->phenology==ANY)
				if (!itemparsed("phengdd5ramp")) badins("phengdd5ramp");
			if (ppft->phenology==RAINGREEN || ppft->phenology==ANY)
				if (!itemparsed("wscal_min")) badins("wscal_min");
			if (!itemparsed("leafphysiognomy")) badins("leafphysiognomy");
			if (!itemparsed("pathway")) badins("pathway");
			if (!itemparsed("pstemp_min")) badins("pstemp_min");
			if (!itemparsed("pstemp_low")) badins("pstemp_low");
			if (!itemparsed("pstemp_high")) badins("pstemp_high");
			if (!itemparsed("pstemp_max")) badins("pstemp_max");
			if (!itemparsed("lambda_max")) badins("lambda_max");
			if (!itemparsed("rootdist")) badins("rootdist");
			if (!itemparsed("gmin")) badins("gmin");
			if (!itemparsed("emax")) badins("emax");
			if (!itemparsed("respcoeff")) badins("respcoeff");

			if (!ifcalcsla || (ppft->phenology == CROPGREEN && run_landcover && ifnlim))
				if (!itemparsed("sla")) badins("sla");
			if (!ifcalccton || (ppft->phenology == CROPGREEN && run_landcover && ifnlim))
				if (!itemparsed("cton_leaf_min")) badins("cton_leaf_min");


			if (!itemparsed("cton_root")) badins("cton_root");
			if (!itemparsed("nuptoroot")) badins("nuptoroot");
			if (!itemparsed("km_volume")) badins("km_volume");
			if (!itemparsed("fnstorage")) badins("fnstorage");

			if (!itemparsed("reprfrac")) badins("reprfrac");
			if (!itemparsed("turnover_leaf")) badins("turnover_leaf");
			if (!itemparsed("turnover_root")) badins("turnover_root");
			if (!itemparsed("ltor_max")) badins("ltor_max");
			if (!itemparsed("intc")) badins("intc");

			if (run_landcover) {
				if (!itemparsed("landcover")) badins("landcover");
				if (!itemparsed("turnover_harv_prod")) badins("turnover_harv_prod");
				if (!itemparsed("harvest_slow_frac")) badins("harvest_slow_frac");
				if (!itemparsed("harv_eff")) badins("harv_eff");
				if (!itemparsed("res_outtake")) badins("res_outtake");

				if (ppft->landcover==CROPLAND) {
					if (ppft->phenology==CROPGREEN) {
						if (!itemparsed("sdatenh")) badins("sdatenh");
						if (!itemparsed("sdatesh")) badins("sdatesh");
						if (!itemparsed("lgp_def")) badins("lgp_def");
						if (!itemparsed("sd_adjust")) badins("sd_adjust");
						if (ppft->sd_adjust) {
							if (!itemparsed("sd_adjust_par1")) badins("sd_adjust_par1");
							if (!itemparsed("sd_adjust_par2")) badins("sd_adjust_par2");
							if (!itemparsed("sd_adjust_par3")) badins("sd_adjust_par3");						
						}
						if (!itemparsed("hlimitdatenh")) badins("hlimitdatenh");
						if (!itemparsed("hlimitdatesh")) badins("hlimitdatesh");
						if (!itemparsed("tb")) badins("tb");
						if (!itemparsed("trg")) badins("trg");
						if (!itemparsed("pvd")) badins("pvd");
						if (!itemparsed("vern_lag")) badins("vern_lag");
						if (!itemparsed("isintercropgrass")) badins("isintercropgrass");
						if (!itemparsed("psens")) badins("psens");
						if (!itemparsed("pb")) badins("pb");
						if (!itemparsed("ps")) badins("ps");
						if (!itemparsed("phu")) badins("phu");
						if (!itemparsed("phu_calc_quad")) badins("phu_calc_quad");
						if (!itemparsed("phu_calc_lin")) badins("phu_calc_lin");
						if (ppft->phu_calc_quad || ppft->phu_calc_lin) {
							if (!itemparsed("phu_min")) badins("phu_min");
							if (!itemparsed("phu_max")) badins("phu_max");
						}
						if (ppft->phu_calc_quad) {
							if (!itemparsed("phu_red_spring_sow")) badins("phu_red_spring_sow");
						}
						else if (ppft->phu_calc_lin) {
							if (!itemparsed("phu_interc")) badins("phu_interc");
							if (!itemparsed("ndays_ramp_phu")) badins("ndays_ramp_phu");
						}
						if (!itemparsed("fphusen")) badins("fphusen");
						if (!itemparsed("shapesenescencenorm")) badins("shapesenescencenorm");
						if (!itemparsed("flaimaxharvest")) badins("flaimaxharvest");
						if (!itemparsed("aboveground_ho")) badins("aboveground_ho");
						if (!itemparsed("ifsdautumn")) badins("ifsdautumn");
						if (!itemparsed("tempautumn")) badins("tempautumn");
						if (!itemparsed("tempspring")) badins("tempspring");
						if (!itemparsed("maxtemp_sowing")) badins("maxtemp_sowing");
						if (!itemparsed("hiopt")) badins("hiopt");
						if (!itemparsed("himin")) badins("himin");
						if (!itemparsed("res_outtake")) badins("res_outtake");
						if (!itemparsed("frootstart")) badins("frootstart");
						if (!itemparsed("frootend")) badins("frootend");
						if (!itemparsed("turnover_harv_prod")) badins("turnover_harv_prod");

						if(ifnlim) {
							if (!itemparsed("fertrate")) badins("fertrate");
							if (!itemparsed("N_appfert")) badins("N_appfert");
							if (!itemparsed("T_vn_min")) badins("T_vn_min");
							if (!itemparsed("T_vn_opt")) badins("T_vn_opt");
							if (!itemparsed("T_vn_max")) badins("T_vn_max");
							if (!itemparsed("T_veg_min")) badins("T_veg_min");
							if (!itemparsed("T_veg_opt")) badins("T_veg_opt");
							if (!itemparsed("T_veg_max")) badins("T_veg_max");
							if (!itemparsed("T_rep_min")) badins("T_rep_min");
							if (!itemparsed("T_rep_opt")) badins("T_rep_opt");
							if (!itemparsed("T_rep_max")) badins("T_rep_max");
							if (!itemparsed("photo")) badins("photo");
							if (!itemparsed("dev_rate_veg")) badins("dev_rate_veg");
							if (!itemparsed("dev_rate_rep")) badins("dev_rate_rep");
							if (!itemparsed("a1")) badins("a1");
							if (!itemparsed("b1")) badins("b1");
							if (!itemparsed("c1")) badins("c1");
							if (!itemparsed("d1")) badins("d1");
							if (!itemparsed("a2")) badins("a2");
							if (!itemparsed("b2")) badins("b2");
							if (!itemparsed("c2")) badins("c2");
							if (!itemparsed("d2")) badins("d2");
							if (!itemparsed("a3")) badins("a3");
							if (!itemparsed("b3")) badins("b3");
							if (!itemparsed("c3")) badins("c3");
							if (!itemparsed("d3")) badins("d3");
						}
					}
					else if (ppft->phenology==ANY) {
						if(ppft->phenology==ANY)
							if (!itemparsed("laimax")) badins("laimax");

						if(ppft->isintercropgrass)
							if (!itemparsed("harv_eff_ic")) badins("harv_eff_ic");
					}
				}
			}

			// guess2008 - DLE
			if (!itemparsed("drought_tolerance")) badins("drought_tolerance");

			// bvoc
			if(ifbvoc){
				if (!itemparsed("ga")) badins("ga");
				if (!itemparsed("eps_iso")) badins("eps_iso");
				if (!itemparsed("seas_iso")) badins("seas_iso");
				if (!itemparsed("eps_mon")) badins("eps_mon");
				if (!itemparsed("storfrac_mon")) badins("storfrac_mon");
			}

			if (ppft->lifeform==TREE) {
				if (!itemparsed("cton_sap")) badins("cton_sap");
				if (!itemparsed("turnover_sap")) badins("turnover_sap");
				if (!itemparsed("wooddens")) badins("wooddens");
				if (!itemparsed("crownarea_max")) badins("crownarea_max");
				if (!itemparsed("k_allom1")) badins("k_allom1");
				if (!itemparsed("k_allom2")) badins("k_allom2");
				if (!itemparsed("k_allom3")) badins("k_allom3");
				if (!itemparsed("k_rp")) badins("k_rp");
				if (!itemparsed("k_latosa")) badins("k_latosa");
				if (vegmode==COHORT || vegmode==INDIVIDUAL) {
					if (!itemparsed("kest_repr")) badins("kest_repr");
					if (!itemparsed("kest_bg")) badins("kest_bg");
					if (!itemparsed("kest_pres")) badins("kest_pres");
					if (!itemparsed("longevity")) badins("longevity");
					if (!itemparsed("greff_min")) badins("greff_min");
					if (!itemparsed("alphar")) badins("alphar");
					if (!itemparsed("est_max")) badins("est_max");
				}
			}
			if (iffire) {
				if (!itemparsed("litterme")) badins("litterme");
				if (!itemparsed("fireresist")) badins("fireresist");
			}
			if (ifcalcsla) {
				if (!itemparsed("leaflong")) {
					sendmessage("Error",
					            "Value required for leaflong when ifcalcsla enabled");
					plibabort();
				}
				if (itemparsed("sla") && !(ppft->phenology == CROPGREEN && ifnlim))
					sendmessage("Warning",
					            "Specified sla value not used when ifcalcsla enabled");
			}
			if (vegmode==COHORT || vegmode==INDIVIDUAL) {
				if (!itemparsed("parff_min")) badins("parff_min");
			}

			if (ifcalccton) {
				if (!itemparsed("leaflong")) {
					sendmessage("Error",
					            "Value required for leaflong when ifcalccton enabled");
					plibabort();
				}
				if (itemparsed("cton_leaf_min") && !(ppft->phenology == CROPGREEN && ifnlim))
					sendmessage("Warning",
					            "Specified cton_leaf_min value not used when ifcalccton enabled");
			}
		}
		else {
			// This PFT has already been parsed once, don't allow changing parameters
			// which would have incurred different checks above.
			if (itemparsed("lifeform") ||
			    itemparsed("phenology")) {
				sendmessage("Error",
				            "Not allowed to redefine lifeform or phenology in second PFT definition");
				plibabort();
			}
		}

		if (itemparsed("include")) {
			includepft_map[ppft->name] = includepft;
		}

		break;

	case CB_CHECKST:
		if (!checked_st[pst->name]) {
			checked_st[pst->name] = true;
		}

		if (itemparsed("stinclude")) {
			includest_map[pst->name] = includest;
		}

		break;

	case CB_CHECKMT:
		if (!checked_mt[pmt->name]) {
			checked_mt[pmt->name] = true;
		}

		if (itemparsed("mtinclude")) {
			includemt_map[pmt->name] = includemt;
		}

		break;
	}
}

void plib_receivemessage(xtring text) {

	// Output of messages to user sent by PLIB

	dprintf((char*)text);
}

void read_instruction_file(const char* insfilename) {
	bool exists_getclim_driver_file;
	xtring getclim_driver_file_path;

	if (!fileexists(insfilename)) {
		fail("Error: could not open %s for input", (const char*)insfilename);
	}

	// Initialise PFT count

	npft = 0;
	nst = 0;
	nmt=0;

	checked_pft.clear();
	includepft_map.clear();

	pftlist.killall();

	initsettings();

	// Clear params from previous run, saving needed params before clearing

	if (param.isparam("getclim_driver_file")) {
		exists_getclim_driver_file = true;
		getclim_driver_file_path = param["getclim_driver_file"].str;
	}
	else {
		exists_getclim_driver_file = false;
	}

	param.killall();

	// Initialise simulation settings and PFT parameters from instruction script
	if (!plib(insfilename)) {
		fail("Bad instruction file!");
	}

	// Reinstate params saved from clearing above
	if (exists_getclim_driver_file && getclim_driver_file_path != "")
		param.addparam("getclim_driver_file", getclim_driver_file_path);

}

void printhelp() {

	// Calls PLIB to output help text

	ifhelp=true;
	plibhelp();
	ifhelp=false;
}
