#pragma once
#include <stdint.h>
#include <chrono>
using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

class NXTimer
{
public:
	NXTimer();
	~NXTimer();

	int64_t GetTimeDelta();
	int64_t GetGlobalTime();
	void Tick();

private:
	TimePoint m_lastTime;
	int64_t m_timeDelta;

	int64_t m_globalTime;
};

