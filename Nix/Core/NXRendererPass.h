#pragma once
#include "BaseDefs/DX12.h"
#include <filesystem>
#include "Ntr.h"
#include "NXCommonTexDefinition.h"
#include "NXTexture.h"

// 记录Pass用到的纹理
struct NXPassTexture
{
	NXPassTexture() : pTexture(nullptr), rtType(NXCommonRTEnum::NXCommonRT_None) {}
	NXPassTexture(const Ntr<NXTexture>& pTex) : pTexture(pTex), rtType(NXCommonRTEnum::NXCommonRT_None) {}
	NXPassTexture(const Ntr<NXTexture>& pTex, NXCommonRTEnum eCommonTex) : pTexture(pTex), rtType(eCommonTex) {}

	NXTexture* operator->() { return pTexture.Ptr(); }

	bool IsValid() { return pTexture.IsValid(); }
	bool IsNull() { return pTexture.IsNull(); }
	bool IsCommonRT() { return rtType != NXCommonRT_None; }

	// 纹理指针，可能是 RT 类型，也可能是自定义的任意 tex
	Ntr<NXTexture> pTexture;
	
	// 纹理是否是通用RT，如果是，在这里记录一下
	// OnResize() 依赖这个参数
	NXCommonRTEnum rtType;
};

// 在DX12要绑定CB，需要提供对应CBV的gpuHandle。
// cmdList将使用gpuHandle。
struct NXCBVManagement
{
	// 用于记录每帧 cmdList如何接收 cbv gpu 虚拟地址。
	// true: 使用 multiFrameGpuVirtAddr 中的地址；
	// false: 派生类手动更新，这里不用管。
	bool autoUpdate = false;

	// 如果启用autoUpdate，使用这里的gpuHandle（D3D12_GPU_VIRTUAL_ADDRESS）。
	MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS> multiFrameGpuVirtAddr;
};

class NXRendererPass
{
public:
	NXRendererPass();
	~NXRendererPass() {}

	void SetPassName(const std::string& passName) { m_passName = passName; }

	void AddInputTex(NXCommonRTEnum eCommonTex);
	void AddInputTex(NXCommonTexEnum eCommonTex);
	void AddInputTex(const Ntr<NXTexture>& pTex);
	void AddOutputRT(NXCommonRTEnum eCommonTex);
	void AddOutputRT(const Ntr<NXTexture>& pTex);
	void SetOutputDS(NXCommonRTEnum eCommonTex);
	void SetOutputDS(const Ntr<NXTexture>& pTex);

	void SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& desc);
	void SetRenderTargetMesh(const std::string& rtSubMeshName);
	void SetBlendState(const D3D12_BLEND_DESC& desc);
	void SetRasterizerState(const D3D12_RASTERIZER_DESC& desc);
	void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& desc);
	void SetSampleDescAndMask(UINT Count, UINT Quality, UINT Mask);
	void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE type);
	void SetShaderFilePath(const std::filesystem::path& shaderFilePath);

	void SetStencilRef(const UINT stencilRef) { m_stencilRef = stencilRef; }

	// 2024.5.26 当前 Nix 的根参数（和采样器）-寄存器的布局规则：
	// 1. 每个CBV占用一个根参数
	// 2. 若存在SRV，则这些SRV都将放到一个描述符Table里。且该Table将始终占用最后一个根参数。
	// （2+. UAV 目前暂时还没涉及到，用到了再说）
	// 3. 任何情况下都不使用根常量
	// 4. 采样器始终使用StaticSampler，不考虑动态Sampler，目前够用了
	void SetRootParams(int CBVNum, int SRVUAVNum);

	// 设置静态CBV。
	// 注意：对一个Pass，如果设置成静态CBV，那么这个CBV的映射地址整个Pass的生命周期内不会改变。
	// 应该根据Pass实际情况决定是否设置，而不是盲目全部设置。
	// rootParamIndex: 根参数的索引
	// slotIndex: 描述符表的索引，如果不提供，则和rootParamIndex相同。
	// gpuVirtAddr: CBV的gpu虚拟地址
	void SetStaticRootParamCBV(int rootParamIndex, const std::vector<D3D12_GPU_VIRTUAL_ADDRESS>& gpuVirtAddrs);
	void SetStaticRootParamCBV(int rootParamIndex, int slotIndex, const std::vector<D3D12_GPU_VIRTUAL_ADDRESS>& gpuVirtAddr);

	void AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& staticSampler);
	void AddStaticSampler(D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addrUVW);

	virtual void Init() = 0;

	// OnResize 会在窗口大小变化时被调用
	// 用于更新Pass 关联的 RT的引用状态
	void OnResize();

	void RenderBefore(ID3D12GraphicsCommandList* pCmdList);
	void Render(ID3D12GraphicsCommandList* pCmdList);

	void Release() {}

protected:
	void InitPSO();

private:
	std::string	m_passName;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC		m_psoDesc;
	ComPtr<ID3D12PipelineState>				m_pPSO;
	ComPtr<ID3D12RootSignature>				m_pRootSig;
	UINT m_stencilRef;

	std::vector<NXPassTexture>				m_pInTexs;
	std::vector<NXPassTexture>				m_pOutRTs;
	NXPassTexture							m_pOutDS;

	std::filesystem::path					m_shaderFilePath;

	// pass 使用的根参数
	std::vector<D3D12_ROOT_PARAMETER>		m_rootParams;

	// pass 使用的 srv/uav 描述符表
	std::vector<D3D12_DESCRIPTOR_RANGE>		m_srvUavRanges;

	// pass 使用的静态采样器
	std::vector<D3D12_STATIC_SAMPLER_DESC>	m_staticSamplers;

	// Pass总是需要开发者描述这个Pass依赖哪些CB。
	std::vector<NXCBVManagement>			m_cbvManagements;

	// rt 使用的 subMesh 的名称。
	// 实际渲染时根据这个名字确定使用 NXSubMeshGeometryEditor 的哪个 subMesh 作为 RT.
	// 一般不需要设置，就用默认的 "_RenderTarget" 就行，但也有特殊情况，
	// 比如渲染 CubeMap 的 Pass，需要设置为 "_CubeMapSphere"，使用一张圆形的 RT。
	std::string								m_rtSubMeshName;
};
