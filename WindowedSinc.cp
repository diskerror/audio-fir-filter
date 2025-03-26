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

using namespace std;

/////////////////////////////////////////////////////////////////////////////////////////
//  Cutoff frequency Fc in fraction of sample rate;
//  Transition width in fraction of sample rate;
//	sinc = sin(2.0 * M_PI * Fc * (i - M / 2)) / (i - M / 2)
//	natFc = 2.0 * M_PI * Fc
//	index i; 0 -> M, inclusive
//	Mo2 = M / 2
void WindowedSinc::SetSinc(double_t Fc, double_t transition)
{
	long double natFc = twoPi * Fc;

	M = (uint32_t) round(4.0 / transition);
	//	If M is not even then add 1
	if (M % 2 != 0) { M++; }

	//  if H existed, delete the old
	delete H;
	H = new long double[M + 1];    //	H has one more slot than M

	int32_t Mo2 = (int32_t) M / 2;
	int32_t i;
	int32_t imMo2;

	for (i = 0; i < Mo2; i++) {
		imMo2 = i - Mo2;
		H[i] = sin(natFc * imMo2) / imMo2;
	}

	//	account for divide by zero
	H[i++] = natFc;

	for (; i <= M; i++) {
		imMo2 = i - Mo2;
		H[i] = sin(natFc * imMo2) / imMo2;
	}

	Hc = H + Mo2;

	//  normalize sum of all points H to 1.0
	NormalGain();
}


// Normalize then apply gain.
void WindowedSinc::NormalGain(long double gain)
{
	long double sum = 0.0;

	for (uint32_t i = 0; i <= M; i++) {
		sum += H[i];
	}

	Gain(gain / sum);
}

//  gain only, -1 inverts. "a" for alpha, the scalor
void WindowedSinc::Gain(long double a)
{
	if (a != 1.0)  //  if not unity gain
	{
		for (uint32_t i = 0; i <= M; ++i) {
			H[i] *= a;
		}
	}
}

//  Applies window to H.
void WindowedSinc::ApplyBlackman()
{
	long double twoPiIoM;

	for (uint32_t i = 0; i <= M; i++) {
		twoPiIoM = twoPi * (i + 1) / (M + 2);
		H[i] *= (0.42 - (0.5 * cos(twoPiIoM)) + (0.08 * cos(2.0 * twoPiIoM)));
	}

	NormalGain();
}

void WindowedSinc::ApplyHamming()
{
	for (uint32_t i = 0; i <= M; i++) {
		H[i] *= (0.54 - (0.46 * cos(twoPi * i / M)));
	}

	NormalGain();
}

void WindowedSinc::MakeIntoHighPass()
{
	Gain(-1.0);
	*Hc += 1.0;
}
