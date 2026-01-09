//
// Created by Reid Woodbury on 5/22/25.
//

#ifndef DISKERROR_PROGRESSBAR_H
#define DISKERROR_PROGRESSBAR_H

#include <iostream>
#include <string>
#include <mutex>
#include <atomic>
#include <vector>

namespace Diskerror {
using namespace std;


class ProgressBar {
	const float          _goal;
	const unsigned short _interval;
	const unsigned short _bar_width;

	unsigned long long _step{0};
	unsigned short _counter{0};

public:
	ProgressBar(const float goal, const unsigned short interval, const unsigned short bar_width = 80) :
	_goal(goal), _interval(interval), _bar_width(bar_width) {
		cout << fixed << setprecision(1) << flush;
	}

	~ProgressBar() = default;

	void Update() {
		//	progress bar, do only every x samples
		if (++_counter >= _interval) {
			_counter = 0;
			const auto progress    = _step / _goal;
			const auto progressPos = static_cast<unsigned short>(round(_bar_width * progress));

			cout << "\r" << "["
					<< string(progressPos, '=') << ">" << string(_bar_width - progressPos, ' ')
					<< "] " << (progress * 100) << " %  " << flush;
		}

		_step++;
	}

	void Final() const {
		cout << "\r" << "[" << string(_bar_width + 1, '=') << "] 100.0 %        " << endl;
		cout.flush();
	}

	void Clear() { _step = 0; };
};

// A thread-safe wrapper/helper for progress reporting
class ThreadSafeProgress {
	ProgressBar&        _bar;
	std::mutex          _mutex;
	std::atomic<size_t> _counter{0};
	const size_t        _total;
	const size_t        _report_interval;

public:
	ThreadSafeProgress(ProgressBar& bar, size_t total)
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

}

#endif //DISKERROR_PROGRESSBAR_H
