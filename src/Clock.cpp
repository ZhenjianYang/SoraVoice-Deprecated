#include "Time.h"

#include <chrono>

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
using TimeUnit = std::chrono::milliseconds;

static unsigned* now = nullptr;
static unsigned* recent = nullptr;
static TimePoint base;

static void InitClock(unsigned &now, unsigned &recent) {
	::now = &now;
	::recent = &recent;
	base = Clock::now();
	now = recent = 0;
}

static void UpdateTime() {
	TimePoint newNow = Clock::now();
	*recent = *now;
	*now = (unsigned)std::chrono::duration_cast<TimeUnit>(newNow - base).count();
}
static const unsigned& Now() { return now; };
static const unsigned& Recent() { return recent; };

static void Sleep(unsigned sleepTime){
	std::this_thread::sleep_for(TimeUnit(sleepTime));
}
