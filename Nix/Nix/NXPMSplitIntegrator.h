#pragma once
#include "NXIntegrator.h"
#include "NXIrradianceCache.h"

class NXPMSplitIntegrator : public NXIntegrator
{
public:
	NXPMSplitIntegrator(const shared_ptr<NXPhotonMap>& pGlobalPhotons, const shared_ptr<NXPhotonMap>& pCausticPhotons);
	~NXPMSplitIntegrator();

	void SetIrradianceCache(shared_ptr<NXIrradianceCache>& pIrradianceCache) { m_pIrradianceCache = pIrradianceCache; }
	Vector3 Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth) override;

private:
	shared_ptr<NXPhotonMap> m_pGlobalPhotonMap;
	shared_ptr<NXPhotonMap> m_pCausticPhotonMap;
	shared_ptr<NXIrradianceCache> m_pIrradianceCache;
};