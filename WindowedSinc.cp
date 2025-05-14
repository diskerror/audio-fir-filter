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

#include <cmath>
#include <stdexcept>

#include "WindowedSinc.h"

namespace Diskerror {

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Cutoff frequency Fc in fraction of sample rate;
//  Transition width M in fraction of sample rate, rounded up to the next odd integer;
//	sinc = sin(2.0 * M_PI * Fc * (i - M / 2)) / (i - M / 2)
//	natFc = 2.0 * M_PI * Fc
//	index i; 0 -> M, inclusive
//	Mo2 = M / 2
void WindowedSinc::SetSinc(double Fc, double transition)
{
	long double natFc = 2.0 * M_PI * Fc;

	M = (int32_t) round(4.0 / transition);
	//	If M is not even then add 1
	if ( M % 2 != 0 ) { M++; }
	Mo2 = M / 2;

	H.resize(M);
	H *= 0;    //	clear

	int32_t i, imMo2;

	for ( i = 0; i < Mo2; i++ ) {
		imMo2 = i - Mo2;
		H[i] = sin(natFc * imMo2) / imMo2;
	}

	//	Account for divide by zero when i == Mo2
	H[i++] = natFc;

	for ( ; i <= M; i++ ) {
		imMo2 = i - Mo2;
		H[i] = sin(natFc * imMo2) / imMo2;
	}

	//  normalize sum of all points H to 1.0
	Normalize();
}


//  gain only, -1 inverts. "a" for alpha, the scalor
void WindowedSinc::Gain(double a)
{
	if ( a != 1.0 ) {  //  if not unity gain
		H *= a;
	}
}

//  Applies window to H.
void WindowedSinc::ApplyBlackman()
{
	long double twoPi = 2.0 * M_PI;
	long double twoPiIoM;

	for ( uint32_t i = 0; i <= M; i++ ) {
		twoPiIoM = twoPi * ( i + 1 ) / ( M + 2 );
		H[i] *= ( 0.42 - ( 0.5 * cos(twoPiIoM) ) + ( 0.08 * cos(2.0 * twoPiIoM) ) );
	}

	Normalize();
}

void WindowedSinc::ApplyHamming()
{
	long double twoPi = 2.0 * M_PI;

	for ( uint32_t i = 0; i <= M; i++ ) {
		H[i] *= ( 0.54 - ( 0.46 * cos(twoPi * i / M) ) );
	}

	Normalize();
}

void WindowedSinc::MakeLowCut()
{
	Gain(-1.0);
	H[Mo2] += 1.0;
}

} //  namespace Diskerror
