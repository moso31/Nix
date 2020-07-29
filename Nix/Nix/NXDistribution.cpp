#include "NXDistribution.h"
#include "NXRandom.h"
#include "NXReflection.h"

using namespace NXReflection;

float NXRDistribution::G(const Vector3& wo, const Vector3& wi)
{
	float result = 1.0f / (1.0f + lambda(wo) + lambda(wi));
	return result;
}

float NXRDistribution::Pdf(const Vector3& wh)
{
	// ��΢�����ܶȷֲ��У�wh����ĸ����ܶ�p(wh)��
	// ͨ����D(wh)���ܶȷֲ���ͬ��
	// �������ҷֲ���
	float result = D(wh) * AbsCosTheta(wh);
	return result;
}

float NXRDistributionBeckmann::D(const Vector3& wh)
{
	// thetaN = wh��n�ļнǡ�
	// thetaH = wh��wo �� wh��wi �ļнǡ�
	float tan2ThetaN = Tan2Theta(wh);
	if (isinf(tan2ThetaN))
		return 0.0f;
	float cos4ThetaN = Cos2Theta(wh) * Cos2Theta(wh);
	float alpha2 = alpha * alpha;
	if (alpha2 <= 0.0f)
		return 0.0f;
	float result = exp(-tan2ThetaN / alpha2) / (XM_PI * alpha2 * cos4ThetaN);
	return result;
}

Vector3 NXRDistributionBeckmann::Sample_wh(const Vector3& wo)
{
	Vector2 u = NXRandom::GetInstance()->CreateVector2();
	float a2lnux = alpha * alpha * logf(u.x);
	float cosTheta = sqrt(1.0f / (1.0f - a2lnux));
	float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
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
	float result = (1 - 1.259f * a + 0.396f * a * a) / (3.535f * a + 2.181f * a * a);
	return result;
}
