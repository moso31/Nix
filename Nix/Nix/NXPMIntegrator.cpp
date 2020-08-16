#include "NXPMIntegrator.h"
#include "NXRandom.h"
#include "SamplerMath.h"
#include "NXCamera.h"
#include "NXCubeMap.h"
#include "NXPrimitive.h"

NXPMIntegrator::NXPMIntegrator(const shared_ptr<NXPhotonMap>& pGlobalPhotons) :
	m_pPhotonMap(pGlobalPhotons)
{
}

NXPMIntegrator::~NXPMIntegrator()
{
}

Vector3 NXPMIntegrator::Radiance(const Ray& cameraRay, const shared_ptr<NXScene>& pScene, int depth)
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
			result *= (float)m_pPhotonMap->GetPhotonCount();
		}
		return result;
	}

	m_pPhotonMap->GetNearest(pos, norm, distSqr, nearestPhotons, 500, FLT_MAX, LocateFilter::Disk);
	if (nearestPhotons.empty())
		return Vector3(0.0f);

	float radius2 = Vector3::DistanceSquared(pos, nearestPhotons.top().position);
	Vector3 flux(0.0f);

	while (!nearestPhotons.empty())
	{
		auto photon = nearestPhotons.top();
		Vector3 f = hitInfo.BSDF->Evaluate(-ray.direction, photon.direction, pdf);
		flux += f * photon.power;
		nearestPhotons.pop();
	}
	result += throughput * flux / (XM_PI * radius2);
	return result;
}
