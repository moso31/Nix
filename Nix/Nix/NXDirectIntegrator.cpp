#include "NXDirectIntegrator.h"
#include "NXPrimitive.h"
#include "NXScene.h"
#include "NXCubeMap.h"

Vector3 NXDirectIntegrator::Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth)
{
	// Ѱ������-����hit
	NXHit hitInfo;
	Vector3 L(0.0f);

	bool isIntersect = pScene->RayCast(ray, hitInfo);
	if (!isIntersect)
	{
		auto pCubeMap = pScene->GetCubeMap();
		if (!pCubeMap || !pCubeMap->GetEnvironmentLight())
			return Vector3(0.0f);

		Vector3 ignore;
		return pCubeMap->GetEnvironmentLight()->GetRadiance(ignore, ignore, ray.direction);
	}
	shared_ptr<NXPBRTangibleLight> pTangibleLight = hitInfo.pPrimitive->GetTangibleLight();
	if (pTangibleLight)
	{
		L += pTangibleLight->GetRadiance(hitInfo.position, hitInfo.normal, hitInfo.direction);
	}

	// ���ɵ�ǰhit��bsdf��Ϊ����Ӹ���ReflectionModel��
	hitInfo.GenerateBSDF(true);

	bool bIsUniformAll = false;
	if (bIsUniformAll)
		L += UniformLightAll(ray, pScene, hitInfo);
	else
		L += UniformLightOne(ray, pScene, hitInfo);

	const static int maxDepth = 5;
	if (depth < maxDepth)
	{
		// ֱ�������ĸ���������ȫ����delta�ֲ������Բ���Ҫpdf
		float pdf;
		Vector3 wo = hitInfo.direction, wi;
		Vector3 f = hitInfo.BSDF->SampleReflectWorld(wo, wi, pdf);
		if (!f.IsZero())
		{
			Ray nextRay = Ray(hitInfo.position, wi);
			nextRay.position += nextRay.direction * NXRT_EPSILON;
			L += f * Radiance(nextRay, pScene, depth + 1) * fabsf(wi.Dot(hitInfo.shading.normal));
		}
		f = hitInfo.BSDF->SampleRefractWorld(wo, wi, pdf);
		if (!f.IsZero())
		{
			Ray nextRay = Ray(hitInfo.position, wi);
			nextRay.position += nextRay.direction * NXRT_EPSILON;
			L += f * Radiance(nextRay, pScene, depth + 1) * fabsf(wi.Dot(hitInfo.shading.normal));
		}
	}

	return L;
}
