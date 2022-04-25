#pragma once
#include "ShaderStructures.h"
#include "NXTransform.h"

class NXCamera : public NXTransform
{
public:
	NXCamera();
	~NXCamera() {}

	void SetTranslation(const Vector3& value) override;
	void SetRotation(const Vector3& value) override;

	void SetLookAt(Vector3 value);

	Vector3 GetForward();
	Vector3 GetLeft();
	Vector3 GetRight();
	Vector3 GetAt();
	Vector3 GetUp();

	const Matrix& GetViewMatrix();
	const Matrix& GetProjectionMatrix();

	Ray GenerateRay(const Vector2& cursorPosition);
	Ray GenerateRay(const Vector2& cursor, const Vector2& imageSize);

	void Init(float fovY, float zNear, float zFar, Vector3 cameraPosition, Vector3 cameraLookAt, Vector3 cameraLookUp);
	void UpdateTransform();
	void Update();
	void Render();
	void Release();

private:
	float m_fovY;
	float m_near, m_far;
	Vector3 m_at;
	Vector3 m_up;

	Matrix m_view;
	Matrix m_projection;
};