//
// Created by Reid Woodbury on 11/15/24.
//

/**
 * Designed with guidence from:
 * The Scientist and Engineer's Guide to
 * Digital Signal Processing
 * (Second Edition)
 * by Steven W. Smith
 */

#ifndef DISKERROR_VECTORMATH_H
#define DISKERROR_VECTORMATH_H

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <random>
#include <vector>

namespace Diskerror {


/**
 * Adds math functions to std::vector<> like valarray.
 * This will focus on the way I've always wanted to process off-line audio data.
 * @tparam T
 */
template<typename T>
class VectorMath : public std::vector<T> {

public:

	VectorMath() = default;

	explicit VectorMath(uint64_t count) : std::vector<T>(count) {}

	~VectorMath() = default;

	VectorMath & operator*=(const T & a)
	{
		for ( T & elem : *this ) elem *= a;
		return *this;
	}

	VectorMath & operator/=(const T & a)
	{
		for ( T & elem : *this ) elem /= a;
		return *this;
	}

	T sum() { return reduce(this->begin(), this->end(), (T) 0.0); }

	T average() { return this->sum() / (T) this->size(); }

	T max()
	{
		auto val = std::numeric_limits<T>::lowest();
		for ( T & elem : *this ) if ( elem > val ) val = elem;
		return val;
	}

	T min()
	{
		auto val = std::numeric_limits<T>::max();
		for ( T & elem : *this ) if ( elem < val ) val = elem;
		return val;
	}

	//  Maximum magitude.
	T max_mag()
	{
		auto    max_mag_val = (T) 0;
		for ( T elem : *this ) { //  do not reference elem
			if ( elem < 0 ) elem                  = -elem;
			if ( elem > max_mag_val ) max_mag_val = elem;
		}
		return max_mag_val;
	}

	void normalize_sum(const T max_possible)
	{
		(*this) *= (max_possible / this->sum());
	}

	void normalize_mag(const T max_possible)
	{
		(*this) *= (max_possible / this->max_mag());
	}

	//  This applies a random number with values from -1.0 to 1.0 in triangular distribution.
	//  This is most useful to apply to elements just before converting floating point values to integer values.
	void dither()
	{
		static std::random_device                rd;
		static std::mt19937                      gen(rd());
		static std::uniform_real_distribution<T> low(-1, 0);
		static std::uniform_real_distribution<T> hi(0, 1);

		//  We can't simply use "*this += (low(gen) + hi(gen));"
		//  because we need "(low(gen) + hi(gen))" to return a different value for each sample.
		for ( T & elem : *this ) elem += (low(gen) + hi(gen));
	}

};

} //	Diskerror

#endif // DISKERROR_VECTORMATH_H
