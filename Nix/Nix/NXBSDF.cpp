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
	// ��wo��wi����������ϵת��Ϊ��������ϵ��
	Vector3 wo = WorldToReflection(woWorld);
	Vector3 wi = WorldToReflection(wiWorld);

	// �����Ǻͼ��������hit���ģ�����Ӧ��ʹ�ü��η���ng�жϵ�ǰBSDF�ķ���/�������ԣ���������ɫ��ֵ����ns��
	// ʹ��nsȷʵ���Եõ���ƽ������ֵ��������ns��ʵ�ʼ���ֵng���ڲ�һ�£����ܻᵼ���γ�©��򰵵㡣
	bool isReflect = woWorld.Dot(ng) * wiWorld.Dot(ng) > 0;

	Vector3 f(0.0f);
	for (int i = 0; i < (int)m_reflectionModels.size(); i++)
	{
		shared_ptr<NXReflectionModel> refModel = m_reflectionModels[i];

		// ֻͳ������ƥ��ķ���ģ��
		if (refModel->IsMatchingType(reflectType))
		{
			// ���������ͬ������hit�п����ǽ���״̬������״̬��
			// ����ǽ���״̬����������REFLECTION����ģ�͡�
			// ���������״̬����������TRANSMISSION����ģ�͡�
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

	// ��ȡ����һ������ģ�ͱ�������
	int sampleId = NXRandom::GetInstance()->CreateInt(0, (int)matchList.size() - 1);
	shared_ptr<NXReflectionModel> sampledModel = m_reflectionModels[matchList[sampleId]];

	// ��wo����������ϵת��Ϊ��������ϵ��
	Vector3 wo = WorldToReflection(woWorld);
	if (wo.z == 0.0f) return Vector3(0.0f);

	// ��ȡ�����������wi
	Vector3 wi;
	Vector3 f = sampledModel->Sample_f(wo, wi, pdf);
	outwiWorld = ReflectionToWorld(wi);

	// ����pdf
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
		// �����Specular����ģ�;�ʡ���ˡ�
		// ���������Sample_f����ֱ�ӵõ����������ϵ�f()ֵ��ֱ��ʹ�ü���
		return f;
	}

	// ����ͼ������з���ģ�͵�fֵ��
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
