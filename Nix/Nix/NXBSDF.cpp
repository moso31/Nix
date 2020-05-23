#include "NXBSDF.h"
#include "NXIntersection.h"
#include "NXRandom.h"

NXBSDF::NXBSDF(const NXHit& pHitInfo, float matIOR) :
	ng(pHitInfo.normal),
	ns(pHitInfo.shading.normal),
	ss(pHitInfo.shading.dpdu),
	ts(ns.Cross(ss)),
	eta(matIOR)
{
}

void NXBSDF::AddReflectionModel(const shared_ptr<NXReflectionModel>& pReflectionModel)
{
	m_reflectionModels.push_back(pReflectionModel);
}

Vector3 NXBSDF::f(const Vector3& woWorld, const Vector3& wiWorld, ReflectionType reflectType)
{
	// 将wo和wi从世界坐标系转换为反射坐标系。
	Vector3 wo = WorldToReflection(woWorld);
	Vector3 wi = WorldToReflection(wiWorld);

	// 光线是和几何体进行hit检测的，所以应该使用几何法线ng判断当前BSDF的反射/折射特性，而不是着色插值法线ns。
	// 使用ns确实可以得到更平滑的数值，但由于ns和实际几何值ng存在不一致，可能会导致形成漏光或暗点。
	bool isReflect = woWorld.Dot(ng) * wiWorld.Dot(ng) > 0;

	Vector3 f(0.0f);
	for (int i = 0; i < (int)m_reflectionModels.size(); i++)
	{
		shared_ptr<NXReflectionModel> refModel = m_reflectionModels[i];

		// 只统计类型匹配的反射模型
		if (refModel->IsMatchingType(reflectType))
		{
			// 根据情况不同，本次hit有可能是进入状态或逃逸状态。
			// 如果是进入状态，则处理所有REFLECTION反射模型。
			// 如果是逃逸状态，则处理所有TRANSMISSION反射模型。
			if ((isReflect && refModel->GetReflectionType() & REFLECTIONTYPE_REFLECTION) ||
				(!isReflect && refModel->GetReflectionType() & REFLECTIONTYPE_TRANSMISSION))
			{
				f += refModel->f(wo, wi);
			}
		}
	}

	return f;
}

Vector3 NXBSDF::Sample_f(const Vector3& woWorld, Vector3& outwiWorld, float& pdf, ReflectionType reflectType)
{
	vector<int> matchList;
	for (int i = 0; i < (int)m_reflectionModels.size(); i++)
		if (m_reflectionModels[i]->IsMatchingType(reflectType))
			matchList.push_back(i);

	if (matchList.empty()) 
		return Vector3(0.0f);

	// 获取是哪一个反射模型被采样。
	int sampleId = NXRandom::GetInstance()->CreateInt(0, (int)matchList.size() - 1);
	shared_ptr<NXReflectionModel> sampledModel = m_reflectionModels[matchList[sampleId]];

	// 将wo从世界坐标系转换为反射坐标系。
	Vector3 wo = WorldToReflection(woWorld);
	if (wo.z == 0.0f) return Vector3(0.0f);

	// 获取随机采样方向wi
	Vector3 wi;
	Vector3 f = sampledModel->Sample_f(wo, wi, pdf);
	outwiWorld = ReflectionToWorld(wi);

	// 计算pdf
	for (int i = 0; i < (int)matchList.size(); i++)
	{
		if (i == sampleId) continue;
		pdf += m_reflectionModels[matchList[i]]->Pdf(wo, wi);
	}
	pdf /= (float)matchList.size();

	if (pdf == 0.0f) 
		return Vector3(0.0f);

	if (sampledModel->GetReflectionType() & REFLECTIONTYPE_SPECULAR)
	{
		// 如果是Specular反射模型就省事了。
		// 完美反射的Sample_f可以直接得到采样方向上的f()值。直接使用即可
		return f;
	}

	// 否则就计算所有反射模型的f值。
	f = Vector3(0.0f);
	bool isReflect = wo.Dot(ng) * wi.Dot(ng) > 0;
	for (int i = 0; i < (int)m_reflectionModels.size(); i++)
	{
		shared_ptr<NXReflectionModel> refModel = m_reflectionModels[i];

		if (refModel->IsMatchingType(reflectType))
		{
			if ((isReflect && refModel->GetReflectionType() & REFLECTIONTYPE_REFLECTION) ||
				(!isReflect && refModel->GetReflectionType() & REFLECTIONTYPE_TRANSMISSION))
			{
				f += refModel->f(wo, wi);
			}
		}
	}

	return f;
}

Vector3 NXBSDF::WorldToReflection(const Vector3& p)
{
	return Vector3(p.Dot(ss), p.Dot(ts), p.Dot(ns));
}

Vector3 NXBSDF::ReflectionToWorld(const Vector3& p)
{
	return Vector3(
		p.x * ss.x + p.y * ts.x + p.z * ns.x,
		p.x * ss.y + p.y * ts.y + p.z * ns.y,
		p.x * ss.z + p.y * ts.z + p.z * ns.z);
}
