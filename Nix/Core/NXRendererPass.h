#pragma once
#include "BaseDefs/DX12.h"
#include <filesystem>
#include "Ntr.h"
#include "NXCommonTexDefinition.h"

struct NXRenderPassRegisterStates
{
	NXRenderPassRegisterStates()
	{
		Reset();
	}

	void Reset()
	{
		bInputLayout = true;
		bBlendState = true;
		bRasterizerState = true;
		bDepthStencilState = true;
		bSampleDescAndMask = true;
		bPrimitiveTopologyType = true;
		bShaderFilePath = false;
		inTexNum = 0;
		outRTNum = 0;
		outDS = false;
		cbvNum = 0;
		srvUavNum = 0;
	}

	bool Check()
	{
		return bInputLayout && bBlendState && bRasterizerState && bDepthStencilState && bSampleDescAndMask && bPrimitiveTopologyType && bShaderFilePath;
	}

	bool bInputLayout;
	bool bBlendState;
	bool bRasterizerState;
	bool bDepthStencilState;
	bool bSampleDescAndMask;
	bool bPrimitiveTopologyType;
	bool bShaderFilePath;
	int inTexNum;
	int outRTNum;
	bool outDS;
	int cbvNum;
	int srvUavNum;
};

class NXTexture;
class NXRendererPass
{
public:
	NXRendererPass();
	~NXRendererPass() {}

	void SetPassName(const std::string& passName) { m_passName = passName; }

	void AddInputTex(NXCommonRTEnum eCommonTex);
	void AddOutputRT(NXCommonRTEnum eCommonTex);
	void SetOutputDS(NXCommonRTEnum eCommonTex);
	void AddInputTex(const Ntr<NXTexture>& pTex);
	void AddOutputRT(const Ntr<NXTexture>& pTex);
	void SetOutputDS(const Ntr<NXTexture>& pTex);

	void SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& desc);
	void SetBlendState(const D3D12_BLEND_DESC& desc);
	void SetRasterizerState(const D3D12_RASTERIZER_DESC& desc);
	void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& desc);
	void SetSampleDescAndMask(UINT Count, UINT Quality, UINT Mask);
	void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE type);
	void SetShaderFilePath(const std::filesystem::path& shaderFilePath);

	// 2024.5.26 当前 Nix 的根参数（和采样器）-寄存器的布局规则：
	// 1. 每个CBV占用一个根参数
	// 2. 所有SRV/UAV都放到一个描述符Table里。Table存在时，将始终占用最后一个根参数。
	// 3. 任何情况下都不使用根常量
	// 4. 采样器始终使用StaticSampler，不考虑动态Sampler，目前够用了
	void SetRootParams(int CBVNum, int SRVUAVNum);
	void SetRootParamCBV(int rootParamIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuVirtAddr);
	void AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& staticSampler);
	void AddStaticSampler(D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addrUVW);

	virtual void Init() = 0;
	void OnResize();
	void Render(ID3D12GraphicsCommandList* pCmdList);

	void Release();

protected:
	void InitPSO();
	void RenderBegin(ID3D12GraphicsCommandList* pCmdList);
	void RenderEnd(ID3D12GraphicsCommandList* pCmdList);

protected:
	std::string								m_passName;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC		m_psoDesc;
	ComPtr<ID3D12PipelineState>				m_pPSO;
	ComPtr<ID3D12RootSignature>				m_pRootSig;

	std::vector<Ntr<NXTexture>>				m_pInTexs;
	std::vector<Ntr<NXTexture>>				m_pOutRTs;
	Ntr<NXTexture>							m_pOutDS;

	std::filesystem::path					m_shaderFilePath;

	std::vector<D3D12_DESCRIPTOR_RANGE>		m_srvUavRanges;
	std::vector<D3D12_ROOT_PARAMETER>		m_rootParams;
	std::vector<D3D12_STATIC_SAMPLER_DESC>	m_staticSamplers;

	std::vector<D3D12_GPU_VIRTUAL_ADDRESS>	m_cbvGpuVirtAddrs;

private:
	NXRenderPassRegisterStates				m_checkStates;
};
