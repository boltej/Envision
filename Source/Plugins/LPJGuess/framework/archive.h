///////////////////////////////////////////////////////////////////////////////////////
/// \file archive.h
/// \brief Classes to make (de)serializing to/from streams convenient
///
/// $Date: 2012-11-20 09:44:41 +0100 (Tue, 20 Nov 2012) $
///
///////////////////////////////////////////////////////////////////////////////////////

#ifndef LPJ_GUESS_ARCHIVE_H
#define LPJ_GUESS_ARCHIVE_H

#include <ostream>
#include <istream>
#include <vector>

/// Abstract base class for ArchiveInStream and ArchiveOutStream
/** The base class declares the transfer function, which will read
 *  data from a stream in ArchiveInStream, and write data to a stream
 *  in ArchiveOutStream. By having the same interface for both
 *  cases, classes can have one function for both serializing and
 *  deserializing.
 *
 *  Sometimes we do need to know which direction the ArchiveStream
 *  is working in though, so that can be queried with the save
 *  function.
 */
class ArchiveStream {
public:

	/// Checks if this ArchiveStream is saving data to stream or reading
	virtual bool save() const = 0;

	/// Write or read data to/from a stream
	/** \param s   Data buffer to write out, or read to.
	 *             Needs to be n bytes.
	 *  \param n   Number of bytes to read or write
	 */
	virtual void transfer(char* s, std::streamsize n) = 0;
};

/// Class for reading data from an istream
/** \see ArchiveStream for more documentation */
class ArchiveInStream : public ArchiveStream {
public:
	ArchiveInStream(std::istream& strm);

	bool save() const;

	void transfer(char* s, std::streamsize n);

private:
	/// The stream we're reading from
	std::istream& in;
};

/// Class for writing data to an ostream
/** \see ArchiveStream for more documentation */
class ArchiveOutStream : public ArchiveStream {
public:
	ArchiveOutStream(std::ostream& strm);

	bool save() const;

	void transfer(char* s, std::streamsize n);

private:
	/// The stream we're writing to
	std::ostream& out;
};

/// Interface showing that a class can serialize itself
/** Classes which can serialize themselves through an ArchiveStream
 *  should inherit from this class, and implement the serialize function.
 */
class Serializable {
public:
	/// Needs to be implemented by all sub-classes
	virtual void serialize(ArchiveStream& arch) = 0;
};

/// Function for checking if something inherits from Serializable
/** This variant will be chosen by the compiler for anything which
  * isn't Serializable (because of the overload below), and so always 
  * returns false.
  *
  * Only intended to be used from the operator& implementation below,
  * which has the added benefit that if & is used for a class which
  * doesn't inherit from Serializable, many modern compilers will
  * warn because we're trying to send a non-POD type to a variadic
  * function.
  */
inline bool inheritsFromSerializable(...) {
	return false;
}

/// Overload for Serializable, always returns true
inline bool inheritsFromSerializable(const Serializable& s) {
	return true;
}

/// Operator overloading of &, allowing serialization to be chained
/** Since the operator returns the original stream, we can serialize
 *  multiple items in the following way:
 *
 *  \code
 *    ArchiveOutStream arch(os);
 *
 *    arch & width & height & name;
 *  \endcode
 *
 *  This function can be used for any item where the memory representation
 *  of the item can be written to file bit by bit, such as the primitive
 *  data types, or classes implementing Serializable.
 */
template<typename T>
ArchiveStream& operator&(ArchiveStream& stream, T& data) {
	if (inheritsFromSerializable(data)) {
		((Serializable&)data).serialize(stream);
	}
	else {
		stream.transfer((char*)&data, sizeof(data));
	}
	return stream;
}

template<typename T>
ArchiveStream& operator&(ArchiveStream& stream, std::vector<T>& data) {
	if (stream.save()) {
		size_t size = data.size();
		stream & size;
	}
	else {
		size_t size;
		stream & size;

		data.resize(size);
	}

	for (size_t i = 0; i < data.size(); ++i) {
		stream & data[i];
	}

	return stream;
}

#endif // LPJ_GUESS_ARCHIVE_H
