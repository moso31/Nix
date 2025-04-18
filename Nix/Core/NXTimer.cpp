#include "NXTimer.h"

NXTimer::NXTimer() :
	m_globalTime(0),
	m_lastTime(std::chrono::steady_clock::now())
{
}

NXTimer::~NXTimer()
{
}

float NXTimer::GetGlobalTimeSeconds()
{
	return (float)m_globalTime / 1000.0f;
}

int64_t NXTimer::GetTimeDelta()
{
	return m_timeDelta;
}

int64_t NXTimer::GetGlobalTime()
{
	return m_globalTime;
}

void NXTimer::Tick()
{
	auto currTime = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currTime - m_lastTime);
	m_timeDelta = duration.count();
	m_globalTime += m_timeDelta;

	m_lastTime = currTime;
}
