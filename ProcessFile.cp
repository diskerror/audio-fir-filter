//
// Created by Reid Woodbury.
//

#include "ProcessFile.h"

#include <iostream>
#include <vector>
#include <thread>
#include <functional>
#include <cmath>

#include "FilterCore.h"
#include <AudioSamples.h>
#include "ProgressBar.h"
#include <VectorMath.h>
#include <WindowedSinc.h>

using namespace std;
using namespace boost;

namespace Diskerror {

void process_file(const filesystem::path& file_path, const FilterOptions& opts) {
	// Show status when verbose is true.
	std::function<void(string const&)> show_status = [](string const&) { return; };
	if (opts.verbose) show_status = [](string const& s) { cout << s << endl; };

	//	open and read file
	show_status("Opening file");
	AudioSamples audio_samples(file_path);

	cout << "Processing file: " << audio_samples.getFileName() << endl;
	show_status("Reading file");
	audio_samples.ReadSamples();

	//	create windowed sinc kernal
	show_status("Creating sinc kernal for this file's sample rate.");
	WindowedSinc<float64_t>
		sinc(opts.freq / audio_samples.getSampleRate(), opts.slope / audio_samples.getSampleRate());
	sinc.ApplyBlackman();
	sinc.MakeLowCut();

	//	define temporary output buffer[chan][samps],
	show_status("Creating temporary buffer.");
	VectorMath<float32_t>
		temp_output(audio_samples.getNumSamples());

	// Create progress bar and thread-safe wrapper
	ProgressBar
		progress_bar(static_cast<float32_t>(audio_samples.getNumSamples()), 7919);
	ThreadSafeProgress safe_progress(progress_bar, audio_samples.getNumSamples());

	// Partition work
	std::vector<std::thread> threads;
	threads.reserve(opts.num_threads);

	const int_fast64_t totalSamples = audio_samples.getNumSamples();
	const int_fast64_t chunkSize    = totalSamples / opts.num_threads;

	for (unsigned int i = 0; i < opts.num_threads; ++i) {
		int_fast64_t start = i * chunkSize;
		int_fast64_t end   = (i == opts.num_threads - 1) ? totalSamples : (start + chunkSize);

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
	if (audio_samples.max_mag() > audio_samples.getSampleMaxMagnitude() || opts.normalize) {
		show_status("Doing audio normalize.");
		audio_samples.Normalize();
	}

	show_status("Converting and writing samples back to file.");
	audio_samples.WriteSamples(true);
	audio_samples.clear();
	audio_samples.shrink_to_fit();

	show_status("");
}

} // namespace Diskerror
