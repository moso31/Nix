#pragma once
#include "NXObject.h"

class NXTransform : public NXObject
{
public:
	NXTransform();
	virtual ~NXTransform() {}

	virtual Vector3 GetTranslation();
	virtual Vector3 GetRotation();
	virtual Vector3 GetScale();

	virtual void SetTranslation(Vector3 value);
	virtual void SetRotation(Vector3 value);
	virtual void SetScale(Vector3 value);

	virtual Matrix GetWorldMatrix();
	virtual Matrix GetWorldMatrixInv();

	virtual void PrevUpdate();

protected:
	Vector3 m_translation;
	Vector3 m_rotation;
	Vector3 m_scale;

	Matrix m_worldMatrix;
	Matrix m_worldMatrixInv;
};