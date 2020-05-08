#pragma once
#include "ShaderStructures.h"
#include "NXTransform.h"

class NXCamera : public NXTransform
{
public:
	NXCamera();
	~NXCamera() {}

	void SetTranslation(Vector3 value);
	void SetRotation(Quaternion value);
	void SetLookAt(Vector3 value);

	Vector3 GetForward();
	Vector3 GetLeft();
	Vector3 GetRight();
	Vector3 GetAt();
	Vector3 GetUp();

	Ray GenerateRay(Vector2 cursorPosition);

	void Init(float zNear, float zFar, Vector3 cameraPosition, Vector3 cameraLookAt, Vector3 cameraLookUp);
	void UpdateTransform();
	void Update();
	void Render();
	void Release();

private:
	float m_near, m_far;
	Vector3 m_at;
	Vector3 m_up;

	Matrix m_view;
	Matrix m_projection;
};