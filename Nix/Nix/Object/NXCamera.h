#pragma once
#include "ShaderStructures.h"
#include "NXTransform.h"

class NXCamera : public NXTransform
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