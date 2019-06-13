////////////////////////////////////////////////////////////////////////////////
/// \file guessmath.h
/// \brief Mathematical utilites header file.
///
/// This header file contains:
///  (1) Definitions of constants and common functions used throughout LPJ-GUESS
///
/// \author Michael Mischurow
/// $Date: 2015-12-22 15:22:04 +0100 (Tue, 22 Dec 2015) $
///
////////////////////////////////////////////////////////////////////////////////

#ifndef LPJ_GUESS_GUESSMATH_H
#define LPJ_GUESS_GUESSMATH_H

#include <assert.h>
#include "archive.h"

#define _USE_MATH_DEFINES
#include <cmath>

#ifndef M_PI
double const PI = 4 * atan(1.0);
#else
double const PI = M_PI;
#undef M_PI
#endif
const double DEGTORAD = PI / 180.;

inline bool negligible(double dval, int limit = 0) {
	// Returns true if |dval| < EPSILON, otherwise false
	return limit ? fabs(dval) < pow(10.0, limit) : fabs(dval) < 1.0e-30;
}

inline bool largerthanzero(double dval, int limit = 0) {
	// Returns true if |dval| < EPSILON, otherwise false
	return limit ? dval > pow(10.0, limit) : dval > 1.0e-30;
}

inline bool equal(double dval1, double dval2) {
	// Returns true if |dval1-dval2| < EPSILON, otherwise false
	return negligible(dval1 - dval2);
}

inline double mean(double* array, int nitem) {

	// Returns arithmetic mean of 'nitem' values in 'array'

	double sum=0.0;
	for (int i=0; i<nitem; sum += array[i++]);
	return sum / (double)nitem;
}

/// Gives the mean of just two values
inline double mean(double x, double y) {
	return (x+y)/2.0;
}

/// Calculates variation coefficient of values in an array
inline double variation_coefficient(double* data, int n) {
	// 0 and 1 will give division with zero.
	if (n <= 1) {
		return -1;
	}

	double avg = mean(data, n);
	double dev = 0;
	for (int i=0; i<n; i++) {
		dev += (data[i] - avg) * (data[i] - avg);
	}
	double std = sqrt(dev / (n-1));

	if (std > 0 && avg > 0) {	// check that data appear in the array
		return std / avg;
	}
	return 0;
}

/// A short version of Richards curve where:
/** a is the lower asymptote,
 *  b is the upper asymptote. If a=0 then b is called the carrying capacity,
 *  c the growth rate,
 *  d is the time of maximum growth
 *  Source: https://en.wikipedia.org/wiki/Generalised_logistic_function, 2013-11-11
 */
inline double richards_curve(double a, double b, double c, double d, double x) {
	return a + (b - a) / (1 + exp(-c * (x - d)));
}

inline void regress(double* x, double* y, int n, double& a, double& b) {

	// Performs a linear regression of array y on array x (n values)
	// returning parameters a and b in the fitted model: y=a+bx
	// Source: Press et al 1986, Sect 14.2

	double sx,sy,sxx,sxy,delta;
	sx = sy = sxy = sxx = 0.0;

	for (int i=0; i<n; i++) {
		sx += x[i];
		sy += y[i];
		sxx+= x[i]*x[i];
		sxy+= x[i]*y[i];
	}
	delta = (double)n*sxx - sx*sx;
	a = (sxx*sy - sx*sxy)/delta;
	b = ((double)n*sxy-sx*sy)/delta;
}

// Forward declarations needed for the friendship between Historic and operator&

template<typename T, size_t capacity>
class Historic;

template<typename T, size_t capacity>
ArchiveStream& operator&(ArchiveStream& stream,
                         Historic<T, capacity>& data);


/// Keeps track of historic values of some variable
/** Useful for calculating running means etc.
 *
 *  The class behaves like a queue with a fixed size,
 *  when a new value is added, and the queue is full,
 *  the oldest value is overwritten.
 */
template<typename T, size_t capacity>
class Historic {
public:

	/// The maximum number of elements stored, given as template parameter
	static const size_t CAPACITY = capacity;

	Historic()
		: current_index(0), full(false) {
	}

	/// Adds a value, overwriting the oldest if full
	void add(double value) {
		values[current_index] = value;

		current_index = (current_index+1) % capacity;

		if (current_index == 0) {
			full = true;
		}
	}

	/// Returns the number of values stored (0-CAPACITY)
	size_t size() const {
		return full ? capacity : current_index;
	}

	/// Calculates arithmetic mean of the stored values
	T mean() const {
		const size_t nvalues = size();

		assert(nvalues != 0);

		return sum()/nvalues;
	}

	/// Sum of stored values
	T sum() const {
		T result = 0.0;

		const size_t nvalues = size();
		for (size_t i = 0; i < nvalues; ++i) {
			result += values[i];
		}
		return result;
	}

	/// Returnes the latest of the stored values
	T lastadd() const {	//latest
		return (*this)[size()-1];
	}

	/// Returns the maximum of the stored values
	T max() const {
		T result = -999999.0;

		const size_t nvalues = size();
		for (size_t i = 0; i < nvalues; ++i) {
			if (values[i]>result) {
				result = values[i];
			}
		}
		return result;
	}

	/// Returns the minimum of the stored values
	T min() const {
		T result = 999999.0;

		const size_t nvalues = size();
		for (size_t i = 0; i < nvalues; ++i) {
			if (values[i]<result) {
				result = values[i];
			}
		}
		return result;
	}

	/// Calculates arithmetic mean of the stored values for a period from current position and backwards nsteps
	T periodicmean(const size_t nsteps) const {

		assert(nsteps != 0);
		if (nsteps > size()){
			return mean();
		} else {
			return periodicsum(nsteps)/nsteps;
		}
	}

	/// Sum of stored values for a period from current position and backwards nsteps
	T periodicsum(const size_t nsteps) const {
		T result = 0.0;

		if (nsteps >= size()){
			return sum();
		} else {
			for (size_t i = size()-1; i >=size()-nsteps; --i) {
				result += (*this)[i];
			}
		}

		return result;
	}

	/// Returns a single value
	/** The values are ordered by age, the oldest value has index 0.
	 *
	 *  \param pos  Index of value to retrieve (must be less than size())
	 */
	T operator[](size_t pos) const {
		assert(pos < size());

		if (full) {
			return values[(current_index+pos)%capacity];
		}
		else {
			return values[pos];
		}
	}

	/// Writes all values to a plain buffer
	/** The values will be ordered by age, the oldest value has index 0.
	 *
	 *  \param buffer   Array to write to, must have room for at least size() values
	 */
	void to_array(T* buffer) const {
		const size_t first_position = full ? current_index : 0;
		const size_t nvalues = size();

		for (size_t i = 0; i < nvalues; ++i) {
			buffer[i] = values[(first_position+i)%capacity];
		}
	}

	friend ArchiveStream& operator&<T, capacity>(ArchiveStream& stream,
	                                             Historic<T, capacity>& data);

private:
	/// The stored values
	T values[capacity];

	/// The next position (in the values array) to write to
	size_t current_index;

	/// Whether we've stored CAPACITY values yet
	bool full;
};

/// Serialization support for Historic
/** We have a friend serialization operator instead of
 *  implementing Serializable, to avoid overhead of a
 *  vtable in Historic (since serialize() is virtual).
 *
 *  This could be implemented without friend, by only
 *  using size(), operator[] and add(). The order of
 *  the elements in the buffer will however not be
 *  preserved then, and for instance the sum() function
 *  will then not give _exactly_ the same results
 *  (due to limited floating point precision).
 */
template<typename T, size_t capacity>
ArchiveStream& operator&(ArchiveStream& stream,
                         Historic<T, capacity>& data) {
	stream & data.values
		& data.current_index
		& data.full;

	return stream;
}

#endif // LPJ_GUESS_GUESSMATH_H
