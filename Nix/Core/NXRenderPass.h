#pragma once
#include "BaseDefs/DX12.h"
#include <filesystem>
#include "Ntr.h"
#include "NXRGPass.h"
#include "NXCommonTexDefinition.h"
#include "NXTexture.h"

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

	// 如果启用autoUpdate, 记录cbv绑定的根参数的slot space.
	int cbvSpace = 0;
	int cbvSlot = 0;
};

class NXRGResource;
class NXRenderPass : public NXRGPass
{
public:
	NXRenderPass(NXRenderPassType type);
	virtual ~NXRenderPass() {}

	virtual void SetupInternal() = 0;

	void SetShaderFilePath(const std::filesystem::path& shaderFilePath) { m_shaderFilePath = shaderFilePath; }

	void SetEntryNameVS(const std::wstring& name) { m_entryNameVS = name; }
	void SetEntryNamePS(const std::wstring& name) { m_entryNamePS = name; }
	void SetEntryNameCS(const std::wstring& name) { m_entryNameCS = name; }

	virtual void Render() = 0;

	// 设置CBV。
	// rootParamIndex: 根参数的索引
	// slotIndex: 描述符表的索引，如果不提供，则和rootParamIndex相同。
	// spaceIndex: 描述符表的space索引
	// gpuVirtAddr: CBV的gpu虚拟地址
	void SetRootParamCBV(int rootParamIndex, int slotIndex, int spaceIndex, const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>* gpuVirtAddrs); 

	// 【TODO：这个在长期规划中 仅仅作为临时处理方法！】
	// 强制NXRG启用特定槽位的register b.
	// 仅用于仍耦合在上一代pass中，还没想好怎么接入NXRG的CBV，比如gbuffer的材质cbv、shadowMap的CSM级联cbv、等等
	void ForceSetRootParamCBV(int rootParamIndex, int slotIndex, int spaceIndex);

	void AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& staticSampler);
	void AddStaticSampler(D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addrUVW);

protected:
	// 初始化根参数和根参数布局；布局中的具体view数量由SetInputXX/SetOutputXX接口决定
	void InitRootParams();

protected:
	ComPtr<ID3D12RootSignature>				m_pRootSig;

	std::filesystem::path m_shaderFilePath;

	// shader入口点名称
	std::wstring m_entryNameVS;
	std::wstring m_entryNamePS;
	std::wstring m_entryNameCS;

	// 当前 Nix 的根参数（和采样器）-寄存器的布局规则：
	// 1. CBV：每个占用一个根参数
	// 2. SRV：
	//	  看这些SRV使用的最大space idx，假设为 N。放到 N 个描述符Table里，并占用 N 个根参数。
	// 3. UAV：
	//	  看这些UAV使用的最大space idx，假设为 N。放到 N 个描述符Table里，并占用 N 个根参数。
	// 4. 任何情况下都不使用根常量
	// 5. 采样器始终使用StaticSampler，暂不考虑space和动态Sampler的问题，目前够用了
	// 
	// - 提交到描述符表时，先按cbv，再srv space0，srv space1...，再uav space0，uav space1...的顺序提交。
	// 
	// pass 使用的根参数+布局
	std::vector<D3D12_ROOT_PARAMETER> m_rootParams;
	NXRGRootParamLayout m_rootParamLayout;
	std::vector<D3D12_DESCRIPTOR_RANGE> m_srvRanges; // 这俩range必须用成员变量保存
	std::vector<D3D12_DESCRIPTOR_RANGE> m_uavRanges;

	// 描述当前pass cb的输入布局
	std::vector<NXCBVManagement> m_cbvManagements;

	// pass 使用的静态采样器
	std::vector<D3D12_STATIC_SAMPLER_DESC>	m_staticSamplers;
};
