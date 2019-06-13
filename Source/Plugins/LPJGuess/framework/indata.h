////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \file indata.h
/// \brief Classes for text input data (used mainly for landcover input).
/// File format can be either line 1:lon lat, line 2 etc.: year data-columns OR line 1: header, 
/// line 2 etc.: lon lat year data-columns. For local static data, use: lon lat data-columns,
/// for global static data, use: dummy data-columns (with "static" as first word in header).
/// \author Mats Lindeskog
/// $Date: 2016-12-08 18:24:04 +0100 (Thu, 08 Dec 2016) $
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef INDATA_H
#define INDATA_H
#include "guess.h"

using std::min;
using std::max;

/// Type for storing grid cell longitude, latitude and description text
struct Coord {
	
	int id;
	double lon;
	double lat;
	xtring descrip;
};

namespace InData {

const int MAXLINE = 20000;
const int MAXNAMESIZE = 50;
const int MAXRECORDS = 200;
const int MAXLINESPARSE = 30000;
const int NOTFOUND = -999;
const double MAX_SEARCHRADIUS = 1.0;

/// Formats in text input file
typedef enum {EMPTY, GLOBAL_STATIC, GLOBAL_YEARLY, LOCAL_STATIC, LOCAL_YEARLY} fileformat;

/// Type for storing grid cell longitude, latitude and associated data position on disk
struct CoordPos {
	double lon;
	double lat;
	long int pos;
};

// Forward declaration of TimeDataDmem
class TimeDataDmem;

/// Class for reading a set of double data at coordinate positions over time (years), alternatively static or/and global.
class TimeDataD	{

	// PRIVATE VARIABLES

	/// File pointer to input file
	FILE *ifp;
	/// File name
	char *fileName;
	/// Number of data columns
	int nColumns;								//Set in ParseFormat()
	/// Number of data years
	int nYears;									//Set in ParseNYears()
	/// Number of data gridcells
	int nCells;									//Set in ParseNCells()
	// Spacial resolution of input data in degrees
	double spatial_resolution;
	/// Offset to be used when searching for coordinates
	double offset;
	/// Whether the input file structure includes a header line with column names and coordinates on each line of data
	bool ifheader;
	/// String array of data column names
	char *header_arr[MAXRECORDS];
	/// Coordinates for current gridcell
	Coord currentStand;
	/// Pointer to data for one (current) gridcell
	double *data;								//allocated in Allocate(), set in Load(), Load(Coord) or LoadNext()
	/// Pointer to array with data years
	int *year;									//allocated in Allocate(), set in Load(), Load(Coord) or LoadNext()
	/// Pointer to array with indication whether data column contains values > 0 or not
	bool *checkdata;							//allocated in CheckIfPresent()
	/// Format of input data
	fileformat format;
	/// First year of data
	int firstyear;								//set in ParseNYears() or ParseNYearsSpatial()
	/// Whether data is currently being checked by CheckIfPresent()
	bool ischeckingdata;
	/// Whether data for the requested coordinates have been found and loaded
	bool loaded;
	/// Whether data sums up to 1.0
	bool unity_data;

	/// Pointer to memory copy of all data for the gridlist
	TimeDataDmem *memory_copy;
	/// Pointer to map of file positions of data for all gridcells in the file
	CoordPos *filemap;

	// PRIVATE METHODS

	/// Methods for parsing input file data format and structure
	fileformat ParseFormat();					//Called from Open(); Returns 0 if wrong format, sets nColumns, ifheader and header_arr[]
	int ParseNYears();							//Called from Open()
	int ParseNYearsGlobal();					//Called from ParseNYears()
	int ParseNYearsLocal();						//Called from ParseNYears()
	void ParseNCells();
	double ParseSpatialResolution();			//Called from Open()
	bool ParseNormalisation();

	/// Allocates memory for dynamic data structures
	bool Allocate();							//Called from Open()
	/// Finds data for a gridcell in input file. Quick version
	bool FindRecord(Coord c) const;
	/// Finds data for a gridcell in input file. Slower version, can handle blank lines
	bool FindRecord2(Coord c) const;
	/// Converts data column name to data column index
	int GetColumn(const char* name) const;		// Returns column number for header name.
	/// Converts calender year to valid year position in data array.
	int CalenderYearToPosition(int calender_year) const;
	/// Creates map of the file positions of all gridcells' data
	void CreateFileMap();
	/// Sets the file pointer to required position (found in the file map)
	void SetPosition(long int pos) {fseek(ifp, pos, 0);}
	/// Rewinds the file pointer
	void Rewind() {if(ifp) rewind(ifp);}
	/// Loads local data for a certain coordinate from a file map. Returns 0 if coordinate not found.
	bool LoadFromMap(Coord c);
	/// Sets offset to be used when searching for coordinates.
	void SetOffset(double gridlist_offset);
	/// Copies all data for the specified gridlist to memory
	void CopyToMemory(int ncells, ListArray_id<Coord>& lonlatlist);

public:

	// PUBLIC METHODS

	/// Constructor
	TimeDataD(fileformat format=EMPTY);		// default format value can only be used with header version input files !
	/// Deconstructor
	~TimeDataD();

	// Methods to open input files and access data

	/// Opens input file, checks format and allocates memory. Returns false if error
	bool Open(const char* name);
	/// Opens input file, checks format and allocates memory. Copies all data for the gridlist into memory if LUTOMEMORY is defined. Returns false if error.
	bool Open(const char* name, ListArray_id<Coord>& gridlist, double gridlist_offset = 0.0);
	/// Releases dynamically allocated memory.
	void Close();
	/// Writes the data of the current coordinate to an output file
	void Output(char* outfile);
	/// Loads global data, closes input file.
	bool Load();
	/// Loads data for a certain coordinate. Returns false if coordinate not found.
	bool Load(Coord c);
	/// Steps through a data file, loading each coordinate's data consecutively. Returns false if error.
	bool LoadNext(long int *pos = NULL);
	/// Returns a single data value for a certain year and data column
	double Get(int calender_year, int column) const;
	/// Returns a single data value for column with header string name. Returns -999 if name not found.
	double Get(int calender_year, const char* name) const;
	/// Copies the data for the current gridcell for one year to an array.
	void Get(int calender_year, double* dataX) const;
	/// Copies all data for the current gridcell to an array.
	void Get(double* dataX) const;

	// Methods to access private data

	/// Returns the number of data columns
	int GetnColumns() const {return nColumns;}
	/// Copies the data column names to a string array
	bool GetHeader(char *cropnames[MAXRECORDS]) const;
	/// Copies the whole header to a string
	bool GetHeaderFull(char *header_line) const;
	/// Returns a pointer to a data name string for a column by its index
	char* GetHeader(int record) const;
	/// Returns the current coordinates
	Coord& GetCoord() {return currentStand;}
	/// Returns the number of gridcells with data in the input file
	int GetNCells();	// Calls ParseNCells() if nCells not yet set
	/// Returns the number of years in the input data
	int GetnYears() const {return nYears;}
	/// Returns the first year in the input data
	int GetFirstyear() const {return firstyear;}
	/// Returns the data format (EMPTY, GLOBAL_STATIC, GLOBAL_YEARLY, LOCAL_STATIC, LOCAL_YEARLY)
	fileformat GetFormat() const {return format;}
	/// Returns true if data for requested coordinates are found, false if not.
	bool isloaded();
	/// Sets spacial resolution
	void SetSpacialResolution(double resolution) {spatial_resolution = resolution;}
	/// Returns spacial resolution
	double GetSpacialResolution() const {return spatial_resolution;}
	double GetOffset() const { return offset;}
	bool NormalisedData();

// Functions for finding out if data columns contain sensible data for a specified gridlist

	/// Checks if data column has any values > 0 in any of the gridcells in the gridlist
	void CheckIfPresent(ListArray_id<Coord>& gridlist);
	// Returns true if data column has any values > 0 in any of the gridcells in the gridlist (after CheckIfPresent() call)
	bool item_has_data(char* name);
	// Returns true if data name is in header
	bool item_in_header(char* name);

	/// Used by TimeDataDmem class to set pointer to full data copy
	void register_memory_copy(TimeDataDmem* mem_copy) {memory_copy = mem_copy;}
};

/// Class for loading all data for a gridlist to memory.
class TimeDataDmem {

	// PRIVATE VARIABLES

	/// Pointer to gridlist
	Coord *gridlist;
	/// Pointer to data array
	double **data;
	/// Number of data columns
	int nColumns;
	/// Number of data years
	int nYears;
	/// Number of data gridcells
	int nCells;
	// Spacial resolution of input data in degrees
	double spatial_resolution;
	/// Whether the input file structure includes a header line with column names and coordinates on each line of data
	bool ifheader;
	/// String array of data column names
	char *header_arr[MAXRECORDS];
	/// Index of current gridcell in data array
	int currentCell;
	/// First year of data
	int firstyear;
	/// Whether data for the requested coordinates have been found and loaded
	bool loaded;

	// PRIVATE METHODS

	/// Converts calender year to valid year position in data array.
	int CalenderYearToPosition(int calender_year) const;
	/// Sets coord at index position
	void SetCoord(int index, Coord c);
	/// Sets data at index position
	void SetData(int index, double* data);

public:

	// PUBLIC METHODS

	/// Constructor
	TimeDataDmem();
	/// Deconstructor
	~TimeDataDmem();

	/// Allocates memory.
	void Open(int nCells, int nColumns, int nYears);
	/// Releases dynamically allocated memory.
	void Close();
	/// Copies all data for gridlist to memory
	void CopyFromTimeDataD(TimeDataD& Data, ListArray_id<Coord>& gridlistX);
	/// Loads data for a certain coordinate. Returns false if coordinate not found.
	bool Load(Coord c);
	/// Returns a single data value for a certain year and data column
	double Get(int calender_year, int column) const;		// Returns a single value.
	/// Returns a single data value for column with header string name. Returns -999 if name not found.
	double Get(int calender_year, const char* name) const;

	/// Returns the first year in the input data
	int GetFirstyear() {return firstyear;}
	/// Returns true if data for requested coordinates are found, false if not.
	bool isloaded() const { return loaded;}
	/// Sets spacial resolution
	void SetSpacialResolution(double resolution) {spatial_resolution = resolution;}
};

} // namespace InData

#endif//INDATA_H
