#pragma once
#include "ShaderStructures.h"
#include "NXTransform.h"

class NSFirstPersonalCamera;

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

	const float GetZNear() { return m_near; }
	const float GetZFar() { return m_far; }

	const float GetFovY() { return m_fovY; }
	void SetFovY(const float value) { m_fovY = value; }

	const Matrix& GetViewMatrix();
	const Matrix& GetViewInverseMatrix();
	const Matrix& GetProjectionMatrix();
	const Matrix& GetProjectionInverseMatrix();
	const Matrix& GetViewProjectionMatrix();
	const Matrix& GetViewProjectionInverseMatrix();

	Ray GenerateRay(const Vector2& cursorPosition);
	Ray GenerateRay(const Vector2& cursor, const Vector2& imageSize);

	void Init(float fovY, float zNear, float zFar, Vector3 cameraPosition, Vector3 cameraLookAt, Vector3 cameraLookUp);
	void UpdateTransform();
	void Update();
	void Render();
	void Release();

	NSFirstPersonalCamera* GetFirstPersonalController();
	void SetFirstPersonalController(NSFirstPersonalCamera* pScript);

private:
	float m_fovY;
	float m_near, m_far;
	Vector3 m_at;
	Vector3 m_up;

	Matrix m_mxView;
	Matrix m_mxViewInv;
	Matrix m_mxProjection;
	Matrix m_mxProjectionInv;

	Matrix m_mxViewProjection;
	Matrix m_mxViewProjectionInv;

	// 编辑器使用的 第一人称控制(FPC)脚本指针
	NSFirstPersonalCamera* m_pEditorFPCScript;
};