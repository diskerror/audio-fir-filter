//
// Created by Reid Woodbury.
//

#ifndef DISKERROR_FILTERCORE_H
#define DISKERROR_FILTERCORE_H

#include <cmath>
#include <cstdint>
#include <boost/cstdfloat.hpp>

#include <AudioSamples.h>
#include <VectorMath.h>
#include <WindowedSinc.h>
#include "ProgressBar.h"

namespace Diskerror {

// Extracted FIR filtering for a range of samples (channel agnostic)
inline void apply_filter_range(
		AudioSamples&            audio_samples,
		WindowedSinc<float64_t>& sinc,
		VectorMath<float32_t>&   temp_output,
		int_fast64_t             startIdx,
		int_fast64_t             endIdx,
		ThreadSafeProgress*      progress
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

} // namespace Diskerror

#endif // DISKERROR_FILTERCORE_H
