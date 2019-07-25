#include <FDataObj.h>
#include <guess.h>
#include <framework.h>
#include <PtrArray.h>

class FlowContext;
class Reach;
class Gridcell;


class GridCellIndexArray : public CArray< UINT > 
   { };



class LPJGuess
{
public:
	LPJGuess(void) : m_pClimateData(NULL)
		, m_col_cfmax(-1)

	{ }

	~LPJGuess(void) { if (m_pClimateData) delete m_pClimateData; }

protected:
	int m_col_cfmax;
	FDataObj *m_pClimateData;

   // for HRUs, hold array of corresponding GridCells
   PtrArray< GridCellIndexArray > m_hruGridCells;
   CArray< HRU*, HRU*> m_gridCellHRUArray;
	// initialization

	bool InitRun(FlowContext *pFlowContext, bool useInitialSeed);
	bool Run(FlowContext *pFlowContext);		// IDU UGB from IDU layer

// public (exported) methods
public:
	//-------------------------------------------------------------------
	//------ models -----------------------------------------------------
	//-------------------------------------------------------------------
//	float Framework(FlowContext *pFlowContext);          // formerly HBV_Global
	bool Guess_Standalone(FlowContext *pFlowContext, LPCTSTR initStr);
	bool Guess_Flow(FlowContext *pFlowContext, bool useInitialSeed);
	bool Init_Guess(FlowContext *pFlowContext,  const char* input_module_name, const char* instruction_file);
	bool Run_Guess(FlowContext *pFlowContext, LPCTSTR initStr);
};