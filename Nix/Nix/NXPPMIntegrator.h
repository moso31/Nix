#pragma once
#include "NXIntegrator.h"
#include "NXPhoton.h"
#include "NXPPMPixel.h"

class NXPPMIntegrator : public NXIntegrator
{
public:
	NXPPMIntegrator();
	~NXPPMIntegrator();

	Vector3 Radiance(const Ray& ray, const std::shared_ptr<NXScene>& pScene, int depth) override;

private:
	std::shared_ptr<NXPhotonMap> m_pPhotonMap;
	PPMPixel m_pixelInfo;
};
