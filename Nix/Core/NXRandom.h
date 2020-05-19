#pragma once
#include <random>
#include "Header.h"
#include "NXInstance.h"

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
	default_random_engine m_rng;
};
