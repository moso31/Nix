#include "NXSPPMIntegrator.h"
#include <vector>
#include "NXRandom.h"
#include "SamplerMath.h"
#include "NXCamera.h"
#include "NXCubeMap.h"
#include "NXPrimitive.h"

NXSPPMIntegrator::NXSPPMIntegrator(const XMINT2& imageSize, std::string outPath, UINT nCausticPhotons, UINT nGlobalPhotons) :
	NXIntegrator(imageSize),
	m_numCausticPhotonsEachStep(nCausticPhotons),
	m_numGlobalPhotonsEachStep(nGlobalPhotons),
	m_pGlobalPhotonMap(nullptr),
	m_pCausticPhotonMap(nullptr)
{
}

NXSPPMIntegrator::~NXSPPMIntegrator()
{
	if (m_pCausticPhotonMap)
	{
		m_pCausticPhotonMap->Release();
		delete m_pCausticPhotonMap;
	}
	if (m_pGlobalPhotonMap)
	{
		m_pGlobalPhotonMap->Release();
		delete m_pGlobalPhotonMap;
	}
}

void NXSPPMIntegrator::Render(NXScene* pScene)
{
	bool bSplitRender = 1;

	printf("Building BVH Trees...");
	pScene->BuildBVHTrees(HLBVH);
	printf("Done.\n");

	int nPixels = m_imageSize.x * m_imageSize.y;
	std::unique_ptr<NXSPPMPixel[]> pixels(new NXSPPMPixel[nPixels]);

#pragma omp parallel for 
	for (int i = 0; i < nPixels; i++)
	{
		// SPPMPixels 初始化
		pixels[i].causticRadius2 = 1e-7f;
		pixels[i].globalRadius2 = 1e-7f;
	}

	for (int k = 0; k < 2; k++)
	{
		printf("SPPM iteration sequences rendering...(Image %d)\n", k);

		bool bRenderOnce = (k < 0) || (k % 10 == 0);
		ImageBMPData* pImageData = nullptr;
		if (bRenderOnce)
		{
			pImageData = new ImageBMPData[nPixels];
			memset(pImageData, 0, sizeof(ImageBMPData) * nPixels);
		}

		printf("Refreshing photon map...");
		k == 0 ? BuildPhotonMap(pScene) : RefreshPhotonMap(pScene);
		printf("done.\n");

		m_progress = 0;
		printf("Rendering...");

		bSplitRender ?
			RenderWithPMSplit(pScene, pixels, pImageData, k, bRenderOnce) :
			RenderWithPM(pScene, pixels, pImageData, k, bRenderOnce);

		m_outFilePath = "D:\\Nix_SPPM_" + std::to_string(k) + ".bmp";
		if (bRenderOnce)
		{
			ImageGenerator::GenerateImageBMP((byte*)pImageData, m_imageSize.x, m_imageSize.y, m_outFilePath.c_str());
			delete pImageData;
		}
		printf("done.\n");
	}
}

void NXSPPMIntegrator::BuildPhotonMap(NXScene* pScene)
{
	m_pCausticPhotonMap = new NXPhotonMap(m_numCausticPhotonsEachStep);
	m_pCausticPhotonMap->Generate(pScene, PhotonMapType::Caustic);
	m_pGlobalPhotonMap = new NXPhotonMap(m_numGlobalPhotonsEachStep);
	m_pGlobalPhotonMap->Generate(pScene, PhotonMapType::Global);
}

void NXSPPMIntegrator::RefreshPhotonMap(NXScene* pScene)
{
	if (m_pCausticPhotonMap) m_pCausticPhotonMap->Release();
	m_pCausticPhotonMap->Generate(pScene, PhotonMapType::Caustic);
	if (m_pGlobalPhotonMap) m_pGlobalPhotonMap->Release();
	m_pGlobalPhotonMap->Generate(pScene, PhotonMapType::Global);
}

void NXSPPMIntegrator::RenderWithPM(NXScene* pScene, std::unique_ptr<NXSPPMPixel[]>& oPixels, ImageBMPData* pImageData, int iter, bool RenderOnce)
{
#pragma omp parallel for
	for (int i = 0; i < m_imageSize.x; i++)
	{
		for (int j = 0; j < m_imageSize.y; j++)
			RenderWithPM(pScene, oPixels, pImageData, iter, RenderOnce, i, j);
		printf("\r%.2f%% ", (float)++m_progress * 100.0f / (float)m_imageSize.x);	// 进度条更新
	}
}

void NXSPPMIntegrator::RenderWithPMSplit(NXScene* pScene, std::unique_ptr<NXSPPMPixel[]>& oPixels, ImageBMPData* pImageData, int iter, bool RenderOnce)
{
#pragma omp parallel for
	for (int i = 0; i < m_imageSize.x; i++)
	{
		for (int j = 0; j < m_imageSize.y; j++)
			RenderWithPMSplit(pScene, oPixels, pImageData, iter, RenderOnce, i, j);
		printf("\r%.2f%% ", (float)++m_progress * 100.0f / (float)m_imageSize.x);	// 进度条更新
	}
}

void NXSPPMIntegrator::RenderWithPMArea(NXScene* pScene, std::unique_ptr<NXSPPMPixel[]>& oPixels, ImageBMPData* pImageData, int iter, bool RenderOnce, int x, int y, int offsetX, int offsetY)
{
#pragma omp parallel for
	for (int i = x; i < min(x + offsetX, m_imageSize.x); i++)
	{
		for (int j = y; j < min(y + offsetY, m_imageSize.y); j++)
		{
			RenderWithPM(pScene, oPixels, pImageData, iter, RenderOnce, i, j);
		}
	}
}

void NXSPPMIntegrator::RenderWithPMSplitArea(NXScene* pScene, std::unique_ptr<NXSPPMPixel[]>& oPixels, ImageBMPData* pImageData, int iter, bool RenderOnce, int x, int y, int offsetX, int offsetY)
{
#pragma omp parallel for
	for (int i = x; i < min(x + offsetX, m_imageSize.x); i++)
	{
		for (int j = y; j < min(y + offsetY, m_imageSize.y); j++)
		{
			RenderWithPMSplit(pScene, oPixels, pImageData, iter, RenderOnce, i, j);
		}
	}
}

void NXSPPMIntegrator::RenderWithPM(NXScene* pScene, std::unique_ptr<NXSPPMPixel[]>& oPixels, ImageBMPData* pImageData, int iter, bool RenderOnce, int x, int y)
{
	Vector2 pixelCoord((float)x, (float)y);
	Vector2 sampleCoord = pixelCoord + NXRandom::GetInstance().CreateVector2();
	Ray ray = pScene->GetMainCamera()->GenerateRay(sampleCoord, Vector2((float)m_imageSize.x, (float)m_imageSize.y));

	UINT pixelOffset = (m_imageSize.y - y - 1) * m_imageSize.x + x;
	NXSPPMPixel& pixel = oPixels[pixelOffset];

	Vector3 nextDirection;
	float pdf;
	NXHit hitInfo;

	Vector3 f(0.0f);
	Vector3 Le(0.0f), Lr(0.0f);
	Vector3 throughput(1.0f);

	bool isDeltaBSDF = false;
	bool bIsDiffuse = false;
	bool bIsIntersect = false;
	int depth = 0;
	while (true)
	{
		bIsIntersect = pScene->RayCast(ray, hitInfo);
		if (depth == 0 || isDeltaBSDF)
		{
			if (!bIsIntersect)
			{
				auto pCubeMap = pScene->GetCubeMap();
				if (pCubeMap)
				{
					auto pCubeMapLight = pCubeMap->GetEnvironmentLight();
					if (pCubeMapLight)
						Le += throughput * pCubeMapLight->GetRadiance(hitInfo.position, hitInfo.normal, ray.direction);
				}

				break;
			}

			if (hitInfo.pPrimitive)
			{
				auto pTangibleLight = hitInfo.pPrimitive->GetTangibleLight();
				if (pTangibleLight)
				{
					Le += throughput * pTangibleLight->GetRadiance(hitInfo.position, hitInfo.normal, hitInfo.direction);
				}
			}
		}

		if (!bIsIntersect) break;
		hitInfo.GenerateBSDF(true);

		NXBSDF::SampleEvents* sampleEvent = new NXBSDF::SampleEvents();
		f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
		bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
		isDeltaBSDF = *sampleEvent & NXBSDF::DELTA;
		delete sampleEvent;

		if (f.IsZero() || pdf == 0) break;
		if (bIsDiffuse) break;

		throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
		ray = Ray(hitInfo.position, nextDirection);
		ray.position += ray.direction * NXRT_EPSILON;

		depth++;
	}

	pixel.radiance += Le;

	if (f.IsZero() || pdf == 0)
	{
		if (RenderOnce)
		{
			Lr = pixel.globalFlux / (XM_PI * (float)m_pGlobalPhotonMap->GetPhotonCount() * (iter + 1) * pixel.globalRadius2);
			Vector3 result = (pixel.radiance / (float)(iter + 1)) + Lr;

			XMINT3 RGBValue(
				result.x > 1.0f ? 255 : (int)(result.x * 255.0f),
				result.y > 1.0f ? 255 : (int)(result.y * 255.0f),
				result.z > 1.0f ? 255 : (int)(result.z * 255.0f));

			int rgbIdx = (m_imageSize.y - y - 1) * m_imageSize.x + x;
			pImageData[rgbIdx].r = RGBValue.x;
			pImageData[rgbIdx].g = RGBValue.y;
			pImageData[rgbIdx].b = RGBValue.z;
		}
		return;
	}

	if (bIsIntersect)
	{
		Vector3 posDiff = hitInfo.position;
		Vector3 normDiff = hitInfo.shading.normal;

		hitInfo.GenerateBSDF(true);

		priority_queue_distance_cartesian<NXPhoton> nearestPhotons([posDiff](const NXPhoton& photonA, const NXPhoton& photonB) {
			float distA = Vector3::DistanceSquared(posDiff, photonA.position);
			float distB = Vector3::DistanceSquared(posDiff, photonB.position);
			return distA < distB;
			});

		const static float alpha = 0.66666667f;
		UINT photons;	// 下一次累积的光子数量
		float radius2;
		if (!pixel.globalEstimateFlag)
		{
			photons = 100;
			m_pGlobalPhotonMap->GetNearest(posDiff, normDiff, radius2, nearestPhotons, photons, FLT_MAX, LocateFilter::Sphere);
		}
		else
		{
			m_pGlobalPhotonMap->GetNearest(posDiff, normDiff, radius2, nearestPhotons, -1, pixel.globalRadius2, LocateFilter::Sphere);
			photons = pixel.globalPhotons + (UINT)(alpha * nearestPhotons.size());
			radius2 = nearestPhotons.empty() ?
				pixel.globalRadius2 :
				pixel.globalRadius2 * ((float)photons / (pixel.globalPhotons + (UINT)nearestPhotons.size()));
		}

		Vector3 flux(0.0f);
		while (!nearestPhotons.empty())
		{
			auto photon = nearestPhotons.top();
			Vector3 f = hitInfo.BSDF->Evaluate(-ray.direction, photon.direction, pdf);
			if (!f.IsZero() && pdf != 0)
				flux += f * photon.power;
			nearestPhotons.pop();
		}
		flux *= throughput;

		if (pixel.globalEstimateFlag)
			flux = (pixel.globalFlux + flux) * radius2 / pixel.globalRadius2;
		else
			pixel.globalEstimateFlag = true;

		pixel.globalFlux = flux;
		pixel.globalPhotons = photons;
		pixel.globalRadius2 = radius2;
		pixel.radiance += Le;
	}

	if (RenderOnce)
	{
		Lr = pixel.globalFlux / (XM_PI * (float)m_pGlobalPhotonMap->GetPhotonCount() * (iter + 1) * pixel.globalRadius2);
		Vector3 result = (pixel.radiance / (float)(iter + 1)) + Lr;

		XMINT3 RGBValue(
			result.x > 1.0f ? 255 : (int)(result.x * 255.0f),
			result.y > 1.0f ? 255 : (int)(result.y * 255.0f),
			result.z > 1.0f ? 255 : (int)(result.z * 255.0f));

		int rgbIdx = (m_imageSize.y - y - 1) * m_imageSize.x + x;
		pImageData[rgbIdx].r = RGBValue.x;
		pImageData[rgbIdx].g = RGBValue.y;
		pImageData[rgbIdx].b = RGBValue.z;
	}
}

void NXSPPMIntegrator::RenderWithPMSplit(NXScene* pScene, std::unique_ptr<NXSPPMPixel[]>& oPixels, ImageBMPData* pImageData, int iter, bool RenderOnce, int x, int y)
{
	Vector2 pixelCoord((float)x, (float)y);
	Vector2 sampleCoord = pixelCoord + NXRandom::GetInstance().CreateVector2();
	Ray ray = pScene->GetMainCamera()->GenerateRay(sampleCoord, Vector2((float)m_imageSize.x, (float)m_imageSize.y));

	UINT pixelOffset = (m_imageSize.y - y - 1) * m_imageSize.x + x;
	NXSPPMPixel& pixel = oPixels[pixelOffset];

	Vector3 nextDirection;
	float pdf, pdfPhotons;
	NXHit hitInfo;

	Vector3 f(0.0f);
	Vector3 Le(0.0f), Ld(0.0f), LrCaustic(0.0f), LrGlobal(0.0f);
	Vector3 throughput(1.0f);

	bool isDeltaBSDF = false;
	bool bIsDiffuse = false;
	bool bIsIntersect = false;
	int depth = 0;
	while (true)
	{
		bIsIntersect = pScene->RayCast(ray, hitInfo);
		if (depth == 0 || isDeltaBSDF)
		{
			if (!bIsIntersect)
			{
				auto pCubeMap = pScene->GetCubeMap();
				if (pCubeMap)
				{
					auto pCubeMapLight = pCubeMap->GetEnvironmentLight();
					if (pCubeMapLight)
						Le += throughput * pCubeMapLight->GetRadiance(hitInfo.position, hitInfo.normal, ray.direction);
				}

				break;
			}

			if (hitInfo.pPrimitive)
			{
				auto pTangibleLight = hitInfo.pPrimitive->GetTangibleLight();
				if (pTangibleLight)
				{
					Le += throughput * pTangibleLight->GetRadiance(hitInfo.position, hitInfo.normal, hitInfo.direction);
				}
			}
		}

		if (!bIsIntersect) break;
		hitInfo.GenerateBSDF(true);
		Ld += throughput * UniformLightOne(ray, pScene, hitInfo);

		NXBSDF::SampleEvents* sampleEvent = new NXBSDF::SampleEvents();
		f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
		bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
		isDeltaBSDF = *sampleEvent & NXBSDF::DELTA;
		delete sampleEvent;

		if (f.IsZero() || pdf == 0) break;
		if (bIsDiffuse) break;

		throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
		if (Vector3::IsNaN(throughput))
		{
			printf("nextDirection: %f %f %f\n", nextDirection.x, nextDirection.y, nextDirection.z);
			printf("f: %f %f %f\n", f.x, f.y, f.z);
			printf("pdf: %f\n", pdf);
		}
		ray = Ray(hitInfo.position, nextDirection);
		ray.position += ray.direction * NXRT_EPSILON;

		depth++;
	}

	pixel.radiance += Le + Ld;

	if (f.IsZero() || pdf == 0)
	{
		if (RenderOnce)
		{
			LrCaustic = pixel.causticFlux / (XM_PI * (float)m_pCausticPhotonMap->GetPhotonCount() * (iter + 1) * pixel.causticRadius2);
			LrGlobal = pixel.globalFlux / (XM_PI * (float)m_pGlobalPhotonMap->GetPhotonCount() * (iter + 1) * pixel.globalRadius2);
			Vector3 result = (pixel.radiance / (float)(iter + 1)) + LrGlobal + LrCaustic;

			XMINT3 RGBValue(
				result.x > 1.0f ? 255 : (int)(result.x * 255.0f),
				result.y > 1.0f ? 255 : (int)(result.y * 255.0f),
				result.z > 1.0f ? 255 : (int)(result.z * 255.0f));

			int rgbIdx = (m_imageSize.y - y - 1) * m_imageSize.x + x;
			pImageData[rgbIdx].r = RGBValue.x;
			pImageData[rgbIdx].g = RGBValue.y;
			pImageData[rgbIdx].b = RGBValue.z;
		}
		return;
	}

	const static float alpha = 0.66666667f;

	if (bIsIntersect)
	{
		// caustics
		Vector3 pos = hitInfo.position;
		Vector3 norm = hitInfo.shading.normal;

		priority_queue_distance_cartesian<NXPhoton> nearestCausticPhotons([pos](const NXPhoton& photonA, const NXPhoton& photonB) {
			float distA = Vector3::DistanceSquared(pos, photonA.position);
			float distB = Vector3::DistanceSquared(pos, photonB.position);
			return distA < distB;
			});

		UINT causticPhotons;	// 下一次累积的光子数量
		float causticRadius2;
		if (!pixel.causticEstimateFlag)
		{
			causticPhotons = 50;
			m_pCausticPhotonMap->GetNearest(pos, norm, causticRadius2, nearestCausticPhotons, causticPhotons, 0.15f, LocateFilter::Sphere);
		}
		else
		{
			m_pCausticPhotonMap->GetNearest(pos, norm, causticRadius2, nearestCausticPhotons, -1, pixel.causticRadius2, LocateFilter::Sphere);
			causticPhotons = pixel.causticPhotons + (UINT)(alpha * nearestCausticPhotons.size());
			causticRadius2 = nearestCausticPhotons.empty() ?
				pixel.causticRadius2 :
				pixel.causticRadius2 * ((float)causticPhotons / (pixel.causticPhotons + (UINT)nearestCausticPhotons.size()));
		}

		Vector3 causticFlux(0.0f);
		if (!nearestCausticPhotons.empty())
		{
			while (!nearestCausticPhotons.empty())
			{
				auto photon = nearestCausticPhotons.top();
				Vector3 f = hitInfo.BSDF->Evaluate(-ray.direction, photon.direction, pdfPhotons);
				if (!f.IsZero() && pdfPhotons != 0.0f)
					causticFlux += f * photon.power;
				nearestCausticPhotons.pop();
			}
			causticFlux *= throughput;
		}

		if (pixel.causticEstimateFlag)
			causticFlux = (pixel.causticFlux + causticFlux) * causticRadius2 / pixel.causticRadius2;
		else
			pixel.causticEstimateFlag = true;

		pixel.causticFlux = causticFlux;
		pixel.causticPhotons = causticPhotons;
		pixel.causticRadius2 = causticRadius2;
	}

	// multiple diffuse reflections.
	throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
	Ray nextRay = Ray(hitInfo.position, nextDirection);
	nextRay.position += nextRay.direction * NXRT_EPSILON;

	if (Vector3::IsNaN(throughput))
	{
		printf("nextDirection: %f %f %f\n", nextDirection.x, nextDirection.y, nextDirection.z);
		printf("f: %f %f %f\n", f.x, f.y, f.z);
		printf("pdf: %f\n", pdf);
	}

	bIsDiffuse = false;
	NXHit hitInfoDiffuse;
	while (true)
	{
		bIsIntersect = pScene->RayCast(nextRay, hitInfoDiffuse);
		if (!bIsIntersect)
			break;

		hitInfoDiffuse.GenerateBSDF(true);

		NXBSDF::SampleEvents* sampleEvent = new NXBSDF::SampleEvents();
		f = hitInfoDiffuse.BSDF->Sample(hitInfoDiffuse.direction, nextDirection, pdf, sampleEvent);
		bIsDiffuse = *sampleEvent & NXBSDF::DIFFUSE;
		delete sampleEvent;

		if (f.IsZero() || pdf == 0) break;
		if (bIsDiffuse) break;

		throughput *= f * fabsf(hitInfoDiffuse.shading.normal.Dot(nextDirection)) / pdf;
		if (Vector3::IsNaN(throughput))
		{
			printf("nextDirection: %f %f %f\n", nextDirection.x, nextDirection.y, nextDirection.z);
			printf("f: %f %f %f\n", f.x, f.y, f.z);
			printf("pdf: %f\n", pdf);
		}
		nextRay = Ray(hitInfoDiffuse.position, nextDirection);
		nextRay.position += nextRay.direction * NXRT_EPSILON;
	}

	if (f.IsZero() || pdf == 0)
	{
		if (RenderOnce)
		{
			LrCaustic = pixel.causticFlux / (XM_PI * (float)m_pCausticPhotonMap->GetPhotonCount() * (iter + 1) * pixel.causticRadius2);
			LrGlobal = pixel.globalFlux / (XM_PI * (float)m_pGlobalPhotonMap->GetPhotonCount() * (iter + 1) * pixel.globalRadius2);
			Vector3 result = (pixel.radiance / (float)(iter + 1)) + LrGlobal + LrCaustic;

			XMINT3 RGBValue(
				result.x > 1.0f ? 255 : (int)(result.x * 255.0f),
				result.y > 1.0f ? 255 : (int)(result.y * 255.0f),
				result.z > 1.0f ? 255 : (int)(result.z * 255.0f));

			int rgbIdx = (m_imageSize.y - y - 1) * m_imageSize.x + x;
			pImageData[rgbIdx].r = RGBValue.x;
			pImageData[rgbIdx].g = RGBValue.y;
			pImageData[rgbIdx].b = RGBValue.z;
		}
		return;
	}

	if (bIsIntersect)
	{
		Vector3 posDiff = hitInfoDiffuse.position;
		Vector3 normDiff = hitInfoDiffuse.shading.normal;

		priority_queue_distance_cartesian<NXPhoton> nearestGlobalPhotons([posDiff](const NXPhoton& photonA, const NXPhoton& photonB) {
			float distA = Vector3::DistanceSquared(posDiff, photonA.position);
			float distB = Vector3::DistanceSquared(posDiff, photonB.position);
			return distA < distB;
			});

		UINT globalPhotons;	// 下一次累积的光子数量
		float globalRadius2;
		if (!pixel.globalEstimateFlag)
		{
			globalPhotons = 100;
			m_pGlobalPhotonMap->GetNearest(posDiff, normDiff, globalRadius2, nearestGlobalPhotons, globalPhotons, FLT_MAX, LocateFilter::Sphere);
		}
		else
		{
			m_pGlobalPhotonMap->GetNearest(posDiff, normDiff, globalRadius2, nearestGlobalPhotons, -1, pixel.globalRadius2, LocateFilter::Sphere);
			globalPhotons = pixel.globalPhotons + (UINT)(alpha * nearestGlobalPhotons.size());
			globalRadius2 = nearestGlobalPhotons.empty() ?
				pixel.globalRadius2 :
				pixel.globalRadius2 * ((float)globalPhotons / (pixel.globalPhotons + (UINT)nearestGlobalPhotons.size()));
		}

		if (globalRadius2 == 0.0f)
		{
			printf("globalPhotons: %d\n", globalPhotons);
			printf("pixel.globalRadius2: %f\n", pixel.globalRadius2);
			printf("pixel.globalPhotons: %d\n", pixel.globalPhotons);
			printf("nearestGlobalPhotons.size(): %d\n", (int)nearestGlobalPhotons.size());
		}

		Vector3 globalFlux(0.0f);
		if (!nearestGlobalPhotons.empty())
		{
			while (!nearestGlobalPhotons.empty())
			{
				auto photon = nearestGlobalPhotons.top();
				Vector3 f = hitInfoDiffuse.BSDF->Evaluate(-nextRay.direction, photon.direction, pdfPhotons);
				if (!f.IsZero() && pdfPhotons != 0.0f)
					globalFlux += f * photon.power;
				nearestGlobalPhotons.pop();
			}
			globalFlux *= throughput;
		}

		if (pixel.globalEstimateFlag) 
			globalFlux = (pixel.globalFlux + globalFlux) * globalRadius2 / pixel.globalRadius2;
		else 
			pixel.globalEstimateFlag = true;

		pixel.globalFlux = globalFlux;
		pixel.globalPhotons = globalPhotons;
		pixel.globalRadius2 = globalRadius2;
	}

	if (RenderOnce)
	{
		LrCaustic = pixel.causticFlux / (XM_PI * (float)m_pCausticPhotonMap->GetPhotonCount() * (iter + 1) * pixel.causticRadius2);
		LrGlobal = pixel.globalFlux / (XM_PI * (float)m_pGlobalPhotonMap->GetPhotonCount() * (iter + 1) * pixel.globalRadius2);
		Vector3 result = (pixel.radiance / (float)(iter + 1)) + LrGlobal + LrCaustic;

		XMINT3 RGBValue(
			result.x > 1.0f ? 255 : (int)(result.x * 255.0f),
			result.y > 1.0f ? 255 : (int)(result.y * 255.0f),
			result.z > 1.0f ? 255 : (int)(result.z * 255.0f));

		int rgbIdx = (m_imageSize.y - y - 1) * m_imageSize.x + x;
		pImageData[rgbIdx].r = RGBValue.x;
		pImageData[rgbIdx].g = RGBValue.y;
		pImageData[rgbIdx].b = RGBValue.z;
	}
}
