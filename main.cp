//
// Created by Reid Woodbury.
//

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <valarray>

using namespace std;

#include "AudioFile.h"
#include "WindowedSinc.h"

//	These values will be changeable programatically at some point.
#define TEMP_FREQ	1000
#define TEMP_SLOPE	100
#define PROGRESS_WIDTH 70.0

inline float_t dither()
{
	static random_device                      rd;
	static mt19937                            gen(rd());
	static uniform_real_distribution<float_t> low(-1, 0);
	static uniform_real_distribution<float_t> hi(0, 1);
	return low(gen) + hi(gen); // This creates triangular dither with values from -1.0 to 1.0.
}

int main(int argc, char **argv)
{
	auto            exitVal	= EXIT_SUCCESS;
	uint_fast64_t	s = 0;	//	Index variable for samples.

	try {
		//  Check for input parameter.
		if (argc < 2) {
			println("Applies high-pass FIR filter to WAV file with cutoff at 20Hz.");
			return exitVal;
		}

		//	THESE WILL BE PULLED FROM OPTIONS IN THE FUTURE.
		//	freq == cutoff frequency. The response is -6dB at this frequency.
		//	A freq of 20Hz and slope of 20Hz means that 30Hz is 0dB and 10Hz is
		//	approximately -80dB with ripple.
		//	Ripple is only in the stop band using Blackman or Hamming windows.
		const float64_t freq  = TEMP_FREQ;    //	Fc = freq / sampleRate
		const float64_t slope = TEMP_SLOPE;    //	transition ~= slope / sampleRate


		//  Loop over file names.
		for (uint32_t a = 1; a < argc; a++) {
			//	open and read file
			auto audioFile = AudioFile::Make(argv[a]);
			audioFile.ReadSamples();
			
			//	create sinc kernal
			WindowedSinc sinc((freq / audioFile.GetSampleRate()), (slope / audioFile.GetSampleRate()));
			sinc.ApplyBlackman();
			sinc.MakeIntoHighPass();

			auto Mo2 = (int32_t) sinc.Get_M() / 2;

			//	define temporary output buffer[samps][chan],
			//		before normalizing and reducing to original number of bits
			valarray<long double>	tempOutput(audioFile.GetNumSamples());
			
			uint32_t progressCount = 0;
			float32_t progress;
			uint16_t progressPos;
			
			cout << fixed << setprecision(1) << flush;
            cout << "Processing file: " << audioFile.file.string() << endl;

			//	for each channel in frame
			for (uint16_t chan = 0; chan < audioFile.GetNumChannels(); chan++) {
				//	for each input sample in channel
				for (s = chan; s < audioFile.GetNumSamples(); s += audioFile.GetNumChannels()) {
					//	Accumulator. Initialized to zero for each time through loop.
					long double acc = 0.0;

					//	for each sinc member
					for (int32_t m = -Mo2; m <= Mo2; m++) {
						uint_fast64_t	smc = s + m * audioFile.GetNumChannels();

						if (smc < 0 || smc >= audioFile.GetNumSamples()) { continue; }

						acc = fmal(audioFile.samples[smc], sinc[m], acc);
					}
					
					//	tempOutput[s][chan] = acc
					tempOutput[s] = (float32_t) acc;

					//	progress bar, do only every x samples
					if (progressCount % 7919 == 0) {
						progress	= (float32_t) progressCount / (float32_t) audioFile.GetNumSamples();
						progressPos	= (uint16_t) round((float32_t) PROGRESS_WIDTH * progress);

						cout << "\r" << "["
						     << string(progressPos, '=') << ">" << string(PROGRESS_WIDTH - progressPos, ' ')
						     << "] " << (progress * 100) + 0.05 << " % " << flush;
					}
					progressCount++;
				}	//	End loop over samples.
			}	//	End loop over channels.
			
			cout << "\r" << "[" << std::string(PROGRESS_WIDTH + 1, '=') << "] 100.0 %       " << endl;
			std::cout.flush();


			//	Only apply adjustment and dither to PCM data.
			if ( audioFile.GetDataEncoding() == 'PCM ' ) {
				//	Normalize.
				float32_t maxVal = fmaxf(abs(tempOutput.max()), abs(tempOutput.min()));
				
				//	Maximum stored == 2^(nBits-1) - 1
				//	multiply adjustment on every member of tempOutput
				switch (audioFile.GetBitsPerSample()) {
					case 8:
						tempOutput *= (127.0 / maxVal);
					break;
					
					case 16:
						tempOutput *= (32767.0 / maxVal);
					break;
					
					case 24:
						tempOutput *= (8388607.0 / maxVal);
					break;
				}
				
				//	add different dither value to each tempOutput value
				for (s = 0; s < audioFile.GetNumSamples(); s++) {
					tempOutput[s] += dither();
				}
			}
			
			//	copy result back to our original file
			//  copy(begin(tempOutput), end(tempOutput), begin(audioFile.samples));
			for (s = 0; s < audioFile.GetNumSamples(); s++) {
				audioFile.samples[s] = tempOutput[s];
			}
			tempOutput.resize(0);
			audioFile.WriteSamples();

		} //  End loop over file names.
	} // End try
	catch (exception &e) {
		cerr << e.what() << endl;
		exitVal = EXIT_FAILURE;
	}
	catch (exception *e) {
		cerr << e->what() << endl;
		exitVal = EXIT_FAILURE;
	}
	
	return exitVal;

} // End main
