#include "NXPBRLight.h"
#include "SamplerMath.h"
#include "NXScene.h"
#include "NXCubeMap.h"
#include "NXPrimitive.h"
#include "NXRandom.h"
#include "NXReflection.h"

using namespace SamplerMath;

bool NXVisibleTest::Do(const Vector3& startPosition, const Vector3& targetPosition)
{
	Vector3 visibleTestDir = targetPosition - startPosition;
	// ��Ҫ��������λ��
	// ��Ϊray�������䷽��ƫ����һ����λ������Ϊ��ȷ����������ڱ�ray���ƹ�ľ��룬Ӧ���ڴ˻������ټ�һ����λ��
	float maxDist = visibleTestDir.Length() - NXRT_EPSILON - NXRT_EPSILON;
	visibleTestDir.Normalize();
	Ray ray(startPosition, visibleTestDir);
	ray.position += ray.direction * NXRT_EPSILON;
	NXHit ignore;
	return !m_pScene->RayCast(ray, ignore, maxDist);
}

Vector3 NXPBRPointLight::Emit(Ray& o_ray, Vector3& o_lightNormal, float& o_pdfPos, float& o_pdfDir)
{
	Vector2 random = NXRandom::GetInstance()->CreateVector2();
	o_lightNormal = UniformSampleSphere(random);
	o_ray = Ray(Position, o_lightNormal);
	o_pdfPos = 1.0f;
	o_pdfDir = UniformSampleSpherePdf();
	return Intensity;
}

Vector3 NXPBRPointLight::Illuminate(const NXHit& hitInfo, Vector3& o_wi, float& o_pdf)
{
	// ���߷��䷽��
	o_wi = (Position - hitInfo.position);
	Vector3 lightRadiance = Intensity / o_wi.LengthSquared();
	o_wi.Normalize();
	o_pdf = 1;
	if (!NXVisibleTest::GetInstance()->Do(hitInfo.position, Position))
		return Vector3(0.0f);

	return lightRadiance;
}

NXPBRDistantLight::NXPBRDistantLight(const Vector3& Direction, const Vector3& Radiance, Vector3 WorldCenter, float WorldRadius) :
	Direction(Direction), 
	Radiance(Radiance),
	WorldCenter(WorldCenter),
	WorldRadius(WorldRadius)
{
	this->Direction.Normalize();
}

Vector3 NXPBRDistantLight::Emit(Ray& o_ray, Vector3& o_lightNormal, float& o_pdfPos, float& o_pdfDir)
{
	Vector3 basis1, basis2;
	Direction.GenerateCoordinateSpace(basis1, basis2);
	Vector2 random = NXRandom::GetInstance()->CreateVector2();
	Vector2 diskCoord = UniformSampleDisk(random);
	Vector3 sampleLightPosition = WorldCenter + (diskCoord.x * basis1 + diskCoord.y * basis2 - Direction) * WorldRadius;

	o_ray = Ray(sampleLightPosition, Direction);
	o_lightNormal = Direction;
	o_pdfPos = 1.0f / (XM_PI * WorldRadius * WorldRadius);
	o_pdfDir = 1.0f;

	return Radiance;
}

Vector3 NXPBRDistantLight::Illuminate(const NXHit& hitInfo, Vector3& o_wi, float& o_pdf)
{
	o_wi = -Direction;
	o_wi.Normalize();
	o_pdf = 1;
	if (!NXVisibleTest::GetInstance()->Do(hitInfo.position, hitInfo.position - Direction))
		return Vector3(0.0f);

	return Radiance;
}

NXPBRTangibleLight::NXPBRTangibleLight(const std::shared_ptr<NXPrimitive>& pPrimitive, const Vector3& Radiance) :
	m_pPrimitive(pPrimitive),
	Radiance(Radiance)
{
}

Vector3 NXPBRTangibleLight::Emit(Ray& o_ray, Vector3& o_lightNormal, float& o_pdfPos, float& o_pdfDirLocal)
{
	Vector3 sampleLightPosition;
	m_pPrimitive->SampleForArea(sampleLightPosition, o_lightNormal, o_pdfPos);
	Vector2 vRandomDir = NXRandom::GetInstance()->CreateVector2();
	Vector3 sampleDir = CosineSampleHemisphere(vRandomDir);
	o_pdfDirLocal = CosineSampleHemispherePdf(sampleDir.z);

	Vector3 basis1, basis2;
	o_lightNormal.GenerateCoordinateSpace(basis1, basis2);
	Vector3 sampleLightDirection = sampleDir.x * basis1 + sampleDir.y * basis2 + sampleDir.z * o_lightNormal;

	o_ray = Ray(sampleLightPosition, sampleLightDirection);
	o_ray.position += sampleLightDirection * NXRT_EPSILON;

	return GetRadiance(sampleLightPosition, o_lightNormal, sampleLightDirection);
}

Vector3 NXPBRTangibleLight::Illuminate(const NXHit& hitInfo, Vector3& o_wi, float& o_pdfW)
{
	Vector3 sampleLightPosition, sampleLightNormal;		// �ƹ�������λ�ú͸ô��ķ�����
	m_pPrimitive->SampleForSolidAngle(hitInfo, sampleLightPosition, sampleLightNormal, o_pdfW);
	o_wi = sampleLightPosition - hitInfo.position;
	o_wi.Normalize();

	if (!NXVisibleTest::GetInstance()->Do(hitInfo.position, sampleLightPosition))
		return Vector3(0.0f);	// ��Ч��Դ�����������������嵲ס

	return GetRadiance(sampleLightPosition, sampleLightNormal, -o_wi);
}

Vector3 NXPBRTangibleLight::GetRadiance(const Vector3& samplePosition, const Vector3& lightSurfaceNormal, const Vector3& targetDirection)
{
	// targetDirection��ԭ�����ⷢ�䡣
	if (lightSurfaceNormal.Dot(targetDirection) <= 0)
		return Vector3(0.0f);	// ��Ч��Դ���������߷��򱳳�����
	return Radiance;
}

float NXPBRTangibleLight::GetPdf(const NXHit& hitInfo, const Vector3& lightPos, const Vector3& lightNorm, const Vector3& lightDir)
{
	return m_pPrimitive->GetPdfSolidAngle(hitInfo, lightPos, lightNorm, lightDir);
}

NXPBREnvironmentLight::NXPBREnvironmentLight(const std::shared_ptr<NXCubeMap>& pCubeMap, const Vector3& Radiance, Vector3 WorldCenter, float WorldRadius) :
	m_pCubeMap(pCubeMap),
	Radiance(Radiance),
	WorldCenter(WorldCenter),
	WorldRadius(WorldRadius)
{
}

Vector3 NXPBREnvironmentLight::Emit(Ray& o_ray, Vector3& o_lightNormal, float& o_pdfPos, float& o_pdfDir)
{
	Vector3 basis1, basis2;

	Vector2 randomDir = NXRandom::GetInstance()->CreateVector2();
	Vector3 sampleDir = UniformSampleSphere(randomDir);	// ��ȫ������Ȳ�����ˮƽ�����µ��������ܺ����õ��������ҿ��ܸ��ã�����
	sampleDir.GenerateCoordinateSpace(basis1, basis2);

	Vector2 randomPos = NXRandom::GetInstance()->CreateVector2();
	Vector2 diskCoord = UniformSampleDisk(randomPos);
	Vector3 sampleLightPosition = WorldCenter + (diskCoord.x * basis1 + diskCoord.y * basis2 + sampleDir) * WorldRadius;

	o_lightNormal = -sampleDir;
	o_ray = Ray(sampleLightPosition, o_lightNormal);
	o_pdfDir = UniformSampleSpherePdf();
	o_pdfPos = 1.0f / (XM_PI * WorldRadius * WorldRadius);

	Vector3 ignore;
	return GetRadiance(ignore, ignore, sampleDir);
}

Vector3 NXPBREnvironmentLight::Illuminate(const NXHit& hitInfo, Vector3& o_wi, float& o_pdf)
{
	Vector2 random = NXRandom::GetInstance()->CreateVector2();
	// ��ʱ�����Ҳ��������ܾ��Ȳ������ã�
	o_wi = CosineSampleHemisphere(random);
	o_pdf = CosineSampleHemispherePdf(o_wi.z);
	o_wi = hitInfo.BSDF->ReflectionToWorld(o_wi);

	if (!NXVisibleTest::GetInstance()->Do(hitInfo.position, hitInfo.position + o_wi * 2.0f * WorldRadius))
		return Vector3(0.0f);	// ��Ч��Դ�����������������嵲ס
	
	Vector3 ignore;
	return GetRadiance(ignore, ignore, o_wi);
}

Vector3 NXPBREnvironmentLight::GetRadiance(const Vector3& samplePosition, const Vector3& lightSurfaceNormal, const Vector3& targetDirection)
{
	// targetDirection���������ⷢ�䡣
	return m_pCubeMap->BackgroundColorByDirection(targetDirection) * Radiance;
}

float NXPBREnvironmentLight::GetPdf(const NXHit& hitInfo, const Vector3& lightPos, const Vector3& lightNorm, const Vector3& lightDir)
{
	Vector3 localDirection = hitInfo.BSDF->WorldToReflection(lightDir);
	return fabsf(localDirection.z * XM_1DIVPI);
}
