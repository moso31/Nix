#pragma once
#include "NXKdTree.h"

class NXPMIntegrator
{
public:
	NXPMIntegrator();
	~NXPMIntegrator();

	void GeneratePhotons(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pCamera);
	Vector3 Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth);

private:
	shared_ptr<NXKdTree> m_pKdTree;
	vector<NXPhoton> m_photons;
};