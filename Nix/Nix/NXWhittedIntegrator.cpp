#include "NXWhittedIntegrator.h"
#include "NXScene.h"
#include "NXCubeMap.h"

Vector3 NXWhittedIntegrator::Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth)
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
		return pCubeMap->GetEnvironmentLight()->GetRadiance(ignore, ignore, ray.direction);
	}

	// 生成当前hit的bsdf（为其添加各种ReflectionModel）
	hitInfo.GenerateBSDF(true);

	// 然后计算当前hit的Radiance：Lo=Le+Lr
	Vector3 L(0.0);

	// 如果hit物体本身就是光源的话，计算Le
	shared_ptr<NXPBRTangibleLight> pTangebleLight = hitInfo.pPrimitive->GetTangibleLight();
	if (pTangebleLight)
	{
		L += pTangebleLight->GetRadiance(hitInfo.position, hitInfo.normal, -hitInfo.direction);
	}

	// 计算Lr分为两部分，L直接+L间接。
	// 先计算直接光照=Integrate(f(wo, wi) + Ld(wo, wi) * cos(wi), d(wi))
	// f：bsdf所有ReflectionModel的f。
	// Ld: 使用Light类，计算光源在Li方向的Radiance。
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
