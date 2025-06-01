#pragma once
#include "NXRenderPass.h"
#include "NXBuffer.h"

class NXComputePass : public NXRenderPass
{
public:
	NXComputePass();
	virtual ~NXComputePass() {}

	virtual void SetupInternal() = 0;

	void InitCSO();

	void SetThreadGroups(uint32_t threadGroupX, uint32_t threadGroupY = 1, uint32_t threadGroupZ = 1);

	void SetInputTex(NXCommonTexEnum eCommonTex, uint32_t slotIndex);
	void SetInputTex(const Ntr<NXTexture>& pTex, uint32_t slotIndex);
	void SetOutputTex(const Ntr<NXTexture>& pTex, uint32_t slotIndex);
	void SetInputBuffer(const Ntr<NXBuffer>& pBuffer, uint32_t slotIndex);
	void SetOutputBuffer(const Ntr<NXBuffer>& pBuffer, uint32_t slotIndex);

	virtual void RenderSetTargetAndState(ID3D12GraphicsCommandList* pCmdList);
	virtual void RenderBefore(ID3D12GraphicsCommandList* pCmdList);
	virtual void Render(ID3D12GraphicsCommandList* pCmdList);

	// 当前 Nix Compute Pass 的根参数（和采样器）-寄存器的布局规则：
	// 1. 每个CBV占用一个根参数
	// 2. 若存在SRV，则这些SRV都将放到一个描述符Table里，并占用一个根参数。
	// 3. 若存在UAV，则这些UAV也将放到一个描述符Table里，并占用一个根参数。
	// （2.3. SRV总是在UAV前面）
	// 4. 任何情况下都不使用根常量
	// 5. 采样器始终使用StaticSampler，不考虑动态Sampler，目前够用了
	void SetRootParams(int CBVNum, int SRVNum, int UAVNum);

	// 设置CBV。
	// rootParamIndex: 根参数的索引
	// slotIndex: 描述符表的索引，如果不提供，则和rootParamIndex相同。
	// gpuVirtAddr: CBV的gpu虚拟地址
	void SetStaticRootParamCBV(int rootParamIndex, const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>* gpuVirtAddrs);
	void SetStaticRootParamCBV(int rootParamIndex, int slotIndex, const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>* gpuVirtAddrs);

	void AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& staticSampler);
	void AddStaticSampler(D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addrUVW);

private:
	D3D12_COMPUTE_PIPELINE_STATE_DESC		m_csoDesc;
	ComPtr<ID3D12PipelineState>				m_pCSO;
	ComPtr<ID3D12RootSignature>				m_pRootSig;

	uint32_t 								m_threadGroupX;
	uint32_t 								m_threadGroupY;
	uint32_t 								m_threadGroupZ;

	std::vector<Ntr<NXResource>>			m_pInRes;
	std::vector<Ntr<NXResource>>			m_pOutRes;

	std::filesystem::path					m_shaderFilePath;

	// pass 使用的根参数
	std::vector<D3D12_ROOT_PARAMETER>		m_rootParams;

	// pass 使用的 srv/uav 描述符表
	// 注意Nix中 srv = 输入，uav = 输出，uav 暂时不支持输入。
	std::vector<D3D12_DESCRIPTOR_RANGE>		m_srvRanges; 
	std::vector<D3D12_DESCRIPTOR_RANGE>		m_uavRanges;

	// pass 使用的静态采样器
	std::vector<D3D12_STATIC_SAMPLER_DESC>	m_staticSamplers;

	// Pass总是需要开发者描述这个Pass依赖哪些CB。
	std::vector<NXCBVManagement>			m_cbvManagements;
};