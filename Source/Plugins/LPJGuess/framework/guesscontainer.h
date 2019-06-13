///////////////////////////////////////////////////////////////////////////////////////
/// \file guesscontainer.h
/// \brief Container class for the main LPJ-GUESS classes which contain sub-objects
///
/// \author Joe Siltberg
/// $Date: 2014-03-19 09:46:54 +0100 (Wed, 19 Mar 2014) $
///
///////////////////////////////////////////////////////////////////////////////////////

#ifndef LPJ_GUESS_GUESS_CONTAINER_H
#define LPJ_GUESS_GUESS_CONTAINER_H

#include <vector>

/// Container class for the main LPJ-GUESS classes which contain sub-objects
/** This is supposed to be the base class for classes like Gridcell, Stand
 *  and Vegetation. 
 *
 *  It's meant to replace the ListArray class and its variants, although
 *  it will be introduced gradually.
 *
 *  The main reasons for replacing ListArray are:
 *
 *  - Polymorphic behaviour, in other words we want the container to be able
 *    to contain objects of different sub-classes. ListArray instantiates its
 *    objects itself, which means all objects are of the same type. 
 *    With GuessContainer the objects are instantiated outside and added to
 *    the container.
 *
 *  - Support for nested/simultaneous iteration. In ListArray the container
 *    owns the one and only iterator, which means you can't have two 
 *    iterations happening over the same container at the same time.
 *    With GuessContainer, iteration works like for the standard STL containers.
 *
 *  The main reasons for having a container class in the first case instead of
 *  using for instance std::vector are:
 *
 *  - The STL container classes aren't meant to be inherited from
 *
 *  - GuessContainer takes ownership of the objects. When an object is removed
 *    from the container it is also deallocated, and when the container itself
 *    is destroyed all objects are deallocated.
 *
 *  - The pointers are more or less hidden. The iterators and operator[] 
 *    returns references to objects instead of pointers. (We're trying to avoid
 *    pointers in most of the LPJ-GUESS code)
 *
 *  This class is implemented as a thin layer around std::vector, much of the
 *  interface is the same.
 */
template<typename T>
class GuessContainer {
public:

	GuessContainer() : next_unique_id(0) {
	}

	/// Destructor, deallocates all objects
	virtual ~GuessContainer() {
		clear();
	}

	/// Returns object at position i
	T& operator[](unsigned int i) {
		return *objects[i];
	}

	/// Returns number of objects in the container
	size_t size() const {
		return objects.size();
	}

	/// Adds an object to the end of the container
	void push_back(T* object) {
		objects.push_back(object);
	}

	/// Removes and deallocates all objects
	void clear() {
		for (size_t i = 0; i < objects.size(); ++i) {
			delete objects[i];
		}
		
		objects.clear();
	}

	// Iteration

	/// Works like std::vector::iterator
	class iterator {
	public:

		friend class GuessContainer<T>;

		bool operator==(const iterator& other) const {
			return itr == other.itr;
		}

		bool operator!=(const iterator& other) const {
			return itr != other.itr;
		}

		/// prefix ++
		iterator& operator++() {
			++itr;
			return *this;
		}

		/// postfix ++
		iterator operator++(int) {
			iterator copy(*this);
			++itr;
			return copy;
		}

		T& operator*() const {
			return **itr;
		}
		
	private:

		typedef typename std::vector<T*>::iterator internal_iterator;

		iterator(const internal_iterator& start) : itr(start) {}

		internal_iterator itr;
	};

	/// Returns an interator pointing to the first object
	iterator begin() {
		return iterator(objects.begin());
	}

	/// Returns an interator pointing past the end
	/** Just like STL containers, end() refers to a theoretical
	 *  position past the end. It does not point to an object
	 *  and must not be dereferenced, but should be used to detect
	 *  when the whole range has been passed.
	 */
	iterator end() {
		return iterator(objects.end());
	}

	/// Removes and deletes an object
	/** Works like std::vector::erase - returns an iterator
	 *  pointing to the object following the erased object.
	 *  Iterators pointing after the object being erased may be
	 *  invalidated. See documentation for std::vector::erase for
	 *  details.
	 */
	iterator erase(iterator itr) {
		delete *(itr.itr);
		return iterator(objects.erase(itr.itr));
	}

protected:
	
	/// Returns a new unique id number
	/** Some sub-classes may wish to assign unique id numbers to their objects,
	 *  so this functionality is provided here as a convenience.
	 */
	unsigned int get_next_id() {
		return next_unique_id++;
	}

private:
	/// Everything is implemented with std::vector
	std::vector<T*> objects;

	/// The next id number to return, \see get_next_id()
	unsigned int next_unique_id;
};

#endif // LPJ_GUESS_GUESS_CONTAINER_H
