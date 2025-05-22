//
// Created by Reid Woodbury.
//

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <boost/cstdfloat.hpp>

using namespace std;
using namespace boost;

#include "AudioFile.h"
#include "VectorMath.h"
#include "WindowedSinc.h"

//	Default values.
#define TEMP_FREQ  15
#define TEMP_SLOPE 10

#define PROGRESS_WIDTH 80.0

void status(bool verbose, string s)
{
	if ( verbose ) cout << s << endl;
}


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
			{"verbose",   no_argument,       nullptr, 'v'},
			{nullptr, 0,                     nullptr, 0}
		};

		bool help      = false;
		bool normalize = false;
		bool verbose   = false;

		//	freq == cutoff frequency. The response is -6dB at this frequency.
		//	A freq of 20Hz and slope of 20Hz means that 30Hz is 0dB and 10Hz is
		//	approximately -80dB with ripple.
		//	Ripple is only in the stop band using Blackman or Hamming windows.
		double freq  = TEMP_FREQ;    //	Fc = freq / sampleRate
		double slope = TEMP_SLOPE;   //	transition ~= slope / sampleRate

		while ( opt != -1 ) {
			int option_index = 0;
			opt = getopt_long(argc, argv, "hf:s:nv", long_options, &option_index);

			switch ( opt ) {
				case 'h':
					help = true;
					opt  = -1;  //  cause the while-loop to stop
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

				case 'v':
					verbose = true;
					break;

				case ':':
					cerr << "option needs a value" << endl;
					break;

				case '?':
					if ( optopt == 'f' || optopt == 's' ) {
						throw invalid_argument("Argument missing.");
					}

					throw invalid_argument("Unknown option.");

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
			cout << "    -v, --verbose                Verbose output." << endl;
			cout << "    -h, --help                   Display this help message." << endl;
			return exitVal;
		}

		//  Loop over file names.
		for ( uint32_t a = optind; a < argc; a++ ) {
			//	open and read file
			auto audioFile = Diskerror::AudioFile(filesystem::path(argv[a]));
			status(verbose, "Opening and reading:");
			status(verbose, audioFile.file.filename());
			audioFile.ReadSamples();

			//	create windowed sinc kernal
			status(verbose, (string) "Creating sinc kernal for this file's sample rate.");
			Diskerror::WindowedSinc<long double>
				sinc(freq / audioFile.GetSampleRate(), slope / audioFile.GetSampleRate());
			sinc.ApplyBlackman();
			sinc.MakeLowCut();

			//	define temporary output buffer[samps][chan],
			status(verbose, (string) "Creating temporary buffer.");
			Diskerror::VectorMath<float32_t>
				tempOutput(audioFile.GetNumSamples());

			uint64_t  progressCount = 0;
			float32_t progress;
			uint16_t  progressPos;

			cout << fixed << setprecision(1) << flush;
			cout << "Processing file: " << audioFile.file.filename() << endl;

			//	for each channel in frame
			for ( uint16_t chan = 0; chan < audioFile.GetNumChannels(); chan++ ) {
				//	for each input sample in channel
				for ( uint_fast64_t s = chan; s < audioFile.GetNumSamples(); s += audioFile.GetNumChannels() ) {
					//	Accumulator. Initialized to zero for each time through loop.
					long double acc = 0.0;

					//	for each sinc member
					for ( int_fast32_t m = -sinc.getMo2(); m <= sinc.getMo2(); m++ ) {
						int_fast64_t smc = s + m * audioFile.GetNumChannels();

						if ( smc < 0 || smc >= audioFile.GetNumSamples() ) { continue; }

						acc = fmal(audioFile.samples[smc], sinc[m + sinc.getMo2()], acc);
					}

					//	tempOutput[s][chan] = acc
					tempOutput[s] = (float32_t) acc;

					//	progress bar, do only every x samples
					if ( progressCount % 7919 == 0 ) {
						progress    = (float32_t) progressCount / (float32_t) audioFile.GetNumSamples();
						progressPos = (uint16_t) round((float32_t) PROGRESS_WIDTH * progress);

						cout << "\r" << "["
						     << string(progressPos, '=') << ">" << string(PROGRESS_WIDTH - progressPos, ' ')
						     << "] " << (progress * 100) + 0.05 << " %  " << flush;
					}
					progressCount++;
				}    //	End loop over samples.
			}    //	End loop over channels.

			cout << "\r" << "[" << string(PROGRESS_WIDTH + 1, '=') << "] 100.0 %        " << endl;
			cout.flush();

			//	copy result back to our original file
			status(verbose, (string) "Copying data back to file's sample buffer.");
			for ( uint_fast64_t s = 0; s < audioFile.GetNumSamples(); ++s ) audioFile.samples[s] = tempOutput[s];
			tempOutput.resize(0);

			//	Normalize if new sample stream goes over maximum sample size.
			//  A DC offset in one direction may cause overflow in the other direction when removed.
			if ( audioFile.samples.max_mag() > audioFile.GetSampleMaxMagnitude() || normalize ) {
				status(verbose, (string) "Doing audio normalize.");
				audioFile.Normalize();
			}

			status(verbose, (string) "Converting and writing samples back to file.");
			audioFile.WriteSamples();

			status(verbose, "");
		} //  End loop over file names.
	} // End try
	catch ( const std::exception & e ) {
		cerr << e.what() << endl;
		exitVal = EXIT_FAILURE;
	}

	return exitVal;

} // End main
