#pragma once
#include "NXIntegrator.h"
#include "NXIrradianceCache.h"

class NXPMSplitIntegrator : public NXIntegrator
{
public:
	NXPMSplitIntegrator(const std::shared_ptr<NXPhotonMap>& pGlobalPhotons, const std::shared_ptr<NXPhotonMap>& pCausticPhotons);
	~NXPMSplitIntegrator();

	void SetIrradianceCache(std::shared_ptr<NXIrradianceCache>& pIrradianceCache) { m_pIrradianceCache = pIrradianceCache; }
	Vector3 Radiance(const Ray& ray, const std::shared_ptr<NXScene>& pScene, int depth) override;

private:
	std::shared_ptr<NXPhotonMap> m_pGlobalPhotonMap;
	std::shared_ptr<NXPhotonMap> m_pCausticPhotonMap;
	std::shared_ptr<NXIrradianceCache> m_pIrradianceCache;
};