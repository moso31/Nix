#pragma once
#include "BaseDefs/DX12.h"
#include "NXRendererPass.h"
#include "NXBuffer.h"

class NXComputePass : public NXRendererPass
{
public:
	NXComputePass();
	virtual ~NXComputePass() {}

	virtual void SetupInternal() = 0;

	// 当前 Nix Compute Pass 的根参数（和采样器）-寄存器的布局规则：
	// 1. 每个CBV占用一个根参数
	// 2. 若存在SRV，则这些SRV都将放到一个描述符Table里，并占用一个根参数。
	// 3. 若存在UAV，则这些UAV也将放到一个描述符Table里，并占用一个根参数。
	// （2.3. SRV总是在UAV前面）
	// 4. 任何情况下都不使用根常量
	// 5. 采样器始终使用StaticSampler，不考虑动态Sampler，目前够用了
	void SetRootParams(int CBVNum, int SRVNum, int UAVNum);

	void SetInputTex(NXCommonTexEnum eCommonTex, uint32_t slotIndex);
	void SetInputTex(const Ntr<NXTexture>& pTex, uint32_t slotIndex);
	void SetOutputTex(const Ntr<NXTexture>& pTex, uint32_t slotIndex);
	void SetInputBuffer(const Ntr<NXBuffer>& pBuffer, uint32_t slotIndex);
	void SetOutputBuffer(const Ntr<NXBuffer>& pBuffer, uint32_t slotIndex);

private:
	D3D12_COMPUTE_PIPELINE_STATE_DESC m_csoDesc;

	std::vector<Ntr<NXResource>>			m_pInRes;
	std::vector<Ntr<NXResource>>			m_pOutRes;

	// pass 使用的根参数
	std::vector<D3D12_ROOT_PARAMETER>		m_rootParams;

	// pass 使用的 srv/uav 描述符表
	std::vector<D3D12_DESCRIPTOR_RANGE>		m_srvRanges;
	std::vector<D3D12_DESCRIPTOR_RANGE>		m_uavRanges;

	// pass 使用的静态采样器
	std::vector<D3D12_STATIC_SAMPLER_DESC>	m_staticSamplers;

	// Pass总是需要开发者描述这个Pass依赖哪些CB。
	std::vector<NXCBVManagement>			m_cbvManagements;
};