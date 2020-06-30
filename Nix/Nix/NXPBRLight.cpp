#include "NXPBRLight.h"
#include "SamplerMath.h"
#include "NXScene.h"
#include "NXCubeMap.h"
#include "NXRandom.h"
#include "NXReflectionModel.h"

using namespace SamplerMath;

bool NXVisibleTest::Do(const Vector3& startPosition, const Vector3& targetPosition)
{
	Vector3 visibleTestDir = targetPosition - startPosition;
	// 需要减两个单位。
	// 因为ray本身向发射方向偏移了一个单位。所以为了确保检测距离短于比ray到灯光的距离，应该在此基础上再减一个单位。
	float maxDist = visibleTestDir.Length() - NXRT_EPSILON - NXRT_EPSILON;
	visibleTestDir.Normalize();
	Ray ray(startPosition, visibleTestDir);
	ray.position += ray.direction * NXRT_EPSILON;
	NXHit ignore;
	return !m_pScene->RayCast(ray, ignore, maxDist);
}

Vector3 NXPBRPointLight::SampleEmissionRadiance(Ray& out_emissionRay, Vector3& out_lightNormal, float& out_pdfPos, float& out_pdfDir)
{
	Vector2 random = NXRandom::GetInstance()->CreateVector2();
	out_lightNormal = UniformSampleSphere(random);
	out_emissionRay = Ray(Position, out_lightNormal);
	out_pdfPos = 1.0f;
	out_pdfDir = UniformSampleSpherePdf();
	return Intensity;
}

Vector3 NXPBRPointLight::SampleIncidentRadiance(const NXHit& hitInfo, Vector3& out_wi, float& out_pdf)
{
	// 光线发射方向
	out_wi = (Position - hitInfo.position);
	Vector3 lightRadiance = Intensity / out_wi.LengthSquared();
	out_wi.Normalize();
	out_pdf = 1;
	if (!NXVisibleTest::GetInstance()->Do(hitInfo.position, Position))
		return Vector3(0.0f);

	return lightRadiance;
}

NXPBRDistantLight::NXPBRDistantLight(const Vector3& Direction, const Vector3& Radiance, const shared_ptr<NXScene>& pScene) : 
	Direction(Direction), 
	Radiance(Radiance),
	SceneBoundingSphere(pScene->GetBoundingSphere())
{
}

Vector3 NXPBRDistantLight::SampleEmissionRadiance(Ray& out_emissionRay, Vector3& out_lightNormal, float& out_pdfPos, float& out_pdfDir)
{
	float worldRadius = SceneBoundingSphere.Radius;

	Vector3 basis1, basis2;
	Direction.GenerateCoordinateSpace(basis1, basis2);
	Vector2 random = NXRandom::GetInstance()->CreateVector2();
	Vector2 diskCoord = UniformSampleDisk(random);
	Vector3 sampleLightPosition = (diskCoord.x * basis1 + diskCoord.y * basis2 - Direction) * worldRadius;

	out_emissionRay = Ray(sampleLightPosition, Direction);
	out_lightNormal = Direction;
	out_pdfPos = 1.0f / (XM_PI * worldRadius * worldRadius);
	out_pdfDir = 1.0f;

	return Radiance;
}

Vector3 NXPBRDistantLight::SampleIncidentRadiance(const NXHit& hitInfo, Vector3& out_wi, float& out_pdf)
{
	out_wi = -Direction;
	out_wi.Normalize();
	out_pdf = 1;
	if (!NXVisibleTest::GetInstance()->Do(hitInfo.position, hitInfo.position - Direction))
		return Vector3(0.0f);

	return Radiance;
}

NXPBRTangibleLight::NXPBRTangibleLight(const shared_ptr<NXPrimitive>& pPrimitive, const Vector3& Radiance) :
	m_pPrimitive(pPrimitive),
	Radiance(Radiance)
{
}

Vector3 NXPBRTangibleLight::SampleEmissionRadiance(Ray& out_emissionRay, Vector3& out_lightNormal, float& out_pdfPos, float& out_pdfDirLocal)
{
	Vector3 sampleLightPosition;
	m_pPrimitive->SampleFromSurface(sampleLightPosition, out_lightNormal, out_pdfPos);
	Vector2 vRandomDir = NXRandom::GetInstance()->CreateVector2();
	Vector3 sampleDir = CosineSampleHemisphere(vRandomDir);
	out_pdfDirLocal = CosineSampleHemispherePdf(sampleDir.z);

	Vector3 basis1, basis2;
	out_lightNormal.GenerateCoordinateSpace(basis1, basis2);
	Vector3 sampleLightDirection = sampleDir.x * basis1 + sampleDir.y * basis2 + sampleDir.z * out_lightNormal;

	out_emissionRay = Ray(sampleLightPosition, sampleLightDirection);
	out_emissionRay.position += sampleLightDirection * NXRT_EPSILON;

	return GetRadiance(sampleLightPosition, out_lightNormal, sampleLightDirection);
}

Vector3 NXPBRTangibleLight::SampleIncidentRadiance(const NXHit& hitInfo, Vector3& out_wi, float& out_pdf)
{
	Vector3 sampleLightPosition, sampleLightNormal;		// 灯光采样点的位置和该处的法向量
	m_pPrimitive->SampleFromSurface(sampleLightPosition, sampleLightNormal, out_pdf);
	out_wi = sampleLightPosition - hitInfo.position;
	out_wi.Normalize();

	if (!NXVisibleTest::GetInstance()->Do(hitInfo.position, sampleLightPosition))
		return Vector3(0.0f);	// 无效光源：被场景中其他物体挡住

	return GetRadiance(sampleLightPosition, sampleLightNormal, -out_wi);
}

Vector3 NXPBRTangibleLight::GetRadiance(const Vector3& samplePosition, const Vector3& lightSurfaceNormal, const Vector3& targetDirection)
{
	// targetDirection从原点向外发射。
	if (lightSurfaceNormal.Dot(targetDirection) <= 0)
		return Vector3(0.0f);	// 无效光源：采样射线方向背朝表面
	return Radiance;
}

float NXPBRTangibleLight::GetPdf(const NXHit& hitInfo, const Vector3& direction)
{
	return m_pPrimitive->GetPdf(hitInfo, direction);
}

NXPBREnvironmentLight::NXPBREnvironmentLight(const shared_ptr<NXCubeMap>& pCubeMap, const Vector3& Radiance, float SceneRadius) :
	m_pCubeMap(pCubeMap),
	Radiance(Radiance),
	SceneRadius(SceneRadius)
{
}

Vector3 NXPBREnvironmentLight::SampleEmissionRadiance(Ray& out_emissionRay, Vector3& out_lightNormal, float& out_pdfPos, float& out_pdfDir)
{
	Vector3 basis1, basis2;

	Vector2 randomDir = NXRandom::GetInstance()->CreateVector2();
	Vector3 sampleDir = UniformSampleSphere(randomDir);	// 从全球方向均匀采样。水平线以下的样本可能很少用到（用余弦可能更好？）。
	sampleDir.GenerateCoordinateSpace(basis1, basis2);

	Vector2 randomPos = NXRandom::GetInstance()->CreateVector2();
	Vector2 diskCoord = UniformSampleDisk(randomPos);
	Vector3 sampleLightPosition = (diskCoord.x * basis1 + diskCoord.y * basis2 + sampleDir) * SceneRadius;

	out_lightNormal = -sampleDir;
	out_emissionRay = Ray(sampleLightPosition, out_lightNormal);
	out_pdfDir = UniformSampleSpherePdf();
	out_pdfPos = 1.0f / (XM_PI * SceneRadius * SceneRadius);

	Vector3 ignore;
	return GetRadiance(ignore, ignore, sampleDir);
}

Vector3 NXPBREnvironmentLight::SampleIncidentRadiance(const NXHit& hitInfo, Vector3& out_wi, float& out_pdf)
{
	Vector2 random = NXRandom::GetInstance()->CreateVector2();
	// 暂时用余弦采样。可能均匀采样更好？
	out_wi = CosineSampleHemisphere(random);
	out_pdf = CosineSampleHemispherePdf(out_wi.z);
	out_wi = hitInfo.BSDF->ReflectionToWorld(out_wi);

	if (!NXVisibleTest::GetInstance()->Do(hitInfo.position, hitInfo.position + out_wi * 2.0f * SceneRadius))
		return Vector3(0.0f);	// 无效光源：被场景中其他物体挡住
	
	Vector3 ignore;
	return GetRadiance(ignore, ignore, out_wi);
}

Vector3 NXPBREnvironmentLight::GetRadiance(const Vector3& samplePosition, const Vector3& lightSurfaceNormal, const Vector3& targetDirection)
{
	// targetDirection从中心向外发射。
	return m_pCubeMap->BackgroundColorByDirection(targetDirection) * Radiance;
}

float NXPBREnvironmentLight::GetPdf(const NXHit& hitInfo, const Vector3& targetDirection)
{
	Vector3 localDirection = hitInfo.BSDF->WorldToReflection(targetDirection);
	return fabsf(localDirection.z * XM_1DIVPI);
}
