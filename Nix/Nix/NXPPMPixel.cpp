#include "NXPPMPixel.h"
#include "NXIntersection.h"
#include "NXScene.h"
#include "NXPrimitive.h"
#include "NXCubeMap.h"

bool NXPPMPixelGenerator::CalculatePixelInfo(const Ray& cameraRay, const std::shared_ptr<NXScene>& pScene, const XMINT2& pixel, float pixelWeight, std::shared_ptr<PPMPixel>& oInfo, int depth)
{
	int maxDepth = 10;

	Ray ray(cameraRay);
	Vector3 nextDirection;
	float pdf;
	NXHit hitInfo;

	Vector3 Lemit(0.0f), throughput(1.0f);
	bool bIsDiffuse = false;
	while (depth < maxDepth)
	{
		hitInfo = NXHit();
		bool bIsIntersect = pScene->RayCast(ray, hitInfo);
		if (!bIsIntersect)
			return false;

		std::shared_ptr<NXPBRAreaLight> pHitAreaLight;
		if (hitInfo.pPrimitive)pHitAreaLight = hitInfo.pPrimitive->GetTangibleLight();
		else if (pScene->GetCubeMap()) pHitAreaLight = pScene->GetCubeMap()->GetEnvironmentLight();
		if (pHitAreaLight)
		{
			Lemit += throughput * pHitAreaLight->GetRadiance(hitInfo.position, hitInfo.normal, -ray.direction);
		}

		hitInfo.GenerateBSDF(true);
		std::shared_ptr<NXBSDF::SampleEvents> sampleEvent = std::make_shared<NXBSDF::SampleEvents>();
		Vector3 f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
		bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
		sampleEvent.reset();

		if (bIsDiffuse) 
			break;

		throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
		ray = Ray(hitInfo.position, nextDirection);
		ray.position += ray.direction * NXRT_EPSILON;
	}

	oInfo->pixel = pixel;
	oInfo->pixelWeight = pixelWeight;
	oInfo->position = hitInfo.position;
	oInfo->direction = hitInfo.direction;
	oInfo->normal = hitInfo.shading.normal;
	oInfo->BSDF = hitInfo.BSDF;
	oInfo->photons = 0;
	oInfo->radius2 = FLT_MAX;
	oInfo->flux = Vector3(0.0f);
	oInfo->Lemit = Lemit;
	return true;
}

void NXPPMPixelGenerator::Resize(const XMINT2& ImageSize, int eachPixelSamples)
{
	m_ImageSize = ImageSize;
	m_pixelInfoList.reserve(ImageSize.x * ImageSize.y * eachPixelSamples);
}

void NXPPMPixelGenerator::AddPixelInfo(const std::shared_ptr<PPMPixel>& info)
{
	XMINT2 p = info->pixel;
	m_pixelInfoList.push_back(info);
}
