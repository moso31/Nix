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

	virtual void Render(ID3D12GraphicsCommandList* pCmdList) = 0;

protected:
	std::string	m_passName;
	NXRenderPassType m_passType;
	std::filesystem::path m_shaderFilePath;
};
