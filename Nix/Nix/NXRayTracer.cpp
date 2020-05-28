#include "NXRayTracer.h"
#include <thread>
#include "ImageGenerator.h"
#include "NXCamera.h"
#include "NXScene.h"
#include "NXRandom.h"

NXRayTracer::NXRayTracer() :
	m_progressCount(0)
{
}

void NXRayTracer::MakeImage(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pMainCamera, const shared_ptr<NXIntegrator>& pIntegrator, const NXRenderImageInfo& ImageInfo)
{
	int sampleCount = ImageInfo.ImageSize.x * ImageInfo.ImageSize.y;
	ImageBMPData* pRGB = new ImageBMPData[sampleCount];
	memset(pRGB, 0, sizeof(ImageBMPData) * sampleCount);

	Vector2 imageSizeInv(1.0f / (float)ImageInfo.ImageSize.x, 1.0f / (float)ImageInfo.ImageSize.y);
	Vector2 NDCToViewSpaceFactorInv(1.0f / pMainCamera->GetProjectionMatrix()._11, 1.0f / pMainCamera->GetProjectionMatrix()._22);

	Matrix mxViewToWorld = pMainCamera->GetViewMatrix().Invert();

	XMINT2 tileSize(64, 64);	// 分成多个64*64的tile，对每个tile使用多线程计算以提速。
	XMINT2 tileCount((ImageInfo.ImageSize.x + tileSize.x - 1) / tileSize.x, (ImageInfo.ImageSize.y + tileSize.y - 1) / tileSize.y);		// XY都有多少个tile
	m_threadCount = tileCount.x * tileCount.y;
	m_progressCount = 0;
	thread* threads = new thread[m_threadCount];

	for (int tx = 0; tx < tileCount.x; tx++)
		for (int ty = 0; ty < tileCount.y; ty++)
			threads[tx * tileCount.y + ty] = thread(&NXRayTracer::MakeImageTile, this, pScene, pIntegrator, mxViewToWorld, ImageInfo, imageSizeInv, NDCToViewSpaceFactorInv, tileSize, XMINT2(tx, ty), pRGB);

	for (int tx = 0; tx < tileCount.x; tx++)
		for (int ty = 0; ty < tileCount.y; ty++)
			threads[tx * tileCount.y + ty].join();

	delete[] threads;
	threads = nullptr;

	ImageGenerator::GenerateImageBMP((BYTE*)pRGB, ImageInfo.ImageSize.x, ImageInfo.ImageSize.y, ImageInfo.outPath.c_str());
}

void NXRayTracer::MakeImageTile(const shared_ptr<NXScene>& pScene, const shared_ptr<NXIntegrator>& pIntegrator, const Matrix& mxViewToWorld, const NXRenderImageInfo& ImageInfo, const Vector2& imageSizeInv, const Vector2& NDCToViewSpaceFactorInv, const XMINT2& tileSize, const XMINT2& tileId, ImageBMPData* pRGB)
{
	for (int px = 0; px < tileSize.x; px++)
	{
		for (int py = 0; py < tileSize.y; py++)
		{
			Vector3 result(0.0f);
			int cx = tileId.x * tileSize.x + px;
			int cy = tileId.y * tileSize.y + py;
			if (cx >= ImageInfo.ImageSize.x || cy >= ImageInfo.ImageSize.y)
				continue;

			Vector2 pixel((float)cx, (float)cy);
			// 每个像素pixelSample个样本。
			for (int pixelSample = 0; pixelSample < ImageInfo.EachPixelSamples; pixelSample++)
			{
				// pixel + [0, 1)^2.
				Vector2 sampleCoord = pixel + NXRandom::GetInstance()->CreateVector2();
				Vector2 sampleNDCxyCoord(sampleCoord * imageSizeInv * 2.0f - Vector2(1.0f));

				Vector3 viewDir(sampleNDCxyCoord * NDCToViewSpaceFactorInv, 1.0f);	// 至此屏幕坐标已被转换为view空间的(x, y, 1)射线。
				viewDir.Normalize();

				Ray rayView(Vector3(0.0f), viewDir);
				Ray rayWorld = rayView.Transform(mxViewToWorld);	// 获取该射线的world空间坐标值

				result += pIntegrator->Radiance(rayWorld, pScene, 0);
			}
			result /= (float)ImageInfo.EachPixelSamples;

			XMINT3 resultRGB(
				result.x > 1.0f ? 255 : (int)(result.x * 255.0f),
				result.y > 1.0f ? 255 : (int)(result.y * 255.0f),
				result.z > 1.0f ? 255 : (int)(result.z * 255.0f));

			int rgbIdx = cy * ImageInfo.ImageSize.x + cx;
			pRGB[rgbIdx].r += resultRGB.x;
			pRGB[rgbIdx].g += resultRGB.y;
			pRGB[rgbIdx].b += resultRGB.z;
		}
	}

	m_progressCount++;
	float process = m_progressCount * 100.0f / m_threadCount;
	printf("\r%.2f%% (%d / %d) ", process, m_progressCount, m_threadCount);
}

void NXRayTracer::CenterRayTest(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pMainCamera, const shared_ptr<NXIntegrator>& pIntegrator)
{
	Matrix mxViewToWorld = pMainCamera->GetViewMatrix().Invert();

	Vector3 viewDir(0.0f, 0.0f, 1.0f);
	Ray rayView(Vector3(0.0f), viewDir);
	Ray rayWorld = rayView.Transform(mxViewToWorld);	// 获取该射线的world空间坐标值

	Vector3 result = pIntegrator->Radiance(rayWorld, pScene, 0);

	printf("%f, %f, %f\n", result.x, result.y, result.z);
}

