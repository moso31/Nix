#pragma once
#include "Header.h"

class NXTimer
{
public:
	NXTimer();
	~NXTimer();

	int64_t GetTimeDelta();
	void Tick();

private:
	time_point<steady_clock> m_lastTime;
	int64_t m_timeDelta;
	int64_t m_global_time;
};

