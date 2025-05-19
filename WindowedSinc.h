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

#ifndef DISKERROR_WINDOWEDSINCVECTOR_H
#define DISKERROR_WINDOWEDSINCVECTOR_H

#include <cmath>
#include <numbers>
#include <vector>

namespace Diskerror {

using namespace std;


/**
 * Generates and manages a list of numbers in the form of a windowed sinc function.
 * Type should only be float, double, or long double.
 * @tparam T
 */
template<typename T>
class WindowedSinc : protected vector<T> {

	const T twoPi = 2.0 * numbers::pi_v<T>;

	int32_t M   = 0;
	int32_t Mo2 = 0;  //	M / 2, or midpoint of size, which is always a positive odd number

public:

	WindowedSinc(const T cutoff_freq, const T transition)
	{
		const T natFc = twoPi * cutoff_freq;
		M = lround(4.0 / transition);
		//	Ensure M is even.
		//  The Book describes loops using the range of 0 to M such that 0 <= i <= M.
		//  That includes the value M!
		//  Therefore, if M==100, then there are 101 slots from 0 to 100.
		//  This also leaves a value in the center, which keeps the filter semetrical.
		if ( M % 2 != 0 ) { ++M; }
		Mo2 = M / 2;

		this->resize(M + 1);

		int32_t i, imMo2;
		for ( i = 0; i < Mo2; ++i ) {
			imMo2 = i - Mo2;
			*(this->data() + i) = sin(natFc * imMo2) / imMo2;
		}

		//	Account for divide by zero when i == Mo2
		*(this->data() + i++) = natFc;

		for ( ; i <= M; ++i ) {
			imMo2 = i - Mo2;
			*(this->data() + i) = sin(natFc * imMo2) / imMo2;
		}

		this->shrink_to_fit();

		//  normalize sum of all H to 1.0
		Normalize();
	}

	~WindowedSinc() = default;

	using vector<T>::at;

	//	For array operator, [0] points to the middle of the coefficients, and uses a signed index.
	//	i: -Mo2 <= i <= Mo2
	T operator[](int32_t i) { return this->data()[i + Mo2]; }

	//	IF unsigned int is used then index from the beginning.
	//  i: 0 <= i <= M
	T operator[](uint32_t i) { return this->data()[i]; }

	using vector<T>::operator[];
	using vector<T>::front;
	using vector<T>::back;
	using vector<T>::begin;
	using vector<T>::end;
	using vector<T>::empty;
	using vector<T>::size;

	int32_t getM() { return M; }

	int32_t getMo2() { return Mo2; }


	/**  Apply gain to H.
	 * -1 inverts.
	 * "a" for alpha, the scalor
	 * does nothing for 1.0, or unity gain
	 */
	void Gain(const T a)
	{
		if ( a != 1.0 ) {
			for_each(this->begin(), this->end(), [&a](T &elem) { elem *= a; });
		}
	}


	WindowedSinc &operator*=(const T &a)
	{
		for_each(this->begin(), this->end(), [&a](T &elem) { elem *= a; });
		return *this;
	}


	//  Applies window to H.
	void ApplyBlackman()
	{
		for ( size_t i = 0; i <= M; i++ ) {
			T twoPiIoM = twoPi * (i + 1) / (M + 2);
			this->data()[i] *= (0.42 - (0.5 * cos(twoPiIoM)) + (0.08 * cos(2.0 * twoPiIoM)));
		}

		Normalize();
	}


	void ApplyHamming()
	{
		for ( size_t i = 0; i <= M; i++ ) {
			this->data()[i] *= (0.54 - (0.46 * cos(twoPi * i / M)));
		}

		Normalize();
	}

	//  TODO:
	void ApplyHanning() {}

	//  TODO:
	void ApplyBartlett() {}


	void MakeLowCut()
	{
		Gain(-1.0);
		this->data()[Mo2] += 1.0;
	}


	//  Return the sum of all members.
	T sum() { return reduce(this->begin(), this->end(), (T) 0.0f); }

	//  Like fmadd() (fused multiply add), this is fused multiply sum.
	T fms(auto first2, int32_t n = 0)
	{
		auto begin1 = this->begin(), last1 = this->end();

		if ( abs(n) > this->size() ) throw runtime_error("'n' is too big");

		if ( n < 0 ) begin1 = this->end() + n;
		if ( n > 0 ) last1  = this->begin() + n;

		return transform_reduce(begin1, last1, first2, 0.0f, plus<>(), multiplies<>());
	}


private:

	// Normalize then apply gain.
	void Normalize() { Gain(1.0 / this->sum()); }

};

} //	Diskerror

#endif // DISKERROR_WINDOWEDSINCVECTOR_H
