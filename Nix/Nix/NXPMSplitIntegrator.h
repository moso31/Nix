#pragma once
#include "NXIntegrator.h"
#include "NXPhoton.h"

class NXPMSplitIntegrator : public NXIntegrator
{
public:
	NXPMSplitIntegrator(const shared_ptr<NXPhotonMap>& pGlobalPhotons, const shared_ptr<NXPhotonMap>& pCausticPhotons);
	~NXPMSplitIntegrator();

	Vector3 Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth) override;

private:
	shared_ptr<NXPhotonMap> m_pGlobalPhotonMap;
	shared_ptr<NXPhotonMap> m_pCausticPhotonMap;
	bool m_bUseIrradianceCache;
};