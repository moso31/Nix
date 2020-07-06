#pragma once
#include "NXIntegrator.h"
#include "NXKdTree.h"

class NXPhotonMappingIntegrator : public NXIntegrator
{
public:
	NXPhotonMappingIntegrator();
	~NXPhotonMappingIntegrator();

	void GeneratePhotons(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pCamera);
	Vector3 Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth) override;

private:
	shared_ptr<NXKdTree> m_pKdTree;
};