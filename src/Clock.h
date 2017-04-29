#pragma once

class Clock
{
public:
	static constexpr int TimeUnitsPerSecond = 1000;

	static void InitClock(unsigned &now, unsigned &recent);

	static void UpdateTime();
	static const unsigned& Now();
	static const unsigned& Recent();
	static void Sleep(unsigned sleepTime);

private:
	virtual ~Clock() = 0;
};

