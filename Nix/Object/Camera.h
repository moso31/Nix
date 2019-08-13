#pragma once
#include "ShaderStructures.h"

class Camera
{
public:
	Camera() = default;

	HRESULT Init();
	void Update();
	void Render();
	void Release();

private:
	ID3D11Buffer*			m_pConstantBuffer;

	ConstantBufferCamera	m_pConstantBufferData;
};