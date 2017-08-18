#include "Clock.h"

#include <chrono>
#include <thread>

using StdClock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
using TimeUnit = std::chrono::milliseconds;

namespace CL {
	static unsigned* now = nullptr;
	static unsigned* recent = nullptr;
	static TimePoint base;
}

void Clock::InitClock(unsigned &now, unsigned &recent) {
	CL::now = &now;
	CL::recent = &recent;
	CL::base = StdClock::now();
	now = recent = 0;
}

void Clock::UpdateTime() {
	TimePoint newNow = StdClock::now();
	*CL::recent = *CL::now;
	*CL::now = (unsigned)std::chrono::duration_cast<TimeUnit>(newNow - CL::base).count();
}
const unsigned& Clock::Now() { return *CL::now; };
const unsigned& Clock::Recent() { return *CL::recent; };

void Clock::Sleep(unsigned sleepTime) {
	std::this_thread::sleep_for(TimeUnit(sleepTime));
}

