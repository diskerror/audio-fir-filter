//
// Created by Reid Woodbury.
//

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <getopt.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <boost/cstdfloat.hpp>

using namespace std;
using namespace boost;

#include <AudioSamples.h>
#include "ProgressBar.h"
#include <VectorMath.h>
#include <WindowedSinc.h>

//	Default values.
#define TEMP_FREQ  15
#define TEMP_SLOPE 10

void ApplyFIRFilterToChannel(
		Diskerror::AudioSamples&            audioSamples,
		Diskerror::WindowedSinc<float64_t>& sinc,
		Diskerror::VectorMath<float32_t>&   tempOutput,
		uint16_t                            channel,
		Diskerror::ProgressBar&             progressBar
	);


int main(const int argc, char** argv) {
	auto exitVal = EXIT_SUCCESS;

	try {
		int          opt            = 0;
		const option long_options[] = {
			{"help", no_argument, nullptr, 'h'},
			{"frequency", required_argument, nullptr, 'f'},
			{"slope", required_argument, nullptr, 's'},
			{"normalize", no_argument, nullptr, 'n'},
			{"verbose", no_argument, nullptr, 'v'},
			{nullptr, 0, nullptr, 0}
		};

		bool help      = false;
		bool normalize = false;
		bool verbose   = false;

		//	freq == cutoff frequency. The response is -6dB at this frequency.
		//	A freq of 20 Hz and slope of 20 Hz means that 30 Hz is 0 dB and 10 Hz is
		//	approximately -80dB with ripple.
		//	Ripple is only in the stop band using Blackman or Hamming windows.
		float64_t freq  = TEMP_FREQ;  //	Fc = freq / sampleRate
		float64_t slope = TEMP_SLOPE; //	transition ~= slope / sampleRate

		while (opt != -1) {
			int option_index = 0;
			opt              = getopt_long(argc, argv, "hf:s:nv", long_options, &option_index);

			switch (opt) {
				case 'h':
					help = true;
					opt = -1; //  cause the while-loop to stop
					break;

				case 'f':
					freq = strtod(optarg, nullptr);
					break;

				case 's':
					slope = strtod(optarg, nullptr);
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
					if (optopt == 'f' || optopt == 's') {
						throw invalid_argument("Argument missing.");
					}

					throw invalid_argument("Unknown option.");

				default:
					break;
			}
		}

		//  Check for input parameter.
		if (optind >= argc || help) {
			cout << "Applies low-cut (high-pass) FIR filter to WAVE or AIFF file." << endl;
			cout << "    -f, --frequency <Hz>   Filter cutoff frequency." << endl;
			cout << "    -s, --slope <Hz>       Filter slope width." << endl;
			cout << "    -n, --normalize        Normalize output to maximum bit depth for PCM files." << endl;
			cout << "    -v, --verbose          Verbose output." << endl;
			cout << "    -h, --help             Display this help message." << endl;
			return exitVal;
		}

		// Show status when verbose is true.
		std::function<void(string const&)> showStatus = [](string const&) { return; };
		if (verbose) showStatus = [](string const& s) { cout << s << endl; };

		//  Loop over file names.
		for (int a = optind; a < argc; a++) {
			//	open and read file
			showStatus("Opening file");
			Diskerror::AudioSamples audioSamples(argv[a]);

			cout << "Processing file: " << audioSamples.getFileName() << endl;
			showStatus("Reading file");
			audioSamples.ReadSamples();

			//	create windowed sinc kernal
			showStatus("Creating sinc kernal for this file's sample rate.");
			Diskerror::WindowedSinc<float64_t>
				sinc(freq / audioSamples.getSampleRate(), slope / audioSamples.getSampleRate());
			sinc.ApplyBlackman();
			sinc.MakeLowCut();

			//	define temporary output buffer[chan][samps],
			showStatus("Creating temporary buffer.");
			Diskerror::VectorMath<float32_t>
				tempOutput(audioSamples.getNumSamples());

			Diskerror::ProgressBar
				progressBar(static_cast<float32_t>(audioSamples.getNumSamples()), 7919);

			for (uint16_t channel = 0; channel < audioSamples.getNumChannels(); channel++) {
				ApplyFIRFilterToChannel(audioSamples, sinc, tempOutput, channel, progressBar);
			}

			progressBar.Final();

			// copy result back to our original file
			showStatus("Copying data back to file's buffer.");
			copy(tempOutput.begin(), tempOutput.end(), audioSamples.begin());
			tempOutput.resize(0);

			// Normalize if new sample stream goes over maximum sample size.
			// A DC offset in one direction may cause overflow in the other direction when removed.
			if (audioSamples.max_mag() > audioSamples.getSampleMaxMagnitude() || normalize) {
				showStatus("Doing audio normalize.");
				audioSamples.Normalize();
			}

			showStatus("Converting and writing samples back to file.");
			audioSamples.WriteSamples(true);
			audioSamples.clear();
			audioSamples.shrink_to_fit();

			showStatus("");
		} //  End loop over file names.
	}     // End try
	catch (const std::exception& e) {
		cerr << e.what() << endl;
		exitVal = EXIT_FAILURE;
	}
	catch (...) {
		cerr << "Caught an unknown exception." << endl;
		exitVal = EXIT_FAILURE;
	}

	return exitVal;
} // End main

// Extracted FIR filtering for a single channel
void ApplyFIRFilterToChannel(
		Diskerror::AudioSamples&            audioSamples,
		Diskerror::WindowedSinc<float64_t>& sinc,
		Diskerror::VectorMath<float32_t>&   tempOutput,
		uint16_t                            channel,
		Diskerror::ProgressBar&             progressBar
	) {
	const uint16_t     numChannels = audioSamples.getNumChannels();
	const int_fast64_t numSamples  = audioSamples.getNumSamples();
	const auto         halfSinc    = static_cast<int_fast32_t>(sinc.getMo2());

	// For each input sample in the channel
	for (int_fast64_t sampleIdx = channel; sampleIdx < numSamples; sampleIdx += numChannels) {
		float_fast64_t accumulator       = 0.0;
		int_fast64_t   filteredSampleIdx = 0;

		// For each sinc member
		for (int_fast32_t m = -halfSinc; m <= halfSinc; ++m) {
			filteredSampleIdx = sampleIdx + m * numChannels;
			if (filteredSampleIdx < 0 || filteredSampleIdx >= numSamples) {
				continue;
			}
			accumulator = fmal(audioSamples[filteredSampleIdx], sinc[m + halfSinc], accumulator);
		}

		tempOutput[sampleIdx] = static_cast<float32_t>(accumulator);
		progressBar.Update();
	}
}
