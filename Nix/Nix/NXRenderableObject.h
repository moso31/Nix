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

	// ��ȡ�任�þ���
	// ������ RenderableObject �����������������m_transformWorldMatrix �� m_worldMatrix��
	// m_transformWorldMatrix �����Ƶ��任��m_worldMatrix ����ʵ����Ⱦ��
	// ��ô����Ҫ��Ϊ������ fbx �� ����� geoMatrix ����
	Matrix GetParentTransformWorldMatrix();

	bool GetVisible() { return m_bIsVisible; }
	void SetVisible(bool value) { m_bIsVisible = value; }

protected:
	AABB m_localAABB;

	Vector3 m_geoTranslation;
	// ��ǰ�����ŷ���ǡ���ת˳��X-Y-Z
	Vector3 m_geoRotation;
	Vector3 m_geoScale;

	Matrix m_transformWorldMatrix;
	Matrix m_transformWorldMatrixInv;

	bool m_bIsVisible;

private:
	Matrix m_geoMatrix;
};