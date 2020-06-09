#include "NXPathIntegrator.h"
#include "NXCubeMap.h"
#include "NXRandom.h"

Vector3 NXPathIntegrator::Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth)
{
	Vector3 throughput(1.0f);
	Vector3 L(0.0f);
	Ray nextRay(ray);
	while (true)
	{
		NXHit hitInfo;
		bool bIsIntersect = pScene->RayCast(nextRay, hitInfo);

		// 只有在第一次迭代的时候考虑直接光照。
		// 后续2-n次迭代，光照相关的数据全部由UniformLightOne提供。
		if (depth == 0)
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

			shared_ptr<NXTangibleLight> pTangibleLight = hitInfo.pPrimitive->GetTangibleLight();
			if (pTangibleLight)
			{
				L += throughput * pTangibleLight->GetRadiance(hitInfo.position, hitInfo.normal, hitInfo.direction);
			}
		}

		if (!bIsIntersect) break;

		hitInfo.ConstructReflectionModel();
		L += throughput * UniformLightOne(nextRay, pScene, hitInfo);
		
		// depth的终指条件建议放在此处。这里可以看作两次迭代计算之间的交界。
		// 后续对throughput的计算，实质上是在为下一层迭代做准备。
		if (depth++ > 5) break;	

		float pdf;
		Vector3 nextDirection;
		Vector3 f = hitInfo.BSDF->Sample_f(hitInfo.direction, nextDirection, pdf);

		if (f.IsZero()) break;

		throughput *= f * hitInfo.normal.Dot(nextDirection) / pdf;
		nextRay = Ray(hitInfo.position, nextDirection);
		nextRay.position += nextRay.direction * NXRT_EPSILON;
	}

	return L;
}
