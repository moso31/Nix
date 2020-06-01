#pragma once
#include "NXIntegrator.h"

class NXWhittedIntegrator : public NXIntegrator
{
public:
	// 求射线wo的出射辐射率。
	Vector3 Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth) override;

private:
};