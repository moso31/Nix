#pragma once
#include "NXIntegrator.h"

class NXPathIntegrator : public NXSampleIntegrator
{
public:
	NXPathIntegrator(const XMINT2& imageSize, int eachPixelSamples, std::string outPath);
	Vector3 Radiance(const Ray& ray, const std::shared_ptr<NXScene>& pScene, int depth) override;
};