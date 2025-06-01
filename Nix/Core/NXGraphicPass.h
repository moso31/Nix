#pragma once
#include "NXRenderPass.h"

class NXPassTexture
{
public:
	NXPassTexture(Ntr<NXTexture> pTexture, uint32_t passSlotIndex = -1) :
		pTexture(pTexture), slotIndex(passSlotIndex) {}

	Ntr<NXTexture> pTexture;
	uint32_t slotIndex = -1;
};

class NXGraphicPass : public NXRenderPass
{
public:
	NXGraphicPass();
	virtual ~NXGraphicPass() {}

	virtual void SetupInternal() = 0;

	void SetInputTex(NXCommonTexEnum eCommonTex, uint32_t slotIndex);
	void SetInputTex(const Ntr<NXTexture>& pTex, uint32_t slotIndex);
	void SetOutputRT(const Ntr<NXTexture>& pTex, uint32_t rtIndex);
	void SetOutputDS(const Ntr<NXTexture>& pTex);

	Ntr<NXTexture> GetInputTex(uint32_t slotIndex) { return m_pInTexs[slotIndex]; }
	Ntr<NXTexture> GetOutputRT(uint32_t index) { return m_pOutRTs[index]; }
	Ntr<NXTexture> GetOutputDS() { return m_pOutDS; }

	void SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& desc);
	void SetRenderTargetMesh(const std::string& rtSubMeshName);
	void SetBlendState(const D3D12_BLEND_DESC& desc);
	void SetRasterizerState(const D3D12_RASTERIZER_DESC& desc);
	void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& desc);
	void SetSampleDescAndMask(UINT Count, UINT Quality, UINT Mask);
	void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE type);

	void SetStencilRef(const UINT stencilRef) { m_stencilRef = stencilRef; }

	// 当前 Nix Graphic Pass 的根参数（和采样器）-寄存器的布局规则：
	// 1. 每个CBV占用一个根参数
	// 2. 若存在SRV，则这些SRV都将放到一个描述符Table里。且该Table将始终占用最后一个根参数。
	// （2+. UAV 目前暂时还没涉及到，用到了再说）
	// 3. 任何情况下都不使用根常量
	// 4. 采样器始终使用StaticSampler，不考虑动态Sampler，目前够用了
	void SetRootParams(int CBVNum, int SRVUAVNum);

	// 设置CBV。
	// rootParamIndex: 根参数的索引
	// slotIndex: 描述符表的索引，如果不提供，则和rootParamIndex相同。
	// gpuVirtAddr: CBV的gpu虚拟地址
	void SetStaticRootParamCBV(int rootParamIndex, const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>* gpuVirtAddrs);
	void SetStaticRootParamCBV(int rootParamIndex, int slotIndex, const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>* gpuVirtAddrs);

	void AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& staticSampler);
	void AddStaticSampler(D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addrUVW);

	// 设置PSO
	// 在所有内容设置完毕后，再调用这个函数，创建PSO
	void InitPSO();

	virtual void RenderSetTargetAndState(ID3D12GraphicsCommandList* pCmdList);
	virtual void RenderBefore(ID3D12GraphicsCommandList* pCmdList);
	virtual void Render(ID3D12GraphicsCommandList* pCmdList);

	void Release() {}

private:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC		m_psoDesc;
	ComPtr<ID3D12PipelineState>				m_pPSO;
	ComPtr<ID3D12RootSignature>				m_pRootSig;
	UINT m_stencilRef;
	Vector2 m_viewPortSize;

	std::vector<Ntr<NXTexture>>				m_pInTexs;
	std::vector<Ntr<NXTexture>>				m_pOutRTs;
	Ntr<NXTexture>							m_pOutDS;

	// pass 使用的根参数
	std::vector<D3D12_ROOT_PARAMETER>		m_rootParams;

	// pass 使用的 srv/uav 描述符表
	std::vector<D3D12_DESCRIPTOR_RANGE>		m_srvUavRanges;

	// pass 使用的静态采样器
	std::vector<D3D12_STATIC_SAMPLER_DESC>	m_staticSamplers;

	// 描述当前pass cb的输入布局
	std::vector<NXCBVManagement>			m_cbvManagements;

	// rt 使用的 subMesh 的名称。
	// 实际渲染时根据这个名字确定使用 NXSubMeshGeometryEditor 的哪个 subMesh 作为 RT.
	// 一般不需要设置，就用默认的 "_RenderTarget" 就行，但也有特殊情况，
	// 比如渲染 CubeMap 的 Pass，需要设置为 "_CubeMapSphere"，使用一张圆形的 RT。
	std::string								m_rtSubMeshName;
};
