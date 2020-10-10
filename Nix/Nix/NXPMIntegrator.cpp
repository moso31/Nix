#include "NXPMIntegrator.h"
#include "NXRandom.h"
#include "SamplerMath.h"
#include "NXCamera.h"
#include "NXCubeMap.h"
#include "NXPrimitive.h"

NXPMIntegrator::NXPMIntegrator(const XMINT2& imageSize, int eachPixelSamples, std::string outPath, UINT nPhotons, UINT nEstimatePhotons) :
	NXSampleIntegrator(imageSize, eachPixelSamples, outPath),
	m_numPhotons(nPhotons),
	m_estimatePhotons(nEstimatePhotons)
{
}

NXPMIntegrator::~NXPMIntegrator()
{
}

void NXPMIntegrator::Render(const std::shared_ptr<NXScene>& pScene)
{
	BuildPhotonMap(pScene);
	NXSampleIntegrator::Render(pScene);
}

Vector3 NXPMIntegrator::Radiance(const Ray& cameraRay, const std::shared_ptr<NXScene>& pScene, int depth)
{
	if (!m_pPhotonMap)
	{
		printf("Error: Couldn't find photon map data!\n");
		return Vector3(0.0f);
	}

	bool PHOTONS_ONLY = false;
	int maxDepth = 10;

	Ray ray(cameraRay);
	Vector3 nextDirection;
	float pdf;
	NXHit hitInfo;

	Vector3 result(0.0f);
	Vector3 throughput(1.0f), f(0.0f);

	bool bIsDiffuse = false;
	while (depth < maxDepth)
	{
		bool bIsIntersect = pScene->RayCast(ray, hitInfo);
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
				result += throughput * pTangibleLight->GetRadiance(hitInfo.position, hitInfo.normal, hitInfo.direction);
			}
		}

		hitInfo.GenerateBSDF(true);
		std::shared_ptr<NXBSDF::SampleEvents> sampleEvent = std::make_shared<NXBSDF::SampleEvents>();
		f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
		bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
		sampleEvent.reset();

		if (f.IsZero() || pdf == 0) break;
		if (bIsDiffuse) break;
		if (PHOTONS_ONLY) break;

		throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
		ray = Ray(hitInfo.position, nextDirection);
		ray.position += ray.direction * NXRT_EPSILON;
	}


	if (f.IsZero() || pdf == 0)
	{
		return result;
	}

	Vector3 pos = hitInfo.position;
	Vector3 norm = hitInfo.shading.normal;

	float distSqr;
	// 大根堆，负责记录pos周围的最近顶点。
	priority_queue_distance_cartesian<NXPhoton> nearestPhotons([pos](const NXPhoton& photonA, const NXPhoton& photonB) {
		float distA = Vector3::DistanceSquared(pos, photonA.position);
		float distB = Vector3::DistanceSquared(pos, photonB.position);
		return distA < distB;
		});

	if (PHOTONS_ONLY)
	{
		m_pPhotonMap->GetNearest(pos, norm, distSqr, nearestPhotons, 1, 0.00005f);
		if (!nearestPhotons.empty())
		{
			result = nearestPhotons.top().power;	// photon data only.
		}
		return result;
	}

	m_pPhotonMap->GetNearest(pos, norm, distSqr, nearestPhotons, m_estimatePhotons, FLT_MAX, LocateFilter::Disk);
	if (nearestPhotons.empty())
		return Vector3(0.0f);

	float radius2 = Vector3::DistanceSquared(pos, nearestPhotons.top().position);
	Vector3 flux(0.0f);

	while (!nearestPhotons.empty())
	{
		auto photon = nearestPhotons.top();
		Vector3 f = hitInfo.BSDF->Evaluate(-ray.direction, photon.direction, pdf);
		float dist2 = Vector3::DistanceSquared(pos, photon.position);
		flux += f * photon.power * GaussianFilter(dist2, radius2);
		nearestPhotons.pop();
	}
	float numPhotons = (float)m_pPhotonMap->GetPhotonCount();
	result += throughput * flux / (XM_PI * radius2 * numPhotons);

	return result;
}

void NXPMIntegrator::BuildPhotonMap(const std::shared_ptr<NXScene>& pScene)
{
	printf("Building Global Photon Map...");
	m_pPhotonMap.reset();
	m_pPhotonMap = std::make_shared<NXPhotonMap>(m_numPhotons);
	m_pPhotonMap->Generate(pScene, PhotonMapType::Global);
	printf("Done.\n");
}

float NXPMIntegrator::GaussianFilter(float distance2, float radius2)
{
	float alpha = 1.818f;
	float beta = 1.953f;
	float oneMinusEB = 1.0f - exp(-beta);
	float oneMinusEBDR = 1.0f - exp(-beta * (distance2 / (2.0f * radius2)));
	return alpha * (1.0f - oneMinusEBDR / oneMinusEB);
}
