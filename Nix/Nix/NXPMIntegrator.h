#pragma once
#include "NXIntegrator.h"
#include "NXPhoton.h"

class NXPMIntegrator : public NXIntegrator
{
public:
	NXPMIntegrator(const std::shared_ptr<NXPhotonMap>& pGlobalPhotons);
	~NXPMIntegrator();

	Vector3 Radiance(const Ray& ray, const std::shared_ptr<NXScene>& pScene, int depth) override;

private:
	std::shared_ptr<NXPhotonMap> m_pPhotonMap;
};