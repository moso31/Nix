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
	// 寻找射线-场景hit
	NXHit hitInfo;
	bool isIntersect = pScene->RayCast(ray, hitInfo);

	// 如果没有相交，拿背景贴图的radiance并结束计算即可。
	if (!isIntersect)
	{	
		// 暂无背景贴图
		return Vector3(0.0);
	}

	// 生成当前hit的bsdf（为其添加各种ReflectionModel）
	hitInfo.ConstructReflectionModel();

	// 计算当前hit的bsdf的Radiance：Lo=Le+Lr
	// 计算Le（如果hit本身就是光源的话）
	if (false)
	{
		//  现在还没有面积光源。
	}

	Vector3 L(0.0);
	// 计算Lr分为两部分，L直接+L间接。
	// 先计算直接光照=Integrate(f(wo, wi) + Ld(wo, wi) * cos(wi), d(wi))
	// f：bsdf所有ReflectionModel的f。
	// Ld: 使用Light类，计算光源在Li方向的Radiance。
	auto pLights = pScene->GetPBRLights();
	for (auto it = pLights.begin(); it != pLights.end(); it++)
	{
		Vector3 incidentDirection;
		Vector3 Li = (*it)->SampleIncidentRadiance(hitInfo, incidentDirection);
		Vector3 f = hitInfo.bsdf.f();

		// 暂时不考虑Visibility Tester
		if (!f.IsZero())
		{
			L += f * Li * cos(incidentDirection * hitInfo.normal);
		}
	}

	// 然后计算间接光照。
	// Whitted积分光照仅统计来自完美镜面反射和完美镜面折射的间接照明。
	// 所以可以直接分成两个函数，专门统计间接反射和间接折射。
	const static int maxDepth = 5;
	if (depth < maxDepth)
	{
		L += SpecularReflect(ray, hitInfo, pScene, depth);
		L += SpecularTransmit(ray, hitInfo, pScene, depth);
	}

	return L;

	// 间接反射：
	// f：仅提取bsdf中具有完美反射的ReflectionModel，并统计其Sample_f。
	// Lr：递归计算，depth+1。

	// 间接折射：
	// f：仅提取bsdf中具有完美折射的ReflectionModel，并统计其Sample_f。
	// Lr：递归计算，depth+1。
}

Vector3 NXIntegrator::SpecularReflect(const Ray& ray, const NXHit& hit, const shared_ptr<NXScene>& pScene, int depth)
{
	return Vector3();
}

Vector3 NXIntegrator::SpecularTransmit(const Ray& ray, const NXHit& hit, const shared_ptr<NXScene>& pScene, int depth)
{
	return Vector3();
}
