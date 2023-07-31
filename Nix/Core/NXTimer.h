#pragma once
#include "Header.h"

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

