#pragma once
#include "NXObject.h"

class NXTransform : public NXObject
{
public:
	NXTransform();
	~NXTransform() {}

	virtual Vector3 GetTranslation();
	virtual Vector3 GetRotation();
	virtual Quaternion GetQuaternion();
	virtual Vector3 GetScale();

	virtual void SetTranslation(const Vector3& value);
	virtual void SetQuaternion(const Quaternion& value);
	virtual void SetRotation(const Vector3& value);
	virtual void SetScale(const Vector3& value);

	Matrix GetLocalMatrix();
	virtual Matrix GetWorldMatrix();
	virtual Matrix GetWorldMatrixInv();

	virtual void UpdateTransform();

protected:
	Vector3 m_translation;
	Vector3 m_eulerAngle;
	Quaternion m_rotation;
	Vector3 m_scale;

	Matrix m_localMatrix;
	Matrix m_worldMatrix;
	Matrix m_worldMatrixInv;
};