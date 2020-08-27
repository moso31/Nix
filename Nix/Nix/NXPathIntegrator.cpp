#include "NXPathIntegrator.h"
#include "NXCubeMap.h"
#include "NXRandom.h"
#include "NXPrimitive.h"

NXPathIntegrator::NXPathIntegrator(const XMINT2& imageSize, int eachPixelSamples, std::string outPath) :
	NXSampleIntegrator(imageSize, eachPixelSamples, outPath)
{
}

Vector3 NXPathIntegrator::Radiance(const Ray& ray, const std::shared_ptr<NXScene>& pScene, int depth)
{
	const int maxDepth = 5;
	Vector3 throughput(1.0f);
	Vector3 L(0.0f);
	Ray nextRay(ray);
	bool bIsSpecular(false);
	while (true)
	{
		NXHit hitInfo;
		bool bIsIntersect = pScene->RayCast(nextRay, hitInfo);

		// 在第一次迭代(depth=0)的时候需要手动计算直接光照。
		// 后续2-n次迭代，光照相关的数据全部由UniformLightOne提供。
		// 同理，在上一次是高亮反射(IsSpecular)的情况下也要手动计算全局光照。
		// 因为由于delta分布的特殊性，这种情况下UniformLightOne提供照明的可能性是0。
		if (depth == 0 || bIsSpecular)
		{
			if (!bIsIntersect)
			{
				auto pCubeMap = pScene->GetCubeMap();
				auto pCubeMapLight = pCubeMap->GetEnvironmentLight();
				if (!pCubeMap || !pCubeMapLight)
					break;

				L += throughput * pCubeMapLight->GetRadiance(Vector3(), Vector3(), -nextRay.direction);
				break;
			}

			std::shared_ptr<NXPBRTangibleLight> pTangibleLight = hitInfo.pPrimitive->GetTangibleLight();
			if (pTangibleLight)
			{
				L += throughput * pTangibleLight->GetRadiance(hitInfo.position, hitInfo.normal, hitInfo.direction);
			}
		}

		if (!bIsIntersect) break;

		hitInfo.GenerateBSDF(true);
		L += throughput * UniformLightOne(nextRay, pScene, hitInfo);
		
		// depth的终指条件建议放在此处。这里可以看作两次迭代计算之间的交界。
		// 后续对throughput的计算，实质上是在为下一层迭代做准备。
		if (depth++ > maxDepth) break;

		float pdf;
		Vector3 nextDirection;
		std::shared_ptr<NXBSDF::SampleEvents> sampleEvent = std::make_shared<NXBSDF::SampleEvents>();
		Vector3 f = hitInfo.BSDF->Sample(hitInfo.direction, nextDirection, pdf, sampleEvent);
		bIsSpecular = *sampleEvent & NXBSDF::DELTA;
		sampleEvent.reset();

		if (f.IsZero()) break;

		throughput *= f * fabsf(hitInfo.shading.normal.Dot(nextDirection)) / pdf;
		nextRay = Ray(hitInfo.position, nextDirection);
		nextRay.position += nextRay.direction * NXRT_EPSILON;

		if (depth > 3)
		{
			// q的最低值设为0.05，以期望20个样本中至少有1个是有效样本。
			// 避免在低频采样的地方选择率过低。
			float q = max(0.05f, throughput.MaxComponent());
			float fRandom = NXRandom::GetInstance()->CreateFloat();
			if (fRandom < 1.0f - q)
				break;
			throughput /= q;
		}
	}

	return L;
}
