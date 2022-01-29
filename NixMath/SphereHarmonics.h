#pragma once
#include "SimpleMath.h"

namespace DirectX
{

namespace SimpleMath
{
namespace SH
{
	// Ԥ�洢10���ڵ� �׳� �� ˫�׳ˣ�������SH��г�������ֵļ���
	constexpr static float factorial[] = { 1.0, 1.0, 2.0, 6.0, 24.0, 120.0, 720.0, 5040.0, 40320.0, 362880.0, 3628800.0 };
	constexpr static float doubleFactorial[] = { 1.0, 1.0, 2.0, 3.0, 8.0, 15.0, 48.0, 105.0, 384.0, 945.0, 3840.0 };

	inline float Factorial(int x)
	{
		assert(x >= 0 && x < 10);
		return factorial[x];
	}

	inline float DoubleFactorial(int x)
	{
		assert(x >= 0 && x < 10);
		return doubleFactorial[x];
	}

	// ���� K_l^m
	float NormalizationFactor(float l, float m);

	// ���� P_l^m(x)
	float Legendre(int l, int m, float x);

	// ���� y_l^m(\theta, \phi)
	float SHBasis(int l, int m, float theta, float phi);
}
}
}