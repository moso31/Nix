#include "NXPMSplitIntegrator.h"
#include "NXRandom.h"
#include "SamplerMath.h"
#include "NXCamera.h"
#include "NXCubeMap.h"
#include "NXPrimitive.h"

NXPMSplitIntegrator::NXPMSplitIntegrator() :
	m_numGlobalPhotons(200000),
	m_numCausticPhotons(200000)
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
		bool bHasSpecularOrGlossy = false;
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
			bHasSpecularOrGlossy |= !bIsDiffuse;
			sampleEvent.reset();

			if (f.IsZero() || pdf == 0) break;

			if (bIsDiffuse)
			{
				if (bHasSpecularOrGlossy)
				{
					// make new photon
					NXPhoton photon;
					photon.position = hitInfo.position;
					photon.direction = hitInfo.direction;
					photon.power = throughput * numCausticPhotonsInv;
					photon.depth = depth;
					causticPhotons.push_back(photon);
				}
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

	bool PHOTONS_ONLY = false;
	int maxDepth = 10;

	Ray ray(cameraRay);
	Vector3 nextDirection;
	float pdf = 0.0f;
	NXHit hitInfo;

	Vector3 f;
	Vector3 result(0.0f);
	Vector3 throughput(1.0f);

	// Direct illumination + specular/glossy reflection
	bool bIsDiffuse = false;
	while (depth < maxDepth)
	{
		hitInfo = NXHit();
		bool bIsIntersect = pScene->RayCast(ray, hitInfo);
		if (!bIsIntersect)
			return result;

		shared_ptr<NXPBRAreaLight> pHitAreaLight;
		if (hitInfo.pPrimitive) pHitAreaLight = hitInfo.pPrimitive->GetTangibleLight();
		else if (pScene->GetCubeMap()) pHitAreaLight = pScene->GetCubeMap()->GetEnvironmentLight();

		if (pHitAreaLight)
		{
			result += throughput * pHitAreaLight->GetRadiance(hitInfo.position, hitInfo.normal, -ray.direction);
		}

		hitInfo.GenerateBSDF(true);
		result += throughput * UniformLightOne(ray, pScene, hitInfo);

		shared_ptr<NXBSDF::SampleEvents> sampleEvent = make_shared<NXBSDF::SampleEvents>();
		f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
		bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
		sampleEvent.reset();

		if (bIsDiffuse || PHOTONS_ONLY) break;

		throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
		ray = Ray(hitInfo.position, nextDirection);
		ray.position += ray.direction * NXRT_EPSILON;
	}

	Vector3 pos = hitInfo.position;
	Vector3 norm = hitInfo.normal;

	// caustics
	priority_quque_NXPhoton nearestCausticPhotons([pos](NXPhoton* photonA, NXPhoton* photonB) {
		float distA = Vector3::DistanceSquared(pos, photonA->position);
		float distB = Vector3::DistanceSquared(pos, photonB->position);
		return distA < distB;
		});

	float distSqr;
	if (PHOTONS_ONLY)
	{
		m_pCausticPhotons->GetNearest(pos, norm, distSqr, nearestCausticPhotons, 1, 0.00005f);
		if (!nearestCausticPhotons.empty())
		{
			result = nearestCausticPhotons.top()->power;	// photon data only.
			result *= (float)m_numCausticPhotons;
		}
		return result;
	}

	m_pCausticPhotons->GetNearest(pos, norm, distSqr, nearestCausticPhotons, 50, 0.15f, LocateFilter::Disk);
	if (!nearestCausticPhotons.empty())
	{
		float radius2 = Vector3::DistanceSquared(pos, nearestCausticPhotons.top()->position);
		if (nearestCausticPhotons.size() < 50) radius2 = max(radius2, 0.15f);

		Vector3 flux(0.0f);
		while (!nearestCausticPhotons.empty())
		{
			float pdfPhoton;
			auto photon = nearestCausticPhotons.top();
			Vector3 f = hitInfo.BSDF->Evaluate(-ray.direction, photon->direction, pdfPhoton);
			flux += f * photon->power;
			nearestCausticPhotons.pop();
		}
		result += throughput * flux / (XM_PI * radius2);
	}

	// multiple diffuse reflections.
	throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
	Ray nextRay = Ray(hitInfo.position, nextDirection);
	nextRay.position += nextRay.direction * NXRT_EPSILON;

	NXHit hitInfoDiffuse;
	if (!pScene->RayCast(nextRay, hitInfoDiffuse))
		return result;

	Vector3 posDiff = hitInfoDiffuse.position;
	Vector3 normDiff = hitInfoDiffuse.normal;

	priority_quque_NXPhoton nearestGlobalPhotons([posDiff](NXPhoton* photonA, NXPhoton* photonB) {
		float distA = Vector3::DistanceSquared(posDiff, photonA->position);
		float distB = Vector3::DistanceSquared(posDiff, photonB->position);
		return distA < distB;
		});

	if (PHOTONS_ONLY)
	{
		m_pGlobalPhotons->GetNearest(posDiff, normDiff, distSqr, nearestGlobalPhotons, 1, 0.00005f);
		if (!nearestGlobalPhotons.empty())
		{
			result = nearestGlobalPhotons.top()->power;	// photon data only.
			result *= (float)m_numGlobalPhotons;
		}
		return result;
	}

	hitInfoDiffuse.GenerateBSDF(true);

	m_pGlobalPhotons->GetNearest(posDiff, normDiff, distSqr, nearestGlobalPhotons, 100, FLT_MAX, LocateFilter::Disk);
	if (!nearestGlobalPhotons.empty())
	{
		float radius2 = Vector3::DistanceSquared(posDiff, nearestGlobalPhotons.top()->position);
		Vector3 flux(0.0f);
		while (!nearestGlobalPhotons.empty())
		{
			float pdfPhoton;
			auto photon = nearestGlobalPhotons.top();
			Vector3 f = hitInfoDiffuse.BSDF->Evaluate(-nextRay.direction, photon->direction, pdfPhoton);
			flux += f * photon->power;
			nearestGlobalPhotons.pop();
		}
		result += throughput * flux / (XM_PI * radius2);
	}

	return result;
}
