#pragma once
#include "NXIntegrator.h"
#include "NXPhoton.h"
#include "NXBSDF.h"

struct NXSPPMPixel
{
	NXSPPMPixel() {}
	~NXSPPMPixel() {}

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

