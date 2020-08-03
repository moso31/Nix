#include "NXPMSplitIntegrator.h"
#include "NXRandom.h"
#include "SamplerMath.h"
#include "NXCamera.h"
#include "NXCubeMap.h"

NXPMSplitIntegrator::NXPMSplitIntegrator() :
	m_numGlobalPhotons(100000),
	m_numCausticPhotons(50000)
{
}

NXPMSplitIntegrator::~NXPMSplitIntegrator()
{
}

void NXPMSplitIntegrator::GeneratePhotons(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pCamera)
{
	vector<NXPhoton> causticPhotons, globalPhotons;

	printf("Generating global photons...");
	float numGlobalPhotonsInv = 1.0f / (float)m_numGlobalPhotons;
	for (int i = 0; i < m_numGlobalPhotons; i++)
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
				photon.power = throughput * numGlobalPhotonsInv;
				photon.depth = depth;
				globalPhotons.push_back(photon);

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

	m_pGlobalPhotons.reset();
	m_pGlobalPhotons = make_shared<NXKdTree>();
	m_pGlobalPhotons->BuildBalanceTree(globalPhotons);
	printf("done.\n");

	printf("Generating caustic photons...");
	float numCausticPhotonsInv = 1.0f / (float)m_numCausticPhotons;
	for (int i = 0; i < m_numCausticPhotons; i++)
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
		bool bIsDiffuse = true;
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

			if (bIsDiffuse && depth)
			{
				// make new photon
				NXPhoton photon;
				photon.position = hitInfo.position;
				photon.direction = hitInfo.direction;
				photon.power = throughput * numCausticPhotonsInv;
				photon.depth = depth;
				causticPhotons.push_back(photon);
				break;
			}
			
			depth++;
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

	m_pCausticPhotons.reset();
	m_pCausticPhotons = make_shared<NXKdTree>();
	m_pCausticPhotons->BuildBalanceTree(causticPhotons);
	printf("done.\n");
}

Vector3 NXPMSplitIntegrator::Radiance(const Ray& cameraRay, const shared_ptr<NXScene>& pScene, int depth)
{
	if (!m_pCausticPhotons)
	{
		printf("Error: Couldn't find photon map data!\n");
		return Vector3(0.0f);
	}

	bool PHOTONS_ONLY = true;
	int maxDepth = 10;

	Ray ray(cameraRay);
	Vector3 nextDirection;
	float pdf;
	NXHit hitInfo;

	Vector3 result(0.0f);
	Vector3 throughput(1.0f);

	bool bIsDiffuse = false;
	while (depth < maxDepth)
	{
		hitInfo = NXHit();
		bool bIsIntersect = pScene->RayCast(ray, hitInfo);
		if (!bIsIntersect)
			return result;

		shared_ptr<NXPBRAreaLight> pHitAreaLight;
		if (hitInfo.pPrimitive)pHitAreaLight = hitInfo.pPrimitive->GetTangibleLight();
		else if (pScene->GetCubeMap()) pHitAreaLight = pScene->GetCubeMap()->GetEnvironmentLight();
		if (pHitAreaLight)
		{
			result += throughput * pHitAreaLight->GetRadiance(hitInfo.position, hitInfo.normal, -ray.direction);
		}

		hitInfo.GenerateBSDF(true);
		shared_ptr<NXBSDF::SampleEvents> sampleEvent = make_shared<NXBSDF::SampleEvents>();
		Vector3 f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
		bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
		sampleEvent.reset();

		if (bIsDiffuse || PHOTONS_ONLY) break;

		throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
		ray = Ray(hitInfo.position, nextDirection);
		ray.position += ray.direction * NXRT_EPSILON;
	}

	Vector3 pos = hitInfo.position;
	Vector3 norm = hitInfo.normal;

	float distSqr;
	// 大根堆，负责记录pos周围的最近顶点。
	priority_quque_NXPhoton nearestPhotons([pos](NXPhoton* photonA, NXPhoton* photonB) {
		float distA = Vector3::DistanceSquared(pos, photonA->position);
		float distB = Vector3::DistanceSquared(pos, photonB->position);
		return distA < distB;
		});

	if (PHOTONS_ONLY)
	{
		m_pCausticPhotons->GetNearest(pos, norm, distSqr, nearestPhotons, 1, 0.00005f);
		if (!nearestPhotons.empty())
		{
			result = nearestPhotons.top()->power;	// photon data only.
			result *= (float)m_numCausticPhotons;
		}
		return result;
	}

	m_pCausticPhotons->GetNearest(pos, norm, distSqr, nearestPhotons, 500, FLT_MAX, LocateFilter::Disk);
	if (nearestPhotons.empty())
		return Vector3(0.0f);

	float radius2 = Vector3::DistanceSquared(pos, nearestPhotons.top()->position);
	Vector3 flux(0.0f);

	while (!nearestPhotons.empty())
	{
		auto photon = nearestPhotons.top();
		Vector3 f = hitInfo.BSDF->Evaluate(-ray.direction, photon->direction, pdf);
		flux += f * photon->power;
		nearestPhotons.pop();
	}
	result += throughput * flux / (XM_PI * radius2);
	return result;
}
