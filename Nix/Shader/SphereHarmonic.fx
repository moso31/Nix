#include "MathUnits.fx"

// 预存储10以内的 阶乘 和 双阶乘，仅用于SH球谐函数部分的计算
const static float factorial[] = { 1.0, 1.0, 2.0, 6.0, 24.0, 120.0, 720.0, 5040.0, 40320.0, 362880.0, 3628800.0 };
const static float doubleFactorial[] = { 1.0, 1.0, 2.0, 3.0, 8.0, 15.0, 48.0, 105.0, 384.0, 945.0, 3840.0 };

inline float Factorial(int x)
{
	return factorial[x];
}

inline float DoubleFactorial(int x)
{
	return doubleFactorial[x];
}

// K_l^m
float NormalizationFactor(float l, float m)
{
	return sqrt(
		(2.0f * l + 1.0f) * Factorial(l - abs(m)) /
		(NX_4PI * Factorial(l + abs(m)))
	);
}

// P_l^m(x)
float Legendre(int l, int m, float x)
{
	// evaluate an Associated Legendre Polynomial P(l,m,x) at x
	float pmm = 1.0;
	if (m > 0) {
		float somx2 = sqrt((1.0 - x) * (1.0 + x));
		float fact = 1.0;
		for (int i = 1; i <= m; i++) {
			pmm *= (-fact) * somx2;
			fact += 2.0;
		}
	}
	if (l == m) return pmm;
	float pmmp1 = x * (2.0 * m + 1.0) * pmm;
	if (l == m + 1) return pmmp1;
	float pll = 0.0;
	for (int ll = m + 2; ll <= l; ++ll) {
		pll = ((2.0 * ll - 1.0) * x * pmmp1 - (ll + m - 1.0) * pmm) / (ll - m);
		pmm = pmmp1;
		pmmp1 = pll;
	}
	return pll;
}


// y_l^m(\theta, \phi)
float SHBasis(int l, int m, float theta, float phi)
{
	float Klm = NormalizationFactor(l, m);
	float sqrt2 = sqrt(2.0f);

	if (m > 0)
		return sqrt2 * Klm * cos(m * phi) * Legendre(l, m, cos(theta));

	if (m < 0)
		return sqrt2 * Klm * sin(-m * phi) * Legendre(l, -m, cos(theta));

	return Klm * Legendre(l, m, cos(theta));
}