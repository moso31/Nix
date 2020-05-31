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
	// Ѱ������-����hit
	NXHit hitInfo;
	bool isIntersect = pScene->RayCast(ray, hitInfo);

	// ���û���ཻ���ñ�����ͼ��radiance���������㼴�ɡ�
	if (!isIntersect)
	{	
		auto pCubeMap = pScene->GetCubeMap();
		if (!pCubeMap || !pCubeMap->GetEnvironmentLight()) 
			return Vector3(0.0f);

		return pScene->GetCubeMap()->GetEnvironmentLight()->GetRadiance(ray.direction); 
	}

	// ���ɵ�ǰhit��bsdf��Ϊ����Ӹ���ReflectionModel��
	hitInfo.ConstructReflectionModel();

	// Ȼ����㵱ǰhit��Radiance��Lo=Le+Lr
	Vector3 L(0.0);

	// ���hit���屾����ǹ�Դ�Ļ�������Le
	shared_ptr<NXPBRAreaLight> pAreaLight = hitInfo.pPrimitive->GetAreaLight();
	if (pAreaLight)
	{
		L += pAreaLight->GetRadiance(hitInfo.position, hitInfo.normal, hitInfo.direction);
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

		return pScene->GetCubeMap()->GetEnvironmentLight()->GetRadiance(ray.direction);
	}

	// ���ɵ�ǰhit��bsdf��Ϊ����Ӹ���ReflectionModel��
	hitInfo.ConstructReflectionModel();

	// Ȼ����㵱ǰhit��Radiance��Lo=Le+Lr
	Vector3 L(0.0);

	// ���hit���屾����ǹ�Դ�Ļ�������Le
	shared_ptr<NXPBRAreaLight> pAreaLight = hitInfo.pPrimitive->GetAreaLight();
	if (pAreaLight)
	{
		L += pAreaLight->GetRadiance(hitInfo.position, hitInfo.normal, hitInfo.direction);
	}

	Vector3 L(0.0f);
	// All : ͳ�����еĹ�Դ����ƽ��ֵ��
	auto pLights = pScene->GetPBRLights();
	for (auto it = pLights.begin(); it != pLights.end(); it++)
	{
		L += DirectEstimate(ray, pScene, *it, hitInfo);
	}
	L /= pLights.size();

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
}

Vector3 NXIntegrator::DirectEstimate(const Ray& ray, const shared_ptr<NXScene>& pScene, const shared_ptr<NXPBRLight>& pLight, const NXHit& hitInfo)
{
	Vector3 L(0.0f);

	// ͳ��pdfʱ����ͳ�ƴ���SPECULAR���͵ķ���ģ�͡�
	ReflectionType refType = (ReflectionType)(REFLECTIONTYPE_ALL & ~REFLECTIONTYPE_SPECULAR);
	bool bIsDeltaLight = pLight->IsDeltaLight;

	// �Ե��β�������������
	// ʹ�����ַ����������Ȼ���light����һ�Σ��ٻ���BSDF����һ�Ρ����ս������ֲ����Ľ����ϡ�

	// ����light�Ĳ���
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
				L += f * Li; // / pdfLight;	 // DeltaLight��pdfʵ����=1��
			}
			else
			{
				// ���ܼ���f()��������ҪBSDF��pdf������Ϊ���ǵ�������Ҫ�ṩBSDF��Light����pdf��ϣ����Ի��ǵü���BSDF��pdfֵ��
				float pdfBSDF = hitInfo.BSDF->Pdf(hitInfo.direction, incidentDirection, refType);
				// ����Ȩ�ء��Ի���Light�Ĳ�����LightΪ��Ҫ��Ȩ��BSDF��Ρ�
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
