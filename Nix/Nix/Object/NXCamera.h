#pragma once
#include "ShaderStructures.h"
#include "NXTransform.h"

class NXCamera : public NXTransform
{
public:
	NXCamera() = default;

	void Init(Vector3 cameraPosition, Vector3 cameraLookAt, Vector3 cameraLookUp);
	void PrevUpdate();
	void Update();
	void Render();
	void Release();

private:
	ID3D11Buffer*			m_cbCamera;
	ConstantBufferCamera	m_cbDataCamera;

	Vector3 m_at;
	Vector3 m_up;

	Matrix m_view;
	Matrix m_projection;
};