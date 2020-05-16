#include "NXRayTracer.h"
#include "NXCamera.h"
#include "NXScene.h"
#include "NXIntersection.h"
#include "NXRandom.h"

NXRayTracer::NXRayTracer()
{
}

void NXRayTracer::MakeImage(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pMainCamera, const shared_ptr<NXIntegrator>& pIntegrator, const NXRenderImageInfo& ImageInfo)
{
	Vector2 imageSizeInv(1.0f / (float)ImageInfo.ImageSize.x, 1.0f / (float)ImageInfo.ImageSize.y);

	Vector2 NDCToViewSpaceFactorInv(1.0f / pMainCamera->GetProjectionMatrix()._11, 1.0f / pMainCamera->GetProjectionMatrix()._22);

	Matrix mxViewToWorld = pMainCamera->GetViewMatrix().Invert();
	// 根据ImageInfo的情况为每个像素生成射线。
	for (int px = 0; px < ImageInfo.ImageSize.x; px++)
	{
		for (int py = 0; py < ImageInfo.ImageSize.y; py++)
		{
			Vector2 pixel(px, py);
			// 每个像素pixelSample个样本。
			for (int pixelSample = 0; pixelSample < ImageInfo.EachPixelSamples; pixelSample++)
			{
				// pixel + [0, 1)^2.
				Vector2 sampleCoord = pixel + NXRandom::GetInstance()->CreateVector2();
				Vector2 sampleNDCxyCoord(sampleCoord * imageSizeInv * 2.0f - Vector2(1.0f));

				Vector3 viewDir(sampleNDCxyCoord * NDCToViewSpaceFactorInv, 1.0f);	// 至此屏幕坐标已被转换为view空间的(x, y, 1)射线。
				
				Ray rayView(Vector3(0.0f), viewDir);
				Ray rayWorld = rayView.Transform(mxViewToWorld);	// 获取该射线的world空间坐标值

				RayCast(pScene, rayWorld, pIntegrator);
			}
		}
	}
}

void NXRayTracer::RayCast(const shared_ptr<NXScene>& pScene, const Ray& rayWorld, const shared_ptr<NXIntegrator>& pIntegrator)
{
	NXHitInfo isect;
	NXHit::GetInstance()->RayCast(pScene, rayWorld, isect);
}

void NXRayTracer::Release()
{
}
