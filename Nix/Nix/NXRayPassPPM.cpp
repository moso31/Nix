#include "NXRayPassPPM.h"
#include "NXRandom.h"
#include "NXScene.h"
#include "NXCamera.h"

void NXRayPassPPMGeneratePixels::Load(const std::shared_ptr<NXScene>& pScene, const XMINT2& imageSize, const XMINT2& tileSize, int eachPixelSamples)
{
	m_pScene = pScene;
	m_pPPMPixelGenerator.reset();
	m_pPPMPixelGenerator = std::make_shared<NXPPMPixelGenerator>(); 
	m_pPPMPixelGenerator->Resize(imageSize, eachPixelSamples);

	m_imageSize = imageSize;
	m_tileSize = tileSize;
	m_eachPixelSamples = eachPixelSamples;

	m_tileCount = XMINT2((m_imageSize.x + m_tileSize.x - 1) / m_tileSize.x, (m_imageSize.y + m_tileSize.y - 1) / m_tileSize.y);
}

void NXRayPassPPMGeneratePixels::Render()
{
	printf("Generating PPM Pixels List...\n");
	bool useOpenMP = false;
	RenderImageDataParallel(useOpenMP);
	printf("done.\n");
}

void NXRayPassPPMGeneratePixels::RenderImageDataParallel(bool useOpenMP)
{
	// 两种并行方案：OpenMP 或 C++17 execution
	if (useOpenMP)
	{
#pragma omp parallel for
		for (int tx = 0; tx < m_tileCount.x; tx++)
			for (int ty = 0; ty < m_tileCount.y; ty++)
			{
				RenderTile(XMINT2(tx, ty));
			}
	}
	else
	{
		std::vector<XMINT2> tasks;
		for (int tx = 0; tx < m_tileCount.x; tx++)
			for (int ty = 0; ty < m_tileCount.y; ty++)
				tasks.push_back(XMINT2(tx, ty));

		std::for_each(std::execution::par, tasks.begin(), tasks.end(), [this](const XMINT2& tileId) {
			RenderTile(tileId);
			});
	}
}

void NXRayPassPPMGeneratePixels::RenderTile(const XMINT2& tileId)
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
			float pixelWeight = 1.0f / (float)m_eachPixelSamples;
			for (int pixelSample = 0; pixelSample < m_eachPixelSamples; pixelSample++)
			{
				// pixel + [0, 1)^2.
				Vector2 sampleCoord = pixelCoord + NXRandom::GetInstance()->CreateVector2();

				Ray rayWorld = m_pScene->GetMainCamera()->GenerateRay(sampleCoord, Vector2((float)m_imageSize.x, (float)m_imageSize.y));

				std::shared_ptr<PPMPixel> pixelInfo = std::make_shared<PPMPixel>();
				if (m_pPPMPixelGenerator->CalculatePixelInfo(rayWorld, m_pScene, XMINT2(pixelX, pixelY), pixelWeight, pixelInfo, 0))
				{
					std::lock_guard<std::mutex> guard(m_mutexPixelInfo);
					//m_mutexPixelInfo.lock();
					m_pPPMPixelGenerator->AddPixelInfo(pixelInfo);
					//m_mutexPixelInfo.unlock();
				}
			}
		}
	}

	//m_progress++;
	//int blockCount = m_tileCount.x * m_tileCount.y;
	//printf("\r%.2f%% ", (float)m_progress * 100.0f / (float)blockCount);
}

void NXRayPassPPM::Load(const std::shared_ptr<NXScene>& pScene, const std::shared_ptr<NXPPMPixelGenerator>& pPPMPixelGenerator, const XMINT2& imageSize, int nPhotonsAtOnce)
{
	m_pScene = pScene;
	m_imageSize = imageSize;
	m_pPPMPixelGenerator = pPPMPixelGenerator;
	m_pImageData = new ImageBMPData[m_imageSize.x * m_imageSize.y];

	m_accumulatePhotons = 0;
	m_nPhotonsAtOnce = nPhotonsAtOnce;
}

void NXRayPassPPM::GeneratePhotonMap()
{
	m_pPhotonMap = std::make_shared<NXPhotonMap>(m_nPhotonsAtOnce);
	m_pPhotonMap->Generate(m_pScene, PhotonMapType::Global);
}

void NXRayPassPPM::Render()
{
	printf("Rendering...");
	int pixelCount = m_imageSize.x * m_imageSize.y;
	memset(m_pImageData, 0, sizeof(ImageBMPData) * pixelCount);

	m_accumulatePhotons += m_nPhotonsAtOnce;
	auto pixelList = m_pPPMPixelGenerator->GetPixelInfoList();
	for (auto it = pixelList.begin(); it != pixelList.end(); it++)
	{
		RenderOneSample(m_pImageData, **it);
	}

	ImageGenerator::GenerateImageBMP((byte*)m_pImageData, m_imageSize.x, m_imageSize.y, m_outFilePath.c_str());
	printf("done.\n");
}

void NXRayPassPPM::Release()
{
	delete m_pImageData;
}

void NXRayPassPPM::RenderOneSample(ImageBMPData* pImageData, PPMPixel& pixel)
{
	Vector3 L(0.0f);
	Vector3 pos = pixel.position;
	priority_queue_distance_cartesian<NXPhoton> nearestPhotons([pos](const NXPhoton& photonA, const NXPhoton& photonB) {
		float distA = Vector3::DistanceSquared(pos, photonA.position);
		float distB = Vector3::DistanceSquared(pos, photonB.position);
		return distA < distB;
		});

	float distSqr;
	if (!pixel.photons)	// 是首次迭代
	{
		m_pPhotonMap->GetNearest(pixel.position, pixel.normal, distSqr, nearestPhotons, 500, FLT_MAX, LocateFilter::Disk);
		pixel.radius2 = distSqr;	// 是首次迭代需要借助数量求出半径。
	}
	else // 第2-n次
		m_pPhotonMap->GetNearest(pixel.position, pixel.normal, distSqr, nearestPhotons, -1, pixel.radius2, LocateFilter::Disk);

	if (nearestPhotons.empty()) return;

	int photonCount = (int)nearestPhotons.size();	// 新的光子数量
	float estimateArea = XM_PI * pixel.radius2;
	float estimateDestiny = (float)(pixel.photons + photonCount) / estimateArea;

	float alpha = pixel.photons ? 0.7f : 1.0f;	// 第一次设为1.0f（完全保留），后续0.7f（部分保留）。
	float ds = (pixel.photons + alpha * (float)photonCount) / (pixel.photons * (float)photonCount);

	// 新的估计半径
	float newRadius2 = pixel.radius2 * ds;
	float pdf;
	while (!nearestPhotons.empty())
	{
		auto photon = nearestPhotons.top();
		Vector3 f = pixel.BSDF->Evaluate(pixel.direction, photon.direction, pdf);
		pixel.flux += f * photon.power;
		nearestPhotons.pop();
	}
	// 新的通量
	Vector3 newFlux = pixel.flux * ds;
	L = newFlux / (XM_PI * newRadius2 * (float)m_accumulatePhotons);
	
	Vector3 result = (pixel.Lemit + L) * pixel.pixelWeight;

	XMINT3 RGBValue(
		result.x > 1.0f ? 255 : (int)(result.x * 255.0f),
		result.y > 1.0f ? 255 : (int)(result.y * 255.0f),
		result.z > 1.0f ? 255 : (int)(result.z * 255.0f));

	int rgbIdx = (m_imageSize.y - pixel.pixel.y - 1) * m_imageSize.x + pixel.pixel.x;
	pImageData[rgbIdx].r += RGBValue.x;
	pImageData[rgbIdx].g += RGBValue.y;
	pImageData[rgbIdx].b += RGBValue.z;
}
