#pragma once
#include "BaseDefs/DX11.h"
#include "ShaderStructures.h"

class NXGlobalBufferManager
{
public:
	static void Init();

	static ComPtr<ID3D11Buffer>					m_cbObject;
	static ConstantBufferObject					m_cbDataObject;

	static ComPtr<ID3D11Buffer>					m_cbCamera;
	static ConstantBufferCamera					m_cbDataCamera;

	static ComPtr<ID3D11Buffer>					m_cbShadowTest;
	static ConstantBufferShadowTest				m_cbDataShadowTest;
};

class NXGlobalInputLayout
{
public:
	static void Init();

	static D3D11_INPUT_ELEMENT_DESC layoutP[1];
	static D3D11_INPUT_ELEMENT_DESC layoutPT[2];
	static D3D11_INPUT_ELEMENT_DESC layoutPNT[3];
	static D3D11_INPUT_ELEMENT_DESC layoutPNTT[4];
	static D3D11_INPUT_ELEMENT_DESC	layoutEditorObject[2];
};