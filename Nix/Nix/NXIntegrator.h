#pragma once
#include "NXIntersection.h"
#include "NXScene.h"

class NXIntegrator
{
public:
	NXIntegrator();
	~NXIntegrator();

	// 求射线wo的出射辐射率。
	Vector3 Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth);
	Vector3 DirectRadiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth);
	Vector3 DirectEstimate(const Ray& ray, const shared_ptr<NXScene>& pScene, const shared_ptr<NXPBRLight>& pLight, const NXHit& hitInfo);
	Vector3 SpecularReflect(const Ray& ray, const NXHit& hit, const shared_ptr<NXScene>& pScene, int depth);
	Vector3 SpecularTransmit(const Ray& ray, const NXHit& hit, const shared_ptr<NXScene>& pScene, int depth);

private:

};
