#pragma once
#include "NXIntegrator.h"
#include "NXPhoton.h"

class NXPMIntegrator : public NXIntegrator
{
public:
	NXPMIntegrator(const shared_ptr<NXPhotonMap>& pGlobalPhotons);
	~NXPMIntegrator();

	Vector3 Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth) override;

private:
	shared_ptr<NXPhotonMap> m_pPhotonMap;
};