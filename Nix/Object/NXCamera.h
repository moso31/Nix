#pragma once
#include "ShaderStructures.h"

class NXCamera
{
public:
	NXCamera() = default;

	HRESULT Init();
	void Update();
	void Render();
	void Release();

private:
	ID3D11Buffer*			m_pConstantBuffer;

	ConstantBufferCamera	m_pConstantBufferData;
};