#include "NXRayTracePassDirect.h"
#include "NXRandom.h"
#include "NXCamera.h"

void NXRayTracePassDirect::Load(const std::shared_ptr<NXScene>& pScene, XMINT2 imageSize, XMINT2 tileSize, int eachPixelSamples)
{
	m_pScene = pScene;
	m_imageSize = imageSize;
	m_tileSize = tileSize;
	m_eachPixelSamples = eachPixelSamples;

	m_imageSizeInv = Vector2(1.0f / (float)m_imageSize.x, 1.0f / (float)m_imageSize.y);

	m_pIntegrator.reset();
	m_pIntegrator = std::make_shared<NXDirectIntegrator>();
}

void NXRayTracePassDirect::Render()
{
	int pixelCount = m_imageSize.x * m_imageSize.y;
	ImageBMPData* pImageData = new ImageBMPData[pixelCount];
	memset(pImageData, 0, sizeof(ImageBMPData) * pixelCount);

	m_progress = 0;
	m_tileCount = XMINT2((m_imageSize.x + m_tileSize.x - 1) / m_tileSize.x, (m_imageSize.y + m_tileSize.y - 1) / m_tileSize.y);

	bool useOpenMP = false;
	RenderImageDataParallel(pImageData, useOpenMP);

	ImageGenerator::GenerateImageBMP((byte*)pImageData, m_imageSize.x, m_imageSize.y, m_outFilePath.c_str());
	delete pImageData;
}

void NXRayTracePassDirect::RenderImageDataParallel(ImageBMPData* pImageData, bool useOpenMP)
{
	// 两种并行方案：OpenMP 或 C++17 execution
	if (useOpenMP)
	{
#pragma omp parallel for
		for (int tx = 0; tx < m_tileCount.x; tx++)
			for (int ty = 0; ty < m_tileCount.y; ty++)
			{
				NXRayTracePassDirect::RenderTile(pImageData, XMINT2(tx, ty));
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

void NXRayTracePassDirect::RenderTile(ImageBMPData* pImageData, const XMINT2& tileId)
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
