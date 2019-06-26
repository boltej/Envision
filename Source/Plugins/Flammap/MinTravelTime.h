/*
This file is part of Envision.

Envision is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Envision is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Envision.  If not, see <http://www.gnu.org/licenses/>

Copywrite 2012 - Oregon State University

*/
//MinTravelTime.h
//defines exported class for MinTravelTime.dll

#ifdef BUILD_MTTDLL 
#define MTTDLL_EXPORT __declspec(dllexport) 
#else 
#define MTTDLL_EXPORT __declspec(dllimport) 

#endif 
#pragma once
//#include "FSProDataTypes.h"
class CMTT;
class CFlamMap;
struct BurnDay;
//#include <list>
class MTTDLL_EXPORT CPerimeterLine
{
public:
	CPerimeterLine();
	~CPerimeterLine();
	int SetNumPoints(int _numPoints);
	int GetNumPoints();
	bool GetPoint(int pointNum, float *x, float *y);
	bool SetPoint(int pointNum, float x, float y);
private:
	int numPoints;
	float *xVals;
	float *yVals;
};

class MTTDLL_EXPORT CPerimeter
{
public:
	CPerimeter();
	~CPerimeter();
	int SetNumLines(int _numLines);
	int GetNumLines();
	CPerimeterLine *GetLine(int lineNum);
private:
	int numLines;
	CPerimeterLine *lines;
};

class MTTDLL_EXPORT CMTTSpot
{
public:
	CMTTSpot();
	~CMTTSpot();
	void SetData(double _launchX, double _launchY, double _landX, double _landY);
	void SetLaunchX(double val);
	void SetLaunchY(double val);
	void SetLandX(double val);
	void SetLandY(double val);
	double GetLaunchX();
	double GetLaunchY();
	double GetLandX();
	double GetLandY();
private:
	double launchX;
	double launchY;
	double landX;
	double landY;
};

class MTTDLL_EXPORT CMTTSpots
{
public:
	CMTTSpots();
	~CMTTSpots();
	int SetNumSpots(int _numSpots);
	int GetNumSpots();
	void SetSpot(int spotNum, double launchX, double launchY, double landX, double landY);
	CMTTSpot *GetSpot(int spotNum);
private:
	int numSpots;
	CMTTSpot *spots;
};

class MTTDLL_EXPORT CMinTravelTime
{
public:
	static const int RATEOFSPREAD = 0;
	static const int INFLUENCE = 1;
	static const int ARRIVALTIME = 2;
	static const int INTENSITYMAP = 3;
	static const int FLOWPATHS = 4;
	static const int MAJORPATHS = 5;
	//static const int ARRIVALCONTOUR = 6;
	CMinTravelTime();
	~CMinTravelTime();
	int ResetMTT();
	CMTT *mtt;
	int SetLandscapeFile(char *_lcpFileName);
	double SetResolution(double res);
	int SetAnalysisArea(double tEast, double tWest, double tNorth, double tSouth);
	int SetIgnitionPoint(double x, double y);
	int SetIgnition(char *shapeFileName);
	int SetBarriers(char *shapeFileName);
	int SetFillBarriers(int set);
	long SetMaxSimTime(long minutes = 0);
	long SetPathInterval(long interval = 500);
	int SetNumProcessors(int numThreads = 1);
	int SetStartProcessor(int _procNum);
	int GetStartProcessor();
	int SetNumLat(int _numLat);
	int GetNumLat();
	int SetNumVert(int _numVert);
	int GetNumVert();
	double SetSpotProbability(double _spotProb);
	double GetSpotProbability();
	int SetSpotDelay(int minutes);
	int GetSpotDelay();
	double GetResolution();
	long GetNumCols();
	long GetNumRows();
	double GetWest();
	double GetSouth();
	double GetEast();
	double GetNorth();
	CFlamMap *GetFlamMap();
	int LoadMTTInputs(char *_inputFile);
	int LoadTOMInputs(char *_inputFile);
	char *CommandFileError(int i_ErrNum); 
	char *GetErrorMessage(int errNum);
	void SetDisableFlows(int disable);
	void SetDisableMajorPaths(int disable);
	int CanLaunchMTT(void);
	int CanLaunchTOM(void);
	int LaunchMTT(void);
	int LaunchTOM(void);
	int LaunchVariableWeatherMTT(BurnDay *_burndays, int _nBurnDays);
	int SetFlamMap(CFlamMap *tFlamMap);
	bool SetRiskGrids(float **_grid, float **_intensity, double  *_spotProbability, double *_spotIgnitionDelay, __int64 _NumGrids);
	//bool SetNetworkFlowGrid(int *grid, int nRows, int nCols);
	//bool SetBurnDays(BurnDay *_bd, int _nBD);
	int LaunchMTTQuick();
	double GetMTTProgress(void);
	int CancelMTT(void);

	int WriteROSGrid(char *trgName);
	int WriteArrivalTimeGrid(char *trgName);
	int WriteFlameLengthGrid(char *trgName);
	int WriteFliMapGrid(char *trgName);
	int WriteInfluenceGrid(char *trgName);
	int WriteROSGridBinary(char *trgName);
	int WriteArrivalTimeGridBinary(char *trgName);
	int WriteFliMapGridBinary(char *trgName);
	int WriteInfluenceGridBinary(char *trgName);
	int WriteFlamMapOutputLayerBinary(int Layer, char *trgName);
	int WriteFlamMapOutputLayerAscii(int Layer, char *trgName);
	int WriteIgnitionGrid(char *trgName);
	int WriteFlowPathsShapeFile(char *trgName);
	int WriteMajorPathsShapeFile(char *trgName);
	int WriteEmberCSV( char *trgName);
	int WriteEmberShapeFile( char *trgName);
	int WriteWindVectorKML(char *fileNamePath, double resolution);
	double GetWindVectorKMLResolution();
	int WriteTimingsFile(char *trgName);
	int WriteTreatOpportunitiesGrid(char *trgName);
	int WriteTreatmentGrid(char *trgName);
	int WriteTreatROSGrid(char *trgName);
	int WriteTreatInfluenceGrid(char *trgName);
	int WriteTreatArrivalTimeGrid(char *trgName);
	int WriteTreatFliMap(char *trgName);
	int WriteTreatFlowPaths(char *trgName);
	int WriteTreatMajorPaths(char *trgName);
	int WriteTreatArrivalTimeContour(char *trgName);

	//grid based output
	float *GetFlamMapOutputLayer(int Layer);
	double *GetROSGrid();
	double *GetArrivalTimeGrid();
	//float *GetROSGrid();
	//float *GetArrivalTimeGrid();
	float *GetFliMapGrid();
	double *GetInfluenceGrid();
	float *GetFlameLengthGrid();
	//float *GetInfluenceGrid();
	float *GetIgnitionGrid();
	int SetIgnitionGrid(float *grid, int nRows, int nCols);
	//TOM output Grids
	float *GetTreatOpportunitiesGrid();
	double *GetTreatmentGrid();
	double *GetTreatROSGrid();
	double *GetTreatInfluenceGrid();
	double *GetTreatArrivalTimeGrid();
	float *GetTreatFliMap();
	//float *GetTreatmentsGrid();
	//float *GetTreatOpportunitiesGrid();

	//vector based output
	long GetNumFlowVecs();
	long GetNumFlowVecPoints(long vecNum);
	float GetFlowVecPointX(long vecNum, long pointNum);
	float GetFlowVecPointY(long vecNum, long pointNum);
	long GetNumMajorVecs();
	long GetNumMajorVecPoints(long vecNum);
	float GetMajorVecPointX(long vecNum, long pointNum);
	float GetMajorVecPointY(long vecNum, long pointNum);
	//vector based TOM output
	long GetNumTreatFlowVecs();
	long GetNumTreatFlowVecPoints(long vecNum);
	float GetTreatFlowVecPointX(long vecNum, long pointNum);
	float GetTreatFlowVecPointY(long vecNum, long pointNum);
	long GetNumTreatMajorVecs();
	long GetNumTreatMajorVecPoints(long vecNum);
	float GetTreatMajorVecPointX(long vecNum, long pointNum);
	float GetTreatMajorVecPointY(long vecNum, long pointNum);
	CPerimeter *GetFirePerimeter(int newNum = -1);
	CMTTSpots *GetFireSpots();

private:
	double *rosGrid;
	double *arrivalTimeGrid;
	//float *rosGrid;
	//float *arrivalTimeGrid;
	float *fliMapGrid;
	double *influenceGrid;
	float *flameLengthGrid;
	//float *influenceGrid;
	float *ignitionGrid;
	double *treatmentGrid;
	float *treatOpportunitiesGrid;
	CPerimeter *perimeter;
	CMTTSpots *mttSpots;
	//int *networkFlowGrid;
	//int nNfgRows;
	//int nNfgRows;


};