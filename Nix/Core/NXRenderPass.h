#pragma once
#include "BaseDefs/DX12.h"
#include <filesystem>
#include "Ntr.h"
#include "NXCommonTexDefinition.h"
#include "NXTexture.h"

// ��DX12Ҫ��CB����Ҫ�ṩ��ӦCBV��gpuHandle��
// cmdList��ʹ��gpuHandle��
struct NXCBVManagement
{
	// ���ڼ�¼ÿ֡ cmdList��ν��� cbv gpu �����ַ��
	// true: ʹ�� multiFrameGpuVirtAddr �еĵ�ַ��
	// false: �������ֶ����£����ﲻ�ùܡ�
	bool autoUpdate = false;

	// �������autoUpdate��ʹ�������gpuHandle��D3D12_GPU_VIRTUAL_ADDRESS����
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
