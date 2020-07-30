#include "NXPhotonMappingIntegrator.h"
#include "NXRandom.h"
#include "NXCamera.h"
#include "SamplerMath.h"

NXPhotonMappingIntegrator::NXPhotonMappingIntegrator() :
	m_numPhotons(100000)
{
}

NXPhotonMappingIntegrator::~NXPhotonMappingIntegrator()
{
}

void NXPhotonMappingIntegrator::GeneratePhotons(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pCamera)
{
	vector<NXPhoton> photons;

	printf("Generate photons...");
	float numPhotonsInv = 1.0f / (float)m_numPhotons;
	for (int i = 0; i < m_numPhotons; i++)
	{
		auto pLights = pScene->GetPBRLights();
		int lightCount = (int)pLights.size();
		int sampleLight = NXRandom::GetInstance()->CreateInt(0, lightCount - 1);

		float pdfLight = 1.0f / lightCount;
		float pdfPos, pdfDir;
		Vector3 throughput;
		Ray ray;
		Vector3 lightNormal;
		Vector3 Le = pLights[sampleLight]->Emit(ray, lightNormal, pdfPos, pdfDir);
		throughput = Le * fabsf(lightNormal.Dot(ray.direction)) / (pdfLight * pdfPos * pdfDir);

		int depth = 0;
		bool bIsDiffuse = false;
		bool bIsGlossy = false;
		while (true)
		{
			NXHit hitInfo;
			bool bIsIntersect = pScene->RayCast(ray, hitInfo);
			if (!bIsIntersect)
				break;

			hitInfo.GenerateBSDF(false);

			float pdf;
			Vector3 nextDirection;
			shared_ptr<NXBSDF::SampleEvents> sampleEvent = make_shared<NXBSDF::SampleEvents>();
			Vector3 f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
			bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
			sampleEvent.reset();

			if (f.IsZero() || pdf == 0) break;

			if (bIsDiffuse)
			{
				// make new photon
				NXPhoton photon;
				photon.position = hitInfo.position;
				photon.direction = hitInfo.direction;
				photon.power = throughput * numPhotonsInv;
				photon.depth = depth;
				m_photons.push_back(photon);

				//if (depth > 1)
				//printf("%d: %f %f %f\n", depth, photon.power.x, photon.power.y, photon.power.z);
				depth++;
			}

			Vector3 reflectance = f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;

			auto mat = hitInfo.pPrimitive->GetPBRMaterial();
			float random = NXRandom::GetInstance()->CreateFloat();

			// Roulette
			if (random > mat->m_probability)
				break;

			throughput *= reflectance / mat->m_probability;

			ray = Ray(hitInfo.position, nextDirection);
			ray.position += ray.direction * NXRT_EPSILON;
		}
	}

	m_pKdTree.reset();
	m_pKdTree = make_shared<NXKdTree>();
	m_pKdTree->BuildBalanceTree(m_photons);
	m_photons.clear();
	// use kd-tree manage all photons.
	printf("done.\n");
}

Vector3 NXPhotonMappingIntegrator::Radiance(const Ray& cameraRay, const shared_ptr<NXScene>& pScene, int depth)
{
	if (!m_pKdTree)
	{
		printf("Error: Couldn't find photon map data!\n");
		return Vector3(0.0f);
	}

	bool PHOTONS_ONLY = false;

	Ray ray(cameraRay);
	Vector3 nextDirection;
	float pdf;
	NXHit hitInfo;

	Vector3 throughput(1.0f);

	bool bIsDiffuse = false;

	while (true)
	{
		hitInfo = NXHit();
		if (!pScene->RayCast(ray, hitInfo))
			return Vector3(0.0f);

		hitInfo.GenerateBSDF(true);
		shared_ptr<NXBSDF::SampleEvents> sampleEvent = make_shared<NXBSDF::SampleEvents>();
		Vector3 f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
		bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
		sampleEvent.reset();

		if (bIsDiffuse || PHOTONS_ONLY) break;

		throughput *= f;
		ray = Ray(hitInfo.position, nextDirection);
		ray.position += ray.direction * NXRT_EPSILON;
	}

	Vector3 pos = hitInfo.position;

	float distSqr;
	// 大根堆，负责记录pos周围的最近顶点。
	priority_quque_NXPhoton nearestPhotons([pos](NXPhoton* photonA, NXPhoton* photonB) {
		float distA = Vector3::DistanceSquared(pos, photonA->position);
		float distB = Vector3::DistanceSquared(pos, photonB->position);
		return distA < distB;
		});

	if (PHOTONS_ONLY)
	{
		m_pKdTree->GetNearest(pos, distSqr, nearestPhotons, 1, 0.00005f);
		Vector3 result(0.0f);
		if (!nearestPhotons.empty())
		{
			result = nearestPhotons.top()->power;	// photon data only.
			result *= (float)m_numPhotons;
		}
		return result;
	}

	m_pKdTree->GetNearest(pos, distSqr, nearestPhotons, 100, FLT_MAX);
	if (nearestPhotons.empty())
		return Vector3(0.0f);

	float radius2 = Vector3::DistanceSquared(pos, nearestPhotons.top()->position);
	//printf("radius2 %f\n", radius2);
	Vector3 flux(0.0f);

	//printf("size: %d\n", (int)nearestPhotons.size());
	while (!nearestPhotons.empty())
	{
		auto photon = nearestPhotons.top();
		Vector3 f = hitInfo.BSDF->Evaluate(-ray.direction, photon->direction, pdf);
		flux += f * photon->power;

		//printf("%d: %f\n", (int)nearestPhotons.size(), r2);
		//printf("%f %f %f\n", photon->power.x, photon->power.y, photon->power.z);

		nearestPhotons.pop();
	}
	Vector3 result = throughput * flux / (XM_PI * radius2);
	//printf("\n");

	return result;
}
