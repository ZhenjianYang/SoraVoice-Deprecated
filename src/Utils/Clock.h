#pragma once

namespace Clock
{
	constexpr int TimeUnitsPerSecond = 1000;

	void InitClock(unsigned &now, unsigned &recent);

	void UpdateTime();
	const unsigned& Now();
	const unsigned& Recent();

	void Sleep(unsigned sleepTime);
};

