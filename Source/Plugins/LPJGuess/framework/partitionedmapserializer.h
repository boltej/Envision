////////////////////////////////////////////////////////////////////////////////
/// \file partitionedmapserializer.h
/// \brief Classes for serializing elements spread out over many processes
///
/// These classes were developed to save state information in LPJ-GUESS. For
/// parallel runs with grid cells spread out over different processes the state
/// information can be saved to files in a way so that they can later be read
/// back when running with a different number of processes.
///
/// PartitionedMapSerializer and PartitionedMapDeserializer handle the more
/// general case of saving an associative array, or map, partitioned over
/// different processes. In the LPJ-GUESS case, the elements of the map
/// are the grid cells, and the keys uniquely identifying elements are
/// coordinates.
///
/// Each process will write its elements to a single file, followed by an index
/// (the keys, sorted). When the elements should be read back each process will
/// read in the index of each state file and read in the elements it wants in
/// the order that minimizes disk seeks.
///
/// \author Joe Siltberg
/// $Date: 2014-06-23 15:50:25 +0200 (Mon, 23 Jun 2014) $
///
////////////////////////////////////////////////////////////////////////////////

#ifndef JL_PARTITIONED_MAP_SERIALIZER_H
#define JL_PARTITIONED_MAP_SERIALIZER_H

#include <vector>
#include <utility>
#include <string>
#include <algorithm>
#include <map>
#include <stdexcept>
#include <fstream>
#include <memory>

/// Creates a path to a state file given directory and process rank
/** For instance, create_path("output", 17) might return something like
 *  "output/17.state".
 */
std::string create_path(const char* directory, int rank);

/// Thrown if an error is encountered when serializing/deserializing
class PartitionedMapSerializerError : public std::runtime_error {
public:
	PartitionedMapSerializerError(const std::string& what) : std::runtime_error(what) {}
};

/// Class for serializing a partitioned map
/** This class writes out a single state file with the elements of the map
 *  which belong to the current process. Together with the elements an
 *  index is also written so that maps can quickly be deserialized even
 *  if the elements -> process mapping is different when deserializing.
 *
 *  \param Element           The type of the elements in the map
 *  \param Key               The type of the keys identifying elements
 *  \param ElementSerializer Functor serializing an element
 *  \param KeySerializer     Functor serializing a key
 */
template<typename Element,
         typename Key,
         typename ElementSerializer,
         typename KeySerializer>
class PartitionedMapSerializer {
public:
	/// Construct a serializer
	/** \param directory Where to place the files
	 *  \param my_rank   An integer uniquely identifying this process
	 *                   (for instance an MPI rank)
	 *  \param es        The functor for serializing elements
	 *  \param ks        The functor for serializing keys
	 */
	PartitionedMapSerializer(const char* directory,
	                         int my_rank,
	                         ElementSerializer es,
	                         KeySerializer ks)
		: element_serializer(es), 
		  key_serializer(ks),
		  file(create_path(directory, my_rank).c_str(), 
		       std::ios::binary | std::ios::trunc) {
		  
		if (file.fail()) {
			throw PartitionedMapSerializerError(std::string("failed to open a file in ") + directory);
		}
	}
	 
	~PartitionedMapSerializer() {
		write_index();
	}

	/// Serializes a single element
	/** \param key      Key for the element to serialize
	 *  \param element  The element to serialize
	 */
	void serialize_element(const Key& key,
	                       const Element& element) {
		add_to_index(key);
		element_serializer(file, element);
	}

private:

	/// Simply adds a key and the current file position to the index, in memory
	/** The index is written out to file in the destructor */
	void add_to_index(const Key& key) {
		index.push_back(std::make_pair(key, file.tellp()));
	}

	/// Writes out the index to the file
	void write_index() {
		// sort it so we can search quickly
		std::sort(index.begin(), index.end());

		// write each key and the position of its element in the file
		for (size_t i = 0; i < index.size(); i++) {
			key_serializer(file, index[i].first);

			// convert to a std::streamsize since that is guaranteed to be defined as a
			// basic integral type which we can serialize by simply taking its binary 
			// representation
			std::streamsize file_position = index[i].second;
			file.write(reinterpret_cast<const char*>(&file_position), 
			           sizeof(file_position));
		}

		// write out the number of elements after the index so it possible to
		// find the start of the index
		size_t number_of_elements = index.size();
		file.write(reinterpret_cast<const char*>(&number_of_elements), 
		           sizeof(number_of_elements));

		if (file.fail()) {
			throw PartitionedMapSerializerError("failed to write out index");
		}
	}
	 
	typedef std::pair<Key, std::streampos> IndexElement;
	typedef std::vector<IndexElement> Index;

	/// The in-memory index, written to file when we're done
	Index index;

	ElementSerializer element_serializer;
	KeySerializer key_serializer;

	/// The file we're serializing to
	std::ofstream file;
};

/// Class for deserializing a partitioned map
/** This class will read in the indices of each state file and then read in
 *  just the elements that belong to this process.
 *
 *  \param Element              The type of the elements in the map
 *  \param Key                  The type of the keys identifying elements
 *  \param ElementDeserializer  Functor deserializing an element
 *  \param KeyDeserializer      Functor deserializing a key
 *  \param KeySize              The size of a serialized key (in bytes)
 */
template<typename Element,
         typename Key,
         typename ElementDeserializer,
         typename KeyDeserializer,
         int KeySize>
class PartitionedMapDeserializer {
public:
	/// Construct a deserializer
	/** \param directory Where to find the state files
	 *  \param max_rank  The rank of the process with highest rank
	 *  \param ed        Functor for deserializing an element
	 *  \param kd        Functor for deserializing a key
	 */
	PartitionedMapDeserializer(const char* directory,
	                           int max_rank,
	                           ElementDeserializer ed,
	                           KeyDeserializer kd)
		: element_deserializer(ed), key_deserializer(kd) {

		// we'll simply try to open all files between 0 and max_rank (inclusive)
		int rank = 0;

		while (rank <= max_rank) {
			std::string path = create_path(directory, rank);

			std::auto_ptr<std::ifstream> stream(
			                                    new std::ifstream(path.c_str(), std::ios::binary | std::ios::in));

			if (!stream->fail()) {
				File* file = new File;
				// read index

				// seek to the number of elements
				stream->seekg(-std::streampos(sizeof(size_t)), std::ios::end);
				size_t number_of_elements;
				stream->read(reinterpret_cast<char*>(&number_of_elements), sizeof(size_t));
					 
				// seek to start of index
				std::streamsize index_size = 
					number_of_elements*(KeySize+sizeof(std::streamsize));
				stream->seekg(-std::streampos(sizeof(size_t))-index_size, std::ios::cur);

				// read the index
				for (size_t i = 0; i < number_of_elements; i++) {
					Key key;
					std::streamsize position;
					key_deserializer(*stream, key);
					stream->read(reinterpret_cast<char*>(&position), sizeof(std::streamsize));
					file->index.push_back(std::make_pair(key, position));
				}

				if (stream->fail()) {
					throw PartitionedMapSerializerError(std::string("failed to read index for state file: ") + path);
				}

				file->stream = stream;
				files.push_back(file);
			}
			rank++;
		}

		if (files.empty()) {
			throw PartitionedMapSerializerError("couldn't open any state files");
		}
	}

	~PartitionedMapDeserializer() {

		for (size_t i = 0; i < files.size(); i++) {
			delete files[i];
		}

	}

	/// Reads in a single element from disk
	/** Use deserialize_elements instead if several elements should be read
	 *  at once as that version will minimize the number of disk seeks.
	 *
	 *  \param key     The key identifying the element to read in
	 *  \param element The element to deserialize to
	 */
	void deserialize_element(const Key& key, Element& element) {

		for (size_t i = 0; i < files.size(); i++) {
			Index& index = files[i]->index;
			std::auto_ptr<std::ifstream>& stream = files[i]->stream;
			// find position of element in this file if it has it
			typename Index::iterator itr = 
				std::lower_bound(index.begin(), index.end(), std::make_pair(key,0), 
				                 IndexElementComparator());
				
			// if it had it, seek to that position and deserialize
			if (itr != index.end() && !(key < (*itr).first)) {
				stream->seekg((*itr).second, std::ios::beg);
				element_deserializer(*stream, element);
				return;
			}
		}

		throw PartitionedMapSerializerError("failed to find element to deserialize");
	}

	/// Reads in several elements from disk
	/** \param keys      The keys identifying the elements to read in
	 *  \param elements  Vector of pointers to elements that should be deserialized.
	 */
	void deserialize_elements(const std::vector<Key>& keys, 
	                          const std::vector<Element*>& elements) {

		// This function is a bit tricky since it should read in the elements
		// from disk in the order which minimizes the number of seeks, but
		// at the same time return the elements in the order of the keys.

		// A vector to keep track of which elements have been deserialized,
		// all set to false initially
		std::vector<bool> found_elements(elements.size());
		  
		// Create a mapping from keys to their position in the keys vector.
		// This way, we can quickly find out where in the elements vector 
		// we can find the element to deserialize for that key.
		std::map<Key, size_t> keys_map;
		for (size_t k = 0; k < keys.size(); k++) {
			keys_map[keys[k]] = k;
		}

		// go through all indexes, finding all file positions to read from

		for (size_t i = 0; i < files.size(); i++) {
			// for each file, we have a vector of (file pos, index) pairs, where
			// file pos is where to read in the file, and index is where the element
			// should be placed in the elements vector
			std::vector<std::pair<std::streampos, size_t> > positions;

			// Simply loop through the index linearly
			Index& index = files[i]->index;
			for (size_t j = 0; j < index.size(); j++) {
				Key& key = index[j].first;

				// Is this one of the keys we're interested in?
				typename std::map<Key, size_t>::iterator itr = keys_map.find(key);
				if (itr != keys_map.end()) {
					// Yes, so save the position of that element in the file and
					// the position in the elements vector where to store the element
					positions.push_back(std::make_pair(index[j].second, (*itr).second));
				}
			}

			// did any of the keys exist in this file?
			if (positions.empty()) {
				// nope, so continue with next file
				continue;
			}

			// sort the positions for this file, so that we will read the 
			// elements from this file in the order they are stored in the file
			std::sort(positions.begin(), positions.end());

			// read all the elements from this file
			std::auto_ptr<std::ifstream>& stream = files[i]->stream;
			stream->seekg(positions[0].first, std::ios::beg);

			for (size_t e = 0; e < positions.size(); e++) {
				// only seek if necessary
				if (stream->tellg() != positions[e].first) {
					stream->seekg(positions[e].first, std::ios::beg);
				}
					 
				// deserialize this element
				element_deserializer(*stream, *elements[positions[e].second]);
				if (!stream->fail()) {
					found_elements[positions[e].second] = true;
				}
				else {
					throw PartitionedMapSerializerError("failed to deserialize element from state file");
				}
			}

			if (stream->fail()) {
				throw PartitionedMapSerializerError("failed to deserialize elements from state file");
			}
		}

		// check if there are any elements not read in
		if (std::find(found_elements.begin(), found_elements.end(), false) != 
		    found_elements.end()) {
			throw PartitionedMapSerializerError("failed to find some elements");
		}
	}

private:
	typedef std::pair<Key, std::streampos> IndexElement;
	typedef std::vector<IndexElement> Index;

	// A functor which compares IndexElements by comparing their keys
	struct IndexElementComparator {
		bool operator()(const IndexElement& left, const IndexElement& right) {
			return left.first < right.first;
		}
	};

	/// Internal representation for each state file
	struct File {
		/// An opened stream for this state file
		std::auto_ptr<std::ifstream> stream;

		/// The index, read in from the file
		Index index;
	};

	/// All the state files
	std::vector<File* > files;

	ElementDeserializer element_deserializer;
	KeyDeserializer key_deserializer;
};

#endif // JL_PARTITIONED_MAP_SERIALIZER_H
