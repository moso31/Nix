#pragma once
#include "Header.h"
#include <random>
#include "NXInstance.h"

using RandomFloatDistribution = std::uniform_real_distribution<float>;
using RandomIntDistribution = std::uniform_int_distribution<int>;

class NXRandom : public	NXInstance<NXRandom>
{
public:
	NXRandom();
	~NXRandom();

	int CreateInt(int minValue, int maxValue);
	float CreateFloat(float minValue = 0.0f, float maxValue = 1.0f);
	Vector2 CreateVector2(float minValue = 0.0f, float maxValue = 1.0f);
	Vector3 CreateVector3(float minValue = 0.0f, float maxValue = 1.0f);
	Vector4 CreateVector4(float minValue = 0.0f, float maxValue = 1.0f);

private:
	int m_rngSeed;
	std::default_random_engine m_rng;
};
