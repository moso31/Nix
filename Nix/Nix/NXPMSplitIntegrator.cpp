#include "NXPMSplitIntegrator.h"
#include "NXRandom.h"
#include "SamplerMath.h"
#include "NXCamera.h"
#include "NXCubeMap.h"
#include "NXPrimitive.h"
#include "NXIntersection.h"

NXPMSplitIntegrator::NXPMSplitIntegrator(const XMINT2& imageSize, int eachPixelSamples, std::string outPath, UINT nCausticPhotons, UINT nGlobalPhotons) :
	NXSampleIntegrator(imageSize, eachPixelSamples, outPath),
	m_numCausticPhotons(nCausticPhotons),
	m_numGlobalPhotons(nGlobalPhotons)
{
}

NXPMSplitIntegrator::~NXPMSplitIntegrator()
{
}

void NXPMSplitIntegrator::Render(const std::shared_ptr<NXScene>& pScene)
{
	BuildPhotonMap(pScene);
	BuildIrradianceCache(pScene);
	//m_pIrradianceCache->Render(pScene, m_imageSize, "D:\\Nix_IrradianceCacheDistribution.bmp");
	NXSampleIntegrator::Render(pScene);
}

Vector3 NXPMSplitIntegrator::Radiance(const Ray& cameraRay, const std::shared_ptr<NXScene>& pScene, int depth)
{
	bool PHOTONS_ONLY = false;

	bool bEmission			= true;
	bool bDirect			= true;
	bool bCaustic			= true;
	bool bDiffuseIndirect	= true;

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

		std::shared_ptr<NXPBRAreaLight> pHitAreaLight;
		if (hitInfo.pPrimitive) pHitAreaLight = hitInfo.pPrimitive->GetTangibleLight();
		else if (pScene->GetCubeMap()) pHitAreaLight = pScene->GetCubeMap()->GetEnvironmentLight();
		if (pHitAreaLight)
		{
			if (bEmission)
				result += throughput * pHitAreaLight->GetRadiance(hitInfo.position, hitInfo.normal, -ray.direction);
		}

		hitInfo.GenerateBSDF(true);
		if (bDirect) result += throughput * UniformLightOne(ray, pScene, hitInfo);

		std::shared_ptr<NXBSDF::SampleEvents> sampleEvent = std::make_shared<NXBSDF::SampleEvents>();
		f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
		bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
		sampleEvent.reset();

		if (bIsDiffuse || PHOTONS_ONLY) break;

		throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
		ray = Ray(hitInfo.position, nextDirection);
		ray.position += ray.direction * NXRT_EPSILON;
	}

	Vector3 pos = hitInfo.position;
	Vector3 norm = hitInfo.shading.normal;
	float distSqr;

	// caustics
	if (bCaustic)
	{
		priority_queue_distance_cartesian<NXPhoton> nearestCausticPhotons([pos](const NXPhoton& photonA, const NXPhoton& photonB) {
			float distA = Vector3::DistanceSquared(pos, photonA.position);
			float distB = Vector3::DistanceSquared(pos, photonB.position);
			return distA < distB;
			});

		if (PHOTONS_ONLY)
		{
			m_pCausticPhotonMap->GetNearest(pos, norm, distSqr, nearestCausticPhotons, 1, 0.00005f);
			if (!nearestCausticPhotons.empty())
			{
				result = nearestCausticPhotons.top().power;	// photon data only.
			}
			return result;
		}

		m_pCausticPhotonMap->GetNearest(pos, norm, distSqr, nearestCausticPhotons, 50, 0.15f, LocateFilter::Disk);
		if (!nearestCausticPhotons.empty())
		{
			float radius2 = Vector3::DistanceSquared(pos, nearestCausticPhotons.top().position);
			if (nearestCausticPhotons.size() < 50) radius2 = max(radius2, 0.15f);

			Vector3 flux(0.0f);
			while (!nearestCausticPhotons.empty())
			{
				float pdfPhoton;
				auto photon = nearestCausticPhotons.top();
				Vector3 f = hitInfo.BSDF->Evaluate(-ray.direction, photon.direction, pdfPhoton);
				flux += f * photon.power;
				nearestCausticPhotons.pop();
			}
			float numPhotons = (float)m_pCausticPhotonMap->GetPhotonCount();
			result += throughput * flux / (XM_PI * radius2 * numPhotons);
		}
	}

	if (bDiffuseIndirect)
	{
		if (m_pIrradianceCache)
		{
			Vector3 Irradiance = m_pIrradianceCache->Irradiance(ray, pScene, depth);
			result += f * Irradiance;
		}
		else
		{
			// multiple diffuse reflections.
			throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
			Ray nextRay = Ray(hitInfo.position, nextDirection);
			nextRay.position += nextRay.direction * NXRT_EPSILON;

			NXHit hitInfoDiffuse;
			if (!pScene->RayCast(nextRay, hitInfoDiffuse))
				return result;

			Vector3 posDiff = hitInfoDiffuse.position;
			Vector3 normDiff = hitInfoDiffuse.shading.normal;

			priority_queue_distance_cartesian<NXPhoton> nearestGlobalPhotons([posDiff](const NXPhoton& photonA, const NXPhoton& photonB) {
				float distA = Vector3::DistanceSquared(posDiff, photonA.position);
				float distB = Vector3::DistanceSquared(posDiff, photonB.position);
				return distA < distB;
				});

			if (PHOTONS_ONLY)
			{
				m_pGlobalPhotonMap->GetNearest(posDiff, normDiff, distSqr, nearestGlobalPhotons, 1, 0.00005f);
				if (!nearestGlobalPhotons.empty())
				{
					result = nearestGlobalPhotons.top().power;	// photon data only.
				}
				return result;
			}

			hitInfoDiffuse.GenerateBSDF(true);

			m_pGlobalPhotonMap->GetNearest(posDiff, normDiff, distSqr, nearestGlobalPhotons, 100, FLT_MAX, LocateFilter::Disk);
			if (!nearestGlobalPhotons.empty())
			{
				float radius2 = Vector3::DistanceSquared(posDiff, nearestGlobalPhotons.top().position);
				Vector3 flux(0.0f);
				while (!nearestGlobalPhotons.empty())
				{
					auto photon = nearestGlobalPhotons.top();
					Vector3 f = hitInfoDiffuse.BSDF->Evaluate(-nextRay.direction, photon.direction, pdf);
					flux += f * photon.power;
					nearestGlobalPhotons.pop();
				}
				float numPhotons = (float)m_pGlobalPhotonMap->GetPhotonCount();
				result += throughput * flux / (XM_PI * radius2 * numPhotons);
			}
		}
	}

	return result;
}

void NXPMSplitIntegrator::BuildPhotonMap(const std::shared_ptr<NXScene>& pScene)
{
	printf("Building Caustic Photon Map...");
	m_pCausticPhotonMap.reset();
	m_pCausticPhotonMap = std::make_shared<NXPhotonMap>(m_numCausticPhotons);
	m_pCausticPhotonMap->Generate(pScene, PhotonMapType::Caustic);
	printf("Done.\n");
	printf("Building Global Photon Map...");
	m_pGlobalPhotonMap.reset();
	m_pGlobalPhotonMap = std::make_shared<NXPhotonMap>(m_numGlobalPhotons);
	m_pGlobalPhotonMap->Generate(pScene, PhotonMapType::Global);
	printf("Done.\n");
}

void NXPMSplitIntegrator::BuildIrradianceCache(const std::shared_ptr<NXScene>& pScene)
{
	printf("Preloading irradiance caches...\n");
	m_pIrradianceCache.reset();
	m_pIrradianceCache = std::make_shared<NXIrradianceCache>();
	m_pIrradianceCache->SetPhotonMaps(m_pGlobalPhotonMap);

	// ½ø¶ÈÌõ
	int process = 0;
	int pxCount = m_imageSize.x * m_imageSize.y;
	int barrier = 5000;
	int nodeCount = (pxCount + barrier - 1) / barrier;

#pragma omp parallel for
	for (int i = 0; i < m_imageSize.x; i++)
	{
		for (int j = 0; j < m_imageSize.y; j++)
		{
			Vector2 pixel((float)i, (float)j);

			Vector2 coord = pixel + Vector2(0.5f, 0.5f);
			Ray rayWorld = pScene->GetMainCamera()->GenerateRay(coord, Vector2((float)m_imageSize.x, (float)m_imageSize.y));
			m_pIrradianceCache->PreIrradiance(rayWorld, pScene, 0);

			process++;
			if (process % barrier == 0)
				printf("\r%.2f%% ", (float)process * 100 / pxCount);
		}
	}
	printf("done. (caches: %zd)\n", m_pIrradianceCache->GetCacheSize());
}
