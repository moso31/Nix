#pragma once
#include "NXIntersection.h"
#include "NXScene.h"

class NXIntegrator
{
public:
	NXIntegrator();
	~NXIntegrator();

	virtual Vector3 Radiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth) = 0;

	/*
	�Ե��β�������������
	�����DeltaLight��
		����light�ṩ�ķ�����м��㼴�ɵõ�ʵ�ʾ�ȷֵ��
	�������DeltaLight��
		�޷��õ���ȷֵ����ʹ�����ַ���������һ�Σ�
			1.����light����һ�Σ�
			2.����BSDF����һ�Ρ�
		֮�����β����Ľ����ϣ��õ�һ����Ϊ׼ȷ�Ĺ���ֵ��
	*/
	Vector3 DirectEstimate(const Ray& ray, const shared_ptr<NXScene>& pScene, const shared_ptr<NXPBRLight>& pLight, const NXHit& hitInfo);
	Vector3 SpecularReflect(const Ray& ray, const NXHit& hit, const shared_ptr<NXScene>& pScene, int depth);
	Vector3 SpecularTransmit(const Ray& ray, const NXHit& hit, const shared_ptr<NXScene>& pScene, int depth);
};
