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

	// 统计pdf时，不统计带有SPECULAR类型的反射模型。
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

		// 基于光源采样一次
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

		// 基于BSDF采样一次
		f = hitInfo.BSDF->Sample(hitInfo.direction, incidentDirection, pdfBSDF);
		if (!f.IsZero())
		{
			// 基于BSDF采样的方向寻找此次采样是否击中光源。
			// 击中光源：使用该光源的自发光数据作为BSDF样本的Li。
			// 未击中光源：使用环境贴图的自发光作为Li（没有环境贴图则返回0.）
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
					// 计算权重。对基于BSDF的采样，BSDF为主要加权，Light其次。
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
	// All: 统计所有的光源
	Vector3 result(0.0f);
	auto pLights = pScene->GetPBRLights();
	for (auto it = pLights.begin(); it != pLights.end(); it++)
		result += DirectEstimate(ray, pScene, *it, hitInfo);
	return result;
}

Vector3 NXIntegrator::UniformLightOne(const Ray& ray, const std::shared_ptr<NXScene>& pScene, const NXHit& hitInfo)
{
	// One: 仅统计单个光源，但对光源的选取完全随机。此方法期望值和All方法等同。
	Vector3 result(0.0f);
	auto pLights = pScene->GetPBRLights();
	int lightCount = (int)pLights.size();
	int index = NXRandom::GetInstance()->CreateInt(0, lightCount - 1);
	return DirectEstimate(ray, pScene, pLights[index], hitInfo) * (float)lightCount;
}
