#pragma once
#include "NXIntegrator.h"
#include "NXPhoton.h"
#include "NXBSDF.h"

struct NXSPPMPixel
{
	NXSPPMPixel() {}
	~NXSPPMPixel() {}

	XMINT2 pixel;	// 像素坐标
	float pixelWeight;	// 权重
	Vector3 position;
	Vector3 normal;
	std::shared_ptr<NXBSDF> BSDF;
	Vector3 direction;
	float radius2;
	UINT photons;
	Vector3 flux;
	Vector3 Lemit;	// 自发光（如果有的话）
};

class NXSPPMIntegrator : public NXIntegrator
{
public:
	NXSPPMIntegrator() {}
	~NXSPPMIntegrator() {}

	void RefreshPhotonMap(const std::shared_ptr<NXScene>& pScene);
	void Render(const std::shared_ptr<NXScene>& pScene);

private:
	std::shared_ptr<NXPhotonMap> m_pPhotonMap;
};

