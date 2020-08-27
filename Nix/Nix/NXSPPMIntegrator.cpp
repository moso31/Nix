#include "NXSPPMIntegrator.h"
#include "NXRandom.h"
#include "SamplerMath.h"
#include "NXCamera.h"
#include "NXCubeMap.h"
#include "NXPrimitive.h"

void NXSPPMIntegrator::RefreshPhotonMap(const std::shared_ptr<NXScene>& pScene)
{
	m_pPhotonMap.reset();
	m_pPhotonMap = std::make_shared<NXPhotonMap>(200000);
	m_pPhotonMap->Generate(pScene, PhotonMapType::Global);
}

void NXSPPMIntegrator::Render(const std::shared_ptr<NXScene>& pScene)
{
	//int maxDepth = 10;

	//Ray ray(cameraRay);
	//Vector3 nextDirection;
	//float pdf;
	//NXHit hitInfo;

	//Vector3 result(0.0f);
	//Vector3 throughput(1.0f);

	//bool bIsDiffuse = false;
	//while (depth < maxDepth)
	//{
	//	hitInfo = NXHit();
	//	bool bIsIntersect = pScene->RayCast(ray, hitInfo);
	//	if (!bIsIntersect)
	//		return result;

	//	std::shared_ptr<NXPBRAreaLight> pHitAreaLight;
	//	if (hitInfo.pPrimitive)pHitAreaLight = hitInfo.pPrimitive->GetTangibleLight();
	//	else if (pScene->GetCubeMap()) pHitAreaLight = pScene->GetCubeMap()->GetEnvironmentLight();
	//	if (pHitAreaLight)
	//	{
	//		result += throughput * pHitAreaLight->GetRadiance(hitInfo.position, hitInfo.normal, -ray.direction);
	//	}

	//	hitInfo.GenerateBSDF(true);
	//	std::shared_ptr<NXBSDF::SampleEvents> sampleEvent = std::make_shared<NXBSDF::SampleEvents>();
	//	Vector3 f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
	//	bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
	//	sampleEvent.reset();

	//	if (bIsDiffuse) break;

	//	throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
	//	ray = Ray(hitInfo.position, nextDirection);
	//	ray.position += ray.direction * NXRT_EPSILON;
	//}

	//Vector3 pos = hitInfo.position;
	//Vector3 norm = hitInfo.shading.normal;

	//float distSqr;
	//// 大根堆，负责记录pos周围的最近顶点。
	//priority_queue_distance_cartesian<NXPhoton> nearestPhotons([pos](const NXPhoton& photonA, const NXPhoton& photonB) {
	//	float distA = Vector3::DistanceSquared(pos, photonA.position);
	//	float distB = Vector3::DistanceSquared(pos, photonB.position);
	//	return distA < distB;
	//	});

	//m_pPhotonMap->GetNearest(pos, norm, distSqr, nearestPhotons, 100, FLT_MAX, LocateFilter::Disk);
	//if (nearestPhotons.empty())
	//	return Vector3(0.0f);

	//float radius2 = Vector3::DistanceSquared(pos, nearestPhotons.top().position);
	//Vector3 flux(0.0f);

	//while (!nearestPhotons.empty())
	//{
	//	auto photon = nearestPhotons.top();
	//	Vector3 f = hitInfo.BSDF->Evaluate(-ray.direction, photon.direction, pdf);
	//	flux += f * photon.power;
	//	nearestPhotons.pop();
	//}
	//float numPhotons = (float)m_pPhotonMap->GetPhotonCount();
	//result += throughput * flux / (XM_PI * radius2 * numPhotons);
	//return result;
}
