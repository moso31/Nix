#pragma once
#include "NXIntegrator.h"

class NXPathIntegrator : public NXIntegrator
{
public:
	Vector3 Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth) override;

private:
};