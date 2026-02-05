//
// Created by Reid Woodbury.
//

#ifndef DISKERROR_FILTERCORE_H
#define DISKERROR_FILTERCORE_H

#include <cmath>
#include <cstdint>
#include <boost/cstdfloat.hpp>

#include <VectorMath.h>
#include <WindowedSinc.h>
#include "ProgressBar.h"

namespace Diskerror {
using namespace boost;

// FIR filtering for a range of samples on a single deinterleaved channel.
inline void apply_filter_range(
		const VectorMath<float32_t>&   channel,
		const WindowedSinc<float64_t>& sinc,
		VectorMath<float32_t>&         temp_output,
		int_fast64_t                   startIdx,
		int_fast64_t                   endIdx,
		ThreadSafeProgress*            progress
	) {
	const auto numSamples = static_cast<int_fast64_t>(channel.size());
	const auto halfSinc   = static_cast<int_fast32_t>(sinc.getMo2());

	// Safe bounds where filter kernel fits entirely
	const int_fast64_t startSafe = halfSinc;
	const int_fast64_t endSafe   = numSamples - halfSinc;

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

	// Loop 1: Prologue (left edge, partial kernel overlap)
	for (; sampleIdx < endIdx && sampleIdx < startSafe; ++sampleIdx) {
		int32_t overlap = static_cast<int32_t>(sampleIdx + halfSinc + 1);
		temp_output[sampleIdx] = static_cast<float32_t>(sinc.fms(channel.begin(), -overlap));
		report_progress();
	}

	// Loop 2: Main Body (full kernel overlap)
	int_fast64_t safeLimit = (endIdx < endSafe) ? endIdx : endSafe;

	for (; sampleIdx < safeLimit; ++sampleIdx) {
		temp_output[sampleIdx] = static_cast<float32_t>(sinc.fms(channel.begin() + sampleIdx - halfSinc));
		report_progress();
	}

	// Loop 3: Epilogue (right edge, partial kernel overlap)
	for (; sampleIdx < endIdx; ++sampleIdx) {
		int32_t remaining = static_cast<int32_t>(numSamples - sampleIdx + halfSinc);
		temp_output[sampleIdx] = static_cast<float32_t>(sinc.fms(channel.begin() + sampleIdx - halfSinc, remaining));
		report_progress();
	}

	flush_progress();
}

} // namespace Diskerror

#endif // DISKERROR_FILTERCORE_H
