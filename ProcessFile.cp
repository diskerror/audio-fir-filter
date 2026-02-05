//
// Created by Reid Woodbury.
//

#include "ProcessFile.h"

#include <iostream>
#include <vector>
#include <thread>
#include <functional>

#include "FilterCore.h"
#include <AudioFile.h>
#include <AudioFormat.h>
#include <AudioSamples.h>
#include "ProgressBar.h"
#include <VectorMath.h>
#include <WindowedSinc.h>

using namespace std;
using namespace boost;

namespace Diskerror {

void process_file(const filesystem::path& input_path, const filesystem::path& output_path, const FilterOptions& opts) {
	// Show status when verbose is true.
	std::function<void(string const&)> show_status = [](string const&) { return; };
	if (opts.verbose) show_status = [](string const& s) { cout << s << endl; };

	//	Open input and read samples.
	show_status("Opening input file.");
	AudioFile   inFile(input_path);
	AudioFormat inFormat(inFile);

	cout << "Processing file: " << input_path.filename().string() << endl;

	show_status("Reading samples.");
	AudioSamples inSamples(&inFile, &inFormat);
	AudioBuffer  buf = inSamples.readAll();

	const auto numChannels    = static_cast<size_t>(inFormat.channels());
	const auto numFrames      = buf[0].size();

	//	Create windowed sinc kernel.
	show_status("Creating sinc kernel for this file's sample rate.");
	WindowedSinc<float64_t>
		sinc(opts.freq / inFormat.sampleRate(), opts.slope / inFormat.sampleRate());
	sinc.makeLowCut();

	//	Filter each channel (deinterleaved).
	show_status("Filtering.");
	for (size_t ch = 0; ch < numChannels; ++ch) {
		VectorMath<float32_t> temp_output(numFrames);

		// Create progress bar and thread-safe wrapper.
		ProgressBar progress_bar(static_cast<float32_t>(numFrames), 7919);
		ThreadSafeProgress safe_progress(progress_bar, numFrames);

		// Partition work across threads.
		std::vector<std::thread> threads;
		threads.reserve(opts.num_threads);

		const auto totalSamples = static_cast<int_fast64_t>(numFrames);
		const int_fast64_t chunkSize = totalSamples / opts.num_threads;

		for (unsigned int i = 0; i < opts.num_threads; ++i) {
			int_fast64_t start = i * chunkSize;
			int_fast64_t end   = (i == opts.num_threads - 1) ? totalSamples : (start + chunkSize);

			threads.emplace_back(
				apply_filter_range,
				std::cref(buf[ch]),
				std::cref(sinc),
				std::ref(temp_output),
				start,
				end,
				&safe_progress);
		}

		for (auto& t : threads) {
			t.join();
		}

		progress_bar.Final();

		// Copy result back into buffer channel.
		buf[ch] = std::move(temp_output);
	}

	// Normalize if any channel exceeds 1.0 or --normalize flag is set.
	float32_t maxMag = 0.0f;
	for (size_t ch = 0; ch < numChannels; ++ch) {
		float32_t chMax = buf[ch].max_mag();
		if (chMax > maxMag) maxMag = chMax;
	}

	if (maxMag > 1.0f || opts.normalize) {
		show_status("Doing audio normalize.");
		AudioSamples::normalize(buf);
	}

	// Create output file with same type, copy all chunks from input.
	show_status("Writing output file.");
	AudioFile outFile(output_path, inFile.type());

	for (size_t i = 0; i < inFile.chunkCount(); ++i) {
		auto chunkSpan = inFile.chunk(i);
		outFile.addChunk(chunkSpan.data());
	}

	outFile.flush();

	// Write filtered samples.
	AudioFormat  outFormat(outFile);
	AudioSamples outSamples(&outFile, &outFormat);
	outSamples.writeAll(buf, true);

	show_status("");
}

} // namespace Diskerror
