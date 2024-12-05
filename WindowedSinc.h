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

#include <cmath>
#include <stdexcept>

using namespace std;

//	Type should only be float, double, or long double.
template<typename FLOAT_TYPE = double_t>
class WindowedSinc
{
	FLOAT_TYPE *H  = nullptr;    //  points to the list of coeficients, sizeof (H) = M
	FLOAT_TYPE *Hc = nullptr;    //  points to the center of the list of coeficients
	uint32_t   M   = 0;         //  number of coeficients
	
	const FLOAT_TYPE twoPi = 2.0 * M_PI;

public:
	WindowedSinc() = default;
	
	WindowedSinc(double_t Fc, double_t transition)
	{
		SetSinc(Fc, transition);
	}

/////////////////////////////////////////////////////////////////////////////////////////
//  copy constructor
//	WindowedSinc::WindowedSinc(WindowedSinc &s)
//	{
//		CopyH(s);
//	}
	
	~WindowedSinc() { if (H) { delete[] H; }}

/////////////////////////////////////////////////////////////////////////////////////////
	//  Cutoff frequency Fc in fraction of sample rate;
	//  Transition width in fraction of sample rate;
	//	sinc = sin(2.0 * M_PI * Fc * (i - M / 2)) / (i - M / 2)
	//	natFc = 2.0 * M_PI * Fc
	//	index i; 0 -> M, inclusive
	//	Mo2 = M / 2
	void SetSinc(double_t Fc, double_t transition)
	{
		FLOAT_TYPE natFc = twoPi * Fc;
		
		M = (uint32_t) round(4.0 / transition);
		//	If M is not even then add 1
		if (M % 2 != 0) { M++; }
		
		//  if H existed, delete the old
		if (H != nullptr) { delete[] H; }
		H = new FLOAT_TYPE[M + 1];    //	H has one more slot than M
		if (!H) { throw runtime_error("WindowedSinc (new): Error creating H"); }
		
		uint32_t Mo2 = M / 2;
		int32_t  i;
		int32_t  imMo2;
		
		for (i = 0; i < Mo2; i++)
		{
			imMo2 = i - Mo2;
			H[i] = sinl(natFc * imMo2) / imMo2;
		}
		
		//	account for divide by zero
		H[i++] = natFc;
		
		for (; i <= M; i++)
		{
			imMo2 = i - Mo2;
			H[i] = sinl(natFc * imMo2) / imMo2;
		}
		
		Hc = H + Mo2;
		
		//  normalize sum of all points H to 1.0
		NormalGain();
	}
	
	//  returns coeficient at index without range checking
	inline FLOAT_TYPE GetCoeff(uint32_t i) { return H[i]; }
	
	//	return pointer to array of coefficients
	inline FLOAT_TYPE *Get_H() { return H; }
	
	//	returns size of H in count of coefficients
	inline uint32_t Get_M() { return M; }
	
	// Normalize then apply gain.
	void NormalGain(FLOAT_TYPE gain = 1.0)
	{
		FLOAT_TYPE sum = 0.0;
		
		for (uint32_t i = 0; i <= M; i++)
		{
			sum += H[i];
		}
		
		Gain(gain / sum);
	}
	
	//  gain only, -1 inverts. "a" for alpha, the scalor
	void Gain(FLOAT_TYPE a)
	{
		if (a != 1.0)  //  if not unity gain
		{
			for (uint32_t i = 0; i <= M; ++i)
			{
				H[i] *= a;
			}
		}
	}
	
	//  Applies window to H.
	void ApplyBlackman()
	{
		FLOAT_TYPE twoPiIoM;
		
		for (uint32_t i = 0; i <= M; i++)
		{
			twoPiIoM = twoPi * (i + 1) / (M + 2);
			H[i] *= (0.42 - (0.5 * cosl(twoPiIoM)) + (0.08 * cosl(2.0 * twoPiIoM)));
		}
		
		NormalGain();
	}
	
	void ApplyHamming()
	{
		for (uint32_t i = 0; i <= M; i++)
		{
			H[i] *= (0.54 - (0.46 * cosl(twoPi * i / M)));
		}
		
		NormalGain();
	}
	
	void MakeIntoHighPass()
	{
		Gain(-1.0);
		*Hc += 1.0;
	}
	
	//	For array operator, [0] points to the middle of the coefficients
	//	i: -Mo2 <= i <= Mo2
	inline FLOAT_TYPE operator[](int32_t i) { return *(Hc + i); }
	
//	inline WindowedSinc &operator+(WindowedSinc &s);
//	inline WindowedSinc &operator-(WindowedSinc &s);//{ return ( *this + (-s) ); }
//	inline WindowedSinc &operator-(void);
//	WindowedSinc &operator*(FLOAT_TYPE &a) { Gain(a); }

private:
//  extra house keeping functions
//	void CopyH(WindowedSinc &s);

};


#endif //DISKERROR_WINDOWEDSINC_H
