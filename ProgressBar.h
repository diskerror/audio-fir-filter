//
// Created by Reid Woodbury on 5/22/25.
//

#ifndef DISKERROR_PROGRESSBAR_H
#define DISKERROR_PROGRESSBAR_H

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

namespace Diskerror {

using namespace std;

class ProgressBar {

	const float          goal;
	const unsigned short interval;
	const unsigned short bar_width;

	unsigned long long step = 0;

public:

	ProgressBar(const float goal, const unsigned short interval, const unsigned short bar_width = 80) :
		goal(goal), interval(interval), bar_width(bar_width)
	{
		cout << fixed << setprecision(1) << flush;
	}

	~ProgressBar() = default;

	void Update()
	{
		//	progress bar, do only every x samples
		if ( this->step % this->interval == 0 ) {
			auto progress    = (float) step / this->goal;
			auto progressPos = (unsigned short) (((float) this->bar_width * progress) + 0.5);

			cout << "\r" << "["
			     << string(progressPos, '=') << ">" << string(this->bar_width - progressPos, ' ')
			     << "] " << (progress * 100) << " %  " << flush;
		}

		step++;
	}

	void Final()
	{
		cout << "\r" << "[" << string(this->bar_width + 1, '=') << "] 100.0 %        " << endl;
		cout.flush();
	}

	void Clear() { step = 0; };

};

}

#endif //DISKERROR_PROGRESSBAR_H
