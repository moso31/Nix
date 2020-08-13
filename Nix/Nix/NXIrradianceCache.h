#pragma once
#include "SimpleMath.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

struct NXIrradianceCache
{
	Vector3 position;
	Vector3 normal;
	float HarmonicDistance;
	Vector3 Irradiance;
};

