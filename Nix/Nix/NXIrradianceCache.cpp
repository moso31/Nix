#include "NXIrradianceCache.h"
#include "NXScene.h"
#include "NXCubeMap.h"
#include "NXPrimitive.h"
#include "SamplerMath.h"

using namespace SamplerMath;

NXIrradianceCache::NXIrradianceCache() :
	m_threshold(0.25f)
{
}

void NXIrradianceCache::PreIrradiance(const Ray& cameraRay, const shared_ptr<NXScene>& pScene, int depth)
{
	int maxDepth = 10;

	Ray ray(cameraRay);
	Vector3 nextDirection;
	float pdf = 0.0f;
	NXHit hitInfo;

	// ֻ��Ҫ�ҵ����յ��״�������λ�á�����Ҫ����������������Integrator�øɵ��¶���
	// Irradianceֻ��Ҫ�Է��ջ������ж�λ��
	bool bIsDiffuse = false;
	while (true)
	{
		hitInfo = NXHit();
		bool bIsIntersect = pScene->RayCast(ray, hitInfo);
		if (!bIsIntersect)
			return;

		hitInfo.GenerateBSDF(true);

		shared_ptr<NXBSDF::SampleEvents> sampleEvent = make_shared<NXBSDF::SampleEvents>();
		Vector3 f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
		bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
		sampleEvent.reset();

		if (bIsDiffuse || depth < maxDepth)
			break;

		ray = Ray(hitInfo.position, nextDirection);
		ray.position += ray.direction * NXRT_EPSILON;

		depth++;
	}

	if (!bIsDiffuse)
		return;

	Vector3 estimateIrradiance;
	if (!FindEstimateCaches(hitInfo.position, hitInfo.shading.normal, estimateIrradiance))
	{
		int sampleTheta = 20;
		int samplePhi = 20;
		NXIrradianceCacheInfo cacheInfo;
		CalculateOneCache(pScene, hitInfo, sampleTheta, samplePhi, cacheInfo);
		m_caches.push_back(cacheInfo);
	}
}

Vector3 NXIrradianceCache::Irradiance(const Ray& cameraRay, const shared_ptr<NXScene>& pScene, int depth)
{
	int maxDepth = 10;

	Ray ray(cameraRay);
	Vector3 nextDirection;
	float pdf = 0.0f;
	NXHit hitInfo;

	Vector3 f;
	Vector3 result(0.0f);

	// ֻ��Ҫ�ҵ����յ��״�������λ�á�����Ҫ����������������Integrator�øɵ��¶���
	// Irradianceֻ��Ҫ�Է��ջ������ж�λ��
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

	Vector3 estimateIrradiance;
	if (FindEstimateCaches(hitInfo.position, hitInfo.shading.normal, estimateIrradiance))
	{
		// ��������IrradianceCache��ʱ��ʹ���ڽ�Cache����ٹ��㡣
		result += estimateIrradiance;
	}
	else
	{
		// ����û��Cache�㣬��ֻ�ܾ�ȷ���㡣
		// �����������ڰ���ռ����ܼ���������ÿ�����򶼼��㣬���ͳ��ƽ��ֵ��
		int sampleTheta = 20;
		int samplePhi = 20;
		NXIrradianceCacheInfo cacheInfo;
		result += CalculateOneCache(pScene, hitInfo, sampleTheta, samplePhi, cacheInfo);
		m_caches.push_back(cacheInfo);
	}

	return result;
}

bool NXIrradianceCache::FindEstimateCaches(const Vector3& position, const Vector3& normal, Vector3& oEstimateIrradiance)
{
	bool find = false;
	Vector3 sum(0.0f);
	float sumWeight = 0.0f;

	for (auto it = m_caches.begin(); it != m_caches.end(); it++)
	{
		float weightInv = Vector3::Distance(position, it->position) / it->harmonicDistance + sqrtf(1.0f - normal.Dot(it->normal));
		if (weightInv < m_threshold)
		{
			float weight = 1.0f / weightInv;
			sum += weight * it->irradiance;
			sumWeight += weight;
			find = true;
		}
	}

	if (find && sumWeight > 0.0f)
	{
		oEstimateIrradiance = sum / sumWeight;
		return true;
	}
	return false;
}

Vector3 NXIrradianceCache::CalculateOneCache(const shared_ptr<NXScene>& pScene, const NXHit& hitInfo, int sampleTheta, int samplePhi, NXIrradianceCacheInfo& oCacheInfo)
{
	float tTheta = 1.0f / (float)sampleTheta;
	float tPhi = 1.0f / (float)samplePhi;
	float count = (float)(sampleTheta * samplePhi);
	float sumHarmonicDistance = 0.0f;

	Vector3 irradiance;
	float ignore;
	// �ڰ���ռ����ܼ�����
	for (float i = 0.0f; i < 1.0f; i += tTheta)
	{
		for (float j = 0.0f; j < 1.0f; j += tPhi)
		{
			// ��ÿ�����򶼼���
			Vector2 u(i, j);
			Vector3 nextDirLocal = SamplerMath::CosineSampleHemisphere(u);
			Vector3 nextDirWorld = hitInfo.BSDF->ReflectionToWorld(nextDirLocal);

			Ray nextRay = Ray(hitInfo.position, nextDirWorld);
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

			priority_queue_distance_cartesian<NXPhoton> nearestPhotons([posDiff](const NXPhoton& photonA, const NXPhoton& photonB) {
				float distA = Vector3::DistanceSquared(posDiff, photonA.position);
				float distB = Vector3::DistanceSquared(posDiff, photonB.position);
				return distA < distB;
				});

			hitInfoDiffuse.GenerateBSDF(true);

			// ʹ���ֳɵ�ȫ�ֹ���ͼ�����Կ���ͳ�Ƴ�ĳһ�㸽����Radiance��
			Vector3 radiance(0.0f);
			m_pPhotonMap->GetNearest(posDiff, normDiff, ignore, nearestPhotons, 100, FLT_MAX, LocateFilter::Disk);
			if (!nearestPhotons.empty())
			{
				float radius2 = Vector3::DistanceSquared(posDiff, nearestPhotons.top().position);
				while (!nearestPhotons.empty())
				{
					float pdfPhoton;
					auto photon = nearestPhotons.top();
					Vector3 f = hitInfoDiffuse.BSDF->Evaluate(-nextRay.direction, photon.direction, pdfPhoton);
					radiance += f * photon.power;
					nearestPhotons.pop();
				}

				radiance /= (XM_PI * radius2);
			}

			irradiance += radiance; // * hitInfo.shading.normal.Dot(nextDirWorld) / SamplerMath::CosineSampleHemispherePdf(���������������˷���Ҳ�ᱻ������);
		}
	}
	// ���ͳ��ƽ��ֵ
	irradiance *= XM_PI / count;	// XM_PI����������������������ᵼ�°�������pdf��piδ�������������ﲹһ�¡�

	// ������Ϻ󴢴�
	oCacheInfo.position = hitInfo.position;
	oCacheInfo.irradiance = irradiance;
	oCacheInfo.normal = hitInfo.shading.normal;
	oCacheInfo.harmonicDistance = count / sumHarmonicDistance;
	//printf("+cache: %f\n", oCacheInfo.harmonicDistance);
	return irradiance;
}
