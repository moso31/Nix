#pragma once
#include "Header.h"
#include "ShaderStructures.h"

class NXGlobalBufferManager
{
public:
	NXGlobalBufferManager();
	~NXGlobalBufferManager();

	void Init();

	static ID3D11Buffer*						m_cbObject;
	static ConstantBufferObject					m_cbDataObject;

	static ID3D11Buffer*						m_cbCamera;
	static ConstantBufferCamera					m_cbDataCamera;

	static ID3D11Buffer*						m_cbShadowMap;
	static ConstantBufferShadowMapTransform		m_cbDataShadowMap;
};
