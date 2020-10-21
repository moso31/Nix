#include "NXFresnel.h"

NXFresnel::NXFresnel()
{
}

NXFresnel::~NXFresnel()
{
}

Vector3 NXFresnelDielectric::FresnelReflectance(float cosThetaI) 
{
	// ������з����п��ܴ��ڽ���״̬(entering)������״̬(!entering)
	bool entering = cosThetaI > 0;
	if (!entering)
	{
		// ����״̬ʱ����ת���������ʣ�����ת���䷽��cosThetaI��ȷ���ͱ��淨��ͬ��
		std::swap(etaI, etaT);
		cosThetaI = abs(cosThetaI);
	}

	float sinThetaI = (1.0f - cosThetaI * cosThetaI);
	float sinThetaT = etaI / etaT * sinThetaI;
	if (sinThetaT >= 1) return Vector3(1.0f); // ȫ�ڷ������

	float cosThetaT = (1.0f - sinThetaT * sinThetaT);

	float Rs = (etaT * cosThetaI - etaI * cosThetaT) / (etaT * cosThetaI + etaI * cosThetaT);
	float Rp = (etaI * cosThetaI - etaT * cosThetaT) / (etaI * cosThetaI + etaT * cosThetaT);

	return Vector3(0.5f * (Rs * Rs + Rp * Rp));
}

Vector3 NXFresnelConductor::FresnelReflectance(float cosThetaI) 
{
	// �����в���������״̬��ֻ�н���״̬��
	// ���Է���һ�������б�Ҫ��cosThetaI���о���ֵ������ȷ�����䷽��ͱ��淨��ͬ��
	cosThetaI = abs(cosThetaI);

	Vector3 eta = etaT / etaI;
	Vector3 k = kT / etaI;

	// �����ֻ��thetaI�ˡ�û��thetaT�������Ķ������䡭����
	Vector3 cos2ThetaI = Vector3(cosThetaI * cosThetaI);
	Vector3 sin2ThetaI = Vector3(1.0f) - cos2ThetaI;

	Vector3 eta2 = eta * eta;
	Vector3 k2 = k * k;
	Vector3 temp1 = eta2 - k2 - sin2ThetaI;
	Vector3 a2plusb2 = Vector3::Sqrt(temp1 * temp1 + 4.0f * eta2 * k2);
	Vector3 a = Vector3::Sqrt(Vector3(0.5f) * (a2plusb2 + temp1));
	Vector3 _2aCosThetaI = 2.0f * a * cosThetaI;

	Vector3 temp2 = a2plusb2 + cos2ThetaI;
	Vector3 Rs = (temp2 - _2aCosThetaI) / (temp2 + _2aCosThetaI);

	Vector3 temp3 = cos2ThetaI * a2plusb2 + sin2ThetaI * sin2ThetaI;
	Vector3 temp4 = _2aCosThetaI * sin2ThetaI;
	Vector3 Rp = (temp3 - temp4) / (temp3 + temp4);
	
	return 0.5 * (Rp + Rs);
}

Vector3 NXFresnelCommon::FresnelReflectance(float cosThetaH)
{
	return F0 + (Vector3(1.0f) - F0) * powf(1.0f - fabsf(cosThetaH), 5.0f);
}
