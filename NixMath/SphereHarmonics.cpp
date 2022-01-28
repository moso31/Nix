#include "SphereHarmonics.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace DirectX::SimpleMath::SH;

float DirectX::SimpleMath::SH::NormalizationFactor(float l, float m)
{
    double temp = ((2.0 * l + 1.0) * Factorial(l - m)) / (XM_4PI * Factorial(l + m));
    return sqrt(temp);

}

float DirectX::SimpleMath::SH::Legendre(int l, int m, float x)
{
    float pmm = 1.0;
    if (m > 0) {
        float somx2 = sqrt((1.0f - x) * (1.0f + x));
        float fact = 1.0f;
        for (int i = 1; i <= m; i++) {
            pmm *= (-fact) * somx2;
            fact += 2.0f;
        }
    }
    if (l == m)
        return pmm;
    float pmmp1 = x * (2.0f * m + 1.0f) * pmm;
    if (l == m + 1)
        return pmmp1;
    float pll = 0.0;
    for (int ll = m + 2; ll <= l; ++ll) {
        pll = ((2.0f * ll - 1.0f) * x * pmmp1 - (ll + m - 1.0f) * pmm) / (ll - m);
        pmm = pmmp1;
        pmmp1 = pll;
    }
    return pll;
}

float DirectX::SimpleMath::SH::SHBasis(int l, int m, float theta, float phi)
{
    const double sqrt2 = sqrt(2.0);
    if (m == 0) return NormalizationFactor(l, 0) * Legendre(l, m, cos(theta));
    else if (m > 0) return sqrt2 * NormalizationFactor(l, m) * cos(m * phi) * Legendre(l, m, cos(theta));
    else return sqrt2 * NormalizationFactor(l, -m) * sin(-m * phi) * Legendre(l, -m, cos(theta));

	//float Klm = NormalizationFactor(l, m);
	//float sqrt2 = sqrt(2.0f);

	//if (m > 0)
	//	return sqrt2 * Klm * cos(m * phi) * Legendre(l, m, cos(theta));
	//
	//if (m < 0)
	//	return sqrt2 * Klm * sin(-m * phi) * Legendre(l, -m, cos(theta));

	//return Klm * Legendre(l, m, cos(theta));
}
