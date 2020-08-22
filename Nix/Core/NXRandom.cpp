#include "NXRandom.h"

NXRandom::NXRandom() :
	m_rngSeed(0),
	m_rng(std::default_random_engine(m_rngSeed))
{
}

NXRandom::~NXRandom()
{
}

int NXRandom::CreateInt(int minValue, int maxValue)
{
	RandomIntDistribution r(minValue, maxValue);
	return r(m_rng);
}

float NXRandom::CreateFloat(float minValue, float maxValue)
{
	RandomFloatDistribution r(minValue, maxValue);
	return r(m_rng);
}

Vector2 NXRandom::CreateVector2(float minValue, float maxValue)
{
	RandomFloatDistribution r(minValue, maxValue);
	return Vector2(r(m_rng), r(m_rng));
}

Vector3 NXRandom::CreateVector3(float minValue, float maxValue)
{
	RandomFloatDistribution r(minValue, maxValue);
	return Vector3(r(m_rng), r(m_rng), r(m_rng));
}

Vector4 NXRandom::CreateVector4(float minValue, float maxValue)
{
	RandomFloatDistribution r(minValue, maxValue);
	return Vector4(r(m_rng), r(m_rng), r(m_rng), r(m_rng));
}
