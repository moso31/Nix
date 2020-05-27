#include "NXIntegrator.h"
#include "NXBSDF.h"
#include "NXCubeMap.h"
#include "NXPBRLight.h"

NXIntegrator::NXIntegrator()
{
}

NXIntegrator::~NXIntegrator()
{
}

Vector3 NXIntegrator::Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth)
{
	// 寻找射线-场景hit
	NXHit hitInfo;
	bool isIntersect = pScene->RayCast(ray, hitInfo);

	// 如果没有相交，拿背景贴图的radiance并结束计算即可。
	if (!isIntersect)
	{	
		auto pCubeMap = pScene->GetCubeMap();
		if (!pCubeMap) return Vector3(0.0f);

		return pScene->GetCubeMap()->BackgroundColorByDirection(ray.direction);
	}

	// 生成当前hit的bsdf（为其添加各种ReflectionModel）
	hitInfo.ConstructReflectionModel();

	// 计算当前hit的bsdf的Radiance：Lo=Le+Lr
	if (hitInfo.pPrimitive->IsAreaLight())
	{
		// 如果hit物体本身就是光源的话，计算Le
		shared_ptr<NXPBRAreaLight> pAreaLight = hitInfo.pPrimitive->GetAreaLight();
	}

	Vector3 L(0.0);

	// 计算Lr分为两部分，L直接+L间接。
	// 先计算直接光照=Integrate(f(wo, wi) + Ld(wo, wi) * cos(wi), d(wi))
	// f：bsdf所有ReflectionModel的f。
	// Ld: 使用Light类，计算光源在Li方向的Radiance。
	auto pLights = pScene->GetPBRLights();
	for (auto it = pLights.begin(); it != pLights.end(); it++)
	{
		Vector3 incidentDirection;
		float pdf;
		Vector3 Li = (*it)->SampleIncidentRadiance(hitInfo, incidentDirection, pdf);
		Vector3 f = hitInfo.BSDF->f(hitInfo.direction, incidentDirection);

		// 暂时不考虑Visibility Tester
		if (!f.IsZero())
		{
			L += f * Li * incidentDirection.Dot(hitInfo.normal);
		}
	}

	// 然后计算间接光照。
	// Whitted积分光照仅统计来自完美镜面反射和完美镜面折射的间接照明。
	// 所以可以直接分成两个函数，专门统计间接反射和间接折射。
	const static int maxDepth = 5;
	if (depth < maxDepth)
	{
		// 间接反射：仅提取bsdf中具有完美反射的ReflectionModel，并统计其Sample_f。
		L += SpecularReflect(ray, hitInfo, pScene, depth);
		// 间接折射：仅提取bsdf中具有完美折射的ReflectionModel，并统计其Sample_f。
		L += SpecularTransmit(ray, hitInfo, pScene, depth);
	}

	return L;
}

Vector3 NXIntegrator::SpecularReflect(const Ray& ray, const NXHit& hit, const shared_ptr<NXScene>& pScene, int depth)
{
	float pdf;
	Vector3 wo = hit.direction, wi;
	Vector3 f = hit.BSDF->Sample_f(wo, wi, pdf, ReflectionType(REFLECTIONTYPE_REFLECTION | REFLECTIONTYPE_SPECULAR));
	
	if (f.IsZero()) return Vector3(0.0f);

	Ray nextRay = Ray(hit.position, wi);
	nextRay.position += nextRay.direction * 0.001f;
	Vector3 result = f * Radiance(nextRay, pScene, depth + 1) * fabsf(wi.Dot(hit.shading.normal)) / pdf;
	return result;
}

Vector3 NXIntegrator::SpecularTransmit(const Ray& ray, const NXHit& hit, const shared_ptr<NXScene>& pScene, int depth)
{
	float pdf;
	Vector3 wo = hit.direction, wi;
	Vector3 f = hit.BSDF->Sample_f(wo, wi, pdf, ReflectionType(REFLECTIONTYPE_TRANSMISSION | REFLECTIONTYPE_SPECULAR));

	if (f.IsZero()) return Vector3(0.0f);

	Ray nextRay = Ray(hit.position, wi);
	nextRay.position += nextRay.direction * 0.001f;
	Vector3 result = f * Radiance(nextRay, pScene, depth + 1) * fabsf(wi.Dot(hit.shading.normal)) / pdf;
	return result;
}
