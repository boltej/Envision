///////////////////////////////////////////////////////////////////////////////////////
/// \file parallel.cpp
/// \brief Functionality for parallel computation
///
/// \author Joe Siltberg
/// $Date: 2016-12-08 18:24:04 +0100 (Thu, 08 Dec 2016) $
///
///////////////////////////////////////////////////////////////////////////////////////

// mpi.h needs to be the first include (or specifically, before stdio.h),
// due to a bug in the MPI-2 standard (both stdio.h and the C++ MPI API
// defines SEEK_SET, SEEK_CUR and SEEK_SET(!))
#ifdef HAVE_MPI
#include <mpi.h>
#endif

#include "config.h"
#include "parallel.h"
#include "shell.h"
#include <memory>
#include <string>

namespace GuessParallel {

bool parallel = false;

#ifdef HAVE_MPI

/// A class whose only purpose is to terminate the MPI library when deleted
class FinalizeCaller {
public:
	~FinalizeCaller() {
		MPI_Finalize();
	}
};

/// The auto pointer will delete the object some time after main() is finished
std::auto_ptr<FinalizeCaller> destructor;

#endif

void init(int& argc, char**& argv) {
#ifdef HAVE_MPI

	// Only initialize MPI is the command line options explicitly
	// asks for a parallel run (with the -parallel option).
	// In most cases it wouldn't hurt to initialize MPI even if
	// it's not used, but apparently some implementations start
	// by going to the users home directory if the program isn't
	// launched by an mpiexec/mpirun launcher.

	// Unfortunately, since MPI initialization must be done before
	// we parse our options with CommandLineArguments, we need to
	// look for the -parallel option here by ourselves.
	// The file-global variable parallel is initiated = false;
	for (int i = 0; i < argc; ++i) {
		if (std::string(argv[i]) == "-parallel") {
			parallel = true;
			break;
		}
	}

	if (parallel) {
		MPI_Init(&argc, &argv);

		// Make sure the MPI_Finalize function is called at program termination
		destructor = std::auto_ptr<FinalizeCaller>(new FinalizeCaller());
	}
#endif
}

int get_rank() {
#ifdef HAVE_MPI
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	return rank;
#else
	return 0;
#endif
}

int get_num_processes() {
#ifdef HAVE_MPI
	if (parallel) {
		int size;
		MPI_Comm_size(MPI_COMM_WORLD, &size);
		return size;
	}else
		return 1;	
#else
	return 1;
#endif
}

}
