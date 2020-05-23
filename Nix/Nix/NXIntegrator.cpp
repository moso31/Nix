#include "NXIntegrator.h"
#include "NXBSDF.h"

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
		// ���ޱ�����ͼ
		return Vector3(0.0);
	}

	// ���ɵ�ǰhit��bsdf��Ϊ����Ӹ���ReflectionModel��
	hitInfo.ConstructReflectionModel();

	// ���㵱ǰhit��bsdf��Radiance��Lo=Le+Lr
	// ����Le�����hit������ǹ�Դ�Ļ���
	if (false)
	{
		//  ���ڻ�û�������Դ��
	}

	Vector3 L(0.0);
	// ����Lr��Ϊ�����֣�Lֱ��+L��ӡ�
	// �ȼ���ֱ�ӹ���=Integrate(f(wo, wi) + Ld(wo, wi) * cos(wi), d(wi))
	// f��bsdf����ReflectionModel��f��
	// Ld: ʹ��Light�࣬�����Դ��Li�����Radiance��
	auto pLights = pScene->GetPBRLights();
	for (auto it = pLights.begin(); it != pLights.end(); it++)
	{
		Vector3 incidentDirection;
		Vector3 Li = (*it)->SampleIncidentRadiance(hitInfo, incidentDirection);
		Vector3 f = hitInfo.BSDF->f(hitInfo.direction, incidentDirection);

		// ��ʱ������Visibility Tester
		if (!f.IsZero())
		{
			L += f * Li * incidentDirection.Dot(hitInfo.normal);
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
