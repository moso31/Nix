#include "NXPhotonMappingIntegrator.h"
#include "NXRandom.h"
#include "NXCamera.h"

NXPhotonMappingIntegrator::NXPhotonMappingIntegrator()
{
}

NXPhotonMappingIntegrator::~NXPhotonMappingIntegrator()
{
}

void NXPhotonMappingIntegrator::GeneratePhotons(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pCamera)
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
		bool bIsSpecular = false;
		while (true)
		{
			NXHit hitInfo;
			bool bIsIntersect = pScene->RayCast(ray, hitInfo);
			if (!bIsIntersect)
				break;

			hitInfo.ConstructReflectionModel();

			Vector3 nextDirection;
			float pdfBSDF;
			shared_ptr<ReflectionType> outReflectType = make_shared<ReflectionType>();
			Vector3 f = hitInfo.BSDF->Sample_f(-ray.direction, nextDirection, pdfBSDF, REFLECTIONTYPE_ALL, outReflectType);
			bIsSpecular = (*outReflectType & REFLECTIONTYPE_SPECULAR);
			outReflectType.reset();

			if (!bIsSpecular)
			{
				if (f.IsZero() || pdfBSDF == 0) break;
				Vector3 reflectance = f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdfBSDF;

				// Roulette
				float q = max(0, 1.0f - reflectance.GetGrayValue());
				float random = NXRandom::GetInstance()->CreateFloat();
				if (random < q) break;

				throughput *= reflectance / (1 - q);

				// make new photon
				NXPhoton photon;
				photon.position = hitInfo.position;
				photon.direction = hitInfo.direction;
				photon.power = throughput * numPhotonsInv;
				photon.depth = depth;
				photons.push_back(photon);
			}
			
			ray = Ray(hitInfo.position, nextDirection);
			ray.position += ray.direction * NXRT_EPSILON;

			depth++;
		}
	}

	m_pKdTree.reset();
	m_pKdTree = make_shared<NXKdTree>();
	m_pKdTree->BuildBalanceTree(photons);
	// use kd-tree manage all photons.
	printf("done.\n");
}

Vector3 NXPhotonMappingIntegrator::Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth)
{
	if (!m_pKdTree)
	{
		printf("Error: Couldn't find photon map data!\n");
		return Vector3(0.0f);
	}

	NXHit hitInfo;
	Vector3 L(0.0f);
	
	if (pScene->RayCast(ray, hitInfo))
	{
		Vector3 pos = hitInfo.position;

		float distSqr;
		// 大根堆，负责记录pos周围的最近顶点。
		priority_quque_NXPhoton nearestPhotons([pos](NXPhoton* photonA, NXPhoton* photonB) {
			float distA = Vector3::DistanceSquared(pos, photonA->position);
			float distB = Vector3::DistanceSquared(pos, photonB->position);
			return distA < distB;
		});

		m_pKdTree->GetNearest(pos, distSqr, nearestPhotons, 500, 100.0f);

		hitInfo.ConstructReflectionModel();

		Vector3 wo = -ray.direction;
		float photonCount = (float)nearestPhotons.size();
		Vector3 result(0.0f);

		float area = 1.0f;
		if (photonCount)
		{
			float rr = Vector3::DistanceSquared(pos, nearestPhotons.top()->position);
			area = rr * XM_PI;
			//printf("%f\n", sqrtf(rr));
		}
		else
		{
			//printf("no radius!\n");
		}
		while (!nearestPhotons.empty())
		{
			auto photon = nearestPhotons.top();
			float pdfBSDF = hitInfo.BSDF->Pdf(wo, photon->direction);
			result += photon->power * hitInfo.BSDF->f(wo, photon->direction); // *hitInfo.shading.normal.Dot(wo) / pdfBSDF;
			nearestPhotons.pop();
		}
		L += result / area;
	}

	return L;
}
