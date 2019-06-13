///////////////////////////////////////////////////////////////////////////////////////
/// \file parallel.h
/// \brief Functionality for parallel computation
///
///        Currently a layer over MPI with functionality LPJ-GUESS might need,
///        but could be implemented as a layer over something else in future.
///
///        The fact that MPI might not be available on the system where LPJ-GUESS
///        was compiled is hidden by this module by simply pretending to be a
///        "parallel" run with just one process.
///
/// \author Joe Siltberg
/// $Date: 2014-06-23 15:50:25 +0200 (Mon, 23 Jun 2014) $
///
///////////////////////////////////////////////////////////////////////////////////////

#ifndef LPJ_GUESS_PARALLEL_H
#define LPJ_GUESS_PARALLEL_H

namespace GuessParallel {

/// Initializes the module
/** For instance by initializing underlying communications library.
 *
 *  \param argc Passed along from main() to underlying communications library
 *  \param argv Passed along from main() to underlying communications library
 */
void init(int& argc, char**& argv);

/// The process id of this process in the parallel run
/** Returns zero when no MPI library is available/used. */
int get_rank();

/// The number of processes involved in the parallel run
/** Returns 1 when no MPI library is available/used. */
int get_num_processes();

}

#endif // LPJ_GUESS_PARALLEL_H
