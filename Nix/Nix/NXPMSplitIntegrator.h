#pragma once
#include "NXIntegrator.h"
#include "NXIrradianceCache.h"

class NXPMSplitIntegrator : public NXSampleIntegrator
{
public:
	NXPMSplitIntegrator(const XMINT2& imageSize, int eachPixelSamples, std::string outPath, UINT nCausticPhotons, UINT nGlobalPhotons);
	~NXPMSplitIntegrator();

	void Render(NXScene* pScene);
	Vector3 Radiance(const Ray& ray, NXScene* pScene, int depth);

private:
	void BuildPhotonMap(NXScene* pScene);
	void BuildIrradianceCache(NXScene* pScene);

private:
	NXPhotonMap* m_pGlobalPhotonMap;
	NXPhotonMap* m_pCausticPhotonMap;
	NXIrradianceCache* m_pIrradianceCache;

	UINT m_numCausticPhotons;
	UINT m_numGlobalPhotons;
};
