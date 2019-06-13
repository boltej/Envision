///////////////////////////////////////////////////////////////////////////////////////
/// \file archive.cpp
/// \brief Classes to make (de)serializing to/from streams convenient
///
/// $Date: 2012-09-20 15:22:28 +0200 (Thu, 20 Sep 2012) $
///
///////////////////////////////////////////////////////////////////////////////////////

#include "config.h"
#include "archive.h"

ArchiveInStream::ArchiveInStream(std::istream& strm)
	: in(strm) {
}

bool ArchiveInStream::save() const {
	return false;
}

void ArchiveInStream::transfer(char* s, std::streamsize n) {
	in.read(s, n);
}

ArchiveOutStream::ArchiveOutStream(std::ostream& strm)
	: out(strm) {
}

bool ArchiveOutStream::save() const {
	return true;
}

void ArchiveOutStream::transfer(char* s, std::streamsize n) {
	out.write(s, n);
}
