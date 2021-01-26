#include <FDataObj.h>
#include <guess.h>
#include <framework.h>
#include <PtrArray.h>
#include "commonoutput.h"
#include "guessserializer.h"
//#include "inputmodule.h"
class FlowContext;
class Reach;
class Gridcell;
class HRU;
class OutputModuleContainer;


class GridCellIndexArray : public CArray< UINT > 
   { };



class LPJGuess
{
public:
	LPJGuess(void) : m_pClimateData(NULL)
		, m_input_module(NULL)
		, m_col_cfmax(-1)

	{ }

	~LPJGuess(void) { if (m_pClimateData) delete m_pClimateData; if (m_input_module) delete m_input_module; }

protected:
	int m_col_cfmax;
	FDataObj *m_pClimateData;

   // for HRUs, hold array of corresponding GridCells
   PtrArray< GridCellIndexArray > m_hruGridCells;
   CArray< HRU*, HRU*> m_gridCellHRUArray;
   PtrArray<Gridcell> m_gridCellArray;
   InputModule* m_input_module;
   GuessOutput::OutputModuleContainer m_output_modules;
   GuessSerializer* m_serializer;
   GuessDeserializer* m_deserializer;
	void move_to_flow(Gridcell& gridcell);
	void out_daily(Gridcell& gridcell);
	// initialization

	//bool InitRun(FlowContext *pFlowContext, bool useInitialSeed);
	//bool Run(FlowContext *pFlowContext);		// IDU UGB from IDU layer

// public (exported) methods
public:
	//-------------------------------------------------------------------
	//------ models -----------------------------------------------------
	//-------------------------------------------------------------------
//	float Framework(FlowContext *pFlowContext);          // formerly HBV_Global
	bool Guess_Standalone(FlowContext *pFlowContext, LPCTSTR initStr);
	bool Guess_Flow(FlowContext *pFlowContext, bool useInitialSeed);
	bool Init_Guess(FlowContext *pFlowContext,  const char* input_module_name, const char* instruction_file);
	bool Init_Guess_Complete(FlowContext *pFlowContext, const char* input_module_name, const char* instruction_file);
	bool Run_Guess(FlowContext *pFlowContext,  const char* input_module_name, const char* instruction_file);
	bool Run_Guess_Complete(FlowContext *pFlowContext, const char* input_module_name, const char* instruction_file);
};