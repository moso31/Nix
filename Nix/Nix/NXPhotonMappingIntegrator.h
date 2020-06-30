#pragma once
#include "NXIntegrator.h"

struct NXPhoton
{
	Vector3 position;
	Vector3 direction;
	Vector3 power;
};

class NXPhotonMappingIntegrator : public NXIntegrator
{
public:
	NXPhotonMappingIntegrator();
	~NXPhotonMappingIntegrator();
	void GeneratePhotons(const shared_ptr<NXScene>& pScene);
	Vector3 Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth) override;

private:
	int m_iPhotonCount;
	vector<NXPhoton> m_photons;
};