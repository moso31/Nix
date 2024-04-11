#pragma once
#include "BaseDefs/DX12.h"
#include "ShaderStructures.h"
#include "Ntr.h"
#include "NXBuffer.h"

struct CBufferColorMapping
{
	Vector4 param0; // x: enable
};

class NXTexture2D;
class NXColorMappingRenderer
{
public:
	NXColorMappingRenderer();
	~NXColorMappingRenderer();

	void Init();
	void Render();

	bool GetEnable() const { return m_bEnablePostProcessing; }
	void SetEnable(bool value) { m_bEnablePostProcessing = value; }

	void Release();

private:
	bool m_bEnablePostProcessing;
	Ntr<NXTexture2D>	m_pTexPassIn;
	Ntr<NXTexture2D>	m_pTexPassOut;

	NXBuffer<CBufferColorMapping> m_cbParams;

	ComPtr<ID3D12GraphicsCommandList> m_pCommandList;
	ComPtr<ID3D12PipelineState> m_pPSO;
	ComPtr<ID3D12RootSignature> m_pRootSig;
};
