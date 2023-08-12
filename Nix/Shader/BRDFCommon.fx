#ifndef _BRDFCOMMON_
#define _BRDFCOMMON_

#include "MathUnits.fx"

struct BSDFValue
{
	float3 Diffuse;
	float3 Specular;
};

float Pow5(float x)
{
	float x2 = x * x;
	return x * x2 * x2;
}

#endif // !_BRDFCOMMON_