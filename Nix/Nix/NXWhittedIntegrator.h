#pragma once
#include "NXIntegrator.h"

class NXWhittedIntegrator : public NXIntegrator
{
public:
	// ������wo�ĳ�������ʡ�
	Vector3 Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth) override;

private:
};