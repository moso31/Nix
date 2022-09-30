#pragma once
#include "NXTransform.h"

class NXHit;

class NXRenderableObject : public NXTransform
{
public:
	NXRenderableObject();
	~NXRenderableObject() {}

	virtual NXRenderableObject* IsRenderableObject() override { return this; }

	virtual void Release();

	virtual AABB GetAABBWorld();
	virtual AABB GetAABBLocal();

	bool RayCast(const Ray& worldRay, NXHit& outHitInfo, float& outDist);

	virtual void InitAABB();

	void SetGeoTranslation(const Vector3& value) { m_geoTranslation = value; }
	void SetGeoRotation(const Vector3& value) { m_geoRotation = value; }
	void SetGeoScale(const Vector3& value) { m_geoScale = value; }

	void UpdateTransform() override;
	void SetWorldTranslation(const Vector3& value) override;

	// 获取变换用矩阵。
	// 派生类 RenderableObject 的世界矩阵有两个：m_transformWorldMatrix 和 m_worldMatrix。
	// m_transformWorldMatrix 用于推导变换。m_worldMatrix 用于实际渲染。
	// 这么做主要是为了适配 fbx 中 额外的 geoMatrix 矩阵。
	Matrix GetParentTransformWorldMatrix();

	bool GetVisible() { return m_bIsVisible; }
	void SetVisible(bool value) { m_bIsVisible = value; }

protected:
	AABB m_localAABB;

	Vector3 m_geoTranslation;
	// 当前对象的欧拉角。旋转顺序：X-Y-Z
	Vector3 m_geoRotation;
	Vector3 m_geoScale;

	Matrix m_transformWorldMatrix;
	Matrix m_transformWorldMatrixInv;

	bool m_bIsVisible;

private:
	Matrix m_geoMatrix;
};