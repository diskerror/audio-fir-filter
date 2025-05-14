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

#ifndef DISKERROR_WINDOWEDSINC_H
#define DISKERROR_WINDOWEDSINC_H

#include <valarray>

namespace Diskerror {

using namespace std;


//	Type should only be float, double, or long double.
class WindowedSinc {

	valarray<long double> H;        //  points to the list of coeficients, sizeof (H) = M
	int32_t               M   = 0;  //  number of coeficients
	int32_t               Mo2 = 0;  //	M / 2

public:
	WindowedSinc() = default;

	WindowedSinc(double Fc, double transition) { SetSinc(Fc, transition); }

/////////////////////////////////////////////////////////////////////////////////////////
//  copy constructor
//	WindowedSinc::WindowedSinc(WindowedSinc &s)
//	{
//		CopyH(s);
//	}

// 	~WindowedSinc() {}

/////////////////////////////////////////////////////////////////////////////////////////
	void SetSinc(double Fc, double transition);

	//  returns coeficient at index without range checking
	inline long double GetCoeff(uint32_t i) { return H[i]; }

	//	return pointer to array of coefficients
// 	inline long double *Get_H() { return *H; }

	//	Returns count of coefficients
	inline uint32_t Get_M() { return M; }

	/**  Apply gain to H.
	 * -1 inverts.
	 * "a" for alpha, the scalor
	 * does nothing for 1.0, or unity gain
	 */
	void Gain(double a);

	//  Applies window to H.
	void ApplyBlackman();

	void ApplyHamming();

	void MakeIntoHighPass();

	//	For array operator, [0] points to the middle of the coefficients
	//	i: -Mo2 <= i <= Mo2
	inline long double operator[](int32_t i) { return H[i + Mo2]; }

	//	IF unsigned int is used then index from the beginning.
	inline long double operator[](uint32_t i) { return H[i]; }

//	inline WindowedSinc &operator+(WindowedSinc &s);
//	inline WindowedSinc &operator-(WindowedSinc &s);//{ return ( *this + (-s) ); }
//	inline WindowedSinc &operator-(void);
//	WindowedSinc &operator*(long double &a) { Gain(a); }

private:
//  extra house keeping functions
//	void CopyH(WindowedSinc &s);

	// Normalize then apply gain.
	void Normalize() { Gain(1.0 / H.sum()); }

};

} //	Diskerror

#endif // DISKERROR_WINDOWEDSINC_H
