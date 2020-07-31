#include "NXRayTracer.h"
#include <thread>
#include <iostream>
#include "NXCamera.h"
#include "NXScene.h"
#include "NXRandom.h"

NXRayTracer::NXRayTracer() :
	m_iTaskIter(0),
	m_iRunningThreadCount(0),
	m_iCompletedThreadCount(0),
	m_iTileSize(XMINT2(64, 64)),
	m_bIsRayTracing(false)
{
}

void NXRayTracer::MakeImage(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pMainCamera, const shared_ptr<NXIntegrator>& pIntegrator, const NXRenderImageInfo& ImageInfo)
{
	m_iStartTime = GetTickCount64();

	m_bIsRayTracing = true;
	m_pScene = pScene;
	m_pIntegrator = pIntegrator;
	m_imageInfo = ImageInfo;

	m_fImageSizeInv = Vector2(1.0f / (float)ImageInfo.ImageSize.x, 1.0f / (float)ImageInfo.ImageSize.y);
	m_fNDCToViewSpaceFactorInv = Vector2(1.0f / pMainCamera->GetProjectionMatrix()._11, 1.0f / pMainCamera->GetProjectionMatrix()._22);
	m_mxViewToWorld = pMainCamera->GetViewMatrix().Invert();
	
	// 获取tile总数
	XMINT2 tileCount((ImageInfo.ImageSize.x + m_iTileSize.x - 1) / m_iTileSize.x, (ImageInfo.ImageSize.y + m_iTileSize.y - 1) / m_iTileSize.y);		

	m_renderTileTaskIn.clear();
	m_renderTileTaskOut.clear();
	for (int tx = 0; tx < tileCount.x; tx++)
		for (int ty = 0; ty < tileCount.y; ty++)
		{
			NXRenderTileTaskInfo task;
			task.tileId = XMINT2(tx, ty);
			m_renderTileTaskIn.push_back(task);
		}
	m_renderTileTaskOut.resize(m_renderTileTaskIn.size());

	m_iTaskIter = 0;
	m_iRunningThreadCount = 0;
	m_iCompletedThreadCount = 0;
}

void NXRayTracer::MakeImageTile(const int taskIter)
{
	const NXRenderTileTaskInfo& taskIn = m_renderTileTaskIn[taskIter];
	NXRenderTileData& taskOut = m_renderTileTaskOut[taskIter];

	taskOut.tileId = taskIn.tileId;
	taskOut.pData.resize(m_iTileSize.x * m_iTileSize.y);
	for (int i = 0; i < m_iTileSize.x; i++)
	{
		for (int j = 0; j < m_iTileSize.y; j++)
		{
			Vector3 result(0.0f);
			int pixelX = taskIn.tileId.x * m_iTileSize.x + i;
			int pixelY = taskIn.tileId.y * m_iTileSize.y + j;
			if (pixelX >= m_imageInfo.ImageSize.x || pixelY >= m_imageInfo.ImageSize.y)
				continue;

			Vector2 pixel((float)pixelX, (float)pixelY);
			// 每个像素pixelSample个样本。
			for (int pixelSample = 0; pixelSample < m_imageInfo.EachPixelSamples; pixelSample++)
			{
				// pixel + [0, 1)^2.
				Vector2 sampleCoord = pixel + NXRandom::GetInstance()->CreateVector2();
				Vector2 sampleNDCxyCoord(sampleCoord * m_fImageSizeInv * 2.0f - Vector2(1.0f));

				Vector3 viewDir(sampleNDCxyCoord * m_fNDCToViewSpaceFactorInv, 1.0f);	// 至此屏幕坐标已被转换为view空间的(x, y, 1)射线。
				viewDir.Normalize();

				Ray rayView(Vector3(0.0f), viewDir);
				Ray rayWorld = rayView.Transform(m_mxViewToWorld);	// 获取该射线的world空间坐标值

				result += m_pIntegrator->Radiance(rayWorld, m_pScene, 0);
			}
			result /= (float)m_imageInfo.EachPixelSamples;

			XMINT3 resultRGB(
				result.x > 1.0f ? 255 : (int)(result.x * 255.0f),
				result.y > 1.0f ? 255 : (int)(result.y * 255.0f),
				result.z > 1.0f ? 255 : (int)(result.z * 255.0f));

			int index = j * m_iTileSize.x + i;
			taskOut.pData[index].r += resultRGB.x;
			taskOut.pData[index].g += resultRGB.y;
			taskOut.pData[index].b += resultRGB.z;
		}
	}

	m_iCompletedThreadCount++;
	m_iRunningThreadCount--;

	int count = (int)m_iCompletedThreadCount;
	int threadCount = (int)m_renderTileTaskIn.size();
	float process = count * 100.0f / threadCount;
	printf("\r%.2f%% (%d / %d) ", process, count, threadCount);
}

void NXRayTracer::CenterRayTest(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pMainCamera, const shared_ptr<NXIntegrator>& pIntegrator, const int testTime)
{
	Matrix mxViewToWorld = pMainCamera->GetViewMatrix().Invert();

	Vector3 viewDir(0.0f, 0.0f, 1.0f);
	Ray rayView(Vector3(0.0f), viewDir);
	Ray rayWorld = rayView.Transform(mxViewToWorld);	// 获取该射线的world空间坐标值

	Vector3 result(0.0f);
	for (int i = 0; i < testTime; i++)
		result += pIntegrator->Radiance(rayWorld, pScene, 0);
	result /= (float)testTime;

	printf("%f, %f, %f\n", result.x, result.y, result.z);
}

void NXRayTracer::GenerateImage()
{
	int pixelCount = m_imageInfo.ImageSize.x * m_imageInfo.ImageSize.y;
	ImageBMPData* pRGB = new ImageBMPData[pixelCount];
	memset(pRGB, 0, sizeof(ImageBMPData) * pixelCount);

	for (int k = 0; k < m_renderTileTaskOut.size(); k++)
	{
		NXRenderTileData data = m_renderTileTaskOut[k];
		for (int i = 0; i < m_iTileSize.x; i++)
		{
			for (int j = 0; j < m_iTileSize.y; j++)
			{
				int pixelX = data.tileId.x * m_iTileSize.x + i;
				int pixelY = data.tileId.y * m_iTileSize.y + j;
				if (pixelX >= m_imageInfo.ImageSize.x || pixelY >= m_imageInfo.ImageSize.y)
					continue;

				int rgbIdx = pixelY * m_imageInfo.ImageSize.x + pixelX;
				int index = j * m_iTileSize.x + i;
				pRGB[rgbIdx] = data.pData[index];
			}
		}
	}

	ImageGenerator::GenerateImageBMP((BYTE*)pRGB, m_imageInfo.ImageSize.x, m_imageInfo.ImageSize.y, m_imageInfo.outPath.c_str());
	delete pRGB;
}

void NXRayTracer::Update()
{
	const static int MAX_THREAD_COUNT = 100;
	if (m_bIsRayTracing)
	{
		while (m_iTaskIter < m_renderTileTaskIn.size() && m_iRunningThreadCount < MAX_THREAD_COUNT)
		{
			// printf("Running Threads: %d\n", (int)m_iRunningThreadCount + 1);
			m_iRunningThreadCount++;
			thread task = thread(&NXRayTracer::MakeImageTile, this, m_iTaskIter++);
			task.detach();
		}

		if ((int)m_iCompletedThreadCount == m_renderTileTaskIn.size())
		{
			GenerateImage();
			m_bIsRayTracing = false;

			m_iEndTime = GetTickCount64();
			printf("Render done. time：%.3f s\n", (float)(m_iEndTime - m_iStartTime) / 1000.0f);
		}
	}
}

