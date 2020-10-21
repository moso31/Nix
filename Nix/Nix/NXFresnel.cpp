#include "NXFresnel.h"

NXFresnel::NXFresnel()
{
}

NXFresnel::~NXFresnel()
{
}

Vector3 NXFresnelDielectric::FresnelReflectance(float cosThetaI) 
{
	// 电介质中法线有可能处于进入状态(entering)或逃逸状态(!entering)
	bool entering = cosThetaI > 0;
	if (!entering)
	{
		// 逃逸状态时，反转上下折射率，并反转入射方向cosThetaI以确保和表面法线同向
		std::swap(etaI, etaT);
		cosThetaI = abs(cosThetaI);
	}

	float sinThetaI = (1.0f - cosThetaI * cosThetaI);
	float sinThetaT = etaI / etaT * sinThetaI;
	if (sinThetaT >= 1) return Vector3(1.0f); // 全内反射情况

	float cosThetaT = (1.0f - sinThetaT * sinThetaT);

	float Rs = (etaT * cosThetaI - etaI * cosThetaT) / (etaT * cosThetaI + etaI * cosThetaT);
	float Rp = (etaI * cosThetaI - etaT * cosThetaT) / (etaI * cosThetaI + etaT * cosThetaT);

	return Vector3(0.5f * (Rs * Rs + Rp * Rp));
}

Vector3 NXFresnelConductor::FresnelReflectance(float cosThetaI) 
{
	// 导体中不存在逃逸状态，只有进入状态。
	// 但以防万一，还是有必要对cosThetaI进行绝对值处理，以确保入射方向和表面法线同向。
	cosThetaI = abs(cosThetaI);

	Vector3 eta = etaT / etaI;
	Vector3 k = kT / etaI;

	// 导体就只有thetaI了。没有thetaT（导体哪儿能折射……）
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
