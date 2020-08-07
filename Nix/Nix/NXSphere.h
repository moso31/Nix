#pragma once
#include "NXPrimitive.h"

class NXSphere : public NXPrimitive
{
public:
	NXSphere();
	~NXSphere() {}

	void Init(float radius, int segmentHorizontal, int segmentVertical);

	virtual void UpdateSurfaceAreaInfo() override;
	virtual float GetSurfaceArea() override;

	virtual bool RayCast(const Ray& worldRay, NXHit& outHitInfo, float& outDist) override;

	// ���ڱ������Primitive�������
	virtual void SampleForArea(Vector3& o_pos, Vector3& o_norm, float& o_pdfA) override;
	virtual float GetPdfArea() override { return 1.0f / GetSurfaceArea(); }

	// ��������Ƕ�Primitive�������
	virtual void SampleForSolidAngle(const NXHit& hitInfo, Vector3& o_pos, Vector3& o_norm, float& o_pdfW) override;
	virtual float GetPdfSolidAngle(const NXHit& hitInfo, const Vector3& posLight, const Vector3& normLight, const Vector3& dirLight) override;

private:
	float m_radius;
	UINT m_segmentVertical;
	UINT m_segmentHorizontal;
};
