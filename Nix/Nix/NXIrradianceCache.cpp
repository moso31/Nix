#include "NXIrradianceCache.h"
#include "NXScene.h"
#include "NXCubeMap.h"
#include "NXPrimitive.h"
#include "SamplerMath.h"

#include "NXCamera.h"
#include "ImageGenerator.h"

using namespace SamplerMath;

NXIrradianceCache::NXIrradianceCache() :
	m_threshold(0.25f)
{
}

void NXIrradianceCache::PreIrradiance(const Ray& cameraRay, const std::shared_ptr<NXScene>& pScene, int depth)
{
	int maxDepth = 10;

	Ray ray(cameraRay);
	Vector3 nextDirection;
	float pdf = 0.0f;
	NXHit hitInfo;

	// 只需要找到最终的首次漫反射位置。不需要计算吞吐量，那是Integrator该干的事儿。
	// Irradiance只需要对辐照缓存点进行定位。
	bool bIsDiffuse = false;
	while (true)
	{
		hitInfo = NXHit();
		bool bIsIntersect = pScene->RayCast(ray, hitInfo);
		if (!bIsIntersect)
			return;

		hitInfo.GenerateBSDF(true);

		std::shared_ptr<NXBSDF::SampleEvents> sampleEvent = std::make_shared<NXBSDF::SampleEvents>();
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

Vector3 NXIrradianceCache::Irradiance(const Ray& cameraRay, const std::shared_ptr<NXScene>& pScene, int depth)
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

		std::shared_ptr<NXBSDF::SampleEvents> sampleEvent = std::make_shared<NXBSDF::SampleEvents>();
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
		// 附近存在IrradianceCache点时，使用邻近Cache点快速估算。
		result += estimateIrradiance;
	}
	else
	{
		// 附近没有Cache点，就只能精确计算。
		// 具体做法：在半球空间上密集采样，对每个方向都计算，最后统计平均值。
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

Vector3 NXIrradianceCache::CalculateOneCache(const std::shared_ptr<NXScene>& pScene, const NXHit& hitInfo, int sampleTheta, int samplePhi, NXIrradianceCacheInfo& oCacheInfo)
{
	float tTheta = 1.0f / (float)sampleTheta;
	float tPhi = 1.0f / (float)samplePhi;
	float count = (float)(sampleTheta * samplePhi);
	float sumHarmonicDistance = 0.0f;

	Vector3 irradiance;
	float ignore;
	// 在半球空间上密集采样
	for (float i = 0.0f; i < 1.0f; i += tTheta)
	{
		for (float j = 0.0f; j < 1.0f; j += tPhi)
		{
			// 对每个方向都计算
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

			// 使用现成的全局光子图，可以快速统计出某一点附近的Radiance。
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

				float numPhotons = (float)m_pPhotonMap->GetPhotonCount();
				radiance /= (XM_PI * radius2 * numPhotons);
			}

			irradiance += radiance; // * hitInfo.shading.normal.Dot(nextDirWorld) / SamplerMath::CosineSampleHemispherePdf(不算两个余弦项了反正也会被抵消掉);
		}
	}
	// 最后统计平均值
	irradiance *= XM_PI / count;	// XM_PI：上面两个余弦项抵消，会导致半球余弦pdf的pi未处理。所以在这里补一下。

	// 计算完毕后储存
	oCacheInfo.position = hitInfo.position;
	oCacheInfo.irradiance = irradiance;
	oCacheInfo.normal = hitInfo.shading.normal;
	oCacheInfo.harmonicDistance = count / sumHarmonicDistance;
	//printf("+cache: %f\n", oCacheInfo.harmonicDistance);
	return irradiance;
}

void NXIrradianceCache::Render(const std::shared_ptr<NXScene>& pScene, const XMINT2& imageSize, std::string outFilePath)
{
	UINT nPixels = imageSize.x * imageSize.y;
	ImageBMPData* pImageData = new ImageBMPData[nPixels];
	memset(pImageData, 0, sizeof(ImageBMPData) * nPixels);

	std::shared_ptr<NXCamera> pCamera = pScene->GetMainCamera();
	Vector3 camPos = pCamera->GetTranslation();
	for(auto cache : m_caches)
	{
		Vector3 camDirView = Vector3::TransformNormal(cache.position - camPos, pCamera->GetViewMatrix());
		float tx = camDirView.x * pCamera->GetProjectionMatrix()._11 / camDirView.z;
		float ty = camDirView.y * pCamera->GetProjectionMatrix()._22 / camDirView.z;
		float x = (int)((tx + 1.0f) * 0.5f * imageSize.x);
		float y = (int)((1.0f - ty) * 0.5f * imageSize.y);

		int rgbIdx = (imageSize.y - y - 1) * imageSize.x + x;
		pImageData[rgbIdx].r = 255;
		pImageData[rgbIdx].g = 255;
		pImageData[rgbIdx].b = 255;
	}

	ImageGenerator::GenerateImageBMP((byte*)pImageData, imageSize.x, imageSize.y, outFilePath.c_str());
	delete pImageData;
}
