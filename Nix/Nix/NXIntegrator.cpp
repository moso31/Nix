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
		Vector3 f = hitInfo.bsdf.f();

		// ��ʱ������Visibility Tester
		if (!f.IsZero())
		{
			L += f * Li * cos(incidentDirection * hitInfo.normal);
		}
	}

	// Ȼ������ӹ��ա�
	// Whitted���ֹ��ս�ͳ�������������淴���������������ļ��������
	// ���Կ���ֱ�ӷֳ�����������ר��ͳ�Ƽ�ӷ���ͼ�����䡣
	const static int maxDepth = 5;
	if (depth < maxDepth)
	{
		L += SpecularReflect(ray, hitInfo, pScene, depth);
		L += SpecularTransmit(ray, hitInfo, pScene, depth);
	}

	return L;

	// ��ӷ��䣺
	// f������ȡbsdf�о������������ReflectionModel����ͳ����Sample_f��
	// Lr���ݹ���㣬depth+1��

	// ������䣺
	// f������ȡbsdf�о������������ReflectionModel����ͳ����Sample_f��
	// Lr���ݹ���㣬depth+1��
}

Vector3 NXIntegrator::SpecularReflect(const Ray& ray, const NXHit& hit, const shared_ptr<NXScene>& pScene, int depth)
{
	return Vector3();
}

Vector3 NXIntegrator::SpecularTransmit(const Ray& ray, const NXHit& hit, const shared_ptr<NXScene>& pScene, int depth)
{
	return Vector3();
}
