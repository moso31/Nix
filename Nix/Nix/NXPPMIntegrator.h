#pragma once
#include "NXIntegrator.h"
#include "NXPhoton.h"
#include "NXBSDF.h"

struct PPMPixel
{
	XMINT2 pixel;
	Vector3 position;
	Vector3 normal;
	std::shared_ptr<NXBSDF> BSDF;
	Vector3 direction;
	float radius2;
	UINT photons;
	Vector3 flux;
};

class NXPPMIntegrator : public NXIntegrator
{
public:
	NXPPMIntegrator();
	~NXPPMIntegrator();

	Vector3 Radiance(const Ray& ray, const std::shared_ptr<NXScene>& pScene, int depth) override;

private:
	std::shared_ptr<NXPhotonMap> m_pPhotonMap;
	PPMPixel m_pixelInfo;
};
