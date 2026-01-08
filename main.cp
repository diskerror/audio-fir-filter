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
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <cmath>
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

// A thread-safe wrapper/helper for progress reporting
class ThreadSafeProgress {
	Diskerror::ProgressBar& _bar;
	std::mutex              _mutex;
	std::atomic<size_t>     _counter{0};
	const size_t            _total;
	const size_t            _report_interval;

public:
	ThreadSafeProgress(Diskerror::ProgressBar& bar, size_t total)
		: _bar(bar), _total(total), _report_interval(total / 100 > 1000 ? total / 100 : 1000) {}

	void report(size_t count) {
		size_t old_val = _counter.fetch_add(count, std::memory_order_relaxed);
		size_t new_val = old_val + count;

		// Only acquire lock and update UI periodically to reduce contention
		if ((new_val / _report_interval) > (old_val / _report_interval) || new_val == _total) {
			std::lock_guard<std::mutex> lock(_mutex);
			// Update the progress bar "count" times.
			// Since we batch updates, we loop here. This is acceptable as the loop count is small
			// relative to the FIR filter complexity.
			for (size_t i = 0; i < count; ++i) _bar.Update();
		}
	}
};

void apply_filter_range(
		Diskerror::AudioSamples&            audio_samples,
		Diskerror::WindowedSinc<float64_t>& sinc,
		Diskerror::VectorMath<float32_t>&   temp_output,
		int_fast64_t                        startIdx,
		int_fast64_t                        endIdx,
		ThreadSafeProgress*                 progress
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
			{"threads", required_argument, nullptr, 't'},
			{nullptr, 0, nullptr, 0}
		};

		bool         help        = false;
		bool         normalize   = false;
		bool         verbose     = false;
		unsigned int num_threads = floorl(std::thread::hardware_concurrency() * 0.7);
		if (num_threads == 0) num_threads = 4; // Fallback

		float64_t freq  = TEMP_FREQ;  //	Fc = freq / sampleRate
		float64_t slope = TEMP_SLOPE; //	transition ~= slope / sampleRate

		while (opt != -1) {
			int option_index = 0;
			opt              = getopt_long(argc, argv, "hf:s:nvt:", long_options, &option_index);

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

				case 't': {
					long val = strtol(optarg, nullptr, 10);
					if (val > 0) num_threads = static_cast<unsigned int>(val);
				}
				break;

				case ':':
					cerr << "option needs a value" << endl;
					break;

				case '?':
					if (optopt == 'f' || optopt == 's' || optopt == 't') {
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
			cout << "    -t, --threads <N>      Number of threads (default: auto)." << endl;
			cout << "    -h, --help             Display this help message." << endl;
			return exitVal;
		}

		// Show status when verbose is true.
		std::function<void(string const&)> show_status = [](string_view) { return; };
		if (verbose) show_status = [](string_view s) { cout << s << endl; };

		show_status(std::format("Using {} threads.", num_threads));

		//  Loop over file names.
		for (int a = optind; a < argc; a++) {
			//	open and read file
			show_status("Opening file");
			Diskerror::AudioSamples audio_samples(argv[a]);

			cout << "Processing file: " << audio_samples.getFileName() << endl;
			show_status("Reading file");
			audio_samples.ReadSamples();

			//	create windowed sinc kernal
			show_status("Creating sinc kernal for this file's sample rate.");
			Diskerror::WindowedSinc<float64_t>
				sinc(freq / audio_samples.getSampleRate(), slope / audio_samples.getSampleRate());
			sinc.ApplyBlackman();
			sinc.MakeLowCut();

			//	define temporary output buffer[chan][samps],
			show_status("Creating temporary buffer.");
			Diskerror::VectorMath<float32_t>
				temp_output(audio_samples.getNumSamples());

			// Create progress bar and thread-safe wrapper
			Diskerror::ProgressBar
				progress_bar(static_cast<float32_t>(audio_samples.getNumSamples()), 7919);
			ThreadSafeProgress safe_progress(progress_bar, audio_samples.getNumSamples());

			// Partition work
			std::vector<std::thread> threads;
			threads.reserve(num_threads);

			const int_fast64_t totalSamples = audio_samples.getNumSamples();
			const int_fast64_t chunkSize    = totalSamples / num_threads;

			for (unsigned int i = 0; i < num_threads; ++i) {
				int_fast64_t start = i * chunkSize;
				int_fast64_t end   = (i == num_threads - 1) ? totalSamples : (start + chunkSize);

				threads.emplace_back(
					apply_filter_range,
					std::ref(audio_samples),
					std::ref(sinc),
					std::ref(temp_output),
					start,
					end,
					&safe_progress);
			}

			// Wait for all threads to complete
			for (auto& t : threads) {
				t.join();
			}

			progress_bar.Final();

			// copy result back to our original file
			show_status("Copying data back to file's buffer.");
			copy(temp_output.begin(), temp_output.end(), audio_samples.begin());
			temp_output.resize(0);

			// Normalize if new sample stream goes over maximum sample size.
			// A DC offset in one direction may cause overflow in the other direction when removed.
			if (audio_samples.max_mag() > audio_samples.getSampleMaxMagnitude() || normalize) {
				show_status("Doing audio normalize.");
				audio_samples.Normalize();
			}

			show_status("Converting and writing samples back to file.");
			audio_samples.WriteSamples(true);
			audio_samples.clear();
			audio_samples.shrink_to_fit();

			show_status("");
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

// Extracted FIR filtering for a range of samples (channel agnostic)
void apply_filter_range(
		Diskerror::AudioSamples&            audio_samples,
		Diskerror::WindowedSinc<float64_t>& sinc,
		Diskerror::VectorMath<float32_t>&   temp_output,
		int_fast64_t                        startIdx,
		int_fast64_t                        endIdx,
		ThreadSafeProgress*                 progress
	) {
	const int_fast64_t numChannels = audio_samples.getNumChannels();
	const int_fast64_t numSamples  = audio_samples.getNumSamples();
	const auto         halfSinc    = static_cast<int_fast32_t>(sinc.getMo2());

	// Safe bounds where filter kernel fits entirely
	const int_fast64_t startSafe = halfSinc * numChannels;
	const int_fast64_t endSafe   = numSamples - halfSinc * numChannels;

	int_fast64_t sampleIdx = startIdx;

	// Progress batching
	const int_fast64_t progress_batch         = 2048;
	int_fast64_t       processed_since_report = 0;

	auto report_progress = [&]() {
		processed_since_report++;
		if (processed_since_report >= progress_batch) {
			if (progress) progress->report(processed_since_report);
			processed_since_report = 0;
		}
	};

	auto flush_progress = [&]() {
		if (processed_since_report > 0 && progress) {
			progress->report(processed_since_report);
			processed_since_report = 0;
		}
	};

	// Loop 1: Prologue (Unsafe region at the start of the buffer)
	// Iterate while in range AND below startSafe
	for (; sampleIdx < endIdx && sampleIdx < startSafe; ++sampleIdx) {
		float_fast64_t accumulator = 0.0;
		for (int_fast32_t m = -halfSinc; m <= halfSinc; ++m) {
			int_fast64_t filteredSampleIdx = sampleIdx + m * numChannels;
			if (filteredSampleIdx >= 0 && filteredSampleIdx < numSamples) {
				accumulator = fmal(audio_samples[filteredSampleIdx], sinc[m + halfSinc], accumulator);
			}
		}
		temp_output[sampleIdx] = static_cast<float32_t>(accumulator);
		report_progress();
	}

	// Loop 2: Main Body (Safe region)
	// Iterate while in range AND below endSafe. Start where Loop 1 left off.
	// Since sampleIdx >= startSafe (from previous loop), we just check < endSafe and < endIdx
	int_fast64_t safeLimit = (endIdx < endSafe) ? endIdx : endSafe;

	for (; sampleIdx < safeLimit; ++sampleIdx) {
		float_fast64_t accumulator = 0.0;
		for (int_fast32_t m = -halfSinc; m <= halfSinc; ++m) {
			accumulator = fmal(audio_samples[sampleIdx + m * numChannels], sinc[m + halfSinc], accumulator);
		}
		temp_output[sampleIdx] = static_cast<float32_t>(accumulator);
		report_progress();
	}

	// Loop 3: Epilogue (Unsafe region at the end of the buffer)
	// Iterate remaining
	for (; sampleIdx < endIdx; ++sampleIdx) {
		float_fast64_t accumulator = 0.0;
		for (int_fast32_t m = -halfSinc; m <= halfSinc; ++m) {
			int_fast64_t filteredSampleIdx = sampleIdx + m * numChannels;
			if (filteredSampleIdx >= 0 && filteredSampleIdx < numSamples) {
				accumulator = fmal(audio_samples[filteredSampleIdx], sinc[m + halfSinc], accumulator);
			}
		}
		temp_output[sampleIdx] = static_cast<float32_t>(accumulator);
		report_progress();
	}

	flush_progress();
}
