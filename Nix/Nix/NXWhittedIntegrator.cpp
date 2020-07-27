#include "NXWhittedIntegrator.h"
#include "NXScene.h"
#include "NXCubeMap.h"

Vector3 NXWhittedIntegrator::Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth)
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
		return pCubeMap->GetEnvironmentLight()->GetRadiance(ignore, ignore, ray.direction);
	}

	// ���ɵ�ǰhit��bsdf��Ϊ����Ӹ���ReflectionModel��
	hitInfo.GenerateBSDF(true);

	// Ȼ����㵱ǰhit��Radiance��Lo=Le+Lr
	Vector3 L(0.0);

	// ���hit���屾����ǹ�Դ�Ļ�������Le
	shared_ptr<NXPBRTangibleLight> pTangebleLight = hitInfo.pPrimitive->GetTangibleLight();
	if (pTangebleLight)
	{
		L += pTangebleLight->GetRadiance(hitInfo.position, hitInfo.normal, -hitInfo.direction);
	}

	// ����Lr��Ϊ�����֣�Lֱ��+L��ӡ�
	// �ȼ���ֱ�ӹ���=Integrate(f(wo, wi) + Ld(wo, wi) * cos(wi), d(wi))
	// f��bsdf����ReflectionModel��f��
	// Ld: ʹ��Light�࣬�����Դ��Li�����Radiance��
	auto pLights = pScene->GetPBRLights();
	for (auto it = pLights.begin(); it != pLights.end(); it++)
	{
		Vector3 incidentDirection;
		float pdfLight = 0.0f, pdfBSDF = 0.0f;
		Vector3 Li = (*it)->SampleIncidentRadiance(hitInfo, incidentDirection, pdfLight);
		if (Li.IsZero() || pdfLight == 0.0f)
			continue;

		Vector3 f = hitInfo.BSDF->Evaluate(hitInfo.direction, incidentDirection, pdfBSDF);
		if (!f.IsZero())
		{
			L += f * Li * incidentDirection.Dot(hitInfo.shading.normal) / pdfLight;
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
