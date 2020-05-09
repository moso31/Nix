#include "NXRayTracer.h"
#include "NXCamera.h"
#include "NXScene.h"
#include "NXIntersection.h"

NXRayTracer::NXRayTracer()
{
	int seed = 0;	// can be changed as time.
	m_rng = default_random_engine(seed);
}

void NXRayTracer::MakeImage(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pMainCamera, const shared_ptr<NXIntegrator>& pIntegrator, const NXRenderImageInfo& ImageInfo)
{
	Vector2 imageSizeInv(1.0f / (float)ImageInfo.ImageSize.x, 1.0f / (float)ImageInfo.ImageSize.y);

	Vector2 NDCToViewSpaceFactorInv(1.0f / pMainCamera->GetProjectionMatrix()._11, 1.0f / pMainCamera->GetProjectionMatrix()._22);

	Matrix mxViewToWorld = pMainCamera->GetViewMatrix().Invert();
	// ����ImageInfo�����Ϊÿ�������������ߡ�
	for (int px = 0; px < ImageInfo.ImageSize.x; px++)
	{
		for (int py = 0; py < ImageInfo.ImageSize.y; py++)
		{
			Vector2 pixel(px, py);
			// ÿ������pixelSample��������
			for (int pixelSample = 0; pixelSample < ImageInfo.EachPixelSamples; pixelSample++)
			{
				uniform_real_distribution<float> fRandom(0.0f, 1.0f);

				// pixel + [0, 1)^2.
				Vector2 sampleCoord = pixel + Vector2(fRandom(m_rng), fRandom(m_rng));
				Vector2 sampleNDCxyCoord(sampleCoord * imageSizeInv * 2.0f - Vector2(1.0f));

				Vector3 viewDir(sampleNDCxyCoord * NDCToViewSpaceFactorInv, 1.0f);	// ������Ļ�����ѱ�ת��Ϊview�ռ��(x, y, 1)���ߡ�
				
				Ray rayView(Vector3(0.0f), viewDir);
				Ray rayWorld = rayView.Transform(mxViewToWorld);	// ��ȡ�����ߵ�world�ռ�����ֵ

				RayCast(pScene, rayWorld, pIntegrator);
			}
		}
	}
}

void NXRayTracer::RayCast(const shared_ptr<NXScene>& pScene, const Ray& rayWorld, const shared_ptr<NXIntegrator>& pIntegrator)
{
	NXIntersectionInfo isect;
	NXIntersection::GetInstance()->RayIntersect(pScene, rayWorld, isect);
}

void NXRayTracer::Release()
{
}
