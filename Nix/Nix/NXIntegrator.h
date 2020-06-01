#pragma once
#include "NXIntersection.h"
#include "NXScene.h"

class NXIntegrator
{
public:
	NXIntegrator();
	~NXIntegrator();

	virtual Vector3 Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth) = 0;

	/*
	对单次采样进行评估。
	如果是DeltaLight：
		仅对light提供的方向进行计算即可得到实际精确值。
	如果不是DeltaLight：
		无法得到精确值，就使用两种方法各采样一次：
			1.基于light采样一次，
			2.基于BSDF采样一次。
		之后将两次采样的结果结合，得到一个较为准确的估算值。
	*/
	Vector3 DirectEstimate(const Ray& ray, const shared_ptr<NXScene>& pScene, const shared_ptr<NXPBRLight>& pLight, const NXHit& hitInfo);
	Vector3 SpecularReflect(const Ray& ray, const NXHit& hit, const shared_ptr<NXScene>& pScene, int depth);
	Vector3 SpecularTransmit(const Ray& ray, const NXHit& hit, const shared_ptr<NXScene>& pScene, int depth);
};
