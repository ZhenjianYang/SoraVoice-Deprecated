#include "Clock.h"

#include <chrono>
#include <thread>

using StdClock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
using TimeUnit = std::chrono::milliseconds;

static unsigned* now = nullptr;
static unsigned* recent = nullptr;
static TimePoint base;

void Clock::InitClock(unsigned &now, unsigned &recent) {
	::now = &now;
	::recent = &recent;
	base = StdClock::now();
	now = recent = 0;
}

void Clock::UpdateTime() {
	TimePoint newNow = StdClock::now();
	*recent = *now;
	*now = (unsigned)std::chrono::duration_cast<TimeUnit>(newNow - base).count();
}
const unsigned& Clock::Now() { return *now; };
const unsigned& Clock::Recent() { return *recent; };

void Clock::Sleep(unsigned sleepTime){
	std::this_thread::sleep_for(TimeUnit(sleepTime));
}
