#pragma once
#include "NXRGUtil.h"
#include "BaseDefs/DX12.h"
#include <filesystem>
#include "Ntr.h"

enum class NXRenderPassType
{
	GraphicPass,
	ComputePass,
	ReadbackBufferPass
};

class NXRGPass
{
public:
	NXRGPass(NXRenderPassType type);
	virtual ~NXRGPass() {}

	bool IsRenderPass() const { return m_passType == NXRenderPassType::GraphicPass || m_passType == NXRenderPassType::ComputePass; }
	NXRenderPassType GetPassType() const { return m_passType; }

	const std::string& GetPassName() { return m_passName; }
	void SetPassName(const std::string& passName) { m_passName = passName; }

	virtual void SetupInternal() = 0;
	virtual void Render(ID3D12GraphicsCommandList* pCmdList) = 0;

private:
	std::string	m_passName;
	NXRenderPassType m_passType;
};
