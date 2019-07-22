#include <FDataObj.h>
#include <guess.h>
#include <framework.h>
class FlowContext;
class Reach;


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
	// initialization
	bool Init(FlowContext *pFlowContext, LPCTSTR);
	bool InitRun(FlowContext *pFlowContext, bool useInitialSeed);
	bool Run(FlowContext *pFlowContext);		// IDU UGB from IDU layer

// public (exported) methods
public:
	//-------------------------------------------------------------------
	//------ models -----------------------------------------------------
	//-------------------------------------------------------------------
//	float Framework(FlowContext *pFlowContext);          // formerly HBV_Global



};