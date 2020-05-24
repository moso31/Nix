#pragma once
#include "NXIntersection.h"
#include "NXScene.h"

class NXIntegrator
{
public:
	NXIntegrator();
	~NXIntegrator();

	// ������wo�ĳ�������ʡ�
	Vector3 Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth);
	Vector3 SpecularReflect(const Ray& ray, const NXHit& hit, const shared_ptr<NXScene>& pScene, int depth);
	Vector3 SpecularTransmit(const Ray& ray, const NXHit& hit, const shared_ptr<NXScene>& pScene, int depth);

private:

};