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
#include <boost/program_options.hpp>

using namespace std;
using namespace boost;
namespace po = boost::program_options;

#include "AudioFile.h"
#include "WindowedSinc.h"

//	These values will be changeable programatically at some point.
#define TEMP_FREQ	300
#define TEMP_SLOPE	20
#define PROGRESS_WIDTH 70.0

#define MAX_8_BIT      127.0
#define MAX_16_BIT   32767.0
#define MAX_24_BIT 8388607.0

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
	auto exitVal = EXIT_SUCCESS;

//    po::options_description optDesc("Options:");
//	optDesc.add_options()
//    	("freq,f", "Cut-off frequency of the filter. For FIR filters this is the -6dB point.")
//    	("slope,s", "Slope width of the cut-off transision.")
//    	("normalize,n", "Normalize PCM files to maximum allowed value for bit depth.")
//    	("help,h", "Applies low-cut (high-pass) FIR filter to WAVE or AIFF file.")
//    ;

//	po::positional_options_description optFiles;
//	optFiles.add("in", -1);
//
//	po::variables_map inputValues;
//	po::store(po::command_line_parser(argc, argv).options(optDesc).positional(optFiles).run(), inputValues);
//	po::notify(inputValues);
//	throw;

	try {
		//  Check for input parameter.
		if (argc < 2) {
			cout << "Applies low-cut (high-pass) FIR filter to WAVE or AIFF file with cutoff at 20Hz." << endl;
			return exitVal;
		}

		//	THESE WILL BE PULLED FROM OPTIONS IN THE FUTURE.
		//	freq == cutoff frequency. The response is -6dB at this frequency.
		//	A freq of 20Hz and slope of 20Hz means that 30Hz is 0dB and 10Hz is
		//	approximately -80dB with ripple.
		//	Ripple is only in the stop band using Blackman or Hamming windows.
		const float64_t freq  = TEMP_FREQ;    //	Fc = freq / sampleRate
		const float64_t slope = TEMP_SLOPE;    //	transition ~= slope / sampleRate

        uint16_t      chan = 0; //	Index variable for channels.
	    uint_fast64_t s    = 0; //	Index variable for samples.
        int_fast32_t  m    = 0; //	Index variable for coefficients.

		//  Loop over file names.
		for (uint32_t a = 1; a < argc; a++) {
			//	open and read file
			auto audioFile = Diskerror::AudioFile::Make(argv[a]);
			audioFile.ReadSamples();
			
			//	create sinc kernal
			Diskerror::WindowedSinc sinc((freq / audioFile.GetSampleRate()), (slope / audioFile.GetSampleRate()));
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
			for (chan = 0; chan < audioFile.GetNumChannels(); chan++) {
				//	for each input sample in channel
				for (s = chan; s < audioFile.GetNumSamples(); s += audioFile.GetNumChannels()) {
					//	Accumulator. Initialized to zero for each time through loop.
					long double acc = 0.0;

					//	for each sinc member
					for (m = -Mo2; m <= Mo2; m++) {
						uint_fast64_t smc = s + m * audioFile.GetNumChannels();

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
				//	Normalize if new sample stream goes over maximum sample size.
				//  A DC offset in one direction may cause overflow in the other direction when removed.
				float32_t maxVal = fmaxf(abs(tempOutput.max()), abs(tempOutput.min()));

				//	Maximum stored == 2^(nBits-1) - 1
				//	multiply adjustment on every member of tempOutput
				switch (audioFile.GetBitsPerSample()) {
					case 8:
					    if (maxVal > MAX_8_BIT) {
    					    tempOutput *= (MAX_8_BIT / maxVal);
					    }
					break;

					case 16:
					    if (maxVal > MAX_16_BIT) {
    					    tempOutput *= (MAX_16_BIT / maxVal);
					    }
					break;

					case 24:
					    if (maxVal > MAX_24_BIT) {
    					    tempOutput *= (MAX_24_BIT / maxVal);
					    }
					break;
				}

				//  We can't simply use "tempOutput += dither();", without the loop,
                //  because we need dither() to return a different value for each sample.
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
	catch (...) {
		exitVal = EXIT_FAILURE;
	}

	return exitVal;

} // End main
