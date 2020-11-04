#pragma once
#include "NXInstance.h"
#include "NXIntersection.h"
#include "ShaderStructures.h"

class NXPrimitive;

class NXVisibleTest : public NXInstance<NXVisibleTest>
{
public:
	NXVisibleTest() {}
	~NXVisibleTest() {}

	void SetScene(NXScene* pScene) { m_pScene = pScene; }
	bool Do(const Vector3& startPosition, const Vector3& targetPosition);

protected:
	NXScene* m_pScene;
};

class NXPBRLight
{
public:
	NXPBRLight() {}
	virtual ~NXPBRLight() {}

	virtual bool IsDeltaLight() = 0;
	virtual Vector3 Emit(Ray& o_ray, Vector3& o_lightNormal, float& o_pdfPos, float& o_pdfDir) = 0;
	virtual Vector3 Illuminate(const NXHit& hitInfo, Vector3& o_wi, float& o_pdf) = 0;
};

// ��ʱPBR��Դ
class NXPBRPointLight : public NXPBRLight
{
public:
	NXPBRPointLight(const Vector3& Position, const Vector3& Intensity) : Position(Position), Intensity(Intensity) {}
	~NXPBRPointLight() {}

	bool IsDeltaLight() override { return true; }

	Vector3 Emit(Ray& o_ray, Vector3& o_lightNormal, float& o_pdfPos, float& o_pdfDir) override;
	Vector3 Illuminate(const NXHit& hitInfo, Vector3& o_wi, float& o_pdf) override;

	// DirectX
	ConstantBufferPointLight GetConstantBuffer();

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

	// DirectX
	ConstantBufferDistantLight GetConstantBuffer();

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

	// ����Ŀ�귽��targetDirection���ķ�������
	// NXTangibleLight�����������á�
	// NXPBREnvironmentLightֻ���ṩĿ�귽��
	virtual Vector3 GetRadiance(const Vector3& samplePosition, const Vector3& lightSurfaceNormal, const Vector3& targetDirection) = 0;

	// ��ȡ��ĳ������������ʱ����Ӧ��pdfֵ��
	virtual float GetPdf(const NXHit& hitInfo, const Vector3& lightPos, const Vector3& lightNorm, const Vector3& lightDir) = 0;
};

class NXPBRTangibleLight : public NXPBRAreaLight
{
public:
	NXPBRTangibleLight(NXPrimitive* pPrimitive, const Vector3& Radiance);

	Vector3 Emit(Ray& o_ray, Vector3& o_lightNormal, float& o_pdfPos, float& o_pdfDir) override;
	Vector3 Illuminate(const NXHit& hitInfo, Vector3& o_wi, float& o_pdf) override;

	// ����� ��������� �� Ŀ�귽�� ������ߵõ���Radianceֵ��
	// ��Ҫ�ṩ ���淢��㴦�ķ����������жϱ��ν����Ƿ�����
	Vector3 GetRadiance(const Vector3& samplePosition, const Vector3& lightSurfaceNormal, const Vector3& targetDirection) override;

	float GetPdf(const NXHit& hitInfo, const Vector3& lightPos, const Vector3& lightNorm, const Vector3& lightDir) override;

public:
	Vector3 Radiance;

private:
	NXPrimitive* m_pPrimitive;
};

class NXPBREnvironmentLight : public NXPBRAreaLight
{
public:
	NXPBREnvironmentLight(NXCubeMap* pCubeMap, const Vector3& Radiance, Vector3 WorldCenter, float WorldRadius);

	Vector3 Emit(Ray& o_ray, Vector3& o_lightNormal, float& o_pdfPos, float& o_pdfDir) override;
	Vector3 Illuminate(const NXHit& hitInfo, Vector3& o_wi, float& o_pdf) override;

	// �����Է���(emission)��radiance��
	Vector3 GetRadiance(const Vector3& samplePosition, const Vector3& lightSurfaceNormal, const Vector3& targetDirection) override;

	float GetPdf(const NXHit& hitInfo, const Vector3& lightPos, const Vector3& lightNorm, const Vector3& lightDir) override;

public:
	Vector3 Radiance;
	Vector3 WorldCenter;
	float WorldRadius;

private:
	NXCubeMap* m_pCubeMap;
};