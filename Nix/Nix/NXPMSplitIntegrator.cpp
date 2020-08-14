#include "NXPMSplitIntegrator.h"
#include "NXRandom.h"
#include "SamplerMath.h"
#include "NXCamera.h"
#include "NXCubeMap.h"
#include "NXPrimitive.h"
#include "NXIntersection.h"

NXPMSplitIntegrator::NXPMSplitIntegrator(const shared_ptr<NXPhotonMap>& pGlobalPhotons, const shared_ptr<NXPhotonMap>& pCausticPhotons) :
	m_pGlobalPhotonMap(pGlobalPhotons),
	m_pCausticPhotonMap(pCausticPhotons),
	m_bUseIrradianceCache(false)
{
}

NXPMSplitIntegrator::~NXPMSplitIntegrator()
{
}

Vector3 NXPMSplitIntegrator::Radiance(const Ray& cameraRay, const shared_ptr<NXScene>& pScene, int depth)
{
	if (!m_pCausticPhotonMap)
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

	Vector3 Irradiance(0.0f);
	bool bIrradianceCache = false;
	if (bIrradianceCache)
	{
		// if (there is at least one stored irradiance value near x)
		{
			// inteprolate irradiance from the stored values.
		}
		// else
		{
			// compute a new irradiance value at x.
			float tolerance = 0.05f;
			float count = truncf(1.0f / tolerance);
			float sumDistInv = 0.0f;
			float HarmonicDist;
			for (float i = 0.0f; i < 1.0f; i += tolerance)
			{
				for (float j = 0.0f; j < 1.0f; j += tolerance)
				{
					Vector2 u(i, j);
					nextDirection = SamplerMath::CosineSampleHemisphere(u);

					Ray nextRay = Ray(hitInfo.position, nextDirection);
					nextRay.position += nextRay.direction * NXRT_EPSILON;

					NXHit hitInfoDiffuse;
					if (!pScene->RayCast(nextRay, hitInfoDiffuse))
					{
						count -= 1.0f;
						continue;
					}

					sumDistInv += 1.0f / Vector3::Distance(hitInfoDiffuse.position, hitInfo.position);

					Vector3 posDiff = hitInfoDiffuse.position;
					Vector3 normDiff = hitInfoDiffuse.normal;

					priority_quque_NXPhoton nearestGlobalPhotons([posDiff](NXPhoton* photonA, NXPhoton* photonB) {
						float distA = Vector3::DistanceSquared(posDiff, photonA->position);
						float distB = Vector3::DistanceSquared(posDiff, photonB->position);
						return distA < distB;
						});

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

						Irradiance += flux / (XM_PI * radius2);
					}
				}
			}

			HarmonicDist = sumDistInv == 0 ? INFINITY : count * count / sumDistInv;
		}
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

	return result;
}
