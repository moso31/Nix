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

void NXPMSplitIntegrator::Render(NXScene* pScene)
{
	BuildPhotonMap(pScene);
	BuildIrradianceCache(pScene);
	//m_pIrradianceCache->Render(pScene, m_imageSize, "D:\\Nix_IrradianceCacheDistribution.bmp");
	NXSampleIntegrator::Render(pScene);
}

Vector3 NXPMSplitIntegrator::Radiance(const Ray& cameraRay, NXScene* pScene, int depth)
{
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

	bool isDeltaBSDF = false;
	bool bIsDiffuse = false;
	bool bIsIntersect = false;

	// Direct illumination + specular/glossy reflection
	while (depth < maxDepth)
	{
		bIsIntersect = pScene->RayCast(ray, hitInfo);
		if (depth == 0 || isDeltaBSDF)
		{
			if (!bIsIntersect)
			{
				auto pCubeMap = pScene->GetCubeMap();
				if (pCubeMap)
				{
					auto pCubeMapLight = pCubeMap->GetEnvironmentLight();
					if (pCubeMapLight)
						result += throughput * pCubeMapLight->GetRadiance(hitInfo.position, hitInfo.normal, ray.direction);
				}
				return result;
			}

			if (hitInfo.pPrimitive)
			{
				auto pTangibleLight = hitInfo.pPrimitive->GetTangibleLight();
				if (pTangibleLight)
				{
					if (bEmission)
						result += throughput * pTangibleLight->GetRadiance(hitInfo.position, hitInfo.normal, hitInfo.direction);
				}
			}
		}

		if (!bIsIntersect) break;
		hitInfo.GenerateBSDF(true);
		if (bDirect) result += throughput * UniformLightOne(ray, pScene, hitInfo);

		NXBSDF::SampleEvents* sampleEvent = new NXBSDF::SampleEvents();
		f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
		bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
		isDeltaBSDF = *sampleEvent & NXBSDF::DELTA;
		delete sampleEvent;

		if (bIsDiffuse) break;

		throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
		ray = Ray(hitInfo.position, nextDirection);
		ray.position += ray.direction * NXRT_EPSILON;

		depth++;
	}

	if (!bIsIntersect)
		return result;

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
			Vector3 Irradiance = m_pIrradianceCache->Irradiance(ray, pScene, 0);
			result += f * Irradiance;
		}
		else
		{
			// multiple diffuse reflections.
			throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
			Ray nextRay = Ray(hitInfo.position, nextDirection);
			nextRay.position += nextRay.direction * NXRT_EPSILON;

			NXHit hitInfoDiffuse;
			while (true)
			{
				bIsIntersect = pScene->RayCast(nextRay, hitInfoDiffuse);
				if (!bIsIntersect)
					break;

				hitInfoDiffuse.GenerateBSDF(true);

				NXBSDF::SampleEvents* sampleEvent = new NXBSDF::SampleEvents();
				f = hitInfoDiffuse.BSDF->Sample(hitInfoDiffuse.direction, nextDirection, pdf, sampleEvent);
				bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
				delete sampleEvent;

				if (bIsDiffuse) break;

				throughput *= f * fabsf(hitInfoDiffuse.shading.normal.Dot(nextDirection)) / pdf;
				nextRay = Ray(hitInfoDiffuse.position, nextDirection);
				nextRay.position += nextRay.direction * NXRT_EPSILON;
			}

			if (bIsIntersect)
			{
				Vector3 posDiff = hitInfoDiffuse.position;
				Vector3 normDiff = hitInfoDiffuse.shading.normal;

				priority_queue_distance_cartesian<NXPhoton> nearestGlobalPhotons([posDiff](const NXPhoton& photonA, const NXPhoton& photonB) {
					float distA = Vector3::DistanceSquared(posDiff, photonA.position);
					float distB = Vector3::DistanceSquared(posDiff, photonB.position);
					return distA < distB;
					});

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
	}

	return result;
}

void NXPMSplitIntegrator::BuildPhotonMap(NXScene* pScene)
{
	printf("Building Caustic Photon Map...");
	if (m_pCausticPhotonMap)
		m_pCausticPhotonMap->Release();
	m_pCausticPhotonMap = new NXPhotonMap(m_numCausticPhotons);
	m_pCausticPhotonMap->Generate(pScene, PhotonMapType::Caustic);
	printf("Done.\n");
	printf("Building Global Photon Map...");
	if (m_pGlobalPhotonMap)
		m_pGlobalPhotonMap->Release();
	m_pGlobalPhotonMap = new NXPhotonMap(m_numGlobalPhotons);
	m_pGlobalPhotonMap->Generate(pScene, PhotonMapType::Global);
	printf("Done.\n");
}

void NXPMSplitIntegrator::BuildIrradianceCache(NXScene* pScene)
{
	printf("Preloading irradiance caches...\n");
	if (m_pIrradianceCache)
		m_pIrradianceCache->Release();
	m_pIrradianceCache = new NXIrradianceCache();
	m_pIrradianceCache->SetPhotonMaps(m_pGlobalPhotonMap);

	// ������
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
