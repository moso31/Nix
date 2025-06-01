#pragma once
#include "BaseDefs/DX12.h"
#include <filesystem>
#include "Ntr.h"
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
};

class NXRenderPass
{
public:
	NXRenderPass();
	virtual ~NXRenderPass() {}

	virtual void SetupInternal() = 0;

	void SetPassName(const std::string& passName) { m_passName = passName; }

protected:
	std::string	m_passName;
};
