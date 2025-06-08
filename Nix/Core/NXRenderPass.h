#pragma once
#include "BaseDefs/DX12.h"
#include <filesystem>
#include "Ntr.h"
#include "NXCommonTexDefinition.h"
#include "NXTexture.h"

struct NXRGRootParamLayout
{
	int cbvCount = 0;
	int srvCount = 0;
	int uavCount = 0;
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
	const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>* multiFrameGpuVirtAddr;
};

enum class NXRenderPassType
{
	GraphicPass,
	ComputePass,
};

class NXRenderPass
{
public:
	NXRenderPass(NXRenderPassType type);
	virtual ~NXRenderPass() {}

	virtual void SetupInternal() = 0;

	NXRenderPassType GetPassType() const { return m_passType; }

	void SetPassName(const std::string& passName) { m_passName = passName; }
	void SetShaderFilePath(const std::filesystem::path& shaderFilePath) { m_shaderFilePath = shaderFilePath; }

	void SetEntryNameVS(const std::wstring& name) { m_entryNameVS = name; }
	void SetEntryNamePS(const std::wstring& name) { m_entryNamePS = name; }
	void SetEntryNameCS(const std::wstring& name) { m_entryNameCS = name; }

	virtual void Render(ID3D12GraphicsCommandList* pCmdList) = 0;

	// 当前 Nix 的根参数（和采样器）-寄存器的布局规则：
	// 1. 每个CBV占用一个根参数
	// 2. 若存在SRV，则这些SRV都将放到一个描述符Table里，并占用一个根参数。
	// 3. 若存在UAV，则这些UAV也将放到一个描述符Table里，并占用一个根参数。
	// （2.3. SRV总是在UAV前面）
	// 4. 任何情况下都不使用根常量
	// 5. 采样器始终使用StaticSampler，不考虑动态Sampler，目前够用了
	virtual void SetRootParamLayout(const NXRGRootParamLayout& layout);

	// 设置CBV。
	// rootParamIndex: 根参数的索引
	// slotIndex: 描述符表的索引，如果不提供，则和rootParamIndex相同。
	// gpuVirtAddr: CBV的gpu虚拟地址
	void SetStaticRootParamCBV(int rootParamIndex, const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>* gpuVirtAddrs);
	void SetStaticRootParamCBV(int rootParamIndex, int slotIndex, const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>* gpuVirtAddrs);

	void AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& staticSampler);
	void AddStaticSampler(D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addrUVW);

protected:
	std::string	m_passName;
	NXRenderPassType m_passType;
	std::filesystem::path m_shaderFilePath;

	// shader入口点名称
	std::wstring m_entryNameVS;
	std::wstring m_entryNamePS;
	std::wstring m_entryNameCS;

	// pass 使用的静态采样器
	std::vector<D3D12_STATIC_SAMPLER_DESC>	m_staticSamplers;

	// pass 使用的根参数+布局
	std::vector<D3D12_ROOT_PARAMETER> m_rootParams;
	NXRGRootParamLayout m_rootParamLayout;

	// 描述当前pass cb的输入布局
	std::vector<NXCBVManagement> m_cbvManagements;

	// pass 使用的 srv/uav 描述符表
	// 注意Nix中 srv = 输入，uav = 输出，uav 暂时不支持输入。
	std::vector<D3D12_DESCRIPTOR_RANGE>		m_srvRanges;
	std::vector<D3D12_DESCRIPTOR_RANGE>		m_uavRanges;
};
