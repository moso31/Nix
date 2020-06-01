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
	// Ѱ������-����hit
	NXHit hitInfo;
	bool isIntersect = pScene->RayCast(ray, hitInfo);

	// ���û���ཻ���ñ�����ͼ��radiance���������㼴�ɡ�
	if (!isIntersect)
	{	
		auto pCubeMap = pScene->GetCubeMap();
		if (!pCubeMap || !pCubeMap->GetEnvironmentLight()) 
			return Vector3(0.0f);

		Vector3 ignore;
		return pCubeMap->GetEnvironmentLight()->GetRadiance(ignore, ignore, -ray.direction);
	}

	// ���ɵ�ǰhit��bsdf��Ϊ����Ӹ���ReflectionModel��
	hitInfo.ConstructReflectionModel();

	// Ȼ����㵱ǰhit��Radiance��Lo=Le+Lr
	Vector3 L(0.0);

	// ���hit���屾����ǹ�Դ�Ļ�������Le
	shared_ptr<NXTangibleLight> pTangebleLight = hitInfo.pPrimitive->GetTangibleLight();
	if (pTangebleLight)
	{
		L += pTangebleLight->GetRadiance(hitInfo.position, hitInfo.normal, hitInfo.direction);
	}

	// ����Lr��Ϊ�����֣�Lֱ��+L��ӡ�
	// �ȼ���ֱ�ӹ���=Integrate(f(wo, wi) + Ld(wo, wi) * cos(wi), d(wi))
	// f��bsdf����ReflectionModel��f��
	// Ld: ʹ��Light�࣬�����Դ��Li�����Radiance��
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

	// Ȼ������ӹ��ա�
	// Whitted���ֹ��ս�ͳ�������������淴���������������ļ��������
	// ���Կ���ֱ�ӷֳ�����������ר��ͳ�Ƽ�ӷ���ͼ�����䡣
	const static int maxDepth = 5;
	if (depth < maxDepth)
	{
		// ��ӷ��䣺����ȡbsdf�о������������ReflectionModel����ͳ����Sample_f��
		L += SpecularReflect(ray, hitInfo, pScene, depth);
		// ������䣺����ȡbsdf�о������������ReflectionModel����ͳ����Sample_f��
		L += SpecularTransmit(ray, hitInfo, pScene, depth);
	}

	return L;
}

Vector3 NXIntegrator::DirectRadiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth)
{
	// Ѱ������-����hit
	NXHit hitInfo;
	bool isIntersect = pScene->RayCast(ray, hitInfo);

	// ���û���ཻ���ñ�����ͼ��radiance���������㼴�ɡ�
	if (!isIntersect)
	{
		auto pCubeMap = pScene->GetCubeMap();
		if (!pCubeMap || !pCubeMap->GetEnvironmentLight())
			return Vector3(0.0f);

		Vector3 ignore;
		return pCubeMap->GetEnvironmentLight()->GetRadiance(ignore, ignore, -ray.direction);
	}

	// ���ɵ�ǰhit��bsdf��Ϊ����Ӹ���ReflectionModel��
	hitInfo.ConstructReflectionModel();

	// Ȼ����㵱ǰhit��Radiance��Lo=Le+Lr
	Vector3 L(0.0f);

	// ���hit���屾����ǹ�Դ�Ļ�������Le
	shared_ptr<NXTangibleLight> pTangibleLight = hitInfo.pPrimitive->GetTangibleLight();
	if (pTangibleLight)
	{
		L += pTangibleLight->GetRadiance(hitInfo.position, hitInfo.normal, hitInfo.direction);
	}

	// All : ͳ�����еĹ�Դ����ƽ��ֵ��
	auto pLights = pScene->GetPBRLights();
	for (auto it = pLights.begin(); it != pLights.end(); it++)
	{
		L += DirectEstimate(ray, pScene, *it, hitInfo);
	}
	L /= (float)pLights.size();

	// Ȼ������ӹ��ա�
	// Whitted���ֹ��ս�ͳ�������������淴���������������ļ��������
	// ���Կ���ֱ�ӷֳ�����������ר��ͳ�Ƽ�ӷ���ͼ�����䡣
	const static int maxDepth = 5;
	if (depth < maxDepth)
	{
		// ��ӷ��䣺����ȡbsdf�о������������ReflectionModel����ͳ����Sample_f��
		L += SpecularReflect(ray, hitInfo, pScene, depth);
		// ������䣺����ȡbsdf�о������������ReflectionModel����ͳ����Sample_f��
		L += SpecularTransmit(ray, hitInfo, pScene, depth);
	}

	return L;
}

Vector3 NXIntegrator::DirectEstimate(const Ray& ray, const shared_ptr<NXScene>& pScene, const shared_ptr<NXPBRLight>& pLight, const NXHit& hitInfo)
{
	/*
	�Ե��β�������������
	�����DeltaLight��
		����light�ṩ�ķ�����м��㼴�ɵõ�ʵ�ʾ�ȷֵ��
	�������DeltaLight��
		�޷��õ���ȷֵ����ʹ�����ַ���������һ�Σ�
			1.����light����һ�Σ�
			2.����BSDF����һ�Ρ�
		֮�����β����Ľ����ϣ��õ�һ����Ϊ׼ȷ�Ĺ���ֵ��
	*/

	Vector3 L(0.0f);

	// ͳ��pdfʱ����ͳ�ƴ���SPECULAR���͵ķ���ģ�͡�
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

		// ���ڹ�Դ����һ��
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

		// ����BSDF����һ��
		f = hitInfo.BSDF->Sample_f(hitInfo.direction, incidentDirection, pdfBSDF, refType);
		if (!f.IsZero())
		{
			auto pAreaLight = dynamic_pointer_cast<NXPBRAreaLight>(pLight);
			
			// ����BSDF�����ķ���Ѱ�Ҵ˴β����Ƿ���й�Դ��
			// ���й�Դ��ʹ�øù�Դ���Է���������ΪBSDF������Li��
			// δ���й�Դ��ʹ�û�����ͼ���Է�����ΪLi��û�л�����ͼ�򷵻�0.��
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
			// ����Ȩ�ء��Ի���BSDF�Ĳ�����BSDFΪ��Ҫ��Ȩ��Light��Ρ�
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
