#pragma once
#include "SimpleMath.h"

namespace DirectX
{

namespace SimpleMath
{
namespace SH
{
	// 预存储10以内的 阶乘 和 双阶乘，仅用于SH球谐函数部分的计算
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

	// 计算 K_l^m
	float NormalizationFactor(float l, float m);

	// 计算 P_l^m(x)
	float Legendre(int l, int m, float x);

	// 计算 y_l^m(\theta, \phi)
	float SHBasis(int l, int m, float theta, float phi);
}
}
}