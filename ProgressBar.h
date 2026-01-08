//
// Created by Reid Woodbury on 5/22/25.
//

#ifndef DISKERROR_PROGRESSBAR_H
#define DISKERROR_PROGRESSBAR_H

#include <iostream>
#include <string>

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
}

#endif //DISKERROR_PROGRESSBAR_H
