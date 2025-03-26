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

using namespace std;

//	Type should only be float, double, or long double.
class WindowedSinc
{
	long double *H  = nullptr;    //  points to the list of coeficients, sizeof (H) = M
	long double *Hc = nullptr;    //  points to the center of the list of coeficients
	uint32_t    M   = 0;         //  number of coeficients

	const long double twoPi = 2.0 * M_PI;

public:
	WindowedSinc() = default;

	WindowedSinc(double_t Fc, double_t transition) { SetSinc(Fc, transition); }

/////////////////////////////////////////////////////////////////////////////////////////
//  copy constructor
//	WindowedSinc::WindowedSinc(WindowedSinc &s)
//	{
//		CopyH(s);
//	}

	~WindowedSinc() { delete H; }

/////////////////////////////////////////////////////////////////////////////////////////
	void SetSinc(double_t Fc, double_t transition);

	//  returns coeficient at index without range checking
	inline long double GetCoeff(uint32_t i) { return H[i]; }

	//	return pointer to array of coefficients
	inline long double *Get_H() { return H; }

	//	returns size of H in count of coefficients
	inline uint32_t Get_M() { return M; }

	// Normalize then apply gain.
	void NormalGain(long double gain = 1.0);

	//  gain only, -1 inverts. "a" for alpha, the scalor
	void Gain(long double a);

	//  Applies window to H.
	void ApplyBlackman();

	void ApplyHamming();

	void MakeIntoHighPass();

	//	For array operator, [0] points to the middle of the coefficients
	//	i: -Mo2 <= i <= Mo2
	inline long double operator[](int32_t i) { return *(Hc + i); }

	//	IF unsigned int is used then index from the beginning.
	inline long double operator[](uint32_t i) { return *(H + i); }

//	inline WindowedSinc &operator+(WindowedSinc &s);
//	inline WindowedSinc &operator-(WindowedSinc &s);//{ return ( *this + (-s) ); }
//	inline WindowedSinc &operator-(void);
//	WindowedSinc &operator*(long double &a) { Gain(a); }

private:
//  extra house keeping functions
//	void CopyH(WindowedSinc &s);

};


#endif // DISKERROR_WINDOWEDSINC_H
