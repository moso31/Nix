#include "NXIntegrator.h"
#include "NXBSDF.h"
#include "NXCubeMap.h"
#include "NXPBRLight.h"
#include "SamplerMath.h"

using namespace SamplerMath;

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
		if (!pCubeMap || !pCubeMap->GetEnvironmentLight()) 
			return Vector3(0.0f);

		return pScene->GetCubeMap()->GetEnvironmentLight()->GetRadiance(ray.direction); 
	}

	// 生成当前hit的bsdf（为其添加各种ReflectionModel）
	hitInfo.ConstructReflectionModel();

	// 然后计算当前hit的Radiance：Lo=Le+Lr
	Vector3 L(0.0);

	// 如果hit物体本身就是光源的话，计算Le
	shared_ptr<NXPBRAreaLight> pAreaLight = hitInfo.pPrimitive->GetAreaLight();
	if (pAreaLight)
	{
		L += pAreaLight->GetRadiance(hitInfo.position, hitInfo.normal, hitInfo.direction);
	}

	// 计算Lr分为两部分，L直接+L间接。
	// 先计算直接光照=Integrate(f(wo, wi) + Ld(wo, wi) * cos(wi), d(wi))
	// f：bsdf所有ReflectionModel的f。
	// Ld: 使用Light类，计算光源在Li方向的Radiance。
	auto pLights = pScene->GetPBRLights();
	for (auto it = pLights.begin(); it != pLights.end(); it++)
	{
		Vector3 incidentDirection;
		float pdf = 0.0f;
		Vector3 Li = (*it)->SampleIncidentRadiance(hitInfo, incidentDirection, pdf);
		if (Li.IsZero() || pdf == 0.0f)
			continue;

		Vector3 f = hitInfo.BSDF->f(hitInfo.direction, incidentDirection);
		if (!f.IsZero())
		{
			L += f * Li * incidentDirection.Dot(hitInfo.normal) / pdf;
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

Vector3 NXIntegrator::DirectRadiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth)
{
	// 寻找射线-场景hit
	NXHit hitInfo;
	bool isIntersect = pScene->RayCast(ray, hitInfo);

	// 如果没有相交，拿背景贴图的radiance并结束计算即可。
	if (!isIntersect)
	{
		auto pCubeMap = pScene->GetCubeMap();
		if (!pCubeMap || !pCubeMap->GetEnvironmentLight())
			return Vector3(0.0f);

		return pScene->GetCubeMap()->GetEnvironmentLight()->GetRadiance(ray.direction);
	}

	// 生成当前hit的bsdf（为其添加各种ReflectionModel）
	hitInfo.ConstructReflectionModel();

	// 然后计算当前hit的Radiance：Lo=Le+Lr
	Vector3 L(0.0);

	// 如果hit物体本身就是光源的话，计算Le
	shared_ptr<NXPBRAreaLight> pAreaLight = hitInfo.pPrimitive->GetAreaLight();
	if (pAreaLight)
	{
		L += pAreaLight->GetRadiance(hitInfo.position, hitInfo.normal, hitInfo.direction);
	}

	Vector3 L(0.0f);
	// All : 统计所有的光源并求平均值。
	auto pLights = pScene->GetPBRLights();
	for (auto it = pLights.begin(); it != pLights.end(); it++)
	{
		L += DirectEstimate(ray, pScene, *it, hitInfo);
	}
	L /= pLights.size();

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
}

Vector3 NXIntegrator::DirectEstimate(const Ray& ray, const shared_ptr<NXScene>& pScene, const shared_ptr<NXPBRLight>& pLight, const NXHit& hitInfo)
{
	Vector3 L(0.0f);

	// 统计pdf时，不统计带有SPECULAR类型的反射模型。
	ReflectionType refType = (ReflectionType)(REFLECTIONTYPE_ALL & ~REFLECTIONTYPE_SPECULAR);
	bool bIsDeltaLight = pLight->IsDeltaLight;

	// 对单次采样进行评估。
	// 使用两种方法采样：先基于light进行一次，再基于BSDF进行一次。最终将这两种采样的结果结合。

	// 基于light的采样
	Vector3 incidentDirection;
	float pdfLight = 0.0f;
	Vector3 Li = pLight->SampleIncidentRadiance(hitInfo, incidentDirection, pdfLight);
	if (!Li.IsZero() && pdfLight > 0.0f)
	{
		Vector3 f = hitInfo.BSDF->f(hitInfo.direction, incidentDirection) * cosFix;

		if (!f.IsZero())
		{
			if (bIsDeltaLight)
			{
				L += f * Li; // / pdfLight;	 // DeltaLight的pdf实际上=1。
			}
			else
			{
				// 尽管计算f()本身并不需要BSDF的pdf，但因为我们的做法需要提供BSDF和Light两种pdf结合，所以还是得计算BSDF的pdf值。
				float pdfBSDF = hitInfo.BSDF->Pdf(hitInfo.direction, incidentDirection, refType);
				// 计算权重。对基于Light的采样，Light为主要加权，BSDF其次。
				float weight = PowerHeuristicWeightPdf(1, pdfLight, 1, pdfBSDF);
				L += f * Li * weight / pdfLight;
			}
		}
	}

	if (!bIsDeltaLight)
	{

	}
}

Vector3 NXIntegrator::SpecularReflect(const Ray& ray, const NXHit& hit, const shared_ptr<NXScene>& pScene, int depth)
{
	float pdf;
	Vector3 wo = hit.direction, wi;
	Vector3 f = hit.BSDF->Sample_f(wo, wi, pdf, ReflectionType(REFLECTIONTYPE_REFLECTION | REFLECTIONTYPE_SPECULAR));
	
	if (f.IsZero()) return Vector3(0.0f);

	Ray nextRay = Ray(hit.position, wi);
	nextRay.position += nextRay.direction * NXRT_EPSILON;
	//Vector3 result = f * Radiance(nextRay, pScene, depth + 1) * fabsf(wi.Dot(hit.shading.normal)) / pdf;
	Vector3 result = f * DirectRadiance(nextRay, pScene, depth + 1) * fabsf(wi.Dot(hit.shading.normal)) / pdf;
	return result;
}

Vector3 NXIntegrator::SpecularTransmit(const Ray& ray, const NXHit& hit, const shared_ptr<NXScene>& pScene, int depth)
{
	float pdf;
	Vector3 wo = hit.direction, wi;
	Vector3 f = hit.BSDF->Sample_f(wo, wi, pdf, ReflectionType(REFLECTIONTYPE_TRANSMISSION | REFLECTIONTYPE_SPECULAR));

	if (f.IsZero()) return Vector3(0.0f);

	Ray nextRay = Ray(hit.position, wi);
	nextRay.position += nextRay.direction * NXRT_EPSILON;
	//Vector3 result = f * Radiance(nextRay, pScene, depth + 1) * fabsf(wi.Dot(hit.shading.normal)) / pdf;
	Vector3 result = f * DirectRadiance(nextRay, pScene, depth + 1) * fabsf(wi.Dot(hit.shading.normal)) / pdf;
	return result;
}
