#pragma once
#include "BaseDefs/DX12.h"
#include "Ntr.h"

class NXTexture2D;
class NXBRDFLut
{
public:
	NXBRDFLut();
	~NXBRDFLut() {}

	void Init();
	void Release();

private:
	void InitVertex();
	void InitRootSignature();
	void DrawBRDFLUT();

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() { return m_pTexBRDFLUT->GetSRV(); }

private:
	const float m_mapSize = 512.0f;
	Ntr<NXTexture2D> m_pTexBRDFLUT;

	ComPtr<ID3D12GraphicsCommandList> m_pCommandList;
	ComPtr<ID3D12PipelineState> m_pPSO;
	ComPtr<ID3D12RootSignature> m_pRootSig;
};
