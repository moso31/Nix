#include "NXIntegrator.h"
#include "NXBSDF.h"
#include "NXCubeMap.h"
#include "NXPrimitive.h"
#include "NXPBRLight.h"
#include "SamplerMath.h"
#include "NXRandom.h"

using namespace SimpleMath;
using namespace SamplerMath;

NXIntegrator::NXIntegrator()
{
}

NXIntegrator::~NXIntegrator()
{
}

Vector3 NXIntegrator::DirectEstimate(const Ray& ray, const std::shared_ptr<NXScene>& pScene, const std::shared_ptr<NXPBRLight>& pLight, const NXHit& hitInfo)
{
	Vector3 L(0.0f);

	// ͳ��pdfʱ����ͳ�ƴ���SPECULAR���͵ķ���ģ�͡�
	bool bIsDeltaLight = pLight->IsDeltaLight();

	if (bIsDeltaLight)
	{
		Vector3 incidentDirection;
		float pdfLight = 0.0f;
		Vector3 Li = pLight->Illuminate(hitInfo, incidentDirection, pdfLight);
		if (!Li.IsZero())
		{
			float pdfBSDF;
			Vector3 f = hitInfo.BSDF->Evaluate(hitInfo.direction, incidentDirection, pdfBSDF);
			if (!f.IsZero())
			{
				L += f * Li * incidentDirection.Dot(hitInfo.shading.normal);
			}
		}
	}
	else
	{
		Vector3 f(0.0f);
		Vector3 incidentDirection;
		float pdfLight = 0.0f, pdfBSDF = 0.0f, pdfWeight = 0.0f;

		// ���ڹ�Դ����һ��
		Vector3 Li = pLight->Illuminate(hitInfo, incidentDirection, pdfLight);
		if (!Li.IsZero() && pdfLight > 0.0f)
		{
			f = hitInfo.BSDF->Evaluate(hitInfo.direction, incidentDirection, pdfBSDF);
			if (!f.IsZero())
			{
				pdfWeight = PowerHeuristicWeightPdf(1, pdfLight, 1, pdfBSDF);
				L += f * Li * incidentDirection.Dot(hitInfo.shading.normal) * pdfWeight / pdfLight;
			}
		}

		// ����BSDF����һ��
		f = hitInfo.BSDF->Sample(hitInfo.direction, incidentDirection, pdfBSDF);
		if (!f.IsZero())
		{
			// ����BSDF�����ķ���Ѱ�Ҵ˴β����Ƿ���й�Դ��
			// ���й�Դ��ʹ�øù�Դ���Է���������ΪBSDF������Li��
			// δ���й�Դ��ʹ�û�����ͼ���Է�����ΪLi��û�л�����ͼ�򷵻�0.��
			Ray ray(hitInfo.position, incidentDirection);
			ray.position += incidentDirection * NXRT_EPSILON;
			Vector3 Li(0.0f);
			NXHit hitLightInfo;
			pScene->RayCast(ray, hitLightInfo);
			std::shared_ptr<NXPBRAreaLight> pHitAreaLight;
			if (hitLightInfo.pPrimitive)
			{
				pHitAreaLight = hitLightInfo.pPrimitive->GetTangibleLight();
			}
			else if (pScene->GetCubeMap())
			{
				pHitAreaLight = pScene->GetCubeMap()->GetEnvironmentLight();
			}

			if (pHitAreaLight == pLight)
			{
				Li = pHitAreaLight->GetRadiance(hitLightInfo.position, hitLightInfo.normal, -incidentDirection);
				pdfLight = pHitAreaLight->GetPdf(hitInfo, hitLightInfo.position, hitLightInfo.normal, -incidentDirection);

				if (!Li.IsZero())
				{
					// ����Ȩ�ء��Ի���BSDF�Ĳ�����BSDFΪ��Ҫ��Ȩ��Light��Ρ�
					pdfWeight = PowerHeuristicWeightPdf(1, pdfBSDF, 1, pdfLight);
					L += f * Li * incidentDirection.Dot(hitInfo.shading.normal) * pdfWeight / pdfBSDF;
				}
			}
		}
	}

	return L;
}

Vector3 NXIntegrator::UniformLightAll(const Ray& ray, const std::shared_ptr<NXScene>& pScene, const NXHit& hitInfo)
{
	// All: ͳ�����еĹ�Դ
	Vector3 result(0.0f);
	auto pLights = pScene->GetPBRLights();
	for (auto it = pLights.begin(); it != pLights.end(); it++)
		result += DirectEstimate(ray, pScene, *it, hitInfo);
	return result;
}

Vector3 NXIntegrator::UniformLightOne(const Ray& ray, const std::shared_ptr<NXScene>& pScene, const NXHit& hitInfo)
{
	// One: ��ͳ�Ƶ�����Դ�����Թ�Դ��ѡȡ��ȫ������˷�������ֵ��All������ͬ��
	Vector3 result(0.0f);
	auto pLights = pScene->GetPBRLights();
	int lightCount = (int)pLights.size();
	int index = NXRandom::GetInstance()->CreateInt(0, lightCount - 1);
	return DirectEstimate(ray, pScene, pLights[index], hitInfo) * (float)lightCount;
}
