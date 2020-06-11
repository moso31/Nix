#include "NXPathIntegrator.h"
#include "NXCubeMap.h"
#include "NXRandom.h"

Vector3 NXPathIntegrator::Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth)
{
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
		shared_ptr<ReflectionType> outReflectType = make_shared<ReflectionType>();
		Vector3 f = hitInfo.BSDF->Sample_f(hitInfo.direction, nextDirection, pdf, REFLECTIONTYPE_ALL, outReflectType);
		bIsSpecular = (*outReflectType & REFLECTIONTYPE_SPECULAR);
		outReflectType.reset();

		if (f.IsZero()) break;

		throughput *= f * fabsf(hitInfo.normal.Dot(nextDirection)) / pdf;
		nextRay = Ray(hitInfo.position, nextDirection);
		nextRay.position += nextRay.direction * NXRT_EPSILON;
	}

	return L;
}
