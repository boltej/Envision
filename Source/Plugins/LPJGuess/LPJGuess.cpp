// LPJGuess.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#pragma hdrstop

#include "LPJGuess.h"
#include "framework.h"
#include <..\Plugins\Flow\Flow.h>

#include <DATE.HPP>
#include "guess.h"
#include "commandlinearguments.h"
#include "guessserializer.h"
#include "parallel.h"

#include "inputmodule.h"
#include "driver.h"
#include "commonoutput.h"

#include "bvoc.h"
#include "landcover.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

xtring file_log = "guess.log";
//extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new LPJGuess; }


// constructor
//LPJGuess::~LPJGuess(void)
 //  {
  // }

// override API Methods
bool LPJGuess::Guess_Standalone(FlowContext *pFlowContext, LPCTSTR initStr)
   {
	//need to pass name of *.ins file. {insfile="C:\\LPJ_GUESS\\input\\global_cru.ins" help=false parallel=false ...}	CommandLineArguments
	//int argc=2, char* argv[]
   //CommandLineArguments args(arg.argc, arg.argv);
		// Call the framework
	//framework(args);

	set_shell(new CommandLineShell(file_log));
	framework(pFlowContext, "cru_ncep", "C:\\envision\\studyareas\\CalFEWS\\LPJGuess\\input\\global_cru_new.ins");

   return TRUE;
   }

bool LPJGuess::Guess_Flow(FlowContext *pFlowContext, bool useInitialSeed)
   {
	if (pFlowContext->timing & GMT_INIT) // Init()
		return LPJGuess::Init_Guess(pFlowContext, "cru_ncep", "C:\\envision\\studyareas\\CalFEWS\\LPJGuess\\input\\global_cru_new.ins");

	if (pFlowContext->timing & GMT_START_STEP) // Init()
		return LPJGuess::Run_Guess(pFlowContext, NULL);
	
   return TRUE;
   }

bool LPJGuess::Init_Guess(FlowContext *pFlowContext, const char* input_module_name, const char* instruction_file)
{
	//need to pass name of *.ins file. {insfile="C:\\LPJ_GUESS\\input\\global_cru.ins" help=false parallel=false ...}	CommandLineArguments
	//int argc=2, char* argv[]
   //CommandLineArguments args(arg.argc, arg.argv);
		// Call the framework
	//framework(args);

	set_shell(new CommandLineShell(file_log));

	// The 'mission control' of the model, responsible for maintaining the
	// primary model data structures and containing all explicit loops through
	// space (grid cells/stands) and time (days and years).

	using std::auto_ptr;

	//	const char* input_module_name = args.get_input_module();

	auto_ptr<InputModule> input_module(InputModuleRegistry::get_instance().create_input_module(input_module_name));

	GuessOutput::OutputModuleContainer output_modules;
	GuessOutput::OutputModuleRegistry::get_instance().create_all_modules(output_modules);

	// Read the instruction file to obtain PFT static parameters and
	// simulation settings
	read_instruction_file(instruction_file/*args.get_instruction_file()*/);

	// Initialise input/output

	input_module->init();
	output_modules.init();

	print_logfile_heading();

	// Nitrogen limitation
	if (ifnlim && !ifcentury) {
		fail("\n\nIf nitrogen limitation is switched on then century soil module also needs to be switched on!");
	}

	// bvoc
	if (ifbvoc) {
		initbvoc();
	}

	// Create objects for (de)serializing grid cells
	auto_ptr<GuessSerializer> serializer;
	auto_ptr<GuessDeserializer> deserializer;

	if (save_state) {
		serializer = auto_ptr<GuessSerializer>(new GuessSerializer(state_path, GuessParallel::get_rank(), GuessParallel::get_num_processes()));
	}

	if (restart) {
		deserializer = auto_ptr<GuessDeserializer>(new GuessDeserializer(state_path));
	}

	while (true) {

		// START OF LOOP THROUGH GRID CELLS

		// Initialise global variable date
		// (argument nyear not used in this implementation)
		date.init(1);

		// Create and initialise a new Gridcell object for each locality
		Gridcell gridcell;
		

		Gridcell *pG = new Gridcell;
		Gridcell gridc = *pG;
		// Call input module to obtain latitude and driver data for this grid cell.
		if (!input_module->getgridcell(gridc)) {
			break;
		}

		// Initialise certain climate and soil drivers
		gridcell.climate.initdrivers(gridcell.get_lat());

		if (run_landcover && !restart) {
			// Read landcover and cft fraction data from 
			// data files for the spinup period and create stands
			landcover_init(gridcell, input_module.get());
		}

		if (restart) {
			// Get the whole grid cell from file...
			deserializer->deserialize_gridcell(gridcell);
			// ...and jump to the restart year
			date.year = state_year;
		}

		float lt = gridcell.get_lat();
		float lg = gridcell.get_lon();

		int hruCount = pFlowContext->pFlowModel->GetHRUCount();
		for (int h = 0; h < hruCount; h++)
		   {
			HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
			if (lt == pHRU->m_centroid.y && lg == pHRU->m_centroid.x)
				pHRU->m_pGuessArray.Add(&gridcell);

		   }
	}
	//Gridcell *pGridcell = new Gridcell;
	//Gridcell gridcell;
	//if (!input_module->getgridcell(gridcell)) {
	//	break;
	//}

	//framework(pFlowContext, "cru_ncep", "C:\\envision\\studyareas\\CalFEWS\\LPJGuess\\input\\global_cru_new.ins");
	//Create array of gridcells from global_cru_new

	return TRUE;
}

bool LPJGuess::Run_Guess(FlowContext *pFlowContext, LPCTSTR initStr)
   {
	// int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
	int hruCount = pFlowContext->pFlowModel->GetHRUCount();

	ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable("HBV");   // store this pointer (and check to make sure it's not NULL)

	int doy = pFlowContext->dayOfYear;  // int( fmod( timeInRun, 364 ) );  // zero based day of year
	int _month = 0; int _day; int _year;
	BOOL ok = ::GetCalDate(doy, &_year, &_month, &_day, TRUE);

	// iterate through hrus/hrulayers 

	for (int h = 0; h < hruCount; h++)
	   {
		HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
		for (int i = 0; i < pHRU->m_pGuessArray.GetSize(); i++)
		   {
			Gridcell *pGridcell = new Gridcell;
		   }
		//Assume each HRU has a array of pointers to guess gridcells
		//framework(pFlowContext, "cru_ncep", "C:\\envision\\studyareas\\CalFEWS\\LPJGuess\\input\\global_cru_new.ins");

	   }
   return TRUE;
   }
