#pragma once
#include "Header.h"

class NXTransform
{
public:
	NXTransform();

	Vector3 GetTranslation();
	Vector3 GetRotation();
	Vector3 GetScale();

	void SetTranslation(Vector3 value);
	void SetRotation(Vector3 value);
	void SetScale(Vector3 value);

	Matrix GetWorldMatrix();
	Matrix GetWorldMatrixInv();

	void PrevUpdate();

protected:
	Vector3 m_translation;
	Vector3 m_rotation;
	Vector3 m_scale;

	Matrix m_worldMatrix;
	Matrix m_worldMatrixInv;
};