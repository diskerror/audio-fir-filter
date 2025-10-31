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
#include <valarray>

#include "VectorMath.h"

namespace Diskerror {
using namespace std;


/**
 * Generates and manages a list of numbers in the form of a windowed sinc function.
 * Type should only be float, double, or long double.
 * @tparam T
 */
template <typename T>
class WindowedSinc : public VectorMath<T> {
	const T twoPi = 2.0 * numbers::pi_v<T>;

	const uint32_t M   = 0;
	const uint32_t Mo2 = 0; //	M / 2, or midpoint of size, which is always a positive odd number

	//  Prevent usage of these.
	using VectorMath<T>::operator=;
	using VectorMath<T>::assign;
	using VectorMath<T>::assign_range;
	using VectorMath<T>::get_allocator;
	using VectorMath<T>::reserve;
	using VectorMath<T>::clear;
	using VectorMath<T>::insert;
	using VectorMath<T>::insert_range;
	using VectorMath<T>::emplace;
	using VectorMath<T>::erase;
	using VectorMath<T>::push_back;
	using VectorMath<T>::emplace_back;
	using VectorMath<T>::append_range;
	using VectorMath<T>::pop_back;
	using VectorMath<T>::resize;
	using VectorMath<T>::swap;

	using VectorMath<T>::normalize_mag;
	using VectorMath<T>::normalize_sum;

	static uint32_t setM(const T transition) {
		if (transition <= 0.0 || transition >= 0.5) throw runtime_error("Transition band width out of range.");
		uint32_t M = lround(4.0 / transition);
		//	Ensure M is even.
		//  The Book describes loops using the range of 0 to M such that 0 <= i <= M.
		//  That includes the value M!
		//  Therefore, if M==100, then there are 101 slots from 0 to 100.
		//  This also leaves a value in the center, which keeps the filter semetrical
		//      about a center value.
		if (M % 2 != 0) ++M;
		return M;
	}

public:
	/**
	 * Constructor
	 *
	 * The cutoff frequency and transition band width are passed as fractions of the sample rate.
	 * Meaningful numbers follow the rule, in: 0.0 < in < 0.5
	 *
	 * @param cutoff_freq
	 * @param transition
	 */
	WindowedSinc(const T cutoff_freq, const T transition) : M(setM(transition)), Mo2(this->M / 2) {
		if (cutoff_freq <= 0.0 || cutoff_freq >= 0.5) throw runtime_error("Cut-off frequency out of range.");
		//		if ( transition <= 0.0 || transition >= 0.5 ) throw runtime_error("Transition band width out of range.");

		const T natFc = twoPi * cutoff_freq;
		//		this->M = lround(4.0 / transition);
		//	Ensure M is even.
		//  The Book describes loops using the range of 0 to M such that 0 <= i <= M.
		//  That includes the value M!
		//  Therefore, if M==100, then there are 101 slots from 0 to 100.
		//  This also leaves a value in the center, which keeps the filter semetrical.
		//		if ( this->M % 2 != 0 ) ++this->M;
		//		this->Mo2 = this->M / 2;

		this->resize(this->M + 1);
		this->shrink_to_fit();

		int32_t i, imMo2;
		for (i = 0; i < this->Mo2; ++i) {
			imMo2      = i - this->Mo2;
			(*this)[i] = sin(natFc * imMo2) / imMo2;
		}

		//	Account for divide by zero when i == Mo2
		(*this)[i++] = natFc;

		for (; i <= M; ++i) {
			imMo2      = i - this->Mo2;
			(*this)[i] = sin(natFc * imMo2) / imMo2;
		}

		//  normalize sum of all H to 1.0
		this->normalize();
	}

	~WindowedSinc() = default;


	[[nodiscard]] uint32_t getM() { return this->M; }

	[[nodiscard]] uint32_t getMo2() const { return this->Mo2; }

	void normalize() { this->normalize_sum(1.0); }


	//  Applies window to H.
	void ApplyBlackman() {
		for (size_t i = 0; i <= this->M; i++) {
			T twoPiIoM = twoPi * (i + 1) / (this->M + 2);
			(*this)[i] *= (0.42 - (0.5 * cos(twoPiIoM)) + (0.08 * cos(2.0 * twoPiIoM)));
		}

		this->normalize();
	}


	void ApplyHamming() {
		for (size_t i = 0; i <= this->M; i++) {
			(*this)[i] *= 0.54 - (0.46 * cos(twoPi * i / this->M));
		}

		this->normalize();
	}

	//  TODO:
	// void ApplyHanning() {}

	//  TODO:
	// void ApplyBartlett() {}


	void MakeLowCut() {
		this->operator*=(-1); //  inverts kernal output
		(*this)[Mo2] += 1.0;  //  makes kernal add in original value; making original minus low pass (high cut).
	}

	//  Like fma() (fused multiply add), this is fused multiply sum.
	//  n: 0 <= n <= M; or
	//  n: 0 <= n < this->size()
	//  TODO: TEST THIS
	T fms(auto begin2, int32_t n = 0) {
		if (abs(n) >= this->M) throw runtime_error("'n' is too big");

		auto begin1 = this->begin(), end1 = this->end();
		if (n < 0) begin1 = this->end() + n;
		if (n > 0) end1 = this->begin() + n;

		return transform_reduce(begin1, end1, begin2, 0.0f, plus<>(), multiplies<>());
	}
};

} //	Diskerror

#endif // DISKERROR_WINDOWEDSINCVECTOR_H
