#include "NXTimer.h"

NXTimer::NXTimer() :
	m_global_time(0)
{
}

NXTimer::~NXTimer()
{
}

int64_t NXTimer::GetTimeDelta()
{
	return m_timeDelta;
}

void NXTimer::Tick()
{
	auto currTime = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currTime - m_lastTime);
	m_timeDelta = duration.count();
	m_global_time += m_timeDelta;
	m_lastTime = currTime;
}
