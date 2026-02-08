#pragma once
#include "NXTransform.h"

class NXHit;

class NXRenderableObject : public NXTransform
{
public:
	NXRenderableObject();
	virtual ~NXRenderableObject() {}

	virtual NXRenderableObject* IsRenderableObject() override { return this; }

	virtual void Release();

	virtual AABB GetAABBWorld();
	virtual AABB GetAABBLocal();

	bool RayCast(const Ray& worldRay, NXHit& outHitInfo, float& outDist);

	virtual void InitAABB();

	void SetGeoTransform(const Matrix& mxGeometry);

	void UpdateTransform() override;
	void SetWorldTranslation(const Vector3& value) override;

	virtual Matrix GetTransformWorldMatrix()	{ return m_transformWorldMatrix; }
	virtual Matrix GetTransformWorldMatrixInv() { return m_transformWorldMatrixInv; }

	bool GetVisible() { return m_bIsVisible; }
	void SetVisible(bool value) { m_bIsVisible = value; }

protected:
	AABB m_localAABB;

	// 针对FBX模型所做的适配
	// transformWorldMatrix = localMatrix * parent.transformWorldMatrix;
	Matrix m_transformWorldMatrix;
	Matrix m_transformWorldMatrixInv;

	bool m_bIsVisible;

private:
	Matrix m_geoMatrix;
	Matrix m_geoMatrixInv;
};