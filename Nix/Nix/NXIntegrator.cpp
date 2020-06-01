#include "NXIntegrator.h"
#include "NXBSDF.h"
#include "NXCubeMap.h"
#include "NXPBRLight.h"
#include "SamplerMath.h"

using namespace SimpleMath;
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

		Vector3 ignore;
		return pCubeMap->GetEnvironmentLight()->GetRadiance(ignore, ignore, -ray.direction);
	}

	// 生成当前hit的bsdf（为其添加各种ReflectionModel）
	hitInfo.ConstructReflectionModel();

	// 然后计算当前hit的Radiance：Lo=Le+Lr
	Vector3 L(0.0);

	// 如果hit物体本身就是光源的话，计算Le
	shared_ptr<NXTangibleLight> pTangebleLight = hitInfo.pPrimitive->GetTangibleLight();
	if (pTangebleLight)
	{
		L += pTangebleLight->GetRadiance(hitInfo.position, hitInfo.normal, hitInfo.direction);
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

		Vector3 ignore;
		return pCubeMap->GetEnvironmentLight()->GetRadiance(ignore, ignore, -ray.direction);
	}

	// 生成当前hit的bsdf（为其添加各种ReflectionModel）
	hitInfo.ConstructReflectionModel();

	// 然后计算当前hit的Radiance：Lo=Le+Lr
	Vector3 L(0.0f);

	// 如果hit物体本身就是光源的话，计算Le
	shared_ptr<NXTangibleLight> pTangibleLight = hitInfo.pPrimitive->GetTangibleLight();
	if (pTangibleLight)
	{
		L += pTangibleLight->GetRadiance(hitInfo.position, hitInfo.normal, hitInfo.direction);
	}

	// All : 统计所有的光源并求平均值。
	auto pLights = pScene->GetPBRLights();
	for (auto it = pLights.begin(); it != pLights.end(); it++)
	{
		L += DirectEstimate(ray, pScene, *it, hitInfo);
	}
	L /= (float)pLights.size();

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

Vector3 NXIntegrator::DirectEstimate(const Ray& ray, const shared_ptr<NXScene>& pScene, const shared_ptr<NXPBRLight>& pLight, const NXHit& hitInfo)
{
	/*
	对单次采样进行评估。
	如果是DeltaLight：
		仅对light提供的方向进行计算即可得到实际精确值。
	如果不是DeltaLight：
		无法得到精确值，就使用两种方法各采样一次：
			1.基于light采样一次，
			2.基于BSDF采样一次。
		之后将两次采样的结果结合，得到一个较为准确的估算值。
	*/

	Vector3 L(0.0f);

	// 统计pdf时，不统计带有SPECULAR类型的反射模型。
	ReflectionType refType = (ReflectionType)(REFLECTIONTYPE_ALL & ~REFLECTIONTYPE_SPECULAR);
	bool bIsDeltaLight = pLight->IsDeltaLight();

	if (bIsDeltaLight)
	{
		Vector3 incidentDirection;
		float pdfLight = 0.0f;
		Vector3 Li = pLight->SampleIncidentRadiance(hitInfo, incidentDirection, pdfLight);
		if (!Li.IsZero())
		{
			Vector3 f = hitInfo.BSDF->f(hitInfo.direction, incidentDirection);
			if (!f.IsZero())
			{
				L += f * Li * incidentDirection.Dot(hitInfo.normal);
			}
		}
	}
	else
	{
		Vector3 f(0.0f);
		Vector3 incidentDirection;
		float pdfLight = 0.0f, pdfBSDF = 0.0f, pdfWeight = 0.0f;

		// 基于光源采样一次
		Vector3 Li = pLight->SampleIncidentRadiance(hitInfo, incidentDirection, pdfLight);
		if (!Li.IsZero() && pdfLight > 0.0f)
		{
			f = hitInfo.BSDF->f(hitInfo.direction, incidentDirection);
			if (!f.IsZero())
			{
				pdfBSDF = hitInfo.BSDF->Pdf(hitInfo.direction, incidentDirection, refType);
				pdfWeight = PowerHeuristicWeightPdf(1, pdfLight, 1, pdfBSDF);
				L += f * Li * incidentDirection.Dot(hitInfo.normal) * pdfWeight / pdfLight;
			}
		}

		// 基于BSDF采样一次
		f = hitInfo.BSDF->Sample_f(hitInfo.direction, incidentDirection, pdfBSDF, refType);
		if (!f.IsZero())
		{
			auto pAreaLight = dynamic_pointer_cast<NXPBRAreaLight>(pLight);
			
			// 基于BSDF采样的方向寻找此次采样是否击中光源。
			// 击中光源：使用该光源的自发光数据作为BSDF样本的Li。
			// 未击中光源：使用环境贴图的自发光作为Li（没有环境贴图则返回0.）
			Ray ray(hitInfo.position, incidentDirection);
			ray.position += incidentDirection * NXRT_EPSILON;
			NXHit hitLightInfo;
			pScene->RayCast(ray, hitLightInfo);
			shared_ptr<NXPBRAreaLight> pHitAreaLight;
			Vector3 Li(0.0f);
			if (hitLightInfo.pPrimitive)
			{
				pHitAreaLight = hitLightInfo.pPrimitive->GetTangibleLight();
			}
			else if (pScene->GetCubeMap())
			{
				pHitAreaLight = pScene->GetCubeMap()->GetEnvironmentLight();
			}

			if (pHitAreaLight) 
				Li = pHitAreaLight->GetRadiance(hitLightInfo.position, hitLightInfo.normal, hitLightInfo.direction);

			pdfLight = pAreaLight->GetPdf(hitLightInfo, hitLightInfo.direction);
			// 计算权重。对基于BSDF的采样，BSDF为主要加权，Light其次。
			pdfWeight = PowerHeuristicWeightPdf(1, pdfBSDF, 1, pdfLight);
			L += f * Li * incidentDirection.Dot(hitInfo.normal) * pdfWeight / pdfBSDF;
		}
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
