#pragma once
#include "SimpleMath.h"

namespace DirectX
{

namespace SimpleMath
{
namespace SH
{
	// 预存储10以内的 阶乘 和 双阶乘，仅用于SH球谐函数部分的计算
	constexpr static float factorial[] = { 1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800 }; 
	constexpr static float doubleFactorial[] = { 1, 1, 2, 3, 8, 15, 48, 105, 384, 945, 3840 };

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