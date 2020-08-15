#include "NXPMSplitIntegrator.h"
#include "NXRandom.h"
#include "SamplerMath.h"
#include "NXCamera.h"
#include "NXCubeMap.h"
#include "NXPrimitive.h"
#include "NXIntersection.h"

NXPMSplitIntegrator::NXPMSplitIntegrator(const shared_ptr<NXPhotonMap>& pGlobalPhotons, const shared_ptr<NXPhotonMap>& pCausticPhotons) :
	m_pGlobalPhotonMap(pGlobalPhotons),
	m_pCausticPhotonMap(pCausticPhotons)
{
}

NXPMSplitIntegrator::~NXPMSplitIntegrator()
{
}

Vector3 NXPMSplitIntegrator::Radiance(const Ray& cameraRay, const shared_ptr<NXScene>& pScene, int depth)
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

		shared_ptr<NXPBRAreaLight> pHitAreaLight;
		if (hitInfo.pPrimitive) pHitAreaLight = hitInfo.pPrimitive->GetTangibleLight();
		else if (pScene->GetCubeMap()) pHitAreaLight = pScene->GetCubeMap()->GetEnvironmentLight();

		if (pHitAreaLight)
		{
			if (bEmission)
				result += throughput * pHitAreaLight->GetRadiance(hitInfo.position, hitInfo.normal, -ray.direction);
		}

		hitInfo.GenerateBSDF(true);
		if (bDirect)
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
	float distSqr;

	// caustics
	if (bCaustic)
	{
		priority_quque_NXPhoton nearestCausticPhotons([pos](NXPhoton* photonA, NXPhoton* photonB) {
			float distA = Vector3::DistanceSquared(pos, photonA->position);
			float distB = Vector3::DistanceSquared(pos, photonB->position);
			return distA < distB;
			});

		if (PHOTONS_ONLY)
		{
			m_pCausticPhotonMap->GetNearest(pos, norm, distSqr, nearestCausticPhotons, 1, 0.00005f);
			if (!nearestCausticPhotons.empty())
			{
				result = nearestCausticPhotons.top()->power;	// photon data only.
				result *= (float)m_pCausticPhotonMap->GetPhotonCount();
			}
			return result;
		}

		m_pCausticPhotonMap->GetNearest(pos, norm, distSqr, nearestCausticPhotons, 50, 0.15f, LocateFilter::Disk);
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
			Vector3 normDiff = hitInfoDiffuse.normal;

			priority_quque_NXPhoton nearestGlobalPhotons([posDiff](NXPhoton* photonA, NXPhoton* photonB) {
				float distA = Vector3::DistanceSquared(posDiff, photonA->position);
				float distB = Vector3::DistanceSquared(posDiff, photonB->position);
				return distA < distB;
				});

			if (PHOTONS_ONLY)
			{
				m_pGlobalPhotonMap->GetNearest(posDiff, normDiff, distSqr, nearestGlobalPhotons, 1, 0.00005f);
				if (!nearestGlobalPhotons.empty())
				{
					result = nearestGlobalPhotons.top()->power;	// photon data only.
					result *= (float)m_pGlobalPhotonMap->GetPhotonCount();
				}
				return result;
			}

			hitInfoDiffuse.GenerateBSDF(true);

			m_pGlobalPhotonMap->GetNearest(posDiff, normDiff, distSqr, nearestGlobalPhotons, 100, FLT_MAX, LocateFilter::Disk);
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
		}
	}

	return result;
}
