////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \file indata.cpp
/// \brief Classes for text input data (used mainly for landcover input).
/// File format can be either line 1:lon lat, line 2 etc.: year data-columns OR line 1: header,
/// line 2 etc.: lon lat year data-columns. For local static data, use: lon lat data-columns,
/// for global static data, use: dummy data-columns (with "static" as first word in header).
/// \author Mats Lindeskog
/// $Date: $
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "indata.h"
#include "config.h"
#include "guess.h"

using namespace InData;

/// Default value for gridlist and text input spatial resolution.
/*  Input data will be parsed for finer resolution than the default value. For coarser resolutions, raise default value.or set manually */
const double DEFAULT_SPATIAL_RESOLUTION = 0.5;

/// Write land use fraction data to memory; enables efficient usage of randomised gridlists for parallell simulations
const bool LUTOMEMORY = true;

// Mapping of input file data when LUTOMEMORY not defined
const bool MAPFILE = true;

const bool ascendinglongitudes = false;	//Not true for randomised gridlists; set to false for now

bool TimeDataD::item_has_data(char* name) {

	int column = GetColumn(name);

	if(column == -1)
		return false;
	else
		return checkdata[column];
}

bool TimeDataD::item_in_header(char* name) {

	if(GetColumn(name) == -1)
		return false;
	else
		return true;
}

void TimeDataD::CheckIfPresent(ListArray_id<Coord>& gridlist) { //Requires gutil.h

	if(checkdata) {
		delete[] checkdata;
		year = NULL;
	}

	checkdata = new bool[nColumns];
	for(int i=0;i<nColumns;i++)
		checkdata[i] = false;
	ischeckingdata = true;
	Rewind();

	if(format == GLOBAL_STATIC || format == GLOBAL_YEARLY) {

		for(int i=0;i<nYears;i++) {
			for(int j=0;j<nColumns;j++) {
				if(Get(firstyear + i, j) > 0.0)
					checkdata[j]=1;
			}
		}
		return;
	}

	gridlist.firstobj();
	while(gridlist.isobj) {
		Coord& c=gridlist.getobj();
		if(Load(c))	{
			for(int i=0; i<nYears; i++) {
				for(int j=0; j<nColumns; j++)	{
					if(Get(firstyear + i, j) > 0.0)
						checkdata[j] = true;
				}
			}
		}
		gridlist.nextobj();
	}

	gridlist.firstobj();
	rewind(ifp);
	ischeckingdata = false;
}

bool TimeDataD::GetHeader(char *cropnames[MAXRECORDS]) const {

	if(ifheader && header_arr) {
		for(int i=0; i<nColumns; i++)
			strncpy(cropnames[i], header_arr[i], MAXNAMESIZE*sizeof(char));
		return true;
	}
	else {
		return false;
	}
}

bool TimeDataD::GetHeaderFull(char *header_line) const {

	if(ifheader && header_arr) {
		if(format==LOCAL_YEARLY)
			strcpy(header_line, "   Lon\t   Lat\t  Year");
		else if(format==GLOBAL_YEARLY)
			strcpy(header_line, "    lon\t    lat");
		else if(format==LOCAL_STATIC)
			strcpy(header_line, "   year");

		for(int i=0; i<nColumns; i++) {

			char buffer[MAXNAMESIZE];
			sprintf(buffer, "\t%8s", header_arr[i]);
			strncat(header_line, buffer, strlen(buffer));
		}
		return true;
	}
	else {
		return false;
	}
}

char* TimeDataD::GetHeader(int record) const {

	if(ifheader && header_arr)
		return (char*)header_arr[record];
	else
		return 0;
}

void TimeDataD::Get(double* dataX) const {

	memcpy(dataX, data, nYears * nColumns * sizeof(double));
}

int TimeDataD::CalenderYearToPosition(int calender_year) const {

	int year = calender_year - firstyear;

	// Use first or last year's data if calender year is not within data period.
	if(year < 0)
		year = 0;
	else if(year >= nYears)
		year = nYears - 1;
	else
		year = calender_year - firstyear;

	return year;
}

void TimeDataD::Get(int calender_year, double* dataX) const {

	int yearX = CalenderYearToPosition(calender_year);

	memcpy(dataX, &data[yearX * nColumns], nColumns * sizeof(double));
}

double TimeDataD::Get(int calender_year, int column) const {

	if(memory_copy && !(format == GLOBAL_STATIC || format == GLOBAL_YEARLY))
		return memory_copy->Get(calender_year, column);

	int yearX = CalenderYearToPosition(calender_year);

	if(column >= nColumns)
		return 0.0;

	return data[nColumns * yearX + column];
}

double TimeDataD::Get(int calender_year, const char* name) const {

	if(memory_copy && !(format == GLOBAL_STATIC || format == GLOBAL_YEARLY))
		return memory_copy->Get(calender_year, name);

	int column = -1;

	for(int i=0; i<nColumns; i++) {

		if(!strcmp(name, header_arr[i])) {
			column = i;
			break;
		}
	}

	if(column == -1) {

		if(calender_year == firstyear)
		printf("WARNING: Value for %s not found in %s.\n", name, fileName);
		return NOTFOUND;
	}
	else {
		return Get(calender_year, column);
	}
}

int TimeDataD::GetColumn(const char* name) const {

	int column = -1;

	for(int i=0; i<nColumns; i++) {

		if(!strcmp(name, header_arr[i])) {
			column = i;
			break;
		}
	}

	if(column == -1) {
		printf("WARNING: Data for %s not found in %s.\n", name, fileName);
		return -1;
	}
	else {
		return column;
	}
}

bool TimeDataD::Open(const char* name) {

	int format_parsed = EMPTY;

	if(ifp) {
		fclose(ifp);
		ifp = NULL;
	}
	if(fileName) {
		delete []fileName;
		fileName = NULL;
	}

	if(name)
		ifp = fopen(name, "r");
	else
		return false;

	if(ifp)	{

		fileName = new char[strlen(name) + 1];

		if(!fileName) {
			printf("Cannot allocate memory for file name string !\n");
			return false;
		}
		else {
			strcpy(fileName,name);
		}

		format_parsed = ParseFormat();

		if(format != format_parsed) {	// Checks format (sets it if header), sets nColumns, ifheader and header_arr[]
			printf("Wrong format in file %s (failing ParseFormat()!\n", name);
			return false;
		}
		else if(format == GLOBAL_YEARLY || format == LOCAL_YEARLY) {

			nYears = ParseNYears();	// Parse numbers of years in input file
			if(nYears == 0)	{
				printf("Wrong format in file %s (nYears=0)!\n", name);
				return false;
			}
		}
		else if(format == GLOBAL_STATIC || format == LOCAL_STATIC) {
			nYears = 1;
		}
		else if(format == EMPTY) {	//should be set by now
			printf("Please set data format at initialization !\n");
			return false;
		}

		if(!Allocate())	{			//Allocate memory for dynamic data
			printf("Could not allocate memory for data from file %s!\n", name);
			return false;
		}
		// Load global data.
		if(format == GLOBAL_STATIC || format == GLOBAL_YEARLY) {
			Load();
		}
		unity_data = ParseNormalisation();
		spatial_resolution = ParseSpatialResolution();
	}
	else {
		printf("TimeDataD::Open: File %s could not be opened for input !\n\n", name);
		return false;
	}

	return true;
}

bool TimeDataD::ParseNormalisation() {

	bool unity_data = true;
	int cell = 0;
	double sum = 0.0;
	const int maxnsample = 200;
	int nsample = min(maxnsample, GetNCells());

	while(LoadNext() && cell < nsample) {
		for(int y=0;y<nYears;y++) {
			sum = 0.0;
			for(int i=0;i<nColumns;i++) {
				sum += Get(y + firstyear, i);
			}
			if(sum > 0.0 && (sum < 0.99 || sum > 1.01))
				unity_data = false;
		}
		cell++;
	}
	Rewind();

	return unity_data;
}

bool TimeDataD::NormalisedData() {

	return unity_data;
}

void TimeDataD::CreateFileMap() {

	if(format == GLOBAL_STATIC || format == GLOBAL_YEARLY)
		return;

	long int pos;
	int i = 0;
	nCells = GetNCells();

	filemap = new CoordPos[nCells];

	char mapname[300];
	strcpy(mapname, fileName);
	strcat(mapname, ".map.bin");
	FILE *ifp_map = fopen(mapname,"rb");
	long lSize;
	if(ifp_map) {
		fseek(ifp_map, 0 ,SEEK_END);
		lSize = ftell(ifp_map);
		rewind(ifp_map);
		if(lSize != nCells * sizeof(CoordPos)) {
			dprintf("Text data map file format is not up to date. New mapping started.\n");
			fclose(ifp_map);
			ifp_map = NULL;
		}
	}

	if(!ifp_map) {
		rewind(ifp);
		while(LoadNext(&pos) && i < nCells) {
			Coord c = GetCoord();
			filemap[i].lon = c.lon;
			filemap[i].lat = c.lat;
			filemap[i].pos = pos;
			i++;
		}
		rewind(ifp);

		// Save the file map to file
		if(ifp_map)
			fclose(ifp_map);
		FILE *ofp_map = fopen(mapname,"wb");
		if(!ofp_map)
			fail("File could not be opened for output, quitting\n");
		fwrite(filemap, sizeof(CoordPos), nCells, ofp_map);
		fclose(ofp_map);
	}
	else {
		// File is already mapped, read map from file.
		fread(filemap, sizeof(CoordPos), nCells, ifp_map);
		fclose(ifp_map);
	}
}

bool TimeDataD::Open(const char* name, ListArray_id<Coord>& gridlist, double gridlist_offset) {

	if(Open(name)) {

		SetOffset(gridlist_offset);

		if(format == GLOBAL_STATIC || format == GLOBAL_YEARLY) {
		}
		else if (LUTOMEMORY) {
			CopyToMemory(gridlist.nobj, gridlist);
		}
		else if (MAPFILE) {
			CreateFileMap();
		}
		return true;
	}
	else {
		return false;
	}
}

fileformat TimeDataD::ParseFormat() {
	//Checks format, sets nColumns, ifheader and header_arr[].
	//Desired format must be set beforehand by program at initiation of class TimeDataD objects (if no header) !

	char line[MAXLINE], *p=NULL, s1[MAXRECORDS][MAXNAMESIZE]={'\0'}, s2[MAXRECORDS][MAXNAMESIZE]={'\0'};
	int count1 = 0, count2 = 0, offset = 0;
	fileformat format_local = EMPTY;
	float d[MAXRECORDS] = {0.0};

	//First line:
	do {
		if(fgets(line,sizeof(line),ifp)) {

			p = strtok(line, "\t\n ");

			if(!p)
				continue;

			strncpy(s1[count1], p, MAXNAMESIZE-1);
			count1++;
			do {
				p = strtok(NULL, "\t\n ");
				if(p) {
					strncpy(s1[count1], p, MAXNAMESIZE-1);
					count1++;
				}
			}
			while(p);

			p = NULL;
		}
		else {
			return EMPTY;
		}
	}
	while(!(count1 > 0));

	for(int q=0;q<count1;q++)
		header_arr[q]=new char[MAXNAMESIZE];

	if(!strcmp(s1[0], "lon") || !strcmp(s1[0], "Lon") || !strcmp(s1[0], "LON")) {

		if(!strcmp(s1[2], "year") || !strcmp(s1[2], "Year")) {
			format_local = LOCAL_YEARLY;
			offset=2;
			for(int i=3; i<count1; i++)
				strncpy(header_arr[i-3],s1[i], MAXNAMESIZE-1);
		}
		else {
			format_local=LOCAL_STATIC;
			for(int i=2; i<count1; i++)
				strncpy(header_arr[i-2],s1[i], MAXNAMESIZE-1);
		}
	}
	else if(!strcmp(s1[0], "year") || !strcmp(s1[0], "Year")) {
			format_local = GLOBAL_YEARLY;
			for(int i=1; i<count1; i++)
				strncpy(header_arr[i-1],s1[i], MAXNAMESIZE-1);
	}
	else if(!strcmp(s1[0], "static")) {
			format_local=GLOBAL_STATIC;
			for(int i=1; i<count1; i++)
				strncpy(header_arr[i-1],s1[i], MAXNAMESIZE-1);
	}
	else {
		ifheader = false;
	}

	//Second line:
	do {

		if(fgets(line,sizeof(line),ifp)) {

			p = strtok(line, "\t\n ");

			if(!p)
				continue;

			strncpy(s2[count2], p, 9);
			count2++;
			do {
				p = strtok(NULL, "\t\n ");
				if(p) {
					strncpy(s2[count2], p, 9);
					count2++;
				}
			}
			while(p);
		}
		else {
			return EMPTY;
		}
	}
	while(!(count2>0));

	rewind(ifp);

	if(format==EMPTY) {
		if(ifheader)
			format = format_local;
		else
			printf("Please set data format at initialization !\n");
	}

	switch(format) {

	case GLOBAL_YEARLY:
		if(format_local==GLOBAL_YEARLY || count1>1 && count1==count2) {
			nColumns = count2 - 1;
			return GLOBAL_YEARLY;
		}
		else {
			printf("Format in input file is incompatible with GLOBAL_YEARLY flag\n");
			return EMPTY;
		}
		break;
	case LOCAL_STATIC:
		if(format_local == LOCAL_STATIC || count1 > 2 && count1 == count2) {
			nColumns=count2-2;
			return LOCAL_STATIC;
		}
		else {
			printf("Format in input file is incompatible with LOCAL_STATIC flag\n");
			return EMPTY;
		}
		break;
	case LOCAL_YEARLY:
		if(format_local == LOCAL_YEARLY || count1 == 2 && count2 > 1) {
			nColumns = count2 - 1 - offset;
			return LOCAL_YEARLY;
		}
		else {
			printf("Format in input file is incompatible with LOCAL_YEARLY flag\n");
			return EMPTY;
		}
		break;
	case GLOBAL_STATIC:
		if(format_local == GLOBAL_STATIC || count1 > 1 && count1 == count2) {
			nColumns = count2 - 1;
			return GLOBAL_STATIC;
		}
	default:
		printf("Format is not set correctly in file %s !\n", fileName);
		return EMPTY;
	}
}

int TimeDataD::ParseNYears() {

	int n_yearsX = 0;

	switch(format) {

	case GLOBAL_YEARLY:
		n_yearsX = ParseNYearsGlobal();
		break;
	case LOCAL_YEARLY:
		n_yearsX = ParseNYearsLocal();
		break;
	default:
		printf("Format in is uncorrectly set by program for file %s !\n", fileName);
		return 0;
	}

	return n_yearsX;
}

double TimeDataD::ParseSpatialResolution() {

	double precision = 100;
	double dif_lon;
	double dif_lat;
	const int maxnsample = 200;
	int nsample = min(maxnsample, GetNCells());

	Coord cvect[maxnsample];

	for (int i=0; i<nsample; i++) {
		LoadNext();
		Coord c = GetCoord();
		cvect[i].lon = c.lon;
		cvect[i].lat = c.lat;
	}
	for (int i=0; i<nsample; i++) {
		for (int j=0; j<nsample; j++) {

			dif_lon = fabs(cvect[i].lon - cvect[j].lon);
			dif_lat = fabs(cvect[i].lat - cvect[j].lat);
			if(largerthanzero(dif_lon, -12))
				precision = min(precision, dif_lon);
			if(largerthanzero(dif_lat, -12))
				precision = min(precision, dif_lat);
		}
	}
	Rewind();
	return min(precision, spatial_resolution);
}

int TimeDataD::GetNCells() {

	if(!nCells)
		ParseNCells();

	return nCells;
}

void TimeDataD::ParseNCells() {

	if(format == GLOBAL_STATIC || format == GLOBAL_YEARLY) {
		nCells = 1;
		return;
	}

	float d1;
	long int oldpos;
	int i = 0, count = 0;
	char line[MAXLINE];
	bool error = false;

	oldpos=ftell(ifp);
	if(oldpos!=0)
		rewind(ifp);

	if(ifheader)
		fgets(line,sizeof(line), ifp);	//ignore header line

	while(!feof(ifp)) {

		line[0] = 0;

		fgets(line,sizeof(line), ifp);
		count=sscanf(line,"%f", &d1);

		if(count > 0)
			i++;
	}

	if(ifheader) {
		nCells = i / nYears;
		if(i % nYears)
			error = true;
	}
	else {
		nCells = i / (nYears + 1);
		if(i % (nYears + 1))
			error = true;
	}

	if(error)
		dprintf("Unexpected number of lines ! No.lines=%d, No.cells=%d, No.years=%d\n", i, nCells, nYears);

	fseek(ifp, oldpos, 0);
}

int TimeDataD::ParseNYearsLocal() {

	int i = 0, count1 = 0, prevLine = 0, nyears1 = 0, nyears2 = 0, n = 0;
	char line[MAXLINE];
	bool new_coord = false;
	float d1 = 0,d2 = 0,d3 = 0, d1_prevLine = 0, d2_prevLine = 0;

	for(i=0; i<MAXLINESPARSE && !feof(ifp);) {

		if(fgets(line,sizeof(line), ifp)) {

			count1 = sscanf(line,"%f%f%f", &d1, &d2, &d3);		// does not count header strings
			if(count1 > 0) {

				if(ifheader && (d1 != d1_prevLine || d2 != d2_prevLine)) {	// First line of new coordinate
					nyears2 = i - prevLine;
					new_coord = true;
					firstyear = (int)d3;
				}
				else if(count1 == 2 && d1 <= 180.0) {
					nyears2 = i - prevLine - 1;
					new_coord = true;
				}

				if(new_coord) {

					if((nyears1 != nyears2) && n > 1) {
						printf("FORMAT ERROR in input file %s !\n", fileName);
						return 0;
					}
					nyears1 = nyears2;		//NB. not set if input file has data for only one coordinate !
					prevLine = i;
					n++;
					new_coord = false;
				}
				else if(!ifheader && i == prevLine + 1) {
					firstyear = (int)d1;
				}

				i++;
				d1_prevLine = d1;
				d2_prevLine = d2;
			}
		}
	}

	if(feof(ifp)) {	// Last cell

		if(ifheader)
			nyears2 = i - prevLine;
		else
			nyears2 = i - prevLine - 1;
		if((nyears1 != nyears2) && n > 1) {
			printf("FORMAT ERROR in input file %s !\n", fileName);
			return 0;
		}
	}

	rewind(ifp);
	return nyears2;
}

int TimeDataD::ParseNYearsGlobal() {

	int count = 0, nyears = 0;
	char line[MAXLINE];
	float d1 = 0,d2 = 0,d3 = 0;

	if(ifheader)
		fgets(line,sizeof(line),ifp);

	while(!feof(ifp)) {

		if(fgets(line,sizeof(line),ifp)) {

			count=sscanf(line,"%f%f%f", &d1, &d2 ,&d3);
			if(count > 0) {

				if(count >= 2) {
					nyears++;
					if(nyears == 1)
						firstyear = (int)d1;
				}
				else {
					printf("FORMAT ERROR in input file %s !\n", fileName);
					nyears = 0;
					break;
				}
			}
		}
	}
	rewind(ifp);
	return nyears;
}

bool TimeDataD::Allocate() {	// Allocates memory for dynamic data: format & nYears must be set before !

	if(year) {
		delete[] year;
		year = NULL;
	}
	if(data) {
		delete[] data;
		data = NULL;
	}

	switch(format) {

	case EMPTY:
		break;
	case GLOBAL_STATIC:
		year = new int;
		data = new double[nColumns];
		if(year)
			*year = 0;
		if(data)
			*data = 0;
		break;
	case GLOBAL_YEARLY:
		year = new int[nYears];
		data = new double[nColumns * nYears];
		if(year) {
			for(int i=0;i<nYears;i++)
				year[i] = 0;
		}
		if(data) {
			for(int i=0;i<nColumns*nYears;i++)
				data[i] = 0.0;
		}
		break;
	case LOCAL_STATIC:
		year = new int;
		data = new double[nColumns];
		if(year)
			*year = 0;
		if(data)
			for(int j=0;j<nColumns;j++)
				data[j] = 0;
		break;
	case LOCAL_YEARLY:
		year = new int[nYears];
		data = new double[nColumns * nYears];
		if(year) {
			for(int i=0;i<nYears;i++)
				year[i] = 0;
		}
		if(data) {
			for(int i=0;i<nColumns*nYears;i++)
				data[i] = 0.0;
		}
		break;
	default:
		;
	}
	if(year && data)
		return true;
	else
		return false;
}

void TimeDataD::SetOffset(double gridlist_offset) {
	if(gridlist_offset)
		offset = gridlist_offset - spatial_resolution / 2.0;
}

bool TimeDataD::Load() {	// for GLOBAL_YEARLY and GLOBAL_STATIC data

	int i = 0, count = 0, yearX = 0, yearX_previous, k = 0;
	char line[MAXLINE], *p = NULL;
	double d1 = 0.0;
	double d[MAXRECORDS] = {0.0};
	float extra = 0.0;
	bool error = false;

	if(ifp) {

		if(format == GLOBAL_YEARLY) {

			if(year) {
				for(int i=0;i<nYears;i++)
					year[i] = 0;
			}
			if(data) {
				for(int i=0;i<nColumns*nYears;i++)
					data[i] = 0.0;
			}

			yearX_previous = firstyear - 1;

			if(ifheader)
				fgets(line, sizeof(line), ifp);

			for(i=0; i<nYears;) {

				k = 0;
				if(fgets(line, sizeof(line), ifp)) {

					for(int j=0;j<nColumns;j++)
						d[j] = 0.0;
					count = 0;

					p=strtok(line, "\t\n ");	// year
					if(!p)
						continue;
					sscanf(p, "%d", &yearX);

					if(yearX != yearX_previous + 1)	{
						printf("FORMAT ERROR in input file %s: Load(). Wrong year in data file ! Missing line ?\n", fileName);
						error = true;
						break;
					}
					else {
						yearX_previous = yearX;
					}

					do {
						p = strtok(NULL, "\t\n ");
						if(p)
							count += sscanf(p, "%lf", &d[k]);
						k++;
					}
					while(p);

					if(count > 0) {

						if(count == nColumns) {
							year[i] = yearX;
							for(int j=0; j<nColumns; j++)
								data[nColumns * i + j] = d[j];
						}
						else {
							printf("FORMAT ERROR in input file %s: Load(), count!=%d\n", fileName, i+1);
							error = true;
						}
						i++;
					}
				}
				else {
					printf("An ERROR occurred reading file %s\n", fileName);
					error = true;
				}
			}
		}
		else if(format == GLOBAL_STATIC) {

			if(fgets(line, sizeof(line), ifp)) {

				if(ifheader) {

					if(fgets(line, sizeof(line), ifp)) {
						p = strtok(line," \t");	//"static"
					}
					else {
						printf("An ERROR occurred reading file %s\n", fileName);
						error = true;
					}
				}
				do {
					p = strtok(NULL, "\t\n ");
					if(p)
						count += sscanf(p, "%lf", &d[k]);
					k++;
				}
				while(p);

				if(count == nColumns) {
					for(i=0; i<nColumns; i++)
						data[i] = d[i];
				}
				else {
					printf("FORMAT ERROR in input file %sf: Load(), count!=%d\n", fileName, nColumns+1);
					error = true;
				}
			}
			else {
				printf("An ERROR occurred reading file %s\n", fileName);
				error = true;
			}
		}
		else {
			printf("Wrong usage of Load(void)\n");
			error = true;
		}
	}
	else {
		printf("Cannot load from unopened file !\n");
		error = true;
	}

	if(ifp) {
		fclose(ifp);
		ifp = NULL;
	}

	if(error) {
		loaded = false;
		return false;
	}
	else {
		loaded = true;
		return true;
	}
}

bool TimeDataD::LoadFromMap(Coord c) {

	double searchradius = min(spatial_resolution / 2.0, MAX_SEARCHRADIUS);
	double min_dist = 1000;
	long int found_pos = -1;
	int found_i;

	for(int i=0; i<nCells; i++) {

		double dif_lon = fabs(filemap[i].lon - c.lon);
		double dif_lat = fabs(filemap[i].lat - c.lat);
		if(dif_lon <= searchradius && dif_lat <= searchradius) {
			if(min_dist > (dif_lon + dif_lat)) {
				min_dist = dif_lon + dif_lat;
				found_pos = filemap[i].pos;
				found_i = i;
			}
		}
	}
	if(found_pos > -1) {
		SetPosition(found_pos);
		LoadNext();
		if(currentStand.lon != filemap[found_i].lon || currentStand.lat != filemap[found_i].lat)
			fail("Error in saved file map for %s. Delete map.bin file and retry\n", fileName);
		loaded = true;
	}
	else {
		loaded = false;
	}
	return loaded;
}

bool TimeDataD::Load(Coord c) {

	if(format == GLOBAL_STATIC || format == GLOBAL_YEARLY) {
		loaded = true;
		return true;
	}

	if(offset) {
		c.lon += offset;
		c.lat += offset;
	}
	if(memory_copy)
		return memory_copy->Load(c);
	else if(filemap)
		return LoadFromMap(c);

	char line[MAXLINE], *p=NULL;
	int i = 0, j = 0, k = 0, count1 = 0, nyears = 0, yearX = 0, yearX_previous;
	float lonX = 0.0, latX = 0.0;
	double d[MAXRECORDS] = {0.0};
	bool error = false;

	if(ifp) {

		if(format==LOCAL_YEARLY) {

			if(FindRecord(c)) {

				if(year) {
					for(int i=0;i<nYears;i++)
						year[i] = 0;
				}
				if(data) {
					for(int i=0;i<nColumns*nYears;i++)
						data[i] = 0.0;
				}
				yearX_previous = firstyear - 1;

				while(i < nYears && yearX < firstyear + nYears - 1) {

					k = 0;
					if(fgets(line, sizeof(line), ifp)) {

						for(int q=0;q<nColumns;q++)
							d[q] = 0.0;;
						count1 = 0;

						if(ifheader) {

							p = strtok(line," \t");	//lon
							sscanf(p, "%f", &lonX);
							p = strtok(NULL, " \t");	//lat
							sscanf(p, "%f", &latX);
							p = strtok(NULL, " \t");	//year

							if(fabs(lonX - c.lon) > spatial_resolution / 2.0 || fabs(latX - c.lat) > spatial_resolution / 2.0) {
								printf("FORMAT ERROR in input file %s for stand at Coordinate %.2f,%.2f: Load(). Wrong coordinates in data file !\n", fileName, c.lon, c.lat);
								printf("Make sure file has correct DOS/Unix text format\n");				
								error = true;
								break;
							}
							else {
								currentStand.lon = lonX;
								currentStand.lat = latX;
							}
						}
						else {
							p = strtok(line, "\t\n ");	//year
						}

						if(!p)
							continue;

						sscanf(p, "%d", &yearX);

						if(yearX != yearX_previous + 1) {
							printf("FORMAT ERROR in input file %s for stand at Coordinate %.2f,%.2f: Load(). Wrong year in data file ! Missing line ?\n", fileName, c.lon, c.lat);
							error=1;
						}

						yearX_previous = yearX;

						do {

							p = strtok(NULL, "\t\n ");
							if(p)
								count1 += sscanf(p, "%lf", &d[k]);
							k++;
						}
						while(p);

						if(count1 > 0) {

							if(count1 == nColumns) {

								year[i] = yearX;
								for(j=0; j<nColumns; j++)
									data[nColumns * i + j] = d[j];
							}
							else {
								printf("FORMAT ERROR in input file %s for stand at Coordinate %.2f,%.2f: Load(), count!=%d, year %d\n", fileName, c.lon, c.lat, nColumns+1, i+1);
								error = true;
								break;
							}
							i++;
						}
					}
					else {
						printf("An ERROR occurred reading file %s\n", fileName);
						error = true;
						break;
					}
				}
			}
			else {
				printf("COULD NOT FIND DATA for %.2f, %.2f in file %s\n", c.lon, c.lat, fileName);
				error = true;
			}
		}
		else if(format == LOCAL_STATIC) {

			if(FindRecord(c)) {

				if(data) {
					for(int i=0;i<nColumns;i++)
						data[i] = 0.0;;
				}

				if(fgets(line, sizeof(line), ifp)) {

					if(d) {
						for(int i=0;i<nColumns;i++)
							d[i] = 0.0;;
					}
					p=strtok(line," \t");	//lon
					sscanf(p, "%f", &lonX);
					p=strtok(NULL, " \t");	//lat
					sscanf(p, "%f", &latX);

					currentStand.lon = lonX;
					currentStand.lat = latX;

					do {

						p = strtok(NULL, "\t\n ");
						if(p)
							count1 += sscanf(p, "%lf", &d[k]);
						k++;
					}
					while(p);

					if(count1 > 0) {

						if(count1 == nColumns) {
							for(j=0; j<nColumns; j++)
								data[j] = d[j];
						}
						else {
							printf("FORMAT ERROR in input file %s for stand at Coordinate %.2f,%.2f: Load(), count!=%d, year %d\n", fileName, c.lon, c.lat, nColumns+1, i+1);
							error=1;
						}
					}
				}
				else {
					printf("An ERROR occurred reading file %s\n", fileName);
					error = true;
				}
			}
			else {
				printf("COULD NOT FIND DATA for %.2f, %.2f in file %s\n",c.lon,c.lat,fileName);
				error = true;
			}
		}
		else {
			printf("Wrong usage of Load(Coord)\n");
			error = true;
		}
	}
	else {
		printf("Cannot load from unopened file !\n");
		error = true;
	}

	if(error) {
		loaded = false;
		return false;
	}
	else {
		loaded = true;
		return true;
	}
}

bool TimeDataD::isloaded() {

	if(memory_copy)
		return memory_copy->isloaded();
	else
		return loaded;
}


bool TimeDataD::LoadNext(long int *pos) {
	// To be called after ParseFormat(), ParseNYears() and Allocate()
	// Only implemented for LOCAL_YEARLY and LOCAL_STATIC
	// Needs to be modified to handle missing lines in data files with header ! (see Load)

	if(format == GLOBAL_STATIC || format == GLOBAL_YEARLY) {
		return 1;
	}

	char line[MAXLINE], *p=NULL;
	double d1, d2, d3, d[MAXRECORDS]={0.0};
	bool error = false, firstyear = true;
	long int fpos;

	if(ifp && !feof(ifp)) {

		if(format == LOCAL_YEARLY) {

			if(year) {
				for(int i=0;i<nYears;i++)
					year[i] = 0;
			}
			if(data) {
				for(int i=0;i<nColumns*nYears;i++)
					data[i] = 0.0;
			}
			if(ifheader) {

				fpos = ftell(ifp);
				if(fpos == 0) {
					fgets(line,sizeof(line), ifp);	//ignore header line
					fpos = ftell(ifp);
				}
				if(pos)
					*pos = fpos;
			}

			int count = 0;

			if(fgets(line,sizeof(line), ifp)) {

				count = sscanf(line,"%lf%lf%lf", &d1, &d2, &d3);
				if(count > 0)	{ // Avoid blank lines at the end of the file

					if(count == 2 || count > 2 && (format == LOCAL_STATIC || ifheader)) {
						currentStand.lon = d1;
						currentStand.lat = d2;
					}
					else {
						printf("FORMAT ERROR in input file %s: LoadNext(), count!=2\n", fileName);
						error = true;
					}
				}
				else {
					printf("WARNING: blank line in file %s: LoadNext(), count==0\n", fileName);
				}
			}
			else {
				error = true;
			}

			for(int i=0; i<nYears && error==false && count>0;) {

				if(ifheader && firstyear)
					firstyear = false;
				else
					fgets(line, sizeof(line), ifp);

				if(line) {

					int k = 0;
					int count1 = 0;
					int yearX = 0;
					for(int q=0;q<nColumns;q++)
						d[q] = 0.0;;

					if(ifheader) {

						p = strtok(line," \t");		//lon
						p = strtok(NULL, " \t");	//lat
						p = strtok(NULL, " \t");	//year
					}
					else {
						p = strtok(line, "\t\n ");	//year
					}
					if(!p)
						continue;
					sscanf(p, "%d", &yearX);

					do {
						p = strtok(NULL, "\t\n ");
						if(p)
							count1 += sscanf(p, "%lf", &d[k]);
						k++;
					}
					while(p);

					if(count1 > 0) {

						if(count1 == nColumns) {
							year[i] = yearX;
							for(int j=0; j<nColumns; j++)
								data[nColumns * i + j] = d[j];
						}
						else {
							printf("FORMAT ERROR in input file %s: LoadNext(), count!=%d, year %d\n", fileName, nColumns+1, i+1);
							printf("Make sure file has correct DOS/Unix text format\n");
							error = true;
							break;
						}
						i++;
					}
				}
				else {
					printf("An ERROR occurred reading file %s\n", fileName);
					error=true;
					break;
				}
			}
		}
		else if(format == LOCAL_STATIC) {

			if(data) {
				for(int i=0;i<nColumns*nYears;i++)
					data[i] = 0.0;
			}

			if(ifheader) {
				fpos = ftell(ifp);
				if(fpos == 0) {
					fgets(line,sizeof(line), ifp);	//ignore header line
					fpos = ftell(ifp);
				}
				if(pos)
					*pos = fpos;
			}

			int count = 0;

			if(fgets(line,sizeof(line), ifp)) {

				count = sscanf(line,"%lf%lf%lf", &d1, &d2, &d3);
				if(count > 0)	{ // Avoid blank lines at the end of the file

					if(count == 2 || count > 2 && (format == LOCAL_STATIC || ifheader)) {
						currentStand.lon = d1;
						currentStand.lat = d2;
					}
					else {
						printf("FORMAT ERROR in input file %s: LoadNext(), count!=2\n", fileName);
						error = true;
					}
				}
				else {
					printf("WARNING: blank line in file %s: LoadNext(), count==0\n", fileName);
				}
			}
			else {
				error = true;
			}

			for(int i=0; i<nYears && error==false && count>0;) {

				int k = 0;
				int count1 = 0;

				if(ifheader && firstyear)
					firstyear = false;
				else
					fgets(line, sizeof(line), ifp);

				if(line) {

					for(int q=0;q<nColumns;q++)
						d[q] = 0.0;;

					if(ifheader) {
						p = strtok(line," \t");	//lon
						p = strtok(NULL, " \t");	//lat
					}

					if(!p)
						continue;

					do {
						p = strtok(NULL, "\t\n ");
						if(p)
							count1 += sscanf(p, "%lf", &d[k]);
						k++;
					}
					while(p);

					if(count1 > 0) {

						if(count1 == nColumns) {
							for(int j=0; j<nColumns; j++)
								data[nColumns * i + j] = d[j];
						}
						else {
							printf("FORMAT ERROR in input file %s: LoadNext(), count!=%d, year %d\n", fileName, nColumns+1, i+1);
							printf("Make sure file has correct DOS/Unix text format\n");
							error = true;
							break;
						}
						i++;
					}
				}
				else
				{
					printf("An ERROR occurred reading file %s\n", fileName);
					error = true;
					break;
				}
			}
		}
	}
	else {
		error = true;
	}

	if(error)
		return false;
	else
		return true;
}

bool TimeDataD::FindRecord(Coord c) const {
	//Fast version. Can not handle blank lines in some cases, will call FindRecord2() in those cases.

	int i = 0, count = 0, n = 0, lap = 0, line_no = 0;
	char line[MAXLINE], *p = NULL;
	double d1 = 0.0,d2 = 0.0,d3 = 0.0;
	bool found = false, error = false, start = true;
	long int oldpos;

	do {
		i = 0;
		start = true;

		while(!feof(ifp)) {
			if(ifheader) {
				if(!(i%nYears))
					oldpos = ftell(ifp);
			}

			if(fgets(line,sizeof(line), ifp)) {

				if(ifheader) {
					if(start == true) {
						start = false;
						if(oldpos == 0)
							continue;
					}
				}

				if(!(i%(nYears+1)) && !ifheader || !(i%nYears) && ifheader) {

					count=sscanf(line,"%lf%lf%lf", &d1, &d2, &d3);
					if(count>0)	{ // Avoid blank line at the end of the file

						if(count == 2 || count > 2 && (d3 == firstyear || format == LOCAL_STATIC) && (format == LOCAL_STATIC || ifheader)) {

							if(fabs(d1 - c.lon) <= spatial_resolution / 2.0 && fabs(d2 - c.lat) <= spatial_resolution / 2.0) {
								found = true;
								break;
							}
							else if(ascendinglongitudes && c.lon<d1) {
								dprintf("c.lon<d1; rewinding...\n");
								break;
							}
						}
						else {
							if(ifheader)
								dprintf("FORMAT ERROR in input file %s: FindRecord(), wrong firstyear, line %d\n", fileName, i);
							else
								dprintf("FORMAT ERROR in input file %s: FindRecord(), count!=2, line %d\n", fileName, i);
							error = true;
							break;
						}
					}
					else {
						dprintf("WARNING: blank line in file %s: FindRecord(), count==0\n", fileName);
						continue;
					}
				}
				i++;
			}
		}

		if(!found) {

			lap++;
			rewind(ifp);
			if(error)
				break;
		}

	} while(!found && lap<2);

	if(found && !error) {
		if(ifheader)
			fseek(ifp, oldpos, 0);
		return true;
	}
	else
		return FindRecord2(c);	//If not found, try FindRecord2()
}

bool TimeDataD::FindRecord2(Coord c) const {
	//This version should handle blank or missing lines at all positions.

	int i = 0, count = 0, n = 0, lap = 0, lastyear;
	char line[MAXLINE], *p = NULL;
	double d1 = 0.0, d2 = 0.0, d3 = 0.0;
	bool found = false, error = false, start = true;
	long int oldpos;

	lastyear = firstyear + nYears - 1;

	do {

		i = 0;
		start = true;

		while(!feof(ifp)) {

			if(ifheader) {
				if(d3==lastyear || start)
					oldpos = ftell(ifp);
			}

			if(fgets(line,sizeof(line),ifp)) {

				if(ifheader) {
					if(start==true) {
						start = false;
						if(oldpos==0)
							continue;
					}
				}

				count = sscanf(line,"%lf%lf%lf", &d1, &d2, &d3);

				if(count>0)	{ // Avoid blank lines at the end of the file

					if(count==2 || count>2 && d3==firstyear && (format==LOCAL_STATIC || ifheader)) {

						if(fabs(d1 - c.lon) <= spatial_resolution / 2.0 && fabs(d2 - c.lat) <= spatial_resolution / 2.0) {
							found=1;
							break;
						}
						else if(ascendinglongitudes && c.lon<d1) {
							dprintf("c.lon<d1; rewinding...\n");
							break;
						}
					}
				}
				else {
					dprintf("WARNING: blank line in file %s: FindRecord2(), count==0\n",fileName);
					continue;
				}
				i++;
			}
		}

		if(!found) {
			lap++;
			rewind(ifp);
		}

	} while(!found && lap<2);

	if(found && !error) {
		if(ifheader)
			fseek(ifp, oldpos, 0);
		return true;
	}
	else {
		return false;
	}
}

void TimeDataD::Output(char *name) {

	int i = 0, j = 0;
	FILE *ofp = NULL;
	static bool first_call = true;

	if(first_call)
		remove(name);

	if(format==GLOBAL_STATIC || format==GLOBAL_YEARLY)
		ofp = fopen(name, "w");
	else if(format==LOCAL_STATIC || format==LOCAL_YEARLY)
		ofp = fopen(name, "a");

	if(ifheader && header_arr && first_call) {

		switch (format) {

		case GLOBAL_STATIC:
			break;
		case GLOBAL_YEARLY:
			fprintf(ofp, "  year\t");
			break;
		case LOCAL_STATIC:
			fprintf(ofp, "   lon\t   lat\t");
			break;
		case LOCAL_YEARLY:
			fprintf(ofp, "   lon\t   lat\t  year\t");
			break;
		default:
			;
		}

		for(int i=0; i<nColumns; i++)
			fprintf(ofp, "%8s\t", header_arr[i]);
		fprintf(ofp, "\n");
		first_call = false;
	}


	switch (format) {

	case GLOBAL_STATIC:
		fprintf(ofp, "%.3lf\n", *data);
		break;
	case GLOBAL_YEARLY:
		for(i=0; i<nYears; i++)
			fprintf(ofp, "%d\t%.3lf\n", year[i], data[i]);
		break;
	case LOCAL_STATIC:
		fprintf(ofp, "%6.2f\t%6.2f",currentStand.lon, currentStand.lat);
		for(j=0; j<nColumns; j++)
			fprintf(ofp, "\t%8.3f", data[nColumns*i+j]);
		fprintf(ofp, "\n");
		break;
	case LOCAL_YEARLY:

		if(!ifheader)
			fprintf(ofp, "%8.2f\t%8.2f\n",currentStand.lon, currentStand.lat);

		for(i=0;i<nYears;i++) {
			if(ifheader)
				fprintf(ofp, "%6.2f\t%6.2f\t",currentStand.lon, currentStand.lat);

			fprintf(ofp, "%6d ", year[i]);
			for(j=0; j<nColumns; j++)
				fprintf(ofp, "\t%8.3f", data[nColumns * i + j]);
			fprintf(ofp, "\n");
		}
		break;
	default:
		;
	}
	if(ofp)
		fclose(ofp);
}

TimeDataD::TimeDataD(fileformat formatX) {

	ifp = NULL;
	fileName = NULL;
	ifheader = true;
	for(int i=0;i<MAXRECORDS;i++) {
		header_arr[i] = NULL;
	}
	currentStand.lon = 0;
	currentStand.lat = 0;
	data = NULL;
	checkdata = NULL;
	ischeckingdata = false;
	unity_data = true;

	nColumns = 0;
	nYears = 0;
	nCells = 0;
	firstyear = -1;
	year = NULL;
	format = formatX;
	memory_copy = NULL;
	filemap = NULL;
	spatial_resolution = DEFAULT_SPATIAL_RESOLUTION;
	offset = 0.0;
	loaded = false;
}

//Deconstructor
TimeDataD::~TimeDataD() {
	Close();
}

void TimeDataD::Close() {

	if(ifp)
		fclose(ifp);
	if(fileName) {
		delete []fileName;
		fileName = NULL;
	}
	if(year) {
		delete[] year;
		year = NULL;
	}
	if(data) {
		delete[] data;
		data = NULL;
	}
	if(checkdata) {
		delete []checkdata;
		checkdata = NULL;
	}
	if(memory_copy) {
		memory_copy->Close();
		delete memory_copy;
	}
	if(filemap)
		delete[] filemap;

	for(int i=0;i<MAXRECORDS;i++) {
		if(&header_arr[i]) {
			delete[] header_arr[i]; 
			header_arr[i] = NULL;
		}
	}
}

void TimeDataD::CopyToMemory(int ncells, ListArray_id<Coord>& lonlatlist) { //Requires gutil.h

	if(format == GLOBAL_STATIC)
		ncells = 1;
	memory_copy = new TimeDataDmem;
	memory_copy->Open(ncells, this->nColumns, this->nYears);
	memory_copy->CopyFromTimeDataD(*this, lonlatlist);
	Rewind();
}

int TimeDataDmem::CalenderYearToPosition(int calender_year) const {

	int yearX = calender_year - firstyear;

	// Use first or last year's data if calender year is not within data period.
	if(yearX < 0)
		yearX = 0;
	else if(yearX >= nYears)
		yearX = nYears -1;
	else
		yearX = calender_year - firstyear;

	return yearX;
}

double TimeDataDmem::Get(int calender_year, int column) const {

	int yearX = CalenderYearToPosition(calender_year);

	if(currentCell >= 0 && column < nColumns)
		return data[currentCell][yearX * nColumns + column];
	else if(nCells == 1)
		return data[0][yearX * nColumns + column];
	else
		return 0.0;
}

double TimeDataDmem::Get(int calender_year, const char* name) const {

	int column = -1;

	for(int i=0; i<nColumns; i++) {
		if(!strcmp(name, header_arr[i])) {
			column = i;
			break;
		}
	}

	if(column == -1) {
		if(calender_year == firstyear)	// firstyear set to -1 for static inputs
		printf("WARNING: Value for %s not found in input file\n", name);
		return NOTFOUND;
	}
	else {
		return Get(calender_year, column);
	}
}

bool TimeDataDmem::Load(Coord c) {

	bool error = true;
	double searchradius = min(spatial_resolution / 2.0, MAX_SEARCHRADIUS);

	//In case gridlist cell order is same as in land use files
	if(currentCell < (nCells - 1) && fabs(gridlist[currentCell+1].lon - c.lon) <= searchradius
			&& fabs(gridlist[currentCell+1].lat - c.lat) <= searchradius) {
		currentCell++;
		error = false;
	}
	else {
		for(int i=0;i<nCells;i++) {
			if(fabs(gridlist[i].lon - c.lon) <= searchradius && fabs(gridlist[i].lat - c.lat) <= searchradius) {
				currentCell = i;
				error = false;
				break;
			}
		}
	}
	if(error) {
		loaded = false;
	}
	else {
		loaded = true;
	}
	return loaded;
}
void TimeDataDmem::SetData(int index, double* dataX) {

	if(data && data[index])
		memcpy(data[index], dataX, nColumns * nYears * sizeof(double));
}

void TimeDataDmem::SetCoord(int index, Coord c) {

	gridlist[index].lon = c.lon;
	gridlist[index].lat = c.lat;
}

void TimeDataDmem::Open(int nCellsX, int nColumnsX, int nYearsX) {

	nCells = 0;
	nColumns = nColumnsX;
	nYears = nYearsX;
	gridlist = new Coord[nCellsX];
	data = new double*[nCellsX];
	for(int i=0; i<nCellsX; i++) {
		data[i] = new double[nColumns * nYears];
		if(data[i]) {
			for(int y=0;y<nColumns*nYears;y++)
				data[i][y] = 0.0;
		}
	}
}

void TimeDataDmem::Close() {

	nColumns = 0;
	nYears = 0;

	if(gridlist) {
		delete []gridlist;
		gridlist = NULL;
	}
	for(int i=0; i<nCells; i++) {
		if(data[i])
			delete[] data[i];
	}
	if(data) {
		delete[] data;
		data = NULL;
	}
	for(int i=0;i<MAXRECORDS;i++) {
		if(header_arr[i]) {
			delete[] header_arr[i]; 
			header_arr[i] = NULL;
		}
	}
	nCells = 0;
}

void TimeDataDmem::CopyFromTimeDataD(TimeDataD& Data, ListArray_id<Coord>& gridlistX) { //Requires gutil.h

	int cell_no = 0;

	for(int q=0;q<Data.GetnColumns();q++)
		header_arr[q]=new char[MAXNAMESIZE];

	if(Data.GetHeader(header_arr))
		ifheader = true;

	firstyear = Data.GetFirstyear();
	spatial_resolution = Data.GetSpacialResolution();
	double searchradius = min(spatial_resolution / 2.0, MAX_SEARCHRADIUS);
	double offset = Data.GetOffset();

	double *celldata;
	celldata = new double[Data.GetnColumns() * Data.GetnYears()];

	if(Data.GetFormat() == GLOBAL_STATIC) {
		Data.Get(celldata);
		SetData(0, celldata);
		return;
	}

	gridlistX.firstobj();

	while(Data.LoadNext() && cell_no < Data.GetNCells()) {

		Coord c =Data.GetCoord();

		double dif_lon;
		double dif_lat;
		unsigned int no = 0;
		while(no < gridlistX.nobj) {

			Coord cc = gridlistX.getobj();
			// data coord close to gridlist coord ?
			dif_lon = fabs(c.lon - (cc.lon + offset));
			dif_lat = fabs(c.lat - (cc.lat + offset));

			if(dif_lon <= searchradius && dif_lat <= searchradius) {
				bool done = false;
				double dif_lon_saved;
				double dif_lat_saved;

				for(int i=cell_no-1; i>=0;i--) {
					// has data close to the gridlist coord already been saved ?
					dif_lon_saved = fabs(gridlist[i].lon - (cc.lon + offset));
					dif_lat_saved = fabs(gridlist[i].lat - (cc.lat + offset));
					if(dif_lon_saved <= searchradius && dif_lat_saved <= searchradius) {
						// is the new data coord closer to the gridlist coord than the already saved coord is ?
						if((dif_lon_saved + dif_lat_saved) > (dif_lon + dif_lat)) {	// This part is probably not needed
							SetCoord(i, c);
							Data.Get(celldata);
							SetData(i, celldata);
						}
						done = true;
						break;	// from saved gridlist loop
					}
				}
				if(!done) {
					SetCoord(cell_no, c);
					Data.Get(celldata);
					SetData(cell_no, celldata);
					cell_no++;
					nCells++;
					break;	// from gridlist loop
				}
			}
			gridlistX.nextobj();
			no++;
			if(!gridlistX.isobj)
				gridlistX.firstobj();
		}
	}

	delete[] celldata;

	Data.register_memory_copy(this);
}

TimeDataDmem::TimeDataDmem() {

	gridlist = NULL;
	data = NULL;
	nCells = 0;
	ifheader = false;
	for(int i=0;i<MAXRECORDS;i++) {
		header_arr[i] = NULL;
	}
	currentCell = -1;
	loaded = false;
}

TimeDataDmem::~TimeDataDmem() {

	Close();
}

