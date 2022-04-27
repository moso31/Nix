#pragma once
#include "NXTransform.h"

class NXHit;

class NXRenderableObject : public NXTransform
{
public:
	NXRenderableObject();
	~NXRenderableObject() {}

	virtual void Release();

	virtual AABB GetAABBWorld();
	virtual AABB GetAABBLocal();

	virtual bool RayCast(const Ray& worldRay, NXHit& outHitInfo, float& outDist) = 0;

	virtual void InitAABB() = 0;

	void SetGeoTranslation(const Vector3& value) { m_geoTranslation = value; }
	void SetGeoRotation(const Vector3& value) { m_geoRotation = value; }
	void SetGeoScale(const Vector3& value) { m_geoScale = value; }

	void SetRotationMatrix(const Matrix& m) { m_rotMatrix = m; }

	void UpdateTransform() override;

protected:
	AABB m_aabb;
	Vector3 m_geoTranslation;
	// 当前对象的欧拉角。旋转顺序：X-Y-Z
	Vector3 m_geoRotation;
	Vector3 m_geoScale;

	Matrix m_rotMatrix;
};