#pragma once
#include "NXIntegrator.h"
#include "NXKdTree.h"

class NXPMIntegrator : public NXIntegrator
{
public:
	NXPMIntegrator();
	~NXPMIntegrator();

	void GeneratePhotons(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pCamera);
	Vector3 Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth) override;

private:
	shared_ptr<NXKdTree> m_pKdTree;
	int m_numPhotons;
};