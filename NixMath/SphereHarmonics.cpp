#include "SphereHarmonics.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace DirectX::SimpleMath::SH;

float DirectX::SimpleMath::SH::NormalizationFactor(float l, float m)
{
	return sqrt(
		(2.0f * l + 1.0f) * Factorial(l - abs(m)) /
		(XM_4PI * Factorial(l + abs(m)))
	);
}

float DirectX::SimpleMath::SH::Legendre(int l, int m, float x)
{
	if (l == 0 && m == 0) return 1.0f;

	if (l == 1 && m == 0) return x;

	if (l == m)
		return (l & 1 ? -1.0f : 1.0f) * DoubleFactorial(2.0f * l - 1.0f) * pow(1.0f - (x * x), 0.5f * l);

	if (l - m == 1)
		return x * (2.0f * m + 1) * Legendre(l - 1, m, x);

	return ((2.0f * l - 1.0f) * x * Legendre(l - 1, m, x) - (l + m - 1) * Legendre(l - 2, m, x)) / (l - m);
}

float DirectX::SimpleMath::SH::SHBasis(int l, int m, float theta, float phi)
{
	float Klm = NormalizationFactor(l, m);
	float sqrt2 = sqrt(2.0f);

	if (m > 0)
		return sqrt2 * Klm * cos(m * phi) * Legendre(l, m, cos(theta));
	
	if (m < 0)
		return sqrt2 * Klm * sin(-m * phi) * Legendre(l, -m, cos(theta));

	return Klm * Legendre(l, m, cos(theta));
}
