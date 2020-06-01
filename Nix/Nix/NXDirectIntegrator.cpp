#include "NXDirectIntegrator.h"
#include "NXScene.h"
#include "NXCubeMap.h"

Vector3 NXDirectIntegrator::Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth)
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