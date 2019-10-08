#pragma once
#include "Header.h"
#include "ShaderStructures.h"

class NXGlobalBufferManager
{
public:
	NXGlobalBufferManager();
	~NXGlobalBufferManager();

	void Init();

	static ID3D11Buffer*			m_cbWorld;
	static ConstantBufferPrimitive	m_cbDataWorld;

	static ID3D11Buffer*			m_cbCamera;
	static ConstantBufferCamera		m_cbDataCamera;
};
