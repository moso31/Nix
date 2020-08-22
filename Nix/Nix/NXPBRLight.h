#pragma once
#include "NXInstance.h"
#include "NXIntersection.h"

class NXPrimitive;

class NXVisibleTest : public NXInstance<NXVisibleTest>
{
public:
	NXVisibleTest() {}
	~NXVisibleTest() {}

	void SetScene(std::shared_ptr<NXScene>& pScene) { m_pScene = pScene; }
	bool Do(const Vector3& startPosition, const Vector3& targetPosition);

protected:
	std::shared_ptr<NXScene> m_pScene;
};

class NXPBRLight
{
public:
	NXPBRLight() {}
	~NXPBRLight() {}

	virtual bool IsDeltaLight() = 0;
	virtual Vector3 Emit(Ray& o_ray, Vector3& o_lightNormal, float& o_pdfPos, float& o_pdfDir) = 0;
	virtual Vector3 Illuminate(const NXHit& hitInfo, Vector3& o_wi, float& o_pdf) = 0;
};

// 临时PBR光源
class NXPBRPointLight : public NXPBRLight
{
public:
	NXPBRPointLight(const Vector3& Position, const Vector3& Intensity) : Position(Position), Intensity(Intensity) {}
	~NXPBRPointLight() {}

	bool IsDeltaLight() override { return true; }

	Vector3 Emit(Ray& o_ray, Vector3& o_lightNormal, float& o_pdfPos, float& o_pdfDir) override;
	Vector3 Illuminate(const NXHit& hitInfo, Vector3& o_wi, float& o_pdf) override;

public:
	Vector3 Position;
	Vector3 Intensity;
};

class NXPBRDistantLight : public NXPBRLight
{
public:
	NXPBRDistantLight(const Vector3& Direction, const Vector3& Radiance, Vector3 WorldCenter, float WorldRadius);

	bool IsDeltaLight() override { return true; }
	Vector3 Emit(Ray& o_ray, Vector3& o_lightNormal, float& o_pdfPos, float& o_pdfDir) override;
	Vector3 Illuminate(const NXHit& hitInfo, Vector3& o_wi, float& o_pdf) override;

public:
	Vector3 Direction;
	Vector3 Radiance;
	Vector3 WorldCenter;
	float WorldRadius;
};

class NXPBRAreaLight : public NXPBRLight
{
public:
	NXPBRAreaLight() {};

	bool IsDeltaLight() override { return false; }

	// 计算目标方向（targetDirection）的法向量。
	// NXTangibleLight三个参数都用。
	// NXPBREnvironmentLight只需提供目标方向。
	virtual Vector3 GetRadiance(const Vector3& samplePosition, const Vector3& lightSurfaceNormal, const Vector3& targetDirection) = 0;

	// 获取朝某个采样方向发射时，对应的pdf值。
	virtual float GetPdf(const NXHit& hitInfo, const Vector3& lightPos, const Vector3& lightNorm, const Vector3& lightDir) = 0;
};

class NXPBRTangibleLight : public NXPBRAreaLight
{
public:
	NXPBRTangibleLight(const std::shared_ptr<NXPrimitive>& pPrimitive, const Vector3& Radiance);

	Vector3 Emit(Ray& o_ray, Vector3& o_lightNormal, float& o_pdfPos, float& o_pdfDir) override;
	Vector3 Illuminate(const NXHit& hitInfo, Vector3& o_wi, float& o_pdf) override;

	// 计算从 任意采样点 朝 目标方向 发射光线得到的Radiance值。
	// 需要提供 灯面发射点处的法向量，以判断本次交互是否相向。
	Vector3 GetRadiance(const Vector3& samplePosition, const Vector3& lightSurfaceNormal, const Vector3& targetDirection) override;

	float GetPdf(const NXHit& hitInfo, const Vector3& lightPos, const Vector3& lightNorm, const Vector3& lightDir) override;
public:
	Vector3 Radiance;

private:
	std::shared_ptr<NXPrimitive> m_pPrimitive;
};

class NXPBREnvironmentLight : public NXPBRAreaLight
{
public:
	NXPBREnvironmentLight(const std::shared_ptr<NXCubeMap>& pCubeMap, const Vector3& Radiance, Vector3 WorldCenter, float WorldRadius);

	Vector3 Emit(Ray& o_ray, Vector3& o_lightNormal, float& o_pdfPos, float& o_pdfDir) override;
	Vector3 Illuminate(const NXHit& hitInfo, Vector3& o_wi, float& o_pdf) override;

	// 计算自发光(emission)的radiance。
	Vector3 GetRadiance(const Vector3& samplePosition, const Vector3& lightSurfaceNormal, const Vector3& targetDirection) override;

	float GetPdf(const NXHit& hitInfo, const Vector3& lightPos, const Vector3& lightNorm, const Vector3& lightDir) override;
public:
	Vector3 Radiance;
	Vector3 WorldCenter;
	float WorldRadius;

private:
	std::shared_ptr<NXCubeMap> m_pCubeMap;
};