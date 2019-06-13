///////////////////////////////////////////////////////////////////////////////////////
/// \file guessserializer.h
/// \brief Classes which handle (de)serializing LPJ-GUESS state
///
/// $Date: 2014-06-23 15:50:25 +0200 (Mon, 23 Jun 2014) $
///
///////////////////////////////////////////////////////////////////////////////////////

#ifndef LPJ_GUESS_GUESS_SERIALIZER_H
#define LPJ_GUESS_GUESS_SERIALIZER_H

#include <vector>

class Gridcell;

/// Takes care of saving LPJ-GUESS state to disk
class GuessSerializer {
public:
	/// Constructor, creates state file and meta data
	/** \param directory     Where to create the files
	 *  \param my_rank       Unique integer identifying this process in a multi
	 *                       process job.
	 *  \param num_processes The number of processes involved in the job
	 */
	GuessSerializer(const char* directory, int my_rank, int num_processes);

	/// Finalizes and closes the state file
	virtual ~GuessSerializer();

	/// Adds the state for a gridcell to the state file
	void serialize_gridcell(const Gridcell& gridcell);

private:
	// Implementation hidden in cpp file
	struct Impl;
	Impl* pimpl;
};

/// Takes care of reading in LPJ-GUESS state from disk
class GuessDeserializer {
public:
	/// Constructor, does some basic checking of the meta data
	GuessDeserializer(const char* directory);

	/// Closes opened files etc.
	virtual ~GuessDeserializer();

	/// Reads in a single grid cell from the state file
	void deserialize_gridcell(Gridcell& gridcell);

	/// Reads in multiple grid cells from the state file
	/** If many grid cells should be deserialized at once, this one is better 
	 *  than many calls to deserialize_gridcell since this one will avoid 
	 *  unnecessary file seeks.
	 */
	void deserialize_gridcells(const std::vector<Gridcell*>& gridcells);

private:
	// Implementation hidden in cpp file
	struct Impl;
	Impl* pimpl;
};

#endif // LPJ_GUESS_GUESS_SERIALIZER_H
