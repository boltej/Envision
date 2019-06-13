////////////////////////////////////////////////////////////////////////////////
/// \file partitionedmapserializer.cpp
/// \brief Implementation file for PartitionedMapSerializer/Deserializer
///
/// Since these classes are templates, most of their implementation is in the
/// header.
///
/// \author Joe Siltberg
/// $Date: 2013-10-10 10:20:33 +0200 (Thu, 10 Oct 2013) $
///
////////////////////////////////////////////////////////////////////////////////

#include "partitionedmapserializer.h"
#include <sstream>

std::string create_path(const char* directory, int rank) {
	std::ostringstream os;
	os << directory << "/" << rank << ".state";
	return os.str();
}
