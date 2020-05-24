#include "NXDistribution.h"
#include "NXRandom.h"
#include "NXReflectionModel.h"

using namespace NXReflection;

float NXRDistribution::G(const Vector3& wo, const Vector3& wi)
{
	return 1.0f / (1.0f + lambda(wo) + lambda(wi));
}

float NXRDistribution::Pdf(const Vector3& wh)
{
	// ��΢�����ܶȷֲ��У�wh����ĸ����ܶ�p(wh)��
	// ͨ����D(wh)���ܶȷֲ���ͬ��
	// �������ҷֲ���
	return D(wh) * AbsCosTheta(wh);
}

float NXRDistributionBeckmann::D(const Vector3& wh)
{
	// thetaN = wh��n�ļнǡ�
	// thetaH = wh��wo��wh��wi�ļнǡ�
	float cos2ThetaN = wh.z * wh.z;
	float sin2ThetaN = 1 - cos2ThetaN;
	float tan2ThetaN = sin2ThetaN / cos2ThetaN;
	if (isinf(tan2ThetaN))
		return 0.0f;
	float cos4ThetaN = cos2ThetaN * cos2ThetaN;
	return exp(-tan2ThetaN / alpha) / (XM_PI * alpha * alpha * cos4ThetaN);
}

Vector3 NXRDistributionBeckmann::Sample_wh(const Vector3& wo)
{
	Vector2 u = NXRandom::GetInstance()->CreateVector2();
	float a2lnux = alpha * alpha * logf(u.x);
	float sinTheta = sqrt(1.0f / (1.0f - a2lnux));
	float cosTheta = sqrt(1.0f - sinTheta * sinTheta);
	float phi = XM_2PI * u.y;
	Vector3 wh = Vector3(sinTheta * cosf(phi), sinTheta * sinf(phi), cosTheta);
	if (!IsSameHemisphere(wo, wh)) wh = -wh;	// wh��woͬһ����
	return wh;
}

float NXRDistributionBeckmann::lambda(const Vector3& w)
{
	float absTanTheta = std::abs(TanTheta(w));
	if (std::isinf(absTanTheta)) return 0.;
	// �ݲ������������
	float a = 1 / (alpha * absTanTheta);
	if (a >= 1.6f) return 0;
	return (1 - 1.259f * a + 0.396f * a * a) / (3.535f * a + 2.181f * a * a);
}