#pragma once
#include "NXIntegrator.h"
#include "NXIrradianceCache.h"

class NXPMSplitIntegrator : public NXSampleIntegrator
{
public:
	NXPMSplitIntegrator(const XMINT2& imageSize, int eachPixelSamples, std::string outPath, UINT nCausticPhotons, UINT nGlobalPhotons);
	~NXPMSplitIntegrator();

	void Render(const std::shared_ptr<NXScene>& pScene);
	Vector3 Radiance(const Ray& ray, const std::shared_ptr<NXScene>& pScene, int depth);

private:
	void BuildPhotonMap(const std::shared_ptr<NXScene>& pScene);
	void BuildIrradianceCache(const std::shared_ptr<NXScene>& pScene);

private:
	std::shared_ptr<NXPhotonMap> m_pGlobalPhotonMap;
	std::shared_ptr<NXPhotonMap> m_pCausticPhotonMap;
	std::shared_ptr<NXIrradianceCache> m_pIrradianceCache;

	UINT m_numCausticPhotons;
	UINT m_numGlobalPhotons;
};
