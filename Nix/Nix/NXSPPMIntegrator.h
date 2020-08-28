#pragma once
#include "NXIntegrator.h"
#include "NXPhoton.h"
#include "NXBSDF.h"

struct NXSPPMPixel
{
	NXSPPMPixel() : flux(0.0f), photons(0), radius2(0.0f), Ld(0.0f) {}
	~NXSPPMPixel() {}

	Vector3 Ld;
	float radius2;
	UINT photons;
	Vector3 flux;
};

class NXSPPMIntegrator : public NXIntegrator
{
public:
	NXSPPMIntegrator(const XMINT2& imageSize, std::string outPath, UINT nPhotons);
	~NXSPPMIntegrator() {}

	void Render(const std::shared_ptr<NXScene>& pScene);
	void RefreshPhotonMap(const std::shared_ptr<NXScene>& pScene);

private:
	std::shared_ptr<NXPhotonMap> m_pPhotonMap;
	std::string m_outFilePath;
	UINT m_numPhotonsEachStep;	// 每次步进所需的光子数量
};

