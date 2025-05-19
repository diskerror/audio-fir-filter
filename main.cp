//
// Created by Reid Woodbury.
//

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <getopt.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <valarray>
#include <boost/math/cstdfloat/cstdfloat_types.hpp>

using namespace std;
using namespace boost;

#include "AudioFile.h"
#include "WindowedSinc.h"

//	Default values.
#define TEMP_FREQ    20
#define TEMP_SLOPE    20

#define PROGRESS_WIDTH 80.0


int main(int argc, char **argv)
{
	auto exitVal = EXIT_SUCCESS;

	try {
		int           opt            = 0;
		struct option long_options[] = {
			{"help",      no_argument,       nullptr, 'h'},
			{"frequency", required_argument, nullptr, 'f'},
			{"slope",     required_argument, nullptr, 's'},
			{"normalize", no_argument,       nullptr, 'n'},
			{nullptr, 0,                     nullptr, 0}
		};

		bool help      = false;
		bool normalize = false;

		//	freq == cutoff frequency. The response is -6dB at this frequency.
		//	A freq of 20Hz and slope of 20Hz means that 30Hz is 0dB and 10Hz is
		//	approximately -80dB with ripple.
		//	Ripple is only in the stop band using Blackman or Hamming windows.
		float64_t freq  = TEMP_FREQ;    //	Fc = freq / sampleRate
		float64_t slope = TEMP_SLOPE;   //	transition ~= slope / sampleRate

		while ( opt != -1 ) {
			int option_index = 0;
			opt = getopt_long(argc, argv, "hf:s:n", long_options, &option_index);

			switch ( opt ) {
				case 'h':
					help = true;
					break;

				case 'f':
					freq = atof(optarg);
					break;

				case 's':
					slope = atof(optarg);
					break;

				case 'n':
					normalize = true;
					break;

				case ':':
					cerr << "option needs a value" << endl;
					break;

				case '?':
					if ( optopt == 'f' || optopt == 's' ) {
						throw runtime_error("Argument missing.");
					}

					throw runtime_error("Unknown option.");

				default:
					break;
			}
		}

		//  Check for input parameter.
		if ( optind >= argc || help ) {
			cout << "Applies low-cut (high-pass) FIR filter to WAVE or AIFF file." << endl;
			cout << "    -f, --frequency <Hz>         Filter cutoff frequency." << endl;
			cout << "    -s, --slope <Hz>             Filter slope." << endl;
			cout << "    -n, --normalize              Normalize output to maximum bit depth for PCM files." << endl;
			cout << "    -h, --help                   Display this help message." << endl;
			return exitVal;
		}

		uint16_t      chan = 0; //	Index variable for channels.
		uint_fast64_t s    = 0; //	Index variable for samples.
		int_fast32_t  m    = 0; //	Index variable for coefficients.

		//  Loop over file names.
		for ( uint32_t a = optind; a < argc; a++ ) {
			//	open and read file
			auto audioFile = Diskerror::AudioFile(argv[a]);
			audioFile.ReadSamples();

			//	create windowed sinc kernal
			Diskerror::WindowedSinc<long double>
				sinc(freq / audioFile.GetSampleRate(), slope / audioFile.GetSampleRate());
			sinc.ApplyBlackman();
			sinc.MakeLowCut();
//			for_each(sinc.begin(), sinc.end(), [](long double &elem) { cout << elem << endl; });
//			cout << "sum " << sinc.sum() << endl;
//			return 1;

			auto Mo2 = (int32_t) sinc.size() / 2;

			//	define temporary output buffer[samps][chan],
			valarray<float32_t> tempOutput(audioFile.GetNumSamples());

			uint32_t  progressCount = 0;
			float32_t progress;
			uint16_t  progressPos;

			cout << fixed << setprecision(1) << flush;
			cout << "Processing file: " << audioFile.file.string() << endl;

			//	for each channel in frame
			for ( chan = 0; chan < audioFile.GetNumChannels(); chan++ ) {
				//	for each input sample in channel
				for ( s = chan; s < audioFile.GetNumSamples(); s += audioFile.GetNumChannels() ) {
					//	Accumulator. Initialized to zero for each time through loop.
					long double acc = 0.0;

					//	for each sinc member
					for ( m = -Mo2; m <= Mo2; m++ ) {
						uint_fast64_t smc = s + m * audioFile.GetNumChannels();

						if ( smc < 0 || smc >= audioFile.GetNumSamples() ) { continue; }

						acc = fmal(audioFile.samples[smc], sinc[m], acc);
					}

					//	tempOutput[s][chan] = acc
					tempOutput[s] = (float32_t) acc;

					//	progress bar, do only every x samples
					if ( progressCount % 7919 == 0 ) {
						progress    = (float32_t) progressCount / (float32_t) audioFile.GetNumSamples();
						progressPos = (uint16_t) round((float32_t) PROGRESS_WIDTH * progress);

						cout << "\r" << "["
						     << string(progressPos, '=') << ">" << string(PROGRESS_WIDTH - progressPos, ' ')
						     << "] " << (progress * 100) + 0.05 << " % " << flush;
					}
					progressCount++;
				}    //	End loop over samples.
			}    //	End loop over channels.

			cout << "\r" << "[" << string(PROGRESS_WIDTH + 1, '=') << "] 100.0 %       " << endl;
			cout.flush();


			//	Only apply adjustment and dither to PCM data.
			if ( audioFile.GetDataEncoding() == 'PCM ' ) {
				//	Normalize if new sample stream goes over maximum sample size.
				//  A DC offset in one direction may cause overflow in the other direction when removed.
				float32_t maxVal = fmaxf(abs(tempOutput.max()), abs(tempOutput.min()));

				//	Maximum stored == 2^(nBits-1) - 1
				//	multiply adjustment on every member of tempOutput
				float32_t max_possible = 0;
				switch ( audioFile.GetBitsPerSample() ) {
					case 8:
						max_possible = Diskerror::AudioFile::MAX_VALUE_8_BIT;
						break;

					case 16:
						max_possible = Diskerror::AudioFile::MAX_VALUE_16_BIT;
						break;

					case 24:
						max_possible = Diskerror::AudioFile::MAX_VALUE_24_BIT;
						break;

					default:
						throw runtime_error("Can't handle PCM files with this bit depth.");
				}

				if ( maxVal > max_possible || normalize ) {
					tempOutput *= (max_possible / maxVal);
				}

				//  We can't simply use "tempOutput += dither();", without the loop,
				//  because we need dither() to return a different value for each sample.
				for ( s = 0; s < audioFile.GetNumSamples(); s++ ) {
					tempOutput[s] += Diskerror::AudioFile::dither();
				}
			}

			//	copy result back to our original file
			//  copy(begin(tempOutput), end(tempOutput), begin(audioFile.samples));
			for ( s = 0; s < audioFile.GetNumSamples(); s++ ) {
				audioFile.samples[s] = tempOutput[s];
			}
			tempOutput.resize(0);
			audioFile.WriteSamples();

		} //  End loop over file names.
	} // End try
	catch ( ... ) {
		exitVal = EXIT_FAILURE;
	}

	return exitVal;

} // End main
