
#pragma once

//#include "resource.h"		// main symbols

#include <EnvExtension.h>
#include <PtrArray.h>

//#include <randgen/Randln.hpp>    // lognormal
//#include <randgen/RandNorm.hpp>  // normal
//#include <randgen/RandUnif.hpp>  // normal

#define _EXPORT __declspec( dllexport )

//class Calvin;


class _EXPORT Calvin : public EnvModelProcess
{

public:
	// constructor
	Calvin(void);
	~Calvin(void);

	// override API Methods
	virtual bool Init(EnvContext *pEnvContext, LPCTSTR initStr);
	virtual bool InitRun(EnvContext *pEnvContext, bool useInitialSeed);
	virtual bool Run(EnvContext *pContext);
   virtual bool EndYear(EnvContext *pEnvContext);

protected:
	// we'll add model code here as needed
	bool InitPython();
	bool Update(EnvContext *pEnvContext);

	CString m_pyFunction;
	CString m_pyModulePath;
	CString m_pyModuleName;

	bool LoadXml(EnvContext*, LPCTSTR filename);

	// member data (from XML)
	CString m_pythonPath;


	//RandLogNormal m_randLogNormal;
	//RandNormal m_randNormal;
	//RandUniform m_randUniform;

	int m_colIduImprValue;     // IMPR_VALUE
public:
   FDataObj *m_pResData;                              // (memory managed here)
   PtrArray < FDataObj > m_reservoirDataArray;
   int m_numRes;
   CString m_reservoirList;
   CString m_outflowLinks;
   CArray< CString, CString > m_reservoirNameArray;
   CArray< CString, CString > m_outflowLinkArray;
};


