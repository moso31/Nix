#include "NXPPMIntegrator.h"
#include "NXRandom.h"
#include "SamplerMath.h"
#include "NXCamera.h"
#include "NXCubeMap.h"
#include "NXPrimitive.h"

NXPPMIntegrator::NXPPMIntegrator()
{
}

NXPPMIntegrator::~NXPPMIntegrator()
{
}

Vector3 NXPPMIntegrator::Radiance(const Ray& cameraRay, const shared_ptr<NXScene>& pScene, int depth)
{
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

		if (bIsDiffuse) 
			break;

		throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
		ray = Ray(hitInfo.position, nextDirection);
		ray.position += ray.direction * NXRT_EPSILON;
	}

	Vector3 pos = hitInfo.position;
	Vector3 norm = hitInfo.shading.normal;

	m_pixelInfo.position = pos;
	m_pixelInfo.direction = hitInfo.direction;
	m_pixelInfo.normal = norm;
	m_pixelInfo.BSDF = hitInfo.BSDF;
	m_pixelInfo.photons = 0;
	m_pixelInfo.radius2 = FLT_MAX;
	m_pixelInfo.flux = Vector3(0.0f);

	int nPhotonsAtOnce = 100000;	// 一次PPM迭代所准备的光子数量
	int nPhotonsAtAll = 0;
	Vector3 L(0.0f);
	for (int t = 0; t < 1; t++);
	{
		m_pPhotonMap = make_shared<NXPhotonMap>(nPhotonsAtOnce);
		m_pPhotonMap->Generate(pScene, pScene->GetMainCamera(), PhotonMapType::Global);
		nPhotonsAtAll += nPhotonsAtOnce;

		priority_queue_distance_cartesian<NXPhoton> nearestPhotons([pos](const NXPhoton& photonA, const NXPhoton& photonB) {
			float distA = Vector3::DistanceSquared(pos, photonA.position);
			float distB = Vector3::DistanceSquared(pos, photonB.position);
			return distA < distB;
			});

		float distSqr;
		if (!m_pixelInfo.photons)	// 是第一次
		{
			m_pPhotonMap->GetNearest(pos, norm, distSqr, nearestPhotons, 500, FLT_MAX, LocateFilter::Disk);
			m_pixelInfo.radius2 = distSqr;	// 第一次需要借助数量求出半径。
		}
		else // 第2-n次
			m_pPhotonMap->GetNearest(pos, norm, distSqr, nearestPhotons, -1, m_pixelInfo.radius2, LocateFilter::Disk);
		if (nearestPhotons.empty())
			return Vector3(0.0f);

		int photonCount = (int)nearestPhotons.size();	// 新的光子数量
		float estimateArea = XM_PI * m_pixelInfo.radius2;
		float estimateDestiny = (float)(m_pixelInfo.photons + photonCount) / estimateArea;

		float alpha = m_pixelInfo.photons ? 0.7f : 1.0f;	// 第一次设为1.0f（完全保留），后续0.7f（部分保留）。
		float ds = (m_pixelInfo.photons + alpha * (float)photonCount) / (m_pixelInfo.photons * (float)photonCount);

		// 新的估计半径
		float newRadius2 = m_pixelInfo.radius2 * ds;	

		while (!nearestPhotons.empty())
		{
			auto photon = nearestPhotons.top();
			Vector3 f = hitInfo.BSDF->Evaluate(-ray.direction, photon.direction, pdf);
			m_pixelInfo.flux += f * photon.power;
			nearestPhotons.pop();
		}
		// 新的通量
		Vector3 newFlux = m_pixelInfo.flux * ds;
		L = newFlux / (XM_PI * newRadius2 * (float)nPhotonsAtAll);
	}
	return result + L;
}
