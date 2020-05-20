#include "NXRayTracer.h"
#include "ImageGenerator.h"
#include "NXCamera.h"
#include "NXScene.h"
#include "NXRandom.h"

NXRayTracer::NXRayTracer()
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
	// 根据ImageInfo的情况为每个像素生成射线。
	for (int px = 0; px < ImageInfo.ImageSize.x; px++)
	{
		for (int py = 0; py < ImageInfo.ImageSize.y; py++)
		{
			Vector3 result(0.0f);
			Vector2 pixel((float)px, (float)py);
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

			int rgbIdx = (ImageInfo.ImageSize.y - py - 1) * ImageInfo.ImageSize.x + px;
			pRGB[rgbIdx].r += resultRGB.x;
			pRGB[rgbIdx].g += resultRGB.y;
			pRGB[rgbIdx].b += resultRGB.z;
		}
	}

	ImageGenerator::GenerateImageBMP((BYTE*)pRGB, ImageInfo.ImageSize.x, ImageInfo.ImageSize.y, "D:\\nix.bmp");
}

void NXRayTracer::CenterRayTest(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pMainCamera, const shared_ptr<NXIntegrator>& pIntegrator)
{
	Matrix mxViewToWorld = pMainCamera->GetViewMatrix().Invert();

	Vector3 viewDir(0.0f, 0.0f, 1.0f);
	Ray rayView(Vector3(0.0f), viewDir);
	Ray rayWorld = rayView.Transform(mxViewToWorld);	// 获取该射线的world空间坐标值

	Vector3 result = pIntegrator->Radiance(rayWorld, pScene, 0);

	printf("%.f, %.f, %.f\n", result.x, result.y, result.z);
}

void NXRayTracer::Release()
{
}
