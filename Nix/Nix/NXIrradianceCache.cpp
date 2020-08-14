#include "NXIrradianceCache.h"
#include "NXScene.h"
#include "NXCubeMap.h"
#include "NXPrimitive.h"
#include "SamplerMath.h"

using namespace SamplerMath;

Vector3 NXIrradianceCache::Irradiance(const Ray& cameraRay, const shared_ptr<NXScene>& pScene, int depth)
{
	int maxDepth = 10;

	Ray ray(cameraRay);
	Vector3 nextDirection;
	float pdf = 0.0f;
	NXHit hitInfo;

	Vector3 f;
	Vector3 result(0.0f);

	// 只需要找到最终的首次漫反射位置。不需要计算吞吐量，那是Integrator该干的事儿。
	// Irradiance只需要对辐照缓存点进行定位。
	bool bIsDiffuse = false;
	while (true)
	{
		hitInfo = NXHit();
		bool bIsIntersect = pScene->RayCast(ray, hitInfo);
		if (!bIsIntersect)
			return result;

		hitInfo.GenerateBSDF(true);

		shared_ptr<NXBSDF::SampleEvents> sampleEvent = make_shared<NXBSDF::SampleEvents>();
		f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
		bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
		sampleEvent.reset();

		if (bIsDiffuse || depth < maxDepth) 
			break;

		ray = Ray(hitInfo.position, nextDirection);
		ray.position += ray.direction * NXRT_EPSILON;

		depth++;
	}

	if (!bIsDiffuse)
		return result;

	// if (there is at least one stored irradiance value near x)
	{
		// inteprolate irradiance from the stored values.
	}
	// else
	{
		// compute a new irradiance value at x.
		float tTheta = 0.05f;
		float tPhi = 0.05f;
		float sampleTheta = truncf(1.0f / tTheta);
		float samplePhi = truncf(1.0f / tPhi);
		float count = sampleTheta * samplePhi;
		float sumHarmonicDistance = 0.0f;

		float distSqr;
		// 精确计算半球方向上的点。
		for (float i = 0.0f; i < 1.0f; i += tTheta)
		{
			for (float j = 0.0f; j < 1.0f; j += tPhi)
			{
				Vector2 u(i, j);
				Vector3 nextDirLocal = SamplerMath::CosineSampleHemisphere(u);
				nextDirection = hitInfo.BSDF->ReflectionToWorld(nextDirLocal);

				Ray nextRay = Ray(hitInfo.position, nextDirection);
				nextRay.position += nextRay.direction * NXRT_EPSILON;

				NXHit hitInfoDiffuse;
				if (!pScene->RayCast(nextRay, hitInfoDiffuse))
				{
					count -= 1.0f;
					continue;
				}

				sumHarmonicDistance += 1.0f / Vector3::Distance(hitInfo.position, hitInfoDiffuse.position);

				Vector3 posDiff = hitInfoDiffuse.position;
				Vector3 normDiff = hitInfoDiffuse.normal;

				priority_quque_NXPhoton nearestPhotons([posDiff](NXPhoton* photonA, NXPhoton* photonB) {
					float distA = Vector3::DistanceSquared(posDiff, photonA->position);
					float distB = Vector3::DistanceSquared(posDiff, photonB->position);
					return distA < distB;
					});

				hitInfoDiffuse.GenerateBSDF(true);

				Vector3 L(0.0f);
				m_pPhotonMap->GetNearest(posDiff, normDiff, distSqr, nearestPhotons, 100, FLT_MAX, LocateFilter::Disk);
				if (!nearestPhotons.empty())
				{
					float radius2 = Vector3::DistanceSquared(posDiff, nearestPhotons.top()->position);
					while (!nearestPhotons.empty())
					{
						float pdfPhoton;
						auto photon = nearestPhotons.top();
						Vector3 f = hitInfoDiffuse.BSDF->Evaluate(-nextRay.direction, photon->direction, pdfPhoton);
						L += f * photon->power;
						nearestPhotons.pop();
					}

					L /= (XM_PI * radius2);
				}

				result += L; // * hitInfo.shading.normal.Dot(nextDirection) / SamplerMath::CosineSampleHemispherePdf(不算两个余弦项了反正也会被抵消掉);
			}
		}
		result *= XM_PI / count;

		// 计算完毕后储存
		NXIrradianceCacheInfo info;
		info.position = hitInfo.position;
		info.irradiance = result;
		info.normal = hitInfo.normal;
		info.harmonicDistance = count / sumHarmonicDistance;

		return result;
	}
}
