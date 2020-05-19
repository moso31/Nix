#include "NXRandom.h"

NXRandom::NXRandom() :
	m_rngSeed(0),
	m_rng(default_random_engine(m_rngSeed))
{
}

NXRandom::~NXRandom()
{
}

int NXRandom::CreateInt(int minValue, int maxValue)
{
	uniform_int_distribution<int> r(minValue, maxValue);
	return r(m_rng);
}

float NXRandom::CreateFloat(float minValue, float maxValue)
{
	uniform_real_distribution<float> r(minValue, maxValue);
	return r(m_rng);
}

Vector2 NXRandom::CreateVector2(float minValue, float maxValue)
{
	uniform_real_distribution<float> r(minValue, maxValue);
	return Vector2(r(m_rng), r(m_rng));
}

Vector3 NXRandom::CreateVector3(float minValue, float maxValue)
{
	uniform_real_distribution<float> r(minValue, maxValue);
	return Vector3(r(m_rng), r(m_rng), r(m_rng));
}

Vector4 NXRandom::CreateVector4(float minValue, float maxValue)
{
	uniform_real_distribution<float> r(minValue, maxValue);
	return Vector4(r(m_rng), r(m_rng), r(m_rng), r(m_rng));
}
