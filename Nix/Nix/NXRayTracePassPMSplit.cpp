#include "NXRayTracePassPMSplit.h"
#include "NXRandom.h"
#include "NXCamera.h"

void NXRayTracePassPMSplitPhotonMap::Load(const std::shared_ptr<NXScene>& pScene, int numCausticPhotons, int numGlobalPhotons)
{
	m_pScene = pScene;
	m_numCausticPhotons = numCausticPhotons;
	m_numGlobalPhotons = numGlobalPhotons;
}

void NXRayTracePassPMSplitPhotonMap::Render()
{
	printf("Generating global photons...");
	m_pGlobalPhotonMap = std::make_shared<NXPhotonMap>(m_numGlobalPhotons);
	m_pGlobalPhotonMap->Generate(m_pScene, PhotonMapType::Global);
	printf("done.\n");

	printf("Generating caustic photons...");
	m_pCausticPhotonMap = std::make_shared<NXPhotonMap>(m_numCausticPhotons);
	m_pCausticPhotonMap->Generate(m_pScene, PhotonMapType::Caustic);
	printf("done.\n");
}

void NXRayTracePassPMSplitIrradianceCache::Load(const std::shared_ptr<NXScene>& pScene, const XMINT2& imageSize, const std::shared_ptr<NXPhotonMap>& pPhotonMap)
{
	m_pScene = pScene;
	m_imageSize = imageSize;

	m_pIrradianceCache.reset();
	m_pIrradianceCache = std::make_shared<NXIrradianceCache>();
	m_pIrradianceCache->SetPhotonMaps(pPhotonMap);
}

void NXRayTracePassPMSplitIrradianceCache::Render()
{
	printf("Preloading irradiance caches...\n");
	// 进度条
	int process = 0;
	int pxCount = m_imageSize.x * m_imageSize.y;
	int barrier = 5000;
	int nodeCount = (pxCount + barrier - 1) / barrier;

#pragma omp parallel for
	for (int i = 0; i < m_imageSize.x; i++)
	{
		for (int j = 0; j < m_imageSize.y; j++)
		{
			Vector2 pixel((float)i, (float)j);

			Vector2 coord = pixel + Vector2(0.5f, 0.5f);
			Ray rayWorld = m_pScene->GetMainCamera()->GenerateRay(coord, Vector2((float)m_imageSize.x, (float)m_imageSize.y));
			m_pIrradianceCache->PreIrradiance(rayWorld, m_pScene, 0);

			process++;
			if (process % barrier == 0)
				printf("\r%.2f%% ", (float)process * 100 / pxCount);
		}
	}
	printf("done. (caches: %zd)\n", m_pIrradianceCache->GetCacheSize());
}

void NXRayTracePassPMSplit::Load(const std::shared_ptr<NXScene>& pScene, const std::shared_ptr<NXPhotonMap>& pGlobalPhotons, const std::shared_ptr<NXPhotonMap>& pCausticPhotons, const std::shared_ptr<NXIrradianceCache>& pIrradianceCache, const XMINT2& imageSize, const XMINT2& tileSize, int eachPixelSamples)
{
	m_pScene = pScene;
	m_imageSize = imageSize;
	m_tileSize = tileSize;
	m_eachPixelSamples = eachPixelSamples;

	m_pIntegrator.reset();
	m_pIntegrator = std::make_shared<NXPMSplitIntegrator>(pGlobalPhotons, pCausticPhotons);
	m_pIntegrator->SetIrradianceCache(pIrradianceCache);
}

void NXRayTracePassPMSplit::Render()
{
	printf("Rendering...");
	int pixelCount = m_imageSize.x * m_imageSize.y;
	ImageBMPData* pImageData = new ImageBMPData[pixelCount];
	memset(pImageData, 0, sizeof(ImageBMPData) * pixelCount);

	m_progress = 0;
	m_tileCount = XMINT2((m_imageSize.x + m_tileSize.x - 1) / m_tileSize.x, (m_imageSize.y + m_tileSize.y - 1) / m_tileSize.y);

	bool useOpenMP = false;
	RenderImageDataParallel(pImageData, useOpenMP);

	ImageGenerator::GenerateImageBMP((byte*)pImageData, m_imageSize.x, m_imageSize.y, m_outFilePath.c_str());
	delete pImageData;
	printf("done.\n");
}

void NXRayTracePassPMSplit::RenderImageDataParallel(ImageBMPData* pImageData, bool useOpenMP)
{
	// 两种并行方案：OpenMP 或 C++17 execution
	if (useOpenMP)
	{
#pragma omp parallel for
		for (int tx = 0; tx < m_tileCount.x; tx++)
			for (int ty = 0; ty < m_tileCount.y; ty++)
			{
				RenderTile(pImageData, XMINT2(tx, ty));
			}
	}
	else
	{
		std::vector<XMINT2> tasks;
		for (int tx = 0; tx < m_tileCount.x; tx++)
			for (int ty = 0; ty < m_tileCount.y; ty++)
				tasks.push_back(XMINT2(tx, ty));

		std::for_each(std::execution::par, tasks.begin(), tasks.end(), [this, pImageData](const XMINT2& tileId) {
			RenderTile(pImageData, tileId);
			});
	}
}

void NXRayTracePassPMSplit::RenderTile(ImageBMPData* pImageData, const XMINT2& tileId)
{
	for (int i = 0; i < m_tileSize.x; i++)
	{
		for (int j = 0; j < m_tileSize.y; j++)
		{
			Vector3 result(0.0f);
			int pixelX = tileId.x * m_tileSize.x + i;
			int pixelY = tileId.y * m_tileSize.y + j;
			if (pixelX >= m_imageSize.x || pixelY >= m_imageSize.y)
				continue;

			Vector2 pixelCoord((float)pixelX, (float)pixelY);
			for (int pixelSample = 0; pixelSample < m_eachPixelSamples; pixelSample++)
			{
				// pixel + [0, 1)^2.
				Vector2 sampleCoord = pixelCoord + NXRandom::GetInstance()->CreateVector2();

				Ray rayWorld = m_pScene->GetMainCamera()->GenerateRay(sampleCoord, Vector2((float)m_imageSize.x, (float)m_imageSize.y));
				result += m_pIntegrator->Radiance(rayWorld, m_pScene, 0);
			}
			result /= (float)m_eachPixelSamples;

			XMINT3 RGBValue(
				result.x > 1.0f ? 255 : (int)(result.x * 255.0f),
				result.y > 1.0f ? 255 : (int)(result.y * 255.0f),
				result.z > 1.0f ? 255 : (int)(result.z * 255.0f));

			int index = (m_tileSize.y - j - 1) * m_tileSize.x + i;
			int rgbIdx = (m_imageSize.y - pixelY - 1) * m_imageSize.x + pixelX;
			pImageData[rgbIdx].r = RGBValue.x;
			pImageData[rgbIdx].g = RGBValue.y;
			pImageData[rgbIdx].b = RGBValue.z;
		}
	}

	m_progress++;
	int blockCount = m_tileCount.x * m_tileCount.y;
	printf("\r%.2f%% ", (float)m_progress * 100.0f / (float)blockCount);
}
