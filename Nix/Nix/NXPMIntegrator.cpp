#include "NXPMIntegrator.h"
#include "NXRandom.h"
#include "SamplerMath.h"
#include "NXCamera.h"
#include "NXCubeMap.h"
#include "NXPrimitive.h"

NXPMIntegrator::NXPMIntegrator(const XMINT2& imageSize, int eachPixelSamples, std::string outPath, UINT nPhotons, UINT nEstimatePhotons) :
	NXSampleIntegrator(imageSize, eachPixelSamples, outPath),
	m_numPhotons(nPhotons),
	m_estimatePhotons(nEstimatePhotons),
	m_pPhotonMap(nullptr)
{
}

NXPMIntegrator::~NXPMIntegrator()
{
	SafeRelease(m_pPhotonMap);
}

void NXPMIntegrator::Render(NXScene* pScene)
{
	BuildPhotonMap(pScene);
	//m_pPhotonMap->Render(pScene, m_imageSize, m_outFilePath);
	NXSampleIntegrator::Render(pScene);
}

Vector3 NXPMIntegrator::Radiance(const Ray& cameraRay, NXScene* pScene, int depth)
{
	if (!m_pPhotonMap)
	{
		printf("Error: Couldn't find photon map data!\n");
		return Vector3(0.0f);
	}

	int maxDepth = 10;

	Ray ray(cameraRay);
	Vector3 nextDirection;
	float pdf;
	NXHit hitInfo;

	Vector3 result(0.0f);
	Vector3 throughput(1.0f), f(0.0f);

	bool isDeltaBSDF = false;
	bool bIsDiffuse = false;
	while (depth < maxDepth)
	{
		bool bIsIntersect = pScene->RayCast(ray, hitInfo);
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
					result += throughput * pTangibleLight->GetRadiance(hitInfo.position, hitInfo.normal, hitInfo.direction);
				}
			}
		}
		
		if (!bIsIntersect)
			return result;

		hitInfo.GenerateBSDF(true);
		NXBSDF::SampleEvents* sampleEvent = new NXBSDF::SampleEvents();
		f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
		bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
		isDeltaBSDF = *sampleEvent & NXBSDF::DELTA;
		SafeDelete(sampleEvent);

		if (f.IsZero() || pdf == 0) break;
		if (bIsDiffuse) break;

		throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
		ray = Ray(hitInfo.position, nextDirection);
		ray.position += ray.direction * NXRT_EPSILON;
		depth++;
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
		flux += f * photon.power;// * GaussianFilter(dist2, radius2);
		nearestPhotons.pop();
	}
	float numPhotons = (float)m_pPhotonMap->GetPhotonCount();
	result += throughput * flux / (XM_PI * radius2 * numPhotons);

	return result;
}

void NXPMIntegrator::BuildPhotonMap(NXScene* pScene)
{
	printf("Building Global Photon Map...");
	if (m_pPhotonMap)
		m_pPhotonMap->Release();
	m_pPhotonMap = new NXPhotonMap(m_numPhotons);
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
