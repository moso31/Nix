#include "NXPMIntegrator.h"
#include "NXScene.h"
#include "NXRandom.h"

NXPMIntegrator::NXPMIntegrator()
{
}

NXPMIntegrator::~NXPMIntegrator()
{
}

void NXPMIntegrator::GeneratePhotons(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pCamera)
{
	vector<NXPhoton> photons;

	printf("Generate photons...");
	int numPhotons = 1000000;
	float numPhotonsInv = 1.0f / (float)numPhotons;
	for (int i = 0; i < numPhotons; i++)
	{
		auto pLights = pScene->GetPBRLights();
		int lightCount = (int)pLights.size();
		int sampleLight = NXRandom::GetInstance()->CreateInt(0, lightCount - 1);

		float pdfLight = 1.0f / lightCount;
		float pdfPos, pdfDir;
		Vector3 throughput;
		Ray ray;
		Vector3 lightNormal;
		Vector3 Le = pLights[sampleLight]->SampleEmissionRadiance(ray, lightNormal, pdfPos, pdfDir);
		throughput = Le * fabsf(lightNormal.Dot(ray.direction)) / (pdfLight * pdfPos * pdfDir);

		int depth = 0;
		int maxDepth = 5;
		while (depth < maxDepth)
		{
			NXHit hitInfo;
			if (!pScene->RayCast(ray, hitInfo))
				break;

			hitInfo.GenerateBSDF(false);
		}
	}

	m_pKdTree.reset();
	m_pKdTree = make_shared<NXKdTree>();
	m_pKdTree->BuildBalanceTree(m_photons);
	m_photons.clear();
	// use kd-tree manage all photons.
	printf("done.\n");
}

Vector3 NXPMIntegrator::Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth)
{
	return Vector3(0.0f);
}
