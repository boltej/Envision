///////////////////////////////////////////////////////////////////////////////////////
/// \file guessserializer.cpp
/// \brief Classes which handle (de)serializing LPJ-GUESS state
///
/// $Date: 2014-06-23 15:50:25 +0200 (Mon, 23 Jun 2014) $
///
///////////////////////////////////////////////////////////////////////////////////////

#include "config.h"
#include "guessserializer.h"

#include "archive.h"
#include "partitionedmapserializer.h"
#include "driver.h"

///////////////////////////////////////////////////////////////////////////////////////
// Functors used by PartidionedMap(De)serializer to read/write coordinates
// and grid cells.
//
// Pretty much all of Guess(De)serializer is implemented by 
// PartitionedMap(De)serializer. These functors supply the domain specific
// knowledge about how and what we're serializing (coordinates and gridcells,
// of which PartitionedMap(De)serializer knows nothing).

class CoordSerializer {
public:
	void operator()(std::ostream& os, const std::pair<double, double>& coord) {
		os.write((char*)(&coord.first), sizeof(coord.first));
		os.write((char*)(&coord.second), sizeof(coord.second));
	}
};

class GridcellSerializer {
public:
	void operator()(std::ostream& os, const Gridcell& gridcell) {
		ArchiveOutStream aos(os);
		const_cast<Gridcell&>(gridcell).serialize(aos);
	}
};

class CoordDeserializer {
public:
	void operator()(std::istream& is, std::pair<double, double>& coord) {
		double a,b;
		is.read((char*)&a, sizeof(double));
		is.read((char*)&b, sizeof(double));
		coord = std::make_pair(a,b);
	}
};

class GridcellDeserializer {
public:
	void operator()(std::istream& is, Gridcell& gridcell) {
		ArchiveInStream ais(is);
		gridcell.serialize(ais);
	}
};


///////////////////////////////////////////////////////////////////////////////////////
// Functions for dealing with the meta data file
// 

namespace {

std::string meta_file_path(const char* directory) {
	return std::string(directory)+"/meta.bin";
}

/// Creates the meta data file
/** The meta data file contains information about the simulation
 *  for which we're saving the state, so that we can do some
 *  basic checking when we restart and fail if for instance
 *  new instruction file has different PFTs
 */
void create_meta_data(const char* directory, int num_processes) {

	// Create the file
	std::ofstream file(meta_file_path(directory).c_str(),
	                   std::ios::binary | std::ios::trunc);

	if (file.fail()) {
		fail("Failed to open meta data file for writing");
	}

	// Write number of processes involved,
	// we need to know this when we restart so we know how many
	// state files to try and open
	file.write((const char*)&num_processes, sizeof(num_processes));

	// Write out the vegetation mode and number of PFTs
	file.write((const char*)&vegmode, sizeof(vegmode));
	file.write((const char*)&npft, sizeof(npft));

	// Write out the names of the PFTs
	for (int i = 0; i < npft; i++) {
		xtring name = pftlist[i].name;
		unsigned long length = name.len();
		file.write((const char*)&length, sizeof(length));
		file.write(name, name.len());
	}
}

/// Reads in the meta data and checks it
/** This is only some basic checking, there are probably
 *  a lot of other things that are unwise to change
 *  before restarting from state files.
 */
void verify_meta_data(const char* directory, int& num_processes) {

	// Open the file
	std::ifstream file(meta_file_path(directory).c_str(),
	                   std::ios::binary | std::ios::in);

	if (file.fail()) {
		fail("Failed to open meta data file for reading");
	}

	// Read number of processes involved in the old simulation
	// (not necessarily the same number as in the current job)
	file.read((char*)&num_processes, sizeof(num_processes));

	// Verify vegetation mode
	vegmodetype vegmode_from_file;
	file.read((char*)&vegmode_from_file, sizeof(vegmode_from_file));

	if (vegmode != vegmode_from_file) {
		fail("State file has incompatible vegetation mode");
	}

	// Verify that the number of PFTs is the same
	int npft_from_file;
	file.read((char*)&npft_from_file, sizeof(npft_from_file));

	if (npft != npft_from_file) {
		fail("State file has different number of PFTs");
	}

	// Verify that the PFTs have the same names
	for (int i = 0; i < npft; i++) {

		const size_t PFT_NAME_MAX_SIZE = 256;

		unsigned long length;
		file.read((char*)&length, sizeof(length));

		if (length > PFT_NAME_MAX_SIZE) {
			fail("PFT name too long");
		}

		char buffer[PFT_NAME_MAX_SIZE+1];
		file.read(buffer, length);
		buffer[length] = '\0';

		if (xtring(buffer) != pftlist[i].name) {
			fail("PFT list changed, expected %s, got %s", 
			     (char*)pftlist[i].name,
			     buffer);
		}
	}
}

}

///////////////////////////////////////////////////////////////////////////////////////
// GuessSerializer
//

// Contains members of GuessSerializer which we don't want in the header
struct GuessSerializer::Impl {

	Impl(const char* directory, int my_rank) 
		: pms(directory, my_rank, GridcellSerializer(), CoordSerializer()) {
	}

	PartitionedMapSerializer<Gridcell,
	                         std::pair<double, double>,
	                         GridcellSerializer,
	                         CoordSerializer> pms;
};


GuessSerializer::GuessSerializer(const char* directory, int my_rank, int num_processes) {
	try {
		pimpl = new Impl(directory, my_rank);

		// In a parallel job, only the first process creates the meta data
		if (my_rank == 0) {
			create_meta_data(directory, num_processes);
		}
	}
	catch (const PartitionedMapSerializerError& e) {
		fail(e.what());
	}
}

GuessSerializer::~GuessSerializer() {
	delete pimpl;
}

void GuessSerializer::serialize_gridcell(const Gridcell& gridcell) {
	try {
		pimpl->pms.serialize_element(std::make_pair(gridcell.get_lon(), gridcell.get_lat()),
		                             gridcell);
	}
	catch (const PartitionedMapSerializerError& e) {
		fail(e.what());
	}
}

///////////////////////////////////////////////////////////////////////////////////////
// GuessDeserializer
//

// Contains members of GuessDeserializer which we don't want in the header
struct GuessDeserializer::Impl {
	Impl(const char* directory, int max_rank)
		: pmd(directory,
		      max_rank,
		      GridcellDeserializer(),
		      CoordDeserializer()) {
	}

	PartitionedMapDeserializer<Gridcell,
	                           std::pair<double,double>,
	                           GridcellDeserializer,
	                           CoordDeserializer,
	                           sizeof(double)*2> pmd;
};

GuessDeserializer::GuessDeserializer(const char* directory) {
	try {
		int num_processes;
		verify_meta_data(directory, num_processes);

		pimpl = new Impl(directory, num_processes - 1);
	}
	catch (const PartitionedMapSerializerError& e) {
		fail(e.what());
	}
}

GuessDeserializer::~GuessDeserializer() {
	delete pimpl;
}

void GuessDeserializer::deserialize_gridcell(Gridcell& gridcell) {
	try {
		pimpl->pmd.deserialize_element(std::make_pair(gridcell.get_lon(), gridcell.get_lat()),
		                               gridcell);
	}
	catch (const PartitionedMapSerializerError& e) {
		fail(e.what());
	}
}

void GuessDeserializer::deserialize_gridcells(const std::vector<Gridcell*>& gridcells) {
	try {
		std::vector<std::pair<double, double> > coords;

		for (size_t i = 0; i < gridcells.size(); i++) {
			coords.push_back(std::make_pair(gridcells[i]->get_lon(), gridcells[i]->get_lat()));
		}

		pimpl->pmd.deserialize_elements(coords, gridcells);
	}
	catch (const PartitionedMapSerializerError& e) {
		fail(e.what());
	}
}
