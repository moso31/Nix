#pragma once
#include "NXIntegrator.h"
#include "NXPhoton.h"

class NXPMIntegrator : public NXSampleIntegrator
{
public:
	NXPMIntegrator(const XMINT2& imageSize, int eachPixelSamples, std::string outPath, UINT nPhotons, UINT nEstimatePhotons);
	~NXPMIntegrator();

	void Render(NXScene* pScene);
	Vector3 Radiance(const Ray& ray, NXScene* pScene, int depth);

	void BuildPhotonMap(NXScene* pScene);
	float GaussianFilter(float distance2, float radius2);

private:
	NXPhotonMap* m_pPhotonMap;
	UINT m_numPhotons;			// 光子数量
	UINT m_estimatePhotons;		// 每次密度估计的最近光子数量
};